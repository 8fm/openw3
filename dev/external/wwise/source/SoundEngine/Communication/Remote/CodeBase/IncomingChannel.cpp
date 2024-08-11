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

#include "IncomingChannel.h"
#include "GameSocketAddr.h"
#include <stdio.h>

#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>

#define INITIAL_BUFFER_SIZE 512

using namespace AKPLATFORM;

IncomingChannel::IncomingChannel( unsigned short in_listenPort, AkMemPoolId in_pool, const AkThreadProperties& in_threadProperties )
	: m_requestedPort( in_listenPort )
	, m_pool( in_pool )
	, m_bErrorProcessingConnection( false )
	, m_serializer( false )
	, m_pRecvBuf( NULL )
	, m_iBufSize( 0 )
#ifdef AK_3DS
	, m_pThread(NULL)
	, m_threadProperties(in_threadProperties)
#endif
{
}

IncomingChannel::~IncomingChannel()
{
}

bool IncomingChannel::Init()
{
	return true;
}

void IncomingChannel::Term()
{
	Reset();

	if ( m_pRecvBuf )
	{
		AkFree( m_pool, m_pRecvBuf );
		m_pRecvBuf = NULL;
	}
}

bool IncomingChannel::StartListening()
{
	m_serverSocket.Create( SOCK_STREAM, IPPROTO_TCP, RequiresAccumulator() );
	m_serverSocket.ReuseAddress();

	GameSocketAddr localAddr( INADDR_ANY, m_requestedPort );

	AkInt32 res = m_serverSocket.Bind( localAddr );

	if ( res == SOCKET_ERROR )
	{
		char szString[256];
		sprintf( szString, "AK::Comm -> IncomingChannel::StartListening() -> m_serverSocket.Bind() failed, requested port == %d (%s)\n", m_requestedPort, GetRequestedPortName() );
		AKPLATFORM::OutputDebugMsg( szString );

		return false;
	}

	if ( m_pRecvBuf == NULL )
	{
		m_pRecvBuf = (AkUInt8*)AkAlloc( m_pool, INITIAL_BUFFER_SIZE ); 
		m_iBufSize = INITIAL_BUFFER_SIZE;
	}

	AKASSERT( ( m_requestedPort == 0 ) || ( m_serverSocket.GetPort() == m_requestedPort ) );

#ifdef AK_3DS
	return m_serverSocket.Listen( 1 ) != SOCKET_ERROR;
#else
	return m_serverSocket.Listen( 5 ) != SOCKET_ERROR;
#endif
}

void IncomingChannel::StopListening()
{
#ifndef AK_3DS
	m_serverSocket.Shutdown( SD_BOTH );
	m_serverSocket.Close();
#endif
}

void IncomingChannel::Process()
{
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
				PeerConnected( hostAddr );

#ifdef AK_3DS
				AKASSERT(m_pThread == NULL);
				m_pThread = (AkThread*)AkMalign(m_pool, sizeof(AkThread), AK_OS_STRUCT_ALIGN);
				if (m_pThread)
				{
					AkPlacementNew( m_pThread ) AkThread();
					AkClearThread( m_pThread );
					AkCreateThread( ProcessConnectionThreadFunc, this, m_threadProperties, m_pThread, "AK::IncomingChannel" );
				}
#endif
			}
		}
	}

#ifndef AK_3DS
	while ( m_connSocket.IsValid() && !m_bErrorProcessingConnection )
	{
		int selVal = m_connSocket.Poll( GameSocket::PollRead, 0 );
		if ( selVal == SOCKET_ERROR )
		{
			m_bErrorProcessingConnection = true;
		}
		else if ( selVal == 0 )
		{
			// nothing ready.
			break;
		}
		else
		{
			ReceiveCommand();
		}
	}
#endif

	if( m_bErrorProcessingConnection )
	{
		PeerDisconnected();
		m_bErrorProcessingConnection = false;
	}
}

bool IncomingChannel::IsReady() const
{
	return m_connSocket.IsValid();
}

AkUInt16 IncomingChannel::GetPort() const
{
	if ( m_requestedPort != 0 )
		return m_requestedPort;

	return m_serverSocket.GetPort();
}

void IncomingChannel::Send( const AkUInt8* in_pData, int in_dataLength )
{
	// When serializing, the serializer puts the length of the data in front of the data
	// so it already fits our protocol { msg lenght | msg } pattern.
	Serializer serializer( false );
	serializer.Put( in_pData, in_dataLength );

	if (m_connSocket.Send( (const char*)serializer.GetWrittenBytes(), serializer.GetWrittenSize(), 0 ) == SOCKET_ERROR)
	{
		//Lost the connection.
		m_bErrorProcessingConnection = true;
	}
}

void IncomingChannel::Reset()
{
	// It doesn't matter if the socket is INVALID_SOCKET or already unusable.
	m_serverSocket.Shutdown( SD_BOTH );
	m_serverSocket.Close();
	m_connSocket.Shutdown( SD_BOTH );
	m_connSocket.Close();

#ifdef AK_3DS
	m_csReset.Lock();
	if( m_pThread && AkIsValidThread( m_pThread ) )
	{
		AkWaitForSingleThread( m_pThread );
		AkCloseThread( m_pThread );
		AkFalign(m_pool, m_pThread);
		m_pThread = NULL;
	}
	m_csReset.Unlock();
#endif
}

void IncomingChannel::ReceiveCommand()
{
	AkUInt32 msgLen = 0;
	m_serializer.Reset();

	// Receive the message length
	int recvVal = m_connSocket.Recv( (char*)m_pRecvBuf, sizeof( AkUInt32 ), 0 );
	if( recvVal <= 0 )
	{
		m_bErrorProcessingConnection = true;
	}
	else
	{
		m_serializer.Deserializing( m_pRecvBuf );
		m_serializer.Get( msgLen );

		if ( msgLen > m_iBufSize || m_pRecvBuf == NULL )
		{
			// grow buffer, if we can!
			AkFree( m_pool, m_pRecvBuf );
			m_pRecvBuf = (AkUInt8*)AkAlloc( m_pool, msgLen );
			if (m_pRecvBuf == NULL)
			{
				//Not enough memory.  Skip this message.  The memory error will be logged in the profiler.
				m_iBufSize = INITIAL_BUFFER_SIZE;
				m_pRecvBuf = (AkUInt8*)AkAlloc( m_pool, INITIAL_BUFFER_SIZE );	//Reset the buffer.  It will succeed because the block was just freed.
				AKASSERT(m_pRecvBuf);
				while(msgLen > 0)
				{
					recvVal = m_connSocket.Recv( m_pRecvBuf, AkMin(INITIAL_BUFFER_SIZE,msgLen), 0 );
					if ( recvVal <= 0 )
					{
						m_bErrorProcessingConnection = true;
						break;
					}

					msgLen -= recvVal;
				}

				return;
			}
			else
			{
				m_iBufSize = msgLen;
			}				
		}

		// Receive the message itself.
		recvVal = m_connSocket.Recv( (char*)m_pRecvBuf, msgLen, 0 );
		if ( recvVal <= 0 )
		{
			m_bErrorProcessingConnection = true;
		}
		else
		{
			ProcessCommand( m_pRecvBuf, msgLen );
		}
	}
}

#ifdef AK_3DS
AK_DECLARE_THREAD_ROUTINE( IncomingChannel::ProcessConnectionThreadFunc )
{
	IncomingChannel& rThis = *AK_GET_THREAD_ROUTINE_PARAMETER_PTR( IncomingChannel );

	do
	{
		rThis.ReceiveCommand();
	} 
	while ( !rThis.m_bErrorProcessingConnection );

	AkExitThread( AK_RETURN_THREAD_OK );
}
#endif
