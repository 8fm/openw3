/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include <pad.h>

#include "../../common/engine/inputUtils.inl"
#include "../../common/engine/inputManager.h"
#include "../../common/engine/gestureSystem.h"
#include "inputDeviceGamepadOrbis.h"
#include "gestureRecognizerOrbis.h"

namespace
{
	inline Float NormalizeAxisValue( Uint8 value, Uint8 deadzone )
	{
		const Int32 adjustedValue = (Int32)value * 2 - 0xFF; // Map from 0,255 to -255,255
		const Int32 adjustedDeadzone = 2 * deadzone;
		const Float adjustedAbsRange = (Float)(0xFF - adjustedDeadzone);
		RED_ASSERT( adjustedAbsRange > 0.f );

		if ( adjustedValue > +adjustedDeadzone )
		{
			return ( adjustedValue - adjustedDeadzone ) / adjustedAbsRange;
		}
		else if ( adjustedValue < -adjustedDeadzone )
		{
			return ( adjustedValue + adjustedDeadzone ) / adjustedAbsRange;
		}

		return 0.f;
	}

	inline Float NormalizeTriggerValue( Uint8 value, Uint8 deadzone )
	{
		if ( value > deadzone )
		{
			const Float absRange = (Float)(0xFF - deadzone);
			return ( value - deadzone ) / absRange;
		}

		return 0.f;
	}
}

//////////////////////////////////////////////////////////////////////////
// CInputDeviceGamepadPS4Dev
//////////////////////////////////////////////////////////////////////////
const CInputDeviceGamepadOrbis::TButton CInputDeviceGamepadOrbis::GAMEPAD_BUTTONS[] = 
{
	SCE_PAD_BUTTON_L3,
	SCE_PAD_BUTTON_R3,
	SCE_PAD_BUTTON_OPTIONS,
	SCE_PAD_BUTTON_UP,
	SCE_PAD_BUTTON_RIGHT,
	SCE_PAD_BUTTON_DOWN,
	SCE_PAD_BUTTON_LEFT,
/*	SCE_PAD_BUTTON_L2,*/
/*	SCE_PAD_BUTTON_R2,*/
	SCE_PAD_BUTTON_L1,
	SCE_PAD_BUTTON_R1,
	SCE_PAD_BUTTON_TRIANGLE,
	SCE_PAD_BUTTON_CIRCLE,
	SCE_PAD_BUTTON_CROSS,
	SCE_PAD_BUTTON_SQUARE,
	SCE_PAD_BUTTON_TOUCH_PAD,
/*	SCE_PAD_BUTTON_INTERCEPTED, */
};

CInputDeviceGamepadOrbis::CInputDeviceGamepadOrbis( SceUserServiceUserId userId )
	: m_platformData( userId )
	, m_gestureRecognizer( nullptr )
	, m_deadZoneLeft( DEFAULT_DEAD_ZONE_DELTA )
	, m_deadZoneRight( DEFAULT_DEAD_ZONE_DELTA )
	, m_touchPadResolutionX( 0 )
	, m_touchPadResolutionY( 0 )
	, m_touchPadPixelDensity( 1.f )
	, m_prevTimestamp( 0 )
	, m_prevButtonMask( 0 )
	, m_prevConnectedCount( INITIAL_CONNECTED_COUNT )
	, m_needsReset( false )
	, m_needsClear( true )
	, m_largeMotor( 0 )
	, m_smallMotor( 0 )
	, m_disconnected( false )
{
	Clear();
}

Bool CInputDeviceGamepadOrbis::Init()
{
	const Int32 portHandle = ::scePadOpen( m_platformData.m_userId, SCE_PAD_PORT_TYPE_STANDARD, 0, nullptr );
	if ( portHandle <= SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE )
	{
		ERR_ENGINE( TXT("Failed to open DualShock4 port for user ID %d. Internal error code 0x%08x"), static_cast< Int32 >( m_platformData.m_userId ), portHandle );
		return false;
	}

	m_platformData.m_portHandle = portHandle;

	m_gestureRecognizer = new CGestureRecognizerOrbis;
	
	if ( !m_gestureRecognizer->Init( m_platformData.m_portHandle ) )
	{
		ERR_ENGINE(TXT("Failed to initialize gesture recognizer!"));
		return false;
	}

	return true;
}

CInputDeviceGamepadOrbis::~CInputDeviceGamepadOrbis()
{
	Cleanup();
}

void CInputDeviceGamepadOrbis::Update( BufferedInput& outBufferedInput )
{
	if ( m_needsReset )
	{
		Reset( outBufferedInput );
		return;
	}

	if ( m_needsClear )
	{
		Clear();
	}

	ScePadData padData;
	Red::System::MemorySet( &padData, 0, sizeof(ScePadData) );

	const Int32 sceErr = ::scePadReadState( m_platformData.m_portHandle, &padData );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE( TXT("Error reading DualShock4 controller. Internal error code: 0x%08x"), sceErr );
		// FIXME: Handle SCE_HID_ERROR_ALREADY_LOGGED_OUT
		
		//TBD: optimize when not actually using the device
		// Reset now since we may *never* reacquire the device
		Reset( outBufferedInput );
		return;
	}

	//FIXME: need event repeat for sticks, so can't check packet numbers yet
	if ( m_prevTimestamp == padData.timestamp && m_prevTimestamp > 0 )
	{
		//return;
	}

	// FIXME: AND GET CONTROLLER INFO!!!
	// Check "connectedCount" to see if changed and we missed during polling and re-get controller info.
	// Do we need the controller info?
	if ( !padData.connected )
	{
		if( !m_disconnected )
		{
			m_disconnected = true;

			SOrbisGamepadEvent event;
			event.m_userId = m_platformData.m_userId;
			event.m_type = EOrbisGamepadEventType::OGE_Disconnected;
			Send( event );
		}

		// Reset now since we may *never* reacquire the device
		Reset( outBufferedInput );
		return;
	}

	if( m_disconnected )
	{
		m_disconnected = false;

		SOrbisGamepadEvent event;
		event.m_userId = m_platformData.m_userId;
		event.m_type = EOrbisGamepadEventType::OGE_Reconnected;
		Send( event );
	}

 	if ( ( padData.buttons & SCE_PAD_BUTTON_INTERCEPTED) != 0 )
 	{
 		Reset( outBufferedInput );
 		return;
 	}

	if ( padData.connectedCount != m_prevConnectedCount )
	{
		m_prevConnectedCount = padData.connectedCount;
		Reset( outBufferedInput );
		return;
	}

	// Note: We'll handle L2 and R2 as trigger axes.
	// Make sure this matches the left out buttons in GAMEPAD_BUTTONS array
	static const Uint32 ignoreButtonMask = (Uint32)SCE_PAD_BUTTON_L2 | (Uint32)SCE_PAD_BUTTON_R2 | (Uint32)SCE_PAD_BUTTON_INTERCEPTED;

	// 1. Generate key events
	const Uint32 updateButtonMask = padData.buttons & ~ignoreButtonMask;
	const Uint32 prevButtonMask = m_prevButtonMask & ~ignoreButtonMask;

	const Uint32 pressedEvents = ( prevButtonMask ^ updateButtonMask ) & updateButtonMask;		// Changed state and set in update
	const Uint32 releasedEvents = ( prevButtonMask ^ updateButtonMask ) & ~updateButtonMask;	// Changed state and cleared in update

	for ( size_t i = 0; i < ARRAY_COUNT( GAMEPAD_BUTTONS ); ++i )
	{
		const TButton button = GAMEPAD_BUTTONS[ i ];
		const EKey mappedKey = MapButtonToKey( button );

		if ( (pressedEvents & static_cast< Uint32 >( button )) != 0 )
		{
			m_keyDownEventResetTable[ (Uint32)mappedKey ] = true;
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyPS4( mappedKey ), IACT_Press, 1.f ) );
		}
		else if ( (releasedEvents & static_cast< Uint32 >( button )) != 0 )
		{
			m_keyDownEventResetTable[ (Uint32)mappedKey ] = false;
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyPS4( mappedKey ), IACT_Release, 0.f ) );
		}
	}

	// 2. Update axis values
	UpdateTriggers( outBufferedInput, padData.analogButtons.l2, padData.analogButtons.r2 );
	UpdateSticks( outBufferedInput, padData.leftStick.x, padData.leftStick.y, padData.rightStick.x, padData.rightStick.y );

	// 3. Update saved gamepad reading
	m_prevTimestamp = padData.timestamp;
	m_prevButtonMask = padData.buttons;
	m_prevConnectedCount = padData.connectedCount;

	// 4. Update Touch Pad
	{
		m_gestureRecognizer->Update( padData, m_gestureEvents );

		CInputManager* inputMgr = GGame->GetInputManager();
		RED_ASSERT( inputMgr );
		THandle< CGestureSystem > gestureSystem = inputMgr->GetGestureSystem();
		gestureSystem->Update( m_gestureEvents );
		if ( m_gestureEvents.Size() > MAX_CACHED_GESTURE_EVENTS )
		{
			m_gestureEvents.Resize( MAX_CACHED_GESTURE_EVENTS );
		}
		m_gestureEvents.ClearFast();
	}
}

void CInputDeviceGamepadOrbis::UpdateTriggers( BufferedInput& outBufferedInput, Uint8 leftValue, Uint8 rightValue )
{
	// Left trigger
	{
		Float& leftTriggerRef = m_axisValueTable[ (Uint32)EAxis::LeftTrigger ];
		const Float oldLeftTrigger = leftTriggerRef;
		leftTriggerRef = NormalizeTriggerValue( leftValue, 0 ); // TBD: deadzone

		if ( leftTriggerRef != 0.f || oldLeftTrigger != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftTrigger ), IACT_Axis, leftTriggerRef ) );
		}
	}

	// Right trigger
	{
		Float& rightTriggerRef = m_axisValueTable[ (Uint32)EAxis::RightTrigger ];
		const Float oldRightTriggerRef = rightTriggerRef;
		rightTriggerRef = NormalizeTriggerValue( rightValue, 0 ); // TBD: deadzone

		if ( rightTriggerRef != 0.f || oldRightTriggerRef != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightTrigger ), IACT_Axis, rightTriggerRef ) );
		}
	}
}

void CInputDeviceGamepadOrbis::UpdateSticks( BufferedInput& outBufferedInput, Uint8 leftStickX, Uint8 leftStickY, Uint8 rightStickX, Uint8 rightStickY )
{
	// Left stick X
	{
		Float& leftStickXRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickX ];
		const Float oldLeftStickXRef = leftStickXRef;
		leftStickXRef = NormalizeAxisValue( leftStickX, m_deadZoneLeft );

		if ( leftStickXRef != 0.f || oldLeftStickXRef != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickX), IACT_Axis, leftStickXRef ) );
		}
	}

	// Left stick Y
	{
		Uint8 invertedY = 0xFF - leftStickY; // Y values inverted on the PS4 compared to Xbox
		
		Float& leftStickYRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickY ];
		const Float oldLeftStickY = leftStickYRef;
		leftStickYRef = NormalizeAxisValue( invertedY, m_deadZoneLeft );
		
		if ( leftStickYRef != 0.f || oldLeftStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickY), IACT_Axis, leftStickYRef ) );
		}
	}

	// Right stick X
	{
		Float& rightStickXRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickX ];
		const Float oldRightStickX = rightStickXRef;
		rightStickXRef = NormalizeAxisValue( rightStickX, m_deadZoneRight );

		if ( rightStickXRef != 0.f || oldRightStickX != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickX), IACT_Axis, rightStickXRef ) );
		}
	}

	// Right stick Y
	{
		Uint8 invertedY = 0xFF - rightStickY; // Y values inverted on the PS4 compared to Xbox
		Float& rightStickYRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickY ];
		const Float oldRightStickY = rightStickYRef;
		rightStickYRef = NormalizeAxisValue( invertedY, m_deadZoneRight );

		if ( rightStickYRef != 0.f || oldRightStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickY), IACT_Axis, rightStickYRef ) );
		}
	}
}

void CInputDeviceGamepadOrbis::Clear()
{
	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevTimestamp = 0;
	m_prevButtonMask = 0;

	m_needsClear = false;
	m_needsReset = false;
}

void CInputDeviceGamepadOrbis::Reset()
{
	m_needsReset = true;
}

void CInputDeviceGamepadOrbis::Reset( BufferedInput& outBufferedInput )
{
	for ( Uint32 i = 0; i < (Uint32)EKey::Count; ++i )
	{
		if ( m_keyDownEventResetTable[ i ] )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyPS4( (EKey)i ), IACT_Release, 0.f ) );
		}
	}

	// Reset axes, since stateful unlike a mouse
	for ( Uint32 i = 0; i < (Uint32)EAxis::Count; ++i )
	{
		// This member variable already accounts for deadzones, so epsilon as well
		if ( m_axisValueTable[ i ] != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( (EAxis)i ), IACT_Axis, 0.f ) );
		}
	}

	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevTimestamp = 0;

	m_needsReset = false;
	m_needsClear = true;	// Reset > Clear but leaves it in a state that eventually requires clearing
}

CInputDeviceGamepadOrbis::EKey CInputDeviceGamepadOrbis::MapButtonToKey( TButton button ) const
{
	EKey mappedKey = EKey::None;
	switch (button)
	{
	case SCE_PAD_BUTTON_L3:
		mappedKey = EKey::LeftThumb;
		break;
	case SCE_PAD_BUTTON_R3:
		mappedKey = EKey::RightThumb;
		break;
	case SCE_PAD_BUTTON_OPTIONS:
		mappedKey = EKey::Start;
		break;
	case SCE_PAD_BUTTON_UP:
		mappedKey = EKey::DigitUp;
		break;
	case SCE_PAD_BUTTON_RIGHT:
		mappedKey = EKey::DigitRight;
		break;
	case SCE_PAD_BUTTON_DOWN:
		mappedKey = EKey::DigitDown;
		break;
	case SCE_PAD_BUTTON_LEFT:
		mappedKey = EKey::DigitLeft;
		break;
// 	case SCE_PAD_BUTTON_L2:
//	case SCE_PAD_BUTTON_R2:
	case SCE_PAD_BUTTON_TOUCH_PAD:
		mappedKey = EKey::TouchPadPress;
		break;
	case SCE_PAD_BUTTON_L1:
		mappedKey = EKey::LeftShoulder;
		break;
	case SCE_PAD_BUTTON_R1:
		mappedKey = EKey::RightShoulder;
		break;
	case SCE_PAD_BUTTON_TRIANGLE:
		mappedKey = EKey::Y_TRIANGLE;
		break;
	case SCE_PAD_BUTTON_CIRCLE:
		mappedKey = EKey::B_CIRCLE;
		break;
	case SCE_PAD_BUTTON_CROSS:
		mappedKey = EKey::A_CROSS;
		break;
	case SCE_PAD_BUTTON_SQUARE:
		mappedKey = EKey::X_SQUARE;
		break;
	default:
		RED_HALT( "Unmapped SCE button '%u'", (Uint32)button );
		break;
	}

	return mappedKey;
}

void CInputDeviceGamepadOrbis::Cleanup()
{
	Clear();

	if ( m_platformData.m_portHandle != SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE )
	{
		::scePadClose( m_platformData.m_portHandle );
		m_platformData.m_portHandle = SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE;
	}

	delete m_gestureRecognizer;
	m_gestureRecognizer = nullptr;

	Events::CNotifier< SOrbisGamepadEvent >::Clear();
}

void CInputDeviceGamepadOrbis::SetBacklightColor( const Color& color )
{
	ScePadLightBarParam lightBarParam;

	lightBarParam.r = (uint8_t)color.R;
	lightBarParam.g = (uint8_t)color.G;
	lightBarParam.b = (uint8_t)color.B;

	::scePadSetLightBar( m_platformData.m_portHandle, &lightBarParam );
}

void CInputDeviceGamepadOrbis::ResetBacklightColor()
{
	::scePadResetLightBar( m_platformData.m_portHandle );
}

void CInputDeviceGamepadOrbis::SetPadVibrate( Float leftVal, Float rightVal )
{
	ScePadVibrationParam vibParam;
	vibParam.largeMotor = (uint8_t)(leftVal * 0xFF);
	vibParam.smallMotor = (uint8_t)(rightVal * 0xFF);

	::scePadSetVibration( m_platformData.m_portHandle, &vibParam );
}

SInputDevicePlatformData* CInputDeviceGamepadOrbis::GetPlatformData()
{
	return &m_platformData;
}
