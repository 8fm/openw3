/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

// FIXME: Check Windows 8 and the XInput version
// FIXME: Make sure steam controllers don't get enumerated as XInput device if using the steamworks API instead

#include "../../common/engine/inputUtils.inl"

#include "inputDeviceGamepadXInput.h"

namespace
{
	inline Float NormalizeAxisValue( Int16 value, Int16 deadzone )
	{
		const Float absRange = (Float)(0x7FFF - deadzone);
		RED_ASSERT( absRange > 0.f );

		if ( value > +deadzone )
		{
			return (value - deadzone) / absRange;
		}
		else if ( value < -deadzone )
		{
			return (value + deadzone) / absRange;
		}

		return 0.f;
	}

	inline Float NormalizeTriggerValue( Uint8 value, Uint8 deadzone )
	{
		if ( value > deadzone )
		{
			const Float absRange = (Float)(0xFF - deadzone);
			RED_ASSERT( absRange > 0.f );

			return ( value - deadzone ) / absRange;		
		}

		return 0.f;
	}
}

//////////////////////////////////////////////////////////////////////////
// CInputDeviceGamepadXInput
//////////////////////////////////////////////////////////////////////////
const CInputDeviceGamepadXInput::TButton CInputDeviceGamepadXInput::GAMEPAD_BUTTONS[] = 
{
	XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X, XINPUT_GAMEPAD_Y,
	XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START,
	XINPUT_GAMEPAD_DPAD_LEFT, XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_DPAD_UP, XINPUT_GAMEPAD_DPAD_DOWN,
	XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_RIGHT_THUMB,
	XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
};

CInputDeviceGamepadXInput::CInputDeviceGamepadXInput( Uint32 userIndex )
	: m_userIndex( userIndex )
	, m_prevPacketNumber( 0 )
	, m_prevButtonMask( 0 )
	, m_needsClear( true )
	, m_needsReset( false )
{
	ASSERT( userIndex < XUSER_MAX_COUNT, TXT("User index '%u' is too high ('%u' max)"), userIndex, (Uint32)(XUSER_MAX_COUNT-1) );
	Clear();
}

void CInputDeviceGamepadXInput::Update( BufferedInput& outBufferedInput )
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
	
	XINPUT_STATE inputState;
	Red::System::MemorySet( &inputState, 0, sizeof(XINPUT_STATE) );

	const DWORD hr = ::XInputGetState( m_userIndex, &inputState );
	if ( ! SUCCEEDED( hr ) )
	{
		//TBD: optimize when not actually using the device
		// Reset now since we may *never* reacquire the device
		Reset( outBufferedInput );
		return;
	}

	const XINPUT_GAMEPAD& gamepadState = inputState.Gamepad;

	//FIXME: need event repeat for sticks, so can't check packet numbers yet
	if ( m_prevPacketNumber == inputState.dwPacketNumber && m_prevPacketNumber > 0 )
	{
		//return;
	}

	// 1. Generate key events
	const TButton updateButtonMask = gamepadState.wButtons;

	const TButton pressedEvents = ( m_prevButtonMask ^ updateButtonMask ) & updateButtonMask;		// Changed state and set in update
	const TButton releasedEvents = ( m_prevButtonMask ^ updateButtonMask ) & ~updateButtonMask;		// Changed state and cleared in update

	for ( size_t i = 0; i < ARRAY_COUNT( GAMEPAD_BUTTONS ); ++i )
	{
		const TButton button = GAMEPAD_BUTTONS[ i ];
		const EKey mappedKey = MapButtonToKey( button );

		if ( (pressedEvents & button ) != 0 )
		{
			if( !CheckDPadExclusion( mappedKey, true ) )
			{
				m_keyDownEventResetTable[ (Uint32)mappedKey ] = true;
				outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyXBOX( mappedKey ), IACT_Press, 1.f ) );
			}
		}
		else if ( (releasedEvents & button ) != 0 )
		{
			if( !CheckDPadExclusion( mappedKey, false ) )
			{
				m_keyDownEventResetTable[ (Uint32)mappedKey ] = false;
				outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadKeyXBOX( mappedKey ), IACT_Release, 0.f ) );
			}
		}
	}

	// 2. Update axis values
	UpdateTriggers( outBufferedInput, gamepadState.bLeftTrigger, gamepadState.bRightTrigger );
	UpdateSticks( outBufferedInput, gamepadState.sThumbLX, gamepadState.sThumbLY, gamepadState.sThumbRX, gamepadState.sThumbRY );

	// 3. Update saved gamepad reading
	m_prevPacketNumber = inputState.dwPacketNumber;
	m_prevButtonMask = updateButtonMask;
}

void CInputDeviceGamepadXInput::UpdateTriggers( BufferedInput& outBufferedInput, Uint8 leftValue, Uint8 rightValue )
{
	// Left trigger
	{
		Float& leftTriggerRef = m_axisValueTable[ (Uint32)EAxis::LeftTrigger ];
		const Float oldLeftTrigger = leftTriggerRef;
		leftTriggerRef = NormalizeTriggerValue( leftValue, XINPUT_GAMEPAD_TRIGGER_THRESHOLD );

		if ( leftTriggerRef != 0.f || oldLeftTrigger != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftTrigger ), IACT_Axis, leftTriggerRef ) );
		}
	}

	// Right trigger
	{
		Float& rightTriggerRef = m_axisValueTable[ (Uint32)EAxis::RightTrigger ];
		const Float oldRightTrigger = rightTriggerRef;
		rightTriggerRef = NormalizeTriggerValue( rightValue, XINPUT_GAMEPAD_TRIGGER_THRESHOLD );

		if ( rightTriggerRef != 0.f || oldRightTrigger != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightTrigger ), IACT_Axis, rightTriggerRef ) );
		}
	}
}

void CInputDeviceGamepadXInput::UpdateSticks( BufferedInput& outBufferedInput, Int16 leftStickX, Int16 leftStickY, Int16 rightStickX, Int16 rightStickY )
{
	// Left stick X
	{
		Float& leftStickXRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickX ];
		const Float oldLeftStickX = leftStickXRef;
		leftStickXRef = NormalizeAxisValue( leftStickX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );

		if ( leftStickXRef != 0.f || oldLeftStickX != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickX), IACT_Axis, leftStickXRef ) );
		}
	}

	// Left stick Y
	{
		Float& leftStickYRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickY ];
		const Float oldLeftStickY = leftStickYRef;
		leftStickYRef = NormalizeAxisValue( leftStickY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE );

		if ( leftStickYRef != 0.f || oldLeftStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickY), IACT_Axis, leftStickYRef ) );
		}
	}

	// Right stick X
	{
		Float& rightStickXRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickX ];
		const Float oldRightStickX = rightStickXRef;
		rightStickXRef = NormalizeAxisValue( rightStickX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

		if ( rightStickXRef != 0.f || oldRightStickX != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickX), IACT_Axis, rightStickXRef ) );
		}
	}

	// Right stick Y
	{
		Float& rightStickYRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickY ];
		const Float oldRightStickY = rightStickYRef;
		rightStickYRef = NormalizeAxisValue( rightStickY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE );

		if ( rightStickYRef != 0.f || oldRightStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickY), IACT_Axis, rightStickYRef ) );
		}
	}
}

void CInputDeviceGamepadXInput::Clear()
{
	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevPacketNumber = 0;
	m_prevButtonMask = 0;

	m_needsClear = false;
	m_needsReset = false;

	for( Uint32 i = 0; i < 4; ++i )
	{
		m_dPadExclusion[ i ] = false;
	}
}

void CInputDeviceGamepadXInput::Reset()
{
	m_needsReset = true;
}

void CInputDeviceGamepadXInput::Reset( BufferedInput& outBufferedInput )
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

	m_prevPacketNumber = 0; // To allow clears when Updated() again

	m_needsReset = false;
	m_needsClear = true;	// Reset > Clear but leaves it in a state that eventually requires clearing
}

CInputDeviceGamepadXInput::EKey CInputDeviceGamepadXInput::MapButtonToKey( TButton button ) const
{
	EKey mappedKey = EKey::None;
	switch (button)
	{
		case XINPUT_GAMEPAD_A:
			mappedKey = EKey::A_CROSS;
			break;
		case XINPUT_GAMEPAD_B:
			mappedKey = EKey::B_CIRCLE;
			break;
		case XINPUT_GAMEPAD_X:
			mappedKey = EKey::X_SQUARE;
			break;
		case XINPUT_GAMEPAD_Y:
			mappedKey = EKey::Y_TRIANGLE;
			break;
		case XINPUT_GAMEPAD_BACK:
			mappedKey = EKey::Back_Select;
			break;
		case XINPUT_GAMEPAD_START:
			mappedKey = EKey::Start;
			break;
		case XINPUT_GAMEPAD_DPAD_UP:
			mappedKey = EKey::DigitUp;
			break;
		case XINPUT_GAMEPAD_DPAD_DOWN:
			mappedKey = EKey::DigitDown;
			break;
		case XINPUT_GAMEPAD_DPAD_LEFT:
			mappedKey = EKey::DigitLeft;
			break;
		case XINPUT_GAMEPAD_DPAD_RIGHT:
			mappedKey = EKey::DigitRight;
			break;
		case XINPUT_GAMEPAD_LEFT_THUMB:
			mappedKey = EKey::LeftThumb;
			break;
		case XINPUT_GAMEPAD_RIGHT_THUMB:
			mappedKey = EKey::RightThumb;
			break;
		case XINPUT_GAMEPAD_LEFT_SHOULDER:
			mappedKey = EKey::LeftShoulder;
			break;
		case XINPUT_GAMEPAD_RIGHT_SHOULDER:
			mappedKey = EKey::RightShoulder;
			break;
		default:
			RED_HALT( "Unmapped XInput button '%u'", (Uint32)button );
			break;
	}

	return mappedKey;
}


CInputDeviceGamepadXInput::EDPadExclusionCheck CInputDeviceGamepadXInput::IsDPad( EKey key )
{
	switch( key )
	{
		case EKey::DigitDown:
			return EDPadExclusionCheck::DPEC_DOWN;
		case EKey::DigitUp:
			return EDPadExclusionCheck::DPEC_UP;
		case EKey::DigitLeft:
			return EDPadExclusionCheck::DPEC_LEFT;
		case EKey::DigitRight:
			return EDPadExclusionCheck::DPEC_RIGHT;
		default:
			return EDPadExclusionCheck::DPEC_NONE;
	}
}

Bool CInputDeviceGamepadXInput::CheckDPadExclusion( EKey key, Bool pressed )
{
	EDPadExclusionCheck exclusionButton = IsDPad( key );
	if( exclusionButton == DPEC_NONE )
	{
		return false;
	}

	if( pressed )
	{
		Bool toRet = false;
		for( Uint32 i = 0; i < 4; ++i )
		{
			if( m_dPadExclusion[ i ] )
			{
				RED_ASSERT( i != ( Uint32 )exclusionButton );
				toRet = true;
				break;
			}
		}

		if( !toRet )
		{
			m_dPadExclusion[ ( Uint32 )exclusionButton ] = true;
		}
		
		return toRet;
	}
	else
	{
		if( m_dPadExclusion[ ( Uint32 )exclusionButton ] )
		{
			m_dPadExclusion[ ( Uint32 )exclusionButton ] = false;
			return false;
		}

		return true;
	}

	return false;
}

void CInputDeviceGamepadXInput::SetPadVibrate( Float leftVal, Float rightVal )
{
	XINPUT_VIBRATION vibration;
	ZeroMemory( &vibration, sizeof(XINPUT_VIBRATION) );
	vibration.wLeftMotorSpeed = (WORD)(leftVal * 0xFFFF);
	vibration.wRightMotorSpeed = (WORD)(rightVal * 0xFFFF);
	XInputSetState( m_userIndex, &vibration );
}

const CName CInputDeviceGamepadXInput::GetDeviceName() const 
{
	return CNAME( xpad );
}
