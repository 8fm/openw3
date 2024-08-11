/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#ifdef RED_NETWORK_ENABLED

#include "networkedLogOutputDevice.h"

CNetworkedLogOutputDevice::CNetworkedLogOutputDevice()
:	m_networkSystem( nullptr )
{
	SetUnsafeToCallOnCrash();
}

CNetworkedLogOutputDevice::~CNetworkedLogOutputDevice()
{

}

void CNetworkedLogOutputDevice::Write( const Red::System::Log::Message& message )
{
	if( m_networkSystem && m_networkSystem->IsInitialized() )
	{
		Red::Network::ChannelPacket packet( "log" );

		// Channel
 		packet.WriteString( message.channelText );

		// Priority
		packet.Write( static_cast< Uint8 >( message.priority ) );

		// Timestamp
		packet.Write( message.dateTime.GetRaw() );

		// Tick
		packet.Write( message.tick );

		// Message
		packet.WriteString( message.text );

		m_networkSystem->Send( "log", packet );
	}
}

#endif // RED_NETWORK_ENABLED
