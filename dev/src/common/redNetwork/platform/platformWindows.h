/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef __RED_NETWORK_WINDOWS_WRAPPERS_H__
#define __RED_NETWORK_WINDOWS_WRAPPERS_H__

//
#include "../../redSystem/types.h"
#include "../../redSystem/crt.h"

// System Includes
#include <winsock2.h>
#include <ws2tcpip.h>

// Enumerations and defines

#define RED_NET_MAX_ADDRSTRLEN	INET6_ADDRSTRLEN

#define RED_NET_INADDR_ANY		INADDR_ANY

#define RED_NET_PF_INET			PF_INET
#define RED_NET_PF_INET6		PF_INET6

#define RED_NET_AF_INET			AF_INET
#define RED_NET_AF_INET6		AF_INET6

#define RED_NET_SOCK_STREAM		SOCK_STREAM
#define RED_NET_IPPROTO_TCP		IPPROTO_TCP

#define RED_NET_SOCK_DGRAM		SOCK_DGRAM
#define RED_NET_IPPROTO_UDP		IPPROTO_UDP

#define RED_NET_SOL_SOCKET		SOL_SOCKET
#define RED_NET_SO_ERROR		SO_ERROR

#define RED_NET_SO_REUSEADDR	SO_REUSEADDR

#define RED_NET_ERR_CONNECT_OK	WSAEWOULDBLOCK
#define RED_NET_ERR_ACCEPT_OK	WSAEWOULDBLOCK
#define RED_NET_ERR_RECEIVE_OK	WSAEWOULDBLOCK
#define RED_NET_ERR_SEND_OK		WSAEWOULDBLOCK

#define RED_NET_ERR_ADDR_IN_USE	WSAEADDRINUSE


// Structs
namespace Red
{
	namespace Network
	{
		typedef int					ErrorCode;

		//////////////////////////////////////////////////////////////////////////
		// Socket
		//////////////////////////////////////////////////////////////////////////
		typedef SOCKET				SocketId;
		static const SocketId		InvalidSocket = INVALID_SOCKET;
		typedef int					ProtocolFamily;

		//////////////////////////////////////////////////////////////////////////
		// Socket Options
		//////////////////////////////////////////////////////////////////////////
		typedef int					SocketOptionLength;

		typedef DWORD				SocketOptionReuseAddr;

		//////////////////////////////////////////////////////////////////////////
		// Address
		//////////////////////////////////////////////////////////////////////////
		typedef sockaddr_in			SockaddrIpv4;
		typedef sockaddr_in6		SockaddrIpv6;

		typedef sockaddr			Sockaddr;

		typedef socklen_t			SockaddrLen;

		extern CONST IN6_ADDR&		RED_INADDR6_ANY;

		typedef ADDRESS_FAMILY		AddressFamily;


		// PS4 currently doesn't support IPv6, so we need to create a dummy struct to match its functionality
		struct SockaddrStorage : public sockaddr_storage
		{
			RED_INLINE SockaddrStorage()
			{
				System::MemoryZero( this, sizeof( SockaddrStorage ) );
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrStorage& other )
			{
				System::MemoryCopy( this, &other, sizeof( sockaddr_storage ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const Sockaddr& other )
			{
				System::MemoryCopy( this, &other, sizeof( Sockaddr ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrIpv4& other )
			{
				System::MemoryCopy( this, &other, sizeof( SockaddrIpv4 ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrIpv6& other )
			{
				System::MemoryCopy( this, &other, sizeof( SockaddrIpv6 ) );
				return *this;
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// Socket Functions
		//////////////////////////////////////////////////////////////////////////

		namespace Base
		{
			RED_INLINE SocketId Socket( ProtocolFamily family, System::Int32 type, System::Int32 protocol )
			{
				return socket( family, type, protocol );
			}

			RED_INLINE System::Bool SetNonBlocking( SocketId socketDescriptor )
			{
				DWORD on = 1;
				return ioctlsocket( socketDescriptor, FIONBIO, &on ) != SOCKET_ERROR;
			}

			RED_INLINE System::Bool Bind( SocketId socketDescriptor, const Sockaddr* address, SockaddrLen size )
			{
				return bind( socketDescriptor, address, size ) != SOCKET_ERROR;
			}

			RED_INLINE System::Bool Listen( SocketId socketDescriptor )
			{
				return listen( socketDescriptor, SOMAXCONN ) != SOCKET_ERROR;
			}

			RED_INLINE System::Bool Accept( SocketId listenSocketDescriptor, SocketId& newSocketDescriptor, Sockaddr* address, SockaddrLen* size )
			{
				newSocketDescriptor = accept( listenSocketDescriptor, address, size );
				return newSocketDescriptor != INVALID_SOCKET;
			}

			RED_INLINE System::Bool Connect( SocketId socketDescriptor, const Sockaddr* address, SockaddrLen size )
			{
				return connect( socketDescriptor, address, size ) != SOCKET_ERROR;
			}

			RED_INLINE System::Bool Close( SocketId socketDescriptor )
			{
				return closesocket( socketDescriptor ) != SOCKET_ERROR;
			}

			RED_INLINE System::Int32 Send( SocketId socketDescriptor, const void* buffer, System::Uint32 size )
			{
				return send( socketDescriptor, static_cast< const char* >( buffer ), size, 0 );
			}

			RED_INLINE System::Int32 Recv( SocketId socketDescriptor, void* buffer, System::Uint32 size )
			{
				return recv( socketDescriptor, static_cast< char* >( buffer ), size, 0 );
			}

			RED_INLINE System::Int32 SendTo( SocketId socketDescriptor, const void* buffer, System::Uint32 bufferSize, const Sockaddr* address, SockaddrLen addressSize )
			{
				return sendto( socketDescriptor, static_cast< const char* >( buffer ), bufferSize, 0, address, addressSize );
			}

			RED_INLINE System::Int32 RecvFrom( SocketId socketDescriptor, void* buffer, System::Uint32 bufferSize, Sockaddr* address, SockaddrLen* addressSize )
			{
				return recvfrom( socketDescriptor, static_cast< char* >( buffer ), bufferSize, 0, address, addressSize );
			}

			RED_INLINE System::Bool GetPeerName( SocketId socketDescriptor, Sockaddr* address, SockaddrLen* size )
			{
				return getpeername( socketDescriptor, address, size ) != SOCKET_ERROR;
			}

			RED_INLINE System::Bool SetSocketOption( SocketId socketDescriptor, System::Int32 level, System::Int32 option, const void* value, SocketOptionLength length )
			{
				return setsockopt( socketDescriptor, level, option, static_cast< const char* >( value ), length ) != SOCKET_ERROR;
			}

			RED_INLINE ErrorCode GetLastError()
			{
				return WSAGetLastError();
			}

			RED_INLINE System::Bool Initialize()
			{
				WSADATA wsaData;
				return WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) == 0;
			}

			RED_INLINE System::Bool Shutdown()
			{
				return WSACleanup() != SOCKET_ERROR;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Ip String <-> Data Conversion
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Int32 INetPtoN( AddressFamily family, const Red::System::AnsiChar* source, void* destination ) { return InetPtonA( family, source, destination ); }
		RED_INLINE Red::System::Int32 INetPtoN( AddressFamily family, const Red::System::UniChar* source, void* destination ) { return InetPtonW( family, source, destination ); }

		RED_INLINE Red::System::Bool INetNtoP( AddressFamily family, const void* source, Red::System::AnsiChar* destination, Red::System::Uint32 destinationSize ) { return ( InetNtopA( family, const_cast< PVOID >( source ), destination, destinationSize ) )? true : false; }
		RED_INLINE Red::System::Bool INetNtoP( AddressFamily family, const void* source, Red::System::UniChar* destination, Red::System::Uint32 destinationSize ) { return ( InetNtopW( family, const_cast< PVOID >( source ), destination, destinationSize ) )? true : false; }

		//////////////////////////////////////////////////////////////////////////
		// Endian Conversion
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE System::Int8		NetworkToHost( const System::Int8 data )			{ return data; }
		RED_INLINE System::Int32	NetworkToHost( const System::Int32 data )			{ return ntohl( data ); }
		RED_INLINE System::Int16	NetworkToHost( const System::Int16 data )			{ return ntohs( data ); }
		RED_INLINE System::Int64	NetworkToHost( const System::Int64 data )
		{
#ifdef RED_ENDIAN_LITTLE
			return
				(
				( ( data & 0x00000000000000FFULL ) << 56 ) |
				( ( data & 0x000000000000FF00ULL ) << 40 ) |
				( ( data & 0x0000000000FF0000ULL ) << 24 ) |
				( ( data & 0x00000000FF000000ULL ) <<  8 ) |
				( ( data & 0x000000FF00000000ULL ) >>  8 ) |
				( ( data & 0x0000FF0000000000ULL ) >> 24 ) |
				( ( data & 0x00FF000000000000ULL ) >> 40 ) |
				( ( data & 0xFF00000000000000ULL ) >> 56 )
				);
#else
			return data;
#endif
		}

		RED_INLINE System::Int8		HostToNetwork( const System::Int8 data )			{ return data; }
		RED_INLINE System::Int16	HostToNetwork( const System::Int16 data )			{ return htons( data ); }
		RED_INLINE System::Int32	HostToNetwork( const System::Int32 data )			{ return htonl( data ); }
		RED_INLINE System::Int64	HostToNetwork( const System::Int64 data )			{ return NetworkToHost( data ); }

		RED_INLINE System::Uint8	NetworkToHost( const System::Uint8 data )			{ return data; }
		RED_INLINE System::Uint32	NetworkToHost( const System::Uint32 data )			{ return ntohl( data ); }
		RED_INLINE System::Uint16	NetworkToHost( const System::Uint16 data )			{ return ntohs( data ); }
		RED_INLINE System::Uint64	NetworkToHost( const System::Uint64 data )			{ return NetworkToHost( static_cast< System::Int64 >( data ) ); }

		RED_INLINE System::Uint8	HostToNetwork( const System::Uint8 data )			{ return data; }
		RED_INLINE System::Uint16	HostToNetwork( const System::Uint16 data )			{ return htons( data ); }
		RED_INLINE System::Uint32	HostToNetwork( const System::Uint32 data )			{ return htonl( data ); }
		RED_INLINE System::Uint64	HostToNetwork( const System::Uint64 data )			{ return NetworkToHost( data ); }

		RED_INLINE System::UniChar	NetworkToHost( const System::UniChar data )			{ return ntohs( data ); }
		RED_INLINE System::UniChar	HostToNetwork( const System::UniChar data )			{ return htons( data ); }
	}
}


#endif // __RED_NETWORK_WINDOWS_WRAPPERS_H__
