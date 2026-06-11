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

#ifndef RME_REPLACE_ITEMS_WINDOW_H_
#define RME_REPLACE_ITEMS_WINDOW_H_

#include "main.h"
#include "common_windows.h"
#include "editor.h"

struct ReplacingItem {
	ReplacingItem() :
		replaceId(0), replaceIdEnd(0), withId(0), secondaryId(0), secondaryPosition(false), total(0), complete(false), isRange(false) { }

	bool operator==(const ReplacingItem& other) const {
		return replaceId == other.replaceId && replaceIdEnd == other.replaceIdEnd && withId == other.withId && secondaryId == other.secondaryId && secondaryPosition == other.secondaryPosition;
	}

	uint16_t replaceId;
	uint16_t replaceIdEnd; // For range selection, 0 means single item
	uint16_t withId;
	uint16_t secondaryId; // Secondary item to place above/below, 0 means none
	bool secondaryPosition; // true for "above", false for "below"
	uint32_t total;
	bool complete;
	bool isRange; // true if replaceIdEnd > 0
};

// ============================================================================
// ReplaceItemsButton

class ReplaceItemsButton : public DCButton {
public:
	ReplaceItemsButton(wxWindow* parent);
	~ReplaceItemsButton() { }

	ItemGroup_t GetGroup() const;
	uint16_t GetItemId() const {
		return m_id;
	}
	void SetItemId(uint16_t id);

private:
	uint16_t m_id;
};

// ============================================================================
// ReplaceItemsListBox

class ReplaceItemsListBox : public wxVListBox {
public:
	ReplaceItemsListBox(wxWindow* parent);

	bool AddItem(const ReplacingItem& item);
	void MarkAsComplete(const ReplacingItem& item, uint32_t total);
	void RemoveSelected();
	bool CanAdd(uint16_t replaceId, uint16_t replaceIdEnd, uint16_t withId) const;
	void SelectAll(); // Override to prevent assertion for single-selection listbox

	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const;
	wxCoord OnMeasureItem(size_t index) const;
	void OnChar(wxKeyEvent& event); // Intercept Ctrl+A to prevent SelectAll assertion

	const std::vector<ReplacingItem>& GetItems() const {
		return m_items;
	}
	size_t GetCount() const {
		return m_items.size();
	}
	const ReplacingItem* GetItemAt(int index) const {
		if (index >= 0 && index < static_cast<int>(m_items.size())) {
			return &m_items.at(index);
		}
		return nullptr;
	}

private:
	std::vector<ReplacingItem> m_items;
	wxBitmap m_arrow_bitmap;
	wxBitmap m_flag_bitmap;
};

// ============================================================================
// ReplaceItemsDialog

struct ItemFinder {
	ItemFinder(uint16_t itemid, uint16_t itemidEnd, int32_t limit = -1) : itemid(itemid), itemidEnd(itemidEnd), limit(limit), exceeded(false) { }

	void operator()(Map& map, Tile* tile, Item* item, long long done) {
		if (exceeded) {
			return;
		}

		uint16_t id = item->getID();
		bool matches = false;
		if (itemidEnd == 0 || itemidEnd == itemid) {
			// Single item
			matches = (id == itemid);
		} else {
			// Range
			matches = (id >= itemid && id <= itemidEnd);
		}

		if (matches) {
			result.push_back(std::make_pair(tile, item));
			if (limit > 0 && result.size() >= size_t(limit)) {
				exceeded = true;
			}
		}
	}

	std::vector<std::pair<Tile*, Item*>> result;

private:
	uint16_t itemid;
	uint16_t itemidEnd;
	int32_t limit;
	bool exceeded;
};

class ReplaceItemsDialog : public wxDialog {
public:
	ReplaceItemsDialog(wxWindow* parent, bool selectionOnly);
	~ReplaceItemsDialog();

	void OnListSelected(wxCommandEvent& event);
	void OnReplaceItemClicked(wxMouseEvent& event);
	void OnWithItemClicked(wxMouseEvent& event);
	void OnSecondaryItemClicked(wxMouseEvent& event);
	void OnAddButtonClicked(wxCommandEvent& event);
	void OnEditButtonClicked(wxCommandEvent& event);
	void OnDuplicateButtonClicked(wxCommandEvent& event);
	void OnRemoveButtonClicked(wxCommandEvent& event);
	void OnImportButtonClicked(wxCommandEvent& event);
	void OnExecuteButtonClicked(wxCommandEvent& event);
	void OnCancelButtonClicked(wxCommandEvent& event);

private:
	void UpdateWidgets();
	bool GetReplaceItemRange(uint16_t& startId, uint16_t& endId);

	ReplaceItemsListBox* list;
	ReplaceItemsButton* replace_button;
	ReplaceItemsButton* with_button;
	ReplaceItemsButton* secondary_button;
	wxGauge* progress;
	wxStaticBitmap* arrow_bitmap;
	wxCheckBox* secondary_above_checkbox;
	wxButton* add_button;
	wxButton* edit_button;
	wxButton* duplicate_button;
	wxButton* remove_button;
	wxButton* import_button;
	wxButton* execute_button;
	wxButton* close_button;
	bool selectionOnly;
	uint16_t replaceIdStart;
	uint16_t replaceIdEnd;
	bool isRangeSelection;
};

#endif
