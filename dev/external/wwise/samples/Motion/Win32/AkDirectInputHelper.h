//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////


#pragma once

#include <stdio.h>      // standard input/output
#include <vector>       // stl vector header

#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <wbemidl.h>
#include <oleauto.h>


#include <AK/SoundEngine/Common/AkTypes.h>

namespace AkDirectInputDevice
{

struct DirectInputController
{
	GUID m_ControllerGUID;
	LPDIRECTINPUTDEVICE8 m_pController;
	wchar_t m_tszProductName[MAX_PATH];
	bool m_bIsActive;
	bool m_bIsInitialised;
};

class AkControllers
{
public:
	std::vector<DirectInputController>  m_DirectInputDevices;	// array of direct input device
	AkInt32 m_nXInputControllerCount;

	AkControllers();
	~AkControllers();
	void Reset();
};

/// Get the controllers list.
/// \sa
/// - \ref InitControllers UpdateControllers
const AkControllers& GetControllers();

/// Initialise direct input interface and update the controllers list.
/// \return true if direct input device found.
/// \sa
/// - \ref UpdateControllers
bool InitControllers(HWND in_hwnd);

/// Update the controllers list with connected device.
/// \return true if direct input device found.
/// \sa
bool UpdateControllers();

/// Get the first connected controller from the list,
/// \return IDirectInputDevice8* if found and NULL otherwise
/// \sa
/// - \ref InitControllers UpdateControllers 
IDirectInputDevice8 *GetFirstDirectInputController();

} //namespeace AkDirectInputDevice
