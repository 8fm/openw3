//////////////////////////////////////////////////////////////////////
//
// Sine.h
//
// Sine tone Wwise plugin: Main header file for the Sine DLL.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

// CSineApp
// See Sine.cpp for the implementation of this class
//

class CSineApp : public CWinApp
{
public:
	CSineApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
