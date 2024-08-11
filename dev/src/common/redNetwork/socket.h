/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_SOCKET_WINDOWS_H_
#define _RED_NETWORK_SOCKET_WINDOWS_H_

#include "../redSystem/types.h"
#include "../redSystem/os.h"

#include "network.h"
#include "address.h"

namespace Red
{
	namespace Network
	{
		class Socket
		{
		public:

			enum State
			{
				State_Uninitialised = 0,
				State_Unbound,
				State_Bound,
				State_Listening,
				State_Connecting,
				State_Connected,

				State_Dropped,		//!< Connection timed out
				State_Closed,		//!< Connection closed gracefully

				State_Max
			};

			enum AddressType
			{
				IPv4 = 0,
				IPv6
			};

			enum Protocol
			{
				TCP = 0,
				UDP
			};

			Socket();
			Socket( SocketId connectedSocket, State state, const Address& peer );

			void operator=( const Socket& other );

			~Socket();

			// Interface
			System::Bool Create( Protocol protocol = TCP, AddressType addressType = IPv4 );
			System::Bool Bind( const Address& address );
			System::Bool Bind( System::Uint16 port );
			System::Bool Listen();
			Socket Accept();
			System::Bool Connect( const Address& destination );

			System::Bool FinishConnecting();

			System::Uint16 Send( const void* buffer, System::Uint32 size );
			System::Uint16 Receive( void* buffer, System::Uint32 size );

			System::Uint16 SendTo( const void* buffer, System::Uint32 size, const Address& destination );
			System::Uint16 ReceiveFrom( void* buffer, System::Uint32 size, Address& source );

			System::Bool SetOptionReuseAddress( System::Bool enabled );

			RED_INLINE void Close() { Close( State_Closed ); }

			RED_INLINE System::Bool IsOpen() const { return static_cast< System::Uint32 >( m_state ) >= State_Bound; }
			RED_INLINE System::Bool IsUnbound() const { return m_state == State_Unbound; }
			RED_INLINE System::Bool IsBound() const { return m_state == State_Bound; }
			RED_INLINE System::Bool IsListening() const { return m_state == State_Listening; }
			RED_INLINE System::Bool IsConnected() const { return m_state == State_Connected; }
			RED_INLINE System::Bool IsConnecting() const { return m_state == State_Connecting; }
			RED_INLINE System::Bool ConnectionClosed() const { return m_state == State_Closed; }
			RED_INLINE System::Bool ConnectionDropped() const { return m_state == State_Dropped; }

			System::Bool IsReady() const;

			RED_INLINE System::Bool operator==( const Socket& other ) const { return m_descriptor == other.m_descriptor; }

			RED_INLINE const Address& GetPeer() const { return m_peer; }

			RED_INLINE SocketId GetRawDescriptor() const { return m_descriptor; }

		private:
			System::Bool GetPeer( Address& peer ) const;
			void Close( State resultantState );

		private:
			SocketId m_descriptor;
			State m_state;
			Protocol m_protocol;
			System::Int32 m_lastErrorCode;
			
			Address m_peer;
		};
	}
}

#endif //_RED_NETWORK_SOCKET_WINDOWS_H_
