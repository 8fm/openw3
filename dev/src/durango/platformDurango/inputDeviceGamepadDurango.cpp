/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/inputUtils.inl"

#include "inputDeviceGamepadDurango.h"

namespace
{
	inline Float NormalizeAxisValue( Float value, Float deadzone )
	{
		const Float rangedValue = value; // Already [-1, 1]
		const Float absRange = 1.f - deadzone;
		RED_ASSERT( absRange > 0.f );

		if ( rangedValue > +deadzone )
		{
			return ( rangedValue - deadzone ) / absRange;
		}
		else if ( rangedValue < -deadzone )
		{
			return ( rangedValue + deadzone ) / absRange;
		}

		return 0.f;
	}
}

//////////////////////////////////////////////////////////////////////////
// CInputDeviceGamepadDurango
//////////////////////////////////////////////////////////////////////////
// FIXME: Deadzone values taken from the XDK samples. Should tweak/make configurable.
const Float CInputDeviceGamepadDurango::LEFT_THUMBSTICK_DEADZONE = 7849 / 32768.0f; 
const Float CInputDeviceGamepadDurango::RIGHT_THUMBSTICK_DEADZONE = 8689 / 32768.0f;

const CInputDeviceGamepadDurango::GamepadButtons CInputDeviceGamepadDurango::GAMEPAD_BUTTONS[] = 
{
	GamepadButtons::A, GamepadButtons::B, GamepadButtons::X, GamepadButtons::Y,
	GamepadButtons::View, GamepadButtons::Menu,
	GamepadButtons::DPadUp, GamepadButtons::DPadDown, GamepadButtons::DPadLeft, GamepadButtons::DPadRight,
	GamepadButtons::LeftThumbstick, GamepadButtons::RightThumbstick,
	GamepadButtons::LeftShoulder, GamepadButtons::RightShoulder,
};

CInputDeviceGamepadDurango::CInputDeviceGamepadDurango()
	: m_gamepad( nullptr )
	, m_prevTimestamp()
	, m_prevButtonMask( 0 )
	, m_needsReset( false )
	, m_needsClear( true )
{
	Clear();
}

void CInputDeviceGamepadDurango::Clear()
{
	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevTimestamp = DateTime();
	m_prevButtonMask = 0;

	m_needsClear = false;
	m_needsReset = false;
}

void CInputDeviceGamepadDurango::Reset()
{
	m_needsReset = true;
}

void CInputDeviceGamepadDurango::Reset( BufferedInput& outBufferedInput )
{
	for ( Uint32 i = 0; i < (Uint32)EKey::Count; ++i )
	{
		if ( m_keyDownEventResetTable[ i ] )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyXBOX( (EKey)i ), IACT_Release, 0.f ) );
		}
	}

	// Reset axes, since stateful unlike a mouse
	for ( Uint32 i = 0; i < (Uint32)EAxis::Count; ++i )
	{
		// This member variable already accounts for deadzones, so epsilon as well
		if ( m_axisValueTable[ i ] != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis((EAxis)i ), IACT_Axis, 0.f ) );
		}
	}

	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevTimestamp = DateTime(); // To allow clears when Updated() again

	m_needsReset = false;
	m_needsClear = true;	// Reset > Clear but leaves it in a state that eventually requires clearing
}

// TBD: Can be nullptr if no gamepad engaged
void CInputDeviceGamepadDurango::SetGamepad( IGamepad^ gamepad )
{
	RED_ASSERT( ::SIsMainThread() );

	if ( m_gamepad != gamepad )
	{
		m_gamepad = gamepad;
		m_needsReset = true;
	}
}

void CInputDeviceGamepadDurango::Update( BufferedInput& outBufferedInput )
{
#ifndef RED_FINAL_BUILD
	extern Bool GHackMouseMode;
	if ( GHackMouseMode )
	{
		return;
	}
#endif

	if ( m_needsReset )
	{
		Reset( outBufferedInput );
		return;
	}

	if ( m_needsClear )
	{
		Clear();
	}

	if ( ! m_gamepad )
	{
		return;
	}

	Windows::Xbox::Input::RawGamepadReading rawGamepadReading;
	try
	{		
		rawGamepadReading = m_gamepad->GetRawCurrentReading();
	}
	catch ( Platform::Exception^ ex )
	{
		ERR_ENGINE(TXT("Exception getting gamepad reading: error code = 0x%x, message = %s"), ex->HResult, ex->Message );
		Reset( outBufferedInput );
		return;
	}

	// 1. Generate key events
	const Uint32 updateButtonMask = static_cast< Uint32 >( rawGamepadReading.Buttons );
	const Uint32 pressedEvents = ( m_prevButtonMask ^ updateButtonMask ) & updateButtonMask;		// Changed state and set in update
	const Uint32 releasedEvents = ( m_prevButtonMask ^ updateButtonMask ) & ~updateButtonMask;	// Changed state and cleared in update

	for ( size_t i = 0; i < ARRAY_COUNT( GAMEPAD_BUTTONS ); ++i )
	{
		const GamepadButtons button = GAMEPAD_BUTTONS[ i ];
		const EKey mappedKey = MapButtonToKey( button );

		if ( (pressedEvents & static_cast< Uint32 >( button )) != 0 )
		{
			m_keyDownEventResetTable[ static_cast< Uint32 >( mappedKey ) ] = true;
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyXBOX( mappedKey ), IACT_Press, 1.f ) );
		}
		else if ( (releasedEvents & static_cast< Uint32 >( button )) != 0 )
		{
			m_keyDownEventResetTable[ static_cast< Uint32 >( mappedKey ) ] = false;
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyXBOX( mappedKey ), IACT_Release, 0.f ) );
		}
	}

	// 2. Update axis values
	// TBD: Trigger deadzones
	UpdateTriggers( outBufferedInput, rawGamepadReading.LeftTrigger, rawGamepadReading.RightTrigger );
	UpdateSticks( outBufferedInput, rawGamepadReading.LeftThumbstickX, rawGamepadReading.LeftThumbstickY, rawGamepadReading.RightThumbstickX, rawGamepadReading.RightThumbstickY );

	// 3. Update saved gamepad reading
	m_prevButtonMask = updateButtonMask;
}

void CInputDeviceGamepadDurango::UpdateTriggers( BufferedInput& outBufferedInput, Float leftValue, Float rightValue )
{
	// Left trigger
	{
		Float& leftTriggerRef = m_axisValueTable[ (Uint32)EAxis::LeftTrigger ];
		const Float oldLeftTrigger = leftTriggerRef;
		leftTriggerRef = leftValue; // TBD: trigger deadzone

		if ( leftTriggerRef != 0.f || oldLeftTrigger != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftTrigger ), IACT_Axis, leftTriggerRef ) );
		}
	}

	// Right trigger
	{
		Float& rightTriggerRef = m_axisValueTable[ (Uint32)EAxis::RightTrigger ];
		const Float oldRightTrigger = rightTriggerRef;
		rightTriggerRef = rightValue; // TBD: trigger deadzone

		if ( rightTriggerRef != 0.f || oldRightTrigger != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightTrigger ), IACT_Axis, rightTriggerRef ) );
		}
	}
}

void CInputDeviceGamepadDurango::UpdateSticks( BufferedInput& outBufferedInput, Float leftStickX, Float leftStickY, Float rightStickX, Float rightStickY )
{
	// Left stick X
	{
		Float& leftStickXRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickX ];
		const Float oldLeftStickX = leftStickXRef;
		leftStickXRef = NormalizeAxisValue( leftStickX, LEFT_THUMBSTICK_DEADZONE );

		if ( leftStickXRef != 0.f || oldLeftStickX != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickX), IACT_Axis, leftStickXRef ) );
		}
	}

	// Left stick Y
	{
		Float& leftStickYRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickY ];
		const Float oldLeftStickY = leftStickYRef;
		leftStickYRef = NormalizeAxisValue( leftStickY, LEFT_THUMBSTICK_DEADZONE );

		if ( leftStickYRef != 0.f || oldLeftStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickY), IACT_Axis, leftStickYRef ) );
		}
	}

	// Right stick X
	{
		Float& rightStickXRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickX ];
		const Float oldRightStickX = rightStickXRef;
		rightStickXRef = NormalizeAxisValue( rightStickX, RIGHT_THUMBSTICK_DEADZONE );

		if ( rightStickXRef != 0.f || oldRightStickX != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickX), IACT_Axis, rightStickXRef ) );
		}
	}

	// Right stick Y
	{
		Float& rightStickYRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickY ];
		const Float oldRightStickY = rightStickYRef;
		rightStickYRef = NormalizeAxisValue( rightStickY, RIGHT_THUMBSTICK_DEADZONE );

		if ( rightStickYRef != 0.f || oldRightStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickY), IACT_Axis, rightStickYRef ) );
		}
	}
}

CInputDeviceGamepadDurango::EKey CInputDeviceGamepadDurango::MapButtonToKey( GamepadButtons button ) const
{
	EKey mappedKey = EKey::None;
	switch (button)
	{
		case GamepadButtons::A:
			mappedKey = EKey::A_CROSS;
			break;
		case GamepadButtons::B:
			mappedKey = EKey::B_CIRCLE;
			break;
		case GamepadButtons::X:
			mappedKey = EKey::X_SQUARE;
			break;
		case GamepadButtons::Y:
			mappedKey = EKey::Y_TRIANGLE;
			break;
		case GamepadButtons::View:
			mappedKey = EKey::Back_Select;
			break;
		case GamepadButtons::Menu:
			mappedKey = EKey::Start;
			break;
		case GamepadButtons::DPadUp:
			mappedKey = EKey::DigitUp;
			break;
		case GamepadButtons::DPadDown:
			mappedKey = EKey::DigitDown;
			break;
		case GamepadButtons::DPadLeft:
			mappedKey = EKey::DigitLeft;
			break;
		case GamepadButtons::DPadRight:
			mappedKey = EKey::DigitRight;
			break;
		case GamepadButtons::LeftThumbstick:
			mappedKey = EKey::LeftThumb;
			break;
		case GamepadButtons::RightThumbstick:
			mappedKey = EKey::RightThumb;
			break;
		case GamepadButtons::LeftShoulder:
			mappedKey = EKey::LeftShoulder;
			break;
		case GamepadButtons::RightShoulder:
			mappedKey = EKey::RightShoulder;
			break;
		default:
			RED_HALT( "Unmapped Durango button '%u'", (Uint32)button );
			break;
	}

	return mappedKey;
}

void CInputDeviceGamepadDurango::SetPadVibrate( Float leftVal, Float rightVal )
{
	if ( ! m_gamepad )
	{
		return;
	}

	try
	{		
		// TBD: trigger values?
		Windows::Xbox::Input::GamepadVibration vibration = { 0 };
		vibration.LeftMotorLevel = leftVal;
		vibration.RightMotorLevel = rightVal;
		m_gamepad->SetVibration( vibration );
	}
	catch ( Platform::Exception^ ex )
	{
		ERR_ENGINE(TXT("Exception getting gamepad reading: error code = 0x%x, message = %s"), ex->HResult, ex->Message );
	}
}
