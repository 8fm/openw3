//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkDelay.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "AkDelay.h"
#include <AK/Plugin/AkDelayFXFactory.h>
#include "DelayPlugin.h"
#include <AK/Wwise/Utilities.h>
#include <AK/Tools/Common/AkAssert.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

// CAkDelayApp
BEGIN_MESSAGE_MAP(CAkDelayApp, CWinApp)
END_MESSAGE_MAP()


// CAkDelayApp construction

CAkDelayApp::CAkDelayApp()
{

}

// The one and only CAkDelayApp object
CAkDelayApp theApp;

// CAkDelayApp initialization
BOOL CAkDelayApp::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
	AKASSERT( in_usCompanyID == DelayPlugin::CompanyID && in_usPluginID == DelayPlugin::PluginID );

	return new DelayPlugin;
}

bool __stdcall AkGetSoundEngineCallbacks( unsigned short in_usCompanyID, unsigned short in_usPluginID, AkCreatePluginCallback & out_funcEffect, AkCreateParamCallback & out_funcParam )
{
	AKASSERT( in_usCompanyID == DelayPlugin::CompanyID && in_usPluginID == DelayPlugin::PluginID );

	out_funcEffect = CreateDelayFX;
	out_funcParam = CreateDelayFXParams;
	return true;
}

/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;
