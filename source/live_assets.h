//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_LIVE_ASSETS_H_
#define RME_LIVE_ASSETS_H_

#include "client_version.h"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

enum LiveAssetKind : uint8_t {
	LIVE_ASSET_CLIENT = 0,
	LIVE_ASSET_ITEMS = 1,
};

struct LiveAssetFile {
	LiveAssetKind kind;
	std::string filename;
	wxFileName sourcePath;
	uint32_t size;
};

struct LiveAssetReceiveState {
	ClientVersionID versionId = CLIENT_VERSION_NONE;
	wxFileName clientDir;
	wxFileName itemsDir;
	std::ofstream output;
	LiveAssetKind currentKind = LIVE_ASSET_CLIENT;
	std::string currentFilename;
	uint32_t expectedSize = 0;
	uint32_t receivedSize = 0;
	bool active = false;
};

static const size_t LIVE_ASSET_CHUNK_SIZE = 60000;

wxFileName getLiveAssetCacheRoot(ClientVersionID versionId);
bool collectServerAssetFiles(std::vector<LiveAssetFile>& files, wxString& error);
bool isLiveAssetCacheComplete(ClientVersionID versionId, const std::vector<LiveAssetFile>& files);
bool applyLiveAssetCacheToVersion(ClientVersionID versionId);
void resetLiveAssetReceiveState(LiveAssetReceiveState& state);
bool beginLiveAssetReceive(LiveAssetReceiveState& state, ClientVersionID versionId, LiveAssetKind kind, const std::string& filename, uint32_t size, wxString& error);
bool appendLiveAssetChunk(LiveAssetReceiveState& state, const std::string& chunk, wxString& error);
bool finishLiveAssetReceive(LiveAssetReceiveState& state, wxString& error);

#endif
