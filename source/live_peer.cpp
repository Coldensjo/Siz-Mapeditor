//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_peer.h"
#include "live_server.h"
#include "live_tab.h"
#include "live_action.h"
#include "live_assets.h"
#include "net_connection.h"

#include "editor.h"
#include "gui.h"

#include <iostream>
#include <vector>

static void livePeerLog(LiveLogTab* log, const wxString& message) {
	std::cout << "[live] " << message.ToStdString() << std::endl;
	std::cout.flush();
	if (log) {
		wxTheApp->CallAfter([log, message]() {
			log->Message(message);
		});
	}
}

LivePeer::LivePeer(LiveServer* server, asio::ip::tcp::socket socket) :
	LiveSocket(),
	readMessage(), server(server), socket(std::move(socket)), color(), id(0), clientId(0), connected(false),
	assetFiles(), assetFileIndex(0), assetStream(), assetBytesRemaining(0) {
	ASSERT(server != nullptr);
	mapVersion = server->getEditor()->getMap().getVersion();
}

LivePeer::~LivePeer() {
	if (socket.is_open()) {
		socket.close();
	}
}

void LivePeer::close() {
	if (socket.is_open()) {
		net_error_code ec;
		socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		socket.close(ec);
	}
	server->removeClient(id);
}

bool LivePeer::handleError(const net_error_code& error) {
	if (error == asio::error::eof || error == asio::error::connection_reset) {
		livePeerLog(log, wxString() + getHostName() + ": disconnected.");
		close();
		return true;
	} else if (error == asio::error::connection_aborted) {
		livePeerLog(log, name + " have left the server.");
		return true;
	}
	return false;
}

std::string LivePeer::getHostName() const {
	net_error_code error;
	auto endpoint = socket.remote_endpoint(error);
	if (error) {
		return "unknown";
	}
	return endpoint.address().to_string();
}

void LivePeer::receiveHeader() {
	readMessage.clear();
	readMessage.position = 0;
	asio::async_read(socket,
		asio::buffer(readMessage.buffer, 4),
		[this](const net_error_code& error, size_t bytesReceived) -> void {
			if (error) {
				if (!handleError(error)) {
					livePeerLog(log, wxString() + getHostName() + ": " + error.message());
				}
			} else if (bytesReceived < 4) {
				livePeerLog(log, wxString() + getHostName() + ": Could not receive header[size: " + std::to_string(bytesReceived) + "], disconnecting client.");
			} else {
				const uint32_t packetSize = readMessage.read<uint32_t>();
				receive(packetSize);
			}
		});
}

void LivePeer::receive(uint32_t packetSize) {
	readMessage.buffer.resize(readMessage.position + packetSize);
	asio::async_read(socket,
		asio::buffer(&readMessage.buffer[readMessage.position], packetSize),
		[this](const net_error_code& error, size_t bytesReceived) -> void {
			if (error) {
				if (!handleError(error)) {
					livePeerLog(log, wxString() + getHostName() + ": " + error.message());
				}
			} else if (bytesReceived < readMessage.buffer.size() - 4) {
				livePeerLog(log, wxString() + getHostName() + ": Could not receive packet[size: " + std::to_string(bytesReceived) + "], disconnecting client.");
			} else {
				NetworkMessage packet = std::move(readMessage);
				if (connected) {
					if (g_gui.IsHeadless()) {
						parseEditorPacket(std::move(packet));
					} else {
						wxTheApp->CallAfter([this, packet = std::move(packet)]() mutable {
							parseEditorPacket(std::move(packet));
						});
					}
				} else {
					// Login handshake runs on the network thread so the headless
					// MapServer does not depend on wx CallAfter dispatch.
					parseLoginPacket(std::move(packet));
				}
				NetworkConnection::getInstance().post([this]() {
					receiveHeader();
				});
			}
		});
}

void LivePeer::send(NetworkMessage& message) {
	send(message, nullptr);
}

void LivePeer::send(NetworkMessage& message, std::function<void()> onSent) {
	NetworkMessage outbound;
	outbound.buffer = message.buffer;
	outbound.position = message.position;
	outbound.size = message.size;

	NetworkConnection::getInstance().dispatch([this, outbound = std::move(outbound), onSent = std::move(onSent)]() mutable {
		memcpy(&outbound.buffer[0], &outbound.size, 4);
		asio::async_write(socket,
			asio::buffer(outbound.buffer, outbound.size + 4),
			[this, onSent = std::move(onSent)](const net_error_code& error, size_t bytesTransferred) -> void {
				if (error) {
					livePeerLog(log, wxString() + getHostName() + ": " + error.message());
				} else if (onSent) {
					onSent();
				}
			});
	});
}

void LivePeer::parseLoginPacket(NetworkMessage message) {
	uint8_t packetType;
	while (message.position < message.buffer.size()) {
		packetType = message.read<uint8_t>();
		switch (packetType) {
			case PACKET_HELLO_FROM_CLIENT:
				parseHello(message);
				break;
			case PACKET_READY_CLIENT:
				parseReady(message);
				break;
			default: {
				livePeerLog(log, "Invalid login packet received, connection severed.");
				close();
				break;
			}
		}
	}
}

void LivePeer::parseEditorPacket(NetworkMessage message) {
	uint8_t packetType;
	while (message.position < message.buffer.size()) {
		packetType = message.read<uint8_t>();
		switch (packetType) {
			case PACKET_REQUEST_NODES:
				parseNodeRequest(message);
				break;
			case PACKET_CHANGE_LIST:
				parseReceiveChanges(message);
				break;
			case PACKET_CLIENT_UPDATE_CURSOR:
				parseCursorUpdate(message);
				break;
			case PACKET_COMMENT_ADD:
				parseCommentAdd(message);
				break;
			case PACKET_COMMENT_REMOVE:
				parseCommentRemove(message);
				break;
			default: {
				livePeerLog(log, "Invalid editor packet received, connection severed.");
				close();
				break;
			}
		}
	}
}

void LivePeer::parseHello(NetworkMessage& message) {
	if (connected) {
		close();
		return;
	}

	uint32_t rmeVersion = message.read<uint32_t>();
	if (rmeVersion != __RME_VERSION_ID__) {
		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Wrong editor version.");

		send(outMessage);
		close();
		return;
	}

	uint32_t netVersion = message.read<uint32_t>();
	if (netVersion != __LIVE_NET_VERSION__) {
		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Wrong protocol version.");

		send(outMessage);
		close();
		return;
	}

	(void)message.read<uint32_t>(); // client-reported version; server assets are authoritative
	std::string nickname = message.read<std::string>();
	std::string password = message.read<std::string>();

	if (server->getPassword() != wxString(password.c_str(), wxConvUTF8)) {
		livePeerLog(log, "Client tried to connect, but used the wrong password, connection refused.");

		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Wrong password.");
		send(outMessage);
		close();
		return;
	}

	name = wxString(nickname.c_str(), wxConvUTF8);
	livePeerLog(log, name + " (" + getHostName() + ") connected.");

	NetworkMessage outMessage;
	outMessage.write<uint8_t>(PACKET_CHANGE_CLIENT_VERSION);
	outMessage.write<uint32_t>(g_gui.GetCurrentVersionID());

	wxString assetError;
	if (!collectServerAssetFiles(assetFiles, assetError)) {
		livePeerLog(log, "Asset sync failed: " + assetError);

		NetworkMessage kickMessage;
		kickMessage.write<uint8_t>(PACKET_KICK);
		kickMessage.write<std::string>(nstr(assetError));
		send(kickMessage);
		close();
		return;
	}

	outMessage.write<uint32_t>(static_cast<uint32_t>(assetFiles.size()));
	for (const LiveAssetFile& file : assetFiles) {
		outMessage.write<uint8_t>(file.kind);
		outMessage.write<std::string>(file.filename);
		outMessage.write<uint32_t>(file.size);
	}

	send(outMessage, [this]() {
		startAssetTransfer();
	});
}

void LivePeer::startAssetTransfer() {
	assetFileIndex = 0;
	sendNextAssetChunk();
}

void LivePeer::sendNextAssetChunk() {
	if (assetFileIndex >= assetFiles.size()) {
		NetworkMessage doneMessage;
		doneMessage.write<uint8_t>(PACKET_ASSET_FILES_DONE);
		send(doneMessage);
		return;
	}

	if (!assetStream.is_open()) {
		const LiveAssetFile& file = assetFiles[assetFileIndex];
		assetStream.open(nstr(file.sourcePath.GetFullPath()), std::ios::binary);
		if (!assetStream.is_open()) {
			livePeerLog(log, "Failed to open asset file: " + file.sourcePath.GetFullPath());
			close();
			return;
		}

		assetBytesRemaining = file.size;

		NetworkMessage beginMessage;
		beginMessage.write<uint8_t>(PACKET_ASSET_FILE_BEGIN);
		beginMessage.write<uint8_t>(file.kind);
		beginMessage.write<std::string>(file.filename);
		beginMessage.write<uint32_t>(file.size);
		send(beginMessage);
	}

	std::vector<char> buffer(LIVE_ASSET_CHUNK_SIZE);
	const size_t toRead = std::min<size_t>(LIVE_ASSET_CHUNK_SIZE, assetBytesRemaining);
	assetStream.read(buffer.data(), static_cast<std::streamsize>(toRead));
	const std::streamsize bytesRead = assetStream.gcount();
	if (bytesRead <= 0) {
		livePeerLog(log, "Failed while reading asset file for transfer.");
		close();
		return;
	}

	std::string chunk(buffer.data(), static_cast<size_t>(bytesRead));
	assetBytesRemaining -= static_cast<uint32_t>(bytesRead);

	NetworkMessage chunkMessage;
	chunkMessage.write<uint8_t>(PACKET_ASSET_FILE_CHUNK);
	chunkMessage.write<std::string>(chunk);
	send(chunkMessage, [this]() {
		if (assetBytesRemaining == 0) {
			if (assetStream.is_open()) {
				assetStream.close();
			}

			NetworkMessage endMessage;
			endMessage.write<uint8_t>(PACKET_ASSET_FILE_END);
			send(endMessage, [this]() {
				++assetFileIndex;
				sendNextAssetChunk();
			});
		} else {
			sendNextAssetChunk();
		}
	});
}

void LivePeer::parseReady(NetworkMessage& message) {
	if (connected) {
		close();
		return;
	}

	connected = true;

	// Find free client id
	clientId = server->getFreeClientId();
	if (clientId == 0) {
		NetworkMessage outMessage;
		outMessage.write<uint8_t>(PACKET_KICK);
		outMessage.write<std::string>("Server is full.");

		send(outMessage);
		close();
		return;
	}

	setUsedColor(LiveSocket::colorForClientId(clientId));

	server->updateClientList();

	// Let's reply
	NetworkMessage outMessage;
	outMessage.write<uint8_t>(PACKET_HELLO_FROM_SERVER);

	const Map& map = server->getEditor()->getMap();
	outMessage.write<std::string>(map.getName());
	outMessage.write<uint16_t>(map.getWidth());
	outMessage.write<uint16_t>(map.getHeight());
	outMessage.write<uint32_t>(clientId);
	outMessage.write<uint8_t>(color.Red());
	outMessage.write<uint8_t>(color.Green());
	outMessage.write<uint8_t>(color.Blue());
	outMessage.write<uint8_t>(color.Alpha());

	server->writeSessionBounds(outMessage);

	send(outMessage);
	server->sendCommentList(this);
}

void LivePeer::parseNodeRequest(NetworkMessage& message) {
	Map& map = server->getEditor()->getMap();
	const LiveSessionBounds& bounds = server->getSessionBounds();
	for (uint32_t nodes = message.read<uint32_t>(); nodes != 0; --nodes) {
		uint32_t ind = message.read<uint32_t>();

		int32_t ndx = ind >> 18;
		int32_t ndy = (ind >> 4) & 0x3FFF;
		bool underground = ind & 1;

		if (bounds.enabled && !bounds.intersectsLeaf(ndx * 4, ndy * 4)) {
			sendNode(clientId, nullptr, ndx, ndy, underground ? 0xFF00 : 0x00FF);
			continue;
		}

		QTreeNode* node = map.createLeaf(ndx * 4, ndy * 4);
		if (node) {
			sendNode(clientId, node, ndx, ndy, underground ? 0xFF00 : 0x00FF);
		}
	}
}

void LivePeer::parseReceiveChanges(NetworkMessage& message) {
	Editor& editor = *server->getEditor();

	const std::string data = message.read<std::string>();
	std::vector<uint8_t> buffer(data.begin(), data.end());
	buffer.insert(buffer.begin(), 0); // Leading byte skipped by OTBM node reader
	mapReader.assign(buffer.data(), buffer.size());

	BinaryNode* rootNode = mapReader.getRootNode();
	BinaryNode* tileNode = rootNode->getChild();

	NetworkedAction* action = static_cast<NetworkedAction*>(editor.createAction(ACTION_REMOTE));
	action->owner = clientId;

	if (tileNode) {
		const LiveSessionBounds& bounds = server->getSessionBounds();
		do {
			Tile* tile = readTile(tileNode, editor, nullptr);
			if (tile) {
				if (!bounds.enabled || bounds.contains(tile->getX(), tile->getY())) {
					action->addChange(newd Change(tile));
				} else {
					delete tile;
				}
			}
		} while (tileNode->advance());
	}
	mapReader.close();

	DirtyList dirty_list;
	dirty_list.owner = clientId;
	if (action->size() > 0) {
		action->commit(&dirty_list);
		editor.getMap().doChange();
	}
	delete action;

	if (!dirty_list.Empty()) {
		server->broadcastNodes(dirty_list);
	}

	if (!g_gui.IsHeadless()) {
		g_gui.RefreshView();
		g_gui.UpdateMinimap();
	}
}

void LivePeer::parseCursorUpdate(NetworkMessage& message) {
	LiveCursor cursor = readCursor(message);
	cursor.id = clientId;
	cursor.color = color;

	server->broadcastCursor(cursor);
	if (!g_gui.IsHeadless()) {
		g_gui.RefreshView();
	}
}

void LivePeer::parseCommentAdd(NetworkMessage& message) {
	const uint16_t x = message.read<uint16_t>();
	const uint16_t y = message.read<uint16_t>();
	const uint8_t z = message.read<uint8_t>();
	const std::string text = message.read<std::string>();
	if (text.empty()) {
		return;
	}

	const LiveSessionBounds& bounds = server->getSessionBounds();
	if (bounds.enabled && !bounds.contains(x, y)) {
		return;
	}

	Editor& editor = *server->getEditor();
	MapComment comment = editor.getMap().getComments().createComment(
		Position(x, y, z),
		nstr(name),
		text
	);
	editor.getMap().doChange();
	server->broadcastComment(comment);

	if (!g_gui.IsHeadless()) {
		g_gui.RefreshView();
	}
}

void LivePeer::parseCommentRemove(NetworkMessage& message) {
	const uint32_t commentId = message.read<uint32_t>();

	Editor& editor = *server->getEditor();
	if (!editor.getMap().getComments().removeById(commentId)) {
		return;
	}

	editor.getMap().doChange();
	server->broadcastCommentRemoved(commentId);

	if (!g_gui.IsHeadless()) {
		g_gui.RefreshView();
	}
}
