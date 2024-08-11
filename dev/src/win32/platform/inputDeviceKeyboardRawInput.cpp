/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */
 
#include "build.h"
#include "inputDeviceKeyboardRawInput.h"
#include "../../common/engine/inputKeys.h"
#include "../../common/engine/inputBufferedInputEvent.h"

CInputDeviceKeyboardRawInput::CInputDeviceKeyboardRawInput()
	: m_isInitialized( false )
{
	for( Uint32 key=0; key<IK_Count; ++key )
	{
		m_keyStates[key] = EKeyEvent::None;
	}
}

CInputDeviceKeyboardRawInput::~CInputDeviceKeyboardRawInput()
{

}

void CInputDeviceKeyboardRawInput::Update(BufferedInput& outBufferedInput)
{
	for( Uint32 key=0; key<IK_Count; ++key )
	{
		if( m_keyStates[key] == EKeyEvent::Pressed )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( (EInputKey)key, IACT_Press, 1.0f ) );
		}
		else if( m_keyStates[key] == EKeyEvent::Released )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( (EInputKey)key, IACT_Release, 0.0f ) );
		}

		// After update we set the 'change' state of the key to None, so next time we won't send input event to game
		m_keyStates[key] = EKeyEvent::None;
	}
}

void CInputDeviceKeyboardRawInput::Reset()
{
	for( Uint32 key=0; key<IK_Count; ++key )
	{
		m_keyStates[key] = EKeyEvent::None;
	}
}

void CInputDeviceKeyboardRawInput::SetWindowHandle(HWND hWnd)
{
	m_device.hwndTarget = hWnd;

	if( m_isInitialized == false )
	{
		LazyInit(hWnd);
	}
}

void CInputDeviceKeyboardRawInput::OnWindowsInput( WPARAM wParam, LPARAM lParam )
{
	RED_FATAL_ASSERT( m_isInitialized == true, "CInputDeviceKeyboardRawInput is not initialized but is receiving windows messages." );

	RAWINPUT rawInput;
	UINT size = sizeof(RAWINPUT);
	::GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &rawInput, &size, sizeof(RAWINPUTHEADER));

	// extract keyboard raw input data
	if (rawInput.header.dwType == RIM_TYPEKEYBOARD)
	{
		const RAWKEYBOARD& rawKB = rawInput.data.keyboard;
		ExtractInputFromRawData( rawKB );
	}
}

void CInputDeviceKeyboardRawInput::ExtractInputFromRawData(const RAWKEYBOARD& rawKeyboard)
{
	UINT virtualKey = rawKeyboard.VKey;
	UINT scanCode = rawKeyboard.MakeCode;
	UINT flags = rawKeyboard.Flags;

	if (virtualKey == 255)
	{
		// Discard 'fake keys' which are part of an escaped sequence
		return;
	}
	else if (virtualKey == VK_SHIFT)
	{
		// Correct 'left/right' 'shift'
		virtualKey = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);
	}
	else if (virtualKey == VK_NUMLOCK)
	{
		// Correct 'pause' / 'break' and 'num lock', and set the extended bit
		scanCode = (MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC) | 0x100);
	}

	// RI_KEY_E0 and RI_KEY_E1 are escape sequences used for certain special keys, such as 'print' and 'pause' / 'break'.
	const bool isRIKeyE0 = ((flags & RI_KEY_E0) != 0);
	const bool isRIKeyE1 = ((flags & RI_KEY_E1) != 0);

	if (isRIKeyE1)
	{
		// For escaped sequences, turn the virtual key into the correct scan code using MapVirtualKey.
		// however, MapVirtualKey is unable to map VK_PAUSE (this is a known bug), hence we map that by hand.
		if (virtualKey == VK_PAUSE)
		{
			scanCode = 0x45;
		}
		else
		{
			scanCode = MapVirtualKey(virtualKey, MAPVK_VK_TO_VSC);
		}
	}

	EInputKey finalKey = (EInputKey)virtualKey;

	switch (virtualKey)
	{
	case VK_CONTROL:
		{
			// If isRIKeyE0 is set, then it's a 'right control', otherwise it's 'left control'
			if (isRIKeyE0)
				finalKey = IK_RControl;
			else
				finalKey = IK_LControl;
			break;
		}

	case VK_MENU:
		{
			// If isRIKeyE0 is set, then it's a 'right alt', but we don't distinguish them in EInputKey, so settings as regular IK_Alt
			finalKey = IK_Alt;
			break;
		}

	case VK_RETURN:
		{
			// If isRIKeyE0 is set, then it's a 'numpad enter', but we don't distinguish them in EInputKey, so settings as regular IK_Enter
			if (isRIKeyE0)
				finalKey = IK_Enter;
			break;
		}

		// The standard 'insert', 'delete', 'home', 'end', 'prior' and 'next' keys will always have their E0 bit set,
		// but the corresponding keys on the numpad will not.
	case VK_INSERT:
		{
			if (!isRIKeyE0)
				finalKey = IK_Insert;
			break;
		}

	case VK_DELETE:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPeriod;
			break;
		}

	case VK_HOME:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad7;
			break;
		}

	case VK_END:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad1;
			break;
		}

	case VK_PRIOR:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad9;
			break;
		}

	case VK_NEXT:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad3;
			break;
		}

		// the standard arrow keys will always have their E0 bit set, but the
		// corresponding keys on the numpad will not.
	case VK_UP:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad8;
			break;
		}

	case VK_DOWN:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad2;
			break;
		}

	case VK_LEFT:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad4;
			break;
		}

	case VK_RIGHT:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad6;
			break;
		}

		// 'numapd 5' doesn't have its E0 bit set
	case VK_CLEAR:
		{
			if (!isRIKeyE0)
				finalKey = IK_NumPad5;
			break;
		}
	}

	// Set the state of the key
	const Bool isPressed = ((flags & RI_KEY_BREAK) == 0);
	m_keyStates[ finalKey ] = isPressed ? EKeyEvent::Pressed : EKeyEvent::Released;
}

void CInputDeviceKeyboardRawInput::LazyInit(HWND hWnd)
{
	m_device.usUsagePage = 0x01;
	m_device.usUsage = 0x06;
	m_device.dwFlags = 0;//RIDEV_NOLEGACY;	// Do not generate legacy messages such as WM_KEYDOWN
	m_device.hwndTarget = hWnd;
	if( !::RegisterRawInputDevices(&m_device, 1, sizeof(m_device)) )
	{
		RED_FATAL_ASSERT(false, "Can't register raw input devices for the application.");
	}

	m_isInitialized = true;
}
