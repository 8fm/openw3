/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "ping.h"

#ifdef RED_NETWORK_ENABLED

namespace Red { namespace Network {

const AnsiChar* Ping::CHANNEL	= "Utility";
const UniChar* Ping::PING		= L"Ping";
const UniChar* Ping::PONG		= L"Pong";

Ping::Ping()
:	m_initialized( false )
{

}

Ping::~Ping()
{
	if( m_initialized && Manager::GetInstance() )
	{
		Manager::GetInstance()->UnregisterListener( CHANNEL, this );
		m_initialized = false;
	}
}

System::Bool Ping::Initialize()
{
	if( Manager::GetInstance() && !m_initialized )
	{
		m_initialized = Manager::GetInstance()->RegisterListener( CHANNEL, this );
	}

	return m_initialized;
}

System::Bool Ping::Send()
{
	if( m_initialized )
	{
		Manager* manager = Manager::GetInstance();
		RED_FATAL_ASSERT( manager, "Network manager is not initialised!" );

		if( manager->IsInitialized() )
		{
			RED_LOG( RED_LOG_CHANNEL( NetUtil ), TXT( "Sending %ls!" ), PING );

			Uint64 ticksAtSend = 0;
			System::Clock::GetInstance().GetTimer().GetTicks( ticksAtSend );

			Red::Network::ChannelPacket packet( CHANNEL );

			RED_VERIFY( packet.WriteString( PING ) );
			RED_VERIFY( packet.Write( ticksAtSend ), TXT( "Couldn't write time of initial send" )  );

			return manager->Send( CHANNEL, packet );
		}
	}

	return false;
}

void Ping::OnPacketReceived( const System::AnsiChar* channelName, IncomingPacket& packet )  
{
	RED_FATAL_ASSERT( Red::System::StringCompare( channelName, CHANNEL ) == 0, "Wrong channel sent to ping utility '%hs' expected '%hs'", channelName, CHANNEL );
	RED_UNUSED( channelName );

	UniChar type[ 8 ];
	Uint64 ticksAtSend = 0;

	RED_VERIFY( packet.ReadString( type, ARRAY_COUNT_U32( type ) ), TXT( "Couldn't fit string into 4 character array - string isn't ping or pong" ) );
	RED_VERIFY( packet.Read( ticksAtSend ), TXT( "Couldn't read time of initial send" ) );

	if( Red::System::StringCompare( type, PING ) == 0 )
	{
		Red::Network::ChannelPacket packet( CHANNEL );

		RED_VERIFY( packet.WriteString( PONG ) );
		RED_VERIFY( packet.Write( ticksAtSend ), TXT( "Couldn't write time of initial send" )  );

		Manager* manager = Manager::GetInstance();
		RED_FATAL_ASSERT( manager, "Network manager is not initialised!" );

		manager->Send( CHANNEL, packet );
	}
	else if( Red::System::StringCompare( type, PONG ) == 0 )
	{
		Uint64 ticksAtReceive = 0;
		System::Clock::GetInstance().GetTimer().GetTicks( ticksAtReceive );

		Uint64 frequency = 0;
		System::Clock::GetInstance().GetTimer().GetFrequency( frequency );
		Double ticksPerMillisecond = frequency / 1000.0;

		Double ms = ( ticksAtReceive - ticksAtSend ) / ticksPerMillisecond;

		RED_LOG( RED_LOG_CHANNEL( Ping ), TXT( "%ls received: %.2Lfms" ), PONG, ms );

		OnPongReceived( ms );
	}
}

} } // namespace Red { namespace Network {

#endif // RED_NETWORK_ENABLED
