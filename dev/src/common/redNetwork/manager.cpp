/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "manager.h"

#include <new>

#include "../redSystem/error.h"

namespace Red { namespace Network {

//////////////////////////////////////////////////////////////////////////
// Connection Event Listener
//////////////////////////////////////////////////////////////////////////

ConnectionEventListener::~ConnectionEventListener()
{

}

//////////////////////////////////////////////////////////////////////////
// Manager
//////////////////////////////////////////////////////////////////////////

#ifdef RED_NETWORK_ENABLED

Manager* Manager::m_instance = nullptr;

const System::AnsiChar Manager::BIND_TO_CHANNEL_PACKET_IDENTIFIER[] = "BIND";

#define WAKE_THREAD_PORT_RANGE_START	36050
#define WAKE_THREAD_PORT_RANGE_END		36060

Manager::Manager()
:	Threads::CThread( "Network" )
,	m_initialized( false )
,	m_shutdown( false )
{
	RED_ASSERT( m_instance == nullptr, TXT( "An instance of the network system manager already exists!" ) );
	m_instance = this;
}

Manager::~Manager()
{
	m_instance = nullptr;
}

void Manager::ThreadFunc()
{
	RED_LOG( RED_LOG_CHANNEL( Net ), TXT( "Thread Started" ) );

	InitializeInternal();

	while( !m_shutdown.GetValue() )
	{
		Update();
	}

	Shutdown();
}

void Manager::Initialize( Memory::ReallocatorFunc reallocFunc, Memory::FreeFunc freeFunc )
{
	Memory::Set( reallocFunc, freeFunc );

	InitThread();
}

void Manager::InitializeInternal()
{
	// Default memory allocations
	m_outgoingPackets.Initialize();

	// Platform specific initialisation
	Base::Initialize();

	// Threadwaker "dummy" UDP socket, that will send dummy data to wake the
	// network thread when there's something to be sent
	m_threadWaker.Create( Socket::UDP );

	System::Uint16 port = WAKE_THREAD_PORT_RANGE_START;
	m_threadWakerBoundAddress = Address( "127.0.0.1", port );
	
	// Search for available port
	while( m_threadWaker.IsUnbound() && !m_threadWaker.Bind( m_threadWakerBoundAddress ) && port <= WAKE_THREAD_PORT_RANGE_END )
	{
		++port;
		m_threadWakerBoundAddress.SetPort( port );
	}

	RED_ASSERT( port >= WAKE_THREAD_PORT_RANGE_START && port <= WAKE_THREAD_PORT_RANGE_END, TXT( "Unable to find a free port inside the specified range %i -> %i" ), WAKE_THREAD_PORT_RANGE_START, WAKE_THREAD_PORT_RANGE_END );
	RED_ASSERT( m_threadWaker.IsBound(), TXT( "Socket is in an invalid state, it should be bound" ) );

	m_initialized.SetValue( true );

	RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "Initialised" ) );
}

void Manager::Update()
{
	CheckForNewConnections();

	UpdatePendingConnections();

	UpdateIncoming();

	UpdateOutgoing();

	WaitUntilNeeded();
}

void Manager::WaitUntilNeeded()
{
	{
		fd_set writeset;
		fd_set readset;
		fd_set errorset;

		FD_ZERO( &writeset );
		FD_ZERO( &readset );
		FD_ZERO( &errorset );

		SocketId highestSocketId = m_threadWaker.GetRawDescriptor();

		if( m_listener.IsListening() )
		{
			FD_SET( m_listener.GetRawDescriptor(), &readset );

			SocketId listenerId = m_listener.GetRawDescriptor();

			if( highestSocketId < listenerId )
			{
				highestSocketId = listenerId;
			}
		}

		FD_SET( m_threadWaker.GetRawDescriptor(), &readset );

		for( auto connection = m_connections.Begin(); connection != m_connections.End(); ++connection )
		{
			if( connection->m_socket.IsConnecting() )
			{
				FD_SET( connection->m_socket.GetRawDescriptor(), &writeset );
				FD_SET( connection->m_socket.GetRawDescriptor(), &errorset );

				if( highestSocketId < connection->m_socket.GetRawDescriptor() )
				{
					highestSocketId = connection->m_socket.GetRawDescriptor();
				}
			}
			else if( connection->m_socket.IsConnected() )
			{
				FD_SET( connection->m_socket.GetRawDescriptor(), &readset );

				if( highestSocketId < connection->m_socket.GetRawDescriptor() )
				{
					highestSocketId = connection->m_socket.GetRawDescriptor();
				}
			}
		}

		// Has to be the value of the highest socket descriptor + 1
		++highestSocketId;

		// Wait for sockets
		select( (int)highestSocketId, &readset, &writeset, &errorset, nullptr );

		// Purge threadwaker buffer
		Address localhost;
		System::Uint8 buffer[ 32 ];
		while( m_threadWaker.ReceiveFrom( buffer, sizeof( buffer ), localhost ) )
		{
			// preventing spin-lock on thread
			Threads::SleepOnCurrentThread( 0 );
		};
	}
}

void Manager::Wake()
{
	System::Uint32 buffer = 0xCA11;

	m_threadWaker.SendTo( &buffer, sizeof( System::Uint32 ), m_threadWakerBoundAddress );
}

void Manager::Shutdown()
{
	m_lock.Acquire();

	for( auto connection = m_connections.Begin(); connection != m_connections.End(); ++connection )
	{
		for( auto listener = connection->m_listeners.Begin(); listener != connection->m_listeners.End(); ++listener )
		{
			( *listener )->OnConnectionClosed( connection->m_socket.GetPeer() );
		}

		connection->m_socket.Close();
	}

	m_lock.Release();

	if( m_listener.IsOpen() )
	{
		m_listener.Close();
	}

	if( m_threadWaker.IsOpen() )
	{
		m_threadWaker.Close();
	}

	Base::Shutdown();

	m_initialized.SetValue( false );
	m_shutdown.SetValue( false );

	RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "Shut Down" ) );
}

System::Bool Manager::ListenForIncomingConnections( System::Uint16 port )
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	// Create the socket that will listen for new incoming connections
	RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "Listening for incoming connections on port %hu" ), port );

	if( !m_initialized.GetValue() )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Net ), TXT( "Cannot listen for incoming connections - system not yet initialised" ) );
		return false;
	}

	if( !m_listener.Create() )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Net ), TXT( "Failed to create socket (Error code: %i)" ), Base::GetLastError() );
		return false;
	}
	
	if( !m_listener.Bind( port ) )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Net ), TXT( "Failed to bind on port %hu (Error code: %i)" ), port, Base::GetLastError() );

		m_listener.Close();
		return false;
	}
	
	if( !m_listener.Listen() )
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Net ), TXT( "Failed to listen (Error code: %i)" ), Base::GetLastError() );

		m_listener.Close();
		return false;
	}

	Wake();

	return true;
}

void Manager::CheckForNewConnections()
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	if( m_listener.IsListening() )
	{
		// Check for new incoming connections
		Socket newConnection = m_listener.Accept();
		if( newConnection.IsConnected() )
		{

			Connection* connection = m_connections.Add();

			if( connection )
			{
				connection->m_socket = newConnection;
				connection->m_incoming.Reset();

				RED_LOG_SPAM( RED_LOG_CHANNEL( Net ), TXT( "Connection Established" ) );
			}
			else
			{
				// We can't deal with any new connections at this time
				RED_LOG_WARNING( RED_LOG_CHANNEL( Net ), TXT( "Connection Refused" ) );
				newConnection.Close();
			}
		}
	}
}

void Manager::UpdatePendingConnections()
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	for( auto pendingConnection = m_pendingConnections.Begin(); pendingConnection != m_pendingConnections.End(); ++pendingConnection )
	{
		Channel* channel = FindChannelLock( pendingConnection->m_channelName );

		RED_ASSERT( channel );

		if( pendingConnection->m_socket->FinishConnecting() )
		{
			if( pendingConnection->m_socket->IsConnected() )
			{
				BindRemoteChannelToSocket( channel, pendingConnection->m_socket );

				//Notify the connection listeners
				auto connection = m_connections.Find( *( pendingConnection->m_socket ) );

				if( connection.IsValid() )
				{
					for( auto listener = connection->m_listeners.Begin(); listener != connection->m_listeners.End(); ++listener )
					{
						( *listener )->OnConnectionSucceeded( pendingConnection->m_socket->GetPeer() );
					}
				}
			}

			// We don't need to worry about the active connection entry (m_connections) as that will be taken care of automatically
			m_pendingConnections.Remove( pendingConnection );
		}
	}
}

void Manager::UpdateIncoming()
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	for( auto connection = m_connections.Begin(); connection != m_connections.End(); ++connection )
	{
		Socket& socket = connection->m_socket;
		IncomingPacket& incoming = connection->m_incoming;

		if( socket.IsConnected() && incoming.Receive( &socket ) )
		{
			ProcessIncomingPacket( incoming, socket );
		}
		else
		{
			if( !socket.IsConnected() && !socket.IsConnecting() )
			{
				if( socket.ConnectionClosed() )
				{
					for( auto listener = connection->m_listeners.Begin(); listener != connection->m_listeners.End(); ++listener )
					{
						( *listener )->OnConnectionClosed( socket.GetPeer() );
					}
				}
				else
				{
					for( auto listener = connection->m_listeners.Begin(); listener != connection->m_listeners.End(); ++listener )
					{
						( *listener )->OnConnectionDropped( socket.GetPeer() );
					}
				}

				RemoveConnection( &socket );
				connection->m_listeners.Clear();
				m_connections.Remove( connection );

				RED_LOG( RED_LOG_CHANNEL( Net ), TXT( "Remote connection closed" ) );
			}
		}
	}
}

void Manager::ProcessIncomingPacket( IncomingPacket& packet, Socket& socket )
{
	System::AnsiChar name[ Channel::NAME_MAX_LENGTH ];

	// First read in the packet type
	if( packet.ReadString( name, Channel::NAME_MAX_LENGTH ) )
	{
		// Figure out type and act accordingly
		if( System::StringCompare( name, BIND_TO_CHANNEL_PACKET_IDENTIFIER ) == 0 )
		{
			// It's an instruction to bind this connection to a specific channel
			packet.ReadString( name, Channel::NAME_MAX_LENGTH );

			Channel* channel = FindChannelLock( name );

			if( channel )
			{
				channel->RegisterDestination( &socket );
			}
		}
		else
		{
			// It's must be the name of a channel
			ToChannel( name, packet );
		}
	}

	packet.Reset();
}


void Manager::RemoveConnection( Socket* connection )
{
	// Remove connection from channel
	{
		Threads::CScopedLock< Threads::CMutex > lock( m_channelLock );

		for( auto channel = m_channels.Begin(); channel != m_channels.End(); ++channel )
		{
			channel->UnregisterDestination( connection );
		}
	}

	// Remove connection as destination from outgoing packets
	{
		for( auto packetIter = m_outgoingPackets.Begin(); packetIter != m_outgoingPackets.End(); ++packetIter )
		{
			PendingPacket* packet = *packetIter;
			for( auto destination = packet->m_destinations.Begin(); destination != packet->m_destinations.End(); ++destination )
			{
				if( destination->m_socket == connection )
				{
					packet->m_destinations.Remove( destination );
				}
			}
		}
	}
}

RED_INLINE Channel* Manager::FindChannelNoLock( const System::AnsiChar* channelName )
{
	for( auto channel = m_channels.Begin(); channel != m_channels.End(); ++channel )
	{
		if( System::StringCompare( channelName, channel->GetName(), Channel::NAME_MAX_LENGTH ) == 0 )
		{
			// Dereference the iterator, then get the address of the channel variable
			return &( *channel );
		}
	}

	return nullptr;
}

RED_INLINE Channel* Manager::FindChannelLock( const System::AnsiChar* channelName )
{
	Threads::CScopedLock< Threads::CMutex > lock( m_channelLock );

	return FindChannelNoLock( channelName );
}

Channel* Manager::CreateChannelInternal( const System::AnsiChar* channelName )
{
	Threads::CScopedLock< Threads::CMutex > lock( m_channelLock );

	Channel* channel = FindChannelNoLock( channelName );

	if( !channel )
	{
		channel = m_channels.Add();

		if( channel )
		{
			channel->SetName( channelName );
		}
	}

	return channel;
}

Manager::PendingPacket* Manager::CreatePacket() const
{
	void* buffer = Memory::Realloc( nullptr, sizeof( PendingPacket ) );
	return ::new( buffer ) PendingPacket;
}

void Manager::DestroyPacket( PendingPacket* packet ) const
{
	Memory::Free( packet );
}

RED_INLINE Manager::Connection* Manager::FindConnection( const Address& target )
{
	//Find out if any of our existing connections matches the target address
	for( auto connection = m_connections.Begin(); connection != m_connections.End(); ++connection )
	{
		if( connection->m_socket.GetPeer() == target )
		{
			// Get the contents of the iterator, and then return the address
			return &( *connection );
		}
	}

	return nullptr;
}

Manager::Connection* Manager::CreateConnection( const Address& target )
{
	Connection* connection = FindConnection( target );

	if( !connection )
	{
		if( m_connections.GetSpaceRemaining() > 0 )
		{
			Socket newSocket;

			if( newSocket.Create() )
			{
				if( newSocket.Connect( target ) )
				{
					connection = m_connections.Add();
					connection->m_socket = newSocket;
					connection->m_incoming.Reset();
					connection->m_listeners.Clear();
				}
			}
		}
	}

	return connection;
}

void Manager::BindRemoteChannelToSocket( Channel* channel, Socket* socket )
{
	// Now that the socket is fully connected, associate it with the channel
	channel->RegisterDestination( socket );

	// Instruct the other side to associate this connection with the same channel
	OutgoingPacket packet;

	// Fill the packet
	packet.WriteString( BIND_TO_CHANNEL_PACKET_IDENTIFIER );
	packet.WriteString( channel->GetName() );

	Send( socket, packet );
}

void Manager::ToChannel( const System::AnsiChar* channelName, IncomingPacket& packet )
{
	Channel* channel = FindChannelLock( channelName );
	
	if( channel )
	{
		channel->ReceivePacket( packet );
	}
}

void Manager::UpdateOutgoing()
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	//Keep sending packet data until we either run out of packets or fail to send the packet to all destinations in one go
	while( m_outgoingPackets.GetSize() > 0 )
	{
		PendingPacket* packet = *( m_outgoingPackets.Front() );

		System::Bool finished = true;
		
		for( auto destination = packet->m_destinations.Begin(); destination != packet->m_destinations.End(); ++destination )
		{
			if( destination->m_state != OutgoingPacketDestination::State_Sent )
			{
				// If the destination hasn't finished connecting, then we're not finished sending the packet
				// If the connection has dropped, then this destination no longer affects the success of this packet
				if( destination->m_socket->IsConnecting() || ( destination->m_socket->IsConnected() && !packet->m_packet.Send( *destination ) ) )
				{
					finished = false;
				}
			}
		}

		if( finished )
		{
			PendingPacket* packet = *( m_outgoingPackets.Front() );
			DestroyPacket( packet );
			m_outgoingPackets.PopFront();
		}
		else
		{
			// The packet hasn't finished sending, wait for the next update
			Wake();
			break;
		}
	}
}

System::Bool Manager::Send( const System::AnsiChar* channelName, const OutgoingPacket& packet )
{
	Channel* channel = FindChannelLock( channelName );

	if( channel )
	{
		Threads::CScopedLock< Threads::CMutex > lock( m_lock );

		// Create pending packet
		PendingPacket* queuedPacket = CreatePacket();
		*( m_outgoingPackets.Add() ) = queuedPacket;

		// Copy Packet
		System::MemoryCopy( &queuedPacket->m_packet, &packet, sizeof( OutgoingPacket ) );

		// Copy destinations from channel to pending packet
		queuedPacket->m_destinations.Clear();

		Channel::DestinationList& destinations = channel->GetDestinations();

		for( auto channelDest = destinations.Begin(); channelDest != destinations.End(); ++channelDest )
		{
			OutgoingPacketDestination* packetDest = queuedPacket->m_destinations.Add();

			packetDest->Initialise( *channelDest );
		}

		//Ensure that the network thread is alive
		Wake();

		return true;
	}
	else
	{
		RED_LOG_ERROR( RED_LOG_CHANNEL( Net ), TXT( "Could not send packet on unknown channel %" ) RED_PRIWas, channelName );
	}

	return false;
}

System::Bool Manager::Send( Socket* destination, const OutgoingPacket& packet )
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	PendingPacket* queuedPacket = CreatePacket();
	*( m_outgoingPackets.Add() ) = queuedPacket;

	queuedPacket->m_destinations.Clear();

	OutgoingPacketDestination* packetDest = queuedPacket->m_destinations.Add();
	packetDest->Initialise( destination );

	System::MemoryCopy( &queuedPacket->m_packet, &packet, sizeof( OutgoingPacket ) );

	//Ensure that the network thread is alive
	Wake();

	return true;
}

System::Bool Manager::RegisterListener( const System::AnsiChar* channelName, ChannelListener* listener )
{
	Channel* channel = CreateChannelInternal( channelName );

	if( channel )
	{
		channel->RegisterListener( listener );

		return true;
	}

	return false;
}

System::Bool Manager::UnregisterListener( const System::AnsiChar* channelName, ChannelListener* listener )
{
	Channel* channel = FindChannelLock( channelName );

	if( channel )
	{
		channel->UnregisterListener( listener );

		return true;
	}

	return false;
}

void Manager::UnregisterListener( ConnectionEventListener* listener )
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	for( auto connection = m_connections.Begin(); connection != m_connections.End(); ++connection )
	{
		for( auto iListener = connection->m_listeners.Begin(); iListener != connection->m_listeners.End(); ++iListener )
		{
			if( *iListener == listener )
			{
				connection->m_listeners.Remove( iListener );
				return;
			}
		}
	}
}

System::Bool Manager::ConnectTo( const Address& target, const System::AnsiChar* channelName, ConnectionEventListener* listener )
{
	System::Bool retval = false;

	Channel* channel = CreateChannelInternal( channelName );

	if( channel )
	{
		Threads::CScopedLock< Threads::CMutex > lock( m_lock );

		Connection* connection = CreateConnection( target );

		if( connection )
		{
			if( connection->m_socket.IsConnecting() )
			{
				// Register connection as pending, we'll check for success or failure later
				{
					PendingConnection* pendingConnection = m_pendingConnections.Add();
					pendingConnection->m_socket = &connection->m_socket;
					System::StringCopy( pendingConnection->m_channelName, channelName, Channel::NAME_MAX_LENGTH );
				}

				// If a listener was supplied, store it 
				if( listener )
				{
					ConnectionEventListener** listenerSlot = connection->m_listeners.Add();

					if( listenerSlot )
					{
						*listenerSlot = listener;
					}
				}

				Wake();

				retval = true;
			}
			else if( connection->m_socket.IsConnected() )
			{
				if( listener )
				{
					ConnectionEventListener** listenerSlot = connection->m_listeners.Add();

					if( listenerSlot )
					{
						// Connection was already established
						listener->OnConnectionAvailable( connection->m_socket.GetPeer() );

						*listenerSlot = listener;
					}
				}

				BindRemoteChannelToSocket( channel, &connection->m_socket );

				retval = true;
			}
		}
	}

	return retval;
}

System::Bool Manager::DisconnectFrom( const Address& target )
{
	Threads::CScopedLock< Threads::CMutex > lock( m_lock );

	Connection* connection = FindConnection( target );

	if( connection )
	{
		// simply close the socket, connection cleanup will occur during update step
		connection->m_socket.Close();

		return true;
	}

	return false;
}

System::Uint32 Manager::GetNumberOfConnectionsToChannel( const System::AnsiChar* channelName )
{
	Channel* channel = FindChannelLock( channelName );

	if( channel )
	{
		return channel->GetDestinations().GetSize();
	}

	return 0;
}

#endif // RED_NETWORK_ENABLED

} } // namespace Red { namespace Network {
