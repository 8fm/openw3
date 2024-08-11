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
#ifndef PROXYCENTRAL_CONNECTED

#include "IALMonitorSubProxy.h"
#include "IALMonitorSink.h"
#include "AkMonitorData.h"

class ALMonitorProxyLocal : public IALMonitorSubProxy
                          , public AK::IALMonitorSink
{
public:
	ALMonitorProxyLocal( IALMonitorSubProxyHolder * in_pHolder );
	virtual ~ALMonitorProxyLocal();

	virtual void Monitor( AkMonitorData::MaskType in_uWhatToMonitor );
	virtual void StopMonitor();
	virtual void SetMeterWatches( AkMonitorData::MeterWatch* in_pWatches, AkUInt32 in_uiWatchCount );
	virtual void SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount );
	virtual void SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount );

	// IALMonitorSink members
	virtual void MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem, bool in_bAccumulate = false );
	virtual void FlushAccumulated();

private:
	IALMonitorSubProxyHolder * m_pHolder;

	bool m_bRegistered;
};

#endif // PROXYCENTRAL_CONNECTED
#endif // AK_OPTIMIZED
