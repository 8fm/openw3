//////////////////////////////////////////////////////////////////////
//
// MP3.cpp
//
// MP3 Wwise plugin: Defines the initialization routines for the DLL.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MP3.h"
#include <AK/Wwise/Utilities.h>
#include <AK/Plugin/AkMP3SourceFactory.h>
#include "MP3Plugin.h"

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

// CMP3App
BEGIN_MESSAGE_MAP(CMP3App, CWinApp)
END_MESSAGE_MAP()

// CMP3App construction
CMP3App::CMP3App()
{
}


// The one and only CMP3App object
CMP3App theApp;


// CMP3App initialization
BOOL CMP3App::InitInstance()
{
	CWinApp::InitInstance();

	return TRUE;
}

// CMP3App exit code
int CMP3App::ExitInstance()
{

	return 0;
}

/////////////// DLL exports ///////////////////

// Plugin creation
AK::Wwise::IPluginBase* __stdcall AkCreatePlugin( unsigned short in_usCompanyID, unsigned short in_usPluginID )
{
	if ( in_usCompanyID == MP3Plugin::CompanyID && in_usPluginID == MP3Plugin::PluginID )
		return new MP3Plugin;

	return NULL;
}

// Sound Engine callbacks
bool __stdcall AkGetSoundEngineCallbacks( unsigned short in_usCompanyID, unsigned short in_usPluginID, AkCreatePluginCallback & out_funcEffect, AkCreateParamCallback & out_funcParam )
{
	if ( in_usCompanyID == MP3Plugin::CompanyID && in_usPluginID == MP3Plugin::PluginID )
	{
		out_funcEffect = CreateMP3Source;
		out_funcParam = CreateMP3SourceParams;
		return true;
	}

	return false;
}

/// Dummy assert hook for Wwise plug-ins using AKASSERT (cassert used by default).
DEFINEDUMMYASSERTHOOK;
