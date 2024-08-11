/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


#include "stdafx.h"

#include <AK/SoundEngine/Common/AkTypes.h>

#include "NotificationChannel.h"
#include "GameSocketAddr.h"
#include "IPConnectorPorts.h"
#include "NetworkTypes.h"
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

NotificationChannel::NotificationChannel(IChannelsHolder *in_pHolder)
	: m_pHolder(in_pHolder)
	, m_bErrorProcessingConnection( false )
	, m_serializer( false )
{
}

NotificationChannel::~NotificationChannel()
{
}

void NotificationChannel::SendNotification( const AkUInt8* in_pNotificationData, int in_dataSize, bool in_bAccumulate ) const
{
	if( m_connSocket.IsValid() && !m_bErrorProcessingConnection )
	{
		if ( in_bAccumulate )
		{
			AkInt32 iPrevSize = m_serializer.GetWrittenSize();
			bool bOK = m_serializer.Put( in_pNotificationData, in_dataSize );
			if ( !bOK )
			{
				// If there is no space left in accumulation buffer, send all previously accumulated data and try again.
				m_serializer.SetWrittenSize( iPrevSize );
				SendAccumulatedData();
				bOK = m_serializer.Put( in_pNotificationData, in_dataSize );
				if ( !bOK )
					m_serializer.Reset(); // Erase all traces of this notification, drop it.
			}
		}
		else
		{
			m_serializer.Reset(); // Reset each use only if not in accumulator mode.

			bool bOK = m_serializer.Put( in_pNotificationData, in_dataSize );
			if( bOK )
			{
				if (m_connSocket.Send( (const char*)m_serializer.GetWrittenBytes(), m_serializer.GetWrittenSize(), 0 ) == SOCKET_ERROR)
					m_bErrorProcessingConnection = true;
			}
			else
			{
				/// Skipping a notification, Maybe it could be a good idea to disconnect...
			}
		}
	}
}

void NotificationChannel::SendAccumulatedData() const
{
	if( m_connSocket.IsValid() && !m_bErrorProcessingConnection )
	{
#if defined(AK_3DS)
		if( m_serializer.GetWrittenSize() < 1000 )
		{
			return;// Wait to accumulate more before sending.
		}
#endif
		if (m_connSocket.Send( (const char*)m_serializer.GetWrittenBytes(), m_serializer.GetWrittenSize(), 0 ) == SOCKET_ERROR)
		{
			m_bErrorProcessingConnection = true;
		}
	}
	m_serializer.Reset();
}

bool NotificationChannel::Init()
{
	return true;
}

void NotificationChannel::Term()
{
	if( m_serverSocket.IsValid() )
	{
		m_serverSocket.Shutdown( SD_BOTH );
		m_serverSocket.Close();
	}

	if( m_connSocket.IsValid() )
	{
		m_connSocket.Shutdown( SD_BOTH );
		m_connSocket.Close();
	}
}

bool NotificationChannel::StartListening()
{
	m_serverSocket.Create( SOCK_STREAM, IPPROTO_TCP );
	m_serverSocket.ReuseAddress();

	GameSocketAddr localAddr( INADDR_ANY, IPConnectorPorts::Current.uNotification );

	AkInt32 res = m_serverSocket.Bind( localAddr );

	if ( res == SOCKET_ERROR )
	{
		char szString[256];
		sprintf( szString, "AK::Comm -> NotificationChannel::StartListening() -> m_serverSocket.Bind() failed, requested port == %d (AkCommSettings::ports.uNotification)\n", IPConnectorPorts::Current.uNotification );
		AKPLATFORM::OutputDebugMsg( szString );

		return false;
	}

	return m_serverSocket.Listen( 1 ) != SOCKET_ERROR;
}

void NotificationChannel::StopListening()
{
#ifndef AK_3DS
	m_serverSocket.Shutdown( SD_BOTH );
	m_serverSocket.Close();
#endif
}

void NotificationChannel::Process()
{
	if ( m_bErrorProcessingConnection )
	{
		m_pHolder->PeerDisconnected();
		m_bErrorProcessingConnection = false;
	}

	if( m_serverSocket.IsValid() )
	{
		int selVal = m_serverSocket.Poll( GameSocket::PollRead, 0 );

		if( selVal == SOCKET_ERROR )
		{
			// Socket closed by us
		}
		else if( selVal != 0 )
		{
			GameSocketAddr hostAddr;

			m_serverSocket.Accept( hostAddr, m_connSocket );

			if( m_connSocket.IsValid() )
			{
				StopListening();
				m_connSocket.NoDelay();
			}
		}
	}
}

bool NotificationChannel::IsReady() const
{
	return m_connSocket.IsValid();
}


AkUInt16 NotificationChannel::GetServerPort() const
{
	return m_serverSocket.GetPort();
}
