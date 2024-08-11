/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "inputDeviceManagerOrbis.h"
#include "inputDeviceGamepadOrbis.h"
#include "gestureRecognizerOrbis.h"

#include <libsysmodule.h>

#ifdef RED_ORBIS_MOUSE_KEYBOARD
#	include "inputDeviceKeyboardOrbisDebug.h"
#	include "inputDeviceMouseOrbisDebug.h"
#	include <dbg_keyboard.h>
#	include <mouse.h>
#	pragma comment( lib, "libSceDbgKeyboard_stub_weak.a" )
#	pragma comment( lib, "libSceMouse_stub_weak.a")
#endif

#ifdef RED_ORBIS_SYSTEM_GESTURE
# include <system_gesture.h>
# pragma comment( lib, "libSceSystemGesture_stub_weak.a")
#endif

#include <pad.h>

#define ORBIS_SINGLE_PLAYER_ONLY

#ifdef ORBIS_SINGLE_PLAYER_ONLY
#	define ORBIS_DEVICE_LIST_RESERVE_SIZE 1
#else
#	define ORBIS_DEVICE_LIST_RESERVE_SIZE SCE_USER_SERVICE_MAX_LOGIN_USERS
#endif

#ifdef RED_ORBIS_MOUSE_KEYBOARD
#	define ORBIS_DEVICE_LIST_RESERVE_SIZE_MODIFICATION 2
#else
#	define ORBIS_DEVICE_LIST_RESERVE_SIZE_MODIFICATION 0
#endif


//////////////////////////////////////////////////////////////////////////
// CInputDeviceManagerOrbis
//////////////////////////////////////////////////////////////////////////
CInputDeviceManagerOrbis::CInputDeviceManagerOrbis()
:	m_promiscuousMode( false )
,	m_enableUserSelection( true )
,	m_activeUser( SCE_USER_SERVICE_USER_ID_INVALID )
{
}

//////////////////////////////////////////////////////////////////////////
CInputDeviceManagerOrbis::~CInputDeviceManagerOrbis()
{
}

//////////////////////////////////////////////////////////////////////////
Bool CInputDeviceManagerOrbis::Init()
{
#ifndef ORBIS_SINGLE_PLAYER_ONLY
	RED_HALT( TXT( "Please be aware that multi-user controller support for orbis should be mostly functional but unmaintained and may not function as expected" ) );
#endif

	if ( !InitPadLibrary() )
	{
		return false;
	}

	m_inputDeviceList.Reserve( ORBIS_DEVICE_LIST_RESERVE_SIZE + ORBIS_DEVICE_LIST_RESERVE_SIZE_MODIFICATION );

#ifdef RED_ORBIS_MOUSE_KEYBOARD
	InitKeyboardLibrary();
	InitMouseLibrary();
#endif

	if ( !InitSystemGestureLibrary() )
	{
		return false;
	}

#ifdef ORBIS_SINGLE_PLAYER_ONLY
	// Get initial user
	SceUserServiceUserId userId;
	ORBIS_SYS_CALL( sceUserServiceGetInitialUser( &userId ) );

	if( !SetActiveUser( userId, true ) )
	{
		return false;
	}
#endif

	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CInputDeviceManagerOrbis::InitPadLibrary()
{
	const Int32 sceErr = ::scePadInit();

	if ( sceErr != SCE_OK )
	{
		WARN_ENGINE( TXT("Failed to init PS4 pad library. Internal error code 0x%08X"), sceErr );
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CInputDeviceManagerOrbis::Shutdown()
{
	ClearDeviceList();
}

template< typename TDevice >
TDevice* CInputDeviceManagerOrbis::CreateDevice( SceUserServiceUserId userId )
{
	// Create new gamepad/keyboard/mouse
	TDevice* device = new TDevice( userId );

	if ( !device->Init() )
	{
		delete device;

		return nullptr;
	}

	m_inputDeviceList.PushBack( device );

	return device;
}

Bool CInputDeviceManagerOrbis::CreateDevices( SceUserServiceUserId userId, Bool kbm )
{
	CInputDeviceGamepadOrbis* gamepad = nullptr;
	if( !( gamepad = CreateDevice< CInputDeviceGamepadOrbis >( userId ) ) )
	{
		ERR_ENGINE( TXT( "Failed to create game pad object" ) );

		return false;
	}

	m_inputDeviceGamepad = gamepad;

	gamepad->RegisterListener( &CInputDeviceManagerOrbis::OnPadEvent, this );

#ifdef RED_ORBIS_MOUSE_KEYBOARD
	if( kbm )
	{
		if( !CreateDevice< CInputDeviceKeyboardOrbisDebug >( userId ) )
		{
			WARN_ENGINE( TXT( "Failed to create debug keyboard object" ) );
		}

		if( !CreateDevice< CInputDeviceMouseOrbisDebug >( userId ) )
		{
			WARN_ENGINE( TXT( "Failed to create debug mouse object" ) );
		}
	}
#else
	RED_UNUSED( kbm );
#endif //RED_ORBIS_MOUSE_KEYBOARD

	return true;
}

//////////////////////////////////////////////////////////////////////////
RED_INLINE void CInputDeviceManagerOrbis::UpdateDevice( BufferedInput& outBufferedInput, Uint32 i )
{
	IInputDevice* inputDevice = m_inputDeviceList[ i ];
	inputDevice->Update( outBufferedInput );
	m_deviceCommandsManager.AddCurrentDevice( inputDevice );
}

Bool CInputDeviceManagerOrbis::PromiscuousModeUpdate( BufferedInput& outBufferedInput, Uint32 i )
{
	if( m_enableUserSelection )
	{
		SceUserServiceUserId activeUser = SCE_USER_SERVICE_USER_ID_INVALID;
		IInputDevice* inputDevice = m_inputDeviceList[ i ];

		SInputDevicePlatformDataOrbis* platformData = static_cast< SInputDevicePlatformDataOrbis* >( inputDevice->GetPlatformData() );

		// Check to see if there was any input from the device we just polled
		for( Uint32 iBuffer = 0; iBuffer < outBufferedInput.Size(); ++iBuffer )
		{
			const SBufferedInputEvent& event = outBufferedInput[ iBuffer ];

			// There was! Select our player and break out of the input examination
			if( event.action == IACT_Press )
			{
				activeUser = platformData->m_userId;
				break;
			}
		}

		if( activeUser != SCE_USER_SERVICE_USER_ID_INVALID )
		{
			// We have an active user, proceed with signing in 
			SetActiveUser( activeUser, true );
			m_onUserSelected( activeUser );
			return true;
		}
	}
	else
	{
		// The user selection process has been prevented from listening for input (probably a dialog box)
		// So allow the input to be passed on to the game where applicable
		if( outBufferedInput.Size() > 0 )
		{
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
void CInputDeviceManagerOrbis::Update( BufferedInput& outBufferedInput )
{
	m_deviceCommandsManager.ResetCurrentDevices();

	if( !m_promiscuousMode )
	{
		for ( Uint32 i = 0; i < m_inputDeviceList.Size(); ++i )
		{
			UpdateDevice( outBufferedInput, i );
		}
	}
	else
	{
		for ( Uint32 iDevice = 0; iDevice < m_inputDeviceList.Size(); ++iDevice )
		{
			UpdateDevice( outBufferedInput, iDevice );

			if( PromiscuousModeUpdate( outBufferedInput, iDevice ) )
			{
				break;
			}
		}
	}

	m_deviceCommandsManager.Update();
}

//////////////////////////////////////////////////////////////////////////
void CInputDeviceManagerOrbis::RequestReset()
{
	// Do nothing
}

//////////////////////////////////////////////////////////////////////////
void CInputDeviceManagerOrbis::ClearDeviceList()
{
	m_inputDeviceList.ClearPtr();
}

Bool CInputDeviceManagerOrbis::SetActiveUserPromiscuous( const TOrbisUserEvent& callback )
{
#ifdef ORBIS_SINGLE_PLAYER_ONLY
	if( m_inputDeviceList.Size() == 0 )
#endif
	{
		SceUserServiceLoginUserIdList userList;
		ORBIS_SYS_CALL_RET( sceUserServiceGetLoginUserIdList( &userList ) );

		// Remove all existing controllers
		ClearDeviceList();

		for( Uint32 i = 0; i < SCE_USER_SERVICE_MAX_LOGIN_USERS; ++i )
		{
			SceUserServiceUserId userId = userList.userId[ i ];

			if( userId != SCE_USER_SERVICE_USER_ID_INVALID )
			{
				// Only add mouse and keyboard for the first logged in user
				CreateDevices( userId, m_inputDeviceList.Size() == 1 );
			}
		}
	}

	// Set promiscuous mode on
	m_promiscuousMode = true;
	m_activeUser = SCE_USER_SERVICE_USER_ID_INVALID;
	m_onUserSelected = callback;

	return true;
}

Bool CInputDeviceManagerOrbis::SetActiveUser( SceUserServiceUserId userId, Bool kbm )
{
	// Remove all existing controllers
	ClearDeviceList();

	CreateDevices( userId, kbm );

	m_promiscuousMode = false;
	m_activeUser = userId;

	return true;
}

#ifdef RED_ORBIS_MOUSE_KEYBOARD
//////////////////////////////////////////////////////////////////////////
Bool CInputDeviceManagerOrbis::InitKeyboardLibrary()
{
	Int32 sceErr = SCE_OK;

	sceErr = ::sceSysmoduleLoadModule( SCE_SYSMODULE_DEBUG_KEYBOARD );
	if ( sceErr != SCE_OK )
	{
		WARN_ENGINE( TXT("Failed to load keyboard library module. Internal error code 0x%08X"), sceErr );
		return false;
	}

	sceErr = ::sceDbgKeyboardInit();
	if ( sceErr != SCE_OK )
	{
		WARN_ENGINE( TXT("Failed to initialize debug keyboard library. Internal error code 0x%08X"), sceErr );
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
Bool CInputDeviceManagerOrbis::InitMouseLibrary()
{
	Int32 sceErr = SCE_OK;

	sceErr = ::sceSysmoduleLoadModule( SCE_SYSMODULE_MOUSE );
	if ( sceErr != SCE_OK )
	{
		WARN_ENGINE( TXT("Failed to load mouse library module. Internal error code 0x%08X"), sceErr );
		return false;
	}

	sceErr = ::sceMouseInit();
	if ( sceErr != SCE_OK )
	{
		WARN_ENGINE( TXT("Failed to initialize mouse library. Internal error code 0x%08X"), sceErr );
		return false;
	}

	return true;
}

#endif // RED_ORBIS_MOUSE_KEYBOARD

Bool CInputDeviceManagerOrbis::InitSystemGestureLibrary()
{
#ifdef RED_ORBIS_SYSTEM_GESTURE
	Int32 sceErr = SCE_OK;

	sceErr = ::sceSysmoduleLoadModule( SCE_SYSMODULE_SYSTEM_GESTURE );
	if ( sceErr != SCE_OK )
	{
		WARN_ENGINE( TXT("Failed to load system gesture library module. Internal error code 0x%08X"), sceErr );
		return false;
	}

	sceErr = ::sceSystemGestureInitializePrimitiveTouchRecognizer( nullptr );
	RED_FATAL_ASSERT( sceErr == SCE_OK, "sceSystemGestureInitializePrimitiveTouchRecognizer documented to always return SCE_OK but returned 0x%08X", sceErr );
	if ( sceErr != SCE_OK )
	{
		return false;
	}
#endif // RED_ORBIS_SYSTEM_GESTURE

	return true;
}

void CInputDeviceManagerOrbis::OnPadEvent( const SOrbisGamepadEvent& event )
{
	RED_FATAL_ASSERT( event.m_userId != SCE_USER_SERVICE_USER_ID_INVALID, "Invalid user id passed through to disconnection event" );

	if( event.m_userId == m_activeUser )
	{
		if( event.m_type == EOrbisGamepadEventType::OGE_Disconnected )
		{
			Send( EControllerEventType::CET_Disconnected );
		}
		else if( event.m_type == EOrbisGamepadEventType::OGE_Reconnected )
		{
			Send( EControllerEventType::CET_Reconnected );
		}
	}

	RED_LOG( CInputDeviceManagerOrbis, TXT( "PAD %hs FOR USER %x" ), ( event.m_type == EOrbisGamepadEventType::OGE_Disconnected )? "DISCONNECTED" : "RECONNECTED", event.m_userId );
}

const CName CInputDeviceManagerOrbis::GetLastUsedDeviceName() const 
{
	return m_inputDeviceGamepad->GetDeviceName();
}
