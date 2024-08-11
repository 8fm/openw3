/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/inputDeviceGamepad.h"
#include "../../common/engine/notifier.h"

#include "inputDevicePlatformDataOrbis.h"
#include "orbisApiCall.h"

enum class EOrbisGamepadEventType
{
	OGE_Reconnected,
	OGE_Disconnected
};

struct SOrbisGamepadEvent
{
	SceUserServiceUserId m_userId;
	EOrbisGamepadEventType m_type;
};

class CGestureRecognizerOrbis;
struct SGestureEvent;

//////////////////////////////////////////////////////////////////////////
// CInputDeviceGamepadOrbis
//////////////////////////////////////////////////////////////////////////
class CInputDeviceGamepadOrbis : public IInputDeviceGamepad, public Events::CNotifier< SOrbisGamepadEvent >
{
private:
	typedef IInputDeviceGamepad				TBaseClass;
	typedef Uint32							TButton;

private:
	static const Uint8						INVALID_TOUCH_PAD_TOUCH_ID		= 255; // VERIFY: Value taken from SDK sample code
	static const Uint8						INITIAL_CONNECTED_COUNT			= 1;

	static const Uint8						DEFAULT_DEAD_ZONE_DELTA			= 13;
	static const Uint16						DEFAULT_TOUCH_PAD_RESOLUTION_X	= 1920;
	static const Uint16						DEFAULT_TOUCH_PAD_RESOLUTION_Y	= 942;
	static const Float						DEFAULT_TOUCH_PAD_PIXEL_DENSITY;

	static const TButton					GAMEPAD_BUTTONS[];

private:
	static const Uint32						MAX_CACHED_GESTURE_EVENTS = 32;

private:
	SInputDevicePlatformDataOrbis			m_platformData;
	CGestureRecognizerOrbis*				m_gestureRecognizer;
	TDynArray< SGestureEvent >				m_gestureEvents;

	Uint8									m_deadZoneLeft;
	Uint8									m_deadZoneRight;
	Uint16									m_touchPadResolutionX;
	Uint16									m_touchPadResolutionY;
	Float									m_touchPadPixelDensity;

	Uint64									m_prevTimestamp;
	Uint32									m_prevButtonMask;
	Uint8									m_prevConnectedCount;

	Bool									m_needsReset;
	Bool									m_needsClear;

	Float									m_axisValueTable[ (Uint32)EAxis::Count ];
	Bool									m_keyDownEventResetTable[ (Uint32)EKey::Count ];

	Uint8									m_largeMotor;
	Uint8									m_smallMotor;

	Bool									m_disconnected;

public:
											CInputDeviceGamepadOrbis( SceUserServiceUserId userId );
											~CInputDeviceGamepadOrbis();
	Bool									Init();

	virtual void							Update( BufferedInput& outBufferedInput ) override final;
	virtual void							Reset() override final;

	virtual void							SetBacklightColor( const Color& color ) override final;
	virtual void							ResetBacklightColor() override final;
	virtual void							SetPadVibrate( Float leftVal, Float rightVal ) override final;
	virtual SInputDevicePlatformData*		GetPlatformData() override final;

private:
	void									Clear();
	void									Reset( BufferedInput& outBufferedInput );

	void									UpdateKeyEvents();

	void									UpdateTriggers( BufferedInput& outBufferedInput, Uint8 leftValue, Uint8 rightValue );
	void									UpdateSticks( BufferedInput& outBufferedInput, Uint8 leftStickX, Uint8 leftStickY, Uint8 rightStickX, Uint8 rightStickY );

	EKey									MapButtonToKey( TButton button ) const;

	void									Cleanup();

public:
	virtual const CName						GetDeviceName() const override { return CNAME( ps4pad ); };
};
