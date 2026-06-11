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

#include "live_server.h"
#include "live_peer.h"
#include "live_tab.h"
#include "live_action.h"

#include "editor.h"
#include "settings.h"
#include "gui.h"

#include <iostream>

// Logs to both the optional GUI log tab (when hosted in the editor) and the
// console (so the standalone server can report activity).
static void liveServerLog(LiveLogTab* log, const wxString& message) {
	std::cout << "[live] " << message.ToStdString() << std::endl;
	std::cout.flush();
	if (log) {
		wxTheApp->CallAfter([log, message]() {
			log->Message(message);
		});
	}
}

LiveServer::LiveServer(Editor& editor) :
	LiveSocket(),
	clients(), acceptor(nullptr), editor(&editor),
	clientIds(0), port(0), stopped(false) {
	mapVersion = editor.getMap().getVersion();
}

LiveServer::~LiveServer() {
	//
}

bool LiveServer::bind() {
	NetworkConnection& connection = NetworkConnection::getInstance();
	if (!connection.start()) {
		setLastError("The previous connection has not been terminated yet.");
		return false;
	}

	auto& service = connection.get_service();
	acceptor = std::make_shared<asio::ip::tcp::acceptor>(service);

	asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port);
	acceptor->open(endpoint.protocol());

	net_error_code error;
	acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true), error);
	if (error) {
		setLastError("Error: " + error.message());
		return false;
	}

	acceptor->bind(endpoint, error);
	if (error) {
		setLastError("Error: " + error.message());
		return false;
	}
	acceptor->listen(asio::socket_base::max_listen_connections, error);
	if (error) {
		setLastError("Error: " + error.message());
		return false;
	}

	acceptClient();
	return true;
}

void LiveServer::close() {
	for (auto& clientEntry : clients) {
		delete clientEntry.second;
	}
	clients.clear();

	if (log) {
		log->Message("Server was shutdown.");
		log->Disconnect();
		log = nullptr;
	}

	stopped = true;
	if (acceptor) {
		acceptor->close();
	}
}

void LiveServer::acceptClient() {
	static uint32_t id = 0;
	if (stopped) {
		return;
	}

	auto newSocket = std::make_shared<asio::ip::tcp::socket>(
		NetworkConnection::getInstance().get_service()
	);

	acceptor->async_accept(*newSocket, [this, newSocket](const net_error_code& error) -> void {
		if (error) {
			//
		} else {
			net_error_code endpointError;
			const std::string host = newSocket->remote_endpoint(endpointError).address().to_string();
			if (!endpointError) {
				std::cout << "[live] Incoming connection from " << host << std::endl;
				std::cout.flush();
			}

			LivePeer* peer = newd LivePeer(this, std::move(*newSocket));
			peer->log = log;
			peer->receiveHeader();

			clients.insert(std::make_pair(id, peer));
			peer->setId(id);
			++id;
		}
		acceptClient();
	});
}

void LiveServer::removeClient(uint32_t id) {
	auto it = clients.find(id);
	if (it == clients.end()) {
		return;
	}

	const uint32_t clientId = it->second->getClientId();
	if (clientId != 0) {
		clientIds &= ~clientId;
		editor->getMap().clearVisible(clientIds);
		cursors.erase(clientId);
	}

	liveServerLog(log, it->second->getName() + " (" + it->second->getHostName() + ") disconnected.");

	LivePeer* peer = it->second;
	clients.erase(it);
	updateClientList();

	wxTheApp->CallAfter([peer]() {
		delete peer;
	});
}

void LiveServer::updateCursor(const Position& position) {
	LiveCursor cursor;
	cursor.id = 0;
	cursor.pos = position;
	cursor.color = wxColor(
		g_settings.getInteger(Config::CURSOR_RED),
		g_settings.getInteger(Config::CURSOR_GREEN),
		g_settings.getInteger(Config::CURSOR_BLUE),
		g_settings.getInteger(Config::CURSOR_ALPHA)
	);
	broadcastCursor(cursor);
}

void LiveServer::updateClientList() {
	rebuildParticipantList();

	if (log) {
		log->UpdateClientList(clients);
	}

	broadcastClientList();
}

void LiveServer::rebuildParticipantList() {
	participants.clear();
	participants.reserve(clients.size());
	for (const auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		if (!peer->connected || peer->clientId == 0) {
			continue;
		}

		LiveParticipant participant;
		participant.id = peer->clientId;
		participant.name = peer->getName();
		participant.color = peer->getUsedColor();
		participants.push_back(participant);
	}
}

void LiveServer::broadcastClientList() {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CLIENT_LIST);
	writeParticipantList(message);

	for (auto& clientEntry : clients) {
		if (clientEntry.second->connected) {
			clientEntry.second->send(message);
		}
	}

	if (!g_gui.IsHeadless()) {
		g_gui.RefreshView();
	}
}

uint16_t LiveServer::getPort() const {
	return port;
}

bool LiveServer::setPort(int32_t newPort) {
	if (newPort < 1 || newPort > 65535) {
		setLastError("Port must be a number in the range 1-65535.");
		return false;
	}
	port = newPort;
	return true;
}

uint32_t LiveServer::getFreeClientId() {
	for (int32_t bit = 1; bit < (1 << 16); bit <<= 1) {
		if (!testFlags(clientIds, bit)) {
			clientIds |= bit;
			return bit;
		}
	}
	return 0;
}

std::string LiveServer::getHostName() const {
	if (acceptor) {
		auto endpoint = acceptor->local_endpoint();
		return endpoint.address().to_string() + ":" + std::to_string(endpoint.port());
	}
	return "localhost";
}

void LiveServer::kickClient(const wxString& name) {
	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		if (peer->getName().IsSameAs(name, false)) {
			NetworkMessage message;
			message.write<uint8_t>(PACKET_KICK);
			message.write<std::string>("You were kicked by the host.");
			peer->send(message);
			peer->close();
			return;
		}
	}
	liveServerLog(log, "No connected user named '" + name + "'.");
}

void LiveServer::broadcastNodes(DirtyList& dirtyList) {
	if (dirtyList.Empty()) {
		return;
	}

	for (const auto& ind : dirtyList.GetPosList()) {
		int32_t ndx = ind.pos >> 18;
		int32_t ndy = (ind.pos >> 4) & 0x3FFF;
		uint32_t floors = ind.floors;

		QTreeNode* node = editor->getMap().getLeaf(ndx * 4, ndy * 4);
		if (!node) {
			continue;
		}

		for (auto& clientEntry : clients) {
			LivePeer* peer = clientEntry.second;

			const uint32_t clientId = peer->getClientId();
			if (dirtyList.owner != 0 && dirtyList.owner == clientId) {
				continue;
			}

			if (node->isVisible(clientId, true)) {
				peer->sendNode(clientId, node, ndx, ndy, floors & 0xFF00);
			}

			if (node->isVisible(clientId, false)) {
				peer->sendNode(clientId, node, ndx, ndy, floors & 0x00FF);
			}
		}
	}
}

void LiveServer::broadcastCursor(const LiveCursor& cursor) {
	if (clients.empty()) {
		return;
	}

	cursors[cursor.id] = cursor;

	NetworkMessage message;
	message.write<uint8_t>(PACKET_CURSOR_UPDATE);
	writeCursor(message, cursor);

	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		if (peer->getClientId() != cursor.id) {
			peer->send(message);
		}
	}
}

void LiveServer::sendCommentList(LivePeer* peer) {
	if (!peer) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_COMMENT_LIST);
	const std::vector<MapComment>& comments = editor->getMap().getComments().all();
	message.write<uint32_t>(static_cast<uint32_t>(comments.size()));
	for (const MapComment& comment : comments) {
		writeMapComment(message, comment);
	}
	peer->send(message);
}

void LiveServer::broadcastComment(const MapComment& comment) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_COMMENT);
	writeMapComment(message, comment);

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

void LiveServer::broadcastCommentRemoved(uint32_t commentId) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_COMMENT_REMOVED);
	message.write<uint32_t>(commentId);

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

void LiveServer::startOperation(const wxString& operationMessage) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_START_OPERATION);
	message.write<std::string>(nstr(operationMessage));

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}

void LiveServer::updateOperation(int32_t percent) {
	if (clients.empty()) {
		return;
	}

	NetworkMessage message;
	message.write<uint8_t>(PACKET_UPDATE_OPERATION);
	message.write<uint32_t>(percent);

	for (auto& clientEntry : clients) {
		clientEntry.second->send(message);
	}
}
