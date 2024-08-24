//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// SourceControlSample.h : main header file for the SourceControlSample DLL
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols


// CSourceControlSampleApp
// See SourceControlSample.cpp for the implementation of this class
//

class CSourceControlSampleApp : public CWinApp
{
public:
	CSourceControlSampleApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
