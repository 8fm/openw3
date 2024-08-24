//////////////////////////////////////////////////////////////////////
//
// ToneGen.h
//
// Main header file for the ToneGen DLL.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

// CToneGenApp
// See ToneGen.cpp for the implementation of this class
//

class CToneGenApp : public CWinApp
{
public:
	CToneGenApp();

// Overrides
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()

};
