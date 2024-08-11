/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../redSystem/types.h"

///////////////////////////////////////////////////////////////////
// Use this interface as a bridge to the 'platform' viewport
// (the desktop on Windows, ApplicationView on durango, etc, etc)
class IPlatformViewport
{
public:
	virtual ~IPlatformViewport(){}
	virtual Red::System::Bool PumpMessages() = 0;
};
