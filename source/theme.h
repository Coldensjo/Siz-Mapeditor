//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////

#ifndef RME_THEME_H_
#define RME_THEME_H_

#include <wx/colour.h>

class wxWindow;

enum class ThemeMode {
	Dark = 0,
	Light = 1,
	System = 2,
};

struct ThemePalette {
	wxColour window;
	wxColour surface;
	wxColour control;
	wxColour border;
	wxColour text;
	wxColour mutedText;
	wxColour selection;
	wxColour hover;
	wxColour disabledText;
};

class ThemeManager {
public:
	static ThemeManager& Get();

	static ThemeMode NormalizeMode(int mode);
	static ThemeMode ModeFromChoice(int choice);
	static ThemePalette PaletteFor(ThemeMode mode);

	ThemeMode GetMode() const;
	const ThemePalette& GetPalette() const;
	bool Apply(ThemeMode mode, wxWindow* root);

private:
	ThemeManager();

	ThemeMode mode;
	ThemePalette palette;
};

#endif
