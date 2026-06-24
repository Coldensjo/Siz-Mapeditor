//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#include "main.h"

#include "theme.h"

#include "map_window.h"

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

wxApp::Appearance AppearanceFor(ThemeMode mode) {
	switch (mode) {
		case ThemeMode::Light:
			return wxApp::Appearance::Light;
		case ThemeMode::System:
			return wxApp::Appearance::System;
		case ThemeMode::Dark:
		default:
			return wxApp::Appearance::Dark;
	}
}

void ApplyPaletteToWindow(wxWindow* window, const ThemePalette& palette, bool isRoot) {
	if (dynamic_cast<MapWindow*>(window)) {
		return;
	}

	window->SetForegroundColour(palette.text);
	window->SetBackgroundColour(isRoot ? palette.window : palette.surface);

	for (wxWindow* child : window->GetChildren()) {
		ApplyPaletteToWindow(child, palette, false);
	}
}

void ApplyPaletteToAui(wxAuiManager* manager, const ThemePalette& palette) {
	wxAuiDockArt* art = manager->GetArtProvider();
	if (!art) {
		return;
	}

	art->SetColor(wxAUI_DOCKART_BACKGROUND_COLOUR, palette.surface);
	art->SetColor(wxAUI_DOCKART_SASH_COLOUR, palette.control);
	art->SetColor(wxAUI_DOCKART_BORDER_COLOUR, palette.border);
	art->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR, palette.surface);
	art->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR, palette.control);
	art->SetColor(wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR, palette.mutedText);
	art->SetColor(wxAUI_DOCKART_ACTIVE_CAPTION_COLOUR, palette.selection);
	art->SetColor(wxAUI_DOCKART_ACTIVE_CAPTION_GRADIENT_COLOUR, palette.hover);
	art->SetColor(wxAUI_DOCKART_ACTIVE_CAPTION_TEXT_COLOUR, palette.text);
	art->SetColor(wxAUI_DOCKART_GRIPPER_COLOUR, palette.mutedText);
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

ThemeMode ThemeManager::ModeFromChoice(int choice) {
	return NormalizeMode(choice);
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
	const ThemeMode requestedMode = NormalizeMode(static_cast<int>(newMode));
	if (wxTheApp && wxTheApp->SetAppearance(AppearanceFor(requestedMode)) == wxApp::AppearanceResult::Failure) {
		return false;
	}

	const ThemePalette requestedPalette = PaletteFor(requestedMode);
	mode = requestedMode;
	palette = requestedPalette;

	if (!root) {
		return true;
	}

	ApplyPaletteToWindow(root, palette, true);
	if (wxAuiManager* manager = wxAuiManager::GetManager(root)) {
		ApplyPaletteToAui(manager, palette);
		manager->Update();
	}

	root->Layout();
	root->Refresh();
	return true;
}

ThemeManager::ThemeManager() : mode(ThemeMode::Dark), palette(PaletteFor(ThemeMode::Dark)) {
}
