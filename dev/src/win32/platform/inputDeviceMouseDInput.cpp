/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/engine/inputUtils.inl"

#include "inputDeviceMouseDInput.h"

//////////////////////////////////////////////////////////////////////////
// CInputDeviceMouseDInput
//////////////////////////////////////////////////////////////////////////
CInputDeviceMouseDInput::CInputDeviceMouseDInput( IDirectInputDevice8* directInputDevice )
	: m_directInputDevice( directInputDevice )
	, m_topLevelHWnd( nullptr )
	, m_captureMode( IDInputInterface::ECaptureMode::None )
	, m_needsClear( true )
	, m_needsReset( false )
{
	RED_ASSERT( directInputDevice );
	Clear();
}

CInputDeviceMouseDInput::~CInputDeviceMouseDInput()
{
	Cleanup();
}

Bool CInputDeviceMouseDInput::Init()
{
	if ( ! m_directInputDevice )
	{
		return false;
	}

	// Set mouse data format
	if ( FAILED( m_directInputDevice->SetDataFormat( &c_dfDIMouse2 ) ) )
	{
		ERR_ENGINE( TXT("Unable to set DirectInput mouse data format") );
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
	if ( FAILED( m_directInputDevice->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph ) ) )
	{
		ERR_ENGINE( TXT("Unable to set DirectInput mouse buffer size") );
		Cleanup();
		return false;
	}

	return true;
}

void CInputDeviceMouseDInput::Clear()
{
	// Don't actually unacquire devices, since could still be relying on the behavior. E.g., even if we don't use the mouse input in foreground mode
	// we don't want other apps getting it either. We're only clearing the *input* reading.

	Red::MemorySet( m_buttonDownEventResetTable, 0, sizeof(m_buttonDownEventResetTable) );
	Red::MemorySet( m_axisMovementResetTable, 0, sizeof(m_axisMovementResetTable) );

	m_needsClear = false;
	m_needsReset = false;
}

void CInputDeviceMouseDInput::Reset()
{
	m_needsReset = true;
}

void CInputDeviceMouseDInput::Reset( BufferedInput& outBufferedInput )
{
	for ( Uint32 i = 0; i < (Uint32)EButton::Count; ++i )
	{
		if ( m_buttonDownEventResetTable[ i ] )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseButton( EButton(i) ), IACT_Release, 0.f ) );
		}
	}

	for ( Uint32 i = 0; i < (Uint32)EAxis::Count; ++i )
	{
		if( m_axisMovementResetTable[ i ] )
		{
			outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis(i) ), IACT_Axis, 0.f ) );
		}
	}

	Red::MemorySet( m_buttonDownEventResetTable, 0, sizeof(m_buttonDownEventResetTable) );
	Red::MemorySet( m_axisMovementResetTable, 0, sizeof(m_axisMovementResetTable) );

	m_needsReset = false;
	m_needsClear = true;	// Reset > Clear but leaves it in a state that eventually requires clearing
}

void CInputDeviceMouseDInput::Cleanup()
{
	Clear();

	if ( m_directInputDevice )
	{
		m_directInputDevice->Unacquire();
		m_directInputDevice->Release();
		m_directInputDevice = nullptr;
	}
}

void CInputDeviceMouseDInput::Unacquire()
{
	if ( m_directInputDevice )
	{
		m_directInputDevice->Unacquire();
	}
}

void CInputDeviceMouseDInput::Update( BufferedInput& outBufferedInput )
{
	RED_ASSERT( m_directInputDevice );

	if ( m_needsReset )
	{
		Reset( outBufferedInput );
	}

	if ( m_needsClear )
	{
		Clear();
	}

	if ( m_captureMode == IDInputInterface::ECaptureMode::None )
	{
		return;
	}

	// Grab data
	DIMOUSESTATE2 state;
	DIDEVICEOBJECTDATA didod[ DINPUT_BUFFER_SIZE ];
	DWORD dwItems = DINPUT_BUFFER_SIZE;
	HRESULT hr = m_directInputDevice->GetDeviceState( sizeof(state), (LPVOID)&state ); 
	if ( SUCCEEDED(hr) )
	{
		hr = m_directInputDevice->GetDeviceData( sizeof(DIDEVICEOBJECTDATA), didod, &dwItems, 0 );
		if ( hr == DI_BUFFEROVERFLOW )
		{
			WARN_ENGINE(TXT("CInputDeviceMouseDInput: buffer overflow"));
			Reset();
		}
	}

	if ( FAILED( hr ) )
	{
		// Might never have acquired the device even when capture mode set (e.g., window minimized)
		// So treat both errors as the same.
		if ( hr == DIERR_NOTACQUIRED || hr == DIERR_INPUTLOST )
		{
			m_directInputDevice->Acquire();	
		}

		//TBD: optimize when not actually using the device
		// Reset now since we may *never* reacquire the device
		Reset( outBufferedInput );
		return;
	}

	Int32 dxTotal = 0;
	Int32 dyTotal = 0;
	Int32 dzTotal = 0;

	// FIXME2<<< Movement while clicking, sort of lose this info anyway or should have movement events before? Don't see it being correct either way.
	// Mouse packet stuff in viewport updates differently anyway.

	// Process events. DirectInput buffered mouse data deals with *events* not immediate state
	// which could be out of sync if the input buffer overflows: DI_BUFFEROVERFLOW
	for (DWORD dwItem = 0; dwItem < dwItems; ++dwItem )
	{
		const DWORD dwOfs = didod[ dwItem ].dwOfs;
		const DWORD dwData = didod[ dwItem ].dwData;
		switch ( dwOfs )
		{
			case DIMOFS_BUTTON0:
			case DIMOFS_BUTTON1:
			case DIMOFS_BUTTON2:
			case DIMOFS_BUTTON3:
			case DIMOFS_BUTTON4:
			case DIMOFS_BUTTON5:
			case DIMOFS_BUTTON6:
			case DIMOFS_BUTTON7:
				{
					const EButton button = (EButton)(dwOfs - DIMOFS_BUTTON0);
					const Bool isPressed = ( dwData & 0x80 ) != 0;
					if ( isPressed )
					{
						m_buttonDownEventResetTable[ (Uint32)button ] = true;
						outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseButton( button ), IACT_Press, 1.0f ) );
					}
					else
					{
						m_buttonDownEventResetTable[ (Uint32)button ] = false;
						outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseButton( button ), IACT_Release, 0.0f ) );
					}
				}
				break;
			case DIMOFS_X:
				{
					dxTotal += Int32(dwData);
				}
				break;
			case DIMOFS_Y:
				{
					dyTotal += Int32(dwData);		
				}
				break;
			case DIMOFS_Z:
				{
					static const Int32 mouseDelta = 3; // Experimental value, also chosen for editor
					dzTotal += Int32(dwData) < 0 ? -mouseDelta : mouseDelta;
				}
				break;
			default:
				RED_HALT( "CInputDeviceMouseDInput::Update - Unknown dwOfs '%u'", (Uint32)dwOfs );
				break;
		}
	}
	
	if ( dxTotal != 0 || m_axisMovementResetTable[ (Uint32)EAxis::DeltaX ] )
	{
		outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaX ), IACT_Axis, Float(dxTotal) ) );
		m_axisMovementResetTable[ (Uint32)EAxis::DeltaX ] = dxTotal != 0;
	}

	if (dyTotal != 0 || m_axisMovementResetTable[ (Uint32)EAxis::DeltaY ] )
	{
		outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaY ), IACT_Axis, Float(dyTotal) ) );
		m_axisMovementResetTable[ (Uint32)EAxis::DeltaY ] = dyTotal != 0;
	}

	if (dzTotal != 0 || m_axisMovementResetTable[ (Uint32)EAxis::DeltaWheel ] )
	{
		outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaWheel ), IACT_Axis, Float(dzTotal) ) );
		m_axisMovementResetTable[ (Uint32)EAxis::DeltaWheel ] = dzTotal != 0;
	}
}

void CInputDeviceMouseDInput::SetCaptureMode( IDInputInterface::ECaptureMode captureMode )
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

		// TMP TMP TMP TMP TMP HACK
		// BUT WHY???
		/*RED_LOG( Cursor, TXT("CInputDeviceMouseDInput ShowCursor(true)") );
		while ( ::ShowCursor( TRUE ) < 0 )
		{
			continue;
		}*/
	}
	else if ( captureMode == IDInputInterface::ECaptureMode::ForegroundExclusive )
	{
		RED_ASSERT( m_topLevelHWnd, TXT("Can't set capture mode without a valid HWND") );
		if ( ! m_topLevelHWnd )
		{
			return;
		}

		const Uint32 flags = DISCL_FOREGROUND | DISCL_NONEXCLUSIVE;

		const HRESULT hr = m_directInputDevice->SetCooperativeLevel( m_topLevelHWnd, flags );
		RED_ASSERT( SUCCEEDED(hr) );

		// Can fail, will try again in Update()
		m_directInputDevice->Acquire();

		// TMP TMP TMP TMP TMP HACK
		// BUT WHY???
		/*RED_LOG( Cursor, TXT("CInputDeviceMouseDInput ShowCursor(false)") );
		while ( ::ShowCursor( FALSE ) >= 0 )
		{
			continue;
		}*/
	}

	m_needsReset = true;

	m_captureMode = captureMode;
}

void CInputDeviceMouseDInput::SetTopLevelHWnd( HWND topLevelHWnd )
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
