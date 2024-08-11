/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "build.h"

typedef Windows::Xbox::Input::Gamepad					Gamepad;
typedef Windows::Xbox::Input::IGamepad					IGamepad;
typedef Windows::Xbox::Input::IGamepadReading			IGamepadReading;
typedef Windows::Xbox::Input::GamepadButtons			GamepadButtons;

static const GamepadButtons GAMEPAD_BUTTONS[] = 
{
	GamepadButtons::A, GamepadButtons::B, GamepadButtons::X, GamepadButtons::Y,
	GamepadButtons::View, GamepadButtons::Menu,
	GamepadButtons::DPadUp, GamepadButtons::DPadDown, GamepadButtons::DPadLeft, GamepadButtons::DPadRight,
	GamepadButtons::LeftThumbstick, GamepadButtons::RightThumbstick,
	GamepadButtons::LeftShoulder, GamepadButtons::RightShoulder,
};

class CInputInitializerDurango
{
public:
	Bool Init()
	{
		m_buttons.Reserve(14);
		for( auto button : GAMEPAD_BUTTONS )
		{
			m_buttons[static_cast<Uint32>(button)] = false;
		}

		m_prevButtonMask = 0;


		IGamepad^ gamepad = nullptr;
		auto gamepads = Gamepad::Gamepads;
		if ( gamepads->Size > 0 )
		{
			m_gamepad = gamepads->GetAt(0);
		}

		return true;
	}

	Bool ReadInput()
	{
		IGamepadReading^ gamepadReading = nullptr;
		gamepadReading = m_gamepad->GetCurrentReading();
		Uint32 currentButtonMask = static_cast< Uint32 >( gamepadReading->Buttons );

		for( auto button : GAMEPAD_BUTTONS )
		{
			Uint32 b = static_cast<Uint32>(button);
			if( !(b & currentButtonMask) && (b & m_prevButtonMask) )
			{
				m_buttons[b] = true;
			}
		}

		m_prevButtonMask = currentButtonMask;
		return true;
	}

	Bool ButtonPressed( Uint32 button )
	{
		if( m_buttons[button] )
		{
			m_buttons[button] = false;
			return true;
		}
		return false;
	}

	void Shutdown()
	{
		
	}

private:
	IGamepad^ m_gamepad;

	Uint32 m_prevButtonMask;
	THashMap< Uint32, Bool > m_buttons;
};
