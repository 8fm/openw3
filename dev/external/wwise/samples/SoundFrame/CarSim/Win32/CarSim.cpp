//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// CarSim.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "CarSim.h"
#include "CarSimDlg.h"
#include "CarSimEngine.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CCarSimApp

BEGIN_MESSAGE_MAP(CCarSimApp, CWinApp)
END_MESSAGE_MAP()


// CCarSimApp construction

CCarSimApp::CCarSimApp()
{
}

CCarSimApp theApp;


// CCarSimApp initialization

BOOL CCarSimApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	// Process the command line params
    CCommandLineInfo cmnds;
    ParseCommandLine( cmnds );

	// Init the online help
	EnableHtmlHelp() ;

	TCHAR szPath[ MAX_PATH ];
	::GetModuleFileName( NULL, szPath, MAX_PATH ) ;

	PathRemoveFileSpec( szPath );
	PathAppend( szPath, _T("WwiseCarSimHelp.chm") );

	m_pszHelpFilePath = _tcsdup( szPath ) ;

	// Init the Car Sim itself
	CarSimEngine m_carSimEngine;
	CCarSimDlg dlg( &m_carSimEngine );
	m_pMainWnd = &dlg;

	m_carSimEngine.Init( &dlg );

	if( cmnds.m_nShellCommand == CCommandLineInfo::FileOpen )
	{
		// If the user specify a simulation file we load it now.
		if( ! m_carSimEngine.GetSimulationSerializer().Load( cmnds.m_strFileName ) )
		{
			CString csError;
			csError.Format( IDS_OPEN_ERROR, cmnds.m_strFileName );
			MessageBox( NULL, csError, NULL, MB_ICONERROR | MB_OK );
		}
	}

	INT_PTR nResponse = dlg.DoModal();

	m_carSimEngine.Term();

	return FALSE;
}
