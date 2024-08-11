/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/inputUtils.inl"

#include "inputDeviceKeyboardDInput.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceKeyboardWin32
//////////////////////////////////////////////////////////////////////////
CInputDeviceKeyboardDInput::CInputDeviceKeyboardDInput( IDirectInputDevice8* directInputDevice, const TKeyTable& directInputKeysLUT )
	: m_directInputDevice( directInputDevice )
	, m_directInputKeysLUT( directInputKeysLUT )
	, m_topLevelHWnd( nullptr )
	, m_captureMode( IDInputInterface::ECaptureMode::None )
	, m_needsClear( true )
	, m_needsReset( false )
{
	RED_ASSERT( directInputDevice );
	
	Clear();
}

CInputDeviceKeyboardDInput::~CInputDeviceKeyboardDInput()
{
	Cleanup();
}

Bool CInputDeviceKeyboardDInput::Init()
{
	if ( ! m_directInputDevice )
	{
		return false;
	}

	// Set keyboard data format
	if ( FAILED( m_directInputDevice->SetDataFormat( &c_dfDIKeyboard ) ) )
	{
		ERR_ENGINE( TXT("Failed to set DirectInput keyboard data format") );
		Cleanup();
		return false;
	}

	// Set Direct Input buffer size
	DIPROPDWORD dipdw;
	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0; 
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = DINPUT_BUFFER_SIZE;
	if ( FAILED( m_directInputDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph) ) )
	{
		ERR_ENGINE( TXT("Unable to set DirectInput keyboard buffer size") );
		Cleanup();
		return false;
	}

	return true;
}

void CInputDeviceKeyboardDInput::Clear()
{
	// Don't unacquire devices, since could still be relying on the behavior. E.g., even if we don't use the mouse input in foreground mode
	// we don't want other apps getting it either

	Red::System::MemorySet( m_keyDownEventResetTable, 0, sizeof(m_keyDownEventResetTable) );

	m_needsClear = false;
	m_needsReset = false;
}

void CInputDeviceKeyboardDInput::Reset()
{
	m_needsReset = true;
}

void CInputDeviceKeyboardDInput::Reset( BufferedInput& outBufferedInput )
{
	for ( Uint32 i = 0; i < (Uint32)EKey::Count; ++i )
	{
		if ( m_keyDownEventResetTable[ i ] )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapKeyboardKey( (EKey)i ), IACT_Release, 0.f ) );
		}
	}

	Red::System::MemorySet( m_keyDownEventResetTable, 0, sizeof(m_keyDownEventResetTable) );

	m_needsReset = false;
	m_needsClear = true;	// Reset > Clear but leaves it in a state that eventually requires clearing
}

void CInputDeviceKeyboardDInput::Unacquire()
{
	if ( m_directInputDevice )
	{
		m_directInputDevice->Unacquire();
	}
}

void CInputDeviceKeyboardDInput::Update( BufferedInput& outBufferedInput )
{
	RED_ASSERT( m_directInputDevice );

	if ( m_needsReset )
	{
		Reset( outBufferedInput );
		
		return;
	}

	if ( m_needsClear )
	{
		Clear();
	}

	if ( m_captureMode == IDInputInterface::ECaptureMode::None )
	{
		return;
	}

	// Grab Direct Input state
	Uint8 keyData[ EKey::Count ];
	DIDEVICEOBJECTDATA didod[ DINPUT_BUFFER_SIZE ];
	DWORD dwItems = DINPUT_BUFFER_SIZE;
	HRESULT hr = m_directInputDevice->GetDeviceState( sizeof(keyData), (LPVOID)&keyData );
	if ( SUCCEEDED(hr) )
	{
		hr = m_directInputDevice->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), didod, &dwItems, 0 );
		if ( hr == DI_BUFFEROVERFLOW )
		{
			WARN_ENGINE(TXT("CInputDeviceKeyboardDInput: buffer overflow"));
		}
	}

	if ( FAILED(hr) )
	{
		// Might never have acquired the device even when capture mode set (e.g., window minimized)
		// So treat both errors as the same.
		if ( hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED )
		{
			m_directInputDevice->Acquire();
		}

		// TBD: Optimize when not actually using device
		// Reset now since we may *never* reacquire the device
		Reset( outBufferedInput );
		return;
	}

	// Process key events. DirectInput buffered keyboard data deals with *events* not immediate state
	// which could be out of sync if the input buffer overflows: DI_BUFFEROVERFLOW
	for ( DWORD dwItem = 0; dwItem < dwItems; ++dwItem )
	{
		const EKey mappedKey = m_directInputKeysLUT[ didod[ dwItem ].dwOfs ];
		if ( mappedKey != EKey::K_None &&
			// Ignore Alt+tab (we don't seem to have any better way to block system keys)
			!(mappedKey == EKey::K_Tab && ::GetAsyncKeyState(VK_MENU)) )
		{
			const Bool pressed = ( didod[ dwItem ].dwData & 0x80 ) != 0;
			if ( pressed )
			{
				m_keyDownEventResetTable[ (Uint32)mappedKey ] = true;
				outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapKeyboardKey( mappedKey ), IACT_Press, 1.0f ) );
			}
			else
			{
				m_keyDownEventResetTable[ (Uint32)mappedKey ] = false;
				outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapKeyboardKey( mappedKey ), IACT_Release, 0.f ) );
			}
		}
	}
}

void CInputDeviceKeyboardDInput::Cleanup()
{
	Clear();

	if ( m_directInputDevice )
	{
		m_directInputDevice->Unacquire();
		m_directInputDevice->Release();
		m_directInputDevice = nullptr;
	}
}

void CInputDeviceKeyboardDInput::SetCaptureMode( IDInputInterface::ECaptureMode captureMode )
{
	if ( m_captureMode == captureMode )
	{
		return;
	}

	m_directInputDevice->Unacquire();

	if ( captureMode == IDInputInterface::ECaptureMode::BackgroundShared )
	{
		RED_ASSERT( m_topLevelHWnd, TXT("Can't set capture mode without a valid HWND") );
		if ( ! m_topLevelHWnd )
		{
			return;
		}

		const HRESULT hr = m_directInputDevice->SetCooperativeLevel( m_topLevelHWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE );
		RED_ASSERT( SUCCEEDED(hr) );

		// Can fail, will try again in Update()
		m_directInputDevice->Acquire();
	}
	else if ( captureMode == IDInputInterface::ECaptureMode::ForegroundExclusive )
	{
		RED_ASSERT( m_topLevelHWnd, TXT("Can't set capture mode without a valid HWND") );
		if ( ! m_topLevelHWnd )
		{
			return;
		}

		const HRESULT hr = m_directInputDevice->SetCooperativeLevel( m_topLevelHWnd, DISCL_FOREGROUND | DISCL_EXCLUSIVE );
		RED_ASSERT( SUCCEEDED(hr) );

		// Can fail, will try again in Update()
		m_directInputDevice->Acquire();
	}

	m_needsReset = true;

	m_captureMode = captureMode;
}

void CInputDeviceKeyboardDInput::SetTopLevelHWnd( HWND topLevelHWnd )
{
	RED_ASSERT( topLevelHWnd );

	if ( m_topLevelHWnd == topLevelHWnd )
	{
		return;
	}

	if ( m_topLevelHWnd )
	{
		const IDInputInterface::ECaptureMode captureMode = m_captureMode;
		SetCaptureMode( IDInputInterface::ECaptureMode::None );
		m_topLevelHWnd = topLevelHWnd;
		SetCaptureMode( captureMode );
	}
	else
	{
		m_topLevelHWnd = topLevelHWnd;
	}
}
