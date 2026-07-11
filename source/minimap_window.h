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

#ifndef RME_MINIMAP_WINDOW_H_
#define RME_MINIMAP_WINDOW_H_

#include <chrono>

class MinimapCanvas;

class MinimapCanvas : public wxPanel {
public:
	MinimapCanvas(wxWindow* parent);
	virtual ~MinimapCanvas();

	void OnPaint(wxPaintEvent&);
	void OnEraseBackground(wxEraseEvent&) { }
	void OnMouseClick(wxMouseEvent&);
	void OnMouseDoubleClick(wxMouseEvent&);
	void OnMouseWheel(wxMouseEvent&);
	void OnMouseMiddleDown(wxMouseEvent&);
	void OnMouseMiddleUp(wxMouseEvent&);
	void OnMouseMove(wxMouseEvent&);
	void OnMouseCaptureLost(wxMouseCaptureLostEvent&);
	void OnSize(wxSizeEvent&);

	void DelayedUpdate();
	void MarkDirty();
	void OnDelayedUpdate(wxTimerEvent& event);
	void OnKey(wxKeyEvent& event);

	void ApplyZoomDelta(double diff, int anchor_x, int anchor_y);
	void ResetView();

	void ComputeViewBounds(Editor& editor, MapCanvas* canvas, int window_width, int window_height, int& start_x, int& start_y, int& end_x, int& end_y);

protected:
	wxBitmap minimap_bitmap;
	wxTimer update_timer;
	int last_start_x;
	int last_start_y;
	int cached_start_x;
	int cached_start_y;
	int cached_floor;
	int cached_width;
	int cached_height;
	double cached_zoom;
	bool cached_show_all_floors;
	bool bitmap_dirty;
	bool panning;
	int pan_offset_x;
	int pan_offset_y;
	int pan_drag_last_x;
	int pan_drag_last_y;
	int last_canvas_center_x;
	int last_canvas_center_y;
	double minimap_zoom;
	std::chrono::steady_clock::time_point last_paint_time;

	static constexpr double MINIMAP_ZOOM_MIN = 0.25;
	static constexpr double MINIMAP_ZOOM_MAX = 16.0;
	static constexpr double MINIMAP_ZOOM_DEFAULT = 1.0;

	DECLARE_EVENT_TABLE()
};

class MinimapCaptionBar : public wxPanel {
public:
	MinimapCaptionBar(wxWindow* parent);
	virtual ~MinimapCaptionBar();

	void UpdateChrome(bool pane_floating, bool pane_active);
	int GetCaptionHeight() const;

	void OnPaint(wxPaintEvent&);
	void OnSize(wxSizeEvent&);
	void OnLeftDown(wxMouseEvent&);

protected:
	bool pane_active;
	bool pane_floating;

	DECLARE_EVENT_TABLE()
};

class MinimapWindow : public wxPanel {
public:
	MinimapWindow(wxWindow* parent);
	virtual ~MinimapWindow();

	void DelayedUpdate();
	void MarkDirty();
	void RefreshCanvas();
	void UpdateCaptionChrome();
	void OnClose(wxCloseEvent&);
	void OnKey(wxKeyEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnAuiPaneEvent(wxAuiManagerEvent& event);
	void OnAuiPaneClose(wxAuiManagerEvent& event);

	void OnZoomIn(wxCommandEvent& event);
	void OnZoomOut(wxCommandEvent& event);
	void OnScrollZoomToggle(wxCommandEvent& event);
	void OnCaptionClose(wxCommandEvent& event);
	void StartCaptionDrag(wxMouseEvent& event);
	void OnCaptionDragMotion(wxMouseEvent& event);
	void OnCaptionDragEnd(wxMouseEvent& event);
	void OnCaptionCaptureLost(wxMouseCaptureLostEvent& event);

	bool IsPaneFloating() const;

protected:
	wxWindow* aui_event_source;
	MinimapCaptionBar* caption_bar;
	MinimapCanvas* canvas;
	wxStaticText* title_label;
	wxCheckBox* scroll_zoom_checkbox;
	wxButton* zoom_out_button;
	wxButton* zoom_in_button;
	wxButton* close_button;
	bool last_floating_state;
	bool last_active_state;
	bool caption_drag_pending;
	wxPoint caption_drag_start_screen;
	wxPoint caption_drag_offset;

	DECLARE_EVENT_TABLE()
};

#endif
