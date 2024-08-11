/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputBufferedInputEvent.h"


class IInputDeviceCommand;

// Inherit from this struct in your respective platforms and
// add any platform generic data that's common to all input devices
struct SInputDevicePlatformData {};

//////////////////////////////////////////////////////////////////////////
// IInputDevice
//////////////////////////////////////////////////////////////////////////
class IInputDevice
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

protected:
											IInputDevice() {}

public:
	virtual									~IInputDevice() {}

	virtual void							Update( BufferedInput& outBufferedInput ) = 0;
	virtual void							Reset() = 0;
	virtual void							PerformCommand( IInputDeviceCommand* command ) = 0;
	virtual SInputDevicePlatformData*		GetPlatformData() { return nullptr; }
	virtual const CName						GetDeviceName() const = 0;

};