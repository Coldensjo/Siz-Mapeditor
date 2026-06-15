//////////////////////////////////////////////////////////////////////
// Live mapping map backup folder and rotation.
//////////////////////////////////////////////////////////////////////

#ifndef RME_LIVE_MAP_BACKUP_H_
#define RME_LIVE_MAP_BACKUP_H_

#include <string>

// Returns map_path + "backup/" with trailing separator.
std::string getLiveMapBackupDirectory(const std::string& mapPath);

// Ensures the backup directory exists.
bool ensureLiveMapBackupDirectory(const std::string& backupDir);

// When 10+ timestamped backup sets exist, zip the oldest 10 and remove them.
void rotateLiveMapBackups(const std::string& backupDir);

#endif
