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

#include "apis/SkinManager.h"
#include "global/SkinDefaults.h"
#include <wx/artprov.h>
#include <wx/filename.h>
#include "generated/configure_launcher.h"

#include "global/MemoryDebugging.h" // Last include for memory debugging

class ArtProvider: public wxArtProvider {
public:
	ArtProvider(SkinSystem *skinSystem);
private:
	SkinSystem *skinSystem;
	virtual wxBitmap CreateBitmap(const wxArtID& id, const wxArtClient& client, const wxSize& size);
};

Skin::Skin() {
	this->windowTitle = NULL;
	this->windowIcon = NULL;
	this->welcomeHeader = NULL;
	this->idealIcon = NULL;
	this->welcomePageText = NULL;
	this->warningIcon = NULL;
	this->bigWarningIcon = NULL;
}

Skin::~Skin() {
	if (this->windowTitle != NULL) delete this->windowTitle;
	if (this->windowIcon != NULL) delete this->windowIcon;
	if (this->welcomeHeader != NULL) delete this->welcomeHeader;
	if (this->idealIcon != NULL) delete this->idealIcon;
	if (this->welcomePageText != NULL) delete this->welcomePageText;
	if (this->warningIcon != NULL) delete this->warningIcon;
	if (this->bigWarningIcon != NULL) delete this->bigWarningIcon;
}

SkinSystem* SkinSystem::skinSystem = NULL;

bool SkinSystem::Initialize() {
	wxASSERT(!SkinSystem::IsInitialized());
	
	SkinSystem::skinSystem = new SkinSystem();
	return true;
}

void SkinSystem::DeInitialize() {
	wxASSERT(SkinSystem::IsInitialized());
	
	SkinSystem* temp = SkinSystem::skinSystem;
	SkinSystem::skinSystem = NULL;
	delete temp;
}

bool SkinSystem::IsInitialized() {
	return (SkinSystem::skinSystem != NULL); 
}

SkinSystem* SkinSystem::GetSkinSystem() {
	wxCHECK_MSG(SkinSystem::IsInitialized(),
		NULL,
		_T("Attempt to get skin system when it has not been initialized."));
	
	return SkinSystem::skinSystem;
}

SkinSystem::SkinSystem()
: TCSkin(NULL), font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT)) {

	wxArtProvider::Push(new ArtProvider(this));

	InitializeDefaultSkin();
}

SkinSystem::~SkinSystem() {
	if (this->TCSkin != NULL) delete this->TCSkin;
}

void SkinSystem::InitializeDefaultSkin() {
	this->defaultSkin.windowTitle = new wxString(DEFAULT_SKIN_WINDOW_TITLE);

	wxFileName filename(_T(RESOURCES_PATH), DEFAULT_SKIN_WINDOW_ICON);
	this->defaultSkin.windowIcon =
		new wxIcon(filename.GetFullPath(), wxBITMAP_TYPE_ANY);
	if (!this->defaultSkin.windowIcon->IsOk()) {
		wxLogFatalError(_T("Default window icon '%s' not valid"),
			filename.GetFullPath().c_str());
	}
	
	filename = wxFileName(_T(RESOURCES_PATH), DEFAULT_SKIN_BANNER);
	this->defaultSkin.welcomeHeader = 
		new wxBitmap(filename.GetFullPath(), wxBITMAP_TYPE_ANY);
	if (!this->defaultSkin.welcomeHeader->IsOk()) {
		wxLogFatalError(_T("Default welcome header '%s' not valid"),
						filename.GetFullPath().c_str());
	}

	filename = wxFileName(_T(RESOURCES_PATH), DEFAULT_SKIN_ICON_IDEAL);
	this->defaultSkin.idealIcon = 
		new wxBitmap(filename.GetFullPath(), wxBITMAP_TYPE_ANY);
	if (!this->defaultSkin.idealIcon->IsOk()) {
		wxLogFatalError(_T("Default ideal icon '%s' not valid"),
						filename.GetFullPath().c_str());
	}

	this->defaultSkin.welcomePageText =
		new wxString(DEFAULT_SKIN_WELCOME_TEXT);
	
	filename = wxFileName(_T(RESOURCES_PATH), DEFAULT_SKIN_ICON_WARNING);
	this->defaultSkin.warningIcon =
		new wxBitmap(filename.GetFullPath(), wxBITMAP_TYPE_ANY);
	if (!this->defaultSkin.warningIcon->IsOk()) {
		wxLogFatalError(_T("Default warning icon '%s' not valid"),
						filename.GetFullPath().c_str());
	}
	
	filename = wxFileName(_T(RESOURCES_PATH), DEFAULT_SKIN_ICON_WARNING_BIG);
	this->defaultSkin.bigWarningIcon =
		new wxBitmap(filename.GetFullPath(), wxBITMAP_TYPE_ANY);
	if (!this->defaultSkin.bigWarningIcon->IsOk()) {
		wxLogFatalError(_T("Default big warning icon '%s' not valid"),
						filename.GetFullPath().c_str());
	}
	
}

wxString SkinSystem::GetTitle() {
	if ( this->TCSkin != NULL
		&& this->TCSkin->windowTitle != NULL ) {
			return *(this->TCSkin->windowTitle);
	} else {
		return *(this->defaultSkin.windowTitle);
	}
}

wxBitmap SkinSystem::GetIdealIcon() {
	if ( this->TCSkin != NULL
		&& this->TCSkin->idealIcon != NULL ) {
			return *(this->TCSkin->idealIcon);
	} else {
		return *(this->defaultSkin.idealIcon);
	}
}

wxBitmap SkinSystem::GetBanner() {
	if ( this->TCSkin != NULL
		&& this->TCSkin->welcomeHeader != NULL ) {
			return *(this->TCSkin->welcomeHeader);
	} else {
		return *(this->defaultSkin.welcomeHeader);
	}
}

wxString SkinSystem::GetWelcomePageText() {
	if ( this->TCSkin != NULL
		&& this->TCSkin->welcomePageText != NULL ) {
			return *(this->TCSkin->welcomePageText);
	} else {
		return *(this->defaultSkin.welcomePageText);
	}
}

wxBitmap SkinSystem::GetWarningIcon() {
	if ( this->TCSkin != NULL
		&& this->TCSkin->warningIcon != NULL ) {
			return *(this->TCSkin->warningIcon);
	} else {
		return *(this->defaultSkin.warningIcon);
	}
}

wxBitmap SkinSystem::GetBigWarningIcon() {
	if ( this->TCSkin != NULL
		&& this->TCSkin->bigWarningIcon != NULL ) {
			return *(this->TCSkin->bigWarningIcon);
	} else {
			return *(this->defaultSkin.bigWarningIcon);
	}
}






/** Opens, verifies and resizes (if nessisary) the 255x112 image that is needed
on the mods page. 
\note Does allocate memory.*/
wxBitmap* SkinSystem::VerifySmallImage(wxString current, wxString shortmodname,
									   wxString filepath) {
	wxFileName filename;
	if ( SkinSystem::SearchFile(&filename, current, shortmodname, filepath) ) {
		wxLogDebug(_T("   Opening: %s"), filename.GetFullPath().c_str());
		wxImage image(filename.GetFullPath());
		if ( image.IsOk() ) {
			if ( image.GetWidth() > 255 || image.GetHeight() > 112 ) {
				wxLogDebug(_T("   Resizing."));
				image = image.Scale(255, 112, wxIMAGE_QUALITY_HIGH);
			}
			return new wxBitmap(image);
		} else {
			wxLogDebug(_T("   Image is not Ok!"));
		}
	}
	return NULL;
}

/** Opens, verifies, the window icon, returning NULL if anything is not valid.
\note Does allocate memory. */
wxIcon* SkinSystem::VerifyWindowIcon(wxString current, wxString shortmodname,
									   wxString filepath) {
   wxFileName filename;
   if ( SkinSystem::SearchFile(&filename, current, shortmodname, filepath) ) {
	   wxLogDebug(_T("   Opening: %s"), filename.GetFullPath().c_str());

	   wxIcon icon(filename.GetFullPath()); // TODO: specify icon format? verify extension?
	   if ( icon.IsOk() ) { // FIXME: check correct width/height to use
		   if ( icon.GetWidth() == 32 && icon.GetHeight() == 32 ) {
			   return new wxIcon(icon);
		   } else {
			   wxLogDebug(_T("   Icon size wrong"));
		   }
	   } else {
		   wxLogDebug(_T("   Icon not valid."));
	   }
   }
   return NULL;
}

/** Returns true if function is able to get a valid filename object for the
passed paths.  Filename is returned via the param filename. */
bool SkinSystem::SearchFile(wxFileName *filename, wxString currentTC,
							wxString shortmodname, wxString filepath) {
	if (shortmodname.IsEmpty()) { // indicates that the mod is (No mod)
		filename->Assign(
			wxString::Format(_T("%s"), filepath.c_str()));
	} else {
		filename->Assign(
			wxString::Format(_T("%s/%s"), shortmodname.c_str(), filepath.c_str()));
	}

	if ( filename->Normalize(wxPATH_NORM_ALL, currentTC, wxPATH_UNIX) ) {
		if ( filename->IsOk() ) {
			if ( filename->FileExists() ) {
				return true;
			} else {
				wxLogDebug(_T("   File '%s' does not exist"),
					filename->GetFullPath().c_str());
			}
		} else {
			wxLogDebug(_T("   File '%s' is not valid"), filename->GetFullPath().c_str());
		}
	} else {
		wxLogDebug(_T("   Unable to normalize '%s' '%s' '%s'"),	currentTC.c_str(),
			shortmodname.c_str(), filepath.c_str());
	}
	return false;
}

/** Verifies that the ideal icon exists and is the correct size. Returns a
new wxBitmap allocated on the heap, otherwise returns NULL if any errors.*/
wxBitmap* SkinSystem::VerifyIdealIcon(wxString currentTC, wxString shortname,
									  wxString filepath) {
	  wxFileName filename;
	  if ( SkinSystem::SearchFile(&filename, currentTC, shortname, filepath) ) {
		  wxLogDebug(_T("   Opening: %s"), filename.GetFullPath().c_str());

		  wxImage image(filename.GetFullPath());

		  if ( image.IsOk() ) {
			  if ( image.GetWidth() == static_cast<int>(SkinSystem::IdealIconWidth)
				  && image.GetHeight() == static_cast<int>(SkinSystem::IdealIconHeight) ) { 
					  return new wxBitmap(image);
			  } else {
				  wxLogDebug(_T("   Icon is incorrect size. Got (%d,%d); Need (%d,%d)"),
					  image.GetWidth(), image.GetHeight(),
					  SkinSystem::IdealIconWidth, SkinSystem::IdealIconHeight);
			  }
		  } else {
			  wxLogDebug(_T("   Icon is not valid."));
		  }
	  }
	  return NULL;
}

wxBitmap SkinSystem::MakeModsListImage(const wxBitmap &orig) {
	wxImage temp = orig.ConvertToImage();
	wxImage temp1 = temp.Scale(SkinSystem::ModsListImageWidth,
		SkinSystem::ModsListImageHeight,
		wxIMAGE_QUALITY_HIGH);
	wxBitmap outimg = wxBitmap(temp1);
	wxASSERT( outimg.GetWidth() == static_cast<int>(SkinSystem::ModsListImageWidth));
	wxASSERT( outimg.GetHeight() == static_cast<int>(SkinSystem::ModsListImageHeight));
	return outimg;
}

ArtProvider::ArtProvider(SkinSystem *skinSystem) {
	this->skinSystem = skinSystem;
}

wxBitmap ArtProvider::CreateBitmap(const wxArtID &id, const wxArtClient &client, const wxSize &size) {
	wxBitmap bitmap;
	if ( id == wxART_HELP ) {
		wxFileName filename(_T(RESOURCES_PATH), DEFAULT_SKIN_ICON_HELP);
		if ( bitmap.LoadFile(filename.GetFullPath(), wxBITMAP_TYPE_ANY) ) {
			return bitmap;
		} else {
			return wxNullBitmap;
		}
	} else {
		return wxNullBitmap;
	}
}
