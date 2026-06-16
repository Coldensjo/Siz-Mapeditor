//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_TILESET_EDIT_H_
#define RME_TILESET_EDIT_H_

#include "main.h"
#include "tileset.h"

#include <vector>

struct TilesetEditEntry {
	enum Kind {
		ITEM,
		BRUSH,
	};

	Kind kind = ITEM;
	uint16_t item_id = 0;
	std::string brush_name;
};

bool TilesetExtractEditEntries(const TilesetCategory* category, std::vector<TilesetEditEntry>& entries, wxString& error);
bool TilesetApplyEditEntries(Tileset* tileset, TilesetCategoryType categoryType, const std::vector<TilesetEditEntry>& entries, wxString& error);
bool TilesetSaveToXml(Tileset* tileset, TilesetCategoryType categoryType, wxString& error);

#endif
