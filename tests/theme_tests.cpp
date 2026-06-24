#include "../source/theme.h"

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>

#include <wx/app.h>
#include <wx/aui/aui.h>

class ThemeTestApp : public wxApp {
public:
	bool OnInit() override {
		return true;
	}
};

wxIMPLEMENT_APP_NO_MAIN(ThemeTestApp);

class TrackingToolBarArt : public wxAuiGenericToolBarArt {
public:
	void UpdateColoursFromSystem() override {
		coloursUpdated = true;
		wxAuiGenericToolBarArt::UpdateColoursFromSystem();
	}

	bool coloursUpdated = false;
};

class TrackingTabArt : public wxAuiGenericTabArt {
public:
	void SetColour(const wxColour& colour) override {
		baseColour = colour;
		wxAuiGenericTabArt::SetColour(colour);
	}

	void SetActiveColour(const wxColour& colour) override {
		activeColour = colour;
		wxAuiGenericTabArt::SetActiveColour(colour);
	}

	wxColour baseColour;
	wxColour activeColour;
};

int main(int argc, char** argv) {
	const auto dark = ThemeManager::PaletteFor(ThemeMode::Dark);
	assert(dark.window == wxColour(30, 30, 30));
	assert(dark.text != dark.window);
	assert(dark.text != dark.mutedText);
	assert(dark.border != dark.surface);
	assert(dark.hover != dark.selection);
	assert(ThemeManager::NormalizeMode(-1) == ThemeMode::Dark);
	assert(ThemeManager::NormalizeMode(0) == ThemeMode::Dark);
	assert(ThemeManager::NormalizeMode(1) == ThemeMode::Light);
	assert(ThemeManager::NormalizeMode(2) == ThemeMode::System);
	assert(ThemeManager::NormalizeMode(3) == ThemeMode::Dark);
	assert(ThemeManager::ModeFromChoice(0) == ThemeMode::Dark);
	assert(ThemeManager::ModeFromChoice(1) == ThemeMode::Light);
	assert(ThemeManager::ModeFromChoice(2) == ThemeMode::System);
	assert(ThemeManager::ModeFromChoice(-1) == ThemeMode::Dark);
	assert(ThemeManager::ModeFromChoice(8) == ThemeMode::Dark);

	wxEntryStart(argc, argv);
	assert(wxTheApp->CallOnInit());
	{
		wxFrame root(nullptr, wxID_ANY, "Theme test");
		wxAuiToolBar toolbar(&root, wxID_ANY);
		auto* toolbarArt = new TrackingToolBarArt();
		toolbar.SetArtProvider(toolbarArt);
		wxAuiNotebook notebook(&root, wxID_ANY);
		auto* tabArt = new TrackingTabArt();
		notebook.SetArtProvider(tabArt);

		assert(ThemeManager::Get().Apply(ThemeMode::Dark, &root));
		assert(toolbarArt->coloursUpdated);
		assert(tabArt->baseColour == ThemeManager::Get().GetPalette().surface);
		assert(tabArt->activeColour == ThemeManager::Get().GetPalette().control);
	}
	wxTheApp->OnExit();
	wxEntryCleanup();
}
