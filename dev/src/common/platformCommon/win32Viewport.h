/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/platformViewport.h"

// Keeping viewport implementation seperate for the game and editor for now

class CWin32GameViewport : public IPlatformViewport
{
public:
	CWin32GameViewport()	{ }
	virtual ~CWin32GameViewport() { }

	Bool PumpMessages();
};

class CWin32EditorViewport : public IPlatformViewport
{
public:
	CWin32EditorViewport()	{ }
	virtual ~CWin32EditorViewport() { }

	Bool PumpMessages();
};