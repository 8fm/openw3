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

#include "DiscoveryChannel.h"
#include "GameSocketAddr.h"
#include "IPConnectorPorts.h"
#include "Network.h"
#include "Serializer.h"
#include "IChannelsHolder.h"

#include <string.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <stdio.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DiscoveryChannel::DiscoveryChannel( IChannelsHolder* in_pHolder )
	: m_eState( StateAvailable )
	, m_pHolder( in_pHolder )
{
	AKPLATFORM::AkMemSet( m_szComputerName, 0, sizeof m_szComputerName );

	AKPLATFORM::AkMemSet( m_szControllerName, 0, sizeof m_szControllerName );
}

DiscoveryChannel::~DiscoveryChannel()
{
}

bool DiscoveryChannel::Init()
{
	AkInt32 stringSize = sizeof m_szComputerName;
	Network::GetMachineName( m_szComputerName, &stringSize );

	m_socket.Create( SOCK_DGRAM, IPPROTO_UDP );
	m_socket.ReuseAddress();

	GameSocketAddr addr( INADDR_ANY, IPConnectorPorts::Current.uDiscoveryBroadcast );

	AkInt32 res = m_socket.Bind( addr );

	if ( res == SOCKET_ERROR )
	{
		char szString[256];
		sprintf( szString, "AK::Comm -> DiscoveryChannel::Init() -> m_socket.Bind() failed, requested port == %d (AkCommSettings::ports.uDiscoveryBroadcast)\n", IPConnectorPorts::Current.uDiscoveryBroadcast );
		AKPLATFORM::OutputDebugMsg( szString );

		return false;
	}

#if defined(AK_IOS) || defined(AK_3DS) // Make sure the TCP ports are ready since the iPhone nor 3DS cannot be discovered when plugged through USB
	bool bRet = m_pHolder->PrepareChannels( "Unknown" );
	return bRet;
#else
	return true;
#endif
}

void DiscoveryChannel::Term()
{
#ifdef AK_IOS //FOR GDC 2010.  Make sure the TCP ports are ready since the iPhone can't be discovered when plugged through USB
	if (m_pHolder)
		m_pHolder->ResetChannels();
#endif

	if( m_socket.IsValid() )
		m_socket.Close();
}

DiscoveryChannel::ResponseState DiscoveryChannel::GetResponseState() const
{
	return m_eState;
}

void DiscoveryChannel::SetResponseState( DiscoveryChannel::ResponseState in_eState )
{
	m_eState = in_eState;
}

void DiscoveryChannel::SetControllerName( const char* in_pszControllerName )
{
	if( in_pszControllerName != NULL )
		::strncpy( m_szControllerName, in_pszControllerName, sizeof m_szControllerName );
	else
		::memset( m_szControllerName, 0, sizeof m_szControllerName );
}

void DiscoveryChannel::Process()
{
	int selVal = m_socket.Poll( GameSocket::PollRead, 0 );

	if( selVal == SOCKET_ERROR )
	{
		// Socket closed by us
#ifdef DEBUG
		if (SOLastError() != 0)
			printf("Discovery socket error %i\n", SOLastError());
#endif
	}
	else if( selVal != 0 )
	{
#if defined AK_3DS
		AkUInt32 uProtocolVersion = AK_COMM_PROTOCOL_VERSION;
		m_socket.Send( &uProtocolVersion, sizeof(uProtocolVersion), 0 );
		m_socket.Shutdown( 0/*Unused*/ );
#else		
		GameSocketAddr hostAddr;

		AkUInt8 recvBuf[512] = { 0 };
		
		// Receiving on UDP returns only the first available datagram.
		int recvVal = m_socket.RecvFrom( recvBuf, sizeof recvBuf, 0, hostAddr );		

		if( recvVal == SOCKET_ERROR )
		{
			// Socket closed by us
		}
		else if( recvVal == 0 )
		{
			// Socket close by the host
		}
		else
		{
			Serializer serializer( !Network::SameEndianAsNetwork() );
			serializer.Deserializing( recvBuf );

			const DiscoveryMessage::Type eType = DiscoveryMessage::PeekType( recvVal, serializer );

			DiscoveryMessage* pResponse = NULL;

			if ( eType == DiscoveryMessage::TypeDiscoveryRequest )
			{
				// Read the Discovery Request
				DiscoveryRequest msg;
				serializer.Get( msg );
				AKASSERT( msg.m_usDiscoveryResponsePort != 0 );

				// Prepare the response
				// MUST BE STATIC because pResponse will point to this structure!
				static DiscoveryResponse response;
				response.m_uiProtocolVersion = AK_COMM_PROTOCOL_VERSION;
				response.m_eConsoleType = CommunicationDefines::g_eConsoleType;
				response.m_eConsoleState = ConvertStateToResponseType();
				response.m_pszConsoleName = m_szComputerName;
				response.m_pszControllerName = NULL;
				if( m_eState == StateBusy )
					response.m_pszControllerName = m_szControllerName;

				hostAddr.SetPort( msg.m_usDiscoveryResponsePort );

				pResponse = &response;
			}
			else if ( eType == DiscoveryMessage::TypeDiscoveryChannelsInitRequest )
			{
				AKASSERT( m_pHolder );

				// Read the Discovery Ports Init Request
				DiscoveryChannelsInitRequest msg;
				serializer.Get( msg );
				AKASSERT( msg.m_usDiscoveryResponsePort != 0 );

				// Prepare the response
				// MUST BE STATIC because pResponse will point to this structure!
				static DiscoveryChannelsInitResponse response;

				IChannelsHolder::Ports ports;

				bool bRet = m_pHolder->PrepareChannels( msg.m_pszControllerName );

				if ( bRet )
				{
					m_pHolder->GetPorts( ports );

					response.m_usCommandPort = ports.m_portCommand;
					response.m_usNotificationPort = ports.m_portNotification;
				}

				hostAddr.SetPort( msg.m_usDiscoveryResponsePort );

				pResponse = &response;
			}

			if ( pResponse )
			{
				serializer.Reset();
				{
					// Calculate the message length and set in our send message.
					serializer.Put( *pResponse );
					pResponse->m_uiMsgLength = serializer.GetWrittenSize();
				}

				serializer.Reset();

				// Serialize the message for good.
				serializer.Put( *pResponse );
				
				m_socket.SendTo( (char*)serializer.GetWrittenBytes(), serializer.GetWrittenSize(), 0, hostAddr );
			}
		}
#endif
	}
}

DiscoveryResponse::ConsoleState DiscoveryChannel::ConvertStateToResponseType() const
{
	DiscoveryResponse::ConsoleState eState = DiscoveryResponse::ConsoleStateAvailable;

	switch( m_eState )
	{
	case StateAvailable:
		eState = DiscoveryResponse::ConsoleStateAvailable;
		break;

	case StateBusy:
		eState = DiscoveryResponse::ConsoleStateBusy;
		break;

	default:
		break;
		// Something went wrong
	}

	return eState;
}
