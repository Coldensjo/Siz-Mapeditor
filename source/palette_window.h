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

#ifndef RME_PALETTE_H_
#define RME_PALETTE_H_

#include "palette_common.h"

class BrushPalettePanel;
class CreaturePalettePanel;
class HousePalettePanel;
class PaletteButton;

class PaletteWindow : public wxPanel {
public:
	wxChoicebook* GetChoicebook() const {
		return nullptr; // Deprecated, kept for compatibility
	}
	PaletteWindow(wxWindow* parent, const TilesetContainer& tilesets);
	~PaletteWindow();

	// Interface
	// Reloads layout g_settings from g_settings (and using map)
	void ReloadSettings(Map* from);
	// Show or hide Tools / Brush Size sections according to settings
	void UpdateToolPanelVisibility();
	// Flushes all pages and forces them to be reloaded from the palette data again
	void InvalidateContents();
	// Refreshes only the page of a single tileset in the given category (if this
	// palette holds it). Returns true if the page was found and refreshed.
	bool RefreshTilesetPage(TilesetCategoryType type, const std::string& tilesetName);
	// (Re)Loads all currently displayed data, called from InvalidateContents implicitly
	void LoadCurrentContents();
	// Goes to the selected page and selects any brush there
	void SelectPage(PaletteType palette);
	// The currently selected brush in this palette
	Brush* GetSelectedBrush() const;
	// The currently selected brush size in this palette
	int GetSelectedBrushSize() const;
	// The currently selected page (terrain, doodad...)
	PaletteType GetSelectedPage() const;
	// Get the house palette panel
	HousePalettePanel* GetHousePalette() const {
		return house_palette;
	}

	// Custom Event handlers (something has changed?)
	// Finds the brush pointed to by whatbrush and selects it as the current brush (also changes page)
	// Returns if the brush was found in this palette
	virtual bool OnSelectBrush(const Brush* whatbrush, PaletteType primary = TILESET_UNKNOWN);
	// Updates the palette window to use the current brush size
	virtual void OnUpdateBrushSize(BrushShape shape, int size);
	// Updates the content of the palette (eg. houses, creatures)
	virtual void OnUpdate(Map* map);

	// wxWidgets Event Handlers
	void OnButtonClick(wxCommandEvent& event);
	// Forward key events to the parent window (The Map Window)
	void OnKey(wxKeyEvent& event);
	void OnClose(wxCloseEvent&);

protected:
	static PalettePanel* CreateTerrainPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateDoodadPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateItemPalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateCreaturePalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateHousePalette(wxWindow* parent, const TilesetContainer& tilesets);
	static PalettePanel* CreateRAWPalette(wxWindow* parent, const TilesetContainer& tilesets);

	void ShowPalettePage(PaletteType type);
	void UpdateCategoryButtonStates();

	// Builds the left-side grid of tileset shortcut icons (2 columns).
	void CreateTilesetShortcutButtons(wxSizer* button_sizer);
	// Returns true if any brush palette holds a tileset with the given name.
	bool HasTilesetInAnyPalette(const std::string& tilesetName) const;
	// Jumps to the named tileset, preferring its Doodad page and falling back
	// to Terrain/Item/RAW. Returns true if the tileset was found and selected.
	bool GoToTileset(const std::string& tilesetName);

	wxPanel* page_container;
	PaletteButton* terrain_button;
	PaletteButton* doodad_button;
	PaletteButton* item_button;
	PaletteButton* house_button;
	PaletteButton* creature_button;
	PaletteButton* raw_button;
	PaletteType current_page;

	BrushPalettePanel* terrain_palette;
	BrushPalettePanel* doodad_palette;
	BrushPalettePanel* item_palette;
	CreaturePalettePanel* creature_palette;
	HousePalettePanel* house_palette;
	BrushPalettePanel* raw_palette;

	DECLARE_EVENT_TABLE()
};

#endif
