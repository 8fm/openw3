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
#include <AK/Tools/Common/AkMonitorError.h>
#include "AkRumbleControllerDirectInput.h"

AkRumbleControllerDirectInput::AkRumbleControllerDirectInput( IDirectInputDevice8 * in_pController)
	:m_pEffect(NULL)
	,m_pController(NULL)
{
	HRESULT hr;

	m_pController = in_pController;

    if( !m_pController || CreateEffect() != DI_OK )
    {
		AK::Monitor::PostString(AKTEXT("Unable to initialise rumble on a direct input device."), AK::Monitor::ErrorLevel_Error);
    }	
	else if (m_pController)
	{
		hr = m_pController->Acquire();
		if (hr != DI_OK)
		{
			AK::Monitor::PostString(AKTEXT("Unable to acquire a direct input controler."), AK::Monitor::ErrorLevel_Error);
		}
	}
}

HRESULT AkRumbleControllerDirectInput::CreateEffect()
{
	if (!m_pController)
		return DIERR_INVALIDPARAM;

	// This application needs only one effect: Applying raw forces.
    DWORD           rgdwAxes[2]     = { DIJOFS_X, DIJOFS_Y };
    LONG            rglDirection[2] = { 0, 0 };
    DICONSTANTFORCE cf              = { 0 };

    DIEFFECT eff;
    ZeroMemory( &eff, sizeof(eff) );
    eff.dwSize                  = sizeof(DIEFFECT);
    eff.dwFlags                 = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration              = INFINITE;
    eff.dwSamplePeriod          = 0;
    eff.dwGain                  = DI_FFNOMINALMAX;
    eff.dwTriggerButton         = DIEB_NOTRIGGER;
    eff.dwTriggerRepeatInterval = 0;
    eff.cAxes                   = 2;
    eff.rgdwAxes                = rgdwAxes;
    eff.rglDirection            = rglDirection;
    eff.lpEnvelope              = 0;
    eff.cbTypeSpecificParams    = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams   = &cf;
    eff.dwStartDelay            = 0;

    // Create the prepared effect
	return m_pController->CreateEffect( GUID_ConstantForce, &eff, &m_pEffect, NULL );
}

AkRumbleControllerDirectInput::~AkRumbleControllerDirectInput()
{
	// Unacquire the device one last time just in case 
    // the app tried to exit while the device is still acquired.
	if (m_pController)
	{
		m_pController->Unacquire();
		m_pController = NULL;
		//It is not our responsabilities to RELEASE the device.
		//If it were, we had to do it after releasing the effect.
	}

	if (m_pEffect)
	{
		m_pEffect->Release();
		m_pEffect = NULL;
	}
}

#include <Math.h>
AKRESULT AkRumbleControllerDirectInput::SetRumble(AkReal32 in_fLarge, AkReal32 in_fSmall)
{
	// Won't be initialized when controller doesn't suport rumble
	if(!m_pEffect)
	{
		return AK_Fail;
	}

	AkInt32 rglDirection[2] = { 0, 0 };
	DICONSTANTFORCE cf;

	    rglDirection[0] = static_cast<AkInt32>(in_fSmall * DI_FFNOMINALMAX);
        rglDirection[1] = static_cast<AkInt32>(in_fLarge * DI_FFNOMINALMAX);
        cf.lMagnitude = (DWORD)sqrt( (double)rglDirection[0] * (double)rglDirection[0] +
                                     (double)rglDirection[1] * (double)rglDirection[1] );

	DIEFFECT eff;
    ZeroMemory( &eff, sizeof(eff) );
    eff.dwSize                = sizeof(DIEFFECT);
    eff.dwFlags               = DIEFF_CARTESIAN ;
    eff.cAxes                 = 2;
    eff.rglDirection          = rglDirection;
    eff.lpEnvelope            = 0;
    eff.cbTypeSpecificParams  = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &cf;
    eff.dwStartDelay            = 0;

	// Now set the new parameters and start the effect immediately.
	return SUCCEEDED(m_pEffect->SetParameters( &eff, DIEP_DIRECTION | DIEP_TYPESPECIFICPARAMS | DIEP_START ))? AK_Success : AK_Fail;
		  
}

bool AkRumbleControllerDirectInput::IsActive()
{
	m_pController->Poll();
	bool bIsActive(false);
	DIDEVCAPS deviceCap;
	deviceCap.dwSize = sizeof(DIDEVCAPS);
	HRESULT hr = m_pController->GetCapabilities( &deviceCap );
	if ((hr == DI_OK) && 
		(deviceCap.dwFlags & DIDC_ATTACHED) )
	{
		bIsActive = true;
	}
	return bIsActive;
}

AKRESULT AkRumbleControllerDirectInput::Term( AK::IAkPluginMemAlloc * in_pAllocator )
{
	AK_PLUGIN_DELETE(in_pAllocator, this);
	return AK_Success;
}
