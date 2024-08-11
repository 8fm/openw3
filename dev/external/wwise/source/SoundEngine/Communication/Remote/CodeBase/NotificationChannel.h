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

#include "INotificationChannel.h"
#include "GameSocket.h"
#include "Serializer.h"
#include "IChannelsHolder.h"

class NotificationChannel : public AK::Comm::INotificationChannel
{
public:
	NotificationChannel(IChannelsHolder *in_pHolder);
	virtual ~NotificationChannel();

	// INotificationChannel members
	virtual void SendNotification( const AkUInt8* in_pNotificationData, int in_dataSize, bool in_bAccumulate ) const;
	virtual void SendAccumulatedData() const;

	bool Init();
	void Term();

	bool StartListening();
	void StopListening();

	void Process();

	bool IsReady() const;

	AkUInt16 GetServerPort() const;

private:
	GameSocket m_serverSocket;
	GameSocket m_connSocket;

	IChannelsHolder* m_pHolder;
	mutable bool m_bErrorProcessingConnection;

    mutable Serializer m_serializer;
};
