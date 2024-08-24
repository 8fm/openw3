//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// SFTest.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CSFTestApp:
// See SFTest.cpp for the implementation of this class
//

class CSFTestApp : public CWinApp
{
public:
	CSFTestApp();

// Overrides
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance(); // return app exit code

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CSFTestApp theApp;