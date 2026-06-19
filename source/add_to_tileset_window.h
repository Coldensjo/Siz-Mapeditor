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

#ifndef RME_ADD_TO_TILESET_WINDOW_H_
#define RME_ADD_TO_TILESET_WINDOW_H_

#include "main.h"

#include "common_windows.h"
#include "tileset.h"

class DCButton;

// Where to drop the item relative to a category's separators.
struct TilesetInsertSpec {
	enum Kind {
		TOP,
		BOTTOM,
		ABOVE_SEPARATOR,
		BELOW_SEPARATOR,
	};

	Kind kind = BOTTOM;
	int separator_index = 0; // 0-based, only used for ABOVE/BELOW_SEPARATOR
};

// Adds (or moves) an item's RAW brush into a tileset category at the requested
// position, updating both the runtime tileset and its XML source file. Returns
// false and fills 'error' on failure.
bool AddItemToTilesetAt(uint16_t itemId, const std::string& tilesetName, TilesetCategoryType categoryType, const TilesetInsertSpec& spec, wxString& error);

// ============================================================================
// Add to Tileset window
// Pick a tileset and a position (relative to its separators) for an item.

class AddToTilesetWindow : public ObjectPropertiesWindowBase {
public:
	// Single-item convenience overload.
	AddToTilesetWindow(wxWindow* parent, uint16_t itemId, TilesetCategoryType defaultCategory = TILESET_RAW, wxPoint pos = wxDefaultPosition);
	// Multi-item overload — all items are added at the same tileset/position.
	AddToTilesetWindow(wxWindow* parent, std::vector<uint16_t> itemIds, TilesetCategoryType defaultCategory = TILESET_RAW, wxPoint pos = wxDefaultPosition);

	// Valid after a successful (OK) add - identifies what was changed so the
	// caller can refresh just that tileset page.
	const std::string& GetResultTileset() const {
		return result_tileset;
	}
	TilesetCategoryType GetResultCategory() const {
		return result_category;
	}

	void OnChangePalette(wxCommandEvent& event);
	void OnChangeTileset(wxCommandEvent& event);
	void OnClickOK(wxCommandEvent&);
	void OnClickCancel(wxCommandEvent&);

private:
	TilesetCategoryType GetSelectedCategory() const;
	void RebuildTilesetChoices();
	void RebuildPositionChoices();

	std::vector<uint16_t> item_ids;

	std::string result_tileset;
	TilesetCategoryType result_category = TILESET_RAW;

	wxChoice* palette_field;
	wxComboBox* tileset_field;
	wxChoice* position_field;

	std::vector<TilesetInsertSpec> position_specs;

	DECLARE_EVENT_TABLE();
};

#endif
