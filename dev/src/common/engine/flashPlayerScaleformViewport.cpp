/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#ifdef USE_SCALEFORM

#include "flashPlayerScaleform.h"
#include "inputBufferedInputEvent.h"

namespace Config
{
	extern TConfigVar<Bool> cvIsHardwareCursor;
}

//////////////////////////////////////////////////////////////////////////

Bool CFlashPlayerScaleform::OnViewportInput( EInputKey key, EInputAction action, Float data )
{
	// Hack: disable anything that might change focus since nobody cares enough in Flash.
	// [PawelM] restored mouse input requested by BartekB
	//if ( key == IK_Alt || key == IK_Tab )
	//{
	//	return true;
	//}

	//ASSERT( ::SIsMainThread() );
	// Alt-tabbing will send Alt on the render thread because rawinputmgr capture input sends fake key release events
	if ( ! ::SIsMainThread() )
	{
		return true;
	}

	// NOTE: This needs changing after dealing with how the viewport provides delta mouse coordinates
	// For now split into two functions to handle it right.

	static EInputKey vlKeyRepeat = IK_None;
	static EInputKey hlKeyRepeat = IK_None;
	static EInputKey vrKeyRepeat = IK_None;
	static EInputKey hrKeyRepeat = IK_None;
	static EInputKey ltKeyRepeat = IK_None;
	static EInputKey rtKeyRepeat = IK_None;

	if ( action == IACT_Axis )
	{
		// Event only sent from viewport when new data
		switch ( key )
		{
		case IK_Pad_LeftAxisX:
			m_inputManager.SetLeftStickX( data );
			break;
		case IK_Pad_LeftAxisY:
			m_inputManager.SetLeftStickY( data );
			break;
		case IK_Pad_RightAxisX:
			m_inputManager.SetRightStickX( data );
			break;
		case IK_Pad_RightAxisY:
			m_inputManager.SetRightStickY( data );
			break;
		case IK_Pad_LeftTrigger:
			m_inputManager.SetLeftTrigger( data );
			break;
		case IK_Pad_RightTrigger:
			m_inputManager.SetRightTrigger( data );
			break;
		default:
			break;
		}
	}

	Float triggerActivationValue = 0.7f;

	Bool mouseMoved = false;
	if ( action == IACT_Axis )
	{
		if ( key == IK_MouseX )
		{
			// More crap - avoid double input
			if ( !Config::cvIsHardwareCursor.Get() )
			{
				m_inputManager.AddMouseDeltaX( data );
				mouseMoved = true;
			}
		}
		else if ( key == IK_MouseY )
		{
			// More crap - avoid double input
			if ( !Config::cvIsHardwareCursor.Get() )
			{
				m_inputManager.AddMouseDeltaY( data );
				mouseMoved = true;
			}
		}
		else if ( key == IK_MouseZ )
		{
			m_inputManager.QueueMouseInput( GFx::Event::MouseWheel, 0, data );
		}
		else if ( key == IK_Pad_LeftAxisY )
		{
			m_inputManager.QueueAnalogInput( SF::Pad_LT /* thumb not trigger */ );

			if ( data > triggerActivationValue && vlKeyRepeat != IK_Joy7 )
			{
				if( vlKeyRepeat == IK_Joy6 ) 
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( vlKeyRepeat ) );
				}

				vlKeyRepeat = IK_Joy7;
				m_inputManager.QueueKeyboardInput(  GFx::Event::KeyDown, static_cast< SF::Key::Code >( vlKeyRepeat ) );
			}
			else if ( data < -triggerActivationValue && vlKeyRepeat != IK_Joy6 )
			{
				if( vlKeyRepeat == IK_Joy7 )
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( vlKeyRepeat ) );
				}

				vlKeyRepeat = IK_Joy6;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( vlKeyRepeat ) );
			}
			else if ( data == 0.f && (vlKeyRepeat == IK_Joy7 || vlKeyRepeat == IK_Joy6) )
			{
				m_inputManager.QueueKeyboardInput(  GFx::Event::KeyUp, static_cast< SF::Key::Code >( vlKeyRepeat ) );
				vlKeyRepeat = IK_None;
			}
		}
		// Left pad axis X
		else if ( key == IK_Pad_LeftAxisX )
		{
			m_inputManager.QueueAnalogInput( SF::Pad_LT /* thumb not trigger */ );

			if ( data > triggerActivationValue && hlKeyRepeat != IK_Joy9 ) 
			{
				if( hlKeyRepeat == IK_Joy8 ) 
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( hlKeyRepeat ) );
				}

				hlKeyRepeat = IK_Joy9;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( hlKeyRepeat ) );
			}
			else if ( data < -triggerActivationValue && hlKeyRepeat != IK_Joy8 )
			{
				if( hlKeyRepeat == IK_Joy9)
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( hlKeyRepeat ) );
				}

				hlKeyRepeat = IK_Joy8;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( hlKeyRepeat ) );
			}
			else if ( data == 0.f && (hlKeyRepeat == IK_Joy8 || hlKeyRepeat == IK_Joy9) )
			{
				m_inputManager.QueueKeyboardInput(  GFx::Event::KeyUp, static_cast< SF::Key::Code >( hlKeyRepeat ) );
				hlKeyRepeat = IK_None;
			}
		}
		// Right pad axis X
		if ( key == IK_Pad_RightAxisX )
		{
			m_inputManager.QueueAnalogInput( SF::Pad_RT /* thumb not trigger */ );

			if ( data > 0.8f && vrKeyRepeat != IK_Joy5 )
			{
				if( vrKeyRepeat == IK_Joy4 )
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( vrKeyRepeat ) );
				}

				vrKeyRepeat = IK_Joy5;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( vrKeyRepeat ) );
			}
			else if ( data < -0.8f && vrKeyRepeat != IK_Joy4 )
			{
				if( vrKeyRepeat == IK_Joy5 )
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( vrKeyRepeat ) );
				}

				vrKeyRepeat = IK_Joy4;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( vrKeyRepeat ) );
			}
			else if ( data == 0.f && (vrKeyRepeat == IK_Joy4 || vrKeyRepeat == IK_Joy5) )
			{
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( vrKeyRepeat ) );
				vrKeyRepeat = IK_None;
			}
		}
		// Right pad axis Y
		else if ( key == IK_Pad_RightAxisY )
		{
			m_inputManager.QueueAnalogInput( SF::Pad_RT /* thumb not trigger */ );

			if ( data > 0.8f && hrKeyRepeat != IK_Joy2 )
			{
				if( hrKeyRepeat == IK_Joy3 )
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( hrKeyRepeat ) );
				}

				hrKeyRepeat = IK_Joy2;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( hrKeyRepeat ) );
			}
			else if ( data < -0.8f && hrKeyRepeat != IK_Joy3 )
			{
				if( hrKeyRepeat == IK_Joy2 )
				{
					m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( hrKeyRepeat ) );
				}

				hrKeyRepeat = IK_Joy3;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( hrKeyRepeat ) );
			}
			else if ( data == 0.f && (hrKeyRepeat == IK_Joy2 || hrKeyRepeat == IK_Joy3) )
			{
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( hrKeyRepeat ) );
				hrKeyRepeat = IK_None;
			}
		}
		// Left pad trigger
		else if ( key == IK_Pad_LeftTrigger )
		{
			m_inputManager.QueueAnalogInput( SF::Pad_L2 /* left trigger */ );

			if ( data > 0.7f && ltKeyRepeat != IK_Pad_LeftTrigger )
			{
				ltKeyRepeat = IK_Pad_LeftTrigger;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( ltKeyRepeat ) );
			}
			else if ( data < 0.2f && ltKeyRepeat == IK_Pad_LeftTrigger )
			{
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( ltKeyRepeat ) );
				ltKeyRepeat = IK_None;
			}
		}
		// Right pad trigger
		else if ( key == IK_Pad_RightTrigger )
		{
			m_inputManager.QueueAnalogInput( SF::Pad_R2 /* right trigger */ );

			if ( data > 0.7f && rtKeyRepeat != IK_Pad_RightTrigger )
			{
				rtKeyRepeat = IK_Pad_RightTrigger;
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyDown, static_cast< SF::Key::Code >( rtKeyRepeat ) );
			}
			else if ( data < 0.2f && rtKeyRepeat == IK_Pad_RightTrigger )
			{
				m_inputManager.QueueKeyboardInput( GFx::Event::KeyUp, static_cast< SF::Key::Code >( rtKeyRepeat ) );
				rtKeyRepeat = IK_None;
			}
		}
	}

	if ( mouseMoved )
	{
		m_inputManager.QueueMouseInput( GFx::Event::MouseMove, 0 );
	}

	if ( key < IK_Last && ( action == IACT_Press || action == IACT_Release ) )
	{
		const Bool down = ( action == IACT_Press );

		Bool isMouse = false;
		switch ( key )
		{
		case IK_LeftMouse:
		case IK_RightMouse:
		case IK_MiddleMouse:
			isMouse = true;		
			break;
		default:
			break;
		}

		if ( isMouse )
		{
			const GFx::Event::EventType mouseEventType =  down ? GFx::Event::MouseDown : GFx::Event::MouseUp;
			m_inputManager.QueueMouseInput( mouseEventType, key - IK_LeftMouse );
		}
		else
		{

			switch ( key )
			{
			case IK_Ctrl:
				m_inputManager.SetCtrlPressed( down );
				break;
			case IK_Alt:
				m_inputManager.SetAltPressed( down );
				break;
			case IK_LShift:
			case IK_RShift:
				m_inputManager.SetShiftPressed( down );
				break;
			}

			const GFx::Event::EventType keyboardEventType = down ? GFx::Event::KeyDown : GFx::Event::KeyUp;

			// Queue even if dpad so we can specifically listen for it if needs be
			m_inputManager.QueueKeyboardInput( keyboardEventType, static_cast< SF::Key::Code >( key ) );

			// Queue an additional direction key event if the dpad, it's up to the Actionscript to make sensible use of input event listeners
			// if they want to listen for this or the dpad key code.
			switch( key )
			{
			case IK_Pad_DigitUp:
				m_inputManager.QueueKeyboardInput( keyboardEventType, static_cast< SF::Key::Code >( IK_Up ) );
				break;
			case IK_Pad_DigitDown:
				m_inputManager.QueueKeyboardInput( keyboardEventType, static_cast< SF::Key::Code >( IK_Down ) );
				break;
			case IK_Pad_DigitLeft:
				m_inputManager.QueueKeyboardInput( keyboardEventType, static_cast< SF::Key::Code >( IK_Left ) );
				break;
			case IK_Pad_DigitRight:
				m_inputManager.QueueKeyboardInput( keyboardEventType, static_cast< SF::Key::Code >( IK_Right ) );
				break;
			default:
				break;
			}
		}
	}
	
	return true;
}

void CFlashPlayerScaleform::CenterMouse()
{
	m_isCenterMouseRequested = true;
}

void CFlashPlayerScaleform::SetMousePosition(Float xPos, Float yPos)
{
	m_isMousePosChangeRequested = true;
	m_desiredMouseX = xPos;
	m_desiredMouseY = yPos;
}

//////////////////////////////////////////////////////////////////////////

void CFlashPlayerScaleform::OnViewportGenerateFragments( CRenderFrame* frame )
{
	ASSERT( ::SIsMainThread() );
}

#endif // USE_SCALEFORM