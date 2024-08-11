/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

//////////////////////////////////////////////////////////////////////////
// IDInputInterface
//////////////////////////////////////////////////////////////////////////
class IDInputInterface
{
public:
	enum class ECaptureMode
	{
		None,
		BackgroundShared,
		ForegroundExclusive,
	};

protected:
											IDInputInterface() {}
	virtual									~IDInputInterface() {}

public:
	virtual void							SetCaptureMode( ECaptureMode captureMode ) = 0;
	virtual void							SetTopLevelHWnd( HWND topLevelHWnd ) = 0;
	virtual void							Unacquire() = 0;
};
