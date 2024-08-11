/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "build.h"

#define BUTTONS_NUM 16

// HACK until a proper user profile manager
#include <user_service.h>
#include <pad.h>

class CInputInitializerOrbis
{
public:
	Bool Init()
	{
		m_buttons.Reserve( BUTTONS_NUM );
		m_buttonsPrev.Reserve( BUTTONS_NUM );

		SceUserServiceUserId userId = TmpHackGetUserId();
		if ( userId < 0 )
		{
			return false;
		}

		if ( ! InitPadLibrary() )
		{
			return false;
		}

		const Int32 portHandle = ::scePadOpen( userId, SCE_PAD_PORT_TYPE_STANDARD, 0, nullptr );
		if ( portHandle <= -1 )
		{
			WARN_GAT( TXT("Failed to open DualShock4 port for user ID %d. Internal error code 0x%08x"), static_cast< Int32 >( userId ), portHandle );
			return false;
		}

		m_portHandle = portHandle;

		return true;
	}

	Bool ReadInput()
	{
		ScePadData data;

		if( ::scePadReadState( m_portHandle, &data ) != SCE_OK )
		{
			WARN_GAT( TXT("Failed to read input.") );
			return false;
		}

		for( Uint32 i = 2; i < SCE_PAD_BUTTON_INTERCEPTED; i <<= 1 )
		{
			Bool isPressed = data.buttons & i;
			m_buttons[i] = isPressed && !m_buttonsPrev[i];
			m_buttonsPrev[i] = isPressed;
		}

		return true;
	}

	Bool ButtonPressed( ScePadButtonDataOffset button )
	{
		return m_buttons[button];
	}

	void Shutdown()
	{
		if ( m_portHandle != -1 )
		{
			::scePadClose( m_portHandle );
			m_portHandle = -1;
		}
	}

private:
	Int32 m_portHandle;

	THashMap< Uint32, Bool > m_buttons;
	THashMap< Uint32, Bool > m_buttonsPrev;

	Bool InitPadLibrary()
	{
		const Int32 sceErr = ::scePadInit();

		if ( sceErr != SCE_OK )
		{
			WARN_GAT( TXT("Failed to init PS4 pad library. Internal error code 0x%08X"), sceErr );
			return false;
		}

		return true;
	}

	SceUserServiceUserId TmpHackGetUserId()
	{
		Int32 sceErr = SCE_OK;

		sceErr = ::sceUserServiceInitialize( nullptr ); // call sceUserServiceTerminate?
		if ( sceErr != SCE_OK )
		{
			WARN_GAT( TXT("sceUserServiceInitialize failed with error code 0x%08X"), sceErr );
			return -1;
		}

		SceUserServiceUserId userId = -1;
		sceErr = ::sceUserServiceGetInitialUser( &userId );
		if ( sceErr != SCE_OK )
		{
			WARN_GAT( TXT("sceUserServiceGetInitialUser failed with error code 0x%08X"), sceErr );
			return -1;
		}

		return userId;
	}
};
