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

/***************************************************************************************************
**
** AkProfile.cpp
**
***************************************************************************************************/
#include "stdafx.h"

#ifndef AK_OPTIMIZED

#include "AkProfile.h"

#include <limits.h>

#include "AkLEngine.h"
#include "AkMath.h"
#include "AkMonitor.h"
#include "AkAudioMgr.h"         // For upper engine access.
#include "AkURenderer.h"        // For lower engine access.
#include "AkAudioLibTimer.h"
#include "AkFxBase.h"
#include "AkTransitionManager.h"
#include "AkRegistryMgr.h"
#include "AkSink.h"
#include "AkAudioLib.h"
#include "AkCritical.h"
#include "AkPlayingMgr.h"

#ifdef AK_MOTION
	#include "AkFeedbackMgr.h"
	#include "AkFeedbackBus.h"
#endif // AK_MOTION

#if defined(AK_VITA) && defined(AK_ENABLE_VITACODECENGINEPERFMONITORING)
	#include <codecengine.h>
#endif

#if !defined(AK_OPTIMIZED)
bool g_bOfflineRendering = false;
AkChannelMask g_eOfflineSpeakerConfig = AK_SPEAKER_SETUP_STEREO;
#endif

/***************************************************************************************************
**
** AkPerf
**
***************************************************************************************************/

AkInt64 AkPerf::m_iLastUpdateAudio = {0};

AkInt64 AkPerf::m_iLastUpdatePlugins = {0};
AkInt64 AkPerf::m_iLastUpdateMemory = {0};
AkInt64 AkPerf::m_iLastUpdateStreaming = {0};

AkUInt32 AkPerf::m_ulPreparedEvents = 0;
AkUInt32 AkPerf::m_ulBankMemory = 0;
AkUInt32 AkPerf::m_ulPreparedEventMemory = 0;

AkInt32 AkPerf::m_iTicksPerPerfBlock = 0;
AkInt32 AkPerf::m_iNextPerf = 0;
AkInt32 AkPerf::m_iTicksPerCursor = 0;
AkInt32 AkPerf::m_iNextCursor = 0;

void AkPerf::Init()
{
	AkUInt64 iNow;
	AKPLATFORM::PerformanceCounter( (AkInt64*)&iNow );
	m_iLastUpdateAudio = iNow;

	m_iLastUpdatePlugins = iNow;
	m_iLastUpdateMemory = iNow;
	m_iLastUpdateStreaming = iNow;

	m_iTicksPerPerfBlock = AkMax(AK_PROFILE_PERF_INTERVALS / AK_MS_PER_BUFFER_TICK, 1);	//Approximate one perf packet per 200 ms.
	m_iNextPerf = 0;

	m_iTicksPerCursor = AkMax(m_iTicksPerPerfBlock / 4, 1);	//Have 4 times more cursor updates.
	m_iNextCursor = 0;
}

void AkPerf::Term()
{

}

void AkPerf::TickAudio()
{
	if ( !AkMonitor::GetNotifFilter() )
		return;

	// Protect access to Upper Engine data -- Might not be necessary, but better be safe.
	CAkFunctionCritical GlobalLock;

	////////////////////////////////////////////////////////////////////////
	// Section 1
	// Perform at every iterations:
	////////////////////////////////////////////////////////////////////////

	if( (AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataMeter ) ) )
	{	
		// Send meter watches
		CAkLEngine::PostMeterWatches();
	}

	////////////////////////////////////////////////////////////////////////
	// Section 2
	// Don't do anything if it's not time yet.
	////////////////////////////////////////////////////////////////////////
	AkUInt64 iNow;
	AKPLATFORM::PerformanceCounter( (AkInt64*)&iNow );

	// High-frequency pipeline stats for proper HDR monitoring.
	PostPipelineStats( iNow );

	m_iNextPerf--;
	m_iNextCursor--;

	bool bDoAudioPerf = ( (AkMonitor::GetNotifFilter() & 
			( AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataAudioPerf ) 
			| AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataFeedback ) 
			| AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataPluginTimer ) ) 
		) && m_iNextPerf <= 0 );

	if ( m_iNextCursor > 0 && !bDoAudioPerf)
		return;

	m_iNextCursor = m_iTicksPerCursor;

	// Don't do anything if nobody's listening to perf.
	if ( bDoAudioPerf )
	{
		m_iNextPerf = m_iTicksPerPerfBlock;

#ifdef AK_MOTION
		//Stamp the feedback timer if we need it either with audio, plugin or feedback perf data.
		AkAudiolibTimer::timerFeedback.Stamp();
#endif // AK_MOTION

		AkReal32 fInterval = (AkReal32) ( ( iNow - m_iLastUpdateAudio ) / AK::g_fFreqRatio );

		// Post primary lower thread info.
		{
			AkUInt32 sizeofData = SIZEOF_MONITORDATA( audioPerfData );
			AkProfileDataCreator creator( sizeofData );
			if ( !creator.m_pData )
				return;

			AkAudiolibTimer::timerAudio.Stamp();			
#ifdef AK_PS4
			AkAudiolibTimer::timerDsp.Stamp();
#endif
			creator.m_pData->eDataType = AkMonitorData::MonitorDataAudioPerf;

			// voice info
			g_pTransitionManager->GetTransitionsUsage( 
				creator.m_pData->audioPerfData.numFadeTransitionsUsed, 
				creator.m_pData->audioPerfData.maxFadeNumTransitions,
				creator.m_pData->audioPerfData.numStateTransitionsUsed, 
				creator.m_pData->audioPerfData.maxStateNumTransitions 
				);

			// global performance timers
			creator.m_pData->audioPerfData.numRegisteredObjects = (AkUInt16)( g_pRegistryMgr->NumRegisteredObject() );
			creator.m_pData->audioPerfData.numPlayingIDs = (AkUInt16)( g_pPlayingMgr->NumPlayingIDs() );

			creator.m_pData->audioPerfData.timers.fInterval = fInterval;
			creator.m_pData->audioPerfData.timers.fAudioThread = AkAudiolibTimer::timerAudio.Millisecs();
		
#ifdef AK_MOTION
			//Remove the portion that belongs to the feedback pipeline
			creator.m_pData->audioPerfData.timers.fAudioThread -= AkAudiolibTimer::timerFeedback.Millisecs();
#endif // AK_MOTION

			// Message queue stats
			creator.m_pData->audioPerfData.uCommandQueueActualSize     = g_pAudioMgr->GetActualQueueSize();
			creator.m_pData->audioPerfData.fCommandQueuePercentageUsed = g_pAudioMgr->GetPercentageQueueFilled()*100;

			// Update DSP usage
#if defined(AK_WII_FAMILY_HW)
			creator.m_pData->audioPerfData.fDSPUsage = AXGetDspCycles() / (AkReal32)AXGetMaxDspCycles();
#elif defined(AK_3DS)
			creator.m_pData->audioPerfData.fDSPUsage = nn::snd::CTR::GetDspCycles() / (AkReal32)nn::snd::CTR::GetMaximumDspCycles();
#elif defined(AK_VITA) && defined(AK_ENABLE_VITACODECENGINEPERFMONITORING)
			SceCodecEnginePmonProcessorLoad load;
			load.size = sizeof( load );
			sceCodecEnginePmonStop();
			sceCodecEnginePmonGetProcessorLoad( &load );
			sceCodecEnginePmonReset();

			creator.m_pData->audioPerfData.fDSPUsage = (float) load.average / 100.0f;
			sceCodecEnginePmonStart();
#elif defined(AK_PS4)
			creator.m_pData->audioPerfData.fDSPUsage = AkAudiolibTimer::timerDsp.Millisecs() / fInterval;
#else
			creator.m_pData->audioPerfData.fDSPUsage = 0;
#endif

			creator.m_pData->audioPerfData.uNumPreparedEvents	= m_ulPreparedEvents;
			creator.m_pData->audioPerfData.uTotalMemBanks		= m_ulBankMemory;
			creator.m_pData->audioPerfData.uTotalPreparedMemory	= m_ulPreparedEventMemory;
			creator.m_pData->audioPerfData.uTotalMediaMemmory	= m_ulPreparedEventMemory + m_ulBankMemory;
		}

		// Auxiliaries, sorted in order of importance. last ones are more likely to be skipped.

		PostMemoryStats( iNow );
		PostPluginTimers( iNow );
		PostStreamingStats( iNow );
#ifdef AK_MOTION
		PostFeedbackStats(fInterval);
#endif // AK_MOTION
		PostEnvironmentStats();
		PostSendsStats();
		PostObsOccStats();
		PostListenerStats();
		PostControllerStats();
#ifdef AK_MOTION
		PostFeedbackDevicesStats();
#endif // AK_MOTION
		PostOutputStats();
		PostGameObjPositions();
		PostWatchGameSyncValues();

		m_iLastUpdateAudio = iNow;
	}

	// This is not perf information, we have to send it to Wwise, even if it is not listening specifically to perfs.
	// It is used to post position of playing segments at regular time intervals.
	PostInteractiveMusicInfo();
}

void AkPerf::PostPluginTimers( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataPluginTimer ) ) )
		return;

	AkUInt32 sizeofItem = SIZEOF_MONITORDATA_TO(pluginTimerData.pluginData)
		+ AkAudiolibTimer::g_PluginTimers.Length() * sizeof( AkMonitorData::PluginTimerData );
		
	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataPluginTimer;

	creator.m_pData->pluginTimerData.ulNumTimers = AkAudiolibTimer::g_PluginTimers.Length();
    creator.m_pData->pluginTimerData.fInterval = (AkReal32) ( ( in_iNow - m_iLastUpdatePlugins ) / AK::g_fFreqRatio );

	int iPlugin = 0;
	for ( AkAudiolibTimer::AkTimerMap::Iterator it = AkAudiolibTimer::g_PluginTimers.Begin(); it != AkAudiolibTimer::g_PluginTimers.End(); ++it )
	{
		AkMonitorData::PluginTimerData & data = creator.m_pData->pluginTimerData.pluginData[ iPlugin++ ];

		(*it).item.timer.Stamp();

		data.uiPluginID = (*it).key;
		data.fMillisecs = (*it).item.timer.Millisecs();
		data.uiNumInstances = (*it).item.uNumInstances;
	}

	m_iLastUpdatePlugins = in_iNow;
}

void AkPerf::PostMemoryStats( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataMemoryPool ) ) )
		return;

	AkUInt32 ulNumPools = AK::MemoryMgr::GetMaxPools();
	AkUInt32 sizeofItem = SIZEOF_MONITORDATA_TO(memoryData.poolData)
		+ ulNumPools * sizeof( AkMonitorData::MemoryPoolData );
		
	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataMemoryPool;

	creator.m_pData->memoryData.ulNumPools = ulNumPools;

	for ( AkUInt32 ulPool = 0; ulPool < ulNumPools; ++ulPool )
	{
		MemoryMgr::PoolStats & stats = ((MemoryMgr::PoolStats *) creator.m_pData->memoryData.poolData)[ ulPool ];
        MemoryMgr::GetPoolStats( ulPool, stats );
	}

	m_iLastUpdateMemory = in_iNow;
}

void AkPerf::PostStreamingStats( AkInt64 in_iNow )
{
	if ( !( AkMonitor::GetNotifFilter() & 
			( AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataStreaming )
			| AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataStreamsRecord )
			| AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataDevicesRecord ) ) ) )
		return;

    // ALGORITHM:
    // Try to reserve room needed to send new device records.
    // If succeeded, get them and send IF ANY.
    // Try to reserve room needed to send new streams records.
    // If succeeded, get them and send IF ANY.
    // Try yo reserve room to send stream data.
    // If succeeded, get them and send ALWAYS.

    AKASSERT( AK::IAkStreamMgr::Get( ) );
    AK::IAkStreamMgrProfile * pStmMgrProfile = AK::IAkStreamMgr::Get( )->GetStreamMgrProfile( );
    if ( pStmMgrProfile == NULL )
        return; // Profiling interface not implemented in that stream manager.

    unsigned int uiNumDevices = pStmMgrProfile->GetNumDevices( );
	uiNumDevices = AkMin( AK_MAX_DEVICE_NUM, uiNumDevices );

	class AkDeviceProfileData
	{
	public:
		AkDeviceProfileData() 
			:pDevice( NULL )
		{}

		~AkDeviceProfileData()
		{
			if ( pDevice )
				pDevice->OnProfileEnd();
		}

		inline AK::IAkDeviceProfile * GetDevice() { return pDevice; }

		inline void SetDevice( AK::IAkDeviceProfile * in_pDevice )
		{
			AKASSERT( in_pDevice && !pDevice );
			in_pDevice->OnProfileStart();
			pDevice = in_pDevice;
		}

		unsigned int        uiNumStreams;

	protected:
		AK::IAkDeviceProfile *  pDevice;
	};
    AkDeviceProfileData pDevices[ AK_MAX_DEVICE_NUM ];
    unsigned int uiNumStreams = 0;

    for ( unsigned int uiDevice=0; uiDevice<uiNumDevices; uiDevice++ )
    {
        AK::IAkDeviceProfile * pDevice = pStmMgrProfile->GetDeviceProfile( uiDevice );
        AKASSERT( pDevice != NULL );
		pDevices[uiDevice].SetDevice( pDevice );

        // Get record if new.
        if ( pDevice->IsNew( ) )
        {
            // Send every new device.
            AkInt32 sizeofItem = SIZEOF_MONITORDATA(deviceRecordData);

            AkProfileDataCreator creator( sizeofItem );
	        if ( !creator.m_pData )
		        return;

            // Can send. Get actual data.
            creator.m_pData->eDataType = AkMonitorData::MonitorDataDevicesRecord;

            pDevice->GetDesc( creator.m_pData->deviceRecordData );
            pDevice->ClearNew( );

            // Send now.
        }

        unsigned int uiNumStmsDevice = pDevice->GetNumStreams( );

        // Store.
        uiNumStreams += pDevices[uiDevice].uiNumStreams = uiNumStmsDevice;

        // Get number of new streams.
        unsigned int uiNumNewStreams = 0;
        IAkStreamProfile * pStmProfile;
        for ( unsigned int uiStm = 0; uiStm < uiNumStmsDevice; ++uiStm )
	    {
            pStmProfile = pDevice->GetStreamProfile( uiStm );
            AKASSERT( pStmProfile != NULL );

            // Get record if new.
            if ( pStmProfile->IsNew( ) )
            {
                ++uiNumNewStreams;
            }
	    }

        // Send new stream records if any.
        if ( uiNumNewStreams > 0 )
        {
            // Compute size needed.
            long sizeofItem = SIZEOF_MONITORDATA_TO( streamRecordData.streamRecords)
                + uiNumNewStreams * sizeof( AkMonitorData::StreamRecord );

            AkProfileDataCreator creator( sizeofItem );
	        if ( !creator.m_pData )
                return;

            // Can send. Get actual data.
            creator.m_pData->eDataType = AkMonitorData::MonitorDataStreamsRecord;

	        creator.m_pData->streamRecordData.ulNumNewRecords = uiNumNewStreams;

            AkUInt32 ulNewStream = 0;

	        for ( unsigned int uiStm = 0; uiStm < uiNumStmsDevice; ++uiStm )
	        {
                pStmProfile = pDevice->GetStreamProfile( uiStm );
                AKASSERT( pStmProfile != NULL );

                // Get record if new.
                if ( pStmProfile->IsNew( ) )
                {
                    pStmProfile->GetStreamRecord( creator.m_pData->streamRecordData.streamRecords[ ulNewStream ] );
                    pStmProfile->ClearNew( );
                    ulNewStream++;
                }
	        }

            // Send new streams now.
        }
    }

	// Send all devices data.
	{
		AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( streamingDeviceData.deviceData )
							+ uiNumDevices * sizeof( AkMonitorData::DeviceData );

        AkProfileDataCreator creator( sizeofItem );
	    if ( !creator.m_pData )
		    return;

        creator.m_pData->eDataType = AkMonitorData::MonitorDataStreamingDevice;

	    creator.m_pData->streamingDeviceData.ulNumDevices = uiNumDevices;
        creator.m_pData->streamingDeviceData.fInterval = (AkReal32) ( ( in_iNow - m_iLastUpdateStreaming ) / AK::g_fFreqRatio );

        for ( unsigned int uiDevice=0; uiDevice<uiNumDevices; uiDevice++ )
        {
			pDevices[uiDevice].GetDevice()->GetData( creator.m_pData->streamingDeviceData.deviceData[ uiDevice ] );
        }
    }

    // Send all streams' data, even if none.

    {
        AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( streamingData.streamData )
                        + uiNumStreams * sizeof( AkMonitorData::StreamData );

        AkProfileDataCreator creator( sizeofItem );
	    if ( !creator.m_pData )
		    return;

        creator.m_pData->eDataType = AkMonitorData::MonitorDataStreaming;
	    creator.m_pData->streamingData.ulNumStreams = uiNumStreams;
        creator.m_pData->streamingData.fInterval = (AkReal32) ( ( in_iNow - m_iLastUpdateStreaming ) / AK::g_fFreqRatio );

        IAkStreamProfile * pStmProfile;
		AkUInt32 uNumStreamsAllDevices = 0;
        for ( unsigned int uiDevice=0; uiDevice<uiNumDevices; uiDevice++ )
        {
	        for ( unsigned int ulStm = 0; ulStm < pDevices[uiDevice].uiNumStreams; ++ulStm )
	        {
                pStmProfile = pDevices[uiDevice].GetDevice()->GetStreamProfile( ulStm );
                AKASSERT( pStmProfile != NULL );
                pStmProfile->GetStreamData( creator.m_pData->streamingData.streamData[ uNumStreamsAllDevices ] );
				++uNumStreamsAllDevices;
	        }
        }
    }

    m_iLastUpdateStreaming = in_iNow;
}

void AkPerf::PostPipelineStats( AkInt64 /*in_iNow*/ )
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataPipeline ) ) )
		return;

	bool bIsFeedbackMonitored = ( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataFeedback ) ) != 0;
	AkUInt16 uFeedbackPipelines = 0;
	AkUInt16 uAudioPipelines = 0;
	AkUInt16 uAudioDevMaps = 0;
	CAkLEngine::GetNumPipelines(uAudioPipelines, uFeedbackPipelines, uAudioDevMaps);
	AkUInt16 uTotalPipeline(uAudioPipelines);
	if (bIsFeedbackMonitored)
		uTotalPipeline += uFeedbackPipelines;

    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( pipelineData.placeholder )
                    + uTotalPipeline * sizeof( AkMonitorData::PipelineData )
					+ uAudioDevMaps * sizeof( AkMonitorData::PipelineDevMap );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataPipeline;

	creator.m_pData->pipelineData.numPipelineData = uTotalPipeline;
	creator.m_pData->pipelineData.numPipelineDevMap = uAudioDevMaps;

	AkMonitorData::PipelineData* pPipelineData = (AkMonitorData::PipelineData*)( &creator.m_pData->pipelineData.placeholder );
	AkMonitorData::PipelineDevMap* pPipelineDevMap = (AkMonitorData::PipelineDevMap*)( pPipelineData + uTotalPipeline );

	CAkLEngine::GetPipelineData( uTotalPipeline, pPipelineData, uAudioDevMaps, pPipelineDevMap, bIsFeedbackMonitored );
}

void AkPerf::PostEnvironmentStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataEnvironment ) ) )
		return;

	g_pRegistryMgr->PostEnvironmentStats();
}

void AkPerf::PostSendsStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataSends ) ) )
		return;

	CAkLEngine::PostSendsData();
}

void AkPerf::PostObsOccStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataObsOcc ) ) )
		return;

	g_pRegistryMgr->PostObsOccStats();
}

void AkPerf::PostListenerStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataListeners ) ) )
		return;

	g_pRegistryMgr->PostListenerStats();
}

void AkPerf::PostControllerStats()
{
#if defined AK_WII_FAMILY
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataControllers ) ) )
		return;

	g_pRegistryMgr->PostControllerStats();
#endif
}

void AkPerf::PostOutputStats()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataOutput ) ) || !g_pAkSink)
		return;

	if ( g_pAkSink->m_stats.m_uOutNum == 0 )
		return;

    AkInt32 sizeofItem = SIZEOF_MONITORDATA( outputData );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataOutput;

	creator.m_pData->outputData.fOffset = g_pAkSink->m_stats.m_fOutSum / g_pAkSink->m_stats.m_uOutNum;
	creator.m_pData->outputData.fPeak = AkMath::Max( fabs( g_pAkSink->m_stats.m_fOutMin ), fabs( g_pAkSink->m_stats.m_fOutMax ) );
	creator.m_pData->outputData.fRMS = sqrt( g_pAkSink->m_stats.m_fOutSumOfSquares / g_pAkSink->m_stats.m_uOutNum );

	g_pAkSink->m_stats.m_fOutMin = (AkReal32) INT_MAX;
	g_pAkSink->m_stats.m_fOutMax = (AkReal32) INT_MIN;
	g_pAkSink->m_stats.m_fOutSum = 0;
	g_pAkSink->m_stats.m_fOutSumOfSquares = 0;
	g_pAkSink->m_stats.m_uOutNum = 0;
}

void AkPerf::PostGameObjPositions()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataGameObjPosition ) ) )
		return;

	AkMonitor::PostWatchedGameObjPositions();
}

void AkPerf::PostWatchGameSyncValues()
{
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataRTPCValues ) ) )
		return;

	AkMonitor::PostWatchesRTPCValues();
}

void AkPerf::PostInteractiveMusicInfo()
{
	extern AkExternalProfileHandlerCallback g_pExternalProfileHandlerCallback;
	if( g_pExternalProfileHandlerCallback )
		g_pExternalProfileHandlerCallback();
}
#ifdef AK_MOTION
void AkPerf::PostFeedbackStats(AkReal32 in_fInterval)
{
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataFeedback ) ) || pMgr == NULL )
		return;

	AkUInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, feedbackData ) + sizeof(AkMonitorData::FeedbackMonitorData);

	AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

	creator.m_pData->eDataType = AkMonitorData::MonitorDataFeedback;
	creator.m_pData->feedbackData.timer.fInterval = in_fInterval;
	creator.m_pData->feedbackData.timer.fAudioThread = AkAudiolibTimer::timerFeedback.Millisecs();
	creator.m_pData->feedbackData.fPeak = AkMath::FastLinTodB(pMgr->GetPeak());
}

void AkPerf::PostFeedbackDevicesStats()
{
	CAkFeedbackDeviceMgr* pMgr = CAkFeedbackDeviceMgr::Get();
	if ( !( AkMonitor::GetNotifFilter() & AKMONITORDATATYPE_TOMASK( AkMonitorData::MonitorDataFeedback ) ) || pMgr == NULL )
		return;

	pMgr->PostDeviceMonitorData();
	
	g_pRegistryMgr->PostfeedbackGameObjStats();
}
#endif // AK_MOTION
#endif
