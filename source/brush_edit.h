//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_BRUSH_EDIT_H_
#define RME_BRUSH_EDIT_H_

#include "main.h"

class Brush;

enum BrushEditEntryKind {
	BRUSH_EDIT_ITEM,
	BRUSH_EDIT_COMPOSITE,
};

struct BrushEditEntry {
	BrushEditEntryKind kind = BRUSH_EDIT_ITEM;
	uint16_t item_id = 0;
	int chance = 1;
	std::string wall_alignment;
	std::vector<std::pair<Position, uint16_t>> composite_tiles;
};

bool BrushCanBeEdited(Brush* brush);
bool BrushExtractEditEntries(Brush* brush, std::vector<BrushEditEntry>& entries, wxString& error);
bool BrushApplyEditEntries(Brush* brush, const std::vector<BrushEditEntry>& entries, wxString& error);
bool BrushSaveToXml(Brush* brush, wxString& error);

#endif
