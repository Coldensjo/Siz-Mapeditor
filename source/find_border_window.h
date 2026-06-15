//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_FIND_BORDER_WINDOW_H_
#define RME_FIND_BORDER_WINDOW_H_

#include <wx/dialog.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/vlbox.h>

class BorderDialogListBox : public wxVListBox {
public:
	BorderDialogListBox(wxWindow* parent, wxWindowID id);

	void SetBorderIds(const std::vector<uint32_t>& ids);
	uint32_t GetSelectedBorderId() const;
	void SelectBorderId(uint32_t borderId);

protected:
	void OnDrawItem(wxDC& dc, const wxRect& rect, size_t n) const override;
	wxCoord OnMeasureItem(size_t n) const override;

private:
	std::vector<uint32_t> border_ids;
};

class FindBorderDialog : public wxDialog {
public:
	FindBorderDialog(wxWindow* parent, uint32_t initialBorderId = 0);
	~FindBorderDialog();

	uint32_t getResultId() const {
		return result_id;
	}

private:
	void RefreshList();
	void UpdatePreview();
	void SelectBorderId(uint32_t borderId);

	void OnFilterChanged(wxCommandEvent& event);
	void OnFilterSpinChanged(wxSpinEvent& event);
	void OnFilterTimer(wxTimerEvent& event);
	void OnListSelected(wxCommandEvent& event);
	void OnListDoubleClick(wxCommandEvent& event);
	void OnPreviewPaint(wxPaintEvent& event);
	void OnClickOK(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

	BorderDialogListBox* border_list;
	wxSpinCtrl* filter_id_spin;
	wxTextCtrl* filter_text;
	wxPanel* preview_panel;
	wxStaticText* preview_label;
	wxButton* ok_button;
	wxTimer filter_timer;
	uint32_t result_id;
	uint32_t preview_border_id;

	DECLARE_EVENT_TABLE()
};

#endif
