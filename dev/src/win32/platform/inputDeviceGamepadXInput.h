/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../../external/dxsdk(June2010)/include/XInput.h"

#include "../../common/engine/inputDeviceGamepad.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceGamepadXInput
//////////////////////////////////////////////////////////////////////////
class CInputDeviceGamepadXInput : public IInputDeviceGamepad
{
private:
	typedef IInputDeviceGamepad TBaseClass;

private:
	typedef WORD							TButton;

private:
	static const TButton					GAMEPAD_BUTTONS[];

private:
	Uint32									m_userIndex;
	Uint32									m_prevPacketNumber;
	TButton									m_prevButtonMask;

private:
	Bool									m_keyDownEventResetTable[ EKey::Count ];
	Float									m_axisValueTable[ EAxis::Count ];

private:
	Bool									m_needsReset;
	Bool									m_needsClear;

private:

	enum EDPadExclusionCheck
	{
		DPEC_RIGHT,
		DPEC_LEFT,
		DPEC_UP,
		DPEC_DOWN,
		DPEC_NONE
	};

	Bool									m_dPadExclusion[4];

public:
											CInputDeviceGamepadXInput( Uint32 userIndex );

public:
	virtual void							Update( BufferedInput& outBufferedInput ) override;
	virtual void							Reset() override;
	virtual void							SetPadVibrate( Float leftVal, Float rightVal ) override;
public:
	Uint32									GetUserIndex() const { return m_userIndex; }

private:
	void									Clear();
	void									Reset( BufferedInput& outBufferedInput );

private:
	void									UpdateKeyEvents();

private:
	EKey									MapButtonToKey( TButton button ) const;
	void									UpdateAxis( EAxis axis, Float value );

private:
	void									UpdateTriggers( BufferedInput& outBufferedInput, Uint8 leftValue, Uint8 rightValue );
	void									UpdateSticks( BufferedInput& outBufferedInput, Int16  leftStickX, Int16 leftStickY, Int16 rightStickX, Int16 rightStickY );

private:
	EDPadExclusionCheck						IsDPad( EKey key );
	Bool									CheckDPadExclusion( EKey key, Bool pressed );

public:
	virtual const CName 					GetDeviceName() const override;

};
