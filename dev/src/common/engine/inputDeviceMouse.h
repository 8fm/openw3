/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputDevice.h"
#include "inputDeviceCommand.h"

//////////////////////////////////////////////////////////////////////////
// IInputDeviceMouse
//////////////////////////////////////////////////////////////////////////
class IInputDeviceMouse : public IInputDevice
{
public:
	enum class EButton
	{
		Left,
		Right,
		Middle,
		B_4,
		B_5,
		B_6,
		B_7,
		B_8,
		Count,
	};

	enum class EAxis
	{
		DeltaX,
		DeltaY,
		DeltaWheel,
		Count,
	};

public:
	virtual void PerformCommand( IInputDeviceCommand* command ) override
	{
		command->PerformCommand( this );
	}

	virtual const CName GetDeviceName() const override
	{
		return CNAME( keyboardmouse );
	}
};