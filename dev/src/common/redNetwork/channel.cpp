/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "channel.h"
#include "socket.h"
#include "../redSystem/error.h"

//////////////////////////////////////////////////////////////////////////
// Packet
//////////////////////////////////////////////////////////////////////////
Red::Network::ChannelPacket::ChannelPacket( const System::AnsiChar* channelName )
:	OutgoingPacket()
{
	WriteString( channelName );
}

Red::Network::ChannelPacket::~ChannelPacket()
{

}

void Red::Network::ChannelPacket::Clear( const System::AnsiChar* channelName )
{
	RED_FATAL_ASSERT( channelName, "NULL channel name!" );

	OutgoingPacket::Clear();
	WriteString( channelName );
}

//////////////////////////////////////////////////////////////////////////
// Listener
//////////////////////////////////////////////////////////////////////////
Red::Network::ChannelListener::ChannelListener()
{

}

Red::Network::ChannelListener::~ChannelListener()
{

}

//////////////////////////////////////////////////////////////////////////
// Channel
//////////////////////////////////////////////////////////////////////////
Red::Network::Channel::Channel()
{

}

Red::Network::Channel::~Channel()
{

}

void Red::Network::Channel::ReceivePacket( IncomingPacket& packet )
{
	// For a freshly received packet, the read position will be just after the header and channel name
	System::Uint16 dataStartPosition = packet.GetPosition();

	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listenerLock );

	FixedPoolList< ChannelListener*, MAX_LISTENERS >::Iterator listener;
	for( listener = m_listeners.Begin(); listener != m_listeners.End(); ++listener )
	{
		packet.SetPosition( dataStartPosition );
		(*listener)->OnPacketReceived( m_name, packet );
	}
}

void Red::Network::Channel::RegisterListener( ChannelListener* listener )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listenerLock );

	if( !m_listeners.Find( listener ).IsValid() )
	{
		ChannelListener** listenerStorage = m_listeners.Add();
		if( listenerStorage )
		{
			*listenerStorage = listener;
		}
	}
}

void Red::Network::Channel::UnregisterListener( ChannelListener* listener )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_listenerLock );

	FixedPoolList< ChannelListener*, MAX_LISTENERS >::Iterator listenerToRemove = m_listeners.Find( listener );

	if( listenerToRemove.IsValid() )
	{
		m_listeners.Remove( listenerToRemove );
	}
}

void Red::Network::Channel::RegisterDestination( Socket* socket )
{
	if( !m_destinations.Find( socket ).IsValid() )
	{
		Socket** destination = m_destinations.Add();

		if( destination )
		{
			*destination = socket;
		}
	}
}

void Red::Network::Channel::UnregisterDestination( Socket* destinationToRemove )
{
	DestinationList::Iterator destination;
	for( destination = m_destinations.Begin(); destination != m_destinations.End(); ++destination )
	{
		if( *destination == destinationToRemove )
		{
			m_destinations.Remove( destination );
			break;
		}
	}
}
