/*
Copyright (C) 2009-2010 wxLauncher Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef ADVSETTINGSPAGE_H
#define ADVSETTINGSPAGE_H

#include <wx/wx.h>
#include "apis/SkinManager.h"
#include "controls/FlagList.h"

class AdvSettingsPage: public wxPanel {
public:
	AdvSettingsPage(wxWindow* parent, SkinSystem* skin);

	void OnNeedUpdateCommandLine(wxCommandEvent &event);

private:
	wxString FormatCommandLineString(const wxString& origCmdLine,
									 const int textAreaWidth);
	SkinSystem *skin;
	FlagListBox* flagListBox;

	// indicates whether a given call to OnExeChanged()
	// is the first call to OnExeChanged() in the current launcher session
	bool isFirstOnExeChanged;
	
public:
	void OnExeChanged(wxCommandEvent& event);
	void OnSelectFlagSet(wxCommandEvent& event);
	void OnDrawStatusChange(wxCommandEvent& event);

	DECLARE_EVENT_TABLE()
};

#endif