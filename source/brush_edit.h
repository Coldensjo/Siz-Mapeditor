//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_BRUSH_EDIT_H_
#define RME_BRUSH_EDIT_H_

#include "main.h"

class Brush;
class AutoBorder;

enum BrushEditEntryKind {
	BRUSH_EDIT_ITEM,
	BRUSH_EDIT_COMPOSITE,
	BRUSH_EDIT_GROUND_BORDER,
	BRUSH_EDIT_GROUND_OPTIONAL,
};

struct BrushEditEntry {
	BrushEditEntryKind kind = BRUSH_EDIT_ITEM;
	uint16_t item_id = 0;
	int chance = 1;
	std::string wall_alignment;
	std::vector<std::pair<Position, uint16_t>> composite_tiles;
	bool border_outer = true;
	std::string border_to = "all";
	uint32_t border_id = 0;
	bool border_super = false;
	uint16_t ground_equivalent = 0;
};

uint16_t BrushEditBorderPreviewId(uint32_t borderId);
std::vector<uint32_t> BrushGetAllBorderIds();
void DrawAutoBorderPreview(wxDC& dc, const wxRect& rect, const AutoBorder* border);

bool BrushCanBeEdited(Brush* brush);
bool BrushExtractEditEntries(Brush* brush, std::vector<BrushEditEntry>& entries, wxString& error);
bool BrushApplyEditEntries(Brush* brush, const std::vector<BrushEditEntry>& entries, wxString& error);
bool BrushSaveToXml(Brush* brush, wxString& error);

#endif
