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

#include "CommunicationCentral.h"
#include "Network.h"
#include "GameSocket.h"
#include "ICommunicationCentralNotifyHandler.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CommunicationCentral::CommunicationCentral( AkMemPoolId in_pool, const AkThreadProperties& in_threadProperties )
	: m_discoveryChannel( this )
	, m_commandChannel( this, in_pool, in_threadProperties )
	, m_notifChannel(this)
	, m_bInitialized( false )
	, m_pNotifyHandler( NULL )
	, m_pool( in_pool )
	, m_bInternalNetworkInit(false)
{
}

CommunicationCentral::~CommunicationCentral()
{
}

void CommunicationCentral::Destroy()
{
	AkDelete( m_pool, this );
}

bool CommunicationCentral::Init( AK::Comm::ICommunicationCentralNotifyHandler* in_pNotifyHandler, AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler, bool in_bInitSystemLib )
{
	AKASSERT( ! m_bInitialized );

	AKRESULT netResult = Network::Init( m_pool, in_bInitSystemLib );
	if (netResult != AK_Success && netResult != AK_PartialSuccess)
		return false;

	m_bInternalNetworkInit = (netResult == AK_Success);

	m_pNotifyHandler = in_pNotifyHandler;
	m_commandChannel.SetCommandChannelHandler( in_pCmdChannelHandler );
	
	if ( m_commandChannel.Init()
		&& m_notifChannel.Init()
		&& m_discoveryChannel.Init() )
	{
		m_bInitialized = true;
	}

	return m_bInitialized;
}

void CommunicationCentral::PreTerm()
{
	m_bInitialized = false;

	ResetChannels();
}

void CommunicationCentral::Term()
{
	m_discoveryChannel.Term();

	Network::Term(m_bInternalNetworkInit);
}

void CommunicationCentral::Process()
{
	if ( ! m_bInitialized )
		return;

	// Go through the channels and call process
	m_discoveryChannel.Process();
	m_notifChannel.Process();
	m_commandChannel.Process();
}

AK::Comm::INotificationChannel* CommunicationCentral::GetNotificationChannel()
{
	return &m_notifChannel;
}

void CommunicationCentral::PeerConnected()
{
	m_discoveryChannel.SetResponseState( DiscoveryChannel::StateBusy );
}

void CommunicationCentral::PeerDisconnected()
{
	ResetChannels();

	if( m_pNotifyHandler != NULL )
	{
		m_pNotifyHandler->PeerDisconnected();
	}
	
	m_discoveryChannel.SetResponseState( DiscoveryChannel::StateAvailable );

#if defined AK_3DS
	PrepareChannels( "Unknown" );
#endif
}

bool CommunicationCentral::PrepareChannels( const char* in_pszControllerName )
{
	m_discoveryChannel.SetControllerName( in_pszControllerName );
	
	return m_commandChannel.StartListening()
			&& m_notifChannel.StartListening();
}

bool CommunicationCentral::ResetChannels()
{
	m_notifChannel.Term();
	m_commandChannel.Term();

	m_discoveryChannel.SetControllerName( NULL );

	return true;
}

void CommunicationCentral::GetPorts( Ports& out_ports ) const
{
	out_ports.m_portCommand = m_commandChannel.GetPort();
	out_ports.m_portNotification = m_notifChannel.GetServerPort();
}
