/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "inputKeys.h"
#include "inputBufferedInputEvent.h"

#ifndef NO_TABLET_INPUT_SUPPORT
class ITabletContext;
#endif

class IRawInputListener
{
public:
	virtual Bool ProcessInput( const BufferedInput& input ) { return false; }
	virtual void OnInputReset() {}

public:
	virtual ~IRawInputListener() {}
};

// More like CBufferedInputDispatcher or CInputDeviceAdapter now, and the "new" CInputManager should be more like CInputActionManager or sth
class CRawInputManager
{
private:
	struct SImmediateInputState
	{
		Bool	m_keyDownLUT[ EInputKey::IK_Count ];
		Float	m_axisStateLUT[ EInputKey::IK_Count ];
	};

private:
	static const Float					DEFAULT_TABLET_PRESSURE;

private:
	SImmediateInputState				m_immediateInputState;
	BufferedInput						m_bufferedInput;
	BufferedInput						m_resetBufferedInput;
	TDynArray< IRawInputListener* >		m_listeners;
	Float								m_tabletPressure;
	Bool								m_isReset;
	Bool								m_isGamepadAvailable;

public:
	CRawInputManager();
	~CRawInputManager();

	Bool ProcessInput();

	// Return true if motor speed was set, or the operation is unsupported at the moment (eg. no vibration-enabled controller). 
	// Return false if it can be retried.
	Bool SetPadVibrate( Float leftVal, Float rightVal );

	RED_INLINE Bool IsPadAvailable() const { return m_isGamepadAvailable; }

	void RequestReset();

	RED_INLINE const BufferedInput& GetBufferedInput() const { return m_bufferedInput; }

	// Check whether a key was pressed in current update
	Bool WasKeyJustPressed( const EInputKey& key ) const;
    Bool WasKeyJustReleased( const EInputKey& key ) const;
	Bool GetKeyState( EInputKey key ) const;
	Float GetAxisState( EInputKey axis ) const;

	// Get pressure from tablet context
	Float GetTabletPressure() { return m_tabletPressure; }

	// This call puts your context (active area) on top the context overlap order.
	void SetTabletOverlap( Bool overlap );

	// Registers listeners that input is passed too
	RED_INLINE void RegisterListener( IRawInputListener* listener ) { m_listeners.PushBackUnique( listener ); }

	// Unregisters listener if exists
	RED_INLINE void UnregisterListener( IRawInputListener* listener ) { m_listeners.Remove( listener ); }

public:
#ifndef NO_TEST_FRAMEWORK
	// Accessor to the listeners so that the test framework can pass on it's recorded input as though it were the raw input manager
	RED_INLINE TDynArray< IRawInputListener* >& GetListeners() { return m_listeners; }
#endif

private:
	Bool ProcessInput( BufferedInput& bufferedInput );
};

// Input manager singleton
typedef TSingleton< CRawInputManager > SRawInputManager;


// Checks if the key is down
#define RIM_IS_KEY_DOWN( key ) SRawInputManager::GetInstance().GetKeyState( key )

// Gets the current value of given axis
#define RIM_GET_AXIS_VALUE( key ) SRawInputManager::GetInstance().GetAxisState( key )

// Checks if the key was pressed at this frame/tick
#define RIM_KEY_JUST_PRESSED( key ) SRawInputManager::GetInstance().WasKeyJustPressed( key )

// Returns the tablet pressure from 0 to 1 for pen front and 0 to -1 for pen back (eraser)
#define RIM_TABLET_PRESSURE SRawInputManager::GetInstance().GetTabletPressure()
