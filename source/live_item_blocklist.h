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
void collectBrushItemIds(const Brush* brush, std::vector<uint16_t>& out);
uint16_t findBlockedItemInBrush(const Brush* brush, const std::set<uint16_t>& blockedItems, const std::set<uint16_t>& dismissedItems);

#endif
