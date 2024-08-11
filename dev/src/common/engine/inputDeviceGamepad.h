/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "inputDevice.h"
#include "inputDeviceCommand.h"

//////////////////////////////////////////////////////////////////////////
// IInputDeviceGamepad
//////////////////////////////////////////////////////////////////////////
class IInputDeviceGamepad : public IInputDevice
{
public:
	enum class EKey
	{
		None,
		A_CROSS,
		B_CIRCLE,
		X_SQUARE,
		Y_TRIANGLE,
		Start,
		Back_Select,
		DigitUp,
		DigitDown,
		DigitLeft,
		DigitRight,
		LeftThumb,		// L3	
		RightThumb,		// R3
		LeftShoulder,	// L1
		RightShoulder,	// R1
		TouchPadPress,	// PS4 specific
		Count,
	};

	enum class EAxis
	{
		LeftTrigger,
		RightTrigger,
		LeftThumbstickX,
		LeftThumbstickY,
		RightThumbstickX,
		RightThumbstickY,
		Count,
	};

protected:
	IInputDeviceGamepad() {}
	virtual ~IInputDeviceGamepad() override {}

public:
	// SetRumble (but steampad haptic feedback might have override on this for "button" feedback like phone)?

	virtual void PerformCommand( IInputDeviceCommand* command ) override
	{
		command->PerformCommand( this );
	}

	virtual void SetBacklightColor( const Color& ) {}
	virtual void ResetBacklightColor() {}

	// Inputs: Left, Right
	virtual void SetPadVibrate( Float, Float ) {}
};