//////////////////////////////////////////////////////////////////////
//
// AudioInput.cpp
//
// AudioInput Wwise plugin: Defines the initialization routines for the DLL.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AudioInput.h"
#include <AK/Wwise/Utilities.h>
#include <AK/Plugin/AkAudioInputSourceFactory.h>
#include "AudioInputPlugin.h"
#include "SoundInputMgr.h"

//
//	Note!
//
//		If this DLL is dynamically linked against the MFC
//		DLLs, any functions exported from this DLL which
//		call into MFC must have the AFX_MANAGE_STATE macro
//		added at the very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

#include <math.h>

// CAudioInputApp
BEGIN_MESSAGE_MAP(CAudioInputApp, CWinApp)
END_MESSAGE_MAP()

static const AkReal32 TWOPI			= 6.283185307179586476925286766559f;
static const AkReal32 FREQUENCY		= 55.0f;	// Frequency (A)
static const AkReal32 SAMPLERATE	= 48000.0f;	// NOTE: This might need to be located somewhere..

// CAudioInputApp construction
CAudioInputApp::CAudioInputApp()
{
}


// The one and only CAudioInputApp object
CAudioInputApp theApp;

// CAudioInputApp initialization
BOOL CAudioInputApp::InitInstance()
{
	CWinApp::InitInstance();

	SoundInputMgr::Instance().Initialize();
	
	return TRUE;
}

// CAudioInputApp exit code
int CAudioInputApp::ExitInstance()
{
	SoundInputMgr::Instance().Term();
	return 0;
}

/////////////// DLL exports ///////////////////

// Plugin creation
AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
	if ( in_usCompanyID == AudioInputPlugin::CompanyID && in_usPluginID == AudioInputPlugin::PluginID )
		return new AudioInputPlugin;

	return NULL;
}

// Sound Engine callbacks
bool __stdcall AkGetSoundEngineCallbacks( unsigned short in_usCompanyID, unsigned short in_usPluginID, AkCreatePluginCallback & out_funcEffect, AkCreateParamCallback & out_funcParam )
{
	if ( in_usCompanyID == AudioInputPlugin::CompanyID && in_usPluginID == AudioInputPlugin::PluginID )
	{
		out_funcEffect = CreateAudioInputSource;
		out_funcParam = CreateAudioInputSourceParams;
		return true;
	}

	return false;
}

/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;