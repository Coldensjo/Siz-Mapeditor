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

#include "settings.h"
#include "client_version.h"
#include "editor.h"
#include "theme.h"
#include "gui.h"
#include "preferences.h"

namespace {

wxTextCtrl* AddFileRow(wxWindow* parent, wxSizer* sizer, const wxString& initialPath, const wxString& wildcard, const wxString& tooltip) {
	auto* rowSizer = newd wxBoxSizer(wxHORIZONTAL);
	auto* pathCtrl = newd wxTextCtrl(parent, wxID_ANY, initialPath);
	auto* browseBtn = newd wxButton(parent, wxID_ANY, "...", wxDefaultPosition, FROM_DIP(parent, wxSize(28, -1)));
	rowSizer->Add(pathCtrl, 1, wxEXPAND);
	rowSizer->Add(browseBtn, 0, wxLEFT, 5);
	sizer->Add(rowSizer, wxSizerFlags(0).Border(wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 25));

	pathCtrl->SetToolTip(tooltip);

	browseBtn->Bind(wxEVT_BUTTON, [parent, pathCtrl, wildcard](wxCommandEvent&) {
		wxFileDialog dlg(parent, "Select file", "", pathCtrl->GetValue(), wildcard, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (dlg.ShowModal() == wxID_OK) {
			pathCtrl->SetValue(dlg.GetPath());
		}
	});

	return pathCtrl;
}

wxString NormalizeDirectoryPath(wxString dir) {
	if (dir.Length() > 0 && dir.Last() != '/' && dir.Last() != '\\') {
		dir.Append(FileName::GetPathSeparator());
	}
	return dir;
}

wxBoxSizer* CreateBrowsePathRow(wxWindow* parent, wxTextCtrl** pathOut, const wxString& initialPath, const wxString& tooltip) {
	auto* rowSizer = newd wxBoxSizer(wxHORIZONTAL);
	*pathOut = newd wxTextCtrl(parent, wxID_ANY, initialPath);
	auto* browseBtn = newd wxButton(parent, wxID_ANY, "...", wxDefaultPosition, FROM_DIP(parent, wxSize(28, -1)));
	rowSizer->Add(*pathOut, 1, wxEXPAND);
	rowSizer->Add(browseBtn, 0, wxLEFT, 5);
	(*pathOut)->SetToolTip(tooltip);

	browseBtn->Bind(wxEVT_BUTTON, [parent, pathOut](wxCommandEvent&) {
		wxDirDialog dlg(parent, "Select directory", (*pathOut)->GetValue());
		if (dlg.ShowModal() == wxID_OK) {
			(*pathOut)->SetValue(dlg.GetPath());
		}
	});

	return rowSizer;
}

} // namespace

BEGIN_EVENT_TABLE(PreferencesWindow, wxDialog)
EVT_BUTTON(wxID_OK, PreferencesWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, PreferencesWindow::OnClickCancel)
EVT_BUTTON(wxID_APPLY, PreferencesWindow::OnClickApply)
EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, PreferencesWindow::OnNotebookPageChanged)
END_EVENT_TABLE()

PreferencesWindow::PreferencesWindow(wxWindow* parent, bool clientVersionSelected) :
	wxDialog(parent, wxID_ANY, "Preferences", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxCLOSE_BOX | wxRESIZE_BORDER) {
	Freeze();

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	book = newd wxNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBK_TOP);
	book->AddPage(CreateGeneralPage(), "General", true);
	book->AddPage(CreateEditorPage(), "Editor");
	book->AddPage(CreateGraphicsPage(), "Graphics");
	book->AddPage(CreateUIPage(), "Interface");
	book->AddPage(CreateClientPage(), "Client Version", clientVersionSelected);

	sizer->Add(book, 1, wxEXPAND | wxALL, 10);

	wxSizer* subsizer = newd wxBoxSizer(wxHORIZONTAL);
	subsizer->Add(newd wxButton(this, wxID_OK, "OK"), wxSizerFlags(1).Center());
	subsizer->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Border(wxALL, 5).Left().Center());
	subsizer->Add(newd wxButton(this, wxID_APPLY, "Apply"), wxSizerFlags(1).Center());
	sizer->Add(subsizer, 0, wxCENTER | wxLEFT | wxBOTTOM | wxRIGHT, 10);

	SetSizer(sizer);
	SetMinSize(FROM_DIP(this, wxSize(420, 480)));
	Layout();
	Centre(wxBOTH);

	if (clientVersionSelected) {
		EnsureClientVersionListBuilt();
	}

	Thaw();
}

void PreferencesWindow::OnNotebookPageChanged(wxBookCtrlEvent& event) {
	if (event.GetSelection() == CLIENT_PAGE_INDEX) {
		EnsureClientVersionListBuilt();
	}
	event.Skip();
}

wxNotebookPage* PreferencesWindow::CreateGeneralPage() {
	wxNotebookPage* general_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);
	wxStaticText* tmptext = nullptr;

	show_welcome_dialog_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Show welcome dialog on startup");
	show_welcome_dialog_chkbox->SetValue(g_settings.getInteger(Config::WELCOME_DIALOG) == 1);
	show_welcome_dialog_chkbox->SetToolTip("Show welcome dialog when starting the editor.");
	sizer->Add(show_welcome_dialog_chkbox, 0, wxLEFT | wxTOP, 5);

	open_map_on_startup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Always open this map on startup");
	const bool open_map_on_startup_enabled = g_settings.getInteger(Config::OPEN_MAP_ON_STARTUP) == 1;
	open_map_on_startup_chkbox->SetValue(open_map_on_startup_enabled);
	open_map_on_startup_chkbox->SetToolTip("Automatically open the selected map when starting the editor.");
	sizer->Add(open_map_on_startup_chkbox, 0, wxLEFT | wxTOP, 5);

	const wxString map_wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ?
		"(*.otbm;*.otgz)|*.otbm;*.otgz" :
		"(*.otbm)|*.otbm|Compressed OpenTibia Binary Map (*.otgz)|*.otgz";
	open_map_on_startup_path = AddFileRow(
		general_page,
		sizer,
		wxstr(g_settings.getString(Config::OPEN_MAP_ON_STARTUP_PATH)),
		map_wildcard,
		"Map file to open automatically when the editor starts."
	);
	open_map_on_startup_path->Enable(open_map_on_startup_enabled);

	if (open_map_on_startup_enabled) {
		show_welcome_dialog_chkbox->SetValue(false);
		show_welcome_dialog_chkbox->Enable(false);
	}

	open_map_on_startup_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
		const bool enabled = event.IsChecked();
		open_map_on_startup_path->Enable(enabled);
		if (enabled) {
			show_welcome_dialog_chkbox->SetValue(false);
			show_welcome_dialog_chkbox->Enable(false);
		} else {
			show_welcome_dialog_chkbox->Enable(true);
		}
	});

	always_make_backup_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Always make map backup");
	always_make_backup_chkbox->SetValue(g_settings.getInteger(Config::ALWAYS_MAKE_BACKUP) == 1);
	sizer->Add(always_make_backup_chkbox, 0, wxLEFT | wxTOP, 5);

	only_one_instance_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Open all maps in the same instance");
	only_one_instance_chkbox->SetValue(g_settings.getInteger(Config::ONLY_ONE_INSTANCE) == 1);
	only_one_instance_chkbox->SetToolTip("When checked, maps opened using the shell will all be opened in the same instance.");
	sizer->Add(only_one_instance_chkbox, 0, wxLEFT | wxTOP, 5);

	ignore_save_prompt_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Skip save confirmation when closing");
	ignore_save_prompt_chkbox->SetValue(g_settings.getBoolean(Config::IGNORE_SAVE_PROMPT));
	ignore_save_prompt_chkbox->SetToolTip("When checked, the editor will not prompt to save changes when closing maps.");
	sizer->Add(ignore_save_prompt_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_save_on_close_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Automatically save maps when closing");
	auto_save_on_close_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_SAVE_ON_CLOSE));
	auto_save_on_close_chkbox->SetToolTip("When checked, the editor will save changes automatically when closing maps.");
	sizer->Add(auto_save_on_close_chkbox, 0, wxLEFT | wxTOP, 5);

	enable_tileset_editing_chkbox = newd wxCheckBox(general_page, wxID_ANY, "Enable tileset editing");
	enable_tileset_editing_chkbox->SetValue(g_settings.getInteger(Config::SHOW_TILESET_EDITOR) == 1);
	enable_tileset_editing_chkbox->SetToolTip("Show tileset editing options.");
	sizer->Add(enable_tileset_editing_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	auto* grid_sizer = newd wxFlexGridSizer(2, 10, 10);
	grid_sizer->AddGrowableCol(1);

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo queue size: "), 0);
	undo_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 0x10000000);
	grid_sizer->Add(undo_size_spin, 0);
	SetWindowToolTip(tmptext, undo_size_spin, "How many action you can undo, be aware that a high value will increase memory usage.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Undo maximum memory size (MB): "), 0);
	undo_mem_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::UNDO_MEM_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 4096);
	grid_sizer->Add(undo_mem_size_spin, 0);
	SetWindowToolTip(tmptext, undo_mem_size_spin, "The approximite limit for the memory usage of the undo queue.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Worker Threads: "), 0);
	worker_threads_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::WORKER_THREADS)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 64);
	grid_sizer->Add(worker_threads_spin, 0);
	SetWindowToolTip(tmptext, worker_threads_spin, "How many threads the editor will use for intensive operations. This should be equivalent to the amount of logical processors in your system.");

	grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "Replace count: "), 0);
	replace_size_spin = newd wxSpinCtrl(general_page, wxID_ANY, i2ws(g_settings.getInteger(Config::REPLACE_SIZE)), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 100000);
	grid_sizer->Add(replace_size_spin, 0);
	SetWindowToolTip(tmptext, replace_size_spin, "How many items you can replace on the map using the Replace Item tool.");

	sizer->Add(grid_sizer, 0, wxALL, 5);
	sizer->AddSpacer(10);

	wxString position_choices[] = {
		"  {x = 0, y = 0, z = 0}",
		R"(  {"x":0,"y":0,"z":0})",
		"  x, y, z",
		"  (x, y, z)",
		"  Position(x, y, z)",
		"  x=\"x\" y=\"y\" z=\"z\"",
		"  centerx=\"x\" centery=\"y\" centerz=\"z\""
	};
	const int radio_choices = sizeof(position_choices) / sizeof(wxString);
	position_format = new wxRadioBox(general_page, wxID_ANY, "Copy Position Format", wxDefaultPosition, wxDefaultSize, radio_choices, position_choices, 1, wxRA_SPECIFY_COLS);
	position_format->SetSelection(g_settings.getInteger(Config::COPY_POSITION_FORMAT));
	sizer->Add(position_format, 0, wxALL | wxEXPAND, 5);
	SetWindowToolTip(position_format, "The position format when copying from the map.");

	general_page->SetSizer(sizer);
	return general_page;
}

wxNotebookPage* PreferencesWindow::CreateEditorPage() {
	wxNotebookPage* editor_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	group_actions_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Group same-type actions");
	group_actions_chkbox->SetValue(g_settings.getBoolean(Config::GROUP_ACTIONS));
	group_actions_chkbox->SetToolTip("This will group actions of the same type (drawing, selection..) when several take place in consecutive order.");
	sizer->Add(group_actions_chkbox, 0, wxLEFT | wxTOP, 5);

	duplicate_id_warn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Warn for duplicate IDs");
	duplicate_id_warn_chkbox->SetValue(g_settings.getBoolean(Config::WARN_FOR_DUPLICATE_ID));
	duplicate_id_warn_chkbox->SetToolTip("Warns for most kinds of duplicate IDs.");
	sizer->Add(duplicate_id_warn_chkbox, 0, wxLEFT | wxTOP, 5);

	house_remove_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "House brush removes items");
	house_remove_chkbox->SetValue(g_settings.getBoolean(Config::HOUSE_BRUSH_REMOVE_ITEMS));
	house_remove_chkbox->SetToolTip("When this option is checked, the house brush will automaticly remove items that will respawn every time the map is loaded.");
	sizer->Add(house_remove_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_assign_doors_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto-assign door ids");
	auto_assign_doors_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_ASSIGN_DOORID));
	auto_assign_doors_chkbox->SetToolTip("This will auto-assign unique door ids to all doors placed with the door brush (or doors painted over with the house brush).\nDoes NOT affect doors placed using the RAW palette.");
	sizer->Add(auto_assign_doors_chkbox, 0, wxLEFT | wxTOP, 5);

	doodad_erase_same_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Doodad brush only erases same");
	doodad_erase_same_chkbox->SetValue(g_settings.getBoolean(Config::DOODAD_BRUSH_ERASE_LIKE));
	doodad_erase_same_chkbox->SetToolTip("The doodad brush will only erase items that belongs to the current brush.");
	sizer->Add(doodad_erase_same_chkbox, 0, wxLEFT | wxTOP, 5);

	eraser_leave_unique_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Eraser leaves unique items");
	eraser_leave_unique_chkbox->SetValue(g_settings.getBoolean(Config::ERASER_LEAVE_UNIQUE));
	eraser_leave_unique_chkbox->SetToolTip("The eraser will leave containers with items in them, items with unique or action id and items.");
	sizer->Add(eraser_leave_unique_chkbox, 0, wxLEFT | wxTOP, 5);

	auto_create_spawn_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Auto create spawn when placing creature");
	auto_create_spawn_chkbox->SetValue(g_settings.getBoolean(Config::AUTO_CREATE_SPAWN));
	auto_create_spawn_chkbox->SetToolTip("When this option is checked, you can place creatures without placing a spawn manually, the spawn will be place automatically.");
	sizer->Add(auto_create_spawn_chkbox, 0, wxLEFT | wxTOP, 5);

	allow_multiple_orderitems_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Prevent toporder conflict");
	allow_multiple_orderitems_chkbox->SetValue(g_settings.getBoolean(Config::RAW_LIKE_SIMONE));
	allow_multiple_orderitems_chkbox->SetToolTip("When this option is checked, you can not place several items with the same toporder on one tile using a RAW Brush.");
	sizer->Add(allow_multiple_orderitems_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	merge_move_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge move");
	merge_move_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_MOVE));
	merge_move_chkbox->SetToolTip("Moved tiles won't replace already placed tiles.");
	sizer->Add(merge_move_chkbox, 0, wxLEFT | wxTOP, 5);

	merge_paste_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Use merge paste");
	merge_paste_chkbox->SetValue(g_settings.getBoolean(Config::MERGE_PASTE));
	merge_paste_chkbox->SetToolTip("Pasted tiles won't replace already placed tiles.");
	sizer->Add(merge_paste_chkbox, 0, wxLEFT | wxTOP, 5);

	live_allow_clipboard_chkbox = newd wxCheckBox(editor_page, wxID_ANY, "Allow copy and paste on live maps");
	live_allow_clipboard_chkbox->SetValue(g_settings.getBoolean(Config::LIVE_ALLOW_CLIPBOARD));
	live_allow_clipboard_chkbox->SetToolTip("When connected to a live map server, enable cut, copy, and paste.\nCan also be set in editor.cfg as LIVE_ALLOW_CLIPBOARD=1 under [Network].");
	sizer->Add(live_allow_clipboard_chkbox, 0, wxLEFT | wxTOP, 5);

	editor_page->SetSizer(sizer);
	return editor_page;
}

wxNotebookPage* PreferencesWindow::CreateGraphicsPage() {
	wxWindow* tmp = nullptr;
	wxNotebookPage* graphics_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	hide_items_when_zoomed_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Hide items when zoomed out");
	hide_items_when_zoomed_chkbox->SetValue(g_settings.getBoolean(Config::HIDE_ITEMS_WHEN_ZOOMED));
	sizer->Add(hide_items_when_zoomed_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(hide_items_when_zoomed_chkbox, "When this option is checked, \"loose\" items will be hidden when you zoom very far out.");

	show_chunk_boundaries_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Show chunk boundaries overlay");
	show_chunk_boundaries_chkbox->SetValue(g_settings.getBoolean(Config::SHOW_CHUNK_BOUNDARIES));
	sizer->Add(show_chunk_boundaries_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(show_chunk_boundaries_chkbox, "Draws a highlight every 256 tiles to visualize chunk boundaries while editing.");

	show_mouse_crosshair_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Show mouse crosshair overlay");
	show_mouse_crosshair_chkbox->SetValue(g_settings.getBoolean(Config::SHOW_MOUSE_CROSSHAIR));
	sizer->Add(show_mouse_crosshair_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(show_mouse_crosshair_chkbox, "Draws horizontal and vertical guide lines following the cursor to help align tiles.");

	icon_selection_shadow_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Use icon selection shadow");
	icon_selection_shadow_chkbox->SetValue(g_settings.getBoolean(Config::USE_GUI_SELECTION_SHADOW));
	sizer->Add(icon_selection_shadow_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(icon_selection_shadow_chkbox, "When this option is checked, selected items in the palette menu will be shaded.");

	use_memcached_chkbox = newd wxCheckBox(graphics_page, wxID_ANY, "Use memcached sprites");
	use_memcached_chkbox->SetValue(g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES));
	sizer->Add(use_memcached_chkbox, 0, wxLEFT | wxTOP, 5);
	SetWindowToolTip(use_memcached_chkbox, "When this is checked, sprites will be loaded into memory at startup and unpacked at runtime. This is faster but consumes more memory.\nIf it is not checked, the editor will use less memory but there will be a performance decrease due to reading sprites from the disk.");

	sizer->AddSpacer(10);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	icon_background_choice = newd wxChoice(graphics_page, wxID_ANY);
	icon_background_choice->Append("Transparent background");
	icon_background_choice->Append("Black background");
	icon_background_choice->Append("Gray background");
	icon_background_choice->Append("White background");
	SetIconBackgroundSelection(IconBackgroundSelection());

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Icon background color: "), 0);
	subsizer->Add(icon_background_choice, 0);
	SetWindowToolTip(icon_background_choice, tmp, "This will change the background color on icons in all windows.");

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Cursor color: "), 0);
	subsizer->Add(cursor_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_RED), g_settings.getInteger(Config::CURSOR_GREEN), g_settings.getInteger(Config::CURSOR_BLUE), g_settings.getInteger(Config::CURSOR_ALPHA))), 0);
	SetWindowToolTip(cursor_color_pick, tmp, "The color of the main cursor on the map (while in drawing mode).");

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Secondary cursor color: "), 0);
	subsizer->Add(cursor_alt_color_pick = newd wxColourPickerCtrl(graphics_page, wxID_ANY, wxColor(g_settings.getInteger(Config::CURSOR_ALT_RED), g_settings.getInteger(Config::CURSOR_ALT_GREEN), g_settings.getInteger(Config::CURSOR_ALT_BLUE), g_settings.getInteger(Config::CURSOR_ALT_ALPHA))), 0);
	SetWindowToolTip(cursor_alt_color_pick, tmp, "The color of the secondary cursor on the map (for houses and flags).");

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Crosshair color: "), 0);
	mouse_crosshair_color_pick = newd wxColourPickerCtrl(
		graphics_page,
		wxID_ANY,
		wxColor(
			g_settings.getInteger(Config::CURSOR_CROSSHAIR_RED),
			g_settings.getInteger(Config::CURSOR_CROSSHAIR_GREEN),
			g_settings.getInteger(Config::CURSOR_CROSSHAIR_BLUE)
		)
	);
	mouse_crosshair_color_pick->Enable(show_mouse_crosshair_chkbox->GetValue());
	subsizer->Add(mouse_crosshair_color_pick, 0);
	SetWindowToolTip(mouse_crosshair_color_pick, tmp, "The color used for the mouse crosshair overlay.");
	show_mouse_crosshair_chkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) {
		mouse_crosshair_color_pick->Enable(event.IsChecked());
	});

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Viewport background color: "), 0);
	viewport_background_color_pick = newd wxColourPickerCtrl(
		graphics_page,
		wxID_ANY,
		wxColor(
			g_settings.getInteger(Config::VIEWPORT_BACKGROUND_RED),
			g_settings.getInteger(Config::VIEWPORT_BACKGROUND_GREEN),
			g_settings.getInteger(Config::VIEWPORT_BACKGROUND_BLUE)
		)
	);
	subsizer->Add(viewport_background_color_pick, 0);
	SetWindowToolTip(viewport_background_color_pick, tmp, "Sets the solid colour behind the map view to reduce eye strain or improve contrast with tiles.");

	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screenshot directory: "), 0);
	subsizer->Add(CreateBrowsePathRow(graphics_page, &screenshot_directory_path, wxstr(g_settings.getString(Config::SCREENSHOT_DIRECTORY)), "Screenshots taken in the editor will be saved to this directory."), 1, wxEXPAND);
	SetWindowToolTip(screenshot_directory_path, tmp, "Screenshots taken in the editor will be saved to this directory.");

	screenshot_format_choice = newd wxChoice(graphics_page, wxID_ANY);
	screenshot_format_choice->Append("PNG");
	screenshot_format_choice->Append("JPG");
	screenshot_format_choice->Append("TGA");
	screenshot_format_choice->Append("BMP");
	const std::string screenshot_format = g_settings.getString(Config::SCREENSHOT_FORMAT);
	if (screenshot_format == "jpg") {
		screenshot_format_choice->SetSelection(1);
	} else if (screenshot_format == "tga") {
		screenshot_format_choice->SetSelection(2);
	} else if (screenshot_format == "bmp") {
		screenshot_format_choice->SetSelection(3);
	} else {
		screenshot_format_choice->SetSelection(0);
	}
	subsizer->Add(tmp = newd wxStaticText(graphics_page, wxID_ANY, "Screenshot format: "), 0);
	subsizer->Add(screenshot_format_choice, 0);
	SetWindowToolTip(screenshot_format_choice, tmp, "This will affect the screenshot format used by the editor.\nTo take a screenshot, press F11.");

	sizer->Add(subsizer, 1, wxEXPAND | wxALL, 5);
	graphics_page->SetSizer(sizer);
	return graphics_page;
}

wxChoice* PreferencesWindow::AddPaletteStyleChoice(wxWindow* parent, wxSizer* sizer, const wxString& short_description, const wxString& description, const std::string& setting) {
	wxStaticText* text = newd wxStaticText(parent, wxID_ANY, short_description);
	sizer->Add(text, 0);

	wxChoice* choice = newd wxChoice(parent, wxID_ANY);
	sizer->Add(choice, 0);

	choice->Append("Large Icons");
	choice->Append("Small Icons");
	choice->Append("Listbox with Icons");
	choice->Append("Exact Size Icons");

	text->SetToolTip(description);
	choice->SetToolTip(description);

	if (setting == "large icons") {
		choice->SetSelection(0);
	} else if (setting == "small icons") {
		choice->SetSelection(1);
	} else if (setting == "listbox") {
		choice->SetSelection(2);
	} else if (setting == "actual icons") {
		choice->SetSelection(3);
	}

	return choice;
}

void PreferencesWindow::SetPaletteStyleChoice(wxChoice* ctrl, int key) {
	switch (ctrl->GetSelection()) {
		case 0: g_settings.setString(key, "large icons"); break;
		case 1: g_settings.setString(key, "small icons"); break;
		case 2: g_settings.setString(key, "listbox"); break;
		case 3: g_settings.setString(key, "actual icons"); break;
		default: break;
	}
}

int PreferencesWindow::IconBackgroundSelection() const {
	switch (g_settings.getInteger(Config::ICON_BACKGROUND)) {
		case 255: return 3;
		case 88: return 2;
		case 0: return 1;
		default: return 0;
	}
}

void PreferencesWindow::SetIconBackgroundSelection(int selection) {
	icon_background_choice->SetSelection(selection);
}

wxNotebookPage* PreferencesWindow::CreateUIPage() {
	wxNotebookPage* ui_page = newd wxPanel(book, wxID_ANY);

	wxSizer* sizer = newd wxBoxSizer(wxVERTICAL);

	auto* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);
	theme_choice = newd wxChoice(ui_page, wxID_ANY);
	theme_choice->Append("Dark");
	theme_choice->Append("Light");
	theme_choice->Append("System");
	theme_choice->SetSelection(static_cast<int>(ThemeManager::Get().GetMode()));
	subsizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Theme:"), 0);
	subsizer->Add(theme_choice, 0);
	terrain_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Terrain Palette Style:",
		"Configures the look of the terrain palette.",
		g_settings.getString(Config::PALETTE_TERRAIN_STYLE)
	);
	doodad_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Doodad Palette Style:",
		"Configures the look of the doodad palette.",
		g_settings.getString(Config::PALETTE_DOODAD_STYLE)
	);
	item_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"Item Palette Style:",
		"Configures the look of the item palette.",
		g_settings.getString(Config::PALETTE_ITEM_STYLE)
	);
	raw_palette_style_choice = AddPaletteStyleChoice(
		ui_page, subsizer,
		"RAW Palette Style:",
		"Configures the look of the raw palette.",
		g_settings.getString(Config::PALETTE_RAW_STYLE)
	);

	sizer->Add(subsizer, 0, wxALL, 6);
	sizer->AddSpacer(10);

	large_terrain_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large terrain palette tool && size icons");
	large_terrain_tools_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_TERRAIN_TOOLBAR));
	sizer->Add(large_terrain_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	show_palette_tools_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Show Tools panel in palette");
	show_palette_tools_chkbox->SetValue(g_settings.getBoolean(Config::SHOW_PALETTE_TOOLS));
	sizer->Add(show_palette_tools_chkbox, 0, wxLEFT | wxTOP, 5);

	show_palette_brush_size_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Show Brush Size panel in palette");
	show_palette_brush_size_chkbox->SetValue(g_settings.getBoolean(Config::SHOW_PALETTE_BRUSH_SIZE));
	sizer->Add(show_palette_brush_size_chkbox, 0, wxLEFT | wxTOP, 5);

	large_doodad_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large doodad size palette icons");
	large_doodad_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_DOODAD_SIZEBAR));
	sizer->Add(large_doodad_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_item_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item size palette icons");
	large_item_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_ITEM_SIZEBAR));
	sizer->Add(large_item_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_house_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large house palette size icons");
	large_house_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_HOUSE_SIZEBAR));
	sizer->Add(large_house_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_raw_sizebar_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large raw palette size icons");
	large_raw_sizebar_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_RAW_SIZEBAR));
	sizer->Add(large_raw_sizebar_chkbox, 0, wxLEFT | wxTOP, 5);

	large_container_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large container view icons");
	large_container_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CONTAINER_ICONS));
	sizer->Add(large_container_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	large_pick_item_icons_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use large item picker icons");
	large_pick_item_icons_chkbox->SetValue(g_settings.getBoolean(Config::USE_LARGE_CHOOSE_ITEM_ICONS));
	sizer->Add(large_pick_item_icons_chkbox, 0, wxLEFT | wxTOP, 5);

	container_default_name_search_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Default to \"Find by Name\" when adding container items");
	container_default_name_search_chkbox->SetValue(g_settings.getBoolean(Config::CONTAINER_FIND_DEFAULT_NAMES));
	container_default_name_search_chkbox->SetToolTip("Automatically switch to the name search mode when opening the container item picker.");
	sizer->Add(container_default_name_search_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	switch_mousebtn_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Switch mousebuttons");
	switch_mousebtn_chkbox->SetValue(g_settings.getBoolean(Config::SWITCH_MOUSEBUTTONS));
	switch_mousebtn_chkbox->SetToolTip("Switches the right and center mouse button.");
	sizer->Add(switch_mousebtn_chkbox, 0, wxLEFT | wxTOP, 5);

	doubleclick_properties_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Double click for properties");
	doubleclick_properties_chkbox->SetValue(g_settings.getBoolean(Config::DOUBLECLICK_PROPERTIES));
	doubleclick_properties_chkbox->SetToolTip("Double clicking on a tile will bring up the properties menu for the top item.");
	sizer->Add(doubleclick_properties_chkbox, 0, wxLEFT | wxTOP, 5);

	inversed_scroll_chkbox = newd wxCheckBox(ui_page, wxID_ANY, "Use inversed scroll");
	inversed_scroll_chkbox->SetValue(g_settings.getFloat(Config::SCROLL_SPEED) < 0);
	inversed_scroll_chkbox->SetToolTip("When this checkbox is checked, dragging the map using the center mouse button will be inversed (default RTS behaviour).");
	sizer->Add(inversed_scroll_chkbox, 0, wxLEFT | wxTOP, 5);

	sizer->AddSpacer(10);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Scroll speed: "), 0, wxLEFT | wxTOP, 5);

	const auto true_scrollspeed = int(std::abs(g_settings.getFloat(Config::SCROLL_SPEED)) * 10);
	scroll_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_scrollspeed, 1, max(true_scrollspeed, 100));
	scroll_speed_slider->SetToolTip("This controls how fast the map will scroll when you hold down the center mouse button and move it around.");
	sizer->Add(scroll_speed_slider, 0, wxEXPAND, 5);

	sizer->Add(newd wxStaticText(ui_page, wxID_ANY, "Zoom speed: "), 0, wxLEFT | wxTOP, 5);

	const auto true_zoomspeed = int(g_settings.getFloat(Config::ZOOM_SPEED) * 10);
	zoom_speed_slider = newd wxSlider(ui_page, wxID_ANY, true_zoomspeed, 1, max(true_zoomspeed, 100));
	zoom_speed_slider->SetToolTip("This controls how fast you will zoom when you scroll the center mouse button.");
	sizer->Add(zoom_speed_slider, 0, wxEXPAND, 5);

	ui_page->SetSizer(sizer);
	return ui_page;
}

wxNotebookPage* PreferencesWindow::CreateClientPage() {
	wxNotebookPage* client_page = newd wxPanel(book, wxID_ANY);

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	auto* options_sizer = newd wxFlexGridSizer(2, 10, 10);
	options_sizer->AddGrowableCol(1);

	default_version_choice = newd wxChoice(client_page, wxID_ANY);
	wxStaticText* default_client_tooltip = newd wxStaticText(client_page, wxID_ANY, "Default client version:");
	options_sizer->Add(default_client_tooltip, 0, wxLEFT | wxTOP, 5);
	options_sizer->Add(default_version_choice, 0, wxTOP, 5);
	SetWindowToolTip(default_client_tooltip, default_version_choice, "This will decide what client version will be used when new maps are created.");

	{
		wxArrayString version_names;
		const ClientVersionList versions = ClientVersion::getAllVisible();
		int default_selection = 0;
		const int default_version_id = g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION);
		for (size_t i = 0; i < versions.size(); ++i) {
			version_names.Add(wxstr(versions[i]->getName()));
			if (versions[i]->getID() == default_version_id) {
				default_selection = static_cast<int>(i);
			}
		}
		default_version_choice->Append(version_names);
		if (!version_names.IsEmpty()) {
			default_version_choice->SetSelection(default_selection);
		}
	}

	check_sigs_chkbox = newd wxCheckBox(client_page, wxID_ANY, "Check file signatures");
	check_sigs_chkbox->SetValue(g_settings.getBoolean(Config::CHECK_SIGNATURES));
	check_sigs_chkbox->SetToolTip("When this option is not checked, the editor will load any OTB/DAT/SPR combination without complaints. This may cause graphics bugs.");
	options_sizer->Add(check_sigs_chkbox, 0, wxLEFT | wxRIGHT | wxTOP, 5);

	topsizer->Add(options_sizer, wxSizerFlags(0).Expand());
	topsizer->AddSpacer(10);

	client_list_window = newd wxScrolledWindow(client_page, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL);
	client_list_window->SetMinSize(FROM_DIP(this, wxSize(450, 300)));
	client_list_sizer = newd wxFlexGridSizer(2, 10, 10);
	client_list_sizer->AddGrowableCol(1);
	client_list_window->SetSizer(client_list_sizer);
	client_list_window->SetScrollRate(5, 5);

	topsizer->Add(client_list_window, 1, wxEXPAND | wxALL, 5);
	client_page->SetSizer(topsizer);
	return client_page;
}

void PreferencesWindow::EnsureClientVersionListBuilt() {
	if (client_list_built) {
		return;
	}

	wxBusyCursor busy;
	client_list_window->Freeze();

	version_dir_pickers.clear();
	version_items_dir_pickers.clear();
	version_monsters_dir_pickers.clear();
	version_npcs_dir_pickers.clear();

	const ClientVersionList versions = ClientVersion::getAllVisible();
	int version_counter = 0;

	for (ClientVersion* version : versions) {
		wxString client_tooltip;
		client_tooltip << "The editor will look for " << wxstr(version->getName()) << " DAT & SPR here.";
		auto* version_label = newd wxStaticText(client_list_window, wxID_ANY, wxstr(version->getName()));
		version_label->SetToolTip(client_tooltip);
		client_list_sizer->Add(version_label, wxSizerFlags(0).Expand());

		wxTextCtrl* client_path = nullptr;
		client_list_sizer->Add(
			CreateBrowsePathRow(client_list_window, &client_path, version->getClientPath().GetFullPath(), client_tooltip),
			wxSizerFlags(0).Border(wxRIGHT, 10).Expand()
		);
		version_dir_pickers.push_back(client_path);

		wxString items_tooltip;
		items_tooltip << "The editor will load items.otb and items.xml for " << wxstr(version->getName()) << " from this folder.";
		auto* items_label = newd wxStaticText(client_list_window, wxID_ANY, "Items directory:");
		items_label->SetToolTip(items_tooltip);
		client_list_sizer->Add(items_label, wxSizerFlags(0).Border(wxLEFT, 20));

		const FileName version_items_path = version->hasCustomItemsPath() ? version->getCustomItemsPath() : version->getDataPath();
		wxTextCtrl* items_path = nullptr;
		client_list_sizer->Add(
			CreateBrowsePathRow(client_list_window, &items_path, version_items_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), items_tooltip),
			wxSizerFlags(0).Border(wxRIGHT, 10).Expand()
		);
		version_items_dir_pickers.push_back(items_path);

		wxString monsters_tooltip;
		monsters_tooltip << "The editor will load monster XML files for " << wxstr(version->getName()) << " from this folder.";
		auto* monsters_label = newd wxStaticText(client_list_window, wxID_ANY, "Monsters directory:");
		monsters_label->SetToolTip(monsters_tooltip);
		client_list_sizer->Add(monsters_label, wxSizerFlags(0).Border(wxLEFT, 20));

		const FileName version_monsters_path = version->hasCustomMonstersPath() ? version->getCustomMonstersPath() : FileName();
		wxTextCtrl* monsters_path = nullptr;
		client_list_sizer->Add(
			CreateBrowsePathRow(client_list_window, &monsters_path, version_monsters_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), monsters_tooltip),
			wxSizerFlags(0).Border(wxRIGHT, 10).Expand()
		);
		version_monsters_dir_pickers.push_back(monsters_path);

		wxString npcs_tooltip;
		npcs_tooltip << "The editor will load NPC XML files for " << wxstr(version->getName()) << " from this folder.";
		auto* npcs_label = newd wxStaticText(client_list_window, wxID_ANY, "NPC directory:");
		npcs_label->SetToolTip(npcs_tooltip);
		client_list_sizer->Add(npcs_label, wxSizerFlags(0).Border(wxLEFT, 20));

		const FileName version_npcs_path = version->hasCustomNpcsPath() ? version->getCustomNpcsPath() : FileName();
		wxTextCtrl* npcs_path = nullptr;
		client_list_sizer->Add(
			CreateBrowsePathRow(client_list_window, &npcs_path, version_npcs_path.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), npcs_tooltip),
			wxSizerFlags(0).Border(wxRIGHT, 10).Expand()
		);
		version_npcs_dir_pickers.push_back(npcs_path);

		++version_counter;
	}

	client_list_window->FitInside();
	client_list_window->Layout();
	client_list_window->Thaw();
	client_list_built = true;
}

void PreferencesWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	if (Apply()) {
		EndModal(0);
	}
}

void PreferencesWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(0);
}

void PreferencesWindow::OnClickApply(wxCommandEvent& WXUNUSED(event)) {
	Apply();
}

bool PreferencesWindow::Apply() {
	const ThemeMode themeMode = ThemeManager::ModeFromChoice(theme_choice->GetSelection());
	if (!ThemeManager::Get().Apply(themeMode, g_gui.root)) {
		wxMessageBox("The selected theme could not be applied.", "Theme", wxOK | wxICON_ERROR, this);
		return false;
	}
	g_settings.setInteger(Config::UI_THEME, static_cast<int>(themeMode));

	bool must_restart = false;
	bool palette_update_needed = false;

	const wxString open_map_path = open_map_on_startup_path->GetValue();
	const bool open_map_on_startup_enabled = open_map_on_startup_chkbox->GetValue() && !open_map_path.empty();

	g_settings.setString(Config::OPEN_MAP_ON_STARTUP_PATH, nstr(open_map_path));
	g_settings.setInteger(Config::OPEN_MAP_ON_STARTUP, open_map_on_startup_enabled ? 1 : 0);

	if (open_map_on_startup_enabled && show_welcome_dialog_chkbox->GetValue() != 0) {
		show_welcome_dialog_chkbox->SetValue(false);
	}

	g_settings.setInteger(Config::WELCOME_DIALOG, open_map_on_startup_enabled ? 0 : show_welcome_dialog_chkbox->GetValue());
	g_settings.setInteger(Config::ALWAYS_MAKE_BACKUP, always_make_backup_chkbox->GetValue());
	g_settings.setInteger(Config::ONLY_ONE_INSTANCE, only_one_instance_chkbox->GetValue());
	g_settings.setInteger(Config::IGNORE_SAVE_PROMPT, ignore_save_prompt_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_SAVE_ON_CLOSE, auto_save_on_close_chkbox->GetValue());
	g_settings.setInteger(Config::UNDO_SIZE, undo_size_spin->GetValue());
	g_settings.setInteger(Config::UNDO_MEM_SIZE, undo_mem_size_spin->GetValue());
	g_settings.setInteger(Config::WORKER_THREADS, worker_threads_spin->GetValue());
	g_settings.setInteger(Config::REPLACE_SIZE, replace_size_spin->GetValue());
	g_settings.setInteger(Config::COPY_POSITION_FORMAT, position_format->GetSelection());

	if (g_settings.getBoolean(Config::SHOW_TILESET_EDITOR) != enable_tileset_editing_chkbox->GetValue()) {
		palette_update_needed = true;
	}
	g_settings.setInteger(Config::SHOW_TILESET_EDITOR, enable_tileset_editing_chkbox->GetValue());

	g_settings.setInteger(Config::GROUP_ACTIONS, group_actions_chkbox->GetValue());
	g_settings.setInteger(Config::WARN_FOR_DUPLICATE_ID, duplicate_id_warn_chkbox->GetValue());
	g_settings.setInteger(Config::HOUSE_BRUSH_REMOVE_ITEMS, house_remove_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_ASSIGN_DOORID, auto_assign_doors_chkbox->GetValue());
	g_settings.setInteger(Config::ERASER_LEAVE_UNIQUE, eraser_leave_unique_chkbox->GetValue());
	g_settings.setInteger(Config::DOODAD_BRUSH_ERASE_LIKE, doodad_erase_same_chkbox->GetValue());
	g_settings.setInteger(Config::AUTO_CREATE_SPAWN, auto_create_spawn_chkbox->GetValue());
	g_settings.setInteger(Config::RAW_LIKE_SIMONE, allow_multiple_orderitems_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_MOVE, merge_move_chkbox->GetValue());
	g_settings.setInteger(Config::MERGE_PASTE, merge_paste_chkbox->GetValue());
	g_settings.setInteger(Config::LIVE_ALLOW_CLIPBOARD, live_allow_clipboard_chkbox->GetValue());

	g_settings.setInteger(Config::USE_GUI_SELECTION_SHADOW, icon_selection_shadow_chkbox->GetValue());
	if (g_settings.getBoolean(Config::USE_MEMCACHED_SPRITES) != use_memcached_chkbox->GetValue()) {
		must_restart = true;
	}
	g_settings.setInteger(Config::USE_MEMCACHED_SPRITES_TO_SAVE, use_memcached_chkbox->GetValue());

	static const int icon_background_values[] = { -1, 0, 88, 255 };
	const int icon_background_selection = icon_background_choice->GetSelection();
	if (icon_background_selection >= 0 && icon_background_selection < 4) {
		const int new_background = icon_background_values[icon_background_selection];
		if (g_settings.getInteger(Config::ICON_BACKGROUND) != new_background) {
			g_gui.gfx.cleanSoftwareSprites();
		}
		g_settings.setInteger(Config::ICON_BACKGROUND, new_background);
	}

	g_settings.setString(Config::SCREENSHOT_DIRECTORY, nstr(screenshot_directory_path->GetValue()));

	const std::string new_format = nstr(screenshot_format_choice->GetStringSelection());
	if (new_format == "PNG") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "png");
	} else if (new_format == "TGA") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "tga");
	} else if (new_format == "JPG") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "jpg");
	} else if (new_format == "BMP") {
		g_settings.setString(Config::SCREENSHOT_FORMAT, "bmp");
	}

	wxColor clr = cursor_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_BLUE, clr.Blue());

	clr = cursor_alt_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_ALT_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_ALT_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_ALT_BLUE, clr.Blue());

	clr = mouse_crosshair_color_pick->GetColour();
	g_settings.setInteger(Config::CURSOR_CROSSHAIR_RED, clr.Red());
	g_settings.setInteger(Config::CURSOR_CROSSHAIR_GREEN, clr.Green());
	g_settings.setInteger(Config::CURSOR_CROSSHAIR_BLUE, clr.Blue());

	clr = viewport_background_color_pick->GetColour();
	g_settings.setInteger(Config::VIEWPORT_BACKGROUND_RED, clr.Red());
	g_settings.setInteger(Config::VIEWPORT_BACKGROUND_GREEN, clr.Green());
	g_settings.setInteger(Config::VIEWPORT_BACKGROUND_BLUE, clr.Blue());

	g_settings.setInteger(Config::HIDE_ITEMS_WHEN_ZOOMED, hide_items_when_zoomed_chkbox->GetValue());
	g_settings.setInteger(Config::SHOW_CHUNK_BOUNDARIES, show_chunk_boundaries_chkbox->GetValue());
	g_settings.setInteger(Config::SHOW_MOUSE_CROSSHAIR, show_mouse_crosshair_chkbox->GetValue());

	SetPaletteStyleChoice(terrain_palette_style_choice, Config::PALETTE_TERRAIN_STYLE);
	SetPaletteStyleChoice(doodad_palette_style_choice, Config::PALETTE_DOODAD_STYLE);
	SetPaletteStyleChoice(item_palette_style_choice, Config::PALETTE_ITEM_STYLE);
	SetPaletteStyleChoice(raw_palette_style_choice, Config::PALETTE_RAW_STYLE);
	g_settings.setInteger(Config::USE_LARGE_TERRAIN_TOOLBAR, large_terrain_tools_chkbox->GetValue());
	g_settings.setInteger(Config::SHOW_PALETTE_TOOLS, show_palette_tools_chkbox->GetValue());
	g_settings.setInteger(Config::SHOW_PALETTE_BRUSH_SIZE, show_palette_brush_size_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_DOODAD_SIZEBAR, large_doodad_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_ITEM_SIZEBAR, large_item_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_HOUSE_SIZEBAR, large_house_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_RAW_SIZEBAR, large_raw_sizebar_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CONTAINER_ICONS, large_container_icons_chkbox->GetValue());
	g_settings.setInteger(Config::USE_LARGE_CHOOSE_ITEM_ICONS, large_pick_item_icons_chkbox->GetValue());
	g_settings.setInteger(Config::CONTAINER_FIND_DEFAULT_NAMES, container_default_name_search_chkbox->GetValue());

	g_settings.setInteger(Config::SWITCH_MOUSEBUTTONS, switch_mousebtn_chkbox->GetValue());
	g_settings.setInteger(Config::DOUBLECLICK_PROPERTIES, doubleclick_properties_chkbox->GetValue());

	float scroll_mul = 1.0f;
	if (inversed_scroll_chkbox->GetValue()) {
		scroll_mul = -1.0f;
	}
	g_settings.setFloat(Config::SCROLL_SPEED, scroll_mul * scroll_speed_slider->GetValue() / 10.f);
	g_settings.setFloat(Config::ZOOM_SPEED, zoom_speed_slider->GetValue() / 10.f);

	if (client_list_built) {
		const ClientVersionList versions = ClientVersion::getAllVisible();
		int version_counter = 0;
		for (ClientVersion* version : versions) {
			version->setClientPath(FileName(NormalizeDirectoryPath(version_dir_pickers[version_counter]->GetValue())));

			const wxString items_dir = NormalizeDirectoryPath(version_items_dir_pickers[version_counter]->GetValue());
			const wxString default_items_dir = version->getDataPath().GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);
			if (items_dir.IsEmpty() || items_dir.CmpNoCase(default_items_dir) == 0) {
				version->clearItemsPath();
			} else {
				FileName items_path;
				items_path.Assign(items_dir);
				version->setItemsPath(items_path);
			}

			const wxString monsters_dir = NormalizeDirectoryPath(version_monsters_dir_pickers[version_counter]->GetValue());
			if (monsters_dir.IsEmpty()) {
				version->clearMonstersPath();
			} else {
				FileName monsters_path;
				monsters_path.Assign(monsters_dir);
				version->setMonstersPath(monsters_path);
			}

			const wxString npcs_dir = NormalizeDirectoryPath(version_npcs_dir_pickers[version_counter]->GetValue());
			if (npcs_dir.IsEmpty()) {
				version->clearNpcsPath();
			} else {
				FileName npcs_path;
				npcs_path.Assign(npcs_dir);
				version->setNpcsPath(npcs_path);
			}

			++version_counter;
		}
	}

	const std::string selected_version_name = nstr(default_version_choice->GetStringSelection());
	for (ClientVersion* version : ClientVersion::getAllVisible()) {
		if (version->getName() == selected_version_name) {
			g_settings.setInteger(Config::DEFAULT_CLIENT_VERSION, version->getID());
			break;
		}
	}

	g_settings.setInteger(Config::CHECK_SIGNATURES, check_sigs_chkbox->GetValue());

	ClientVersion::saveVersions();
	ClientVersion::loadVersions();

	g_settings.save();

	if (must_restart) {
		g_gui.PopupDialog(this, "Notice", "You must restart the editor for the changes to take effect.", wxOK);
	}

	if (!palette_update_needed) {
		g_gui.RebuildPalettes();
	} else {
		wxString error;
		wxArrayString warnings;
		g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true);
		g_gui.PopupDialog("Error", error, wxOK);
		g_gui.ListDialog("Warnings", warnings);
	}

	return true;
}
