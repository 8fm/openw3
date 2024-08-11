/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI 

#include "redGuiTypes.h"
#include "viewport.h"

namespace RedGui
{
	class CRedGuiImage;
	class CRedGuiInputManager
	{
	public:
		CRedGuiInputManager();
		virtual ~CRedGuiInputManager();

		void DisposeCursors();

		// Events
		Event1_Package EventChangeMouseFocus;
		Event1_Package EventChangeKeyFocus;

		// Callback functions
		Bool OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
		Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );
		Bool OnViewportTrack( const CMousePacket& packet );
		void OnTick(Float timeDelta);

		// Is any control have mouse focus
		RED_INLINE Bool IsFocusMouse() const;
		// Is any control have key focus
		RED_INLINE Bool IsFocusKey() const;
		// Is any control captured mouse
		RED_INLINE Bool IsCapturedMouse() const;

		// Set key focus for control
		void SetKeyFocusControl(CRedGuiControl* control);
		// Drop key focus for control
		void ResetKeyFocusControl(CRedGuiControl* control);
		// Drop anu key focus
		void ResetKeyFocusControl();

		void SetActiveModalWindow( CRedGuiWindow* modalWindow );
		void ResetModalWindow( CRedGuiWindow* modalWindow );
		Bool CanBeOnTop( CRedGuiControl* control );

		// Get mouse focused control
		CRedGuiControl* GetMouseFocusControl() const;
		// Get key focused control
		CRedGuiControl* GetKeyFocusControl() const;

		// Get position of last mouse button press
		const Vector2& GetLastPressedPosition(EMouseButton button) const;
		// Get current mouse position
		const Vector2& GetMousePosition() const;

		// Is control button pressed
		Bool IsControlPressed() const;
		// Is shift button pressed
		Bool IsShiftPressed() const;
		// Is alt button pressed
		Bool IsAltPressed() const;

		// Reset mouse capture
		void ResetMouseCaptureControl();
		void ResetMouseFocusControl();

		void ShowCursor();
		void HideCursor();

		void ChangeMouseMode( enum EMouseMode value );

		void SetSoftwareCursorEnable( Bool value );
		Bool GetSoftwareCursorEnable() const;

		Bool CursorIsInViewport() const;

		void LoadPointerImages();

	private:
		Bool HandleMousePush(Int32 button);
		Bool HandleMouseRelease(Int32 button);
		Bool HandleMouseMove(const Vector2& position);
		Bool HandleMouseWheel(Float data);

		Bool HandleKeyPush(enum EInputKey key, Char text);
		Bool HandleKeyRelease(enum EInputKey key);

		Bool ProcessRedGuiEvents( enum EInputKey key, enum EInputAction action, Float data );

		void FirstEncoding(enum EInputKey key, Bool isKeyPressed);
		void StoreKey(EInputKey key, Char text);
		void ResetKey();

		void UpdateCursorImage( const Vector2& position );

		void ConvertInputKeyToChar(enum EInputKey key, Char& text);

		// focused controls
		CRedGuiControl*				m_controlMouseFocus;		//!< control focused by mouse
		CRedGuiControl*				m_controlKeyFocus;			//!< control focused by keyboard or gamepad
		TDynArray< CRedGuiWindow* >	m_modalWindows;				//!<

		Bool						m_isControlPressed;			//!< true - if control key is pressed
		Bool						m_isShiftPressed;			//!< true - if shift key is pressed
		Bool						m_isAltPressed;				//!< true - if alt key is pressed
		Bool						m_capsLockActive;			//!< true - if capslock key is active

		Vector2						m_mousePosition;			//!< current mouse position
		EMouseMode				m_mouseMouseInRedGui;		//

		Red::System::StopClock		m_timer;					//!< used for double click

		Vector2						m_lastPressed[MB_Count];	//!< last mouse press position		
		Bool						m_mouseCapture[MB_Count];	//!< true is mouse button captured by active control

		EInputKey					m_holdKey;					//!<
		EInputAction				m_holdAction;				//!<
		Char						m_holdChar;					//!<
		Bool						m_firstPressKey;			//!<
		Float						m_timerKey;					//!<
		Bool						m_cursorInViewport;			//!<

		const Float					m_inputIntervalKey;			//!<
		const Float					m_inputDelayFirstKey;		//!<
		const Float					m_inputTimeDoubleClick;		//!<

		CRedGuiImage*				m_pointer;					//!< 
		CRedGuiImage*				m_pointers[ MP_Count ];		//!<

#ifndef RED_PLATFORM_CONSOLE
		HCURSOR						m_hardwarePointers[ MP_Count ];//!<
#endif
	};

}	// namespace RedGui

#endif	// NO_RED_GUI
