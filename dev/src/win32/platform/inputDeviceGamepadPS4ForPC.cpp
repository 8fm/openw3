/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "inputDeviceGamepadPS4ForPC.h"

#ifdef RED_INPUT_DEVICE_GAMEPAD_PS4FORPC

#include "../../../external/ps4padpc/include/pad.h"
#include "../../common/engine/inputKeyToDeviceMapping.h"
#include "../../common/engine/inputBufferedInputEvent.h"
#include "../../common/engine/inputUtils.inl"
#include "../../common/engine/inputDeviceGamepad.h"

#pragma comment(lib, "hid.lib")

#ifdef RED_CONFIGURATION_DEBUG
	#pragma comment(lib, "../../../external/ps4padpc/lib/Debug/libScePad_static_debug_vs2012.lib")
#else
	#pragma comment(lib, "../../../external/ps4padpc/lib/Release/libScePad_static_vs2012.lib")
#endif

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

const CInputDeviceGamepadPS4ForPC::TButton CInputDeviceGamepadPS4ForPC::GAMEPAD_BUTTONS[] = 
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

Bool CInputDeviceGamepadPS4ForPC::InitializePadLibrary()
{
	int result = scePadInit();
	if (result < 0)
	{
		WARN_ENGINE( TXT("Failed to initialize DualShock4 pad. Internal error code 0x%08x"), result );
		return false;
	}

	return true;
}

void CInputDeviceGamepadPS4ForPC::ShutdownPadLibrary()
{
	scePadTerminate();
}

CInputDeviceGamepadPS4ForPC::CInputDeviceGamepadPS4ForPC()
	: m_portHandle( INVALID_PORT_HANDLE )
	, m_deadZoneLeft( DEFAULT_DEAD_ZONE_DELTA )
	, m_deadZoneRight( DEFAULT_DEAD_ZONE_DELTA )
	, m_prevTimestamp( 0 )
	, m_prevButtonMask( 0 )
	, m_prevConnectedCount( INITIAL_CONNECTED_COUNT )
	, m_needsReset( false )
	, m_needsClear( true )
{
	Clear();
}

CInputDeviceGamepadPS4ForPC::~CInputDeviceGamepadPS4ForPC()
{
	Close();
}

void CInputDeviceGamepadPS4ForPC::UpdateSavedGamepadReading(Uint64 timestamp, Uint32 buttons, Uint8 connectedCount)
{
	m_prevTimestamp = timestamp;
	m_prevButtonMask = buttons;
	m_prevConnectedCount = connectedCount;
}

Bool CInputDeviceGamepadPS4ForPC::Init()
{
	// Open pad
	const Int32 portHandle = ::scePadOpen( SCE_USER_SERVICE_STATIC_USER_ID_1, SCE_PAD_PORT_TYPE_STANDARD, 0, nullptr );
	if ( portHandle <= INVALID_PORT_HANDLE )
	{
		ERR_ENGINE( TXT("Failed to open DualShock4 port for user ID 1. Internal error code 0x%08x"), portHandle );
		return false;
	}
	m_portHandle = portHandle;

	// TODO: initialize gesture recognizer here

	return true;
}

void CInputDeviceGamepadPS4ForPC::Close()
{
	if( m_portHandle != INVALID_PORT_HANDLE )
	{
		Int32 result = scePadClose(m_portHandle);
		if (result < 0)
		{
			ERR_ENGINE( TXT("Failed to close DualShock4 port for user ID 1. Internal error code 0x%08x"), result );
		}
	}
}

void CInputDeviceGamepadPS4ForPC::Update(BufferedInput& outBufferedInput)
{
	ResetIfNecessary(outBufferedInput);
	ClearIfNecessary();

	ScePadData padData;
	Red::System::MemorySet( &padData, 0, sizeof(ScePadData) );
	Bool readSuccessful = ReadPadState( &padData );

	if ( padData.connectedCount != m_prevConnectedCount )
	{
		m_prevConnectedCount = padData.connectedCount;
		Reset( outBufferedInput );
		return;
	}

	if( readSuccessful && padData.connected )
	{
		UpdateButtons( outBufferedInput, padData.buttons );
		UpdateTriggers( outBufferedInput, padData.analogButtons );
		UpdateSticks( outBufferedInput, padData.leftStick, padData.rightStick );

		UpdateSavedGamepadReading( padData.timestamp, padData.buttons, padData.connectedCount );

		// TODO: update touch pad
	}
	else
	{
		Reset(outBufferedInput);	// Reset now since we may *never* reacquire the device
	}

	// TODO: update gesture recognizer here
}

void CInputDeviceGamepadPS4ForPC::Reset()
{
	m_needsReset = true;
}

void CInputDeviceGamepadPS4ForPC::ResetIfNecessary(BufferedInput& outBufferedInput)
{
	if( m_needsReset == true )
	{
		Reset( outBufferedInput );
		m_needsReset = false;
	}
}

void CInputDeviceGamepadPS4ForPC::Reset(BufferedInput& outBufferedInput)
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
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis((EAxis)i ), IACT_Axis, 0.f ) );
		}
	}

	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevTimestamp = 0;

	m_needsReset = false;
	m_needsClear = true;	// Reset > Clear but leaves it in a state that eventually requires clearing
}

void CInputDeviceGamepadPS4ForPC::ClearIfNecessary()
{
	if( m_needsClear == true )
	{
		Clear();
		m_needsClear = false;
	}
}

void CInputDeviceGamepadPS4ForPC::Clear()
{
	Red::System::MemoryZero( m_keyDownEventResetTable, sizeof( m_keyDownEventResetTable ) );
	Red::System::MemoryZero( m_axisValueTable, sizeof( m_axisValueTable ) );

	m_prevTimestamp = 0;
	m_prevButtonMask = 0;

	m_needsClear = false;
	m_needsReset = false;
}

void CInputDeviceGamepadPS4ForPC::UpdateButtons(BufferedInput& outBufferedInput, Uint32 buttons)
{
	// Note: We'll handle L2 and R2 as trigger axes.
	// Make sure this matches the left out buttons in GAMEPAD_BUTTONS array
	static const Uint32 ignoreButtonMask = (Uint32)SCE_PAD_BUTTON_L2 | (Uint32)SCE_PAD_BUTTON_R2 | (Uint32)SCE_PAD_BUTTON_INTERCEPTED;

	// 1. Generate key events
	const Uint32 updateButtonMask = buttons & ~ignoreButtonMask;
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
}

void CInputDeviceGamepadPS4ForPC::UpdateTriggers(BufferedInput& outBufferedInput, const ScePadAnalogButtons& analogButtons)
{
	// Left trigger
	{
		Uint8 leftValue = analogButtons.l2;
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
		Uint8 rightValue = analogButtons.r2;
		Float& rightTriggerRef = m_axisValueTable[ (Uint32)EAxis::RightTrigger ];
		const Float oldRightTriggerRef = rightTriggerRef;
		rightTriggerRef = NormalizeTriggerValue( rightValue, 0 ); // TBD: deadzone

		if ( rightTriggerRef != 0.f || oldRightTriggerRef != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightTrigger ), IACT_Axis, rightTriggerRef ) );
		}
	}
}

void CInputDeviceGamepadPS4ForPC::UpdateSticks(BufferedInput& outBufferedInput, const ScePadAnalogStick& leftStick, const ScePadAnalogStick& rightStick)
{
	// Left stick X
	{
		Float& leftStickXRef = m_axisValueTable[ (Uint32)EAxis::LeftThumbstickX ];
		const Float oldLeftStickXRef = leftStickXRef;
		leftStickXRef = NormalizeAxisValue( leftStick.x, m_deadZoneLeft );

		if ( leftStickXRef != 0.f || oldLeftStickXRef != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::LeftThumbstickX), IACT_Axis, leftStickXRef ) );
		}
	}

	// Left stick Y
	{
		Uint8 invertedY = 0xFF - leftStick.y; // Y values inverted on the PS4 compared to Xbox

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
		rightStickXRef = NormalizeAxisValue( rightStick.x, m_deadZoneRight );

		if ( rightStickXRef != 0.f || oldRightStickX != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickX), IACT_Axis, rightStickXRef ) );
		}
	}

	// Right stick Y
	{
		Uint8 invertedY = 0xFF - rightStick.y; // Y values inverted on the PS4 compared to Xbox
		Float& rightStickYRef = m_axisValueTable[ (Uint32)EAxis::RightThumbstickY ];
		const Float oldRightStickY = rightStickYRef;
		rightStickYRef = NormalizeAxisValue( invertedY, m_deadZoneRight );

		if ( rightStickYRef != 0.f || oldRightStickY != 0.f )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapGamepadAxis( EAxis::RightThumbstickY), IACT_Axis, rightStickYRef ) );
		}
	}
}

void CInputDeviceGamepadPS4ForPC::SetBacklightColor(const Color& color)
{
	ScePadLightBarParam lightBarParam;

	lightBarParam.r = (uint8_t)color.R;
	lightBarParam.g = (uint8_t)color.G;
	lightBarParam.b = (uint8_t)color.B;

	scePadSetLightBar( m_portHandle, &lightBarParam );
}

void CInputDeviceGamepadPS4ForPC::ResetBacklightColor()
{
	scePadResetLightBar( m_portHandle );
}

void CInputDeviceGamepadPS4ForPC::SetPadVibrate(Float leftValue, Float rightValue)
{
	ScePadVibrationParam vibParam;
	vibParam.largeMotor = (uint8_t)(leftValue * 0xFF);
	vibParam.smallMotor = (uint8_t)(rightValue * 0xFF);

	scePadSetVibration( m_portHandle, &vibParam );
}

const CName CInputDeviceGamepadPS4ForPC::GetDeviceName() const
{
	return CNAME(ps4pad);
}

Bool CInputDeviceGamepadPS4ForPC::ReadPadState(ScePadData* padData) const
{
	const Int32 sceErr = ::scePadReadState( m_portHandle, padData );
	if ( sceErr != SCE_OK )
	{
		ERR_ENGINE( TXT("Error reading DualShock4 controller. Internal error code: 0x%08x"), sceErr );
		return false;
	}

	return true;
}

CInputDeviceGamepadPS4ForPC::EKey CInputDeviceGamepadPS4ForPC::MapButtonToKey(TButton button) const
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

#endif // RED_INPUT_DEVICE_GAMEPAD_PS4FORPC
