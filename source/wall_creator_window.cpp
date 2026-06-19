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

#include "items.h"
#include "item.h"
#include "brush.h"
#include "wall_brush.h"
#include "materials.h"
#include "tileset.h"

#include "gui.h"
#include "application.h"
#include "wall_creator_window.h"

namespace {

// The four pieces, in the order used by CreateWallBrush's pieceIds array.
constexpr int kWallPieceCount = 4;
const char* kWallPieceType[kWallPieceCount] = { "horizontal", "vertical", "corner", "pole" };
const char* kWallPieceLabel[kWallPieceCount] = { "Horizontal", "Vertical", "Corner", "Pole" };

std::string locateSourceFile(bool wantWall) {
	for (const auto& entry : g_brushes.getMap()) {
		Brush* brush = entry.second;
		if (!brush) {
			continue;
		}
		if (wantWall && brush->isWall() && !brush->getSourceFile().empty()) {
			return brush->getSourceFile();
		}
	}
	return std::string();
}

std::string locateTilesetSourceFile() {
	for (const auto& entry : g_materials.tilesets) {
		if (entry.second && !entry.second->getSourceFile().empty()) {
			return entry.second->getSourceFile();
		}
	}
	return std::string();
}

void persistTilesetMembership(const std::string& file, const std::string& tilesetName, const std::string& brushName) {
	pugi::xml_document doc;
	if (!doc.load_file(file.c_str())) {
		return;
	}

	pugi::xml_node root = doc.child("materials");
	if (!root) {
		return;
	}

	pugi::xml_node tilesetNode;
	for (pugi::xml_node child = root.first_child(); child; child = child.next_sibling()) {
		if (as_lower_str(child.name()) == "tileset" && tilesetName == child.attribute("name").as_string()) {
			tilesetNode = child;
			break;
		}
	}
	if (!tilesetNode) {
		tilesetNode = root.append_child("tileset");
		tilesetNode.append_attribute("name") = tilesetName.c_str();
	}

	pugi::xml_node terrainNode = tilesetNode.child("terrain");
	if (!terrainNode) {
		terrainNode = tilesetNode.append_child("terrain");
	}

	pugi::xml_node brushNode = terrainNode.append_child("brush");
	brushNode.append_attribute("name") = brushName.c_str();

	doc.save_file(file.c_str());
}

} // namespace

bool CreateWallBrush(const std::string& name, const uint16_t pieceIds[4], const std::string& tilesetName, wxString& error) {
	if (name.empty()) {
		error = "Please enter a name for the wall.";
		return false;
	}
	if (name == "all" || name == "none") {
		error = "'all' and 'none' are reserved names.";
		return false;
	}
	if (g_brushes.getBrush(name)) {
		error = "A brush named '" + wxstr(name) + "' already exists.";
		return false;
	}

	for (int i = 0; i < kWallPieceCount; ++i) {
		if (pieceIds[i] == 0 || g_items.getItemType(pieceIds[i]).id == 0) {
			error = wxString::Format("Please choose a valid item for the '%s' piece.", kWallPieceLabel[i]);
			return false;
		}
	}

	if (tilesetName.empty()) {
		error = "Please choose or type a tileset to add the wall to.";
		return false;
	}

	std::string wallsFile = locateSourceFile(true);
	if (wallsFile.empty()) {
		error = "Could not locate walls.xml to save the new wall.";
		return false;
	}

	pugi::xml_document doc;
	if (!doc.load_file(wallsFile.c_str())) {
		error = wxString::Format("Could not open '%s'.", wxstr(wallsFile));
		return false;
	}

	pugi::xml_node root = doc.child("materials");
	if (!root) {
		error = "Invalid walls.xml (missing <materials> root).";
		return false;
	}

	// Brush definitions must come before any <tileset> block that references
	// them, otherwise the tileset loader can't find the brush.
	pugi::xml_node firstTileset;
	for (pugi::xml_node child = root.first_child(); child; child = child.next_sibling()) {
		if (as_lower_str(child.name()) == "tileset") {
			firstTileset = child;
			break;
		}
	}

	pugi::xml_node brushNode = firstTileset ? root.insert_child_before("brush", firstTileset) : root.append_child("brush");
	brushNode.append_attribute("name") = name.c_str();
	brushNode.append_attribute("type") = "wall";
	brushNode.append_attribute("server_lookid") = pieceIds[0];

	for (int i = 0; i < kWallPieceCount; ++i) {
		pugi::xml_node wallNode = brushNode.append_child("wall");
		wallNode.append_attribute("type") = kWallPieceType[i];
		pugi::xml_node itemNode = wallNode.append_child("item");
		itemNode.append_attribute("id") = pieceIds[i];
		itemNode.append_attribute("chance") = 100;
	}

	const std::string previousLoad = g_brushes.getCurrentLoadFile();
	g_brushes.setCurrentLoadFile(wallsFile);
	wxArrayString warnings;
	bool registered = g_brushes.unserializeBrush(brushNode, warnings);
	g_brushes.setCurrentLoadFile(previousLoad);

	Brush* brush = g_brushes.getBrush(name);
	if (!registered || !brush || !brush->isWall()) {
		wxString detail;
		for (size_t i = 0; i < warnings.GetCount(); ++i) {
			detail += "\n" + warnings[i];
		}
		error = "Failed to register the new wall brush." + detail;
		return false;
	}

	if (!doc.save_file(wallsFile.c_str())) {
		error = wxString::Format("Could not save '%s'.", wxstr(wallsFile));
		return false;
	}

	Tileset* tileset;
	std::string tilesetFile;
	auto it = g_materials.tilesets.find(tilesetName);
	if (it != g_materials.tilesets.end()) {
		tileset = it->second;
		tilesetFile = tileset->getSourceFile();
	} else {
		tileset = newd Tileset(g_brushes, tilesetName);
		g_materials.tilesets.insert(std::make_pair(tilesetName, tileset));
		tilesetFile = locateTilesetSourceFile();
		tileset->setSourceFile(tilesetFile);
	}

	TilesetCategory* category = tileset->getCategory(TILESET_TERRAIN);
	if (!category->containsBrush(brush)) {
		brush->flagAsVisible();
		category->brushlist.push_back(brush);
	}

	if (!tilesetFile.empty()) {
		persistTilesetMembership(tilesetFile, tilesetName, name);
	}

	g_materials.modify();
	return true;
}
