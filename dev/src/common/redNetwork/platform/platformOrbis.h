/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef __RED_NETWORK_ORBIS_WRAPPERS_H__
#define __RED_NETWORK_ORBIS_WRAPPERS_H__

// System Includes
#include <net.h>

#pragma comment( lib, "libSceNet_stub_weak.a" )
#pragma comment( lib, "libScePosix_stub_weak.a" )

#include "../../redSystem/crt.h"
#include "../../redSystem/error.h"

// Enumerations and defines

#define RED_NET_MAX_ADDRSTRLEN	SCE_NET_INET_ADDRSTRLEN

#define RED_NET_INADDR_ANY		SCE_NET_INADDR_ANY

#define RED_NET_PF_INET			SCE_NET_AF_INET
#define RED_NET_PF_INET6		(SceNetSaFamily_t)-1

#define RED_NET_AF_INET			SCE_NET_AF_INET
#define RED_NET_AF_INET6		(SceNetSaFamily_t)-1

#define RED_NET_SOCK_STREAM		SCE_NET_SOCK_STREAM
#define RED_NET_IPPROTO_TCP		SCE_NET_IPPROTO_TCP

#define RED_NET_SOCK_DGRAM		SCE_NET_SOCK_DGRAM
#define RED_NET_IPPROTO_UDP		SCE_NET_IPPROTO_UDP

#define RED_NET_SO_ERROR		SCE_NET_SO_ERROR
#define RED_NET_SOL_SOCKET		SCE_NET_SOL_SOCKET

#define RED_NET_SO_REUSEADDR	SCE_NET_SO_REUSEADDR

#define RED_NET_LISTEN_BACKLOG	10

#define RED_NET_ERR_CONNECT_OK	SCE_NET_EINPROGRESS
#define RED_NET_ERR_ACCEPT_OK	SCE_NET_EWOULDBLOCK
#define RED_NET_ERR_RECEIVE_OK	SCE_NET_EWOULDBLOCK
#define RED_NET_ERR_SEND_OK		SCE_NET_EWOULDBLOCK

#define RED_NET_ERR_ADDR_IN_USE	SCE_NET_EADDRINUSE

// Structs
namespace Red
{
	namespace Network
	{
		typedef int					ErrorCode;

		//////////////////////////////////////////////////////////////////////////
		// Socket
		//////////////////////////////////////////////////////////////////////////
		typedef SceNetId			SocketId;
		static const SocketId		InvalidSocket = -1;
		typedef int					ProtocolFamily;

		//////////////////////////////////////////////////////////////////////////
		// Socket Options
		//////////////////////////////////////////////////////////////////////////
		typedef int					SocketOptionLength;

		typedef int					SocketOptionReuseAddr;

		//////////////////////////////////////////////////////////////////////////
		// Address
		//////////////////////////////////////////////////////////////////////////
		typedef SceNetSockaddrIn	SockaddrIpv4;
		typedef SceNetSockaddr		Sockaddr;
		typedef SceNetSaFamily_t	AddressFamily;

		typedef SceNetSocklen_t		SockaddrLen;

		// PS4 currently doesn't support IPv6, so we'll have some dummy structs for now
		struct in6_addr
		{
			unsigned char   s6_addr[16];   // load with inet_pton()

			in6_addr() {}
		};

		struct SockaddrIpv6
		{
			u_int16_t       sin6_family;   // address family, AF_INET6
			u_int16_t       sin6_port;     // port number, Network Byte Order
			u_int32_t       sin6_flowinfo; // IPv6 flow information
			in6_addr		sin6_addr;     // IPv6 address
			u_int32_t       sin6_scope_id; // Scope ID
		};

		const in6_addr		RED_INADDR6_ANY;

		// Wrapper for sockaddr_storage
		// This exists because Orbis doesn't support IPv6
		struct SockaddrStorage : public SceNetSockaddr
		{
			AddressFamily& ss_family;

			RED_INLINE SockaddrStorage() : ss_family( sa_family )
			{
				System::MemoryZero( this, sizeof( SceNetSockaddr ) );
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrStorage& other )
			{
				static_assert( sizeof *this >= sizeof other, "Something is seriously wrong with the size of SockaddrStorage" );

				System::MemoryCopy( this, &other, sizeof( SceNetSockaddr ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const Sockaddr& other )
			{
				static_assert( sizeof *this >= sizeof other, "Sockaddr should always be smaller than SockaddrStorage, as it's technically only big enough for IPv4" );

				System::MemoryCopy( this, &other, sizeof( Sockaddr ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrIpv4& other )
			{
				static_assert( sizeof *this >= sizeof other, "SockaddrStorage should always be bigger than SockaddrIpv4, as it can also contain IPv6 addresses" );

				System::MemoryCopy( this, &other, sizeof( SockaddrIpv4 ) );
				return *this;
			}

			RED_INLINE const SockaddrStorage& operator=( const SockaddrIpv6& other )
			{
				// Once Orbis *does* support IPv6, we can remove this custom SockaddrStorage
				// class and replace it with a simple typedef (For all platforms)
				RED_HALT( "ORBIS DOES NOT SUPPORT IPv6" );

				System::MemoryCopy( this, &other, sizeof( SockaddrIpv6 ) );
				return *this;
			}
		};

		//////////////////////////////////////////////////////////////////////////
		// Socket Functions
		//////////////////////////////////////////////////////////////////////////

		namespace Base
		{
			RED_INLINE SocketId Socket( AddressFamily family, System::Int32 type, System::Int32 protocol )
			{
				return sceNetSocket( nullptr, family, type, protocol );
			}

			RED_INLINE System::Bool SetNonBlocking( SocketId socketDescriptor )
			{
				int on = 1;
				return sceNetSetsockopt( socketDescriptor, SCE_NET_SOL_SOCKET, SCE_NET_SO_NBIO, &on, sizeof( int ) ) == 0;
			}

			RED_INLINE System::Bool Bind( SocketId socketDescriptor, const Sockaddr* address, SockaddrLen size )
			{
				return sceNetBind( socketDescriptor, address, size ) == 0;
			}

			RED_INLINE System::Bool Listen( SocketId socketDescriptor )
			{
				return sceNetListen( socketDescriptor, RED_NET_LISTEN_BACKLOG ) == 0;
			}

			RED_INLINE System::Bool Accept( SocketId listenSocketDescriptor, SocketId& newSocketDescriptor, Sockaddr* address, SockaddrLen* size )
			{
				newSocketDescriptor = sceNetAccept( listenSocketDescriptor, address, size );
				return newSocketDescriptor >= 0;
			}

			RED_INLINE System::Bool Connect( SocketId socketDescriptor, const Sockaddr* address, SockaddrLen size )
			{
				return sceNetConnect( socketDescriptor, address, size ) == 0;
			}

			RED_INLINE System::Bool Close( SocketId socketDescriptor )
			{
				return sceNetSocketClose( socketDescriptor ) == 0;
			}

			RED_INLINE System::Int32 Send( SocketId socketDescriptor, const void* buffer, System::Uint32 size )
			{
				return sceNetSend( socketDescriptor, buffer, size, 0 );
			}

			RED_INLINE System::Int32 Recv( SocketId socketDescriptor, void* buffer, System::Uint32 size )
			{
				return sceNetRecv( socketDescriptor, buffer, size, 0 );
			}

			RED_INLINE System::Int32 SendTo( SocketId socketDescriptor, const void* buffer, System::Uint32 bufferSize, const Sockaddr* address, SockaddrLen addressSize )
			{
				return sceNetSendto( socketDescriptor, buffer, bufferSize, 0, address, addressSize );
			}

			RED_INLINE System::Int32 RecvFrom( SocketId socketDescriptor, void* buffer, System::Uint32 bufferSize, Sockaddr* address, SockaddrLen* addressSize )
			{
				return sceNetRecvfrom( socketDescriptor, buffer, bufferSize, 0, address, addressSize );
			}

			RED_INLINE System::Bool GetPeerName( SocketId socketDescriptor, Sockaddr* address, SockaddrLen* size )
			{
				return sceNetGetpeername( socketDescriptor, address, size ) == 0;
			}

			RED_INLINE System::Bool SetSocketOption( SocketId socketDescriptor, System::Int32 level, System::Int32 option, const void* value, SocketOptionLength length )
			{
				return sceNetSetsockopt( socketDescriptor, level, option, value, length ) == 0;
			}

			RED_INLINE ErrorCode GetLastError()
			{
				return sce_net_errno;
			}

			RED_INLINE System::Bool Initialize()
			{
				return sceNetInit() == 0;
			}

			RED_INLINE System::Bool Shutdown()
			{
				return sceNetTerm() == 0;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Ip String <-> Data Conversion
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Red::System::Int32 INetPtoN( AddressFamily family, const Red::System::AnsiChar* source, void* destination ) { return sceNetInetPton( family, source, destination ); }
		RED_INLINE Red::System::Int32 INetPtoN( AddressFamily family, const Red::System::UniChar* source, void* destination )
		{
			Red::System::AnsiChar convertedIp[ RED_NET_MAX_ADDRSTRLEN ];
			Red::System::WideCharToStdChar( convertedIp, source, RED_NET_MAX_ADDRSTRLEN );

			return sceNetInetPton( family, convertedIp, destination );
		}

		RED_INLINE Red::System::Bool INetNtoP( AddressFamily family, const void* source, Red::System::AnsiChar* destination, Red::System::Uint32 destinationSize )
		{
			return ( sceNetInetNtop( family, source, destination, destinationSize ) )? true : false;
		}


		//////////////////////////////////////////////////////////////////////////
		// Endian Conversion
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE System::Int8		NetworkToHost( const System::Int8 data )			{ return data; }
		RED_INLINE System::Int32	NetworkToHost( const System::Int32 data )			{ return sceNetNtohl( data ); }
		RED_INLINE System::Int16	NetworkToHost( const System::Int16 data )			{ return sceNetNtohs( data ); }
		RED_INLINE System::Int64	NetworkToHost( const System::Int64 data )			{ return sceNetNtohll( data ); }

		RED_INLINE System::Int8		HostToNetwork( const System::Int8 data )			{ return data; }
		RED_INLINE System::Int16	HostToNetwork( const System::Int16 data )			{ return sceNetHtons( data ); }
		RED_INLINE System::Int32	HostToNetwork( const System::Int32 data )			{ return sceNetHtonl( data ); }
		RED_INLINE System::Int64	HostToNetwork( const System::Int64 data )			{ return sceNetHtonll( data ); }

		RED_INLINE System::Uint8	NetworkToHost( const System::Uint8 data )			{ return data; }
		RED_INLINE System::Uint32	NetworkToHost( const System::Uint32 data )			{ return sceNetNtohl( data ); }
		RED_INLINE System::Uint16	NetworkToHost( const System::Uint16 data )			{ return sceNetNtohs( data ); }
		RED_INLINE System::Uint64	NetworkToHost( const System::Uint64 data )			{ return sceNetNtohll( data ); }

		RED_INLINE System::Uint8	HostToNetwork( const System::Uint8 data )			{ return data; }
		RED_INLINE System::Uint16	HostToNetwork( const System::Uint16 data )			{ return sceNetHtons( data ); }
		RED_INLINE System::Uint32	HostToNetwork( const System::Uint32 data )			{ return sceNetHtonl( data ); }
		RED_INLINE System::Uint64	HostToNetwork( const System::Uint64 data )			{ return sceNetHtonll( data ); }

		RED_INLINE System::UniChar	NetworkToHost( const System::UniChar data )			{ return sceNetNtohs( data ); }
		RED_INLINE System::UniChar	HostToNetwork( const System::UniChar data )			{ return sceNetHtons( data ); }
	}
}

#endif // __RED_NETWORK_ORBIS_WRAPPERS_H__
