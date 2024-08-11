/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "inputKeys.h"
#include "inputDeviceKeyboard.h"
#include "inputDeviceMouse.h"
#include "inputDeviceGamepad.h"

namespace InputUtils
{
	EInputKey RED_INLINE MapGamepadAxis( IInputDeviceGamepad::EAxis axis )
	{
		EInputKey mappedAxis = IK_None;

		switch (axis)
		{
		case IInputDeviceGamepad::EAxis::LeftTrigger:
			mappedAxis = IK_Pad_LeftTrigger;
			break;
		case IInputDeviceGamepad::EAxis::RightTrigger:
			mappedAxis = IK_Pad_RightTrigger;
			break;
		case IInputDeviceGamepad::EAxis::LeftThumbstickX:
			mappedAxis = IK_Pad_LeftAxisX;
			break;
		case IInputDeviceGamepad::EAxis::LeftThumbstickY:
			mappedAxis = IK_Pad_LeftAxisY;
			break;
		case IInputDeviceGamepad::EAxis::RightThumbstickX:
			mappedAxis = IK_Pad_RightAxisX;
			break;
		case IInputDeviceGamepad::EAxis::RightThumbstickY:
			mappedAxis = IK_Pad_RightAxisY;
			break;
		default:
			RED_HALT( "Unknown gamepad axis %u", (Uint32)axis);
			break;
		}

		return mappedAxis;
	}

	EInputKey RED_INLINE MapGamepadKeyCommon( IInputDeviceGamepad::EKey key )
	{
		EInputKey mappedKey = IK_None;

		switch (key)
		{
		case IInputDeviceGamepad::EKey::None:
			mappedKey = IK_None;
			break;
		case IInputDeviceGamepad::EKey::A_CROSS:
			mappedKey = IK_Pad_A_CROSS;
			break;
		case IInputDeviceGamepad::EKey::B_CIRCLE:
			mappedKey = IK_Pad_B_CIRCLE;
			break;
		case IInputDeviceGamepad::EKey::X_SQUARE:
			mappedKey = IK_Pad_X_SQUARE;
			break;
		case IInputDeviceGamepad::EKey::Y_TRIANGLE:
			mappedKey = IK_Pad_Y_TRIANGLE;
			break;
		case IInputDeviceGamepad::EKey::Start:
			mappedKey = IK_Pad_Start;
			break;
		case IInputDeviceGamepad::EKey::Back_Select:
			mappedKey = IK_Pad_Back_Select;
			break;
		case IInputDeviceGamepad::EKey::DigitUp:
			mappedKey = IK_Pad_DigitUp;
			break;
		case IInputDeviceGamepad::EKey::DigitDown:
			mappedKey = IK_Pad_DigitDown;
			break;
		case IInputDeviceGamepad::EKey::DigitLeft:
			mappedKey = IK_Pad_DigitLeft;
			break;
		case IInputDeviceGamepad::EKey::DigitRight:
			mappedKey = IK_Pad_DigitRight;
			break;
		case IInputDeviceGamepad::EKey::LeftThumb:
			mappedKey = IK_Pad_LeftThumb;
			break;
		case IInputDeviceGamepad::EKey::RightThumb:
			mappedKey = IK_Pad_RightThumb;
			break;
		case IInputDeviceGamepad::EKey::LeftShoulder:
			mappedKey = IK_Pad_LeftShoulder;
			break;
		case IInputDeviceGamepad::EKey::RightShoulder:
			mappedKey = IK_Pad_RightShoulder;
			break;
		default:
			RED_HALT( "Unknown gamepad key %u", (Uint32)key );
			break;
		}

		return mappedKey;
	}

	EInputKey RED_INLINE MapGamepadKeyXBOX( IInputDeviceGamepad::EKey key )
	{
		EInputKey mappedKey = IK_None;

		switch (key)
		{
		case IInputDeviceGamepad::EKey::Start:
			mappedKey = IK_Pad_Start;
			break;
		default:
			mappedKey = MapGamepadKeyCommon( key );
		}

		return mappedKey;
	}

	EInputKey RED_INLINE MapGamepadKeyPS4( IInputDeviceGamepad::EKey key )
	{
		EInputKey mappedKey = IK_None;

		switch (key)
		{
		case IInputDeviceGamepad::EKey::Start:
			mappedKey = IK_PS4_OPTIONS;
			break;
		case IInputDeviceGamepad::EKey::TouchPadPress:
			mappedKey = IK_PS4_TOUCH_PRESS;
			break;
		default:
			mappedKey = MapGamepadKeyCommon( key );
		}

		return mappedKey;
	}

	EInputKey RED_INLINE MapKeyboardKey( IInputDeviceKeyboard::EKey key )
	{
		return (EInputKey)(Uint32)key;
	}

	EInputKey RED_INLINE MapMouseButton( IInputDeviceMouse::EButton button )
	{
		EInputKey mappedButton = IK_None;

		switch (button)
		{
		case IInputDeviceMouse::EButton::Left:
			mappedButton = IK_LeftMouse;
			break;
		case IInputDeviceMouse::EButton::Right:
			mappedButton = IK_RightMouse;
			break;
		case IInputDeviceMouse::EButton::Middle:
			mappedButton = IK_MiddleMouse;
			break;
		case IInputDeviceMouse::EButton::B_4:
			mappedButton = IK_Mouse4;
			break;
		case IInputDeviceMouse::EButton::B_5:
			mappedButton = IK_Mouse5;
			break;
		case IInputDeviceMouse::EButton::B_6:
			mappedButton = IK_Mouse6;
			break;
		case IInputDeviceMouse::EButton::B_7:
			mappedButton = IK_Mouse7;
			break;
		case IInputDeviceMouse::EButton::B_8:
			mappedButton = IK_Mouse8;
			break;
		default:
			RED_HALT( "Unknown mouse button %u", (Uint32)button);
			break;
		}

		return mappedButton;
	}

	EInputKey RED_INLINE MapMouseAxis( IInputDeviceMouse::EAxis axis )
	{
		EInputKey mappedAxis = IK_None;

		switch (axis)
		{
		case IInputDeviceMouse::EAxis::DeltaX:
			mappedAxis = IK_MouseX;
			break;
		case IInputDeviceMouse::EAxis::DeltaY:
			mappedAxis = IK_MouseY;
			break;
		case IInputDeviceMouse::EAxis::DeltaWheel:
			mappedAxis = IK_MouseZ;
			break;
		default:
			RED_HALT( "Unknown mouse axis %u", (Uint32)axis);
			break;
		}

		return mappedAxis;
	}

} // namespace InputUtils