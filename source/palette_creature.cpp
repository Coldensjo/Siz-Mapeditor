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

#include "main.h"

#include "settings.h"
#include "brush.h"
#include "gui.h"
#include "palette_creature.h"
#include "creature_brush.h"
#include "creatures.h"
#include "items.h"
#include "spawn_brush.h"
#include "materials.h"

#include <algorithm>
#include <tuple>
#include <utility>
#include <vector>

#include <wx/dcclient.h>

class CreatureListBox : public wxVListBox {
public:
	CreatureListBox(wxWindow* parent, wxWindowID id);

	void ClearEntries();
	void Append(const wxString& label, Brush* brush);
	void Sort();
	void SelectAll(); // Override to prevent assertion for single-selection listbox

	Brush* GetBrush(size_t index) const;
	Brush* GetSelectedBrush() const;
	bool SetSelectionByName(const std::string& name);
	size_t GetCount() const;
	void OnChar(wxKeyEvent& event); // Intercept Ctrl+A to prevent SelectAll assertion

private:
	struct Entry {
		wxString name;
		Brush* brush;
	};

	using SpriteList = std::vector<Sprite*>;

	std::pair<int, int> GetSpriteDimensions(Sprite* sprite) const;
	void CollectEntrySprites(const Entry& entry, SpriteList& sprites) const;

	void RefreshAfterChange();

protected:
	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const override;
	wxCoord OnMeasureItem(size_t index) const override;

private:
	std::vector<Entry> entries;
};

CreatureListBox::CreatureListBox(wxWindow* parent, wxWindowID id) :
	wxVListBox(parent, id, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE | wxWANTS_CHARS) {
	SetItemCount(0);
	Bind(wxEVT_CHAR, &CreatureListBox::OnChar, this);
}

void CreatureListBox::SelectAll() {
	// No-op for single-selection listbox to prevent assertion
}

void CreatureListBox::OnChar(wxKeyEvent& event) {
	// Intercept Ctrl+A to prevent SelectAll assertion for single-selection listbox
	if (isSelectAllShortcut(event)) {
		// Do nothing - prevent base class from calling SelectAll
		return;
	}
	event.Skip();
}

void CreatureListBox::ClearEntries() {
	SetSelection(wxNOT_FOUND);
	entries.clear();
	SetItemCount(0);
	RefreshAll();
}

void CreatureListBox::Append(const wxString& label, Brush* brush) {
	entries.push_back({ label, brush });
	SetItemCount(entries.size());
}

Brush* CreatureListBox::GetBrush(size_t index) const {
	if (index >= entries.size()) {
		return nullptr;
	}
	return entries[index].brush;
}

Brush* CreatureListBox::GetSelectedBrush() const {
	int selection = GetSelection();
	if (selection == wxNOT_FOUND) {
		if (entries.empty()) {
			return nullptr;
		}
		selection = 0;
	}
	return GetBrush(static_cast<size_t>(selection));
}

bool CreatureListBox::SetSelectionByName(const std::string& name) {
	if (entries.empty()) {
		return false;
	}

	wxString target(wxstr(name));
	for (size_t index = 0; index < entries.size(); ++index) {
		if (entries[index].name.IsSameAs(target, false)) {
			SetSelection(index);
			return true;
		}
	}

	SetSelection(0);
	return false;
}

void CreatureListBox::Sort() {
	if (entries.empty()) {
		RefreshAfterChange();
		return;
	}

	Brush* selected = GetSelectedBrush();
	std::stable_sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
		return a.name.CmpNoCase(b.name) < 0;
	});

	if (selected) {
		for (size_t index = 0; index < entries.size(); ++index) {
			if (entries[index].brush == selected) {
				SetSelection(index);
				break;
			}
		}
	}

	RefreshAfterChange();
}

std::pair<int, int> CreatureListBox::GetSpriteDimensions(Sprite* sprite) const {
	int width = 32;
	int height = 32;
	if (GameSprite* game_sprite = dynamic_cast<GameSprite*>(sprite)) {
		int actual_width = std::max<int>(1, int(game_sprite->width)) * SPRITE_PIXELS;
		int actual_height = std::max<int>(1, int(game_sprite->height)) * SPRITE_PIXELS;
		int draw_height = game_sprite->getDrawHeight();
		if (draw_height > 0) {
			actual_height = std::max(actual_height, draw_height);
		}
		width = std::max<int>(32, actual_width);
		height = std::max<int>(32, actual_height);
		// Scale 32x32 sprites (1x1 tiles) up to 64x64 to match 64x64 sprites (2x2 tiles)
		if (width == SPRITE_PIXELS && height == SPRITE_PIXELS) {
			width = 64;
			height = 64;
		}
	}
	return { width, height };
}

void CreatureListBox::CollectEntrySprites(const Entry& entry, SpriteList& sprites) const {
	sprites.clear();
	Brush* brush = entry.brush;
	if (!brush) {
		return;
	}

	if (brush->isCreature()) {
		CreatureBrush* creature_brush = dynamic_cast<CreatureBrush*>(brush);
		if (!creature_brush) {
			return;
		}

		CreatureType* type = creature_brush->getType();
		if (!type) {
			return;
		}

		const Outfit& outfit = type->outfit;
		if (outfit.lookItem != 0 && g_items.typeExists(outfit.lookItem)) {
			ItemType& item = g_items.getItemType(outfit.lookItem);
			if (item.sprite) {
				sprites.push_back(item.sprite);
			}
			return;
		}

		if (outfit.lookMount != 0) {
			if (Sprite* mount_sprite = g_gui.gfx.getCreatureSprite(outfit.lookMount)) {
				sprites.push_back(mount_sprite);
			}
		}

		if (outfit.lookType != 0) {
			if (Sprite* creature_sprite = g_gui.gfx.getCreatureSprite(outfit.lookType)) {
				sprites.push_back(creature_sprite);
			}
		}
		return;
	}

	int look_id = brush->getLookID();
	if (look_id != 0) {
		if (Sprite* sprite = g_gui.gfx.getSprite(look_id)) {
			sprites.push_back(sprite);
		}
	}
}

void CreatureListBox::RefreshAfterChange() {
	SetItemCount(entries.size());
	RefreshAll();
}

size_t CreatureListBox::GetCount() const {
	return entries.size();
}

void CreatureListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const {
	ASSERT(index < entries.size());
	const Entry& entry = entries[index];
	SpriteList sprites;
	CollectEntrySprites(entry, sprites);

	int icon_width = 32;
	int icon_height = 32;
	for (Sprite* sprite : sprites) {
		if (!sprite) {
			continue;
		}
		auto [sprite_width, sprite_height] = GetSpriteDimensions(sprite);
		icon_width = std::max(icon_width, sprite_width);
		icon_height = std::max(icon_height, sprite_height);
	}

	int icon_x = rect.GetX();
	int icon_y = rect.GetY() + (rect.GetHeight() - icon_height);
	for (Sprite* sprite : sprites) {
		if (!sprite) {
			continue;
		}
		auto [sprite_width, sprite_height] = GetSpriteDimensions(sprite);
		int draw_y = icon_y + (icon_height - sprite_height);
		
		// For 32x32 sprites (1x1 tiles), scale up to 64x64 to match 64x64 sprites (2x2 tiles)
		int draw_width = sprite_width;
		int draw_height = sprite_height;
		if (GameSprite* game_sprite = dynamic_cast<GameSprite*>(sprite)) {
			int actual_width = std::max<int>(1, int(game_sprite->width)) * SPRITE_PIXELS;
			int actual_height = std::max<int>(1, int(game_sprite->height)) * SPRITE_PIXELS;
			if (actual_width == SPRITE_PIXELS && actual_height == SPRITE_PIXELS) {
				// This is a 32x32 sprite, scale it to 64x64
				draw_width = 64;
				draw_height = 64;
			}
		}
		
		sprite->DrawTo(&dc, SPRITE_SIZE_ACTUAL, icon_x, draw_y, draw_width, draw_height);
	}

	if (IsSelected(index)) {
		if (HasFocus()) {
			dc.SetTextForeground(wxColour(0xFF, 0xFF, 0xFF));
		} else {
			dc.SetTextForeground(wxColour(0x00, 0x00, 0xFF));
		}
	} else {
		dc.SetTextForeground(wxColour(0x00, 0x00, 0x00));
	}

	int text_width;
	int text_height;
	dc.GetTextExtent(entries[index].name, &text_width, &text_height);
	int text_x = icon_x + icon_width + 8;
	int text_y = rect.GetY() + (rect.GetHeight() - text_height) / 2;
	dc.DrawText(entries[index].name, text_x, text_y);
}

wxCoord CreatureListBox::OnMeasureItem(size_t index) const {
	ASSERT(index < entries.size());
	const Entry& entry = entries[index];
	SpriteList sprites;
	CollectEntrySprites(entry, sprites);

	int icon_height = 32;
	for (Sprite* sprite : sprites) {
		if (!sprite) {
			continue;
		}
		icon_height = std::max(icon_height, GetSpriteDimensions(sprite).second);
	}

	int text_height = 0;
	if (wxWindow* window = const_cast<CreatureListBox*>(this)) {
		wxClientDC dc(window);
		wxCoord w;
		dc.GetTextExtent(entries[index].name, &w, &text_height);
	}
	return std::max<int>(icon_height, text_height) + 4;
}

// ============================================================================
// Creature palette

BEGIN_EVENT_TABLE(CreaturePalettePanel, PalettePanel)
EVT_CHOICE(PALETTE_CREATURE_TILESET_CHOICE, CreaturePalettePanel::OnTilesetChange)

EVT_LISTBOX(PALETTE_CREATURE_LISTBOX, CreaturePalettePanel::OnListBoxChange)

EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_TIME, CreaturePalettePanel::OnChangeSpawnTime)
EVT_SPINCTRL(PALETTE_CREATURE_SPAWN_SIZE, CreaturePalettePanel::OnChangeSpawnSize)
END_EVENT_TABLE()

CreaturePalettePanel::CreaturePalettePanel(wxWindow* parent, wxWindowID id) :
	PalettePanel(parent, id),
	handling_event(false) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxSizer* sidesizer = newd wxStaticBoxSizer(wxVERTICAL, this, "Creatures");
	tileset_choice = newd wxChoice(this, PALETTE_CREATURE_TILESET_CHOICE, wxDefaultPosition, wxDefaultSize, (int)0, (const wxString*)nullptr);
	sidesizer->Add(tileset_choice, 0, wxEXPAND);

	creature_list = newd CreatureListBox(this, PALETTE_CREATURE_LISTBOX);
	sidesizer->Add(creature_list, 1, wxEXPAND);
	topsizer->Add(sidesizer, 1, wxEXPAND);

	// Brush selection
	sidesizer = newd wxStaticBoxSizer(newd wxStaticBox(this, wxID_ANY, "Brushes", wxDefaultPosition, wxSize(150, 200)), wxVERTICAL);

	wxFlexGridSizer* grid = newd wxFlexGridSizer(2, 10, 10);
	grid->AddGrowableCol(1);

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawntime"));
	creature_spawntime_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_TIME, i2ws(g_settings.getInteger(Config::DEFAULT_SPAWNTIME)), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 0, 86400, g_settings.getInteger(Config::DEFAULT_SPAWNTIME));
	grid->Add(creature_spawntime_spin, 0, wxEXPAND);
	g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());

	grid->Add(newd wxStaticText(this, wxID_ANY, "Spawn size"));
	spawn_size_spin = newd wxSpinCtrl(this, PALETTE_CREATURE_SPAWN_SIZE, i2ws(5), wxDefaultPosition, wxSize(50, 20), wxSP_ARROW_KEYS, 1, g_settings.getInteger(Config::MAX_SPAWN_RADIUS), g_settings.getInteger(Config::CURRENT_SPAWN_RADIUS));
	grid->Add(spawn_size_spin, 0, wxEXPAND);
	grid->AddSpacer(0);

	sidesizer->Add(grid, 0, wxEXPAND);
	topsizer->Add(sidesizer, 0, wxEXPAND);
	SetSizerAndFit(topsizer);

	OnUpdate();
}

CreaturePalettePanel::~CreaturePalettePanel() {
	////
}

PaletteType CreaturePalettePanel::GetType() const {
	return TILESET_CREATURE;
}

void CreaturePalettePanel::SelectFirstBrush() {
	if (creature_list->GetCount() > 0) {
		creature_list->SetSelection(0);
	}
}

Brush* CreaturePalettePanel::GetSelectedBrush() const {
	if (creature_list->GetCount() == 0) {
		return nullptr;
	}

	Brush* brush = creature_list->GetSelectedBrush();
	if (brush && brush->isCreature()) {
		g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
		g_settings.setInteger(Config::CURRENT_SPAWN_RADIUS, spawn_size_spin->GetValue());
		g_settings.setInteger(Config::DEFAULT_SPAWNTIME, creature_spawntime_spin->GetValue());
		return brush;
	}
	return nullptr;
}

bool CreaturePalettePanel::SelectBrush(const Brush* whatbrush) {
	if (!whatbrush) {
		return false;
	}

	if (whatbrush->isCreature()) {
		int current_index = tileset_choice->GetSelection();
		if (current_index != wxNOT_FOUND) {
			const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(current_index));
			if (tsc) {
				for (BrushVector::const_iterator iter = tsc->brushlist.begin(); iter != tsc->brushlist.end(); ++iter) {
					if (*iter == whatbrush) {
						SelectCreature(whatbrush->getName());
						return true;
					}
				}
			}
		}
		// Not in the current display, search the hidden one's
		for (size_t i = 0; i < tileset_choice->GetCount(); ++i) {
			if (current_index != (int)i) {
				const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(i));
				if (tsc) {
					for (BrushVector::const_iterator iter = tsc->brushlist.begin();
						 iter != tsc->brushlist.end();
						 ++iter) {
						if (*iter == whatbrush) {
							SelectTileset(i);
							SelectCreature(whatbrush->getName());
							g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
							return true;
						}
					}
				}
			}
		}
	} else if (whatbrush->isSpawn()) {
		// Spawn brush is a special brush that doesn't need to be selected from a list
		// Just return true to indicate successful selection
		return true;
	}
	return false;
}

int CreaturePalettePanel::GetSelectedBrushSize() const {
	return spawn_size_spin->GetValue();
}

void CreaturePalettePanel::OnUpdate() {
	tileset_choice->Clear();
	g_materials.createOtherTileset();

	for (TilesetContainer::const_iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		const TilesetCategory* tsc = iter->second->getCategory(TILESET_CREATURE);
		if (tsc && tsc->size() > 0) {
			tileset_choice->Append(wxstr(iter->second->name), const_cast<TilesetCategory*>(tsc));
		} else if (iter->second->name == "NPCs" || iter->second->name == "Others") {
			Tileset* ts = const_cast<Tileset*>(iter->second);
			TilesetCategory* rtsc = ts->getCategory(TILESET_CREATURE);
			tileset_choice->Append(wxstr(ts->name), rtsc);
		}
	}
	SelectTileset(0);
}

void CreaturePalettePanel::OnUpdateBrushSize(BrushShape shape, int size) {
	const int value = size >= 0 ? size : g_gui.GetBrushSize();
	return spawn_size_spin->SetValue(value);
}

void CreaturePalettePanel::OnSwitchIn() {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(creature_spawntime_spin->GetValue());
	g_gui.SetBrushSize(spawn_size_spin->GetValue());
}

void CreaturePalettePanel::SelectTileset(size_t index) {
	ASSERT(tileset_choice->GetCount() >= index);

	creature_list->ClearEntries();
	if (tileset_choice->GetCount() == 0) {
		// No tilesets
		return;
	}

	const TilesetCategory* tsc = reinterpret_cast<const TilesetCategory*>(tileset_choice->GetClientData(index));
	if (tsc) {
		for (BrushVector::const_iterator iter = tsc->brushlist.begin();
			 iter != tsc->brushlist.end();
			 ++iter) {
			creature_list->Append(wxstr((*iter)->getName()), *iter);
		}
		creature_list->Sort();
		SelectCreature(0);

		tileset_choice->SetSelection(index);
	}
}

void CreaturePalettePanel::SelectCreature(size_t index) {
	ASSERT(creature_list->GetCount() >= index);

	if (creature_list->GetCount() > 0) {
		creature_list->SetSelection(index);
	}
}

void CreaturePalettePanel::SelectCreature(std::string name) {
	if (creature_list->GetCount() > 0) {
		if (!creature_list->SetSelectionByName(name)) {
			creature_list->SetSelection(0);
		}
	}
}

void CreaturePalettePanel::OnTilesetChange(wxCommandEvent& event) {
	SelectTileset(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnListBoxChange(wxCommandEvent& event) {
	SelectCreature(event.GetSelection());
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SelectBrush();
}

void CreaturePalettePanel::OnChangeSpawnTime(wxSpinEvent& event) {
	g_gui.ActivatePalette(GetParentPalette());
	g_gui.SetSpawnTime(event.GetPosition());
}

void CreaturePalettePanel::OnChangeSpawnSize(wxSpinEvent& event) {
	if (!handling_event) {
		handling_event = true;
		g_gui.ActivatePalette(GetParentPalette());
		g_gui.SetBrushSize(event.GetPosition());
		handling_event = false;
	}
}
