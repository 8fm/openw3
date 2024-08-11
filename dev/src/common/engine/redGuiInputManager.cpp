/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI 

#include "redGuiInputManager.h"
#include "redGuiControl.h"
#include "redGuiImage.h"
#include "redGuiDesktop.h"
#include "redGuiManager.h"
#include "inputBufferedInputEvent.h"
#include "inputKeys.h"
#include "rawInputManager.h"
#include "baseEngine.h"
#include "../core/gatheredResource.h"
#include "../core/configVar.h"
#include "redGuiManager.h"

namespace Config
{
	extern TConfigVar<Bool> cvIsHardwareCursor;
}

namespace RedGui
{
	CRedGuiInputManager::CRedGuiInputManager()
		: m_controlKeyFocus( nullptr )
		, m_controlMouseFocus( nullptr )
		, m_isShiftPressed( false )
		, m_isControlPressed( false )
		, m_isAltPressed( false )
		, m_holdAction( IACT_None )
		, m_holdKey( IK_None )
		, m_holdChar( 0 )
		, m_firstPressKey( false )
		, m_timerKey( 0.0f )
		, m_inputIntervalKey( 0.05f )
		, m_inputDelayFirstKey( 0.4f )
		, m_inputTimeDoubleClick( 0.25f )
		, m_mouseMouseInRedGui( MM_Normal )
	{
		ResetMouseCaptureControl();
	}

	CRedGuiInputManager::~CRedGuiInputManager()
	{
		DisposeCursors();
	}

	Bool CRedGuiInputManager::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		m_holdAction = action;
		Bool result1 = ProcessRedGuiEvents( key, action, data );

		Bool result2 = false;
		Char text = '\0';

		switch(action)
		{
		case IACT_Press:
			ConvertInputKeyToChar( key, text );
			result2 = HandleKeyPush( key, text );

			break;
		case IACT_Release:
			result2 = HandleKeyRelease(key);
			break;
		case IACT_Axis:
			if(key == IK_MouseZ)
			{
				result2 = HandleMouseWheel(data);
			}
			break;
		}

		return ( result1 || result2 );
	}

	Bool CRedGuiInputManager::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
	{
		Bool result = false;
		if(state == 0)
		{
			result = HandleMouseRelease(button);
		}
		else if(state == 1)
		{
			result = HandleMousePush(button);
		}

		return result;
	}

	Bool CRedGuiInputManager::OnViewportTrack( const CMousePacket& packet )
	{
		// remove black parts from left and right side and top and bottom side
		Int32 deltaX = (packet.m_viewport->GetFullWidth() - packet.m_viewport->GetWidth())/2;
		Int32 deltaY = (packet.m_viewport->GetFullHeight() - packet.m_viewport->GetHeight())/2;

		Bool result = HandleMouseMove(Vector2((Float)packet.m_x-deltaX, (Float)packet.m_y-deltaY));

		if( packet.m_y < 0 || packet.m_y > (Int32)packet.m_viewport->GetFullHeight() ||
			packet.m_x < 0 || packet.m_x > (Int32)packet.m_viewport->GetFullWidth() )
		{
			if( m_cursorInViewport == true )
			{
				if( GIsEditor == true || GIsEditorGame == true )
				{
					packet.m_viewport->SetMouseMode( m_mouseMouseInRedGui, true );
					HideCursor();
				}
			}
			m_cursorInViewport = false;
		}
		else
		{
			if( m_cursorInViewport == false )
			{
				if( GIsEditor == true || GIsEditorGame == true )
				{
					packet.m_viewport->SetMouseMode( m_mouseMouseInRedGui, true );
					ShowCursor();
				}
			}
			m_cursorInViewport = true;
		}
	
		return result;
	}

	void CRedGuiInputManager::OnTick(Float timeDelta)
	{
#ifndef RED_PLATFORM_CONSOLE
		if( GetMouseFocusControl() != nullptr )
		{
			EMousePointer pointerId = GetMouseFocusControl()->GetPointer();
			::SetCursor( m_hardwarePointers[pointerId] );
		}
		else
		{
			::SetCursor( m_hardwarePointers[MP_Arrow] );
		}

		//
		if( GGame != nullptr )
		{
			IViewport* viewport = GGame->GetViewport();
			if( viewport != nullptr )
			{
				if( m_mouseMouseInRedGui == MM_Normal )
				{
					ShowCursor();
				}
				viewport->SetMouseMode( m_mouseMouseInRedGui );
			}
		}
#endif

		//
		if(m_holdKey == IK_None)
		{
			return;
		}

		if(IsFocusKey() == false)
		{
			m_holdAction = IACT_None;
			m_holdKey = IK_None;
			m_holdChar = 0;
			return;
		}

		m_timerKey += timeDelta;

		if(m_firstPressKey == true)
		{
			if(m_timerKey > m_inputDelayFirstKey)
			{
				m_firstPressKey = false;
				m_timerKey = 0.0f;
			}
		}
		else
		{
			if(m_timerKey > m_inputIntervalKey)
			{
				while(m_timerKey > m_inputIntervalKey)
				{
					m_timerKey -= m_inputIntervalKey;
				}
				m_controlKeyFocus->PropagateKeyButtonPressed(m_holdKey, m_holdChar);
				ProcessRedGuiEvents( m_holdKey, m_holdAction, 0.0f );

				//
				if(IsFocusKey() == true)
				{
					m_controlKeyFocus->PropagateKeyButtonReleased(m_holdKey);
				}
			}
		}
	}

	Bool CRedGuiInputManager::HandleMousePush(Int32 button)
	{
		CRedGuiControl* oldKeyControlFocus = m_controlKeyFocus;

		// remove key focus if any control doesn't have mouse focus
		if(IsFocusMouse() == false)
		{
			ResetKeyFocusControl();

			if(oldKeyControlFocus != m_controlKeyFocus)
			{
				EventChangeKeyFocus(m_controlKeyFocus);
			}

			return false;
		}

		// if clicked control is inactive
		if(m_controlMouseFocus->GetEnabled() == false)
		{
			return true;
		}

		// start capture
		m_mouseCapture[button] =  true;
		// remember last pressed position
		m_lastPressed[button] = m_mousePosition;

		// find first control in hierarchy which need a key focus, start from control with mouse focus and go to up hierarchy (to parent side)
		CRedGuiControl* control = m_controlMouseFocus;
		while( control != nullptr && control->GetNeedKeyFocus() == false)
		{
			control = control->GetParent();
		}
		SetKeyFocusControl(control);

		//
		if(IsFocusMouse() == true)
		{
			if(m_controlMouseFocus != nullptr)
			{
				// change state
				m_controlMouseFocus->SetState(STATE_Pushed);
				m_controlMouseFocus->MoveToTop();
				m_controlMouseFocus->PropagateMouseButtonPressed(m_mousePosition, (EMouseButton)button);
				m_controlMouseFocus->UpdateKeyFocusedHierarchy();
			}
		}

		if(oldKeyControlFocus != m_controlKeyFocus)
		{
			EventChangeKeyFocus(m_controlKeyFocus);
		}

		return true;
	}

	Bool CRedGuiInputManager::HandleMouseRelease(Int32 button)
	{
		if(IsFocusMouse() == true)
		{
			if(m_controlMouseFocus->GetEnabled() == false)
			{
				return true;
			}

			// drop capture
			if(m_mouseCapture[button] == true)
			{
				m_mouseCapture[button] = false;
			}

			m_controlMouseFocus->PropagateMouseButtonReleased(m_mousePosition, (EMouseButton)button);

			if(m_controlMouseFocus != nullptr)
			{
				// change state
				m_controlMouseFocus->SetState(STATE_Highlighted);
			
				if(button == MB_Left)
				{
					// get tick count from engine
					Double ticks = m_timer.GetDelta();

					if(ticks < m_inputTimeDoubleClick)
					{
						m_controlMouseFocus->PropagateMouseButtonClick( m_mousePosition, (EMouseButton)button );
						if(m_controlMouseFocus != nullptr)
						{
							m_controlMouseFocus->PropagateMouseButtonDoubleClick( m_mousePosition, (EMouseButton)button );
						}
					}
					else
					{
						CRedGuiControl* control = GRedGui::GetInstance().GetLayerManager()->GetControlFromPoint(m_mousePosition);
						if(control == m_controlMouseFocus)
						{
							m_controlMouseFocus->PropagateMouseButtonClick( m_mousePosition, (EMouseButton)button );
						}
						m_timer.Reset();
					}
				}
			}

			HandleMouseMove(m_mousePosition);

			return true;
		}

		return false;
	}

	Bool CRedGuiInputManager::HandleMouseMove( const Vector2& position )
	{
		// set cursor
		UpdateCursorImage( position );

		// get new mouse position
		m_mousePosition.Set(position.X, position.Y);

		if(IsCapturedMouse() == true)
		{
			if(IsFocusMouse() == true)
			{
				for(Uint32 i=0; i<MB_Count; ++i)
				{
					if(m_mouseCapture[i] == true)
					{
						m_controlMouseFocus->PropagateMouseDrag(position, EMouseButton(i));
					}
				}
			}
			else
			{
				ResetMouseCaptureControl();
			}

			return true;
		}

		CRedGuiControl* oldControlMouseFocus = m_controlMouseFocus;

		// get control under mouse position
		CRedGuiControl* control = GRedGui::GetInstance().GetLayerManager()->GetControlFromPoint(m_mousePosition);

		// modal window mode
		if( m_modalWindows.Size() > 0 )
		{
			Bool foundModalParent = false;
			CRedGuiWindow* activeModalWindow = m_modalWindows.Back();

			//
			for( CRedGuiControl* ctrl = control; ctrl != nullptr; ctrl = ctrl->GetParent() )
			{
				if( ctrl == activeModalWindow )
				{
					foundModalParent = true;
					break;
				}
			}

			if( foundModalParent == false )
			{
				return false;
			}
		}

		// if control was not change
		if(m_controlMouseFocus == control)
		{
			Bool isFocus = IsFocusMouse();
			if( isFocus == true )
			{
				m_controlMouseFocus->PropagateMouseMove(m_mousePosition);
			}
			return isFocus;
		}

		//
		CRedGuiControl* saveControl = nullptr;

		// change states
		if(control != nullptr)
		{
			control->SetState(STATE_Highlighted);
		}
		if(m_controlMouseFocus != nullptr)
		{
			m_controlMouseFocus->SetState(STATE_Normal);
		}

		// set new focused control
		for(CRedGuiControl* root = control; root != nullptr; root = root->GetParent())
		{
			if(root->GetRootMouseFocus() == true)
			{
				saveControl = root;
				break;
			}
			root->SetRootMouseFocus(true);
			root->PropagateMouseChangeRootFocus(true);
		}

		// remove old focus
		for(CRedGuiControl* root = m_controlMouseFocus; root != nullptr; root = root->GetParent())
		{
			if(root == saveControl)
			{
				break;
			}
			root->SetRootMouseFocus(false);
			root->PropagateMouseChangeRootFocus(false);
		}

		if(IsFocusMouse() == true && m_controlMouseFocus->GetEnabled() == true)
		{
			m_controlMouseFocus->PropagateMouseLostFocus(control);
		}

		if((control != nullptr) && (control->GetEnabled() == true))
		{
			control->PropagateMouseMove(m_mousePosition);
			control->PropagateMouseSetFocus(m_controlMouseFocus);
		}

		m_controlMouseFocus = control;

		if(oldControlMouseFocus != m_controlMouseFocus)
		{
			EventChangeMouseFocus(m_controlMouseFocus);
		}

		return IsFocusMouse();
	}

	Bool CRedGuiInputManager::HandleMouseWheel(Float data)
	{
		if( data != 0.0f )
		{
			Bool isFocus = IsFocusMouse();
			if (isFocus == true)
			{
				m_controlMouseFocus->PropagateMouseWheel((Int32)data);
			}
			return isFocus;
		}
		return false;
	}

	Bool CRedGuiInputManager::HandleKeyPush(enum EInputKey key, Char text)
	{
		FirstEncoding(key, true);
		StoreKey(key, text);

		Bool wasFocusKey = IsFocusKey();

		// pass keystrokes to the current active text control
		if( wasFocusKey == true )
		{
			m_controlKeyFocus->PropagateKeyButtonPressed(key, text);
		}

		return wasFocusKey;
	}

	Bool CRedGuiInputManager::HandleKeyRelease(enum EInputKey key)
	{
		FirstEncoding(key, false);

		ResetKey();

		Bool wasFocusKey = IsFocusKey();

		// pass keystrokes to the current active text control
		if(IsFocusKey() == true)
		{
			m_controlKeyFocus->PropagateKeyButtonReleased(key);
		}

		return wasFocusKey;
	}

	void CRedGuiInputManager::FirstEncoding(enum EInputKey key, Bool isKeyPressed)
	{
		if( RIM_IS_KEY_DOWN(IK_Ctrl) == true )
		{
			m_isControlPressed = true;
		}
		else
		{
			m_isControlPressed = false;
		}

		if( RIM_IS_KEY_DOWN( IK_CapsLock ) == true )
		{
			m_capsLockActive = !m_capsLockActive;
			return;
		}

		if( RIM_IS_KEY_DOWN( IK_Shift ) == true )
		{
			m_isShiftPressed = true;
			if( m_capsLockActive == true )
			{
				m_isShiftPressed = false;
			}
		}
		else
		{
			m_isShiftPressed = false;
			if( m_capsLockActive == true )
			{
				m_isShiftPressed = true;
		}
		}

		if( RIM_IS_KEY_DOWN( IK_Alt ) == true )
		{
			m_isAltPressed = true;
		}
		else
		{
			m_isAltPressed = false;
		}
	}

	void CRedGuiInputManager::SetKeyFocusControl(CRedGuiControl* control)
	{
		if(control == m_controlKeyFocus)
		{
			return;
		}

		//
		CRedGuiControl* saveControl = nullptr;

		// set new focused control
		for(CRedGuiControl* root = control; root != nullptr; root = root->GetParent())
		{
			if(root->GetRootKeyFocus() == true)
			{
				saveControl = root;
				break;
			}
			root->SetRootKeyFocus(true);
			root->PropagateKeyChangeRootFocus(true);
		}

		//
		for(CRedGuiControl* root = m_controlKeyFocus; root != nullptr; root = root->GetParent())
		{
			if(root == saveControl)
			{
				break;
			}
			root->SetRootKeyFocus(false);
			root->PropagateKeyChangeRootFocus(false);
		}

		// restore state
		if(m_controlKeyFocus)
		{
			m_controlKeyFocus->PropagateKeyLostFocus(control);
		}

		// set new focused control
		if(control != nullptr && control->GetNeedKeyFocus() == true)
		{
			control->PropagateKeySetFocus(m_controlKeyFocus);
		}

		m_controlKeyFocus = control;
	}

	void CRedGuiInputManager::ResetMouseFocusControl()
	{
		CRedGuiControl* mouseFocus = m_controlMouseFocus;
		m_controlMouseFocus = nullptr;

		if( mouseFocus != nullptr )
		{
			for(CRedGuiControl* root = mouseFocus; root != nullptr; root = root->GetParent())
			{
				root->SetRootMouseFocus(false);
				root->PropagateMouseChangeRootFocus(false);
			}

			for(Uint32 i=0; i<MB_Count; ++i)
			{
				m_mouseCapture[i] = false;
				mouseFocus->PropagateMouseButtonReleased(m_lastPressed[i], (EMouseButton)i);
			}

			if(mouseFocus != nullptr)
			{
				mouseFocus->PropagateMouseLostFocus(nullptr);
			}
		}
	}

	void CRedGuiInputManager::StoreKey(EInputKey key, Char text)
	{
		m_holdKey = IK_None;
		m_holdChar = 0;

		if(IsFocusKey() == false)
		{
			return;
		}
		if( (key == IK_Ctrl) || (key == IK_Alt) || (key == IK_Shift) )
		{
			return;
		}

		m_firstPressKey = true;
		m_holdKey = key;
		m_holdChar = text;
		m_timerKey = 0.0f;
	}

	void CRedGuiInputManager::ResetKey()
	{
		m_holdAction = IACT_None;
		m_holdKey = IK_None;
		m_holdChar = 0;
	}

	void CRedGuiInputManager::ResetKeyFocusControl(CRedGuiControl* control)
	{
		if(m_controlKeyFocus == control)
		{
			SetKeyFocusControl(nullptr);
		}
	}

	Bool CRedGuiInputManager::IsFocusMouse() const
	{
		return m_controlMouseFocus != nullptr;
	}

	Bool CRedGuiInputManager::IsFocusKey() const
	{
		return m_controlKeyFocus != nullptr;
	}

	Bool CRedGuiInputManager::IsCapturedMouse() const
	{
		for(Uint32 i=0; i<MB_Count; ++i)
		{
			if(m_mouseCapture[i] == true)
			{
				return true;
			}
		}
		return false;
	}

	void CRedGuiInputManager::ResetKeyFocusControl()
	{
		SetKeyFocusControl(nullptr);
	}

	CRedGuiControl* CRedGuiInputManager::GetMouseFocusControl() const
	{
		return m_controlMouseFocus;
	}

	CRedGuiControl* CRedGuiInputManager::GetKeyFocusControl() const
	{
		return m_controlKeyFocus;
	}

	const Vector2& CRedGuiInputManager::GetLastPressedPosition(EMouseButton button) const
	{
		return m_lastPressed[button];
	}

	const Vector2& CRedGuiInputManager::GetMousePosition() const
	{
		return m_mousePosition;
	}

	Bool CRedGuiInputManager::IsControlPressed() const
	{
		return m_isControlPressed;
	}

	Bool CRedGuiInputManager::IsShiftPressed() const
	{
		return m_isShiftPressed;
	}

	Bool CRedGuiInputManager::IsAltPressed() const
	{
		return m_isAltPressed;
	}

	void CRedGuiInputManager::ResetMouseCaptureControl()
	{
		for(Uint32 i=0; i<MB_Count; ++i)
		{
			m_mouseCapture[i] = false;
		}
	}

	void CRedGuiInputManager::ConvertInputKeyToChar( enum EInputKey key, Char& text )
	{
		text = static_cast< Char >( key );
		Bool shift = IsShiftPressed();
		Bool alt = IsAltPressed();

		switch( key )
		{
		case IK_Enter:
			text = '\n';
			break;
		case IK_Space:
			text = ' ';
			break;
		case IK_Minus:
			text = ( shift ==  true ) ? '_' : '-';
			break;
		case IK_Equals:
			text = ( shift ==  true ) ? '+' : '=';
			break;
		case IK_Tilde:
			text = ( shift ==  true ) ? '~' : '`';
			break;
		case IK_Semicolon:
			text = ( shift ==  true ) ? ':' : ';';
			break;
		case IK_Comma:
			text = ( shift ==  true ) ? '<' : ',';
			break;
		case IK_Period:
			text = ( shift ==  true ) ? '>' : '.';
			break;
		case IK_Slash:
			text = ( shift ==  true ) ? '?' : '/';
			break;
		case IK_LeftBracket:
			text = ( shift ==  true ) ? '{' : '[';
			break;
		case IK_RightBracket:
			text = ( shift ==  true ) ? '}' : ']';
			break;
		case IK_Backslash:
			text = ( shift ==  true ) ? '|' : '\\';
			break;
		case IK_SingleQuote:
			text = ( shift ==  true ) ? '"' : '\'';
			break;
		case IK_0:
			text = ( shift ==  true ) ? ')' : '0';
			break;
		case IK_1:
			text = ( shift ==  true ) ? '!' : '1';
			break;
		case IK_2:
			text = ( shift ==  true ) ? '@' : '2';
			break;
		case IK_3:
			text = ( shift ==  true ) ? '#' : '3';
			break;
		case IK_4:
			text = ( shift ==  true ) ? '$' : '4';
			break;
		case IK_5:
			text = ( shift ==  true ) ? '%' : '5';
			break;
		case IK_6:
			text = ( shift ==  true ) ? '^' : '6';
			break;
		case IK_7:
			text = ( shift ==  true ) ? '&' : '7';
			break;
		case IK_8:
			text = ( shift ==  true ) ? '*' : '8';
			break;
		case IK_9:
			text = ( shift ==  true ) ? '(' : '9';
			break;
		case IK_A:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x104) : (Char)(0x105) ) : text;
			break;
		case IK_E:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x118) : (Char)(0x119) ) : text;
			break;
		case IK_O:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0xD3) : (Char)(0xF3) ) : text;
			break;
		case IK_S:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x15A) : (Char)(0x15B) ) : text;
			break;
		case IK_L:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x141) : (Char)(0x142) ) : text;
			break;
		case IK_Z:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x17B) : (Char)(0x17C) ) : text;
			break;
		case IK_X:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x179) : (Char)(0x17A) ) : text;
			break;
		case IK_C:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x106) : (Char)(0x107) ) : text;
			break;
		case IK_N:
			text = ( alt == true ) ? ( ( shift == true ) ? (Char)(0x143) : (Char)(0x144) ) : text;
			break;
		}

		// TODO - add support for more special characters
	}

	void CRedGuiInputManager::UpdateCursorImage( const Vector2& position )
	{
		if(GetMouseFocusControl() != nullptr)
		{
			EMousePointer pointerId = GetMouseFocusControl()->GetPointer();

			if ( !Config::cvIsHardwareCursor.Get() )
			{
				m_pointer->SetVisible( false );
				m_pointer = m_pointers[pointerId];
				m_pointer->SetVisible( true );
				m_pointer->SetPosition( position );
			}
			else
			{
#ifndef RED_PLATFORM_CONSOLE
				::SetCursor( m_hardwarePointers[pointerId] );
#endif
			}
		}
	}

	void CRedGuiInputManager::LoadPointerImages()
	{
		for( Uint32 i=0; i<MP_Count; ++i )
		{
			m_pointers[i] = new CRedGuiImage( 0, 0, 16, 16 );
			m_pointers[i]->SetVisible( false );
			m_pointers[i]->SetIgnoreGlobalAlpha( true );
			m_pointers[i]->AttachToLayer( TXT("Pointers") );
		}

		m_pointers[MP_Arrow]->SetImage( Resources::GArrowCursor );
		m_pointers[MP_Hand]->SetImage( Resources::GHandCursor );
		m_pointers[MP_Wait]->SetImage( Resources::GWaitCursor );
		m_pointers[MP_Text]->SetImage( Resources::GTextCursor );
		m_pointers[MP_Move]->SetImage( Resources::GMoveCursor );
		m_pointers[MP_HResize]->SetImage( Resources::GHorizontalResizeCursor );
		m_pointers[MP_VResize]->SetImage( Resources::GVerticalResizeCursor );
		m_pointers[MP_SlashResize]->SetImage( Resources::GSlashResizeCursor );
		m_pointers[MP_BackslashResize]->SetImage( Resources::GBackslashResizeCursor );

		// set default
		m_pointer = m_pointers[MP_Arrow];

		// load hardware pointers
#ifndef RED_PLATFORM_CONSOLE
		m_hardwarePointers[MP_Arrow] = ::LoadCursor( nullptr, IDC_ARROW );
		m_hardwarePointers[MP_Hand] = ::LoadCursor( nullptr, IDC_HAND );
		m_hardwarePointers[MP_Wait] = ::LoadCursor( nullptr, IDC_WAIT );
		m_hardwarePointers[MP_Text] = ::LoadCursor( nullptr, IDC_IBEAM );
		m_hardwarePointers[MP_Move] = ::LoadCursor( nullptr, IDC_SIZEALL );
		m_hardwarePointers[MP_HResize] = ::LoadCursor( nullptr, IDC_SIZEWE );
		m_hardwarePointers[MP_VResize] = ::LoadCursor( nullptr, IDC_SIZENS );
		m_hardwarePointers[MP_SlashResize] = ::LoadCursor( nullptr, IDC_SIZENWSE );
		m_hardwarePointers[MP_BackslashResize] = ::LoadCursor( nullptr, IDC_SIZENESW );
#endif
	}

	void CRedGuiInputManager::DisposeCursors()
	{
		for( Uint32 i=0; i<MP_Count; ++i )
		{
			m_pointers[i]->Dispose();
		}
	}

	void CRedGuiInputManager::ShowCursor()
	{
		if ( !Config::cvIsHardwareCursor.Get() )
		{
			if( m_pointer != nullptr )
			{
				m_pointer->SetVisible( true );
#ifndef RED_PLATFORM_CONSOLE
				while( ::ShowCursor( false ) >= 0 );
#endif
			}
		}
		else
		{
			m_pointer->SetVisible( false );
#ifndef RED_PLATFORM_CONSOLE
			while( ::ShowCursor( true ) < 0 );
#endif
		}
	}

	void CRedGuiInputManager::HideCursor()
	{
		if ( !Config::cvIsHardwareCursor.Get() )
		{
			if( m_pointer != nullptr )
			{
				m_pointer->SetVisible( false );
#ifndef RED_PLATFORM_CONSOLE
				while( ::ShowCursor( true ) < 0 );
#endif
			}
		}
		else
		{
			m_pointer->SetVisible( false );
#ifndef RED_PLATFORM_CONSOLE
			if( GIsGame == true || GIsEditorGame == true )
			{
				while( ::ShowCursor( false ) >= 0 );
			}
#endif
		}
	}

	Bool CRedGuiInputManager::CursorIsInViewport() const
	{
		return m_cursorInViewport;
	}

	Bool CRedGuiInputManager::ProcessRedGuiEvents( enum EInputKey key, enum EInputAction action, Float data )
	{
		static Bool canMoveByPad = false;
		static Bool canResizeByPad = false;
		static Float previousFrameData = 0.0f;

		CRedGuiDesktop* activeDesktop = GRedGui::GetInstance().GetActiveDesktop();
		if( activeDesktop == nullptr )
		{
			return false;
		}

		// special case only for desktop
		if( previousFrameData == 0.0f && data == 1.0f )
		{
			if( ( key == IK_Pad_LeftTrigger && action == IACT_Axis ) || ( key == IK_Tab  && this->IsControlPressed() == true && this->IsShiftPressed() == true && action == IACT_Press ))
			{
				if( m_modalWindows.Size() > 0 )
				{
					activeDesktop->SetActiveModalWindow( m_modalWindows.Back() );
					return true;
				}

				activeDesktop->PropagateInternalInputEvent( RGIE_PreviousWindow, data );
				previousFrameData = data;
				return true;
			}
			else if( ( key == IK_Pad_RightTrigger && action == IACT_Axis ) || ( key == IK_Tab  && this->IsControlPressed() == true && action == IACT_Press ) )
			{
				if( m_modalWindows.Size() > 0 )
				{
					activeDesktop->SetActiveModalWindow( m_modalWindows.Back() );
					return true;
				}

				activeDesktop->PropagateInternalInputEvent( RGIE_NextWindow, data );
				previousFrameData = data;
				return true;
			}
		}
		previousFrameData = data;

		// process input events related with axis
		if( action == IACT_Axis )
		{
			const Uint32 GProperFps = 60;
			Float currentFps = GEngine->GetLastTickRate();
			Float factor = ( GProperFps / currentFps ) * 4;

			if( key == IK_Pad_LeftAxisX )
			{
				if( canMoveByPad == true )
				{
					activeDesktop->PropagateInternalInputEvent( RGIE_MoveWindow, Vector2( data * factor, 0.0f ) );
					return true;
				}
				else
				{
					activeDesktop->PropagateInternalInputEvent( RGIE_Move, Vector2( data * factor, 0.0f ) );
					return true;
				}
			}
			else if( key == IK_Pad_LeftAxisY )
			{
				if( canMoveByPad == true )
				{
					activeDesktop->PropagateInternalInputEvent( RGIE_MoveWindow, Vector2( 0.0f, data * -factor ) );
					return true;
				}
				else
				{
					activeDesktop->PropagateInternalInputEvent( RGIE_Move, Vector2( 0.0f, data * -factor ) );
					return true;
				}
			}
			else if( key == IK_Pad_RightAxisX )
			{
				if( canResizeByPad == true )
				{
					activeDesktop->PropagateInternalInputEvent( RGIE_ResizeWindow, Vector2( data * factor, 0.0f ) );
					return true;
				}
			}
			else if( key == IK_Pad_RightAxisY )
			{
				if( canResizeByPad == true )
				{
					activeDesktop->PropagateInternalInputEvent( RGIE_ResizeWindow, Vector2( 0.0f, data * -factor) );
					return true;
				}
			}
		}

		// process input events related with key
		if( action == IACT_Press )
		{
			ERedGuiInputEvent internalEvent = RGIE_Count;

			if( key == IK_Pad_LeftThumb )
			{
				canMoveByPad = !canMoveByPad;
				if( canMoveByPad == true )
				{
					canResizeByPad = false;
				}
				return true;
			}
			else if( key == IK_Pad_RightThumb )
			{
				canResizeByPad = !canResizeByPad;
				if( canResizeByPad == true )
				{
					canMoveByPad = false;
				}
				return true;
			}
			else if( key == IK_Pad_RightShoulder || ( key == IK_Tab  && this->IsShiftPressed() == false && this->IsAltPressed() == false ) )
			{
				internalEvent = RGIE_PreviousControl;
			}
			else if( key == IK_Pad_LeftShoulder || ( key == IK_Tab && this->IsShiftPressed() == true&& this->IsAltPressed() == false ) )
			{
				internalEvent = RGIE_NextControl;
			}
			if( key == IK_Pad_A_CROSS || ( key == IK_Enter && this->IsShiftPressed() == false && this->IsAltPressed() == false ) )
			{
				internalEvent = RGIE_Execute;
			}
			else if( key == IK_Pad_Y_TRIANGLE || ( key == IK_Enter && this->IsShiftPressed() == true && this->IsAltPressed() == false ) )
			{
				internalEvent = RGIE_ActivateSpecialMode;
			}
			else if( key == IK_Pad_X_SQUARE || key == IK_Space )
			{
				internalEvent = RGIE_Select;
			}
			else if( key == IK_Pad_B_CIRCLE || key == IK_Backspace )
			{
				internalEvent = RGIE_Back;
			}
			else if( key == IK_Pad_DigitLeft || key == IK_Left )
			{
				internalEvent = RGIE_Left;
			}
			else if( key == IK_Pad_DigitRight || key == IK_Right )
			{
				internalEvent = RGIE_Right;
			}
			else if( key == IK_Pad_DigitUp || key == IK_Up )
			{
				internalEvent = RGIE_Up;
			}
			else if( key == IK_Pad_DigitDown || key == IK_Down )
			{
				internalEvent = RGIE_Down;
			}
			else if ( key == IK_Pad_Start )
			{
				internalEvent = RGIE_OptionsButton;
			}

			if( internalEvent != RGIE_Count )
			{
				activeDesktop->PropagateInternalInputEvent( internalEvent, data );
				return true;
			}
		}

		return false;
	}

	void CRedGuiInputManager::SetSoftwareCursorEnable( Bool value )
	{
		Config::cvIsHardwareCursor.Set( !value );
		SConfig::GetInstance().Save();
	}

	Bool CRedGuiInputManager::GetSoftwareCursorEnable() const
	{
		return !Config::cvIsHardwareCursor.Get();
	}

	void CRedGuiInputManager::SetActiveModalWindow( CRedGuiWindow* modalWindow )
	{
		ResetKeyFocusControl();
		ResetMouseCaptureControl();
		ResetMouseFocusControl();

		m_modalWindows.PushBack( modalWindow );
		modalWindow->MoveToTop();

		SetKeyFocusControl( modalWindow );
	}

	void CRedGuiInputManager::ResetModalWindow( CRedGuiWindow* modalWindow )
	{
		m_modalWindows.Remove( modalWindow );
	}

	Bool CRedGuiInputManager::CanBeOnTop( CRedGuiControl* control )
	{
		if( m_modalWindows.Empty() == true )
		{
			return true;
		}

		// special case for modal window
		CRedGuiWindow* modalWindow = m_modalWindows.Back();
		for( CRedGuiControl* ctrl = control; ctrl != nullptr; ctrl = ctrl->GetParent())
		{
			if( ctrl == modalWindow )
			{
				return true;
			}
		}
		return false;
	}

	void CRedGuiInputManager::ChangeMouseMode( enum EMouseMode value )
	{
		m_mouseMouseInRedGui = value;
	}

}	// namespace RedGui

#endif	// NO_RED_GUI
