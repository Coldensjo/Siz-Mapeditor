//////////////////////////////////////////////////////////////////////
// Live mapping map backup folder and rotation.
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_map_backup.h"

#include <wx/dir.h>
#include <wx/filename.h>

#include <algorithm>
#include <iostream>
#include <set>
#include <vector>

namespace {

constexpr size_t kLiveMapBackupZipBatchSize = 10;

bool isBackupTimestamp(const std::string& value) {
	if (value.size() != 19) {
		return false;
	}

	for (size_t i = 0; i < value.size(); ++i) {
		const char c = value[i];
		if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) {
			if (c != '-') {
				return false;
			}
		} else if (c < '0' || c > '9') {
			return false;
		}
	}

	return true;
}

std::string extractBackupTimestamp(const std::string& filename) {
	const size_t lastDot = filename.rfind('.');
	if (lastDot == std::string::npos || lastDot == 0) {
		return {};
	}

	const size_t prevDot = filename.rfind('.', lastDot - 1);
	if (prevDot == std::string::npos) {
		return {};
	}

	const std::string timestamp = filename.substr(prevDot + 1, lastDot - prevDot - 1);
	return isBackupTimestamp(timestamp) ? timestamp : std::string();
}

bool isMapBackupFile(const wxString& filename) {
	const std::string name = filename.ToStdString();
	return name.size() > 5 && (name.rfind(".otbm") == name.size() - 5 || name.rfind(".otgz") == name.size() - 5);
}

std::vector<std::string> collectBackupTimestamps(const std::string& backupDir) {
	wxArrayString files;
	if (wxDir::GetAllFiles(wxstr(backupDir), &files, wxEmptyString, wxDIR_FILES) == 0) {
		return {};
	}

	std::set<std::string> timestamps;
	for (const wxString& file : files) {
		if (!isMapBackupFile(file)) {
			continue;
		}

		const std::string timestamp = extractBackupTimestamp(file.ToStdString());
		if (!timestamp.empty()) {
			timestamps.insert(timestamp);
		}
	}

	return std::vector<std::string>(timestamps.begin(), timestamps.end());
}

bool fileMatchesTimestamp(const std::string& filename, const std::string& timestamp) {
	const std::string needle = "." + timestamp + ".";
	return filename.find(needle) != std::string::npos;
}

#ifdef OTGZ_SUPPORT
bool zipBackupBatch(const std::string& backupDir, const std::vector<std::string>& timestamps) {
	if (timestamps.empty()) {
		return false;
	}

	wxArrayString files;
	if (wxDir::GetAllFiles(wxstr(backupDir), &files, wxEmptyString, wxDIR_FILES) == 0) {
		return false;
	}

	std::vector<std::string> filesToArchive;
	for (const wxString& file : files) {
		const std::string filename = file.ToStdString();
		if (filename.size() > 4 && filename.rfind(".zip") == filename.size() - 4) {
			continue;
		}

		for (const std::string& timestamp : timestamps) {
			if (fileMatchesTimestamp(filename, timestamp)) {
				filesToArchive.push_back(filename);
				break;
			}
		}
	}

	if (filesToArchive.empty()) {
		return false;
	}

	const std::string zipName = backupDir + "backups-" + timestamps.front() + "_to_" + timestamps.back() + ".zip";
	struct archive* archiveHandle = archive_write_new();
	if (!archiveHandle) {
		return false;
	}

	archive_write_set_format_zip(archiveHandle);
	if (archive_write_open_filename(archiveHandle, zipName.c_str()) != ARCHIVE_OK) {
		archive_write_free(archiveHandle);
		return false;
	}

	bool success = true;
	for (const std::string& filename : filesToArchive) {
		const std::string fullPath = backupDir + filename;
		wxFile file(wxstr(fullPath), wxFile::read);
		if (!file.IsOpened()) {
			success = false;
			break;
		}

		const wxFileOffset fileSize = file.Length();
		if (fileSize < 0) {
			success = false;
			break;
		}

		std::vector<char> buffer(static_cast<size_t>(fileSize));
		if (file.Read(buffer.data(), buffer.size()) != buffer.size()) {
			success = false;
			break;
		}
		file.Close();

		struct archive_entry* entry = archive_entry_new();
		archive_entry_set_pathname(entry, filename.c_str());
		archive_entry_set_size(entry, buffer.size());
		archive_entry_set_filetype(entry, AE_IFREG);
		archive_entry_set_perm(entry, 0644);

		if (archive_write_header(archiveHandle, entry) != ARCHIVE_OK) {
			archive_entry_free(entry);
			success = false;
			break;
		}

		if (archive_write_data(archiveHandle, buffer.data(), buffer.size()) < 0) {
			archive_entry_free(entry);
			success = false;
			break;
		}

		archive_entry_free(entry);
	}

	archive_write_close(archiveHandle);
	archive_write_free(archiveHandle);

	if (!success) {
		std::remove(zipName.c_str());
		return false;
	}

	for (const std::string& filename : filesToArchive) {
		std::remove((backupDir + filename).c_str());
	}

	std::cout << "[live] Archived " << timestamps.size() << " map backup(s) to " << zipName << std::endl;
	return true;
}
#endif

} // namespace

std::string getLiveMapBackupDirectory(const std::string& mapPath) {
	FileName directory(wxstr(mapPath));
	const wxString backupDir = directory.GetPath(wxPATH_GET_SEPARATOR | wxPATH_GET_VOLUME) + "backup" + FileName::GetPathSeparator();
	return nstr(backupDir);
}

bool ensureLiveMapBackupDirectory(const std::string& backupDir) {
	FileName directory(wxstr(backupDir));
	if (directory.DirExists()) {
		return true;
	}

	return directory.Mkdir(0755, wxPATH_MKDIR_FULL);
}

void rotateLiveMapBackups(const std::string& backupDir) {
#ifdef OTGZ_SUPPORT
	std::vector<std::string> timestamps = collectBackupTimestamps(backupDir);
	while (timestamps.size() >= kLiveMapBackupZipBatchSize) {
		const std::vector<std::string> batch(
			timestamps.begin(),
			timestamps.begin() + static_cast<std::ptrdiff_t>(kLiveMapBackupZipBatchSize)
		);

		if (!zipBackupBatch(backupDir, batch)) {
			std::cerr << "[live] Failed to archive old map backups." << std::endl;
			break;
		}

		timestamps = collectBackupTimestamps(backupDir);
	}
#endif
}
