/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#ifndef RED_FINAL_BUILD

#include "inputDeviceMouseOrbisDebug.h"

#include <mouse.h>

#include "../../common/engine/inputUtils.inl"

//////////////////////////////////////////////////////////////////////////
// CInputDevicMouseOrbisDebug
//////////////////////////////////////////////////////////////////////////
CInputDeviceMouseOrbisDebug::CInputDeviceMouseOrbisDebug( SceUserServiceUserId userId )
:	m_platformData( userId )
{
}

CInputDeviceMouseOrbisDebug::~CInputDeviceMouseOrbisDebug()
{
	if ( m_platformData.IsPortHandleValid() )
	{
		::sceMouseClose( m_platformData.m_portHandle );
		m_platformData.m_portHandle = SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE;
	}
}

Bool CInputDeviceMouseOrbisDebug::Init()
{
	const Int32 portHandle = ::sceMouseOpen( m_platformData.m_userId, SCE_MOUSE_PORT_TYPE_STANDARD, 0, nullptr );
	if ( portHandle <= SInputDevicePlatformDataOrbis::INVALID_PORT_HANDLE )
	{
		ERR_ENGINE( TXT("Failed to open mouse for user %d. Internal error code 0x%08x"), static_cast< Int32 >( m_platformData.m_userId ), portHandle );
		return false;
	}

	m_platformData.m_portHandle = portHandle;

	m_wasLeftDown = false;
	m_wasMiddleDown = false;
	m_wasRightDown = false;

	return true;
}

void CInputDeviceMouseOrbisDebug::Reset()
{
	// Do nothing
}

void CInputDeviceMouseOrbisDebug::Update( BufferedInput& outBufferedInput )
{
	SceMouseData mouseData[ 8 ];
	Red::System::MemorySet( &mouseData, 0, sizeof( SceMouseData ) );
	int ret;

	ret = sceMouseRead( m_platformData.m_portHandle, mouseData, sizeof( mouseData ) / sizeof( SceMouseData ) );

	// Create input events
	if( ret > 0 )
	{
		for( int i = 0 ; i < ret ; ++i )
		{
			if ( mouseData[ i ].xAxis != 0 )
			{
				outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaX ), IACT_Axis, Float( mouseData[ i ].xAxis ) ) );
			}
			if ( mouseData[ i ].yAxis != 0 )
			{
				outBufferedInput.PushBack( SBufferedInputEvent( InputUtils::MapMouseAxis( EAxis::DeltaY ), IACT_Axis, Float( mouseData[ i ].yAxis ) ) );
			}

			if ( mouseData[ i ].buttons & SCE_MOUSE_BUTTON_PRIMARY )
			{
				outBufferedInput.PushBack( SBufferedInputEvent( IK_LeftMouse, IACT_Press,1.0f ) );
				m_wasLeftDown = true;
			}
			else
			{
				if( m_wasLeftDown )
				{
					outBufferedInput.PushBack( SBufferedInputEvent( IK_LeftMouse,IACT_Release, 0.f ) );

					m_wasLeftDown = false;
				}
			}
			if ( mouseData[ i ].buttons & SCE_MOUSE_BUTTON_OPTIONAL )
			{
				m_wasMiddleDown = true;
				outBufferedInput.PushBack( SBufferedInputEvent( IK_MiddleMouse, IACT_Press, 1.0f ) );
			}
			else
			{
				if( m_wasMiddleDown )
				{
					outBufferedInput.PushBack( SBufferedInputEvent( IK_MiddleMouse,IACT_Release, 0.f ) );
					m_wasMiddleDown = false;
				}
			}
			if ( mouseData[ i ].buttons & SCE_MOUSE_BUTTON_SECONDARY )
			{
				m_wasRightDown = true;
				outBufferedInput.PushBack( SBufferedInputEvent( IK_RightMouse, IACT_Press, 1.0f ) );
			}
			else
			{
				if( m_wasRightDown )
				{
					outBufferedInput.PushBack( SBufferedInputEvent( IK_RightMouse,IACT_Release, 0.f ) );
					m_wasRightDown = false;
				}
			}
		}
	}
}

SInputDevicePlatformData* CInputDeviceMouseOrbisDebug::GetPlatformData()
{
	return &m_platformData;
}

#endif // ! RED_FINAL_BUILD
