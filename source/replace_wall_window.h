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

#ifndef RME_REPLACE_WALL_WINDOW_H_
#define RME_REPLACE_WALL_WINDOW_H_

#include "main.h"

class Editor;
class WallBrush;
class BrushPanel;

// Replaces every tile in the wall network connected to startPos with newBrush.
bool ReplaceConnectedWalls(Editor& editor, const Position& startPos, WallBrush* sourceBrush, WallBrush* newBrush);

class ReplaceWallDialog : public wxDialog {
public:
	ReplaceWallDialog(wxWindow* parent, Editor& editor, const Position& startPos, WallBrush* sourceBrush);
	~ReplaceWallDialog();

protected:
	void OnTilesetSelected(wxCommandEvent& event);
	void OnClickOK(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	void LoadSelectedTileset();
	bool TilesetHasWallBrushes(const std::string& tilesetName) const;
	std::string FindTilesetForBrush(WallBrush* brush) const;

	Editor& editor;
	Position start_pos;
	WallBrush* source_brush;

	wxChoice* tileset_field;
	BrushPanel* brush_panel;

	DECLARE_EVENT_TABLE();
};

#endif
