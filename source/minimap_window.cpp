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

#include "graphics.h"
#include "editor.h"
#include "map.h"

#include "gui.h"
#include "live_socket.h"
#include "map_display.h"
#include "minimap_window.h"
#include <algorithm>
#include <wx/rawbmp.h>

namespace {

void DrawLiveUserMarkers(wxDC& dc, Editor& editor, int start_x, int start_y, int floor, int window_width, int window_height) {
	if (!editor.IsLive()) {
		return;
	}

	LiveSocket& live = editor.GetLive();
	const std::vector<LiveCursor> cursorList = live.getCursorList();
	if (cursorList.empty()) {
		return;
	}

	const std::vector<LiveParticipant>& participants = live.getParticipantList();
	const uint32_t ownClientId = live.getOwnClientId();

	wxFont label_font(*wxSWISS_FONT);
	label_font.SetPointSize(8);
	label_font.SetWeight(wxFONTWEIGHT_BOLD);
	dc.SetFont(label_font);

	for (const LiveCursor& cursor : cursorList) {
		if (cursor.id == ownClientId) {
			continue;
		}
		if (!isLiveMarkerVisibleOnFloor(floor, cursor.pos.z)) {
			continue;
		}

		const int screen_x = cursor.pos.x - start_x;
		const int screen_y = cursor.pos.y - start_y;
		if (screen_x < 0 || screen_y < 0 || screen_x >= window_width || screen_y >= window_height) {
			continue;
		}

		wxColor color = cursor.color;
		wxString user_name;
		for (const LiveParticipant& participant : participants) {
			if (participant.id == cursor.id) {
				color = participant.color;
				user_name = participant.name;
				break;
			}
		}

		constexpr int marker_radius = 4;

		// Dark halo + white ring + filled marker for contrast on any terrain.
		dc.SetPen(wxPen(*wxBLACK, 2));
		dc.SetBrush(*wxTRANSPARENT_BRUSH);
		dc.DrawCircle(screen_x, screen_y, marker_radius + 1);

		dc.SetPen(wxPen(*wxWHITE, 2));
		dc.DrawCircle(screen_x, screen_y, marker_radius);

		dc.SetPen(*wxTRANSPARENT_PEN);
		dc.SetBrush(wxBrush(color));
		dc.DrawCircle(screen_x, screen_y, marker_radius - 1);

		if (user_name.empty()) {
			continue;
		}

		const wxSize text_size = dc.GetTextExtent(user_name);
		const int pad_x = 3;
		const int pad_y = 1;

		int label_x = screen_x + marker_radius + 3;
		int label_y = screen_y - text_size.y / 2;

		if (label_x + text_size.x + pad_x * 2 > window_width) {
			label_x = screen_x - marker_radius - 3 - text_size.x - pad_x * 2;
		}
		if (label_y < 0) {
			label_y = screen_y + marker_radius + 2;
		}
		if (label_y + text_size.y + pad_y * 2 > window_height) {
			label_y = screen_y - marker_radius - text_size.y - 2;
		}

		label_x = std::max(0, label_x);
		label_y = std::max(0, label_y);

		const wxRect label_bg(label_x, label_y, text_size.x + pad_x * 2, text_size.y + pad_y * 2);
		dc.SetPen(wxPen(color, 1));
		dc.SetBrush(wxBrush(wxColor(16, 16, 16)));
		dc.DrawRectangle(label_bg);

		dc.SetTextForeground(*wxWHITE);
		dc.DrawText(user_name, label_x + pad_x, label_y + pad_y);
	}
}

} // namespace

BEGIN_EVENT_TABLE(MinimapWindow, wxPanel)
EVT_LEFT_DOWN(MinimapWindow::OnMouseClick)
EVT_SIZE(MinimapWindow::OnSize)
EVT_PAINT(MinimapWindow::OnPaint)
EVT_ERASE_BACKGROUND(MinimapWindow::OnEraseBackground)
EVT_CLOSE(MinimapWindow::OnClose)
EVT_TIMER(wxID_ANY, MinimapWindow::OnDelayedUpdate)
EVT_KEY_DOWN(MinimapWindow::OnKey)
END_EVENT_TABLE()

MinimapWindow::MinimapWindow(wxWindow* parent) :
    wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(205, 130)),
    update_timer(this),
    last_start_x(0),
    last_start_y(0),
    cached_start_x(-1),
    cached_start_y(-1),
    cached_floor(-1),
    cached_width(0),
    cached_height(0),
    cached_show_all_floors(false),
    bitmap_dirty(true),
    last_paint_time(std::chrono::steady_clock::now() - std::chrono::milliseconds(80)) {
}

MinimapWindow::~MinimapWindow() {
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	MarkDirty();
	Refresh();
	event.Skip();
}

void MinimapWindow::OnClose(wxCloseEvent&) {
	g_gui.DestroyMinimap();
}

void MinimapWindow::DelayedUpdate() {
    // We only update the window after actions have taken place so we can coalesce
    // expensive redraws, but keep the latency low when the user is navigating.
    bitmap_dirty = true;

    constexpr int kDefaultFastIntervalMs = 40;
    constexpr int kMinTimerIntervalMs = 16;

    const int configured_delay = g_settings.getInteger(Config::MINIMAP_UPDATE_DELAY);
    int fast_interval = kDefaultFastIntervalMs;
    if (configured_delay > 0) {
        fast_interval = std::min(fast_interval, configured_delay);
    }
    fast_interval = std::max(fast_interval, kMinTimerIntervalMs);

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_paint_time).count();

    if (elapsed >= fast_interval) {
        if (update_timer.IsRunning()) {
            update_timer.Stop();
        }
        Refresh();
        return;
    }

    int wait_for = static_cast<int>(fast_interval - elapsed);
    if (configured_delay > 0) {
        wait_for = std::min(wait_for, configured_delay);
    }
    wait_for = std::max(wait_for, kMinTimerIntervalMs);
    update_timer.Start(wait_for, true);
}

void MinimapWindow::MarkDirty() {
	bitmap_dirty = true;
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event) {
	Refresh();
}

void MinimapWindow::OnPaint(wxPaintEvent& event) {
	wxBufferedPaintDC pdc(this);

	pdc.SetBackground(*wxBLACK_BRUSH);
	pdc.Clear();

	if (!g_gui.IsEditorOpen()) {
		return;
	}
	Editor& editor = *g_gui.GetCurrentEditor();

	const wxSize client_size = GetClientSize();
	const int window_width = client_size.GetWidth();
	const int window_height = client_size.GetHeight();
	if (window_width <= 0 || window_height <= 0) {
		return;
	}
	// printf("W:%d\tH:%d\n", window_width, window_height);
	int center_x, center_y;

	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	canvas->GetScreenCenter(&center_x, &center_y);

	int start_x, start_y;
	int end_x, end_y;
	start_x = center_x - window_width / 2;
	start_y = center_y - window_height / 2;

	end_x = center_x + window_width / 2;
	end_y = center_y + window_height / 2;

	if (start_x < 0) {
		start_x = 0;
		end_x = window_width;
	} else if (end_x > editor.map.getWidth()) {
		start_x = editor.map.getWidth() - window_width;
		end_x = editor.map.getWidth();
	}
	if (start_y < 0) {
		start_y = 0;
		end_y = window_height;
	} else if (end_y > editor.map.getHeight()) {
		start_y = editor.map.getHeight() - window_height;
		end_y = editor.map.getHeight();
	}

	start_x = max(start_x, 0);
	start_y = max(start_y, 0);
	end_x = min(end_x, editor.map.getWidth());
	end_y = min(end_y, editor.map.getHeight());

	last_start_x = start_x;
	last_start_y = start_y;

	int floor = g_gui.GetCurrentFloor();
	bool show_all_floors = g_settings.getBoolean(Config::SHOW_ALL_FLOORS);

	// Floor range to composite when show_all_floors is on (mirrors map_drawer SetupVars logic).
	// Iterating from start_floor down to end_floor; lower z (physically higher) overwrites, so
	// elevated structures (bridges, upper floors) take visual priority over ground tiles — exactly
	// matching the main renderer where lower-z layers are drawn last (on top).
	int floor_start, floor_end;
	if (show_all_floors && floor <= GROUND_LAYER) {
		floor_start = GROUND_LAYER;
		floor_end   = 0;
	} else {
		floor_start = floor;
		floor_end   = floor;
	}

	if (g_gui.IsRenderingEnabled()) {
		bool needs_rebuild = bitmap_dirty || !minimap_bitmap.IsOk()
			|| cached_floor != floor
			|| cached_show_all_floors != show_all_floors
			|| cached_start_x != start_x || cached_start_y != start_y
			|| cached_width != window_width || cached_height != window_height;

		if (needs_rebuild) {
			if (!minimap_bitmap.IsOk() || minimap_bitmap.GetWidth() != window_width || minimap_bitmap.GetHeight() != window_height) {
				minimap_bitmap = wxBitmap(window_width, window_height, 32);
			}

			wxAlphaPixelData pixel_data(minimap_bitmap);
			if (pixel_data) {
				wxAlphaPixelData::Iterator pixel_it(pixel_data);
				for (int window_y = 0; window_y < window_height; ++window_y) {
					wxAlphaPixelData::Iterator row_start = pixel_it;
					int map_y = start_y + window_y;
					for (int window_x = 0; window_x < window_width; ++window_x, ++pixel_it) {
						int map_x = start_x + window_x;
						uint8_t color_index = 0;
						if (map_x >= 0 && map_y >= 0 && map_x < editor.map.getWidth() && map_y < editor.map.getHeight()) {
							for (int z = floor_start; z >= floor_end; --z) {
								if (Tile* tile = editor.map.getTile(map_x, map_y, z)) {
									uint8_t c = tile->getMiniMapColor();
									if (c != 0) {
										color_index = c;
									}
								}
							}
						}

						const RGBQuad& color = minimap_color[color_index];
						pixel_it.Red() = color.red;
						pixel_it.Green() = color.green;
						pixel_it.Blue() = color.blue;
						pixel_it.Alpha() = 0xFF;
					}
					pixel_it = row_start;
					pixel_it.OffsetY(pixel_data, 1);
				}
				cached_start_x = start_x;
				cached_start_y = start_y;
				cached_floor = floor;
				cached_show_all_floors = show_all_floors;
				cached_width = window_width;
				cached_height = window_height;
				bitmap_dirty = false;
			} else {
				bitmap_dirty = true;
			}
		}

		if (minimap_bitmap.IsOk()) {
			pdc.DrawBitmap(minimap_bitmap, 0, 0, false);
		}

		DrawLiveUserMarkers(pdc, editor, start_x, start_y, floor, window_width, window_height);

		if (g_settings.getInteger(Config::MINIMAP_VIEW_BOX)) {
			pdc.SetPen(*wxWHITE_PEN);
			// Draw the rectangle on the minimap

			// Some view info
			int screensize_x, screensize_y;
			int view_scroll_x, view_scroll_y;

			canvas->GetViewBox(&view_scroll_x, &view_scroll_y, &screensize_x, &screensize_y);

			// bounds of the view
			int view_start_x, view_start_y;
			int view_end_x, view_end_y;

			int tile_size = int(TileSize / canvas->GetZoom()); // after zoom

			int floor_offset = (floor > GROUND_LAYER ? 0 : (GROUND_LAYER - floor));

			view_start_x = view_scroll_x / TileSize + floor_offset;
			view_start_y = view_scroll_y / TileSize + floor_offset;

			view_end_x = view_start_x + screensize_x / tile_size + 1;
			view_end_y = view_start_y + screensize_y / tile_size + 1;

			for (int x = view_start_x; x <= view_end_x; ++x) {
				pdc.DrawPoint(x - start_x, view_start_y - start_y);
				pdc.DrawPoint(x - start_x, view_end_y - start_y);
			}
			for (int y = view_start_y; y < view_end_y; ++y) {
				pdc.DrawPoint(view_start_x - start_x, y - start_y);
				pdc.DrawPoint(view_end_x - start_x, y - start_y);
			}
		}
	}

	last_paint_time = std::chrono::steady_clock::now();
}

void MinimapWindow::OnMouseClick(wxMouseEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}
	int new_map_x = last_start_x + event.GetX();
	int new_map_y = last_start_y + event.GetY();
	g_gui.SetScreenCenterPosition(Position(new_map_x, new_map_y, g_gui.GetCurrentFloor()));
	Refresh();
	g_gui.RefreshView();
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}
