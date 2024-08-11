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

#include "AkMonitorData.h"

class IALMonitorSubProxy
{
public:
	virtual void Monitor( AkMonitorData::MaskType in_uWhatToMonitor ) = 0;
	virtual void StopMonitor() = 0;
	virtual void SetMeterWatches( AkMonitorData::MeterWatch* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;
	virtual void SetWatches( AkMonitorData::Watch* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;
	virtual void SetGameSyncWatches( AkUniqueID* in_pWatches, AkUInt32 in_uiWatchCount ) = 0;

	enum MethodIDs
	{
		MethodMonitor = 1,
		MethodStopMonitor,
		MethodSetMeterWatches,
		MethodSetWatches,
		MethodSetGameSyncWatches,

		LastMethodID
	};
};

class IALMonitorSubProxyHolder
{
public:
	// Takes ownership of passed-in item, and will eventually call free() on it.
	virtual void MonitorNotification( AkMonitorData::MonitorDataItem * in_pMonitorItem ) = 0;
};

#endif // #ifndef AK_OPTIMIZED
