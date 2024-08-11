/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_ADDRESS_H_
#define _RED_NETWORK_ADDRESS_H_

#include "../redSystem/os.h"
#include "../redSystem/types.h"
#include "../redSystem/crt.h"

#include "network.h"

namespace Red
{
	namespace Network
	{
		class Address
		{
		public:

			static const System::Uint32 MAX_IP_STRING_LENGTH = RED_NET_MAX_ADDRSTRLEN;

			enum EFamily
			{
				Family_IPv4 = RED_NET_AF_INET,
				Family_IPv6 = RED_NET_AF_INET6
			};

			Address();

			Address( Red::System::Uint16 port, EFamily family );

			Address( const System::AnsiChar* ip, System::Uint16 port, EFamily family = Family_IPv4 );
			Address( const System::UniChar* ip, System::Uint16 port, EFamily family = Family_IPv4 );
			
			Address( const Address& address );

			Address( const SockaddrIpv4& ip4address );
			Address( const SockaddrIpv6& ip6address );
			Address( const Sockaddr& addr );
			Address( const SockaddrStorage& addr );

			RED_INLINE const Address& operator=( const Address& other )
			{
				m_address = other.m_address;
				return *this;
			}

			RED_INLINE const Address& operator=( const SockaddrIpv4& other )
			{
				m_address = other;
				return *this;
			}

			RED_INLINE const Address& operator=( const SockaddrIpv6& other )
			{
				m_address = other;
				return *this;
			}

			RED_INLINE const Address& operator=( const Sockaddr& other )
			{
				m_address = other;
				return *this;
			}

			RED_INLINE const Address& operator=( const SockaddrStorage& other )
			{
				m_address = other;
				return *this;
			}

			RED_INLINE System::Bool IsIPv4() const
			{
				return m_address.ss_family == RED_NET_AF_INET;
			}

			RED_INLINE Sockaddr* GetNative()
			{
				return reinterpret_cast< Sockaddr* >( &m_address );
			}

			RED_INLINE const Sockaddr* GetNative() const
			{
				return reinterpret_cast< const Sockaddr* >( &m_address );
			}

			RED_INLINE SockaddrLen GetNativeSize() const
			{
				return ( IsIPv4() )? sizeof( SockaddrIpv4 ) : sizeof( SockaddrIpv6 );
			}

			void SetPort( System::Uint16 port );

			RED_INLINE System::Bool operator==( const Address& other ) const
			{
				SockaddrLen size = GetNativeSize();
				return size == other.GetNativeSize() && System::MemoryCompare( &m_address, &other.m_address, size ) == 0;
			}

			System::Bool GetIp( System::AnsiChar* buffer, System::Uint32 size ) const;
			System::Uint16 GetPort() const;

		private:
			const void* GetIPStorage() const;
			RED_INLINE void* GetIPStorage() { return const_cast< void* >( static_cast< const Address* >( this )->GetIPStorage() ); }

		private:
			SockaddrStorage m_address;
		};
	}
}

#endif // _RED_NETWORK_ADDRESS_H_
