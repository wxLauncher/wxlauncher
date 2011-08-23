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

#include <wx/wx.h>
#include <wx/fileconf.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/dir.h>

#include "generated/configure_launcher.h"
#include "apis/ProfileManager.h"
#include "apis/PlatformProfileManager.h"
#include "wxLauncherApp.h"
#include "global/ids.h"

#include "global/MemoryDebugging.h"

ProMan* ProMan::proman = NULL;
bool ProMan::isInitialized = false;

const wxString& ProMan::DEFAULT_PROFILE_NAME = _T("Default");
#define GLOBAL_INI_FILE_NAME _T("global.ini")

///////////// Events

/** EVT_PROFILE_EVENT */
DEFINE_EVENT_TYPE(EVT_PROFILE_CHANGE);

DEFINE_EVENT_TYPE(EVT_CURRENT_PROFILE_CHANGED);

#include <wx/listimpl.cpp> // required magic incatation
WX_DEFINE_LIST(EventHandlers);


void ProMan::GenerateChangeEvent() {
	wxCommandEvent event(EVT_PROFILE_CHANGE, wxID_NONE);
	wxLogDebug(_T("Generating profile change event"));
	EventHandlers::iterator iter = this->eventHandlers.begin();
	do {
		wxEvtHandler* current = *iter;
		current->AddPendingEvent(event);
		wxLogDebug(_T(" Sent Profile Change event to %p"), &(*iter));
		iter++;
	} while (iter != this->eventHandlers.end());
}

void ProMan::GenerateCurrentProfileChangedEvent() {
	wxCommandEvent event(EVT_CURRENT_PROFILE_CHANGED, wxID_NONE);
	wxLogDebug(_T("Generating current profile changed event"));
	EventHandlers::iterator iter = this->eventHandlers.begin();
	while (iter != this->eventHandlers.end()) {
		wxEvtHandler* current = *iter;
		current->AddPendingEvent(event);
		wxLogDebug(_T(" Sent current profile changed event to %p"), &(*iter));
		iter++;
	} 
}

void ProMan::AddEventHandler(wxEvtHandler *handler) {
	this->eventHandlers.Append(handler);
}

void ProMan::RemoveEventHandler(wxEvtHandler *handler) {
	this->eventHandlers.DeleteObject(handler);
}
	

/** Sets up the profile manager. Must be called on program startup so that
it can intercept global wxWidgets configuation functions. 
\return true when setup was successful, false if proman is not
ready and the program should not continue. */
bool ProMan::Initialize() {
	wxConfigBase::DontCreateOnDemand();

	ProMan::proman = new ProMan();

	wxFileName file;
	file.Assign(GET_PROFILE_STORAGEFOLDER(), GLOBAL_INI_FILE_NAME);

	if ( !file.IsOk() ) {
		wxLogError(_T(" '%s' is not valid!"), file.GetFullPath().c_str());
		return false;
	}

	wxLogInfo(_T(" My profiles file is: %s"), file.GetFullPath().c_str());
	if ( !wxFileName::DirExists(file.GetPath())
		&& !wxFileName::Mkdir(file.GetPath(), 0700, wxPATH_MKDIR_FULL ) ) {
		wxLogError(_T(" Unable to make profile folder."));
		return false;
	}

	wxFFileInputStream globalProfileInput(file.GetFullPath(),
		(file.FileExists())?_T("rb"):_T("w+b"));
	ProMan::proman->globalProfile = new wxFileConfig(globalProfileInput);

	// fetch all profiles.
	wxArrayString foundProfiles;
	wxDir::GetAllFiles(GET_PROFILE_STORAGEFOLDER(), &foundProfiles, _T("pro?????.ini"));

	wxLogInfo(_T(" Found %d profile(s)."), foundProfiles.Count());
	for( size_t i = 0; i < foundProfiles.Count(); i++) {
		wxLogDebug(_T("  Opening %s"), foundProfiles[i].c_str());
		wxFFileInputStream instream(foundProfiles[i]);
		wxFileConfig *config = new wxFileConfig(instream);
		
		wxString name;
		config->Read(PRO_CFG_MAIN_NAME, &name, wxString::Format(_T("Profile %05d"), i));

		ProMan::proman->profiles[name] = config;
		wxLogDebug(_T("  Opened profile named: %s"), name.c_str());
	}

	wxString currentProfile;
	ProMan::proman->globalProfile->Read(
		GBL_CFG_MAIN_LASTPROFILE, &currentProfile, ProMan::DEFAULT_PROFILE_NAME);
	
	wxLogDebug(_T(" Searching for profile: %s"), currentProfile.c_str());
	if ( ProMan::proman->profiles.find(currentProfile)
	== ProMan::proman->profiles.end() ) {
		// lastprofile does not exist
		wxLogDebug(_T(" lastprofile '%s' does not exist!"), currentProfile.c_str());
		if ( ProMan::proman->profiles.find(ProMan::DEFAULT_PROFILE_NAME)
		== ProMan::proman->profiles.end() ) {
			// default profile also does not exist.
			// Means this is likely the first run this system
			// Create a default profile
			wxLogInfo(_T(" Default profile does not exist! Creating..."));
			ProMan::proman->CreateNewProfile(ProMan::DEFAULT_PROFILE_NAME);
			wxLogInfo(_T(" Priming profile..."));
			PullProfile(ProMan::proman->profiles[ProMan::DEFAULT_PROFILE_NAME]);
		}
		wxLogInfo(_T(" Resetting lastprofile to Default."));
		ProMan::proman->globalProfile->Write(GBL_CFG_MAIN_LASTPROFILE, ProMan::DEFAULT_PROFILE_NAME);
		wxFFileOutputStream globalProfileOutput(file.GetFullPath());
		ProMan::proman->globalProfile->Save(globalProfileOutput);
		currentProfile = ProMan::DEFAULT_PROFILE_NAME;
	}

	wxLogDebug(_T(" Making '%s' the application profile"), currentProfile.c_str());
	if ( !ProMan::proman->SwitchTo(currentProfile) ) {
		wxLogError(_T("Unable to set current profile to '%s'"), currentProfile.c_str());
		return false;
	}

	ProMan::isInitialized = true;
	wxLogDebug(_T(" Profile Manager is set up"));
	return true;
}

/** clean up the memory that the manager is using. */
bool ProMan::DeInitialize() {
	if ( ProMan::isInitialized ) {
		ProMan::isInitialized = false;

		delete ProMan::proman;
		ProMan::proman = NULL;
		
		return true;
	} else {
		return false;
	}
}


ProMan* ProMan::GetProfileManager() {
	if ( ProMan::isInitialized ) {
		return ProMan::proman;
	} else {
		return NULL;
	}
}

/** Private constructor.  Just makes instance variables safe.  Call Initialize()
to setup class, then call GetProfileManager() to get a pointer to the instance.
*/
ProMan::ProMan() {
	this->globalProfile = NULL;
	this->hasUnsavedChanges = false;
	this->isAutoSaving = true;
	this->currentProfile = NULL;
}

/** Destructor. */
ProMan::~ProMan() {
	if ( this->globalProfile != NULL ) {
		wxLogInfo(_T("saving global profile before exiting."));
		wxFileName file;
		file.Assign(GET_PROFILE_STORAGEFOLDER(), GLOBAL_INI_FILE_NAME);
		wxFFileOutputStream globalProfileOutput(file.GetFullPath());
		this->globalProfile->Save(globalProfileOutput);
		
		delete this->globalProfile;
	} else {
		wxLogWarning(_T("global profile is null, cannot save it"));
	}

	this->SaveCurrentProfile();

	// don't leak the wxFileConfigs
	ProfileMap::iterator iter = this->profiles.begin();
	while ( iter != this->profiles.end() ) {
		delete iter->second;
		iter++;
	}
}
/** Creates a new profile including the directory for it to go in, the entry
in the profiles map. Returns true if creation was successful. */
bool ProMan::CreateNewProfile(wxString newName) {
	wxFileName profile;
	profile.Assign(
		GET_PROFILE_STORAGEFOLDER(),
		wxString::Format(_T("pro%05d.ini"), this->profiles.size()));

	wxASSERT_MSG( profile.IsOk(), _T("Profile filename is invalid"));

	if ( !wxFileName::DirExists(profile.GetPath())
		&& !wxFileName::Mkdir( profile.GetPath(), wxPATH_MKDIR_FULL) ) {
		wxLogWarning(_T("  Unable to create profile folder: %s"), profile.GetPath().c_str());
		return false;
	}

	wxFFileInputStream configInput(profile.GetFullPath(), _T("w+b"));
	wxFileConfig* config = new wxFileConfig(configInput);
	config->Write(PRO_CFG_MAIN_NAME, newName);
	config->Write(PRO_CFG_MAIN_FILENAME, profile.GetFullName());
	wxFFileOutputStream configOutput(profile.GetFullPath());
	config->Save(configOutput);

	this->profiles[newName] = config;
	return true;
}

// global profile access functions

/** Tests whether the key strName is in the global profile. */
bool ProMan::GlobalExists(wxString& strName) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(_T("attempt to check existence of key %s in null global profile"),
			strName.c_str());
		return false;
	} else {
		return this->globalProfile->Exists(strName);
	}
}

/** Tests whether the key strName is in the global profile */
bool ProMan::GlobalExists(const wxChar* strName) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(_T("attempt to check existence of key %s in null global profile"),
			strName);
		return false;
	} else {
		return this->globalProfile->Exists(strName);
	}
}

/** Reads a bool from the global profile. Returns true on success. */
bool ProMan::GlobalRead(const wxString& key, bool* b) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(
			_T("attempt to read bool for key %s from null global profile"),
			key.c_str());
		return false;
	} else {
		return this->globalProfile->Read(key, b);
	}
}

/** Reads a bool from the global profile,
 using the default value if the key is not present.
 Returns true on success. */
bool ProMan::GlobalRead(const wxString& key, bool* b, bool defaultVal) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(
			_T("attempt to read bool for key %s with default value %s from null global profile"),
			key.c_str(), defaultVal ? _T("true") : _T("false"));
		return false;
	} else {
		return this->globalProfile->Read(key, b, defaultVal);
	}
}

/** Reads a string from the global profile. Returns true on success. */
bool ProMan::GlobalRead(const wxString& key, wxString* str) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(
			_T("attempt to read string for key %s with from null global profile"),
			key.c_str());
		return false;
	} else {
		return this->globalProfile->Read(key, str);
	}
}

/** Reads a string from the global profile,
 using the default value if the key is not present.
Returns true on success. */
bool ProMan::GlobalRead(const wxString& key, wxString* str, const wxString& defaultVal) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(
			_T("attempt to read string for key %s with default value %s from null global profile"),
			key.c_str(), defaultVal.c_str());
		return false;
	} else {
		return this->globalProfile->Read(key, str, defaultVal);
	}
}

/** Reads a long from the global profile,
 using the default value if the key is not present.
 Returns true on success. */
bool ProMan::GlobalRead(const wxString& key, long* l, long defaultVal) const {
	if (this->globalProfile == NULL) {
		wxLogWarning(
			_T("attempt to read long for key %s with default value %d from null global profile"),
			key.c_str(), defaultVal);
		return false;
	} else {
		return this->globalProfile->Read(key, l, defaultVal);
	}
}

/** Writes a string for the given key to the global profile.
 Returns true on success. */
bool ProMan::GlobalWrite(const wxString& key, const wxString& value) {
	if (this->globalProfile == NULL) {
		wxLogWarning(_T("attempt to write %s to %s in null global profile"),
			value.c_str(), key.c_str());
		return false;
	} else {
		return this->globalProfile->Write(key, value);
	}
}

/** Writes a long for the given key to the global profile.
 Returns true on success. */
bool ProMan::GlobalWrite(const wxString& key, long value) {
	if (this->globalProfile == NULL) {
		wxLogWarning(_T("attempt to write %d to %s in null global profile"),
			value, key.c_str());
		return false;
	} else {
		return this->globalProfile->Write(key, value);
	}
}

/** Writes a bool for the given key to the global profile.
 Returns true on success. */
bool ProMan::GlobalWrite(const wxString& key, bool value) {
	if (this->globalProfile == NULL) {
		wxLogWarning(_T("attempt to write %s to %s in null global profile"),
			value ? _T("true") : _T("false"), key.c_str());
		return false;
	} else {
		return this->globalProfile->Write(key, value);
	}
}

// current profile access functions

/** Reads a bool from the current profile. Returns true on success. */
bool ProMan::ProfileRead(const wxString& key, bool* b) const {
	if (this->currentProfile == NULL) {
		wxLogWarning(
			_T("attempt to read bool for key %s from null current profile"),
			key.c_str());
		return false;
	} else {
		return this->currentProfile->Read(key, b);
	}
}

/** Reads a bool from the current profile,
 using the default value if the key is not present.
 Returns true on success. */
bool ProMan::ProfileRead(const wxString& key, bool* b, bool defaultVal) const {
	if (this->currentProfile == NULL) {
		wxLogWarning(
			_T("attempt to read bool for key %s with default value %s from null current profile"),
			key.c_str(), defaultVal ? _T("true") : _T("false"));
		return false;
	} else {
		return this->currentProfile->Read(key, b, defaultVal);
	}
}

/** Reads a string from the current profile. Returns true on success. */
bool ProMan::ProfileRead(const wxString& key, wxString* str) const {
	if (this->currentProfile == NULL) {
		wxLogWarning(
			_T("attempt to read string for key %s with from null current profile"),
			key.c_str());
		return false;
	} else {
		return this->currentProfile->Read(key, str);
	}
}

/** Reads a string from the current profile,
 using the default value if the key is not present.
 Returns true on success. */
bool ProMan::ProfileRead(const wxString& key, wxString* str, const wxString& defaultVal) const {
	if (this->currentProfile == NULL) {
		wxLogWarning(
			_T("attempt to read string for key %s with default value %s from null current profile"),
			key.c_str(), defaultVal.c_str());
		return false;
	} else {
		return this->currentProfile->Read(key, str, defaultVal);
	}
}

/** Reads a long from the current profile,
 using the default value if the key is not present.
 Returns true on success. */
bool ProMan::ProfileRead(const wxString& key, long* l, long defaultVal) const {
	if (this->currentProfile == NULL) {
		wxLogWarning(
			_T("attempt to read long for key %s with default value %d from null current profile"),
			key.c_str(), defaultVal);
		return false;
	} else {
		return this->currentProfile->Read(key, l, defaultVal);
	}
}

/** Writes a string for the given key to the current profile.
 Returns true on success. */
bool ProMan::ProfileWrite(const wxString& key, const wxString& value) {
	if (this->currentProfile == NULL) {
		wxLogWarning(_T("attempt to write %s to %s in null current profile"),
			value.c_str(), key.c_str());
		return false;
	} else {
		if (!this->currentProfile->Exists(key)) {
			wxLogDebug(_T("adding entry %s with value %s to current profile"),
				key.c_str(), value.c_str());
			this->SetHasUnsavedChanges();
		} else {
			wxString oldValue;
			if (this->currentProfile->Read(key, &oldValue) && (value != oldValue)) {
				wxLogDebug(_T("replacing old value %s with value %s for current profile entry %s"),
					oldValue.c_str(), value.c_str(), key.c_str());
				this->SetHasUnsavedChanges();
			}
		}
		return this->currentProfile->Write(key, value);
	}
}

/** Writes a long for the given key to the current profile.
 Returns true on success. */
bool ProMan::ProfileWrite(const wxString& key, long value) {
	if (this->currentProfile == NULL) {
		wxLogWarning(_T("attempt to write %d to %s in null current profile"),
			value, key.c_str());
		return false;
	} else {
		if (!this->currentProfile->Exists(key)) {
			wxLogDebug(_T("adding entry %s with value %d to current profile"),
				key.c_str(), value);
			this->SetHasUnsavedChanges();
		} else {
			long oldValue;
			if (this->currentProfile->Read(key, &oldValue) && (value != oldValue)) {
				wxLogDebug(_T("replacing old value %d with value %d for current profile entry %s"),
					oldValue, value, key.c_str());
				this->SetHasUnsavedChanges();
			}
		}
		return this->currentProfile->Write(key, value);
	}
}

/** Writes a bool for the given key to the current profile.
 Returns true on success. */
bool ProMan::ProfileWrite(const wxString& key, bool value) {
	if (this->currentProfile == NULL) {
		wxLogWarning(_T("attempt to write %s to %s in null current profile"),
			value ? _T("true") : _T("false"), key.c_str());
		return false;
	} else {
		if (!this->currentProfile->Exists(key)) {
			wxLogDebug(_T("adding entry %s with value %s to current profile"),
				key.c_str(), value ? _T("true") : _T("false"));
			this->SetHasUnsavedChanges();
		} else {
			bool oldValue;
			if (this->currentProfile->Read(key, &oldValue) && (value != oldValue)) {
				wxLogDebug(_T("replacing old value %s with value %s for current profile entry %s"),
					oldValue ? _T("true") : _T("false"),
					value ? _T("true") : _T("false"),
					key.c_str());
				this->SetHasUnsavedChanges();
			}
		}
		
		return this->currentProfile->Write(key, value);
	}
}

/** Deletes an entry from the current profile,
 deleting the group if the entry was the only one in the group
 and the second parameter is true. */
bool ProMan::ProfileDeleteEntry(const wxString& key, bool bDeleteGroupIfEmpty) {
	if (this->currentProfile == NULL) {
		wxLogWarning(_T("attempt to delete entry %s in null current profile"),
			key.c_str());
		return false;
	} else {
		if (this-currentProfile->Exists(key)) {
			wxLogDebug(_T("deleting key %s in profile"),
				key.c_str());
			this->SetHasUnsavedChanges();
		}
		return this->currentProfile->DeleteEntry(key, bDeleteGroupIfEmpty);
	}
}

/** Returns true if the named profile exists, false otherwise. */
bool ProMan::DoesProfileExist(wxString name) {
	/* Item exists if the returned value from find() does not equal 
	the value of .end().  As per the HashMap docs. */
	return (this->profiles.find(name) != this->profiles.end());
}

/** Returns an wxArrayString of all of the profile names. */
wxArrayString ProMan::GetAllProfileNames() {
	wxArrayString out(this->profiles.size());

	ProfileMap::iterator iter = this->profiles.begin();
	do {
		out.Add(iter->first);
		iter++;
	} while (iter != this->profiles.end());

	return out;
}

/** Save the current profile to disk. Does not affect Global or anyother profile.
We assume all profiles other than the current app profile does not have any 
unsaved changes. So we only check the current profile. */
void ProMan::SaveCurrentProfile() {
	wxConfigBase* configbase = wxFileConfig::Get(false);
	if ( configbase == NULL ) {
		wxLogWarning(_T("There is no global file config."));
		return;
	}
	wxFileConfig* config = dynamic_cast<wxFileConfig*>(configbase);
	if ( config != NULL ) {
		if ( this->isAutoSaving ) {
			wxString profileFilename;
			if ( !config->Read(PRO_CFG_MAIN_FILENAME, &profileFilename) ) {
				wxLogWarning(_T("Current profile does not have a file name, and I am unable to auto save."));
			} else {
				wxFileName file;
				file.Assign(GET_PROFILE_STORAGEFOLDER(), profileFilename);
				wxASSERT( file.IsOk() );
				wxFFileOutputStream configOutput(file.GetFullPath());
				config->Save(configOutput);
				this->ResetHasUnsavedChanges();
				wxLogDebug(_T("Current config saved (%s)."), file.GetFullPath().c_str());
			}
		} else {
			wxLogWarning(_T("Current Profile Manager is being destroyed without saving changes."));
		}
	} else {
		wxLogWarning(_T("Configbase is not a wxFileConfig."));
	}
}

wxString ProMan::GetCurrentName() {
	return this->currentProfileName;
}

bool ProMan::SwitchTo(wxString name) {
	if ( this->profiles.find(name) == this->profiles.end() ) {
		return false;
	} else {
		this->currentProfileName = name;
		this->currentProfile = this->profiles.find(name)->second;
		wxFileConfig::Set(this->currentProfile);
		this->globalProfile->Write(GBL_CFG_MAIN_LASTPROFILE, name);
		this->ResetHasUnsavedChanges();
		this->GenerateCurrentProfileChangedEvent();
		return true;
	}
}

bool ProMan::CloneProfile(wxString originalName, wxString copyName) {
	wxLogDebug(_T("Cloning original profile (%s) to %s"), originalName.c_str(), copyName.c_str());
	if ( !this->DoesProfileExist(originalName) ) {
		wxLogWarning(_("Original Profile '%s' does not exist!"), originalName.c_str());
		return false;
	}
	if ( this->DoesProfileExist(copyName) ) {
		wxLogWarning(_("Target profile '%s' already exists!"), copyName.c_str());
		return false;
	}
	if ( !this->CreateNewProfile(copyName) ) {
		return false;
	}

	wxFileConfig* config = this->profiles[copyName];
	wxCHECK_MSG( config != NULL, false, _T("Create returned true but did not create profile"));

	wxString str;
	long cookie;
	bool cont = config->GetFirstEntry(str, cookie);
	while ( cont ) {
		wxLogDebug(_T("  Got: %s"), str.c_str());

		cont = config->GetNextEntry(str, cookie);
	}
	this->GenerateChangeEvent();
	return true;
}

bool ProMan::DeleteProfile(wxString name) {
	wxLogDebug(_T("Deleting profile: %s"), name.c_str());
	if ( name == ProMan::DEFAULT_PROFILE_NAME ) {
		wxLogWarning(_("Cannot delete 'Default' profile."));
		return false;
	}
	if ( name == this->currentProfileName ) {
		wxLogInfo(_T("Deleting current profile. Switching current to 'Default' profile"));
		this->SwitchTo(ProMan::DEFAULT_PROFILE_NAME);
	}
	if ( this->DoesProfileExist(name) ) {
		wxLogDebug(_T(" Profile exists"));
		wxFileConfig* config = this->profiles[name];

		wxString filename;
		if ( !config->Read(PRO_CFG_MAIN_FILENAME, &filename) ) {
			wxLogWarning(_T("Unable to get filename to delete %s"), name.c_str());
			return false;
		}

		wxFileName file;
		file.Assign(GET_PROFILE_STORAGEFOLDER(), filename);

		if ( file.FileExists() ) {
			wxLogDebug(_T(" Backing file exists"));
			if ( wxRemoveFile(file.GetFullPath()) ) {
				this->profiles.erase(this->profiles.find(name));
				delete config;
				
				wxLogMessage(_("Profile '%s' deleted."), name.c_str());
				this->GenerateChangeEvent();
				return true;
			} else {
				wxLogWarning(_("Unable to delete file for profile '%s'"), name.c_str());
			}
		} else {
			wxLogWarning(_("Backing file (%s) for profile '%s' does not exist"), file.GetFullPath().c_str(), name.c_str());
		}
	} else {
		wxLogWarning(_("Profile %s does not exist. Cannot delete."), name.c_str());
	}
	return false;
}

/** Applies the current profile to the registry where 
 Freespace 2 can read it. */
ProMan::RegistryCodes ProMan::PushCurrentProfile() {
	if (this->currentProfile == NULL) {
		wxLogError(_T("PushCurrentProfile: attempt to push null current profile"));
		return ProMan::UnknownError;
	} else {
		return ProMan::PushProfile(this->currentProfile);
	}
}

/** Applies the passed wxFileConfig profile to the registry where 
Freespace 2 can read it. */
ProMan::RegistryCodes ProMan::PushProfile(wxFileConfig *cfg) {
	wxCHECK_MSG(cfg != NULL, ProMan::UnknownError, _T("ProMan::PushProfile given null wxFileConfig!"));
#if IS_WIN32
	// check if binary supports configfile
	return RegistryPushProfile(cfg);
#elif IS_LINUX || IS_APPLE
	return FilePushProfile(cfg);
#else
#error "One of IS_WIN32, IS_LINUX, IS_APPLE must evaluate to true"
#endif
}

/** Takes the settings in the registry and puts them into the wxFileConfig */
ProMan::RegistryCodes ProMan::PullProfile(wxFileConfig *cfg) {
	wxCHECK_MSG(cfg != NULL, ProMan::UnknownError, _T("ProMan::PullProfile given null wxFileConfig!"));
#if IS_WIN32
	// check if binary supports configfile
	return RegistryPullProfile(cfg);
#elif IS_LINUX || IS_APPLE
	return FilePullProfile(cfg);
#else
#error "One of IS_WIN32, IS_LINUX, IS_APPLE must evaluate to true"
#endif
}