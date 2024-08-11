/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_PING_H_
#define _RED_NETWORK_PING_H_

#include "../redSystem/settings.h"
#include "../redSystem/types.h"

#ifdef RED_NETWORK_ENABLED

#include "manager.h"
#include "channel.h"

namespace Red
{
	namespace Network
	{
		//////////////////////////////////////////////////////////////////////////
		// Custom Ping/Pong protocol
		//////////////////////////////////////////////////////////////////////////
		class Ping : public ChannelListener
		{
		public:
			Ping();
			virtual ~Ping();

			// Listen and respond to pings
			System::Bool Initialize();

			// Send pings to specified address
			template< typename TChar >
			System::Bool ConnectTo( const TChar* ip, System::Uint16 port );

			System::Bool Send();
			virtual void OnPacketReceived( const System::AnsiChar* channelName, IncomingPacket& packet ) override final;
			
			virtual void OnPongReceived( Double ms ) { RED_UNUSED( ms ); }

		private:
			System::Bool m_initialized;

			static const AnsiChar* CHANNEL;
			static const UniChar* PING;
			static const UniChar* PONG;
		};

		template< typename TChar >
		System::Bool Ping::ConnectTo( const TChar* ip, System::Uint16 port )
		{
			if( m_initialized )
			{
				Red::Network::Address address( ip, port );
				Red::Network::Manager::GetInstance()->ConnectTo( address, CHANNEL );
			}

			return false;
		}
	}
}

#endif // RED_NETWORK_ENABLED

#endif // _RED_NETWORK_PING_H_
