//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_LIVE_ITEM_BLOCKLIST_H_
#define RME_LIVE_ITEM_BLOCKLIST_H_

#include "net_connection.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

class Brush;

bool parseBlockItemSpec(const std::string& spec, std::set<uint16_t>& out, std::string& error);
void writeBlockedItemList(NetworkMessage& message, const std::set<uint16_t>& blockedItems);
void readBlockedItemList(NetworkMessage& message, std::set<uint16_t>& blockedItems);

// Persist the blocked item ids (with human-readable names) to an XML file so the
// list survives map server restarts. Names are written for readability only; on
// load only the ids matter. Loading a missing file is not an error (out is left
// empty and the function returns true).
bool loadBlockedItemList(const std::string& path, std::set<uint16_t>& blockedItems, std::string& error);
bool saveBlockedItemList(const std::string& path, const std::set<uint16_t>& blockedItems, std::string& error);

void collectBrushItemIds(const Brush* brush, std::vector<uint16_t>& out);
uint16_t findBlockedItemInBrush(const Brush* brush, const std::set<uint16_t>& blockedItems, const std::set<uint16_t>& dismissedItems);

#endif
