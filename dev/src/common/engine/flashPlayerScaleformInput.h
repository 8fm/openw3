/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#ifdef USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// IScaleformInputEventListener
//////////////////////////////////////////////////////////////////////////
class IScaleformInputEventListener
{
protected:
	IScaleformInputEventListener() {}
	virtual ~IScaleformInputEventListener() {}

public:
	virtual void OnGFxKeyEvent( const GFx::KeyEvent& event )=0;
	virtual void OnGFxMouseEvent( const GFx::MouseEvent& event )=0;
	virtual void OnGFxGamePadAnalogEvent( const GFx::GamePadAnalogEvent& event )=0;
};

//////////////////////////////////////////////////////////////////////////
// CScaleformInputManager
//////////////////////////////////////////////////////////////////////////
class CScaleformInputManager
{
private:
	IScaleformInputEventListener* m_inputEventListener;

private:
	struct KeyInput
	{
		GFx::Event::EventType	m_eventType;
		SF::Key::Code			m_keyCode;
	};

	struct MouseInput
	{
		GFx::Event::EventType	m_eventType;
		Uint32					m_button;
		Float					m_scrollValue;
	};

	struct AnalogInput
	{
		SF::PadKeyCode			m_padKeyCode;
	};

	struct InputQueueEntry
	{
		enum Type
		{
			InputTypeKey,
			InputTypeMouse,
			InputTypeAnalog,
		};

		union Input
		{
			KeyInput	m_keyInput;
			MouseInput	m_mouseInput;
			AnalogInput m_analogInput;
		};

		Input	u;
		Type	m_type;

		InputQueueEntry() {}
		InputQueueEntry( const KeyInput& keyInput ) : m_type( InputTypeKey ) { u.m_keyInput = keyInput; }
		InputQueueEntry( const MouseInput& mouseInput ) : m_type( InputTypeMouse ) { u.m_mouseInput = mouseInput; }
		InputQueueEntry( const AnalogInput& analogInput ) : m_type( InputTypeAnalog ) { u.m_analogInput = analogInput; }
	};

private:
	struct SAxisPerTickData
	{
		Vector2 m_leftStick; 
		Vector2 m_rightStick;
		Float	m_leftTrigger;
		Float	m_rightTrigger;

		SAxisPerTickData()
			: m_leftStick( 0.0f, 0.0f )
			, m_rightStick( 0.0f, 0.0f )
			, m_leftTrigger( 0.0f )
			, m_rightTrigger( 0.0f )
		{
		}

		RED_INLINE void Clear() { m_leftStick.X = m_leftStick.Y = m_rightStick.X = m_rightStick.Y = m_leftTrigger = m_rightTrigger = 0.0f; }
	};

	struct SMouseState
	{
		Float m_x;
		Float m_y;

		SMouseState() : m_x( 0.f ), m_y( 0.f ) {}
		RED_INLINE void Clear() { m_x = m_y = 0.f; }
	};

	struct SKeyModifierState
	{
		SF::KeyModifiers m_modifiers;

		//TBD: Can add other modifiers if really needed
		RED_INLINE void SetShiftPressed( Bool value )  { m_modifiers.SetShiftPressed( value ); }
		RED_INLINE void SetCtrlPressed( Bool value )   { m_modifiers.SetCtrlPressed( value ); }
		RED_INLINE void SetAltPressed( Bool value )    { m_modifiers.SetAltPressed( value ); }
		RED_INLINE SF::KeyModifiers GetModifiers() const { return m_modifiers; }
		RED_INLINE void Clear() { m_modifiers.Reset(); }

		SKeyModifierState() : m_modifiers() {}
	};

private:
	struct SAnalogEventFinder
	{
		SF::PadKeyCode m_padKeyCode;

		SAnalogEventFinder( SF::PadKeyCode padKeyCode )
			: m_padKeyCode( padKeyCode )
		{}

		Bool operator()( const InputQueueEntry& other ) const
		{
			if ( other.m_type == InputQueueEntry::InputTypeAnalog )
			{
				SF::PadKeyCode otherPadKeyCode = other.u.m_analogInput.m_padKeyCode;
				return m_padKeyCode == otherPadKeyCode;
			}
			return false;
		}
	};

private:
	SMouseState					m_mouseState;
	SKeyModifierState			m_keyModifierState;
	SAxisPerTickData			m_axisPerTickData;
	Rect						m_viewport;

private:
	TDynArray< InputQueueEntry > m_inputQueue;

public:
	void SetCtrlPressed( Bool value );
	void SetAltPressed( Bool value );
	void SetShiftPressed( Bool value );

public:
	void SetMouseX( Float value );
	void SetMouseY( Float value );
	void AddMouseDeltaX( Float delta );
	void AddMouseDeltaY( Float delta );
	void CenterMouse();
	void ChangeMousePosition( Float xPos, Float yPos );

public:
	void SetLeftStickX( Float data );
	void SetLeftStickY( Float data );
	void SetRightStickX( Float data );
	void SetRightStickY( Float data );
	void SetLeftTrigger( Float data );
	void SetRightTrigger( Float data );

public:
	//FIXME:/CHANGEME: - inputting and then outputting GFx events...
	void QueueKeyboardInput( GFx::Event::EventType eventType, SF::Key::Code keyCode );
	void QueueMouseInput( GFx::Event::EventType eventType, Uint32 button, Float scrollValue = 0.0f );	
	void QueueAnalogInput( SF::PadKeyCode padKeyCode );	
	void ProcessInput();
	void ClearInput();
	void SetViewport( const Rect& viewport );

public:
	CScaleformInputManager();
	~CScaleformInputManager();

public:
	void SetInputEventListener( IScaleformInputEventListener* listener );

private:
	void DispatchQueuedInputEvents();
};

#endif // USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////
#include "flashPlayerScaleformInput.inl"
