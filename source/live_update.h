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

#ifndef RME_LIVE_UPDATE_H_
#define RME_LIVE_UPDATE_H_

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include <wx/filename.h>
#include <wx/string.h>

// A single file the live server offers to outdated clients as part of an
// editor update package (the editor executable and, optionally, its DLLs).
struct LiveUpdateFile {
	std::string filename; // base name as written on the client (e.g. "Editor_x64.exe")
	wxFileName sourcePath; // server-side absolute path to read from
	uint32_t size = 0;
};

// Client-side state while receiving a pushed update into the staging folder.
struct LiveUpdateReceiveState {
	wxFileName stagingDir;
	std::ofstream output;
	std::string currentFilename;
	uint32_t expectedSize = 0;
	uint32_t receivedSize = 0;
	std::vector<std::string> receivedFiles;
	bool active = false;
};

static const size_t LIVE_UPDATE_CHUNK_SIZE = 60000;

// Server side: turn the configured list of paths into transferable update
// files. Returns false (with error) if any listed path is missing/unreadable.
bool collectUpdateFiles(const std::vector<wxString>& paths, std::vector<LiveUpdateFile>& files, wxString& error);

// Client side: folder next to the running editor where staged files land.
wxFileName getLiveUpdateStagingDir();
void resetLiveUpdateReceiveState(LiveUpdateReceiveState& state);
bool beginLiveUpdateReceive(LiveUpdateReceiveState& state, const std::string& filename, uint32_t size, wxString& error);
bool appendLiveUpdateChunk(LiveUpdateReceiveState& state, const std::string& chunk, wxString& error);
bool finishLiveUpdateReceive(LiveUpdateReceiveState& state, wxString& error);

// Client side: swap the staged files over the live ones (the running executable
// is renamed to *.old, which Windows permits), then relaunch the editor. Pass
// reconnect=true to have the new instance rejoin the last live session. Takes
// plain copies (staging folder + downloaded file names) rather than the live
// receive state so it is safe to run after the live client has been torn down.
// Returns false (with error) only if staging fails before the relaunch; on
// failure any partially-swapped file is rolled back so a working editor remains.
bool applyLiveUpdateAndRestart(const wxFileName& stagingDir, const std::vector<std::string>& files, bool reconnect, wxString& error);

// Startup housekeeping: remove leftover *.old files from a previous self-update.
void cleanupLiveUpdateLeftovers();

#endif
