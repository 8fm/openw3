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

#include "CommandChannel.h"
#include "IChannelsHolder.h"
#include "ICommandChannelHandler.h"
#include "Serializer.h"
#include "IPConnectorPorts.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CommandChannel::CommandChannel( IChannelsHolder* in_pHolder, AkMemPoolId in_pool, const AkThreadProperties& in_threadProperties)
	: IncomingChannel( IPConnectorPorts::Current.uCommand, in_pool, in_threadProperties )
	, m_pCmdChannelHandler( NULL )
	, m_pHolder( in_pHolder )
{
	// AKASSERT( m_pCmdChannelHandler );
}

CommandChannel::~CommandChannel()
{
}

void CommandChannel::SetCommandChannelHandler( AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler )
{
	m_pCmdChannelHandler = in_pCmdChannelHandler;
}

bool CommandChannel::ProcessCommand( AkUInt8* in_pData, AkUInt32 /*in_uDataLength*/ )
{	
	if( !m_pCmdChannelHandler )
	{
		return false;
	}

	AkUInt32 uReturnDataSize = 0;
	const AkUInt8* pReturnData = m_pCmdChannelHandler->HandleExecute( in_pData, uReturnDataSize );

    if( uReturnDataSize )
	{
	    AKASSERT( pReturnData );
	
		Send( pReturnData, uReturnDataSize );
	}

	return true;
}

void CommandChannel::PeerConnected( const GameSocketAddr& )
{
	m_pHolder->PeerConnected();
}

void CommandChannel::PeerDisconnected()
{
	m_pHolder->PeerDisconnected();
}

const char* CommandChannel::GetRequestedPortName()
{
	return "AkCommSettings::ports.uCommand";
}
