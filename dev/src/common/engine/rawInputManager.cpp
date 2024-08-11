#include "build.h"
#include "rawInputManager.h"
#include "inputDeviceManager.h"
#include "inputEditorInterface.h"
#include "inputDeviceTablet.h"
#include "game.h"
#include "baseEngine.h"


// Can't be zero by default because the terrain painting tools use this as a scale multiplier even with no tablet...
const Float CRawInputManager::DEFAULT_TABLET_PRESSURE = 1.f;

CRawInputManager::CRawInputManager()
	: m_tabletPressure( DEFAULT_TABLET_PRESSURE )
	, m_isReset( false )
	, m_isGamepadAvailable( true ) // FIXME2<<< Change this when the mouse UI actually works
{
	Red::System::MemoryZero( &m_immediateInputState, sizeof(m_immediateInputState) );
}

CRawInputManager::~CRawInputManager()
{
}

void CRawInputManager::RequestReset()
{
#ifndef RED_FINAL_BUILD
	if( GGame->GetGameplayConfig().m_disableResetInput )
	{
		return;
	}
#endif

	RED_ASSERT( ::SIsMainThread() );

	if ( ! m_isReset )
	{
		m_isReset = true;
		Red::System::ScopedFlag< Bool > flag( m_isReset, false );

		IInputDeviceManager* inputDeviceManager = GEngine->GetInputDeviceManager();
		if ( !inputDeviceManager )
		{
			return;
		}

		inputDeviceManager->RequestReset();

		// So dumb... but so is the way we stop the game through the frame or pause key
		// Need to reset now. Earlier we just recursed over the current state (and from another thread in r4launcher!!!)
		// NOTE: this normally will get called at the end of ProcessInput() by a IRawInputListener
		// so we can reentrantly call ProcessInput to change the state
		ProcessInput( m_resetBufferedInput );
		
		// Won't iterate on this any further if from a rawinputlistener, so update here
		// Yes, this is shit
		m_bufferedInput = m_resetBufferedInput;
		m_resetBufferedInput.ClearFast();
	}
}

Bool CRawInputManager::ProcessInput()
{
	return ProcessInput( m_bufferedInput );
}

Bool CRawInputManager::ProcessInput( BufferedInput& bufferedInput )
{
	RED_ASSERT( ::SIsMainThread() );

	bufferedInput.ClearFast();

	IInputDeviceManager* inputDeviceManager = GEngine->GetInputDeviceManager();
	RED_ASSERT( inputDeviceManager );

	inputDeviceManager->Update( bufferedInput );

	// Should really only change to "up" if nothing set it to down, but it would be a more expensive check
	// for something that really shouldn't be used anyway - more for debug page input and editor support
	// Same for multiple controllers
	for ( auto it = bufferedInput.Begin(); it != bufferedInput.End(); ++it )
	{
		const SBufferedInputEvent& event = *it;
		if ( event.action == IACT_Press )
		{
			m_immediateInputState.m_keyDownLUT[ event.key ] = true;
		}
		else if ( event.action == IACT_Release )
		{
			m_immediateInputState.m_keyDownLUT[ event.key ] = false;
		}
		else if ( event.action == IACT_Axis )
		{
			m_immediateInputState.m_axisStateLUT[ event.key ] = event.data;
		}
		else
		{
			// No change of state
		}
	}

	m_immediateInputState.m_keyDownLUT[ IK_Ctrl ] = m_immediateInputState.m_keyDownLUT[ IK_LControl ] || m_immediateInputState.m_keyDownLUT[ IK_RControl ];
	m_immediateInputState.m_keyDownLUT[ IK_Shift ] = m_immediateInputState.m_keyDownLUT[ IK_LShift ] || m_immediateInputState.m_keyDownLUT[ IK_RShift ];

#ifndef NO_TABLET_INPUT_SUPPORT
	if ( inputDeviceManager->GetEditorInterface() && inputDeviceManager->GetEditorInterface()->GetTablet() )
	{
		IInputDeviceTablet* inputDeviceTablet = inputDeviceManager->GetEditorInterface()->GetTablet();
		m_tabletPressure = inputDeviceTablet->GetPresureState();
	}
	else
	{
		m_tabletPressure = DEFAULT_TABLET_PRESSURE;
	}
#endif

	// TBD: Call even if empty? Does anything rely on being "ticked" in this way?
	for ( Int32 j = m_listeners.SizeInt() - 1; j >= 0; --j )
	{
		IRawInputListener* listener = m_listeners[ j ];
		if ( listener && listener->ProcessInput( bufferedInput ) )
		{
			break;
		}
	}

	// Input is valid
	return true;
}

Bool CRawInputManager::SetPadVibrate( Float leftVal, Float rightVal )
{
	IInputDeviceManager* inputDeviceManager = GEngine->GetInputDeviceManager();
	RED_ASSERT( inputDeviceManager );
	inputDeviceManager->Commands().SetPadVibrate( leftVal, rightVal );

	// Return value isn't very useful here anymore. The rumble update logic should probably be moved into the device itself
	// or somewhere lower than game.
	return true;
}

struct PressedInputFinder : private Red::System::NonCopyable
{
	const EInputKey m_key;

	PressedInputFinder( const EInputKey& key )	: m_key( key ) {}

	Bool operator()( const SBufferedInputEvent& evt ) const { return evt.key == m_key && evt.action == IACT_Press; }
};

struct ReleasedInputFinder : private Red::System::NonCopyable
{
    const EInputKey m_key;

    ReleasedInputFinder( const EInputKey& key )	: m_key( key ) {}

    Bool operator()( const SBufferedInputEvent& evt ) const { return evt.key == m_key && evt.action == IACT_Release; }
};

Bool CRawInputManager::WasKeyJustPressed( const EInputKey& key ) const
{
	RED_ASSERT( ::SIsMainThread() );
	BufferedInput::const_iterator it = FindIf( m_bufferedInput.Begin(), m_bufferedInput.End(), PressedInputFinder( key ) );

	return it != m_bufferedInput.End();
}

Bool CRawInputManager::WasKeyJustReleased( const EInputKey& key ) const
{
    RED_ASSERT( ::SIsMainThread() );
    BufferedInput::const_iterator it = FindIf( m_bufferedInput.Begin(), m_bufferedInput.End(), ReleasedInputFinder( key ) );

    return it != m_bufferedInput.End();
}

Bool CRawInputManager::GetKeyState( EInputKey key ) const
{
	RED_ASSERT( ::SIsMainThread() );

	return m_immediateInputState.m_keyDownLUT[ key ];
}

Float CRawInputManager::GetAxisState( EInputKey axis ) const
{
	RED_ASSERT( ::SIsMainThread() );

	return m_immediateInputState.m_axisStateLUT[ axis ];

	return 0.f;
}
