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

#ifndef AK_OPTIMIZED

#include "IALMonitorSink.h"
#include "CommandDataSerializer.h"

namespace AK
{
	namespace Comm
	{
		class INotificationChannel;
	}
}

class CommandDataSerializer;

class ALMonitorProxyConnected : public AK::IALMonitorSink
{
public:
	ALMonitorProxyConnected();
	virtual ~ALMonitorProxyConnected();

	// IALMonitorSink members
	virtual void MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem, bool in_bAccumulate = false );
	virtual void FlushAccumulated();
	
	// ALMonitorProxyConnected members
	void SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel );

	void HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rSerializer );

private:
    CommandDataSerializer m_serializer;// making it a member, allowing to save millions of allocations.

	AK::Comm::INotificationChannel* m_pNotificationChannel;
};
#endif // #ifndef AK_OPTIMIZED
