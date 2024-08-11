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

#include "stdafx.h"
#ifndef AK_OPTIMIZED

#include "ALMonitorProxyConnected.h"

#include "AkCritical.h"
#include "IALMonitorSubProxy.h"
#include "INotificationChannel.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

#define RESERVED_MEM_FOR_OUTGOING 2048

ALMonitorProxyConnected::ALMonitorProxyConnected()
	: m_pNotificationChannel( NULL) 
{
	//Reserve some memory to be able to send profiling data.
	m_serializer.GetWriter()->Reserve(RESERVED_MEM_FOR_OUTGOING);
}

ALMonitorProxyConnected::~ALMonitorProxyConnected()
{
	if (AK::SoundEngine::GetMonitor())
		AK::SoundEngine::GetMonitor()->Unregister( this );
}

void ALMonitorProxyConnected::MonitorNotification( const AkMonitorData::MonitorDataItem& in_rMonitorItem, bool in_bAccumulate )
{
	AKASSERT( m_pNotificationChannel );

    m_serializer.GetWriter()->Clear();
	if( m_serializer.Put( in_rMonitorItem ) )
	{
		m_pNotificationChannel->SendNotification( m_serializer.GetWrittenBytes(), m_serializer.GetWrittenSize(), in_bAccumulate );
	}
	else
	{
		// OK, we are not sending a notification bcause we don't have suficient memory, that may hurt, the result is kind of hard to predict
		// Wwise profiler may end up in an inconsistent state.
	}
}

void ALMonitorProxyConnected::FlushAccumulated()
{
	AKASSERT( m_pNotificationChannel );
	m_pNotificationChannel->SendAccumulatedData();
}

void ALMonitorProxyConnected::SetNotificationChannel( AK::Comm::INotificationChannel* in_pNotificationChannel )
{
	m_pNotificationChannel = in_pNotificationChannel;
}

void ALMonitorProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rSerializer )
{
	ALMonitorProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	CAkFunctionCritical SpaceSetAsCritical;

	switch( cmdData.m_methodID )
	{
	case IALMonitorSubProxy::MethodMonitor:
		{
			ALMonitorProxyCommandData::Monitor monitor;
			if( in_rSerializer.Get( monitor ) )
				AK::SoundEngine::GetMonitor()->Register( this, monitor.m_uWhatToMonitor );
			break;
		}

	case IALMonitorSubProxy::MethodStopMonitor:
		{
			ALMonitorProxyCommandData::StopMonitor stopMonitor;
			if (in_rSerializer.Get( stopMonitor ))
				AK::SoundEngine::GetMonitor()->Unregister( this );
			// The remote proxy expects an answer.
			out_rSerializer.Put( AK_Success );
			break;
		}

	case IALMonitorSubProxy::MethodSetMeterWatches:
		{
			ALMonitorProxyCommandData::SetMeterWatches setMeterWatches;
			if (in_rSerializer.Get( setMeterWatches ))
				AK::SoundEngine::GetMonitor()->SetMeterWatches( setMeterWatches.m_pWatches, setMeterWatches.m_uiWatchCount );
			break;
		}

	case IALMonitorSubProxy::MethodSetWatches:
		{
			ALMonitorProxyCommandData::SetWatches setWatches;
			if (in_rSerializer.Get( setWatches ))
				AK::SoundEngine::GetMonitor()->SetWatches( setWatches.m_pWatches, setWatches.m_uiWatchCount );
			break;
		}

	case IALMonitorSubProxy::MethodSetGameSyncWatches:
		{
			ALMonitorProxyCommandData::SetGameSyncWatches setGameSyncWatches;
			if( in_rSerializer.Get( setGameSyncWatches ) )
				AK::SoundEngine::GetMonitor()->SetGameSyncWatches( setGameSyncWatches.m_pWatches, setGameSyncWatches.m_uiWatchCount );
			break;
		}

	default:
		AKASSERT( !"Unsupported command." );
	}
}

#endif // #ifndef AK_OPTIMIZED
