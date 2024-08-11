/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceGamepad.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceGamepadDurango
//////////////////////////////////////////////////////////////////////////
class CInputDeviceGamepadDurango : public IInputDeviceGamepad
{
private:
	typedef IInputDeviceGamepad TBaseClass;

private:
	typedef Windows::Xbox::Input::IGamepad					IGamepad;
	typedef Windows::Xbox::Input::IGamepadReading			IGamepadReading;
	typedef Windows::Xbox::Input::GamepadButtons			GamepadButtons;
	typedef	Windows::Foundation::DateTime					DateTime;

private:
	static const Float						LEFT_THUMBSTICK_DEADZONE;
	static const Float						RIGHT_THUMBSTICK_DEADZONE;

private:
	static const GamepadButtons				GAMEPAD_BUTTONS[];

private:
	Bool									m_keyDownEventResetTable[ EKey::Count ];
	Float									m_axisValueTable[ EAxis::Count ];

private:
	IGamepad^								m_gamepad;

private:
	DateTime								m_prevTimestamp;
	Uint32									m_prevButtonMask;

private:
	Bool									m_needsReset;
	Bool									m_needsClear;

public:
											CInputDeviceGamepadDurango();

public:
	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;
	virtual void							SetPadVibrate( Float leftVal, Float rightVal ) override;

public:
	void									SetGamepad( IGamepad^ gamepad );
	IGamepad^								GetGamepad() const { return m_gamepad; }

private:
	void									Clear();
	void									Reset( BufferedInput& outBufferedInput );

private:
	EKey									MapButtonToKey( GamepadButtons button ) const;
	void									UpdateTriggers( BufferedInput& outBufferedInput,  Float leftValue, Float rightValue );
	void									UpdateSticks( BufferedInput& outBufferedInput, Float leftStickX, Float leftStickY, Float rightStickX, Float rightStickY );

public:
	virtual const CName						GetDeviceName() const override { return CNAME( xpad ); };

};