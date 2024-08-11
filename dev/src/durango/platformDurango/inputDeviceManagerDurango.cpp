/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "inputDeviceManagerDurango.h"
#include "inputDeviceGamepadDurango.h"

#if !defined( RED_FINAL_BUILD )
# include "inputDeviceKeyboardDurangoDebug.h"
# include "inputDeviceMouseDurangoDebug.h"
#endif

//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerDurango
//////////////////////////////////////////////////////////////////////////
CInputDeviceManagerDurango::CInputDeviceManagerDurango()
:	m_inputDeviceGamepadDurango( nullptr )
,	m_queuedPad( nullptr )
,	m_activeGamepadId( 0 )
,	m_activePadDisconnected( false )
,	m_padChangeQueued( false )
{
}

CInputDeviceManagerDurango::~CInputDeviceManagerDurango()
{
}

Bool CInputDeviceManagerDurango::Init()
{
	m_inputDeviceGamepadDurango = new CInputDeviceGamepadDurango;

	m_inputDeviceList.PushBack( m_inputDeviceGamepadDurango );

#if !defined( RED_FINAL_BUILD )
	m_inputDeviceList.PushBack( new CInputDeviceKeyboardDurangoDebug );
	m_inputDeviceList.PushBack( new CInputDeviceMouseDurangoDebug );
#endif

	return true;
}

void CInputDeviceManagerDurango::Shutdown()
{
	m_inputDeviceList.ClearPtrFast();
	m_inputDeviceGamepadDurango = nullptr;
}

void CInputDeviceManagerDurango::Update( TDynArray< SBufferedInputEvent >& outBufferedInput )
{
	m_deviceCommandsManager.ResetCurrentDevices();

	if( m_activePadDisconnected )
	{
		RED_LOG( Controllers, TXT( "CInputDeviceManagerDurango::Update() Sending disconnect event!" ) );
		Send( EControllerEventType::CET_Disconnected );

		m_activePadDisconnected = false;
	}

	UpdateProfileGamepadChange();

	for ( Uint32 i = 0; i < m_inputDeviceList.Size(); ++i )
	{
		IInputDevice* inputDevice = m_inputDeviceList[ i ];
		inputDevice->Update( outBufferedInput );
		m_deviceCommandsManager.AddCurrentDevice( inputDevice );
	}

	m_deviceCommandsManager.Update();
}

void CInputDeviceManagerDurango::OnUserProfileGamepadChanged( IGamepad^ gamepad )
{
	m_queuedPad = gamepad;
	m_padChangeQueued = true;
}

void CInputDeviceManagerDurango::UpdateProfileGamepadChange()
{
	if ( m_padChangeQueued && m_inputDeviceGamepadDurango )
	{
		if( m_queuedPad )
		{
			if( m_activeGamepadId != m_queuedPad->Id )
			{
				m_activeGamepadId = m_queuedPad->Id;

				m_inputDeviceGamepadDurango->SetGamepad( m_queuedPad );

				m_queuedPad = nullptr;

				RED_LOG( Controllers, TXT( "CInputDeviceManagerDurango::OnUserProfileGamepadChanged() Sending reconnected event!" ) );
				Send( EControllerEventType::CET_Reconnected );
			}
		}
		else
		{
			if( m_activeGamepadId != 0 )
			{
				m_activePadDisconnected = true;
				RED_LOG( Controllers, TXT( "CInputDeviceManagerDurango::OnUserProfileGamepadChanged() m_activePadDisconnected = true" ) );
			}

			m_activeGamepadId = 0;

			m_inputDeviceGamepadDurango->SetGamepad( m_queuedPad );
		}

		m_padChangeQueued = false;
	}
}

void CInputDeviceManagerDurango::RequestReset()
{
	// Do nothing
}

Windows::Xbox::Input::IGamepad^ CInputDeviceManagerDurango::GetActiveGamepad() const
{
	return m_inputDeviceGamepadDurango->GetGamepad();
}

const CName CInputDeviceManagerDurango::GetLastUsedDeviceName() const 
{
	return m_inputDeviceGamepadDurango->GetDeviceName();
}
