/**
* Copyright (c) 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once

// System Includes
#include <sys/socket.h>
#include <arpa/inet.h>
#include <endian.h>    // __BYTE_ORDER __LITTLE_ENDIAN
#include <byteswap.h>  // bswap_64()

#ifndef __LITTLE_ENDIAN
#	error Big endian machines are not supported!
#endif

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

#define RED_NET_ERR_CONNECT_OK	EWOULDBLOCK
#define RED_NET_ERR_ACCEPT_OK	EWOULDBLOCK
#define RED_NET_ERR_RECEIVE_OK	EWOULDBLOCK
#define RED_NET_ERR_SEND_OK		EWOULDBLOCK
#define RED_NET_ERR_SEND_NO_BUF	ENOBUFS

#define RED_NET_ERR_CONNECT_INPROGRESS EALREADY
#define RED_NET_ERR_ALREADY_CONNECTED EISCONN

#define RED_NET_ERR_ADDR_IN_USE	EADDRINUSE

#define RED_NET_SHUTDOWN_RECEIVE	SHUT_RD
#define RED_NET_SHUTDOWN_SEND		SHUT_WR
#define RED_NET_SHUTDOWN_BOTH		SHUT_RDWR 

// Structs
namespace red
{
	namespace Network
	{
		typedef int32_t				ErrorCode;

		//////////////////////////////////////////////////////////////////////////
		// Socket
		//////////////////////////////////////////////////////////////////////////
		typedef int32_t				SocketId;
		static const SocketId		InvalidSocket = -1;
		typedef int32_t				ProtocolFamily;

		//////////////////////////////////////////////////////////////////////////
		// Socket Options
		//////////////////////////////////////////////////////////////////////////
		typedef uint32_t			SocketOptionLength;

		typedef uint32_t			SocketOptionReuseAddr;

		//////////////////////////////////////////////////////////////////////////
		// Address
		//////////////////////////////////////////////////////////////////////////
		typedef sockaddr_in			SockaddrIpv4;
		typedef sockaddr_in6		SockaddrIpv6;

		typedef sockaddr			Sockaddr;

		typedef socklen_t			SockaddrLen;

		extern const struct in6_addr&	RED_INADDR6_ANY;

		typedef sa_family_t			AddressFamily;


		// PS4 currently doesn't support IPv6, so we need to create a dummy struct to match its functionality
		struct SockaddrStorage : public sockaddr_storage
		{
			RED_INLINE SockaddrStorage()
			{
				red::Memzero( this, sizeof( SockaddrStorage ) );
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrStorage& other )
			{
				red::Memcpy( this, &other, sizeof( sockaddr_storage ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const Sockaddr& other )
			{
				red::Memcpy( this, &other, sizeof( Sockaddr ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrIpv4& other )
			{
				red::Memcpy( this, &other, sizeof( SockaddrIpv4 ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrIpv6& other )
			{
				red::Memcpy( this, &other, sizeof( SockaddrIpv6 ) );
				return *this;
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// Socket Functions
		//////////////////////////////////////////////////////////////////////////

		namespace Base
		{
			extern REDNETWORK_API SocketId Socket( ProtocolFamily family, Int32 type, Int32 protocol );

			extern REDNETWORK_API Bool SetNonBlocking( SocketId socketDescriptor );

			extern REDNETWORK_API Bool Bind( SocketId socketDescriptor, const Sockaddr* address, SockaddrLen size );

			extern REDNETWORK_API Bool Listen( SocketId socketDescriptor );

			extern REDNETWORK_API Bool Accept( SocketId listenSocketDescriptor, SocketId& newSocketDescriptor, Sockaddr* address, SockaddrLen* size );

			extern REDNETWORK_API Bool Connect( SocketId socketDescriptor, const Sockaddr* address, SockaddrLen size );

			extern REDNETWORK_API Bool Shutdown( SocketId socketDescriptor, int how );

			extern REDNETWORK_API Bool Close( SocketId socketDescriptor );

			extern REDNETWORK_API Int32 Send( SocketId socketDescriptor, const void* buffer, Uint32 size );

			extern REDNETWORK_API Int32 Recv( SocketId socketDescriptor, void* buffer, Uint32 size );

			extern REDNETWORK_API Int32 SendTo( SocketId socketDescriptor, const void* buffer, Uint32 bufferSize, const Sockaddr* address, SockaddrLen addressSize );

			extern REDNETWORK_API Int32 RecvFrom( SocketId socketDescriptor, void* buffer, Uint32 bufferSize, Sockaddr* address, SockaddrLen* addressSize );

			extern REDNETWORK_API Bool GetPeerName( SocketId socketDescriptor, Sockaddr* address, SockaddrLen* size );

			extern REDNETWORK_API Bool SetSocketOption( SocketId socketDescriptor, Int32 level, Int32 option, const void* value, SocketOptionLength length );

			extern REDNETWORK_API Bool GetSocketOption( SocketId socketDescriptor, Int32 level, Int32 option, void* value, SocketOptionLength* length );

			extern REDNETWORK_API ErrorCode GetLastError();

			extern REDNETWORK_API Bool Initialize();

			extern REDNETWORK_API Bool Shutdown();
		}

		//////////////////////////////////////////////////////////////////////////
		// Ip string <-> Data Conversion
		//////////////////////////////////////////////////////////////////////////
		extern REDNETWORK_API red::Int32 INetPtoN( AddressFamily family, const red::AnsiChar* source, void* destination );
		extern REDNETWORK_API red::Int32 INetPtoN( AddressFamily family, const red::UniChar* source, void* destination );

		extern REDNETWORK_API red::Bool INetNtoP( AddressFamily family, const void* source, red::AnsiChar* destination, red::Uint32 destinationSize );
		extern REDNETWORK_API red::Bool INetNtoP( AddressFamily family, const void* source, red::UniChar* destination, red::Uint32 destinationSize );

		//////////////////////////////////////////////////////////////////////////
		// Endian Conversion
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Int8		NetworkToHost( const Int8 data )			{ return data; }
		RED_INLINE Int16	NetworkToHost( const Int16 data )			{ return bswap_16( data ); }
		RED_INLINE Int32	NetworkToHost( const Int32 data )			{ return bswap_32( data ); }
		RED_INLINE Int64	NetworkToHost( const Int64 data )			{ return bswap_64( data ); }

		RED_INLINE Int8		HostToNetwork( const Int8 data )			{ return data; }
		RED_INLINE Int16	HostToNetwork( const Int16 data )			{ return bswap_16( data ); }
		RED_INLINE Int32	HostToNetwork( const Int32 data )			{ return bswap_32( data ); }
		RED_INLINE Int64	HostToNetwork( const Int64 data )			{ return bswap_64( data ); }

		RED_INLINE Uint8	NetworkToHost( const Uint8 data )			{ return data; }
		RED_INLINE Uint16	NetworkToHost( const Uint16 data )			{ return bswap_16( data ); }
		RED_INLINE Uint32	NetworkToHost( const Uint32 data )			{ return bswap_32( data ); }
		RED_INLINE Uint64	NetworkToHost( const Uint64 data )			{ return bswap_64( data ); }

		RED_INLINE Uint8	HostToNetwork( const Uint8 data )			{ return data; }
		RED_INLINE Uint16	HostToNetwork( const Uint16 data )			{ return bswap_16( data ); }
		RED_INLINE Uint32	HostToNetwork( const Uint32 data )			{ return bswap_32( data ); }
		RED_INLINE Uint64	HostToNetwork( const Uint64 data )			{ return bswap_64( data ); }

		RED_INLINE UniChar	NetworkToHost( const UniChar data )			{ return bswap_16( data ); }
		RED_INLINE UniChar	HostToNetwork( const UniChar data )			{ return bswap_16( data ); }
	}
}
