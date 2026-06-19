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
#include "items.h"

#include "ext/pugixml.hpp"

#include <algorithm>
#include <wx/filename.h>

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

bool loadBlockedItemList(const std::string& path, std::set<uint16_t>& blockedItems, std::string& error) {
	blockedItems.clear();

	if (!wxFileName(wxString(path.c_str(), wxConvUTF8)).FileExists()) {
		// No saved list yet; treat as an empty list rather than an error.
		return true;
	}

	pugi::xml_document doc;
	const pugi::xml_parse_result result = doc.load_file(path.c_str());
	if (!result) {
		error = std::string("Could not parse blocked item list: ") + result.description();
		return false;
	}

	pugi::xml_node rootNode = doc.child("blocked_items");
	if (!rootNode) {
		error = "Blocked item list is missing its <blocked_items> root node.";
		return false;
	}

	for (pugi::xml_node itemNode : rootNode.children("item")) {
		const long itemId = itemNode.attribute("id").as_int(0);
		if (itemId >= 1 && itemId <= 65535) {
			blockedItems.insert(static_cast<uint16_t>(itemId));
		}
	}
	return true;
}

bool saveBlockedItemList(const std::string& path, const std::set<uint16_t>& blockedItems, std::string& error) {
	pugi::xml_document doc;

	pugi::xml_node decl = doc.prepend_child(pugi::node_declaration);
	decl.append_attribute("version") = "1.0";

	pugi::xml_node rootNode = doc.append_child("blocked_items");
	for (uint16_t itemId : blockedItems) {
		pugi::xml_node itemNode = rootNode.append_child("item");
		itemNode.append_attribute("id") = itemId;

		const ItemType& itemType = g_items.getItemType(itemId);
		if (!itemType.name.empty()) {
			itemNode.append_attribute("name") = itemType.name.c_str();
		}
	}

	if (!doc.save_file(path.c_str(), "\t", pugi::format_default, pugi::encoding_utf8)) {
		error = "Could not write blocked item list to " + path;
		return false;
	}
	return true;
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
