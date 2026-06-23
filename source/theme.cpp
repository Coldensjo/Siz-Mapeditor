//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "theme.h"

#include <wx/settings.h>

namespace {

ThemePalette DarkPalette() {
	return {
		wxColour(30, 30, 30),
		wxColour(37, 37, 38),
		wxColour(45, 45, 48),
		wxColour(63, 63, 70),
		wxColour(220, 220, 220),
		wxColour(160, 160, 165),
		wxColour(9, 71, 113),
		wxColour(62, 62, 66),
		wxColour(120, 120, 125),
	};
}

}

ThemeManager& ThemeManager::Get() {
	static ThemeManager manager;
	return manager;
}

ThemeMode ThemeManager::NormalizeMode(int mode) {
	switch (mode) {
		case static_cast<int>(ThemeMode::Light):
			return ThemeMode::Light;
		case static_cast<int>(ThemeMode::System):
			return ThemeMode::System;
		case static_cast<int>(ThemeMode::Dark):
		default:
			return ThemeMode::Dark;
	}
}

ThemePalette ThemeManager::PaletteFor(ThemeMode mode) {
	if (mode == ThemeMode::System) {
		mode = wxSystemSettings::GetAppearance().AreAppsDark() ? ThemeMode::Dark : ThemeMode::Light;
	}

	if (mode != ThemeMode::Light) {
		return DarkPalette();
	}

	return {
		wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW),
		wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE),
		wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE),
		wxSystemSettings::GetColour(wxSYS_COLOUR_3DSHADOW),
		wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT),
		wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT),
		wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT),
		wxSystemSettings::GetColour(wxSYS_COLOUR_3DLIGHT),
		wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT),
	};
}

ThemeMode ThemeManager::GetMode() const {
	return mode;
}

const ThemePalette& ThemeManager::GetPalette() const {
	return palette;
}

bool ThemeManager::Apply(ThemeMode newMode, wxWindow* root) {
	(void)root;
	mode = NormalizeMode(static_cast<int>(newMode));
	palette = PaletteFor(mode);
	return true;
}

ThemeManager::ThemeManager() : mode(ThemeMode::Dark), palette(PaletteFor(ThemeMode::Dark)) {
}
