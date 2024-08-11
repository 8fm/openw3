/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once
#ifndef __NETWORKED_REMOTE_CONNECTION_H__
#define __NETWORKED_REMOTE_CONNECTION_H__

#ifdef RED_NETWORK_ENABLED

#include "../redNetwork/channel.h"

/// Remote connection handler
class CRemoteConnection : public Red::Network::ChannelListener
{
public:
	CRemoteConnection();
	~CRemoteConnection();

	// flush pending messages
	void Flush();

private:
	static const AnsiChar* CHANNEL_NAME;
	static const Uint32 CHANNEL_MAGIC;

	struct DelayedMessage
	{
		Uint32		m_transaction;
		StringAnsi	m_command;		
	};

	typedef TDynArray< DelayedMessage >		TDelayedMessages;

	Red::Threads::CMutex	m_lock;
	Uint32					m_lastReceivedTransaction;
	TDelayedMessages		m_messages;

	// Red::Network::ChannelListener interface
	virtual void OnPacketReceived( const Red::System::AnsiChar* channelName, Red::Network::IncomingPacket& packet ) override;

	// Send response
	void Respond( const Uint32 transactionID, const StringAnsi& txt );

	// Process command (asysnc) - can fail for some systems
	const Bool ProcessAsync( const StringAnsi& command, StringAnsi& result ) const;

	// Process command (sync)
	const Bool ProcessSync( const StringAnsi& command, StringAnsi& result ) const;
};

/// Remote connection interface
extern CRemoteConnection* GRemoteConnection;

#endif // RED_NETWORK_ENABLED

#endif
