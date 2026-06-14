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

#ifndef RME_LIVE_CLIENT_H_
#define RME_LIVE_CLIENT_H_

#include "live_socket.h"
#include "net_connection.h"
#include "live_assets.h"

#include <chrono>
#include <functional>
#include <map>
#include <set>
#include <vector>

class DirtyList;
class MapTab;
class LiveLogTab;

class LiveClient : public LiveSocket {
public:
	LiveClient();
	~LiveClient();

	//
	bool connect(const std::string& address, uint16_t port);
	void tryConnect(asio::ip::tcp::resolver::results_type endpoints);

	void close();
	bool handleError(const net_error_code& error);

	//
	std::string getHostName() const;

	//
	void receiveHeader();
	void receive(uint32_t packetSize);
	void send(NetworkMessage& message);
	void send(NetworkMessage& message, std::function<void()> onSent);

	//
	void updateCursor(const Position& position);

	LiveLogTab* createLogWindow(wxWindow* parent);
	MapTab* createEditorWindow();

	// send packets
	void sendHello();
	void sendNodeRequests();
	void sendChanges(DirtyList& dirtyList);
	void sendReady();
	void sendCommentAdd(const Position& pos, const std::string& text);
	void sendCommentRemove(uint32_t commentId);

	// Flags a node as queried and stores it, need to call SendNodeRequest to send it to server
	void queryNode(int32_t ndx, int32_t ndy, bool underground);
	void tickNodeRequests();
	void requestViewportRefresh();
	bool consumeViewportRefresh();
	void invalidateViewport(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y);

	Editor* getEditor() const {
		return editor;
	}

protected:
	void parsePacket(NetworkMessage message);

	// parse packets
	void parseHello(NetworkMessage& message);
	void parseKick(NetworkMessage& message);
	void parseClientAccepted(NetworkMessage& message);
	void parseChangeClientVersion(NetworkMessage& message);
	void parseAssetFileBegin(NetworkMessage& message);
	void parseAssetFileChunk(NetworkMessage& message);
	void parseAssetFileEnd(NetworkMessage& message);
	void parseAssetFilesDone(NetworkMessage& message);
	void finishLiveVersionLoad();
	void parseNode(NetworkMessage& message);
	void parseCursorUpdate(NetworkMessage& message);
	void parseClientList(NetworkMessage& message);
	void parseCommentList(NetworkMessage& message);
	void parseComment(NetworkMessage& message);
	void parseCommentRemoved(NetworkMessage& message);
	void parseStartOperation(NetworkMessage& message);
	void parseUpdateOperation(NetworkMessage& message);

	//
	NetworkMessage readMessage;

	std::set<uint32_t> queryNodeList;
	std::map<uint32_t, std::chrono::steady_clock::time_point> pendingNodeRequests;
	bool viewportRefreshPending;
	bool hasPendingCommentList;
	std::vector<MapComment> pendingCommentList;
	wxString currentOperation;

	std::shared_ptr<asio::ip::tcp::resolver> resolver;
	std::shared_ptr<asio::ip::tcp::socket> socket;

	Editor* editor;

	wxColor ownClientColor;
	bool stopped;

	ClientVersionID pendingVersionId;
	std::vector<LiveAssetFile> pendingAssetManifest;
	LiveAssetReceiveState assetReceiveState;
	bool ignoreIncomingAssets;
	bool waitingForServerAssets;
	uint64_t assetBytesExpected;
	uint64_t assetBytesReceived;
	int assetProgressReported;
};

#endif
