/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifdef RED_NETWORK_ENABLED

#include "../redNetwork/channel.h"

namespace Config
{
	class CConfigSystem;

	/// Network based remote controller for the config
	class CRemoteConfig : public Red::Network::ChannelListener
	{
	public:
		// Initialize remote config support - should be called once
		static void Initialize();

		// Close remote config support
		static void Shutdown();

	private:
		CRemoteConfig();
		~CRemoteConfig();

		static const AnsiChar* CHANNEL_NAME;
		static const Uint32 CHANNEL_MAGIC;

		static const Uint32 MAX_PACKET_SIZE;

		static CRemoteConfig*	st_instance;
		CConfigSystem*			m_manager;

		// Red::Network::ChannelListener interface
		virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override;

		// Process the packet
		bool ProcessPacket( const AnsiChar* cmd, Red::Network::IncomingPacket& packet );
	};

} // Config

#endif // RED_NETWORK_ENABLED
