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

#include <wx/grid.h>
#include <wx/menu.h>
#include <wx/textctrl.h>

#include "gui.h"
#include "editor_tabs.h"
#include "live_tab.h"
#include "live_socket.h"
#include "live_peer.h"

enum {
	LIVE_LOG_MENU_COPY = wxID_HIGHEST + 1,
	LIVE_LOG_MENU_SELECT_ALL,
};

LiveLogTab::LiveLogTab(MapTabbook* aui, LiveSocket* socket) :
	EditorTab(),
	wxPanel(aui),
	aui(aui),
	socket(socket),
	log(nullptr),
	user_list(nullptr) {
	if (this->socket) {
		this->socket->log = this;
		for (const wxString& pending : this->socket->pendingLogMessages) {
			Message(pending);
		}
		this->socket->pendingLogMessages.clear();
	}

	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxPanel* splitter = newd wxPanel(this);
	topsizer->Add(splitter, 1, wxEXPAND);

	// Setup left panel (the log)
	wxPanel* left_pane = newd wxPanel(splitter);
	wxSizer* left_sizer = newd wxBoxSizer(wxVERTICAL);

	wxFont time_font(*wxSWISS_FONT);

	log = newd wxTextCtrl(
		left_pane,
		wxID_ANY,
		wxEmptyString,
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP
	);
	log->SetFont(time_font);
	log->Bind(wxEVT_CONTEXT_MENU, &LiveLogTab::OnLogContextMenu, this);
	log->Bind(wxEVT_MENU, &LiveLogTab::OnLogMenu, this);

	left_sizer->Add(log, 1, wxEXPAND);
	left_pane->SetSizerAndFit(left_sizer);

	// Setup right panel (connected users)
	user_list = newd wxGrid(splitter, wxID_ANY, wxDefaultPosition, wxSize(280, 100));
	user_list->CreateGrid(0, 3);
	user_list->DisableDragRowSize();
	user_list->DisableDragColSize();
	user_list->SetSelectionMode(wxGrid::wxGridSelectRows);
	user_list->SetRowLabelSize(0);
	user_list->EnableEditing(false);

	user_list->SetColLabelValue(0, "");
	user_list->SetColSize(0, 24);
	user_list->SetColLabelValue(1, "#");
	user_list->SetColSize(1, 36);
	user_list->SetColLabelValue(2, "Name");
	user_list->SetColSize(2, 200);

	// Finalize
	SetSizerAndFit(topsizer);

	wxSizer* split_sizer = newd wxBoxSizer(wxHORIZONTAL);
	split_sizer->Add(left_pane, wxSizerFlags(1).Expand());
	split_sizer->Add(user_list, wxSizerFlags(0).Expand());
	splitter->SetSizerAndFit(split_sizer);

	aui->AddTab(this, true);
}

LiveLogTab::~LiveLogTab() {
	// Clear the socket's back-reference so it does not dangle if the tab is
	// destroyed while the socket is still alive.
	if (socket) {
		socket->log = nullptr;
		socket = nullptr;
	}
}

bool LiveLogTab::IsCurrent() const {
	auto editor_tab = aui->GetCurrentTab();
	if (!editor_tab) {
		return false;
	}

	auto live_tab = dynamic_cast<LiveLogTab*>(editor_tab);
	if (!live_tab) {
		return false;
	}

	return live_tab == this;
}

wxString LiveLogTab::GetTitle() const {
	if (socket) {
		return "Live Log - " + wxstr(socket->getHostName());
	}
	return "Live Log - Disconnected";
}

void LiveLogTab::Disconnect() {
	if (socket) {
		socket->log = nullptr;
	}
	socket = nullptr;
	Refresh();
}

static wxString format00(wxDateTime::wxDateTime_t t) {
	wxString str;
	if (t < 10) {
		str << "0";
	}
	str << t;
	return str;
}

void LiveLogTab::Message(const wxString& str) {
	wxDateTime t = wxDateTime::Now();
	wxString line;
	line << format00(t.GetHour()) << ":"
		 << format00(t.GetMinute()) << ":"
		 << format00(t.GetSecond())
		 << "\tServer\t"
		 << str
		 << "\n";

	log->AppendText(line);
	log->ShowPosition(log->GetLastPosition());
}

void LiveLogTab::OnLogContextMenu(wxContextMenuEvent& evt) {
	wxMenu menu;
	menu.Append(LIVE_LOG_MENU_COPY, "&Copy\tCtrl+C");
	menu.Append(LIVE_LOG_MENU_SELECT_ALL, "Select &All\tCtrl+A");
	log->PopupMenu(&menu);
}

void LiveLogTab::OnLogMenu(wxCommandEvent& evt) {
	switch (evt.GetId()) {
		case LIVE_LOG_MENU_COPY:
			if (log->CanCopy()) {
				log->Copy();
			}
			break;
		case LIVE_LOG_MENU_SELECT_ALL:
			log->SelectAll();
			break;
		default:
			break;
	}
}

void LiveLogTab::OnResizeClientList(wxSizeEvent& evt) {
	evt.Skip();
}

void LiveLogTab::UpdateClientList(const std::unordered_map<uint32_t, LivePeer*>& updatedClients) {
	if (user_list->GetNumberRows() > 0) {
		user_list->DeleteRows(0, user_list->GetNumberRows());
	}

	clients = updatedClients;
	user_list->AppendRows(clients.size());

	int32_t i = 0;
	for (auto& clientEntry : clients) {
		LivePeer* peer = clientEntry.second;
		user_list->SetCellBackgroundColour(i, 0, peer->getUsedColor());
		user_list->SetCellValue(i, 1, i2ws((peer->getClientId() >> 1) + 1));
		user_list->SetCellValue(i, 2, peer->getName());
		++i;
	}
}
