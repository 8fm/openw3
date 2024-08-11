/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "timerWindows.h"
#include "types.h"

// This wants to be removed as soon as we have a real solution in place
#ifdef CONTINUOUS_SCREENSHOT_HACK

Red::System::Double Red::System::Timer::GetTimeHackSeconds() const
{
	return TimeHackBaseTime;
}

void Red::System::Timer::EnableGameTimeHack()
{
	if ( !IsEnabledTimeHack )
	{
		TimeHackBaseTime =  GetSeconds();
		IsEnabledTimeHack = true;
	}
}

void Red::System::Timer::DisableGameTimeHack()
{
	if ( IsEnabledTimeHack )
	{
		IsEnabledTimeHack = false;
		TimeHackCorrection = GetSeconds() - TimeHackBaseTime;
	}
}

Red::System::Double Red::System::Timer::NextFrameGameTimeHack()
{
	if ( IsEnabledTimeHack )
	{
		TimeHackBaseTime += 1.0 / ScreenshotFramerate;
		return TimeHackBaseTime;
	}
	else
	{
		return GetSeconds();
	}
}

void Red::System::Timer::SetScreenshotFramerate( Double framerate )
{
	ScreenshotFramerate = framerate;
}

#endif
