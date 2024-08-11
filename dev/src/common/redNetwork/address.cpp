/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "address.h"
#include "../redSystem/error.h"

Red::Network::Address::Address()
{
}

Red::Network::Address::Address( Red::System::Uint16 port, EFamily family )
{
	if( family == Family_IPv4 )
	{
		SockaddrIpv4* address = reinterpret_cast< SockaddrIpv4* >( &m_address );

		address->sin_family = RED_NET_AF_INET;
		address->sin_addr.s_addr = RED_NET_INADDR_ANY;
		address->sin_port = HostToNetwork( port );
	}
	else if( family == Family_IPv6 )
	{
		SockaddrIpv6* address = reinterpret_cast< SockaddrIpv6* >( &m_address );

		address->sin6_family = RED_NET_AF_INET6;
		address->sin6_addr = RED_INADDR6_ANY;
		address->sin6_port = HostToNetwork( port );
	}
}

Red::Network::Address::Address( const Red::System::AnsiChar* ip, Red::System::Uint16 port, EFamily family )
{
	m_address.ss_family = static_cast< AddressFamily >( family );
	SetPort( port );

	System::Int32 result = INetPtoN( m_address.ss_family, ip, GetIPStorage() );
	RED_UNUSED( result );
}

Red::Network::Address::Address( const Red::System::UniChar* ip, Red::System::Uint16 port, EFamily family )
{
	m_address.ss_family = static_cast< AddressFamily >( family );
	SetPort( port );

	System::Int32 result = INetPtoN( m_address.ss_family, ip, GetIPStorage() );
	RED_UNUSED( result );
}

Red::Network::Address::Address( const Address& address )
{
	*this = address;
}

Red::Network::Address::Address( const SockaddrIpv4& address )
{
	*this = address;
}

Red::Network::Address::Address( const SockaddrIpv6& address )
{
	*this = address;
}

Red::Network::Address::Address( const Sockaddr& address )
{
	*this = address;
}

Red::Network::Address::Address( const SockaddrStorage& address )
{
	*this = address;
}

void Red::Network::Address::SetPort( System::Uint16 port )
{
	if( m_address.ss_family == RED_NET_AF_INET )
	{
		SockaddrIpv4* ipv4Address = reinterpret_cast< SockaddrIpv4* >( &m_address );
		ipv4Address->sin_port = HostToNetwork( port );
	}
	else if( m_address.ss_family == RED_NET_AF_INET6 )
	{
		SockaddrIpv6* ipv6Address = reinterpret_cast< SockaddrIpv6* >( &m_address );
		ipv6Address->sin6_port = HostToNetwork( port );
	}
	else
	{
		RED_HALT( "Invalid or Uninitialised family" );
	}
}

const void* Red::Network::Address::GetIPStorage() const
{
	if( m_address.ss_family == RED_NET_AF_INET )
	{
		const SockaddrIpv4* ipv4Address = reinterpret_cast< const SockaddrIpv4* >( &m_address );
		return &( ipv4Address->sin_addr );
	}
	else if( m_address.ss_family == RED_NET_AF_INET6 )
	{
		const SockaddrIpv6* ipv6Address = reinterpret_cast< const SockaddrIpv6* >( &m_address );
		return &( ipv6Address->sin6_addr );
	}
	else
	{
		RED_HALT( "Invalid or Uninitialised family" );
	}

	return nullptr;
}

Red::System::Bool Red::Network::Address::GetIp( System::AnsiChar* buffer, System::Uint32 size ) const
{
	return INetNtoP( m_address.ss_family, GetIPStorage(), buffer, size );
}

Red::System::Uint16 Red::Network::Address::GetPort() const
{
	if( m_address.ss_family == RED_NET_AF_INET )
	{
		const SockaddrIpv4* ipv4Address = reinterpret_cast< const SockaddrIpv4* >( &m_address );
		return HostToNetwork( ipv4Address->sin_port );
	}
	else if( m_address.ss_family == RED_NET_AF_INET6 )
	{
		const SockaddrIpv6* ipv6Address = reinterpret_cast< const SockaddrIpv6* >( &m_address );
		return HostToNetwork( ipv6Address->sin6_port );
	}
	else
	{
		RED_HALT( "Invalid or Uninitialised family" );
	}

	return 0;
}

