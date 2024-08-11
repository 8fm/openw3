/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/


#pragma once

#include "ICommunicationCentral.h"
#include "DiscoveryChannel.h"
#include "CommandChannel.h"
#include "NotificationChannel.h"
#include "IChannelsHolder.h"

#include <AK/Tools/Common/AkObject.h>

class CommunicationCentral 
	: public AK::Comm::ICommunicationCentral
	, public IChannelsHolder
{
public:
	CommunicationCentral( AkMemPoolId in_pool, const AkThreadProperties& in_threadProperties );
	virtual ~CommunicationCentral();

	// ICommunicationCentral members
	virtual void Destroy();

	virtual bool Init( AK::Comm::ICommunicationCentralNotifyHandler* in_pNotifyHandler, AK::Comm::ICommandChannelHandler* in_pCmdChannelHandler, bool in_bInitSystemLib );
	virtual void PreTerm();
	virtual void Term();

	virtual void Process();

	virtual AK::Comm::INotificationChannel* GetNotificationChannel();

	// IChannelsHolder members
	virtual void PeerConnected();
	virtual void PeerDisconnected();

	virtual bool PrepareChannels( const char* in_pszControllerName );
	virtual bool ResetChannels();
	virtual void GetPorts( Ports& out_ports ) const;

	virtual AkMemPoolId GetPool() const {return m_pool;}

private:
	DiscoveryChannel m_discoveryChannel;
	CommandChannel m_commandChannel;
	NotificationChannel m_notifChannel;

	bool m_bInitialized;

	AK::Comm::ICommunicationCentralNotifyHandler* m_pNotifyHandler;

	AkMemPoolId m_pool;
	bool m_bInternalNetworkInit;
};
