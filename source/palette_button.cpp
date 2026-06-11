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
#include "palette_button.h"
#include <wx/renderer.h>

BEGIN_EVENT_TABLE(PaletteButton, wxButton)
	EVT_PAINT(PaletteButton::OnPaint)
	EVT_ENTER_WINDOW(PaletteButton::OnMouseEnter)
	EVT_LEAVE_WINDOW(PaletteButton::OnMouseLeave)
	EVT_LEFT_DOWN(PaletteButton::OnMouseDown)
	EVT_LEFT_UP(PaletteButton::OnMouseUp)
END_EVENT_TABLE()

PaletteButton::PaletteButton(wxWindow* parent, wxWindowID id, const wxString& label, 
	const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, 
	const wxString& name) : wxButton(parent, id, label, pos, size, style, validator, name),
	isPressed(false), isHovered(false) {
	// Ensure button has a solid background color
	SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}

wxString PaletteButton::GetLabelWithoutAmpersand() const {
	wxString label = wxButton::GetLabel();
	wxString result;
	for (size_t i = 0; i < label.length(); ++i) {
		if (label[i] == '&' && i + 1 < label.length() && label[i + 1] != '&') {
			// Skip the ampersand, but keep track that next char should be underlined
			continue;
		}
		if (i > 0 && label[i - 1] == '&' && label[i] != '&') {
			// This character should be underlined, but we'll handle that in drawing
			result += label[i];
		} else if (label[i] != '&') {
			result += label[i];
		} else if (label[i] == '&' && i + 1 < label.length() && label[i + 1] == '&') {
			// Double ampersand means literal ampersand
			result += '&';
			++i; // Skip the second ampersand
		}
	}
	return result;
}

int PaletteButton::GetUnderlineIndex() const {
	wxString label = wxButton::GetLabel();
	int resultIndex = 0;
	for (size_t i = 0; i < label.length(); ++i) {
		if (label[i] == '&' && i + 1 < label.length() && label[i + 1] != '&') {
			// Return the index in the label without ampersand (which is resultIndex)
			return resultIndex;
		} else if (label[i] != '&' || (i + 1 < label.length() && label[i + 1] == '&')) {
			// Count this character (skip single & but count double &&)
			resultIndex++;
			if (label[i] == '&' && i + 1 < label.length() && label[i + 1] == '&') {
				++i; // Skip the second &
			}
		}
	}
	return -1;
}

void PaletteButton::OnPaint(wxPaintEvent& event) {
	// Don't call event.Skip() - we'll render everything ourselves
	wxBufferedPaintDC dc(this);
	
	wxSize size = GetClientSize();
	wxRect rect(0, 0, size.GetWidth(), size.GetHeight());
	
	// First, fill the entire button area with the background color to prevent transparency
	wxColour bgColour = GetBackgroundColour();
	if (!bgColour.IsOk()) {
		bgColour = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
	}
	dc.SetBrush(wxBrush(bgColour));
	dc.SetPen(wxPen(bgColour));
	dc.DrawRectangle(rect);
	
	// Render button using native renderer
	int flags = 0;
	if (!IsEnabled()) {
		flags |= wxCONTROL_DISABLED;
	}
	if (HasFocus()) {
		flags |= wxCONTROL_FOCUSED;
	}
	if (isPressed) {
		flags |= wxCONTROL_PRESSED;
	}
	if (isHovered && IsEnabled()) {
		flags |= wxCONTROL_CURRENT;
	}
	
	wxRendererNative::Get().DrawPushButton(this, dc, rect, flags);
	
	// Draw text with underline
	wxString label = GetLabelWithoutAmpersand();
	int underlineIndex = GetUnderlineIndex();
	
	if (!label.IsEmpty()) {
		// Get text metrics
		wxSize textSize = dc.GetTextExtent(label);
		
		// Calculate text position (centered)
		// Adjust for pressed state (button appears pushed down)
		int textX = (size.GetWidth() - textSize.GetWidth()) / 2;
		int textY = (size.GetHeight() - textSize.GetHeight()) / 2;
		if (isPressed) {
			textX += 1;
			textY += 1;
		}
		
		// Set text color based on enabled state
		if (IsEnabled()) {
			dc.SetTextForeground(GetForegroundColour());
		} else {
			dc.SetTextForeground(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
		}
		
		// Draw the text
		dc.DrawText(label, textX, textY);
		
		// Draw underline if needed
		if (underlineIndex >= 0 && underlineIndex < static_cast<int>(label.length())) {
			// Calculate the position of the character to underline
			wxString beforeChar = label.SubString(0, underlineIndex - 1);
			wxString charToUnderline = label.SubString(underlineIndex, underlineIndex);
			wxSize beforeSize = dc.GetTextExtent(beforeChar);
			wxSize charSize = dc.GetTextExtent(charToUnderline);
			
			// Draw underline
			int underlineX = textX + beforeSize.GetWidth();
			int underlineY = textY + textSize.GetHeight() - 1;
			dc.SetPen(wxPen(GetForegroundColour(), 1));
			dc.DrawLine(underlineX, underlineY, underlineX + charSize.GetWidth(), underlineY);
		}
	}
}

void PaletteButton::OnMouseEnter(wxMouseEvent& event) {
	isHovered = true;
	Refresh();
	event.Skip();
}

void PaletteButton::OnMouseLeave(wxMouseEvent& event) {
	isHovered = false;
	isPressed = false;
	Refresh();
	event.Skip();
}

void PaletteButton::OnMouseDown(wxMouseEvent& event) {
	isPressed = true;
	Refresh();
	event.Skip();
}

void PaletteButton::OnMouseUp(wxMouseEvent& event) {
	isPressed = false;
	Refresh();
	event.Skip();
}

