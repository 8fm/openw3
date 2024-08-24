//////////////////////////////////////////////////////////////////////
//
// Sine.cpp
//
// Sine tone Wwise plugin: Defines the initialization routines for the DLL.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Sine.h"
#include <AK/Wwise/Utilities.h>
#include <AK/Plugin/AkSineSourceFactory.h>
#include "SinePlugin.h"

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

// CSineApp
BEGIN_MESSAGE_MAP(CSineApp, CWinApp)
END_MESSAGE_MAP()


// CSineApp construction
CSineApp::CSineApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CSineApp object
CSineApp theApp;


// CSineApp initialization
BOOL CSineApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

/////////////// DLL exports ///////////////////

// Plugin creation
AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
	if ( in_usCompanyID == SinePlugin::CompanyID && in_usPluginID == SinePlugin::PluginID )
		return new SinePlugin;

	return NULL;
}

// Sound Engine callbacks
bool __stdcall AkGetSoundEngineCallbacks( unsigned short in_usCompanyID, unsigned short in_usPluginID, AkCreatePluginCallback & out_funcEffect, AkCreateParamCallback & out_funcParam )
{
	if ( in_usCompanyID == SinePlugin::CompanyID && in_usPluginID == SinePlugin::PluginID )
	{
		out_funcEffect = CreateSineSource;
		out_funcParam = CreateSineSourceParams;
		return true;
	}

	return false;
}

/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;
