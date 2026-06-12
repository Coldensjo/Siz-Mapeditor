//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_assets.h"
#include "gui.h"

#include <wx/dir.h>

namespace {
wxFileName cacheDirectoryForKind(const wxFileName& root, LiveAssetKind kind) {
	wxFileName dir(root);
	switch (kind) {
		case LIVE_ASSET_ITEMS:
			dir.AppendDir("items");
			break;
		case LIVE_ASSET_MONSTERS:
			dir.AppendDir("monsters");
			break;
		case LIVE_ASSET_NPCS:
			dir.AppendDir("npcs");
			break;
		default:
			dir.AppendDir("client");
			break;
	}
	dir.Mkdir(0755, wxPATH_MKDIR_FULL);
	return dir;
}

wxFileName cachedAssetPath(const wxFileName& root, LiveAssetKind kind, const wxString& relativeFilename) {
	wxFileName target = cacheDirectoryForKind(root, kind);
	wxString relativePath = relativeFilename;
	relativePath.Replace("\\", "/");

	const wxArrayString parts = wxSplit(relativePath, '/');
	for (size_t index = 0; index + 1 < parts.size(); ++index) {
		if (!parts[index].empty()) {
			target.AppendDir(parts[index]);
		}
	}

	if (!parts.empty()) {
		target.SetFullName(parts.back());
	}

	return target;
}

bool fileSizeIfExists(const wxFileName& path, uint32_t& size) {
	if (!path.FileExists()) {
		return false;
	}

	wxStructStat statBuffer;
	if (wxStat(path.GetFullPath(), &statBuffer) != 0) {
		return false;
	}

	size = static_cast<uint32_t>(statBuffer.st_size);
	return true;
}

void appendAssetFile(std::vector<LiveAssetFile>& files, LiveAssetKind kind, const wxFileName& path) {
	if (!path.FileExists()) {
		return;
	}

	uint32_t size = 0;
	if (!fileSizeIfExists(path, size)) {
		return;
	}

	LiveAssetFile file;
	file.kind = kind;
	file.filename = nstr(path.GetFullName());
	file.sourcePath = path;
	file.size = size;
	files.push_back(file);
}

void appendCreatureDirectoryFiles(std::vector<LiveAssetFile>& files, LiveAssetKind kind, const FileName& directory) {
	wxString basePath = directory.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	if (basePath.empty() || !directory.DirExists()) {
		return;
	}

	wxArrayString allFiles;
	if (wxDir::GetAllFiles(basePath, &allFiles, "*.xml", wxDIR_FILES | wxDIR_DIRS) == 0) {
		return;
	}

	for (const wxString& fullPath : allFiles) {
		wxString relativePath = fullPath;
		if (relativePath.StartsWith(basePath)) {
			relativePath = relativePath.Mid(basePath.length());
		}
		relativePath.Replace("\\", "/");
		while (relativePath.StartsWith("/")) {
			relativePath = relativePath.Mid(1);
		}
		if (relativePath.empty()) {
			continue;
		}

		FileName sourcePath(fullPath);
		uint32_t size = 0;
		if (!fileSizeIfExists(sourcePath, size)) {
			continue;
		}

		LiveAssetFile file;
		file.kind = kind;
		file.filename = nstr(relativePath);
		file.sourcePath = sourcePath;
		file.size = size;
		files.push_back(file);
	}
}
} // namespace

wxFileName getLiveAssetCacheRoot(ClientVersionID versionId) {
	const ClientVersion* version = ClientVersion::get(versionId);
	wxString versionLabel = version ? wxstr(version->getName()) : wxString::Format("version_%u", static_cast<unsigned>(versionId));
	versionLabel.Replace("/", "_");
	versionLabel.Replace("\\", "_");

	wxFileName root = g_gui.GetLocalDataDirectory() + "live_cache" + FileName::GetPathSeparator() + versionLabel + FileName::GetPathSeparator();
	root.Mkdir(0755, wxPATH_MKDIR_FULL);
	return root;
}

bool collectServerAssetFiles(std::vector<LiveAssetFile>& files, wxString& error) {
	files.clear();

	if (!g_gui.IsVersionLoaded()) {
		error = "The live server has no client version loaded.";
		return false;
	}

	const ClientVersion& version = g_gui.GetCurrentVersion();
	const FileName clientPath = version.getClientPath();
	if (!clientPath.DirExists()) {
		error = "The live server is missing its client asset directory.";
		return false;
	}

	appendAssetFile(files, LIVE_ASSET_CLIENT, version.getMetadataPath());
	appendAssetFile(files, LIVE_ASSET_CLIENT, version.getSpritesPath());

	wxDir clientDir(clientPath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
	wxString otfiFile;
	if (clientDir.GetFirst(&otfiFile, "*.otfi", wxDIR_FILES)) {
		do {
			appendAssetFile(files, LIVE_ASSET_CLIENT, wxFileName(clientPath.GetFullPath(), otfiFile));
		} while (clientDir.GetNext(&otfiFile));
	}

	const FileName itemsPath = version.getItemsPath();
	appendAssetFile(files, LIVE_ASSET_ITEMS, wxFileName(itemsPath.GetFullPath(), "items.otb"));
	appendAssetFile(files, LIVE_ASSET_ITEMS, wxFileName(itemsPath.GetFullPath(), "items.xml"));

	if (version.hasCustomMonstersPath()) {
		appendCreatureDirectoryFiles(files, LIVE_ASSET_MONSTERS, version.getMonstersPath());
	}

	if (version.hasCustomNpcsPath()) {
		appendCreatureDirectoryFiles(files, LIVE_ASSET_NPCS, version.getNpcsPath());
	}

	if (files.empty()) {
		error = "The live server could not locate any asset files to send.";
		return false;
	}

	return true;
}

bool isLiveAssetCacheComplete(ClientVersionID versionId, const std::vector<LiveAssetFile>& files) {
	if (files.empty()) {
		return false;
	}

	const wxFileName root = getLiveAssetCacheRoot(versionId);
	for (const LiveAssetFile& file : files) {
		wxFileName cached = cachedAssetPath(root, file.kind, wxstr(file.filename));
		uint32_t size = 0;
		if (!fileSizeIfExists(cached, size) || size != file.size) {
			return false;
		}
	}
	return true;
}

bool applyLiveAssetCacheToVersion(ClientVersionID versionId) {
	ClientVersion* version = ClientVersion::get(versionId);
	if (version == nullptr) {
		return false;
	}

	const wxFileName root = getLiveAssetCacheRoot(versionId);
	const wxFileName clientDir = cacheDirectoryForKind(root, LIVE_ASSET_CLIENT);
	const wxFileName itemsDir = cacheDirectoryForKind(root, LIVE_ASSET_ITEMS);
	const wxFileName monstersDir = cacheDirectoryForKind(root, LIVE_ASSET_MONSTERS);
	const wxFileName npcsDir = cacheDirectoryForKind(root, LIVE_ASSET_NPCS);

	version->setClientPath(clientDir);
	version->setItemsPath(itemsDir);

	wxArrayString cachedMonsterFiles;
	const wxString monstersPath = monstersDir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	if (!monstersPath.empty() && wxDir::GetAllFiles(monstersPath, &cachedMonsterFiles, "*.xml", wxDIR_FILES | wxDIR_DIRS) > 0) {
		version->setMonstersPath(monstersDir);
	} else {
		version->clearMonstersPath();
	}

	wxArrayString cachedNpcFiles;
	const wxString npcsPath = npcsDir.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
	if (!npcsPath.empty() && wxDir::GetAllFiles(npcsPath, &cachedNpcFiles, "*.xml", wxDIR_FILES | wxDIR_DIRS) > 0) {
		version->setNpcsPath(npcsDir);
	} else {
		version->clearNpcsPath();
	}

	return true;
}

void resetLiveAssetReceiveState(LiveAssetReceiveState& state) {
	if (state.output.is_open()) {
		state.output.close();
	}
	state = LiveAssetReceiveState();
}

bool beginLiveAssetReceive(LiveAssetReceiveState& state, ClientVersionID versionId, LiveAssetKind kind, const std::string& filename, uint32_t size, wxString& error) {
	if (state.output.is_open()) {
		state.output.close();
	}

	state.versionId = versionId;
	state.currentKind = kind;
	state.currentFilename = filename;
	state.expectedSize = size;
	state.receivedSize = 0;
	state.active = true;

	const wxFileName root = getLiveAssetCacheRoot(versionId);
	state.clientDir = cacheDirectoryForKind(root, LIVE_ASSET_CLIENT);
	state.itemsDir = cacheDirectoryForKind(root, LIVE_ASSET_ITEMS);
	state.monstersDir = cacheDirectoryForKind(root, LIVE_ASSET_MONSTERS);
	state.npcsDir = cacheDirectoryForKind(root, LIVE_ASSET_NPCS);

	wxFileName target = cachedAssetPath(root, kind, wxstr(filename));
	target.Mkdir(0755, wxPATH_MKDIR_FULL);
	state.output.open(nstr(target.GetFullPath()), std::ios::binary | std::ios::trunc);
	if (!state.output.is_open()) {
		error = "Could not write downloaded asset file.";
		return false;
	}

	return true;
}

bool appendLiveAssetChunk(LiveAssetReceiveState& state, const std::string& chunk, wxString& error) {
	if (!state.output.is_open()) {
		error = "No asset download is in progress.";
		return false;
	}

	state.output.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
	if (!state.output.good()) {
		error = "Failed while writing downloaded asset data.";
		return false;
	}

	state.receivedSize += static_cast<uint32_t>(chunk.size());
	return true;
}

bool finishLiveAssetReceive(LiveAssetReceiveState& state, wxString& error) {
	if (!state.output.is_open()) {
		error = "No asset download is in progress.";
		return false;
	}

	state.output.close();
	if (state.receivedSize != state.expectedSize) {
		error = wxString::Format(
			"Incomplete asset download for %s (%u of %u bytes).",
			wxstr(state.currentFilename),
			state.receivedSize,
			state.expectedSize
		);
		return false;
	}

	return true;
}
