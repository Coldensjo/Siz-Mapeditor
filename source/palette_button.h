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

#ifndef RME_PALETTE_BUTTON_H
#define RME_PALETTE_BUTTON_H

#include "main.h"

class PaletteButton : public wxButton {
public:
	PaletteButton(wxWindow* parent, wxWindowID id, const wxString& label, 
		const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, 
		long style = 0, const wxValidator& validator = wxDefaultValidator, 
		const wxString& name = wxButtonNameStr);

protected:
	void OnPaint(wxPaintEvent& event);
	void OnMouseEnter(wxMouseEvent& event);
	void OnMouseLeave(wxMouseEvent& event);
	void OnMouseDown(wxMouseEvent& event);
	void OnMouseUp(wxMouseEvent& event);
	wxString GetLabelWithoutAmpersand() const;
	int GetUnderlineIndex() const;

	bool isPressed;
	bool isHovered;

	DECLARE_EVENT_TABLE()
};

#endif

