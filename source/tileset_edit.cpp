//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "tileset_edit.h"
#include "brush.h"
#include "items.h"
#include "materials.h"
#include "raw_brush.h"

namespace {

pugi::xml_node findTilesetNode(pugi::xml_node root, const std::string& tilesetName) {
	for (pugi::xml_node child = root.first_child(); child; child = child.next_sibling()) {
		if (as_lower_str(child.name()) != "tileset") {
			continue;
		}
		pugi::xml_attribute attribute = child.attribute("name");
		if (attribute && attribute.as_string() == tilesetName) {
			return child;
		}
	}
	return pugi::xml_node();
}

pugi::xml_node findTilesetNodeRecursive(pugi::xml_node node, const std::string& tilesetName) {
	if (pugi::xml_node found = findTilesetNode(node, tilesetName)) {
		return found;
	}
	for (pugi::xml_node child = node.first_child(); child; child = child.next_sibling()) {
		if (as_lower_str(child.name()) == "include") {
			continue;
		}
		if (pugi::xml_node found = findTilesetNodeRecursive(child, tilesetName)) {
			return found;
		}
	}
	return pugi::xml_node();
}

void writeCompactItems(pugi::xml_node parent, const std::vector<uint16_t>& sortedIds) {
	if (sortedIds.empty()) {
		return;
	}

	uint16_t rangeStart = sortedIds.front();
	uint16_t rangeEnd = sortedIds.front();
	for (size_t i = 1; i <= sortedIds.size(); ++i) {
		if (i < sortedIds.size() && sortedIds[i] == static_cast<uint16_t>(rangeEnd + 1)) {
			rangeEnd = sortedIds[i];
			continue;
		}

		pugi::xml_node item = parent.append_child("item");
		if (rangeStart == rangeEnd) {
			item.append_attribute("id") = rangeStart;
		} else {
			item.append_attribute("fromid") = rangeStart;
			item.append_attribute("toid") = rangeEnd;
		}

		if (i < sortedIds.size()) {
			rangeStart = rangeEnd = sortedIds[i];
		}
	}
}

void deleteUniquePaletteSeparators(const BrushVector& brushes, std::vector<Brush*>& deleted) {
	for (Brush* brush : brushes) {
		if (brush && brush->isPaletteSeparator()) {
			if (std::find(deleted.begin(), deleted.end(), brush) == deleted.end()) {
				delete brush;
				deleted.push_back(brush);
			}
		}
	}
}

void syncMirroredCategories(Tileset* tileset, const std::string& xmlTag, const BrushVector& brushes) {
	std::vector<Brush*> deletedSeparators;
	for (TilesetCategory* category : tileset->categories) {
		if (category && category->getXmlSourceTag() == xmlTag) {
			deleteUniquePaletteSeparators(category->brushlist, deletedSeparators);
			category->brushlist = brushes;
		}
	}
}

void deletePaletteSeparators(BrushVector& brushes) {
	std::vector<Brush*> deleted;
	deleteUniquePaletteSeparators(brushes, deleted);
}

} // namespace

bool TilesetExtractEditEntries(const TilesetCategory* category, std::vector<TilesetEditEntry>& entries, wxString& error) {
	entries.clear();
	if (!category) {
		error = "No tileset category selected.";
		return false;
	}

	for (Brush* brush : category->brushlist) {
		if (!brush) {
			continue;
		}
		if (brush->isPaletteSeparator()) {
			TilesetEditEntry entry;
			entry.kind = TilesetEditEntry::SEPARATOR;
			entries.push_back(entry);
		} else if (brush->isRaw()) {
			TilesetEditEntry entry;
			entry.kind = TilesetEditEntry::ITEM;
			entry.item_id = brush->asRaw()->getItemID();
			entries.push_back(entry);
		} else {
			TilesetEditEntry entry;
			entry.kind = TilesetEditEntry::BRUSH;
			entry.brush_name = brush->getName();
			entries.push_back(entry);
		}
	}
	return true;
}

bool TilesetApplyEditEntries(Tileset* tileset, TilesetCategoryType categoryType, const std::vector<TilesetEditEntry>& entries, wxString& error) {
	if (!tileset) {
		error = "No tileset selected.";
		return false;
	}

	TilesetCategory* category = tileset->getCategory(categoryType);
	if (!category) {
		error = "Tileset category not found.";
		return false;
	}

	BrushVector newBrushes;
	for (const TilesetEditEntry& entry : entries) {
		if (entry.kind == TilesetEditEntry::SEPARATOR) {
			newBrushes.push_back(newd PaletteSeparatorBrush());
		} else if (entry.kind == TilesetEditEntry::ITEM) {
			if (entry.item_id == 0 || g_items.getItemType(entry.item_id).id == 0) {
				error = wxString::Format("Invalid item id #%u.", entry.item_id);
				return false;
			}

			ItemType& it = g_items[entry.item_id];
			RAWBrush* brush;
			if (it.raw_brush) {
				brush = it.raw_brush;
			} else {
				brush = it.raw_brush = newd RAWBrush(it.id);
				it.has_raw = true;
				g_brushes.addBrush(it.raw_brush);
			}
			brush->flagAsVisible();
			it.in_other_tileset = true;
			newBrushes.push_back(brush);
		} else {
			Brush* brush = g_brushes.getBrush(entry.brush_name);
			if (!brush) {
				error = wxString::Format("Brush '%s' does not exist.", wxstr(entry.brush_name));
				return false;
			}
			brush->flagAsVisible();
			newBrushes.push_back(brush);
		}
	}

	const std::string xmlTag = category->getXmlSourceTag();
	if (!xmlTag.empty()) {
		syncMirroredCategories(tileset, xmlTag, newBrushes);
	} else {
		deletePaletteSeparators(category->brushlist);
		category->brushlist = newBrushes;
	}

	return true;
}

bool TilesetSaveToXml(Tileset* tileset, TilesetCategoryType categoryType, wxString& error) {
	if (!tileset) {
		error = "No tileset selected.";
		return false;
	}

	const std::string& sourceFile = tileset->getSourceFile();
	if (sourceFile.empty()) {
		error = "This tileset has no known source XML file.";
		return false;
	}

	TilesetCategory* category = tileset->getCategory(categoryType);
	if (!category) {
		error = "Tileset category not found.";
		return false;
	}

	const std::string& xmlTag = category->getXmlSourceTag();
	if (xmlTag.empty()) {
		error = "Cannot determine the XML section for this tileset category.";
		return false;
	}

	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(sourceFile.c_str());
	if (!result) {
		error = wxString::Format("Could not open '%s' for writing.", wxstr(sourceFile));
		return false;
	}

	pugi::xml_node root = doc.child("materials");
	if (!root) {
		error = "Invalid materials file root.";
		return false;
	}

	pugi::xml_node tilesetNode = findTilesetNode(root, tileset->name);
	if (!tilesetNode) {
		error = wxString::Format("Could not find tileset '%s' in '%s'.", wxstr(tileset->name), wxstr(sourceFile));
		return false;
	}

	pugi::xml_node categoryNode = tilesetNode.child(xmlTag.c_str());
	if (!categoryNode) {
		categoryNode = tilesetNode.append_child(xmlTag.c_str());
	}

	for (pugi::xml_node child = categoryNode.first_child(); child;) {
		pugi::xml_node next = child.next_sibling();
		categoryNode.remove_child(child);
		child = next;
	}

	for (Brush* brush : category->brushlist) {
		if (!brush) {
			continue;
		}
		if (brush->isPaletteSeparator()) {
			categoryNode.append_child("separator");
			continue;
		}
		if (brush->isRaw()) {
			pugi::xml_node itemNode = categoryNode.append_child("item");
			itemNode.append_attribute("id") = brush->asRaw()->getItemID();
		} else {
			pugi::xml_node brushNode = categoryNode.append_child("brush");
			brushNode.append_attribute("name") = brush->getName().c_str();
		}
	}

	if (!doc.save_file(sourceFile.c_str())) {
		error = wxString::Format("Could not save '%s'.", wxstr(sourceFile));
		return false;
	}

	return true;
}
