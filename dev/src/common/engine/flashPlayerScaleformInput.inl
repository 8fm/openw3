/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

namespace Config
{
	extern TConfigVar<Float> cvMouseSensitivityInUI;
}

RED_INLINE void CScaleformInputManager::SetCtrlPressed( Bool value )
{
	m_keyModifierState.SetCtrlPressed( value );
}

RED_INLINE void CScaleformInputManager::SetAltPressed( Bool value )
{
	m_keyModifierState.SetAltPressed( value );
}

RED_INLINE void CScaleformInputManager::SetShiftPressed( Bool value )
{
	m_keyModifierState.SetShiftPressed( value );
}

RED_INLINE void CScaleformInputManager::SetMouseX( Float value )
{
	m_mouseState.m_x = value;
}

RED_INLINE void CScaleformInputManager::SetMouseY( Float value )
{
	m_mouseState.m_y = value;
}

RED_INLINE void CScaleformInputManager::AddMouseDeltaX( Float delta )
{
	m_mouseState.m_x += delta * Config::cvMouseSensitivityInUI.Get();
}

RED_INLINE void CScaleformInputManager::AddMouseDeltaY( Float delta )
{
	m_mouseState.m_y += delta * Config::cvMouseSensitivityInUI.Get();
}

RED_INLINE void CScaleformInputManager::SetLeftStickX( Float data )
{
	m_axisPerTickData.m_leftStick.X = data;
}

RED_INLINE void CScaleformInputManager::SetLeftStickY( Float data )
{
	m_axisPerTickData.m_leftStick.Y = data;
}

RED_INLINE void CScaleformInputManager::SetRightStickX( Float data )
{
	m_axisPerTickData.m_rightStick.X = data;
}

RED_INLINE void CScaleformInputManager::SetRightStickY( Float data )
{
	m_axisPerTickData.m_rightStick.Y = data;
}

RED_INLINE void CScaleformInputManager::SetLeftTrigger( Float data )
{
	m_axisPerTickData.m_leftTrigger = data;
}

RED_INLINE void CScaleformInputManager::SetRightTrigger( Float data )
{
	m_axisPerTickData.m_rightTrigger = data;
}

RED_INLINE void CScaleformInputManager::QueueKeyboardInput( GFx::Event::EventType eventType, SF::Key::Code keyCode )
{
	const KeyInput ki = { eventType, keyCode };
	m_inputQueue.PushBack( InputQueueEntry( ki ) );
}

#endif // USE_SCALEFORM