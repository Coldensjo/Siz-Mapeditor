//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "live_item_blocklist.h"
#include "brush.h"
#include "brush_edit.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "wall_brush.h"
#include "raw_brush.h"

#include <algorithm>

namespace {

bool parseItemIdToken(const std::string& token, long& out) {
	if (token.empty()) {
		return false;
	}

	char* end = nullptr;
	const long value = std::strtol(token.c_str(), &end, 10);
	if (end == token.c_str() || *end != '\0' || value < 1 || value > 65535) {
		return false;
	}

	out = value;
	return true;
}

void appendUniqueItemId(std::vector<uint16_t>& out, uint16_t itemId) {
	if (itemId == 0) {
		return;
	}
	if (std::find(out.begin(), out.end(), itemId) == out.end()) {
		out.push_back(itemId);
	}
}

} // namespace

bool parseBlockItemSpec(const std::string& spec, std::set<uint16_t>& out, std::string& error) {
	const size_t dash = spec.find('-');
	if (dash == std::string::npos) {
		long itemId = 0;
		if (!parseItemIdToken(spec, itemId)) {
			error = "Expected an item id or id range (e.g. 2900 or 2900-2950).";
			return false;
		}
		out.insert(static_cast<uint16_t>(itemId));
		return true;
	}

	long fromId = 0;
	long toId = 0;
	if (!parseItemIdToken(spec.substr(0, dash), fromId) || !parseItemIdToken(spec.substr(dash + 1), toId)) {
		error = "Expected an item id or id range (e.g. 2900 or 2900-2950).";
		return false;
	}

	if (fromId > toId) {
		std::swap(fromId, toId);
	}

	for (long itemId = fromId; itemId <= toId; ++itemId) {
		out.insert(static_cast<uint16_t>(itemId));
	}
	return true;
}

void writeBlockedItemList(NetworkMessage& message, const std::set<uint16_t>& blockedItems) {
	message.write<uint32_t>(static_cast<uint32_t>(blockedItems.size()));
	for (uint16_t itemId : blockedItems) {
		message.write<uint16_t>(itemId);
	}
}

void readBlockedItemList(NetworkMessage& message, std::set<uint16_t>& blockedItems) {
	blockedItems.clear();
	const uint32_t count = message.read<uint32_t>();
	for (uint32_t i = 0; i < count; ++i) {
		blockedItems.insert(message.read<uint16_t>());
	}
}

void collectBrushItemIds(const Brush* brush, std::vector<uint16_t>& out) {
	out.clear();
	if (!brush) {
		return;
	}

	Brush* mutableBrush = const_cast<Brush*>(brush);
	if (const RAWBrush* rawBrush = mutableBrush->asRaw()) {
		appendUniqueItemId(out, rawBrush->getItemID());
		return;
	}

	std::vector<BrushEditEntry> entries;
	if (const DoodadBrush* doodadBrush = mutableBrush->asDoodad()) {
		wxString error;
		if (!doodadBrush->extractEditEntries(entries, error)) {
			return;
		}
	} else if (const GroundBrush* groundBrush = mutableBrush->asGround()) {
		if (!groundBrush->extractEditEntries(entries)) {
			return;
		}
	} else if (const WallBrush* wallBrush = mutableBrush->asWall()) {
		if (!wallBrush->extractEditEntries(entries)) {
			return;
		}
	} else {
		return;
	}

	for (const BrushEditEntry& entry : entries) {
		appendUniqueItemId(out, entry.item_id);
		for (const auto& tileEntry : entry.composite_tiles) {
			appendUniqueItemId(out, tileEntry.second);
		}
	}
}

uint16_t findBlockedItemInBrush(const Brush* brush, const std::set<uint16_t>& blockedItems, const std::set<uint16_t>& dismissedItems) {
	if (blockedItems.empty()) {
		return 0;
	}

	std::vector<uint16_t> itemIds;
	collectBrushItemIds(brush, itemIds);
	for (uint16_t itemId : itemIds) {
		if (blockedItems.count(itemId) != 0 && dismissedItems.count(itemId) == 0) {
			return itemId;
		}
	}
	return 0;
}
