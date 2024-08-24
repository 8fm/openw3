//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// SFTest.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "SFTest.h"
#include "SFTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSFTestApp

BEGIN_MESSAGE_MAP(CSFTestApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CSFTestApp construction

CSFTestApp::CSFTestApp()
{
}


// The one and only CSFTestApp object

CSFTestApp theApp;


// CSFTestApp initialization

BOOL CSFTestApp::InitInstance()
{
	// InitCommonControls() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	InitCommonControls();

	::OleInitialize( NULL );

	CWinApp::InitInstance();

	CSFTestDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

int CSFTestApp::ExitInstance()
{
	::OleUninitialize();

	return __super::ExitInstance();
}