//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_EDIT_TILESET_WINDOW_H_
#define RME_EDIT_TILESET_WINDOW_H_

#include "main.h"
#include "common_windows.h"
#include "tileset_edit.h"

class Tileset;

void OpenTilesetEditor(Tileset* tileset, TilesetCategoryType categoryType);

class EditTilesetWindow;

class EditTilesetListBox : public wxVListBox {
public:
	EditTilesetListBox(wxWindow* parent, wxWindowID id, EditTilesetWindow* owner);

protected:
	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const;
	wxCoord OnMeasureItem(size_t n) const;

private:
	EditTilesetWindow* owner;
};

class EditTilesetWindow : public ObjectPropertiesWindowBase {
public:
	EditTilesetWindow(wxWindow* parent, Tileset* tileset, TilesetCategoryType categoryType);

	wxString FormatEntryLabel(const TilesetEditEntry& entry) const;

private:
	void RefreshList();
	void UpdateEditFields();
	void SetSelectedItemId(uint16_t itemId);

	void OnSelectionChanged(wxCommandEvent& event);
	void OnItemIdChanged(wxSpinEvent& event);
	void OnClickPickItem(wxCommandEvent& event);
	void OnClickAddItem(wxCommandEvent& event);
	void OnClickRemoveEntry(wxCommandEvent& event);
	void OnClickSave(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	friend class EditTilesetListBox;

	Tileset* edit_tileset;
	TilesetCategoryType category_type;
	std::vector<TilesetEditEntry> entries;
	EditTilesetListBox* entry_list;
	DCButton* item_preview_button;
	wxSpinCtrl* item_id_spin;
	wxButton* pick_item_button;
	wxStaticText* item_name_label;

	DECLARE_EVENT_TABLE()
};

#endif
