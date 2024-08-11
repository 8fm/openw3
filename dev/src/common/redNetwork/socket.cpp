/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "socket.h"

#include "../redSystem/error.h"

Red::Network::Socket::Socket()
:	m_descriptor( InvalidSocket )
,	m_state( State_Uninitialised )
,	m_lastErrorCode( 0 )
{

}

Red::Network::Socket::Socket( SocketId connectedSocket, State state, const Address& peer )
:	m_descriptor( connectedSocket )
,	m_state( state )
,	m_lastErrorCode( 0 )
{
	System::MemoryCopy( &m_peer, &peer, sizeof( Address ) );
	
	// At the moment, any socket created like this will be TCP
	m_protocol = TCP;
}

void Red::Network::Socket::operator=( const Socket& other )
{
	System::MemoryCopy( this, &other, sizeof( Socket ) );
}

Red::Network::Socket::~Socket()
{
}

Red::System::Bool Red::Network::Socket::Create( Protocol protocol, AddressType addressType )
{
	RED_ASSERT( m_descriptor == InvalidSocket, TXT( "Socket is already in use" ) );

	ProtocolFamily pf = ( addressType == IPv4 )? RED_NET_PF_INET : RED_NET_PF_INET6;

	System::Int32 type = RED_NET_SOCK_STREAM;
	System::Int32 proto = RED_NET_IPPROTO_TCP;

	if( protocol == UDP )
	{
		type = RED_NET_SOCK_DGRAM;
		proto = RED_NET_IPPROTO_UDP;
	}

	m_protocol = protocol;
	m_descriptor = Base::Socket( pf, type, proto );

	if( m_descriptor == InvalidSocket )
	{
		m_lastErrorCode = Base::GetLastError();
		return false;
	}

	m_state = State_Unbound;

	if( !Base::SetNonBlocking( m_descriptor ) )
	{
		m_lastErrorCode = Base::GetLastError();
		Close( State_Uninitialised );
		return false;
	}

	return true;
}

Red::System::Bool Red::Network::Socket::Bind( const Address& address )
{
	RED_ASSERT( m_descriptor != InvalidSocket, TXT( "Socket is uninitialised" ) );
	RED_ASSERT( m_state == State_Unbound, TXT( "Socket is not unbound: %i" ), m_state );

	if( !Base::Bind( m_descriptor, address.GetNative(), address.GetNativeSize() ) )
	{
		m_lastErrorCode = Base::GetLastError();

		if( m_lastErrorCode != RED_NET_ERR_ADDR_IN_USE )
		{
			Close( State_Uninitialised );
		}

		return false;
	}

	m_state = State_Bound;

	return true;
}

Red::System::Bool Red::Network::Socket::Bind( Red::System::Uint16 port )
{
	SockaddrIpv4 address;
	address.sin_family = RED_NET_AF_INET;
	address.sin_addr.s_addr = RED_NET_INADDR_ANY;
	address.sin_port = HostToNetwork( port );
	System::MemoryZero( address.sin_zero, sizeof( address.sin_zero ) );

	return Bind( address );
}

Red::System::Bool Red::Network::Socket::Listen()
{
	RED_ASSERT( m_descriptor != InvalidSocket, TXT( "Socket is uninitialised" ) );
	RED_ASSERT( m_state == State_Bound, TXT( "Socket is not bound: %i" ), m_state );
	
	if( !Base::Listen( m_descriptor ) )
	{
		m_lastErrorCode = Base::GetLastError();
		Close( State_Uninitialised );
		return false;
	}

	m_state = State_Listening;

	return true;
}

Red::Network::Socket Red::Network::Socket::Accept()
{
	SockaddrStorage destinationAddress;

	System::MemoryZero( &destinationAddress, sizeof( SockaddrStorage ) );
	SockaddrLen sizeofDestinationAddress = sizeof( SockaddrStorage );

	SocketId connectedSocket;
	
	State newSocketState = State_Connected;

	if( !Base::Accept( m_descriptor, connectedSocket, reinterpret_cast< Sockaddr* >( &destinationAddress ), &sizeofDestinationAddress ) )
	{
		newSocketState = State_Uninitialised;

		ErrorCode errorCode = Base::GetLastError();

		if( errorCode != RED_NET_ERR_ACCEPT_OK )
		{
			m_lastErrorCode = errorCode;
			Close( State_Dropped );
		}

		return Red::Network::Socket();
	}

	Address peer;

	SockaddrLen size = peer.GetNativeSize();
	if( !Base::GetPeerName( connectedSocket, peer.GetNative(), &size ) )
	{
		m_lastErrorCode = Base::GetLastError();
	}

	return Red::Network::Socket( connectedSocket, newSocketState, peer );
}

Red::System::Bool Red::Network::Socket::Connect( const Address& destination )
{
	if( !Base::Connect( m_descriptor, destination.GetNative(), destination.GetNativeSize() ) )
	{
		System::MemoryCopy( &m_peer, &destination, sizeof( Address ) );

		ErrorCode errorCode = Base::GetLastError();

		if( errorCode != RED_NET_ERR_CONNECT_OK )
		{
			m_lastErrorCode = errorCode;
			Close( State_Uninitialised );
			return false;
		}
	}

	m_state = State_Connecting;

	return true;
}

void Red::Network::Socket::Close( State resultantState )
{
	Base::Close( m_descriptor );
	m_descriptor = InvalidSocket;
	m_state = resultantState;
}

Red::System::Uint16 Red::Network::Socket::Send( const void* buffer, System::Uint32 size )
{
	System::Int32 result = Base::Send( m_descriptor, buffer, size );

	if( result < 0 )
	{
		m_lastErrorCode = Base::GetLastError();

		if( m_lastErrorCode != RED_NET_ERR_SEND_OK )
		{
			// Ungraceful disconnection
			Close( State_Dropped );
		}
		
		return 0;
	}

	return static_cast< System::Uint16 >( result );
}

Red::System::Uint16 Red::Network::Socket::Receive( void* buffer, System::Uint32 size )
{
	System::Int32 result = Base::Recv( m_descriptor, buffer, size );

	if( result < 0 )
	{
		ErrorCode errorCode = Base::GetLastError();

		if( errorCode != RED_NET_ERR_RECEIVE_OK )
		{
			// Ungraceful disconnection
			m_lastErrorCode = errorCode;
			Close( State_Dropped );
		}
		else
		{
			// No bytes received, connection still active
			result = 0;
		}
	}
	else if( result == 0 && size != 0 )
	{
		// Graceful disconnection
		Close( State_Closed );
	}

	return static_cast< System::Uint16 >( result );
}

Red::System::Uint16 Red::Network::Socket::SendTo( const void* buffer, System::Uint32 size, const Address& destination )
{
	System::Int32 result = Base::SendTo( m_descriptor, buffer, size, destination.GetNative(), destination.GetNativeSize() );

	return static_cast< System::Uint16 >( result );
}

Red::System::Uint16 Red::Network::Socket::ReceiveFrom( void* buffer, System::Uint32 size, Address& source )
{
	SockaddrLen len = source.GetNativeSize();
	System::Int32 result = Base::RecvFrom( m_descriptor, buffer, size, source.GetNative(), &len );

	if( result < 0 )
	{
		ErrorCode errorCode = Base::GetLastError();
		
		if( errorCode != RED_NET_ERR_RECEIVE_OK )
		{
			Close( State_Dropped );
		}

		return 0;
	}

	return static_cast< System::Uint16 >( result );
}

Red::System::Bool Red::Network::Socket::IsReady() const
{
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	fd_set writeset;
	FD_ZERO( &writeset );

	FD_SET( m_descriptor, &writeset );

	select( 1, nullptr, &writeset, nullptr, &timeout );

	return ( FD_ISSET( m_descriptor, &writeset ) )? true : false;
}

Red::System::Bool Red::Network::Socket::GetPeer( Address& peer ) const
{
	RED_ASSERT( IsConnected() );

	SockaddrLen size = peer.GetNativeSize();
	return Base::GetPeerName( m_descriptor, peer.GetNative(), &size );
}

Red::System::Bool Red::Network::Socket::FinishConnecting()
{
	if( m_state == State_Connecting )
	{
		timeval timeout;
		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

		fd_set writeset;
		FD_ZERO( &writeset );

		fd_set errorset;
		FD_ZERO( &errorset );

		FD_SET( m_descriptor, &writeset );
		FD_SET( m_descriptor, &errorset );

		select( (int)m_descriptor, nullptr, &writeset, &errorset, &timeout );

		if( FD_ISSET( m_descriptor, &writeset ) )
		{
			m_state = State_Connected;

			return true;
		}
		else if( FD_ISSET( m_descriptor, &errorset ) )
		{
// 			int val;
// 			int size = sizeof( int );
// 
// 			if( getsockopt( m_descriptor, RED_NET_SOL_SOCKET, RED_NET_SO_ERROR, (char*)&val, &size ) == 0 )
// 			{
// 
// 			}

			m_state = State_Dropped;

			return true;
		}
	}

	return false;
}

Red::System::Bool Red::Network::Socket::SetOptionReuseAddress( System::Bool enabled )
{
	SocketOptionReuseAddr value = ( enabled )? 1 : 0;

	return Base::SetSocketOption( m_descriptor, RED_NET_SOL_SOCKET, RED_NET_SO_REUSEADDR, &value, sizeof( SocketOptionReuseAddr ) );
}
