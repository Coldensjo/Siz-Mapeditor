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

#ifndef RME_WALL_CREATOR_WINDOW_H_
#define RME_WALL_CREATOR_WINDOW_H_

#include "main.h"

// Creates and registers a new wall brush from four piece item ids, saves it to
// walls.xml (inserted before the tileset block), and adds it to the given
// tileset. The four ids are ordered: horizontal, vertical, corner, pole.
// Returns false and fills 'error' on failure.
bool CreateWallBrush(const std::string& name, const uint16_t pieceIds[4], const std::string& tilesetName, wxString& error);

#endif
