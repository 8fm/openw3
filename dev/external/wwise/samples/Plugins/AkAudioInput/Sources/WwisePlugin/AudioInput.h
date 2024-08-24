//////////////////////////////////////////////////////////////////////
//
// AudioInput.h
//
// Audio Input Wwise plugin: Main header file for the Audio Input DLL.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

// CAudioInputApp
// See AudioInput.cpp for the implementation of this class
//

class CAudioInputApp : public CWinApp
{
public:
	CAudioInputApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int  ExitInstance(); 

	DECLARE_MESSAGE_MAP()
};
