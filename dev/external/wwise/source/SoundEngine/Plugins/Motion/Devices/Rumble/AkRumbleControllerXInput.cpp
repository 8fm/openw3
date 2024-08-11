/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h"
#include "AkRumbleControllerXInput.h"
#include "AkMonitor.h"

typedef DWORD (WINAPI *_XInputGetCapabilities)
(
 DWORD                dwUserIndex,   // [in] Index of the gamer associated with the device
 DWORD                dwFlags,       // [in] Input flags that identify the device type
 XINPUT_CAPABILITIES* pCapabilities  // [out] Receives the capabilities
 ) ;

typedef DWORD (WINAPI *_XInputSetState)
(
 DWORD             dwUserIndex,  // [in] Index of the gamer associated with the device
 XINPUT_VIBRATION* pVibration    // [in, out] The vibration information to send to the controller
 );

static _XInputGetCapabilities s_XInputGetCapabilities = NULL;
static _XInputSetState s_XInputSetState = NULL;

#if defined AK_WIN && !defined AK_USE_METRO_API
static HMODULE s_hXInputDLL = NULL;
static int s_cLibRef = 0;
#endif

void InitXInputDLL()
{
#if defined AK_WIN && !defined AK_USE_METRO_API
	if (s_hXInputDLL)
	{
		s_cLibRef++;
		return;
	}

	//Check if there is one of the XInput libs loaded already.  We support 1.4 and 1.3.  Prefer the most recent.
	//Avoid loading 2 versions of the same dll.
	//Note: GetModuleHandle does NOT increase the internal dll reference count.	
	if (::GetModuleHandle(L"XInput1_4.dll") != NULL)
		s_hXInputDLL = ::LoadLibrary(L"XInput1_4.dll");			

	if (s_hXInputDLL == NULL)
	{		
		if (::GetModuleHandle(L"XInput1_3.dll") != NULL)
			s_hXInputDLL = ::LoadLibrary(L"XInput1_3.dll");
	}

	if (s_hXInputDLL == NULL)
		s_hXInputDLL = ::LoadLibrary(L"XInput1_4.dll");

	if (s_hXInputDLL == NULL)
		s_hXInputDLL = ::LoadLibrary(L"XInput1_3.dll");

	if (s_hXInputDLL == NULL)
		return;

	s_cLibRef++;

	s_XInputGetCapabilities = (_XInputGetCapabilities) ::GetProcAddress(s_hXInputDLL, "XInputGetCapabilities");
	s_XInputSetState = (_XInputSetState) ::GetProcAddress(s_hXInputDLL, "XInputSetState");
#else
	// Static linking
	s_XInputGetCapabilities = XInputGetCapabilities;
	s_XInputSetState = XInputSetState;
#endif
}

AkRumbleControllerXInput::AkRumbleControllerXInput(AkUInt8 in_iPlayer)
	:m_iPlayer(in_iPlayer)
{
	InitXInputDLL();
}



AKRESULT AkRumbleControllerXInput::SetRumble(AkReal32 in_fLarge, AkReal32 in_fSmall)
{	
	AKASSERT(s_XInputSetState != NULL);	//Should not be called if the DLL could not be loaded (IsActive will be false)

	//For the XBox controller, the response is not linear.  It goes from 0 to -20 dB (approximately) following
	//a kind of inverse exponential.  We will linearize the response.  To simplify the maths, we will simply
	//use 2 linear interpolation with different slopes.  The corner of the 2 slopes was determined experimentally.
	const AkReal32 fRange = 65535.f;
	const AkReal32 fLimit = 0.08f * fRange;	//Below 10% the motor doesn't turn.
	const AkReal32 fCornerX = 0.4f;
	const AkReal32 fCornerY = 0.15f * fRange;
	const AkReal32 fFirstSlope = (fRange-fCornerY - 2*fLimit)/(1-fCornerX);
	const AkReal32 fFirstOffset = fRange-fFirstSlope;
	const AkReal32 fSecondSlope = (fCornerY-fLimit)/fCornerX;

	XINPUT_VIBRATION oCurrent;		//Current motor speeds
	
	if (in_fLarge > fCornerX)
		oCurrent.wLeftMotorSpeed = (AkUInt16)(in_fLarge * fFirstSlope + fFirstOffset);
	else if (in_fLarge > 0.05 )
		oCurrent.wLeftMotorSpeed = (AkUInt16)(in_fLarge * fSecondSlope + fLimit);
	else
		oCurrent.wLeftMotorSpeed = 0;

	//These values were obtained through experimentation.  Below 5%, there is nothing
	//happening.  So the interpolation starts at that value.  
	//We want the full scale applied to the real range of values.
	if (in_fSmall > 0.05)
		oCurrent.wRightMotorSpeed = AkUInt16(in_fSmall * (fRange -8192.0f) + 8192.0f);
	else
		oCurrent.wRightMotorSpeed = 0;

	DWORD winError = s_XInputSetState(m_iPlayer, &oCurrent);
	
	AKRESULT result;
	
	switch(winError)
	{
	case NOERROR:
		result = AK_Success;
		break;
	case ERROR_BUSY:
		result = AK_Busy;
		break;
	default:
		result = AK_Fail;
		break;
	}

	return result;
}

bool AkRumbleControllerXInput::IsActive()
{
#if defined AK_WIN && !defined AK_USE_METRO_API
	if (s_XInputGetCapabilities == NULL)
		return false;
#endif

	bool bIsActive(false);
	DWORD err;
	XINPUT_CAPABILITIES oCap;
	err = s_XInputGetCapabilities(m_iPlayer, XINPUT_FLAG_GAMEPAD, &oCap);
	if (err == ERROR_SUCCESS && (oCap.Vibration.wLeftMotorSpeed != 0 || oCap.Vibration.wRightMotorSpeed != 0))
		bIsActive = true;
	return bIsActive;
}

AKRESULT AkRumbleControllerXInput::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AKRESULT result = SetRumble(0,0);
	size_t uWatchDog = 0;
	for(uWatchDog = 0; uWatchDog < 10000 && result == AK_Busy; uWatchDog++)
	{
		result = SetRumble(0,0);
	}

	AK_PLUGIN_DELETE( in_pAllocator, this);

#if defined AK_WIN && !defined AK_USE_METRO_API
	if (s_cLibRef > 0)
	{
		s_cLibRef--;
		if (s_cLibRef == 0)
		{
			FreeLibrary(s_hXInputDLL);
			s_hXInputDLL = NULL;
			s_XInputGetCapabilities = NULL;
			s_XInputSetState = NULL;
		}
	}
#endif
	return AK_Success;
}
