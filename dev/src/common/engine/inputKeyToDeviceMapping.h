#pragma once

#include "inputKeys.h"

namespace InputUtils
{
	enum EInputDeviceType
	{
		IDT_GAMEPAD,
		IDT_KEYBOARD,
		IDT_MOUSE,
		IDT_UNKNOWN
	};

	RED_INLINE EInputDeviceType GetDeviceType( EInputKey key )
	{
		if( ( ( key >= IK_Pad_A_CROSS ) && ( key <= IK_Pad_RightAxisY ) ) || key ==	IK_PS4_OPTIONS || key == IK_PS4_TOUCH_PRESS)
		{
			return IDT_GAMEPAD;
		}

		if( ( key >= IK_LeftMouse ) && ( key <= IK_MiddleMouse ) )
		{
			return IDT_MOUSE;
		}

		if( ( key >= IK_Mouse4  ) && ( key <= IK_Mouse8 ) )
		{
			return IDT_MOUSE;
		}

		if( ( key >= IK_MouseX ) && ( key <= IK_MouseW ) )
		{
			return IDT_MOUSE;
		}

		if( ( key >= IK_Space ) && ( key <= IK_9) )
		{
			return IDT_KEYBOARD;
		}

		if( ( key >= IK_A ) && ( key <= IK_F24 ) )
		{
			return IDT_KEYBOARD;
		}

		if( ( key >= IK_Shift ) && ( key <= IK_CapsLock ) )
		{
			return IDT_KEYBOARD;
		}

		if( ( key == IK_Escape ) || ( key == IK_Tab ) || ( key == IK_Enter ) )
		{
			return IDT_KEYBOARD;
		}

		if( key >= IK_Semicolon || key <= IK_Tilde )
		{
			return IDT_KEYBOARD;
		}

		return IDT_UNKNOWN;
	}
}