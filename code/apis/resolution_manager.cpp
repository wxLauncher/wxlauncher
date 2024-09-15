/*
Copyright (C) 2016 wxLauncher Team

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

#include "generated/configure_launcher.h"

#include "SDL.h"

#include "apis/resolution_manager.hpp"
#include "global/ProfileKeys.h"

/** \namespace ResolutionMan
The ResolutionMan namespace contains the code that gets the
available resolutions for the BasicSettingsTab.
*/

using namespace ResolutionMan;

Resolution::Resolution(const int width, const int height, const bool isHeader) {
	wxASSERT_MSG(width > 0, wxString::Format(_T("Resolution: provided width %d is invalid"), width));
	wxASSERT_MSG(height > 0, wxString::Format(_T("Resolution: provided height %d is invalid"), height));
	this->width = width;
	this->height = height;
	this->isHeader = isHeader;
	resString = isHeader ? wxString::Format(_T("--- %d:%d ---"), width, height)
		: wxString::Format(CFG_RES_FORMAT_STRING, width, height);
}

Resolution* Resolution::New() const {
	return new Resolution(this->GetWidth(), this->GetHeight(),
		this->IsHeader());
}

// sort by aspect ratio, then by size, in descending order
int CompareResolutions(Resolution **resp1, Resolution **resp2) {

	wxASSERT_MSG(resp1 != NULL, _T("CompareResolutions: provided resp1 is NULL"));
	wxASSERT_MSG((*resp1) != NULL, _T("CompareResolutions: resp1 points to NULL"));
	wxASSERT_MSG(resp2 != NULL, _T("CompareResolutions: provided resp2 is NULL"));
	wxASSERT_MSG((*resp2) != NULL, _T("CompareResolutions: resp2 points to NULL"));

	// compare the two width/height ratios by cross-multiplying and comparing the results
	const int width1 = (*resp1)->GetWidth();
	const int height1 = (*resp1)->GetHeight();
	const int width2 = (*resp2)->GetWidth();
	const int height2 = (*resp2)->GetHeight();

	wxASSERT_MSG(width1 > 0, wxString::Format(_T("CompareResolutions: width1 %d is invalid"), width1));
	wxASSERT_MSG(height1 > 0, wxString::Format(_T("CompareResolutions: height1 %d is invalid"), height1));
	wxASSERT_MSG(width2 > 0, wxString::Format(_T("CompareResolutions: width2 %d is invalid"), width2));
	wxASSERT_MSG(height2 > 0, wxString::Format(_T("CompareResolutions: height2 %d is invalid"), height2));

	const int value1 = width1 * height2;
	const int value2 = width2 * height1;
	int result;
	// first compare aspect ratio
	if (value1 < value2) {
		result = -1;
	}
	else if (value1 > value2) {
		result = 1;
	}
	else { // then compare size if aspect ratios are equal
		if (width1 < width2) {
			result = -1;
		}
		else if (width1 > width2) {
			result = 1;
		}
		else {
			result = 0;
		}
	}

	return (-1) * result; // for reverse comparison (descending order)
}

// brought to you by http://en.wikipedia.org/wiki/Euclidean_algorithm#Implementations
int ComputeGCD(int a, int b) {
	wxASSERT_MSG(a > 0, wxString::Format(_T("ComputeGCD(a=%d, b=%d): a must be positive"), a, b));
	wxASSERT_MSG(b > 0, wxString::Format(_T("ComputeGCD(a=%d, b=%d): b must be positive"), a, b));

	//	part of the original algorithm, but irrelevant, since a and b are positive
	//	if (a == 0) {
	//		return b;
	//	}
	while (b != 0) {
		if (a > b) {
			a = a - b;
		}
		else {
			b = b - a;
		}
	}
	return a;
}

// Assumed resolutions has been sorted using CompareResolutions
void AddHeaders(ResolutionArray &resolutions) {
	if (resolutions.IsEmpty()) {
		wxLogWarning(_T("AddHeaders: provided ResolutionArray is empty"));
		return;
	}

	ResolutionArray headers;
	wxArrayInt headerIndexes;

	int lastAspectWidth = -1;
	int lastAspectHeight = -1;
	int width;
	int height;
	int gcd;

	// find aspect ratios and determine where the aspect ratio headers should be inserted
	for (int i = 0, n = resolutions.GetCount(); i < n; ++i) {
		width = resolutions.Item(i)->GetWidth();
		height = resolutions.Item(i)->GetHeight();
		gcd = ComputeGCD(width, height);
		width /= gcd;
		height /= gcd;

		// special exception: 8:5 should be 16:10
		if ((width == 8) && (height == 5)) {
			width *= 2;
			height *= 2;
		}

		if ((width != lastAspectWidth) || (height != lastAspectHeight)) {
			wxLogDebug(_T(" found aspect ratio %d:%d"), width, height);
			headers.Add(new Resolution(width, height, true));
			headerIndexes.Add(i);
			lastAspectWidth = width;
			lastAspectHeight = height;
		}
	}

	// now insert the headers into resolutions
	// must insert in reverse to avoid messing up insertion indexes
	for (int i = headerIndexes.GetCount() - 1; i >= 0; --i) {
		resolutions.Insert(headers.Item(i), headerIndexes.Item(i));
	}
}


#ifdef WIN32
void EnumerateGraphicsModes_win32(
	ResolutionArray &out_modes,
	const DWORD minHorizontalRes,
	const DWORD minVerticalRes)
{
	DEVMODE deviceMode;
	DWORD modeCounter = 0;
	BOOL result;

	wxLogDebug(_T("Enumerating graphics modes with Win32"));

	do {
		memset(&deviceMode, 0, sizeof(DEVMODE));
		deviceMode.dmSize = sizeof(DEVMODE);

		result = EnumDisplaySettings(NULL, modeCounter, &deviceMode);

		if (result == TRUE) {
			wxLogDebug(_T(" %dx%d %d bit %d hertz (%d)"),
				deviceMode.dmPelsWidth,
				deviceMode.dmPelsHeight,
				deviceMode.dmBitsPerPel,
				deviceMode.dmDisplayFrequency,
				deviceMode.dmDisplayFlags);

			Resolution candidate(
				deviceMode.dmPelsWidth, deviceMode.dmPelsHeight, false);

			// check to see if the resolution has already been added
			// since the current out_modes list is unsorted we need to
			// iterate over all of the existing ones
			bool resExists = false;
			for (auto it = out_modes.begin(), end = out_modes.end();
				it != end; ++it)
			{
				Resolution* p = (Resolution*)(*it);
				if ((p)->IsSameResolution(candidate)) {
					resExists = true;
					break; // no need to keep looking
				}
			}

			if (!resExists &&
				(deviceMode.dmPelsWidth >= minHorizontalRes) &&
				(deviceMode.dmPelsHeight >= minVerticalRes))
			{
				out_modes.Add(candidate.New());
			}
		}
		modeCounter++;
	} while (result == TRUE);
}
#endif

void EnumerateGraphicsModes_sdl(
	ResolutionArray &out_modes,
	const int minHorizontalRes,
	const int minVerticalRes)
{
	wxLogDebug(_T("Enumerating graphics modes with SDL"));
	// FSO currently only supports the primary display
	const int DISPLAY_INDEX = 0;

#if IS_APPLE || IS_WIN32
	// It seems that linux cannot handle having the video subsystem inited
	// too late
	if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0) {
		wxLogFatalError(wxT_2("SDL video subsystem failed to initialize!"));
	}
#endif

	if (SDL_GetNumVideoDisplays() > 0) {
		int numDisplayModes = SDL_GetNumDisplayModes(DISPLAY_INDEX);

		SDL_DisplayMode mode;

		for (int i = 0; i < numDisplayModes; ++i) {
			if (SDL_GetDisplayMode(DISPLAY_INDEX, i, &mode) != 0) {
				wxLogWarning(_T("SDL_GetDisplayMode failed: %s"),
					SDL_GetError());
				continue;
			}

			if ((mode.w >= minHorizontalRes) && (mode.h >= minVerticalRes)) {
				out_modes.Add(new Resolution(mode.w, mode.h, false));
			}
		}
	}
	else {
		wxLogWarning(_T("SDL reported no displays!"));
	}

#if IS_APPLE || IS_WIN32
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
#endif
}

/** Get available graphics modes for API and return them sorted */
void ResolutionMan::EnumerateGraphicsModes(
	ApiType type, ResolutionArray& out_modes,
	const long minHorizontalRes, const long minVerticalRes)
{
#if IS_WIN32
	if (type == ResolutionMan::API_WIN32) {
		EnumerateGraphicsModes_win32(out_modes,
			static_cast<DWORD>(minHorizontalRes),
			static_cast<DWORD>(minVerticalRes));
	}
#endif
#if HAS_SDL
	if (type == ResolutionMan::API_SDL) {
		EnumerateGraphicsModes_sdl(out_modes,
			static_cast<int>(minHorizontalRes),
			static_cast<int>(minVerticalRes));
	}
#endif

	// then arrange the resolutions for drop down box and insert them
	out_modes.Sort(CompareResolutions);

	AddHeaders(out_modes);
}