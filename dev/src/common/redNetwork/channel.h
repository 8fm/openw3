/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#ifndef _RED_NETWORK_CHANNEL_H_
#define _RED_NETWORK_CHANNEL_H_

#include "../redSystem/types.h"

#include "../redThreads/redThreadsThread.h"

#include "fixedPoolList.h"

#include "packet.h"
#include "address.h"

namespace Red
{
	namespace Network
	{
		//////////////////////////////////////////////////////////////////////////
		// Packet
		//////////////////////////////////////////////////////////////////////////
		class ChannelPacket : public OutgoingPacket
		{
		public:
			ChannelPacket( const System::AnsiChar* channelName );
			~ChannelPacket();

			void Clear( const System::AnsiChar* channelName );
		};

		//////////////////////////////////////////////////////////////////////////
		// Listener
		//////////////////////////////////////////////////////////////////////////
		class ChannelListener
		{
		public:
			ChannelListener();
			virtual ~ChannelListener();

			virtual void OnPacketReceived( const System::AnsiChar* channelName, IncomingPacket& packet ) = 0;
		};

		//////////////////////////////////////////////////////////////////////////
		// Channel
		//////////////////////////////////////////////////////////////////////////
		class Channel
		{
		private:
			static const System::Uint32 MAX_QUEUED_OUTGOING_PACKETS = 4;
			static const System::Uint32 MAX_DESTINATIONS = 4;
			static const System::Uint32 MAX_LISTENERS = 8;

		public:
			static const System::Uint32 NAME_MAX_LENGTH = 32;

		public:
			typedef FixedPoolList< Socket*, MAX_DESTINATIONS > DestinationList;

		public:
			Channel();
			~Channel();

			void operator=( const Channel& ) {}

			RED_INLINE void SetName( const System::AnsiChar* name ) { System::StringCopy( m_name, name, NAME_MAX_LENGTH ); }
			RED_INLINE const System::AnsiChar* GetName() const { return m_name; }

			void ReceivePacket( IncomingPacket& packet );
			void SendPacket( const OutgoingPacket& packet );

			void RegisterListener( ChannelListener* listener );
			void UnregisterListener( ChannelListener* listener );

			void RegisterDestination( Socket* destination );
			void UnregisterDestination( Socket* destination );

			RED_INLINE DestinationList& GetDestinations() { return m_destinations; }

		private:
			System::AnsiChar m_name[ NAME_MAX_LENGTH ];

			// List of sockets with which to send outgoing packets associated with this channel
			DestinationList m_destinations;

			// List of receivers to which incoming packets on this channel will be sent
			Threads::CMutex m_listenerLock;
			FixedPoolList< ChannelListener*, MAX_LISTENERS > m_listeners;
		};
	}
}

#endif //_RED_NETWORK_CHANNEL_H_
