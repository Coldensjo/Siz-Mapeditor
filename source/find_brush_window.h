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

#ifndef RME_FIND_BRUSH_WINDOW_H_
#define RME_FIND_BRUSH_WINDOW_H_

#include "main.h"
#include "tileset.h"

class BrushIconBox;
class KeyForwardingTextCtrl;

// A non-modal, dockable brush search panel. Unlike the "Jump to Brush" dialog
// this one stays open after picking a brush, so the user can keep searching
// and painting without reopening it every time.
class FindBrushWindow : public wxPanel {
public:
	FindBrushWindow(wxWindow* parent);
	~FindBrushWindow();

	// Clears the search box and results, then focuses the search field -
	// used when the window is (re)activated via its hotkey.
	void ResetSearch();
	// Focuses the search field and selects its contents.
	void FocusSearch();
	// Drops all search results without touching focus - the brushes/items
	// they point to are about to be (or were just) freed by a client version
	// reload, so any held Brush* pointers would otherwise dangle. Called by
	// GUI::UnloadVersion() before it frees g_brushes/g_items.
	void InvalidateResults();

protected:
	void OnTextChange(wxCommandEvent& event);
	void OnTextIdle(wxTimerEvent& event);
	void OnSearchEnter(wxCommandEvent& event);
	void OnSearchSetFocus(wxFocusEvent& event);
	void OnSearchKillFocus(wxFocusEvent& event);

	void RefreshResults();
	void RebuildIconBox();
	void PickBrush(Brush* brush);

	KeyForwardingTextCtrl* search_field;
	wxStaticText* status_label;
	wxPanel* results_panel;
	wxSizer* results_sizer;
	BrushIconBox* icon_box;
	wxTimer idle_input_timer;

	// The main frame's menu accelerators include many bare, unmodified letter
	// keys (T, D, I, F, H, ...). Since this panel is docked inside that same
	// frame (unlike the modal "Jump to Brush" dialog), those accelerators
	// would otherwise steal every matching keystroke before it reaches the
	// search field. GUI::DisableHotkeys() makes MainFrame::MSWTranslateMessage
	// skip menu-accelerator matching entirely while the field has focus.
	bool hotkeys_disabled_by_us;

	// Ad-hoc, non-persisted tileset holding only the brushes matching the
	// current search string - BrushIconBox needs a TilesetCategory to draw.
	Tileset result_tileset;
	TilesetCategory* result_category;
};

#endif
