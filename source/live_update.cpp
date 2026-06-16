//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_update.h"

#include <wx/dir.h>
#include <wx/filefn.h>
#include <wx/stdpaths.h>
#include <wx/utils.h>

namespace {

wxString executableDirectory() {
	wxFileName execFile;
	try {
		execFile = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast&) {
		execFile.Assign(wxGetCwd(), wxEmptyString);
	}
	return execFile.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
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

} // namespace

bool collectUpdateFiles(const std::vector<wxString>& paths, std::vector<LiveUpdateFile>& files, wxString& error) {
	files.clear();

	for (const wxString& rawPath : paths) {
		wxString trimmed = rawPath;
		trimmed.Trim(true).Trim(false);
		if (trimmed.empty()) {
			continue;
		}

		wxFileName sourcePath(trimmed);
		if (!sourcePath.FileExists()) {
			error = "Update file not found: " + trimmed;
			files.clear();
			return false;
		}

		uint32_t size = 0;
		if (!fileSizeIfExists(sourcePath, size) || size == 0) {
			error = "Update file is empty or unreadable: " + trimmed;
			files.clear();
			return false;
		}

		LiveUpdateFile file;
		file.filename = nstr(sourcePath.GetFullName());
		file.sourcePath = sourcePath;
		file.size = size;
		files.push_back(file);
	}

	if (files.empty()) {
		error = "No update files are configured.";
		return false;
	}

	return true;
}

wxFileName getLiveUpdateStagingDir() {
	wxFileName dir;
	dir.AssignDir(executableDirectory());
	dir.AppendDir("update_staging");
	return dir;
}

void resetLiveUpdateReceiveState(LiveUpdateReceiveState& state) {
	if (state.output.is_open()) {
		state.output.close();
	}
	state = LiveUpdateReceiveState();
}

bool beginLiveUpdateReceive(LiveUpdateReceiveState& state, const std::string& filename, uint32_t size, wxString& error) {
	if (state.output.is_open()) {
		state.output.close();
	}

	if (!state.stagingDir.IsOk() || state.stagingDir.GetPath().empty()) {
		state.stagingDir = getLiveUpdateStagingDir();
	}
	state.stagingDir.Mkdir(0755, wxPATH_MKDIR_FULL);

	// Update packages are flat lists of file names; guard against path traversal.
	wxString baseName = wxFileName(wxstr(filename)).GetFullName();
	if (baseName.empty()) {
		error = "Server sent an update file with an invalid name.";
		return false;
	}

	wxFileName target(state.stagingDir);
	target.SetFullName(baseName);

	state.output.open(nstr(target.GetFullPath()), std::ios::binary | std::ios::trunc);
	if (!state.output.is_open()) {
		error = "Could not write to the update staging folder: " + target.GetFullPath();
		return false;
	}

	state.currentFilename = nstr(baseName);
	state.expectedSize = size;
	state.receivedSize = 0;
	state.active = true;
	return true;
}

bool appendLiveUpdateChunk(LiveUpdateReceiveState& state, const std::string& chunk, wxString& error) {
	if (!state.output.is_open()) {
		error = "Received an update chunk before the file started.";
		return false;
	}

	state.output.write(chunk.data(), static_cast<std::streamsize>(chunk.size()));
	if (!state.output.good()) {
		error = "Failed while writing the downloaded update.";
		return false;
	}

	state.receivedSize += static_cast<uint32_t>(chunk.size());
	return true;
}

bool finishLiveUpdateReceive(LiveUpdateReceiveState& state, wxString& error) {
	if (state.output.is_open()) {
		state.output.close();
	}

	if (state.receivedSize != state.expectedSize) {
		error = wxString::Format("Update file '%s' was truncated (%u of %u bytes).",
			wxstr(state.currentFilename), state.receivedSize, state.expectedSize);
		return false;
	}

	state.receivedFiles.push_back(state.currentFilename);
	state.currentFilename.clear();
	state.expectedSize = 0;
	state.receivedSize = 0;
	return true;
}

bool applyLiveUpdateAndRestart(const LiveUpdateReceiveState& state, bool reconnect, wxString& error) {
	if (state.receivedFiles.empty()) {
		error = "The server did not send any update files.";
		return false;
	}

	const wxString execDir = executableDirectory();
	wxFileName execFile;
	try {
		execFile = dynamic_cast<wxStandardPaths&>(wxStandardPaths::Get()).GetExecutablePath();
	} catch (const std::bad_cast&) {
		error = "Could not locate the running editor executable.";
		return false;
	}
	const wxString runningExe = execFile.GetFullName();

	// Swap each staged file over the live one. The running executable cannot be
	// deleted on Windows, but it can be renamed; we move it aside to *.old and
	// drop the new build in its place.
	for (const std::string& name : state.receivedFiles) {
		wxFileName staged(state.stagingDir);
		staged.SetFullName(wxstr(name));

		wxFileName target;
		target.AssignDir(execDir);
		target.SetFullName(wxstr(name));

		if (!staged.FileExists()) {
			error = "Staged update file is missing: " + staged.GetFullPath();
			return false;
		}

		if (target.FileExists()) {
			wxFileName backup = target;
			backup.SetFullName(target.GetFullName() + ".old");
			if (backup.FileExists()) {
				wxRemoveFile(backup.GetFullPath());
			}
			if (!wxRenameFile(target.GetFullPath(), backup.GetFullPath(), true)) {
				error = "Could not replace '" + target.GetFullName() + "'. Close other editor windows and try again.";
				return false;
			}
		}

		if (!wxRenameFile(staged.GetFullPath(), target.GetFullPath(), true)) {
			// Best effort: try a copy if rename across handles failed.
			if (!wxCopyFile(staged.GetFullPath(), target.GetFullPath(), true)) {
				error = "Could not install the new '" + target.GetFullName() + "'.";
				return false;
			}
			wxRemoveFile(staged.GetFullPath());
		}
	}

	// Relaunch the (now updated) editor executable.
	wxFileName newExe;
	newExe.AssignDir(execDir);
	newExe.SetFullName(runningExe);

	wxString command = "\"" + newExe.GetFullPath() + "\"";
	if (reconnect) {
		command += " --live-reconnect";
	}

	if (wxExecute(command, wxEXEC_ASYNC) == 0) {
		error = "The update was installed but the editor could not be relaunched automatically. Please start it again.";
		return false;
	}

	return true;
}

void cleanupLiveUpdateLeftovers() {
	const wxString execDir = executableDirectory();
	if (execDir.empty()) {
		return;
	}

	wxArrayString oldFiles;
	wxDir::GetAllFiles(execDir, &oldFiles, "*.old", wxDIR_FILES);
	for (const wxString& file : oldFiles) {
		wxRemoveFile(file);
	}

	// Drop any half-finished staging folder from an interrupted update.
	wxFileName staging = getLiveUpdateStagingDir();
	if (staging.DirExists()) {
		wxArrayString stagedFiles;
		wxDir::GetAllFiles(staging.GetFullPath(), &stagedFiles, wxEmptyString, wxDIR_FILES);
		for (const wxString& file : stagedFiles) {
			wxRemoveFile(file);
		}
		wxRmdir(staging.GetFullPath());
	}
}
