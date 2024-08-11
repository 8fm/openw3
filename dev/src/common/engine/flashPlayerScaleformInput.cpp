/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "flashPlayerScaleformInput.h"

namespace Config
{
	TConfigVar<Float> cvMouseSensitivityInUI( "Input", "UIMouseSensitivity", 1.0f, eConsoleVarFlag_Save );
}

CScaleformInputManager::CScaleformInputManager()
	: m_inputEventListener( nullptr )
	, m_viewport( Rect::EMPTY )
{
}

CScaleformInputManager::~CScaleformInputManager()
{
}

void CScaleformInputManager::SetInputEventListener( IScaleformInputEventListener* listener )
{
	m_inputEventListener = listener;
}

void CScaleformInputManager::QueueAnalogInput( SF::PadKeyCode padKeyCode )
{
	if ( !GGame->AnalogScaleformEnabled() || ::FindIf( m_inputQueue.Begin(), m_inputQueue.End(), SAnalogEventFinder( padKeyCode ) ) != m_inputQueue.End() )
	{
		return;
	}

	const AnalogInput ai = { padKeyCode };
	m_inputQueue.PushBack( InputQueueEntry( ai ) );
}

void CScaleformInputManager::QueueMouseInput( GFx::Event::EventType eventType, Uint32 button, Float scrollValue /*= 0.0f */ )
{
	if ( !GGame->AnalogScaleformEnabled() && eventType == GFx::Event::MouseMove)
	{
		return;
	}
	const MouseInput mi = { eventType, button, scrollValue };
	m_inputQueue.PushBack( InputQueueEntry( mi ) );
}

void CScaleformInputManager::ProcessInput()
{
	if ( m_inputEventListener)
	{
		DispatchQueuedInputEvents();
	}

	m_inputQueue.ClearFast();
	m_axisPerTickData.Clear();
}

void CScaleformInputManager::DispatchQueuedInputEvents()
{
	ASSERT( m_inputEventListener );

	for ( TDynArray< InputQueueEntry >::const_iterator it = m_inputQueue.Begin(); it != m_inputQueue.End(); ++it )
	{
		const InputQueueEntry& entry = *it;
		switch( entry.m_type )
		{
		case InputQueueEntry::InputTypeKey:
			{
				const KeyInput& keyInput = entry.u.m_keyInput;
				m_inputEventListener->OnGFxKeyEvent( GFx::KeyEvent( keyInput.m_eventType, keyInput.m_keyCode, 0, 0, m_keyModifierState.GetModifiers() ) );
			}
			break;

		case InputQueueEntry::InputTypeMouse:
			{
				const MouseInput& mouseInput = entry.u.m_mouseInput;
				m_inputEventListener->OnGFxMouseEvent( GFx::MouseEvent( mouseInput.m_eventType, mouseInput.m_button,
					m_mouseState.m_x, m_mouseState.m_y, mouseInput.m_scrollValue ) );
			}
			break;

		case InputQueueEntry::InputTypeAnalog:
			{
				const AnalogInput& analogInput = entry.u.m_analogInput;
				const SF::PadKeyCode padKeyCode =  analogInput.m_padKeyCode;

				switch ( padKeyCode )
				{
				case SF::Pad_LT:
					m_inputEventListener->OnGFxGamePadAnalogEvent( GFx::GamePadAnalogEvent( padKeyCode, m_axisPerTickData.m_leftStick.X, m_axisPerTickData.m_leftStick.Y ) );
					break;
				case SF::Pad_RT:
					m_inputEventListener->OnGFxGamePadAnalogEvent( GFx::GamePadAnalogEvent( padKeyCode, m_axisPerTickData.m_rightStick.X, m_axisPerTickData.m_rightStick.Y ) );
					break;
				case SF::Pad_L2:
					m_inputEventListener->OnGFxGamePadAnalogEvent( GFx::GamePadAnalogEvent( padKeyCode, m_axisPerTickData.m_leftTrigger, 0.f ) );
					break;
				case SF::Pad_R2:
					m_inputEventListener->OnGFxGamePadAnalogEvent( GFx::GamePadAnalogEvent( padKeyCode, m_axisPerTickData.m_rightTrigger, 0.f ) );
					break;
				default:
					break;
				}
			}
			break;

		default:
			break;
		}
	}
}

void CScaleformInputManager::ClearInput()
{
	m_inputQueue.ClearFast();
	m_mouseState.Clear();
	m_keyModifierState.Clear();
	m_axisPerTickData.Clear();
	// m_repeatKeysState.Clear();
}

void CScaleformInputManager::SetViewport( const Rect& viewport )
{
	m_viewport = viewport;

	// Add left and top to account for cachets!
	m_mouseState.m_x = Clamp<Float>(m_mouseState.m_x, 0.f, Float(m_viewport.Width() + m_viewport.m_left-1));
	m_mouseState.m_y = Clamp<Float>(m_mouseState.m_y, 0.f, Float(m_viewport.Height() + m_viewport.m_top-1));
}

void CScaleformInputManager::CenterMouse()
{
	// Add left and top to account for cachets!
	m_mouseState.m_x = (m_viewport.Width() + m_viewport.m_left-1)/2.f;
	m_mouseState.m_y = (m_viewport.Height() + m_viewport.m_top-1)/2.f;
	QueueMouseInput( GFx::Event::MouseMove, 0 );
}

void CScaleformInputManager::ChangeMousePosition( Float xPos, Float yPos )
{
	m_mouseState.m_x = xPos * m_viewport.Width();
	m_mouseState.m_y = yPos * m_viewport.Height();
	QueueMouseInput( GFx::Event::MouseMove, 0 );
}

#endif // USE_SCALEFORM