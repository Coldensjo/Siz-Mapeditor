//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_EDIT_BRUSH_WINDOW_H_
#define RME_EDIT_BRUSH_WINDOW_H_

#include "main.h"
#include "common_windows.h"
#include "brush_edit.h"

class Brush;
class DCButton;

void OpenBrushEditor(Brush* brush);

class EditBrushWindow;

class EditBrushListBox : public wxVListBox {
public:
	EditBrushListBox(wxWindow* parent, wxWindowID id, EditBrushWindow* owner);

protected:
	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
	wxCoord OnMeasureItem(size_t n) const;

private:
	EditBrushWindow* owner;
};

class EditCompositeTileListBox : public wxVListBox {
public:
	EditCompositeTileListBox(wxWindow* parent, wxWindowID id, EditBrushWindow* owner);

protected:
	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
	wxCoord OnMeasureItem(size_t n) const;

private:
	EditBrushWindow* owner;
};

class EditBrushWindow : public ObjectPropertiesWindowBase {
public:
	EditBrushWindow(wxWindow* parent, Brush* brush);

	BrushEditEntry* GetSelectedEntry();
	const BrushEditEntry* GetSelectedEntry() const;
	std::vector<std::pair<Position, uint16_t>>* GetSelectedCompositeTiles();

private:
	void RefreshList();
	void RefreshCompositeTileList();
	void UpdateEditFields();
	void SetSelectedItemId(uint16_t itemId);
	void UpdatePreviewSprite(const BrushEditEntry* entry, const std::pair<Position, uint16_t>* tile = nullptr);
	void UpdateCompositePanelVisibility();
	void ApplyBorderFieldsFromControls();
	wxString FormatEntryLabel(const BrushEditEntry& entry) const;
	wxString FormatCompositeTileLabel(const std::pair<Position, uint16_t>& tile) const;

	void OnSelectionChanged(wxCommandEvent& event);
	void OnCompositeTileSelectionChanged(wxCommandEvent& event);
	void OnChanceChanged(wxSpinEvent& event);
	void OnItemIdChanged(wxSpinEvent& event);
	void OnTilePositionChanged(wxSpinEvent& event);
	void OnBorderFieldChanged(wxCommandEvent& event);
	void OnBorderSpinChanged(wxSpinEvent& event);
	void OnClickPickItem(wxCommandEvent& event);
	void OnClickPickBorder(wxCommandEvent& event);
	void OnClickAddItem(wxCommandEvent& event);
	void OnClickAddComposite(wxCommandEvent& event);
	void OnClickAddBorder(wxCommandEvent& event);
	void OnClickAddOptional(wxCommandEvent& event);
	void OnClickAddTile(wxCommandEvent& event);
	void OnClickRemoveTile(wxCommandEvent& event);
	void OnClickRemoveEntry(wxCommandEvent& event);
	void OnClickSave(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	friend class EditBrushListBox;
	friend class EditCompositeTileListBox;

	Brush* edit_brush;
	std::vector<BrushEditEntry> entries;
	EditBrushListBox* entry_list;
	wxStaticBoxSizer* composite_panel;
	EditCompositeTileListBox* composite_tile_list;
	DCButton* item_preview_button;
	wxSpinCtrl* tile_x_spin;
	wxSpinCtrl* tile_y_spin;
	wxSpinCtrl* tile_z_spin;
	wxStaticText* tile_pos_label;
	wxChoice* border_align_choice;
	wxTextCtrl* border_to_text;
	wxSpinCtrl* border_id_spin;
	wxCheckBox* border_super_check;
	wxSpinCtrl* ground_equivalent_spin;
	wxStaticText* border_align_label;
	wxStaticText* border_to_label;
	wxStaticText* border_id_label;
	wxStaticText* ground_equivalent_label;
	wxSpinCtrl* item_id_spin;
	wxSpinCtrl* chance_spin;
	wxButton* pick_item_button;
	wxButton* pick_border_button;
	wxStaticText* item_name_label;
	wxButton* add_composite_button;
	wxButton* add_border_button;
	wxButton* add_optional_button;
	wxButton* add_tile_button;
	wxButton* remove_tile_button;

	DECLARE_EVENT_TABLE()
};

#endif
