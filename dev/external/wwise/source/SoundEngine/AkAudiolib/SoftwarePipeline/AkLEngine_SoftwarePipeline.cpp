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

//////////////////////////////////////////////////////////////////////
//
// AkLEngine.cpp
//
// Implementation of the IAkLEngine interface. Contains code that is
// common to multiple platforms built on the software pipeline.
//
//   - Code common with non-software-pipeline platforms can
//     be found in Common/AkLEngine_Common.cpp
//   - Platform-specific code can be found in <Platform>/AkLEngine.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>

#include "AkLEngine_SoftwarePipeline.h"
#ifdef AK_VITA_HW
#include "AkLEngineHw.h"
#endif

#include "AkAudioLibTimer.h"
#include "AkSink.h"
#include "AkFxBase.h"
#include "AkAudioMgr.h"
#include "AkPlayingMgr.h"
#include "AkPositionRepository.h"
#include "AkSound.h"
#include "AkVPLFilterNodeOutOfPlace.h"
#include "AkBus.h"
#include "AkOutputMgr.h"
#include "Ak3DListener.h"
#include "AkSpeakerPan.h"
#include "AkEffectsMgr.h"

#ifdef AK_MOTION
	#include "AkFeedbackMgr.h"
	#include "AkFeedbackBus.h"
#endif // AK_MOTION

extern AkPlatformInitSettings g_PDSettings;
extern AkInitSettings		g_settings;

AKRESULT CAkLEngine::SoftwareInit()
{
#if defined(AK_CPU_X86) && !defined(AK_IOS)
	CAkResampler::InitDSPFunctTable();
#endif

	// Check memory manager.
	if ( !AK::MemoryMgr::IsInitialized() )
	{
		AKASSERT( !"Memory manager does not exist" );
		return AK_Fail;
	}
	// Check Stream manager.
	if ( AK::IAkStreamMgr::Get() == NULL )
	{
		AKASSERT( !"Stream manager does not exist" );
		return AK_Fail;
	}

	// Create LEngine memory pool(s).
	AKRESULT eResult = CAkLEngine::CreateLEnginePools();
	if ( eResult != AK_Success ) return eResult;

	// Listener.
	CAkListener::Init();

	//Add the main output to the output manager.
	eResult = CAkOutputMgr::AddMainDevice(g_settings.settingsMainOutput, g_settings.eMainOutputType, 0xFF/*Listener 0*/, NULL);
	if (eResult != AK_Success)
	{
		eResult = CAkOutputMgr::AddMainDevice(g_settings.settingsMainOutput, AkSink_Dummy, 0xFF/*Listener 0*/, NULL);
		if (eResult != AK_Success)
			return AK_Fail;
	}

	CAkSink * pSink = CAkOutputMgr::OutputBegin().pItem->pSink;

	g_pAkSink = pSink;
	g_settings.settingsMainOutput.uChannelMask = pSink->GetSpeakerConfig(); // preserve channel config when resetting device.

	// Effects Manager.
	eResult = CAkEffectsMgr::Init();
	if( eResult != AK_Success ) return eResult;

	// Command queue
	eResult = CAkLEngineCmds::Init();
	if( eResult != AK_Success ) return eResult;

	// Create the speaker panner.
	CAkSpeakerPan::Init();
	
	return eResult;
}

void CAkLEngine::SoftwareTerm()
{
	CAkLEngineCmds::Term();
	m_arrayVPLs.Term();
	m_Sources.Term();

	// Buffer Cache
	for ( AkUInt32 iSlot = 0; iSlot < NUM_CACHED_BUFFER_SIZES; ++iSlot )
	{
		for ( AkUInt32 iBuffer = 0, cBuffers = m_CachedAudioBuffers[ iSlot ].Length(); iBuffer < cBuffers; ++iBuffer )
			AkFalign( g_LEngineDefaultPoolId, m_CachedAudioBuffers[ iSlot ][ iBuffer ] );

		m_CachedAudioBuffers[ iSlot ].Term();
	}

	CAkEffectsMgr::Term();

	CAkOutputMgr::Term();	

	g_pAkSink = NULL;

#ifdef AK_MOTION
	if (m_pDeviceMgr != NULL)
	{
		m_pDeviceMgr->Destroy();
		m_pDeviceMgr = NULL; 
	}
#endif

	// Destroy pools.
	DestroyLEnginePools();
}

void CAkLEngine::TransferBuffer( AkVPL* in_pVPL )
{
	AkAudioBufferBus* pBuffer;

	// Get the resulting buffer for this bus
	in_pVPL->m_MixBus.GetResultingBuffer( pBuffer );

	// Add this buffer to the parent mix node
	in_pVPL->m_MixBus.UpdateFinalVolumes();
	if ( in_pVPL->GetParent() )
		in_pVPL->GetParent()->m_MixBus.ConsumeBuffer( *pBuffer, in_pVPL->m_MixBus.IsPanning(), in_pVPL->m_MixBus.m_FinalVolumes );
	else
	{
		AkDevice* pDevice = CAkOutputMgr::GetDevice(in_pVPL->m_uDevice);
		if (pDevice)
			pDevice->pFinalMix->ConsumeBuffer( pBuffer, in_pVPL->m_MixBus.IsPanning(), in_pVPL->m_MixBus.m_FinalVolumes );
	}
}

#if ! defined( AK_PS3 )
	#include <AK/SoundEngine/Common/AkSimd.h>
#endif // ! defined( AK_PS3 )

//-----------------------------------------------------------------------------
// Defines.
//-----------------------------------------------------------------------------

#define MINIMUM_STARVATION_NOTIFICATION_DELAY 8 //in buffers

//-----------------------------------------------------------------------------
//Static variables.
//-----------------------------------------------------------------------------
CAkLEngine::AkArrayVPL		CAkLEngine::m_arrayVPLs;
CAkLEngine::BufferCache		CAkLEngine::m_CachedAudioBuffers[NUM_CACHED_BUFFER_SIZES];
AkListVPLSrcs				CAkLEngine::m_Sources;

void CAkLEngine::Stop()
{
#ifdef AK_VITA_HW
	CAkLEngineHw::Stop();
#endif

	//Destroy all remaining playing sources
	for (AkListVPLSrcs::IteratorEx it = m_Sources.BeginEx(); it != m_Sources.End(); )
	{
		AKPBI_CBXCLASS * pCbx = *it;
		it = m_Sources.Erase( it );
		CAkLEngine::VPLDestroySource( pCbx, false );
	}

	// Clean up all VPL mix busses.
	DestroyAllVPLMixBusses();

	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		(*it).pFinalMix->DropFx();

	CAkLEngineCmds::DestroyDisconnectedSources();
}

#ifndef AK_OPTIMIZED
void CAkLEngine::GetNumPipelines(AkUInt16 &out_rAudio, AkUInt16& out_rFeedback, AkUInt16& out_rDevMap)
{
#ifdef AK_VITA_HW
	GetNumPipelinesHw(out_rAudio, out_rFeedback, out_rDevMap);
	return;
#else

	out_rAudio = 0;
	out_rFeedback = 0;
	out_rDevMap = 0;
	
	//Add all busses
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
#ifdef AK_MOTION
		if ((*iterVPL)->m_bFeedback)
			++out_rFeedback; // bus
		else
#endif // AK_MOTION
			if ( (*iterVPL)->m_MixBus.GetBusID() != AK_INVALID_UNIQUE_ID ) // skip dry bus
				++out_rAudio; // bus
	}

	//Add all sources
	for( AkListVPLSrcs::Iterator itSrc = m_Sources.Begin(); itSrc != m_Sources.End(); ++itSrc )
	{
#ifdef AK_MOTION
		if ((*itSrc)->GetContext()->IsForFeedbackPipeline())
			++out_rFeedback;
		else
#endif
			++out_rAudio;

		bool bMainDevice = false;
		bool bNonMainDevice = false;
		CAkVPLSrcCbxNode * pCbx = *itSrc;
		for( AkDeviceInfoList::Iterator itDev = pCbx->m_OutputDevices.Begin(); itDev != pCbx->m_OutputDevices.End(); ++itDev )
		{
			if( (*itDev)->uDeviceID != AK_MAIN_OUTPUT_DEVICE || (*itDev)->bCrossDeviceSend )
			{
				bNonMainDevice = true;
				out_rDevMap++;
			}
			else
				bMainDevice = true;
		}
		if( bMainDevice && bNonMainDevice )
			out_rDevMap++;
	}

	// Master Bus
	++out_rAudio;
#endif //AK_VITA_HW
}

AkUInt32 GetEffectiveChannelMask( CAkBusCtx in_ctxBus, AkOutputDeviceID in_deviceID )
{
	AkUInt32 channelMask = AK_CHANNEL_MASK_PARENT;
	while ( in_ctxBus.HasBus() )
	{
		channelMask = in_ctxBus.GetChannelConfig();
		if ( channelMask != AK_CHANNEL_MASK_PARENT )
			return channelMask;
		in_ctxBus = in_ctxBus.GetParentCtx();
	}

	AkDevice* pDevice = CAkOutputMgr::GetDevice( in_deviceID );
	if ( pDevice  )
		channelMask = pDevice->pSink->GetSpeakerConfig();

	return channelMask;
}

void GetSrcOutputBusVolume(
		CAkVPLSrcCbxNode* in_pSrcCbxNode,
		AkOutputDeviceID in_uDeviceID,
		AkReal32* out_pfOutputBusVolume,
		AkUInt16 in_uNumChannelIn,
		AkUInt16 in_uNumChannelOut )
{
	// Take the maximum volume per output channel.

	AkReal32* apVolume[AK_MAX_NUM_CHANNELS];

	AkUInt16 inChan = 0;
	for ( ; inChan < in_uNumChannelIn; ++inChan )
		apVolume[ inChan ] = (AkReal32*)&in_pSrcCbxNode->LastVolumes( in_uDeviceID, inChan );
	for ( ; inChan < AK_MAX_NUM_CHANNELS; ++inChan )
		apVolume[ inChan ] = NULL;

	AkUInt16 outChan = 0;
	for ( ; outChan < in_uNumChannelOut; ++outChan )
	{
		out_pfOutputBusVolume[ outChan ] = 0;
		for ( AkUInt16 inChan = 0; inChan < in_uNumChannelIn; ++inChan )
		{
			out_pfOutputBusVolume[ outChan ] = AkMath::Max( out_pfOutputBusVolume[ outChan ], apVolume[ inChan ][ outChan ] );
		}
		out_pfOutputBusVolume[ outChan ] = AkMath::FastLinTodB( out_pfOutputBusVolume[ outChan ] );
	}
	for ( ; outChan < AK_MAX_NUM_CHANNELS; ++outChan )
		out_pfOutputBusVolume[ outChan ] = AK_SAFE_MINIMUM_VOLUME_LEVEL;
}

void CAkLEngine::GetPipelineData(AkUInt16 in_uMaxPipelineData, AkMonitorData::PipelineData * out_pPipelineData, AkUInt16 in_uMaxPipelineDevMap, AkMonitorData::PipelineDevMap * out_pPipelineDevMap, bool in_bGetFeedback )
{
#ifdef AK_VITA_HW
	GetPipelineDataHw(in_uMaxPipelineData, out_pPipelineData, in_uMaxPipelineDevMap, out_pPipelineDevMap, in_bGetFeedback );
	return;
#else

	AkUInt16 uNumPipelineData = 0;
	AkUInt16 uNumPipelineDevMap = 0;

	AkMonitorData::PipelineData *pMasterBusPipelineData = out_pPipelineData;
	// Master bus first
	{
		AKASSERT( uNumPipelineData < in_uMaxPipelineData );
		const CAkBusCtx &l_BusCtx = g_MasterBusCtx;
		AkUniqueID l_BusID = l_BusCtx.ID();

		//TODO: provide profiling information about all device's endpoints.
		CAkVPLFinalMixNode *pFinalMixNode = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pFinalMix;

		// Bus pipeline data
		out_pPipelineData->pipelineID = 0; // 0 means bus.
		out_pPipelineData->bus.bFeedback = false;
		out_pPipelineData->bus.mixBusID = l_BusID;
		out_pPipelineData->bus.mixBusParentID = l_BusCtx.GetParentCtx().ID();
		out_pPipelineData->bus.deviceID = AK_MAIN_OUTPUT_DEVICE;
		out_pPipelineData->bus.channelMask = GetEffectiveChannelMask( l_BusCtx, AK_MAIN_OUTPUT_DEVICE );

		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
			out_pPipelineData->bus.fxID[ uFXIndex ] = pFinalMixNode->GetFXID( uFXIndex ); 

		out_pPipelineData->bus.fBusVolume = pFinalMixNode->GetPreviousVolumedB();
		out_pPipelineData->bus.uVoiceCount = (AkUInt16) pFinalMixNode->GetMixingVoiceCount();
		out_pPipelineData->bus.fDownstreamGain = AkMath::FastLinTodB( pFinalMixNode->GetPreviousVolume() );

		uNumPipelineData++;
		out_pPipelineData++;
	}

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
#ifdef AK_MOTION
		if (!in_bGetFeedback && (*iterVPL)->m_bFeedback)
			continue;	//Not needed for feedback (for now)
#endif // AK_MOTION

		const CAkBusCtx &l_BusCtx = (*iterVPL)->m_MixBus.GetBusContext();
		AkUniqueID l_BusID = l_BusCtx.ID();

		if ( l_BusID != AK_INVALID_UNIQUE_ID )
		{
			AKASSERT( uNumPipelineData < in_uMaxPipelineData );
			// Bus pipeline data
			out_pPipelineData->pipelineID = 0; // 0 means bus.
	#ifdef AK_MOTION
			out_pPipelineData->bus.bFeedback = (*iterVPL)->m_bFeedback;
	#else // AK_MOTION
			out_pPipelineData->bus.bFeedback = false;
	#endif // AK_MOTION

			out_pPipelineData->bus.fBusVolume = (*iterVPL)->m_MixBus.GetPreviousVolumedB();
			out_pPipelineData->bus.uVoiceCount = (AkUInt16) (*iterVPL)->m_MixBus.GetMixingVoiceCount();
			out_pPipelineData->bus.fDownstreamGain = AkMath::FastLinTodB( (*iterVPL)->m_fDownstreamGain );
	
			out_pPipelineData->bus.mixBusID = l_BusID;
			out_pPipelineData->bus.mixBusParentID = l_BusCtx.GetParentCtx().ID();
			out_pPipelineData->bus.deviceID = (*iterVPL)->m_uDevice;
			out_pPipelineData->bus.channelMask = GetEffectiveChannelMask( l_BusCtx, out_pPipelineData->bus.deviceID );

			for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
				out_pPipelineData->bus.fxID[ uFXIndex ] = (*iterVPL)->m_MixBus.GetFXID( uFXIndex );

			uNumPipelineData++;
			out_pPipelineData++;
		}
		else
		{
			// WG-21538
			// The VPL with l_BusID == AK_INVALID_UNIQUE_ID is the VPL representing the master bus submix.
			// Its voice count must be added to the input connexions of the final mixer as this is not represented in the Profiler.
			// but since this mixer itself was one of the input of the final, me must add -1 so it is consistent to the
			// user comprehension.
			// The ultimate fix could be to represent this node in the profiler, but it is not helping the user.
			pMasterBusPipelineData->bus.uVoiceCount += (AkUInt16) ( (*iterVPL)->m_MixBus.GetMixingVoiceCount() - 1 );
			pMasterBusPipelineData->bus.fDownstreamGain = AkMath::FastLinTodB( (*iterVPL)->m_fDownstreamGain );
		}

#ifdef AK_MOTION
        AkVolumeValue l_BusVolume = l_BusCtx.GetVolume( BusVolumeType_IncludeEntireBusTree );
		if ((*iterVPL)->m_bFeedback)
		{
			CAkFeedbackBus *pMaster = CAkFeedbackBus::GetMasterMotionBusAndAddRef();
			if (pMaster != NULL)
			{
				l_BusVolume += pMaster->GetBusEffectiveVolume( BusVolumeType_IncludeEntireBusTree, AkPropID_Volume );
				pMaster->Release();
			}
		}
#endif // AK_MOTION
	}

	// Voice pipeline data
	for( AkListVPLSrcs::Iterator iterVPLSrc = m_Sources.Begin(); iterVPLSrc != m_Sources.End(); ++iterVPLSrc)
	{
		CAkVPLSrcCbxNode * pCbx = *iterVPLSrc;
		out_pPipelineData->pipelineID = pCbx->m_PipelineID;

		CAkPBI * pCtx = pCbx->GetContext();
		if ( pCtx )
		{
#ifdef AK_MOTION
			if (!in_bGetFeedback && pCtx->IsForFeedbackPipeline())
				continue;	//Not needed for feedback (for now)
#endif

			AKASSERT( uNumPipelineData < in_uMaxPipelineData );
			out_pPipelineData->voice.playingID	= pCtx->GetPlayingID();
			out_pPipelineData->voice.gameObjID	= GameObjectToWwiseGameObject( pCtx->GetGameObjectPtr()->ID() );
			out_pPipelineData->voice.soundID	= pCtx->GetSoundID();
			AkDeviceInfo* pMainOutput = pCbx->GetDeviceInfo(AK_MAIN_OUTPUT_DEVICE);
			if (pMainOutput)
				out_pPipelineData->voice.mixBusID = pMainOutput->pMixBus->m_MixBus.GetBusContext().ID();	//This can happen in the sound isn't caught by any listener and the virtual behavior is "continue to play"
			else
				out_pPipelineData->voice.mixBusID = (AkUniqueID)~0;	//This source goes in a secondary output and will be reported in the DeviceMap below.

#ifdef AK_MOTION
			AkFeedbackParams* pFeedbackParams = pCtx->GetFeedbackParameters();
			out_pPipelineData->voice.feedbackMixBusID = pFeedbackParams != NULL ? pFeedbackParams->m_pOutput->ID() : 0;
#else // AK_MOTION
			out_pPipelineData->voice.feedbackMixBusID = 0;
#endif // AK_MOTION

			AkVPLSrcCbxRec & cbxRec =  pCbx->m_cbxRec;
			for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
			{
				if ( cbxRec.m_pFilter[ uFXIndex ] )
				{
					out_pPipelineData->voice.fxID[ uFXIndex ] = cbxRec.m_pFilter[ uFXIndex ]->GetFXID();
				}
				else
				{
					out_pPipelineData->voice.fxID[ uFXIndex ] = AK_INVALID_PLUGINID;
				}
			}

			AkSoundParamsEx params;
			pCtx->GetEffectiveParamsForMonitoring( params );

			out_pPipelineData->voice.priority = pCtx->GetPriority();
			out_pPipelineData->voice.fBaseVolume = AkMath::FastLinTodB( pCbx->BaseVolume() );
			out_pPipelineData->voice.fMaxVolume = pMainOutput ? AkMath::FastLinTodB( pMainOutput->fMaxVolume ) : AK_SAFE_MINIMUM_VOLUME_LEVEL;
			out_pPipelineData->voice.fHdrWindowTop = pCbx->GetHdrWindowTop();
			out_pPipelineData->voice.fEnvelope = pCbx->GetEnvelope();
			out_pPipelineData->voice.fNormalizationGain = pCbx->GetNormalizationMonitoring();
			out_pPipelineData->voice.fBaseLPF = params.LPF;
			out_pPipelineData->voice.fVoiceLPF = pCbx->LastLPF();

			// Game-Defined
			out_pPipelineData->voice.fGameAuxSendVolume = params.fGameAuxSendVolume;

			// Take the maximum volume per output channel.
			GetSrcOutputBusVolume( pCbx, 0,
				out_pPipelineData->voice.fOutputBusVolume,
				(AkUInt16) AK::GetNumChannels( pCtx->GetMediaFormat().GetChannelMask() ),
				(AkUInt16)( sizeof(AkSpeakerVolumes)/sizeof(AkReal32) ) );

			out_pPipelineData->voice.fOutputBusLPF = pCbx->LastObsLPF();
			
			out_pPipelineData->voice.bIsStarted = pCbx->GetState() != NodeStateInit && pCtx->GetFrameOffset() < 0;

			out_pPipelineData->voice.bIsVirtual = pCbx->IsVirtualVoice();
			out_pPipelineData->voice.bIsForcedVirtual = pCtx->IsForcedVirtualized();
		}
		else
		{
			out_pPipelineData->voice.gameObjID = WWISE_INVALID_GAME_OBJECT;
		}

		// Pipeline to OutputDevice map used by monitoring UI
		bool bMainDevice = false;
		bool bNonMainDevice = false;
		AkUInt16 uNumChannelIn = (AkUInt16)AK::GetNumChannels( pCtx ? pCtx->GetMediaFormat().GetChannelMask() : 0 );
		AkUInt16 uNumChannelOut = (AkUInt16)( sizeof(AkSpeakerVolumes)/sizeof(AkReal32) );

		for( AkDeviceInfoList::Iterator iterDevice = pCbx->m_OutputDevices.Begin(); iterDevice != pCbx->m_OutputDevices.End(); ++iterDevice )
		{
			if( (*iterDevice)->uDeviceID != AK_MAIN_OUTPUT_DEVICE || (*iterDevice)->bCrossDeviceSend )
			{
				// Send a device map for a cross-device send so that the profiler can make the connection in the voices graph.
				AKASSERT( uNumPipelineDevMap < in_uMaxPipelineDevMap );
				out_pPipelineDevMap->pipelineID = pCbx->m_PipelineID;
				out_pPipelineDevMap->mixBusID = (*iterDevice)->pMixBus->m_MixBus.GetBusContext().ID();
				out_pPipelineDevMap->deviceID = (*iterDevice)->uDeviceID;
				out_pPipelineDevMap->fMaxVolume = AkMath::FastLinTodB( (*iterDevice)->fMaxVolume );

				GetSrcOutputBusVolume( pCbx,
					(*iterDevice)->uDeviceID,
					out_pPipelineDevMap->fOutputBusVolume,
					uNumChannelIn,
					uNumChannelOut );

				out_pPipelineDevMap++;
				uNumPipelineDevMap++;
				bNonMainDevice = true;
			}
			else
				bMainDevice = true;
		}

		// Include main device map ONLY if there's also a non-main device map
		if ( bMainDevice && bNonMainDevice )
		{
			AKASSERT( uNumPipelineDevMap < in_uMaxPipelineDevMap );
			out_pPipelineDevMap->pipelineID = pCbx->m_PipelineID;
			AkUniqueID mixBusID = ( out_pPipelineData->voice.mixBusID != 0 )
				? out_pPipelineData->voice.mixBusID : g_MasterBusCtx.ID();
			out_pPipelineDevMap->mixBusID = mixBusID;
			out_pPipelineDevMap->deviceID = AK_MAIN_OUTPUT_DEVICE;
			out_pPipelineDevMap->fMaxVolume = out_pPipelineData->voice.fMaxVolume;

			out_pPipelineDevMap++;
			uNumPipelineDevMap++;
		}

		uNumPipelineData++;
		out_pPipelineData++;
	}

	AKASSERT( uNumPipelineData == in_uMaxPipelineData );
	AKASSERT( uNumPipelineDevMap == in_uMaxPipelineDevMap );
#endif //AK_VITA_HW
}

AkUInt32 CAkLEngine::GetNumBusses()
{
	AkUInt32 uNumBusses = 0;
	// Normal busses
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		if( (*iterVPL)->m_MixBus.GetBusID() != AK_INVALID_UNIQUE_ID )// Skipping master bus duplicate, will be done lower in +1.
		{
			++uNumBusses;
		}
	}

	// We must check we really have a master bus.
	// There exist moments where there is no master bus but profiling can be running, must handle it.
	if( g_MasterBusCtx.ID() != AK_INVALID_UNIQUE_ID )
		++uNumBusses;//we have a master bus

	return uNumBusses;
}

void CountWatchedBusses( AkUniqueID in_busID, AkVPL* in_pVPL, AkUInt32& io_uiCount )
{
	AKASSERT( in_pVPL );
	if( in_busID != AK_INVALID_UNIQUE_ID )	// in order to exclude final mix bus node (?)
	{
		AkUInt8 types = AkMonitor::GetMeterWatchDataTypes( in_busID );

		if ( types & AkMonitorData::BusMeterDataType_HdrPeak )
		{
			if ( in_pVPL
				&& in_pVPL->IsHDR() )
				++io_uiCount;
			types = types & ~AkMonitorData::BusMeterDataType_HdrPeak;
		}

		// Other meters require correctly allocated meter context.
		if ( in_pVPL->m_MixBus.GetMeterCtx() )
			io_uiCount += AK::GetNumChannels( types );
	}
}

/// FIXME There is one too many.
void CountWatchedBussesFinalMixNode( AkUniqueID in_busID, CAkVPLFinalMixNode * in_pFinalMixNode, AkUInt32& io_uiCount )
{
	AKASSERT( in_pFinalMixNode );
	if( in_busID != AK_INVALID_UNIQUE_ID )
	{
		AkUInt8 types = AkMonitor::GetMeterWatchDataTypes( in_busID );

		if ( types & AkMonitorData::BusMeterDataType_HdrPeak )
		{
			/** Final mix node is never HDR because of (artificial) special case.
			if ( in_pVPL
				&& in_pVPL->IsHDR() )
				++io_uiCount;
				**/
			types = types & ~AkMonitorData::BusMeterDataType_HdrPeak;
		}

		// Other meters require correctly allocated meter context.
		if ( in_pFinalMixNode->GetMeterCtx() )
			io_uiCount += AK::GetNumChannels( types );
	}
}

void PrepareMeterData( 
	AkUInt8 in_uDataType,
	AkUniqueID in_BusID, 
	AkChannelMask in_channelMask,
	AkSIMDSpeakerVolumes& in_meterVolumes,
	AkMonitorData::BusMeterData& io_MeterData )
{
	AkUInt32 uBusChannelCount = GetNumChannels( in_channelMask );
	
	io_MeterData.idBus = in_BusID;
	io_MeterData.uDataType = in_uDataType;
	io_MeterData.uChannelMask = (AkUInt16)in_channelMask;

	in_meterVolumes.FastLinTodB();

	AkUInt32 i = 0;
	for(; i < uBusChannelCount; ++i )
	{
		io_MeterData.aChannels[i] = in_meterVolumes.aVolumes[i];
	}
	for(; i < AK_MAX_NUM_CHANNELS; ++i )
	{
		io_MeterData.aChannels[i] = AK_SAFE_MINIMUM_VOLUME_LEVEL;
	}
}

void BuildMeterMonitorData( AkUniqueID in_busID, AkVPL* in_pVPL, CAkBusFX* in_pMixBus, AkProfileDataCreator& in_creator, AkUInt32& io_uNumMeterWatchIndex )
{
	if( in_busID != AK_INVALID_UNIQUE_ID )// Skipping master bus duplicate, will be done lower in +1.
	{
		AkUInt8 types = AkMonitor::GetMeterWatchDataTypes( in_busID );

		if( types & AkMonitorData::BusMeterDataType_HdrPeak && in_pVPL && in_pVPL->IsHDR() )
		{
			// Post the Hdr peak
			AkMonitorData::BusMeterData& busMeterData = in_creator.m_pData->meterData.busMeters[io_uNumMeterWatchIndex];
			busMeterData.idBus = in_busID;
			busMeterData.uDataType = AkMonitorData::BusMeterDataType_HdrPeak;
			busMeterData.uChannelMask = AK_SPEAKER_SETUP_MONO;
			busMeterData.aChannels[0] = ((AkHdrBus*)in_pVPL)->GetHdrWindowTop() - AkMath::FastLinTodB( in_pVPL->m_fDownstreamGain );

			++io_uNumMeterWatchIndex;
		}

		// All meters except HDR depend on the AkMeterCtx.

#ifndef AK_OPTIMIZED

		const AkMeterCtx * pMetering = in_pMixBus->GetMeterCtx();
		if ( !pMetering )
			return;

		if ( types & AkMonitorData::BusMeterDataType_Peak )
		{
			// Post the Peak
			AkMonitorData::BusMeterData& busMeterData = in_creator.m_pData->meterData.busMeters[io_uNumMeterWatchIndex];
			AkSIMDSpeakerVolumes vMeterValues = pMetering->GetMeterPeak();

			PrepareMeterData( 
				AkMonitorData::BusMeterDataType_Peak,
				in_busID, 
				in_pMixBus->GetChannelMask(),
				vMeterValues,
				busMeterData );

			++io_uNumMeterWatchIndex;
		}

		if ( types & AkMonitorData::BusMeterDataType_RMS )
		{
			// Post the RMS
			AkMonitorData::BusMeterData& busMeterData = in_creator.m_pData->meterData.busMeters[io_uNumMeterWatchIndex];
			AkSIMDSpeakerVolumes vMeterValues = pMetering->GetMeterRMS();

			PrepareMeterData( 
				AkMonitorData::BusMeterDataType_RMS,
				in_busID, 
				in_pMixBus->GetChannelMask(),
				vMeterValues,
				busMeterData );

			++io_uNumMeterWatchIndex;
		}

		if( types & AkMonitorData::BusMeterDataType_TruePeak )
		{
			// Post the True peak
			AkMonitorData::BusMeterData& busMeterData = in_creator.m_pData->meterData.busMeters[io_uNumMeterWatchIndex];
			AkSIMDSpeakerVolumes vMeterValues = pMetering->GetMeterTruePeak();

			PrepareMeterData( 
				AkMonitorData::BusMeterDataType_TruePeak,
				in_busID, 
				in_pMixBus->GetChannelMask(),
				vMeterValues,
				busMeterData );

			++io_uNumMeterWatchIndex;
		}

		if( types & AkMonitorData::BusMeterDataType_KPower )
		{
			// Post the loudness
			AkMonitorData::BusMeterData& busMeterData = in_creator.m_pData->meterData.busMeters[io_uNumMeterWatchIndex];
			busMeterData.idBus = in_busID;
			busMeterData.uDataType = AkMonitorData::BusMeterDataType_KPower;
			busMeterData.uChannelMask = AK_SPEAKER_SETUP_STEREO;
			busMeterData.aChannels[0] = pMetering->GetMeanPowerLoudness();
			busMeterData.aChannels[1] = (AkReal32)LE_MAX_FRAMES_PER_BUFFER / (AkReal32)AK_CORE_SAMPLERATE;
			
			++io_uNumMeterWatchIndex;
		}

#endif

	}
}

void CAkLEngine::PostMeterWatches()
{
	AkUInt32 uNumMeterWatch = 0;

	//TODO: provide profiling information about all device's endpoints.
	CAkVPLFinalMixNode *pFinalMixNode = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pFinalMix;

	// Count busses first
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL* pVPL = (*iterVPL);
		CountWatchedBusses( pVPL->m_MixBus.GetBusID(), pVPL, uNumMeterWatch );
	}

	// Master bus
	{
		CountWatchedBussesFinalMixNode( g_MasterBusCtx.ID(), pFinalMixNode, uNumMeterWatch );
	}

	// Then post watches
	if( uNumMeterWatch > 0 )
	{
		AkUInt32 uArraydatasize = uNumMeterWatch * sizeof( AkMonitorData::BusMeterData );
		AkProfileDataCreator creator( SIZEOF_MONITORDATA_TO( meterData.busMeters ) + uArraydatasize );
		if ( creator.m_pData )
		{
			creator.m_pData->eDataType = AkMonitorData::MonitorDataMeter;
			creator.m_pData->meterData.uNumBusses = (AkUInt16) uNumMeterWatch;

			AkUInt32 uNumMeterWatchIndex = 0;
			for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
			{
				AkVPL* pVPL = (*iterVPL);
				BuildMeterMonitorData( pVPL->m_MixBus.GetBusID(), pVPL, &(pVPL->m_MixBus), creator, uNumMeterWatchIndex );
			}

			// Post master bus
			BuildMeterMonitorData( g_MasterBusCtx.ID(), NULL, pFinalMixNode, creator, uNumMeterWatchIndex );
		}
	}
}

void CAkLEngine::PostSendsData()
{
#ifdef AK_VITA_HW
	PostSendsDataHw();
	return;
#else

	AkUInt32 ulNumSends = 0;

	// Count sends
	// Voice pipeline data
	for ( AkListVPLSrcs::Iterator iterVPLSrc = m_Sources.Begin(); iterVPLSrc != m_Sources.End(); ++iterVPLSrc )
	{
	        CAkVPLSrcCbxNode * pCbx = *iterVPLSrc;

		CAkPBI * pCtx = pCbx->GetContext();
		if ( pCtx )
		{
			AkSoundParamsEx params;
			pCtx->GetEffectiveParamsForMonitoring( params );

			// User-Defined Aux Sends
			for( AkUInt32 i = 0; i < AK_NUM_AUX_SEND_PER_OBJ_PROFILER; ++i )
			{
				if( params.aAuxSend[i] != AK_INVALID_AUX_ID )
					ulNumSends++;
			}

			if( pCtx->IsGameDefinedAuxEnabled() )
			{
				// Game-Defined Aux Sends
				CAkRegisteredObj* pGameObj = pCtx->GetGameObjectPtr();
				const AkAuxSendValue * AK_RESTRICT pGameEnvValues = pGameObj->GetGameDefinedAuxSends();
				bool bInfoIsValid = true;

				for( AkUInt32 i = 0; i < AK_MAX_GAME_DEFINED_AUX_PER_OBJ_PROFILER; ++i )
				{
					bInfoIsValid = bInfoIsValid && pGameEnvValues[i].auxBusID != AK_INVALID_AUX_ID;
					if( bInfoIsValid )
						ulNumSends++;
				}
			}
		}
	}

    AkInt32 sizeofItem = SIZEOF_MONITORDATA_TO( sendsData.sends )
						+ ulNumSends * sizeof( AkMonitorData::SendsData );

    AkProfileDataCreator creator( sizeofItem );
	if ( !creator.m_pData )
		return;

    creator.m_pData->eDataType = AkMonitorData::MonitorDataSends;

	creator.m_pData->sendsData.ulNumSends = ulNumSends;

	AkMonitorData::SendsData* pSendsData = creator.m_pData->sendsData.sends;

	// Fill data
	// Voice pipeline data
	for ( AkListVPLSrcs::Iterator iterVPLSrc = m_Sources.Begin(); iterVPLSrc != m_Sources.End(); ++iterVPLSrc )
	{
	        CAkVPLSrcCbxNode * pCbx = *iterVPLSrc;

		CAkPBI * pCtx = pCbx->GetContext();
		if ( pCtx )
		{
			AkSoundParamsEx params;
			pCtx->GetEffectiveParamsForMonitoring( params );

			// User-Defined Aux Sends
			for( AkUInt32 i = 0; i < AK_NUM_AUX_SEND_PER_OBJ_PROFILER; ++i )
			{
				if( params.aAuxSend[i] != AK_INVALID_AUX_ID )
				{
					pSendsData->pipelineID = pCbx->m_PipelineID;
					pSendsData->gameObjID	= GameObjectToWwiseGameObject( pCtx->GetGameObjectPtr()->ID() );
					pSendsData->soundID	= pCtx->GetSoundID();

					pSendsData->auxBusID = params.aAuxSend[i];
					pSendsData->fVolume = params.aUserAuxSendVolume[i];
					pSendsData->eSendType = (AkMonitorData::SendType)(AkMonitorData::SendType_UserDefinedSend0 + i);
					pSendsData++;
				}
			}

			if( pCtx->IsGameDefinedAuxEnabled() )
			{
				// Game-Defined Aux Sends
				CAkRegisteredObj* pGameObj = pCtx->GetGameObjectPtr();
				const AkAuxSendValue * AK_RESTRICT pGameEnvValues = pGameObj->GetGameDefinedAuxSends();
				bool bInfoIsValid = true;

				for( AkUInt32 i = 0; i < AK_MAX_GAME_DEFINED_AUX_PER_OBJ_PROFILER; ++i )
				{
					bInfoIsValid = bInfoIsValid && pGameEnvValues[i].auxBusID != AK_INVALID_AUX_ID;

					if( bInfoIsValid )
					{
						pSendsData->pipelineID = pCbx->m_PipelineID;
						pSendsData->gameObjID	= GameObjectToWwiseGameObject( pCtx->GetGameObjectPtr()->ID() );
						pSendsData->soundID	= pCtx->GetSoundID();

						pSendsData->auxBusID = pGameEnvValues[i].auxBusID;
						pSendsData->fVolume = AkMath::FastLinTodB( pGameEnvValues[i].fControlValue );
						pSendsData->eSendType = (AkMonitorData::SendType)(AkMonitorData::SendType_GameDefinedSend0 + i);
						pSendsData++;
					}
				}
			}
		}
	}
#endif
}

#endif

//-----------------------------------------------------------------------------
// Name: ResetBusVolume
// Desc: Set the volume of a specified bus.
//
// Parameters:
//	AkUniqueID   in_MixBusID     : ID of a specified bus.
//	AkVolumeValue in_Volume : Volume to set.
//-----------------------------------------------------------------------------
void CAkLEngine::ResetBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_Volume )
{
	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pVPL = *iterVPL;

		if( pVPL->m_MixBus.GetBusID() == in_MixBusID )
		{
			pVPL->m_MixBus.ResetNextVolume( in_Volume );
		}
	}

} // ResetBusVolume

//-----------------------------------------------------------------------------
// Name: ResetMasterBusVolume
// Desc: Set the volume of master bus.
//
// Parameters:
//	AkVolumeValue in_Volume : Volume to set.
//-----------------------------------------------------------------------------
void CAkLEngine::ResetMasterBusVolume( bool in_bMain, AkVolumeValue in_Volume )
{
	if (in_bMain)
	{
		AkDevice *pMain = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE);
		if ( pMain )
			pMain->pFinalMix->ResetNextVolume( in_Volume );
	}
	else
	{
		for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		{
			if ((*it).uDeviceID != AK_MAIN_OUTPUT_DEVICE)
				(*it).pFinalMix->ResetNextVolume( in_Volume );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: SetBusVolume
// Desc: Set the volume of a specified bus.
//
// Parameters:
//	AkUniqueID   in_MixBusID     : ID of a specified bus.
//	AkVolumeValue in_VolumeOffset : Volume to set.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::SetBusVolume( AkUniqueID in_MixBusID, AkVolumeValue in_VolumeOffset )
{
	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pVPL = *iterVPL;

		if( pVPL->m_MixBus.GetBusID() == in_MixBusID )
		{
			pVPL->m_MixBus.SetVolumeOffset( in_VolumeOffset );
		}
	}
	
} // SetBusVolume

//-----------------------------------------------------------------------------
// Name: SetMasterBusVolume
// Desc: Set the volume of master bus.
//
// Parameters:
//	AkVolumeValue in_VolumeOffset : Volume to set.
//-----------------------------------------------------------------------------
void CAkLEngine::SetMasterBusVolume( bool in_bMain, AkVolumeValue in_VolumeOffset )
{	
	if (in_bMain)
	{
		AkDevice *pMain = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE);
		if ( pMain )
			pMain->pFinalMix->SetVolumeOffset( in_VolumeOffset );
	}
	else
	{
		//For now, set the volume of all the secondary master busses.
		for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		{
			if ((*it).uDeviceID != AK_MAIN_OUTPUT_DEVICE)
				(*it).pFinalMix->SetVolumeOffset( in_VolumeOffset );
		}
	}
}

void CAkLEngine::EnableVolumeCallback( AkUniqueID in_MixBusID, bool in_bEnable )
{
	// Find the bus and set the volume.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pVPL = *iterVPL;

		if( pVPL->m_MixBus.GetBusID() == in_MixBusID )
		{
			pVPL->m_MixBus.EnableVolumeCallbackCheck( in_bEnable );
		}
	}
}

//-----------------------------------------------------------------------------
// Name: BypassBusFx
// Desc: Bypass the effect of a specified bus.
//
// Parameters:
//	AkUniqueID in_MixBusID    : ID of a specified bus.
//	bool	   in_bIsBypassed : true=bypass effect, false=do not bypass.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::BypassBusFx( AkUniqueID in_MixBusID, AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
{
	// Find the bus and bypass the fx.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pVPL = *iterVPL;

		if( pVPL->m_MixBus.GetBusID() == in_MixBusID )
		{
			pVPL->m_MixBus.SetInsertFxBypass( in_bitsFXBypass, in_uTargetMask );
		}
	}

} // BypassBusFx

//-----------------------------------------------------------------------------
// Name: BypassMasterBusFx
// Desc: Bypass the effect the master
//
// Parameters:
//	bool	   in_bIsBypassed : true=bypass effect, false=do not bypass.
//
// Return:
//	None.
//-----------------------------------------------------------------------------
void CAkLEngine::BypassMasterBusFx( AkUInt32 in_bitsFXBypass, AkUInt32 in_uTargetMask )
	{
		AkDevice *pMain = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE);
		if ( pMain )
			pMain->pFinalMix->SetInsertFxBypass( in_bitsFXBypass, in_uTargetMask );

} // BypassMasterBusFx

void CAkLEngine::PositioningChangeNotification( AkUniqueID in_MixBusID, AkReal32 in_RTPCValue, AkRTPC_ParameterID in_ParameterID )
{
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pVPL = *iterVPL;

		if( pVPL->m_MixBus.GetBusID() == in_MixBusID )
		{
			pVPL->m_MixBus.PositioningChangeNotification( in_RTPCValue, in_ParameterID );
		}
	}
}

void CAkLEngine::StopMixBussesUsingThisSlot( const CAkUsageSlot* in_pSlot )
{
	// Stop any bus currently using this slot.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pVPL = *iterVPL;
		if( pVPL->m_MixBus.IsUsingThisSlot( in_pSlot ) )
			pVPL->m_MixBus.DropFx();
	}

	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
	{
		CAkVPLFinalMixNode *pFinalMix = (*it).pFinalMix;
		if( pFinalMix && pFinalMix->IsUsingThisSlot( in_pSlot ) )
		{
			pFinalMix->DropFx();
			pFinalMix->Stop();
		}
	}
}

void CAkLEngine::ResetAllEffectsUsingThisMedia( const AkUInt8* in_pData )
{
	// Stop any bus currently using this slot.
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		if( (*iterVPL)->m_MixBus.IsUsingThisSlot( in_pData ) )
			(*iterVPL)->m_MixBus.SetAllInsertFx();
	}

	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
	{
		CAkVPLFinalMixNode *pFinalMix = (*it).pFinalMix;
		if( pFinalMix && pFinalMix->IsUsingThisSlot( in_pData ) )
			pFinalMix->SetAllInsertFx();
	}
}

void * CAkLEngine::GetCachedAudioBuffer( AkUInt32 in_uSize )
{
	if (in_uSize < CACHED_BUFFER_SIZE_DIVISOR)
		in_uSize = CACHED_BUFFER_SIZE_DIVISOR;

	AkUInt32 uCacheIndex = ( in_uSize - 1 ) / CACHED_BUFFER_SIZE_DIVISOR;
	AKASSERT( uCacheIndex < NUM_CACHED_BUFFER_SIZES );

	BufferCache & CachedBuffers = m_CachedAudioBuffers[ uCacheIndex ];

	void * pvReturned;

	if ( !CachedBuffers.IsEmpty() )
	{
		pvReturned = CachedBuffers.Last();
		CachedBuffers.RemoveLast();
	}
	else
	{
		pvReturned = AkMalign( g_LEngineDefaultPoolId, in_uSize, AK_BUFFER_ALIGNMENT);
	}

	return pvReturned;
}

void CAkLEngine::ReleaseCachedAudioBuffer( AkUInt32 in_uSize, void * in_pvBuffer )
{
	if (in_uSize < CACHED_BUFFER_SIZE_DIVISOR)
		in_uSize = CACHED_BUFFER_SIZE_DIVISOR;

	BufferCache & CachedBuffers = m_CachedAudioBuffers[ ( in_uSize - 1 ) / CACHED_BUFFER_SIZE_DIVISOR ];

	void ** ppvBuffer = CachedBuffers.AddLast();
	if ( ppvBuffer )
	{
		*ppvBuffer = in_pvBuffer;
	}
	else
	{
		AkFalign( g_LEngineDefaultPoolId, in_pvBuffer );
	}
}

void CAkLEngine::UpdateMasterBusFX( AkUInt32 in_uFXIndex )
{
	AkDevice *pMain = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE);
	if ( pMain )
		pMain->pFinalMix->SetInsertFx( g_MasterBusCtx, in_uFXIndex );
}

void CAkLEngine::UpdateMixBusFX( AkUniqueID in_MixBusID, AkUInt32 in_uFXIndex )
{
	AkVPL * l_pMixBus = NULL;

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		l_pMixBus = *iterVPL;
		if( l_pMixBus->m_MixBus.GetBusID() == in_MixBusID )
		{
			if ( l_pMixBus->m_MixBus.GetState() == NodeStateStop )
				continue;
			else
				l_pMixBus->m_MixBus.SetInsertFx( l_pMixBus->m_MixBus.GetBusContext(), in_uFXIndex );
		}
	}
}

void CAkLEngine::ForAllPluginParam( 
	class CAkFxBase * in_pFx, 
	AkForAllPluginParamFunc in_funcForAll, 
	void * in_pCookie )
{

	CAkVPLFinalMixNode* pFinalMixBus = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pFinalMix;

	// Master Bus

	for ( AkUInt32 iFx = 0; iFx < AK_NUM_EFFECTS_PER_OBJ; ++iFx )
	{
		AkFXDesc fxInfo;
		g_MasterBusCtx.GetFX( iFx, fxInfo );
		if ( fxInfo.pFx )
		{
			if ( fxInfo.pFx == in_pFx )
			{
				AK::IAkPluginParam * pParam = pFinalMixBus->GetPluginParam( iFx );
				if ( pParam )
					in_funcForAll( pParam, NULL, in_pCookie );
			}
		}
	}

	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL * pMixBus = *iterVPL;

		// take care of bus fx
		for ( AkUInt32 iFx = 0; iFx < AK_NUM_EFFECTS_PER_OBJ; ++iFx )
		{
			AkFXDesc fxInfo;
			pMixBus->m_MixBus.GetBusContext().GetFX( iFx, fxInfo );
			if ( fxInfo.pFx )
			{
				if ( fxInfo.pFx == in_pFx )
				{
					AK::IAkPluginParam * pParam = pMixBus->m_MixBus.GetPluginParam( iFx );
					if ( pParam )
						in_funcForAll( pParam, NULL, in_pCookie );
				}
			}
		}
	}

	for ( AkListVPLSrcs::Iterator itSrc = m_Sources.Begin(); itSrc != m_Sources.End(); ++itSrc )
	{
			CAkVPLSrcCbxNode * pCbx = *itSrc;
		CAkRegisteredObj * pGameObj = pCbx->GetContext()->GetGameObjectPtr();
		CAkSoundBase * pSoundBase = pCbx->GetContext()->GetSound();

		// Source Plugin

		CAkVPLSrcNode * pSrc = pCbx->m_pSources[ 0 ];
		if ( pSrc )
		{
			AK::IAkPluginParam * pParam = pSrc->GetPluginParam();
			if ( pParam )
			{
				if ( pSoundBase->Category() == ObjCategory_Sound )
				{
					CAkSound * pSound = (CAkSound *) pSoundBase;
					AkSrcTypeInfo * pSrcTypeInfo = pSound->GetSrcTypeInfo();
					CAkFxBase * pFx = g_pIndex->m_idxFxCustom.GetPtrAndAddRef( pSrcTypeInfo->mediaInfo.sourceID );
					if ( pFx )
					{
						if ( pFx == in_pFx )
							in_funcForAll( pParam, pGameObj, in_pCookie );
						pFx->Release();
					}
				}
			}
		}

		// Insert FX on sound

		for ( AkUInt32 iFx = 0; iFx < AK_NUM_EFFECTS_PER_OBJ; ++iFx )
		{
			CAkVPLFilterNodeBase * pFilter = pCbx->m_cbxRec.m_pFilter[ iFx ];
			if ( pFilter )
			{
				AkFXDesc fxInfo;
				pSoundBase->GetFX( iFx, fxInfo, pCbx->GetContext()->GetGameObjectPtr() );
				if ( fxInfo.pFx )
				{
					if ( fxInfo.pFx == in_pFx )
					{
						AK::IAkPluginParam * pParam = pFilter->GetPluginParam();
						if ( pParam )
							in_funcForAll( pParam, pGameObj, in_pCookie );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Name: ResolveCommandVPL
// Desc: Find a command's VPLSrc and VPL (for playing sounds, i.e. stop, pause, 
//       resume). Call only when resolution is needed (VPL and VPLSrc not set).
//       The command is resolved only if the source exists AND it is connected
//       to a bus.
//
// Parameter:
//	AkLECmd	& io_cmd: ( Stop, Pause, Resume ).
//-----------------------------------------------------------------------------
CAkVPLSrcCbxNodeBase * CAkLEngine::ResolveCommandVPL( AkLECmd & io_cmd )
{
	// Check parameters. 
	AKASSERT( io_cmd.m_pCtx != NULL && 
		io_cmd.m_bSourceConnected == false && 
		( io_cmd.m_eState == LEStateStop ||
		io_cmd.m_eState == LEStatePause ||
		io_cmd.m_eState == LEStateResume ||
		io_cmd.m_eState == LEStateStopLooping || 
		io_cmd.m_eState == LEStateSeek ) );

#ifdef AK_VITA_HW
		return ResolveCommandVPLHw( io_cmd );
#else
	CAkPBI * l_pCtx = io_cmd.m_pCtx;

	for( AkListVPLSrcs::Iterator iterVPLSrc = m_Sources.Begin(); iterVPLSrc != m_Sources.End();	++iterVPLSrc )
	{
			CAkVPLSrcCbxNode * pCbx = *iterVPLSrc;
		AKASSERT( pCbx != NULL );

		// Matching contexts found. WG-16144: only match with next source for stop looping command.
		if ( ( pCbx->m_pSources[ 0 ] && pCbx->m_pSources[ 0 ]->GetContext() == l_pCtx )
			|| ( io_cmd.m_eState == LEStateStopLooping && pCbx->m_pSources[ 1 ] && pCbx->m_pSources[ 1 ]->GetContext() == l_pCtx ) )
		{
			if( pCbx->GetState() != NodeStateInit )
			{
				// Set info.
				io_cmd.m_bSourceConnected = true;
			}
			return pCbx;
		}
	}

	// VPLSrc was not found. If it is unconnected, the PBI might still know about it;
	// if not, the command should be removed from the list.

	CAkVPLSrcCbxNodeBase * pCbx = l_pCtx->GetCbx();

	// WG-16144: only match with next source for stop looping command.
	// WG-16743: Cbx may have been cleared if playback failed during this audio frame.
	if ( pCbx 
		&& ( ( pCbx->m_pSources[ 0 ] && pCbx->m_pSources[ 0 ]->GetContext() == l_pCtx )
			|| io_cmd.m_eState == LEStateStopLooping ) )
	{
		return pCbx;
	}

	return NULL;
#endif
} // ResolveCommandVPL


AkVPL* CAkLEngine::GetAndConnectBus( CAkPBI * in_pCtx, CAkVPLSrcCbxNodeBase * in_pCbx, AkOutputDeviceID in_uDevice )
{
	AkVPL * pVPL;

	CAkBusCtx ctxBus;
	ctxBus.SetBus( in_pCtx->GetOutputBusPtr() );	

#ifdef AK_MOTION
	pVPL = GetVPLMixBus( ctxBus, in_uDevice, in_pCtx->IsForFeedbackPipeline() );
#else // AK_MOTION
	pVPL = GetVPLMixBus( ctxBus, in_uDevice );
#endif // AK_MOTION

	if( pVPL != NULL )
	{
		//---------------------------------------------------------------------------
		// Connect the source to VPL mix bus.
		//---------------------------------------------------------------------------
		if ( in_pCtx->IsHDR() )
			in_pCbx->SetHdrBus( (AkHdrBus*)pVPL->GetHdrBus() );

		in_pCbx->AddOutputBus(pVPL, in_uDevice, false);
	}

	return pVPL;
}

AkVPL* CAkLEngine::GetAuxBus(CAkBus* in_pAuxBus, AkMergedEnvironmentValue* in_pSend, AkOutputDeviceID in_uDevice, CAkVPLSrcCbxNodeBase * in_pCbx )
{
	AkVPL * pVPL = GetExistingVPLMixBus(in_pSend->auxBusID, in_uDevice);
	if (!pVPL)
	{
		CAkBusCtx l_BusContext;
		l_BusContext.SetBus(in_pAuxBus);

	#ifdef AK_MOTION
		pVPL = GetVPLMixBus( l_BusContext, in_uDevice, false );
	#else // AK_MOTION
		pVPL = GetVPLMixBus( l_BusContext, in_uDevice );
	#endif // AK_MOTION
	}

	if (pVPL)
	{
		in_pSend->PerDeviceAuxBusses.Set(in_uDevice, pVPL);
		pVPL->m_bReferenced = true;	

		//If the target bus is in the same hierarchy as the dry path, don't compute a new set of volumes.
		CAkBus* pBus = (CAkBus*)(in_pCbx->GetContext()->GetSound()->GetMixingBus());
		bool bIsDryInMainHierarchy = pBus == NULL || pBus->IsInMainHierarchy();
		if (pVPL && in_pAuxBus->IsInMainHierarchy() != bIsDryInMainHierarchy)
		{
			//Check if it was already added first.
			AkDeviceInfoList::Iterator it = in_pCbx->BeginBus();
			for (; it != in_pCbx->EndBus() && pVPL != (*it)->pMixBus; ++it)
				/*Empty, looking for pVPL == (*it).pMixBus*/;

			//Not found, add it.
			if (!(it != in_pCbx->EndBus()))	//Why !(!=) ? Because there is a != overload but not a == overload.
				in_pCbx->AddOutputBus(pVPL, in_uDevice, true);
		}		
	}
	else
	{
		AkDeviceVPLArray::Iterator it = in_pSend->PerDeviceAuxBusses.FindEx(in_uDevice);
		if ( it != in_pSend->PerDeviceAuxBusses.End() )
			in_pSend->PerDeviceAuxBusses.EraseSwap(it);
	}

	return pVPL;
}


AkForceInline AKRESULT ConnectBusForDevice(CAkVPLSrcCbxNode * in_pCbx, CAkPBI *in_pCtx, AkOutputDeviceID in_uDevice, AkVPL*& out_pVPL)
{
	//Is this sound going through the right output hierarchy?
	AKRESULT res = AK_Success;
	out_pVPL = NULL;
	CAkBus *pBus = (CAkBus*)in_pCtx->GetSound()->GetMixingBus();
	bool bBusIsMain = pBus == NULL || pBus->IsInMainHierarchy();
	bool bDeviceIsMain = in_uDevice == AK_MAIN_OUTPUT_DEVICE;
	if (bBusIsMain == bDeviceIsMain)
	{
		out_pVPL = CAkLEngine::GetAndConnectBus(in_pCtx, in_pCbx, in_uDevice);
		res = out_pVPL ? AK_Success : AK_Fail;
	}

	return res;
}

AKRESULT CAkLEngine::EnsureVPLExists(CAkVPLSrcCbxNode * in_pCbx, CAkPBI *in_pCtx)
{
	if ( !in_pCbx->IsVirtualVoice() )
	{
		if ( in_pCbx->AddPipeline() != AK_Success )
		{
			VPLDestroySource( in_pCbx, true );
			return AK_Fail;
		}
	}

	AkVPL* pVPL = NULL;
	const AkUInt32 uListeners = in_pCtx->GetListenerMask();
	bool bError = false;
	AKRESULT eResult;
	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
	{		
		if (uListeners & (*it).uListeners)
		{
			AkDeviceInfo *pDevice = in_pCbx->GetDeviceInfo((*it).uDeviceID);
			if ( pDevice == NULL )
			{
				//The Game Object and Device are tied through the listener.  But is the sound going in this output?
				eResult = ConnectBusForDevice(in_pCbx, in_pCtx, (*it).uDeviceID, pVPL);
				if (eResult != AK_Success)
				{
					bError = true;
					break;
				}
			}
		}
	}
		
	if (bError)
	{			
		VPLDestroySource( in_pCbx, true );
		return AK_Fail;
	}

	//The source list must be kept sorted by VPL.  Find where to insert.
	AkListVPLSrcs::IteratorEx it = m_Sources.BeginEx(); 
	for(; it != m_Sources.End() && (*it)->HasOutputDevice() && (*it)->BeginBus().pItem->pMixBus != pVPL; ++it )
		/*Empty on purpose*/;

	m_Sources.Insert(it, in_pCbx);	

	return AK_Success;
}

//-----------------------------------------------------------------------------
// Name: AddSound
// Desc: Add the sound to a new VPL or an existing VPL to play. 
//
// Parameters:
//	CAkPBI * in_pContext :	Pointer to a sound context.
//	AkUInt8	      in_bMode	  : Playing mode, in-game, or application.
//	bool		  in_bPaused  : true = sound is paused on play.
//
// Return: 
//  In general, remove the command from the list if the code is not AK_Success.
//	Ak_Success:          Sound was added.
//  AK_InvalidParameter: Invalid parameters.
//  AK_Fail:             Invalid pointer.
//  AK_AlreadyConnected: Sound is part of a sample accurate container, already
//                       part of a cbx node. The Play command should be removed.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::AddSound( AkLECmd & io_cmd )
{
	// Check parameters.
	AKASSERT( io_cmd.m_pCtx != NULL && 
		io_cmd.m_bSourceConnected == false && 
		io_cmd.m_pCtx->GetCbx() == NULL );

	
#ifdef AK_VITA_HW
	return AddSoundHw( io_cmd );
#else

	CAkPBI * l_pCtx	= io_cmd.m_pCtx;
	AKRESULT l_eResult = AK_Success;

	CAkVPLSrcCbxNode * pCbx = FindExistingVPLSrc( l_pCtx );
	if ( pCbx )
	{
		pCbx->AddSrc( l_pCtx, false );

		l_pCtx->NotifAddedAsSA();

		return AK_AlreadyConnected;
	}

	pCbx = AkNew( g_LEngineDefaultPoolId, CAkVPLSrcCbxNode );
	if( pCbx == NULL )
	{
		l_pCtx->Destroy( CtxDestroyReasonPlayFailed );
		return AK_Fail;
	}

	#ifndef AK_OPTIMIZED
		pCbx->m_PipelineID	= m_VPLPipelineID++;	// Profiling id.
		if ( m_VPLPipelineID == 0 )
			m_VPLPipelineID = 1; // 0 means bus
	#endif

	#ifdef AK_MOTION
		if (AK_EXPECT_FALSE(l_pCtx->IsForFeedbackPipeline()))
			pCbx->Init( AK_FEEDBACK_SAMPLE_RATE );
		else
			pCbx->Init( AK_CORE_SAMPLERATE );
	#else // AK_MOTION
		AKASSERT( ! l_pCtx->IsForFeedbackPipeline() );
		pCbx->Init( AK_CORE_SAMPLERATE );
	#endif // AK_MOTION

	l_eResult = pCbx->AddSrc( l_pCtx, true );
	if( l_eResult == AK_FormatNotReady )
	{
		// Format is not ready.
		// ProcessCommands() will resolve VPL connection when it can.
		// Store in list of sources not connected.
		CAkLEngineCmds::AddDisconnectedSource( pCbx );
		return AK_Success;
	}
	else if ( l_eResult != AK_Success)
	{
		// NOTE: on error the Context is destroyed in AddSrc();
		VPLDestroySource(pCbx, l_eResult != AK_PartialSuccess);
	}
	else
	{
 		l_eResult = EnsureVPLExists(pCbx, l_pCtx);
		io_cmd.m_bSourceConnected = (l_eResult == AK_Success);
	}

	return l_eResult;
#endif
} // AddSound

//-----------------------------------------------------------------------------
// Name: VPLTryConnectSource
// Desc: After having added a sound whose VPLCreateSource returned 
// 		 AK_FormatNotReady (pipeline not created), the renderer should call this
//		 function to complete the connection. 
//		 Tries adding pipeline, if successful, connects to bus.
//
// Parameters:
//	CAkPBI * in_pContext :	Pointer to a sound context.
//	AkVPLSrc * in_pVPLSrc	  : Incomplete VPL source.
//
// Return: 
//	Ak_Success:          VPL connection is complete.
//  AK_FormatNotReady: 	 Source not ready yet.
//  AK_Fail:             Error. Source is destroyed.
//-----------------------------------------------------------------------------
AKRESULT CAkLEngine::VPLTryConnectSource( CAkPBI * in_pContext, CAkVPLSrcCbxNodeBase * in_pCbx )
{
	AKASSERT( in_pCbx->GetState() != NodeStateStop );

	AKRESULT l_eResult = in_pCbx->FetchStreamedData( in_pContext );
	if ( l_eResult == AK_FormatNotReady )
	{
		// Streamed source is not ready or 
		// Frame offset greater than an audio frame: keep it in list of sources not connected.
		return AK_FormatNotReady;
	}

	//---------------------------------------------------------------------------
	// Source ready: remove from non-connected srcs list, connect to bus.
	//---------------------------------------------------------------------------
	CAkLEngineCmds::RemoveDisconnectedSource( in_pCbx );

	// ...or there was an error.
	if ( l_eResult == AK_Success )
	{
#ifdef AK_VITA_HW
		l_eResult = CAkLEngine::VPLConnectSourceHw( in_pContext, in_pCbx );
		if ( l_eResult != AK_Success )
			VPLDestroySource( in_pCbx, true );
		
		return l_eResult;
#else
		return EnsureVPLExists((CAkVPLSrcCbxNode*)in_pCbx, in_pContext);	
#endif
	}
	else
		VPLDestroySource( in_pCbx, true );

	return AK_Fail;
} // VPLTryConnectSource

AkVPL *	CAkLEngine::GetExistingVPLMixBus( AkUniqueID in_busID, AkOutputDeviceID in_uDevice )
{
	for( AkArrayVPL::Iterator iterVPL = m_arrayVPLs.Begin(); iterVPL != m_arrayVPLs.End(); ++iterVPL )
	{
		AkVPL *	l_pMixBus = *iterVPL;

		if( l_pMixBus->m_MixBus.GetBusID() == in_busID && l_pMixBus->m_uDevice == in_uDevice && l_pMixBus->m_MixBus.GetState() != NodeStateStop )
			return l_pMixBus;
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Name: GetVPLMixBus
// Desc: Get the VPL mix bus that is associated with the source.
//
// Return: 
//	AkVPL * : Pointer to a AkVPL.
//-----------------------------------------------------------------------------
AkVPL *	CAkLEngine::GetVPLMixBus(
	CAkBusCtx & in_ctxBus, AkOutputDeviceID in_uDevice
#ifdef AK_MOTION
	, bool in_bForFeedback
#endif // AK_MOTION
)
{
	AkVPL *		l_pMixBus	= GetExistingVPLMixBus( in_ctxBus.ID(), in_uDevice );

	if( l_pMixBus )
		return l_pMixBus;

	return GetVPLMixBusInternal( in_ctxBus, in_uDevice
#ifdef AK_MOTION
	, in_bForFeedback
#endif // AK_MOTION
	);
}

AkVPL *	CAkLEngine::GetVPLMixBusInternal(
	CAkBusCtx & in_ctxBus, AkOutputDeviceID in_uDevice
#ifdef AK_MOTION
	, bool in_bForFeedback
#endif // AK_MOTION
	)
{
	AkVPL * pParentBus = NULL;
	CAkBusCtx parentCtx =  in_ctxBus.GetParentCtx();
	if ( parentCtx.HasBus() )
	{
		pParentBus = GetVPLMixBus( parentCtx, in_uDevice
#ifdef AK_MOTION
			, in_bForFeedback 
#endif // AK_MOTION
			);
		if ( pParentBus == NULL )
			return NULL;
	}

	AkVPL* l_pMixBus = CreateVPLMixBus( in_ctxBus, in_uDevice, pParentBus
#ifdef AK_MOTION
		, in_bForFeedback 
#endif // AK_MOTION
		);
	if( l_pMixBus == NULL )
		return NULL;

	// The master bus itself calls SetInsertFx when (re)starting
	if( l_pMixBus && !in_ctxBus.IsTopBusCtx() )
	{
		l_pMixBus->m_MixBus.InitVolumes();			
		l_pMixBus->m_MixBus.UpdateBypassFx();		
	}

	return l_pMixBus;
}


void CAkLEngine::EnsureAuxBusExist( CAkVPLSrcCbxNodeBase * in_pCbx, AkMergedEnvironmentValue* in_pMergedEnvSlot )
{
	CAkPBI* pPBI = in_pCbx->GetContext();
	//Check if this auxiliary bus outputs in a secondary device
	CAkSmartPtr<CAkBus> spAuxBus;
	spAuxBus.Attach( (CAkBus*)g_pIndex->GetNodePtrAndAddRef( in_pMergedEnvSlot->auxBusID, AkNodeType_Bus ) );
	if( spAuxBus == NULL )
		return;

	const AkUInt32 uListeners = pPBI->GetListenerMask();

	if (spAuxBus->IsInMainHierarchy())
	{
		//Main hierarchy
		AkDevice * pDevice = CAkOutputMgr::GetDevice( AK_MAIN_OUTPUT_DEVICE );
		if ( uListeners & pDevice->uListeners )			// Only for devices connected to active listeners.
			GetAuxBus(spAuxBus, in_pMergedEnvSlot, AK_MAIN_OUTPUT_DEVICE, in_pCbx);
	}
	else
	{
		for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		{
			if ((*it).uDeviceID != AK_MAIN_OUTPUT_DEVICE	//Ignore the main device, we know this send doesn't go to it.
				&& uListeners & (*it).uListeners )			// Only for devices connected to active listeners.
				GetAuxBus(spAuxBus, in_pMergedEnvSlot, (*it).uDeviceID, in_pCbx);
		}
	}
}

//-----------------------------------------------------------------------------
// Name: CreateVPLMixBus
// Desc: Create a bus of specified type.
//
// Parameters:
//	bool	   in_bEnviron : true = environmental bus, false = not an env bus.
//  AkUniqueID  in_BusID   : Bus id.
//
// Return: 
//	AkVPL * : Pointer to a AkVPL.
//-----------------------------------------------------------------------------
AkVPL * CAkLEngine::CreateVPLMixBus( CAkBusCtx in_BusCtx, AkOutputDeviceID in_uDevice, AkVPL* in_pParentBus								
#ifdef AK_MOTION
	, bool in_bForFeedback
#endif // AK_MOTION
	)
{
	// Create the VPL mix bus.
	AkVPL * l_pMixBus;
	if ( in_BusCtx.HasBus() && in_BusCtx.GetBus()->IsHdrBus() )
	{
		AkNew2( l_pMixBus, g_LEngineDefaultPoolId, AkHdrBus, AkHdrBus( in_BusCtx ) );
	}
	else
	{
		AkNew2( l_pMixBus, g_LEngineDefaultPoolId, AkVPL, AkVPL() );
	}

	if( l_pMixBus == NULL )
		return NULL;

	AkUInt16 usMaxFrames;
	AkChannelMask uChannelMask;
	AkChannelMask uParentMask;
#ifdef AK_MOTION
	if ( in_bForFeedback )	
	{
		//The inner mixer is NOT used, so allocate the smallest possible buffer.
		usMaxFrames = AK_BUFFER_ALIGNMENT;
		uParentMask = uChannelMask = AK_SPEAKER_SETUP_MONO;
	}
	else
#endif // AK_MOTION
	{
		usMaxFrames = LE_MAX_FRAMES_PER_BUFFER;
		uParentMask = in_pParentBus ? in_pParentBus->m_MixBus.GetChannelMask() : CAkOutputMgr::GetDevice(in_uDevice)->pSink->GetSpeakerConfig();
		uChannelMask = in_BusCtx.GetChannelConfig();
		if (uChannelMask == AK_CHANNEL_MASK_PARENT)
			uChannelMask = uParentMask;
	}

#ifdef AK_VITA_HW
	CAkBusFX* pParentBusFx = in_pParentBus ? (CAkBusFX*)(&in_pParentBus->m_MixBus) : (CAkBusFX*) CAkOutputMgr::GetDevice(in_uDevice)->pFinalMix; 
	AKRESULT l_eResult = l_pMixBus->m_MixBus.Init( uChannelMask, uParentMask, usMaxFrames, in_BusCtx, pParentBusFx );
#else
	AKRESULT l_eResult = l_pMixBus->m_MixBus.Init( uChannelMask, uParentMask, usMaxFrames, in_BusCtx );
#endif
	
	if( l_eResult != AK_Success 
		|| m_arrayVPLs.AddLast( l_pMixBus ) == NULL )
	{
		l_eResult = AK_Fail;
		goto ErrorCreateVPLMixBus;
	}

	l_pMixBus->m_uDevice = in_uDevice;
	l_pMixBus->SetParent( in_pParentBus );
#ifdef AK_MOTION
	l_pMixBus->m_bFeedback = in_bForFeedback;
	if (!in_bForFeedback )
#endif // AK_MOTION
	{
		if ( in_pParentBus )
			in_pParentBus->m_MixBus.Connect();
		else
			CAkOutputMgr::GetDevice(in_uDevice)->pFinalMix->Connect( &l_pMixBus->m_MixBus );
	}

ErrorCreateVPLMixBus :

	if( l_eResult != AK_Success )
	{
		if( l_pMixBus != NULL )
		{
			AkDelete( g_LEngineDefaultPoolId, l_pMixBus );
			l_pMixBus = NULL;
		}
	}
	return l_pMixBus;

} // CreateVPLMixBus

void CAkLEngine::DestroyAllVPLMixBusses()
{
	// Destroy from end of array so that child busses are destroyed before their parents
	for ( int iVPL = m_arrayVPLs.Length() - 1; iVPL >= 0; --iVPL )
		AkDelete( g_LEngineDefaultPoolId, m_arrayVPLs[ iVPL ] );

	m_arrayVPLs.RemoveAll();
}

//-----------------------------------------------------------------------------
// Name: FindExistingVPLSrc
// Desc: Find an existing VPLSrc for the given PBI. Used for sample-accurate transitions.
//-----------------------------------------------------------------------------
CAkVPLSrcCbxNode * CAkLEngine::FindExistingVPLSrc( CAkPBI * in_pCtx )
{
	AkUniqueID l_ID = in_pCtx->GetSequenceID();
	if( l_ID == AK_INVALID_SEQUENCE_ID )
		return NULL;

	for( AkListVPLSrcs::Iterator iterSrc = m_Sources.Begin(); iterSrc != m_Sources.End(); ++iterSrc )
	{
		// Should only need to look at the first sound to see if there is a match.
			CAkVPLSrcCbxNode * pCbx = *iterSrc;

		if( pCbx->GetContext()->GetSequenceID() == l_ID )
			return pCbx;
	}

	return NULL;
} // FindExistingVPLSrc

//---------------------------------------------------------------------
// Check if the Voice is starving
//---------------------------------------------------------------------
void CAkLEngine::HandleStarvation()
{
	//Check if any device reported starvation.
	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
	{
		if( (*it).pSink->IsStarved() )
		{
			(*it).pSink->ResetStarved();

			AkUInt32 uTimeNow = g_pAudioMgr->GetBufferTick();
			if( m_uLastStarvationTime == 0 || uTimeNow - m_uLastStarvationTime > MINIMUM_STARVATION_NOTIFICATION_DELAY )
			{
				MONITOR_ERROR( AK::Monitor::ErrorCode_VoiceStarving );
				m_uLastStarvationTime = uTimeNow;
				break;
			}
		}
	}

#ifdef AK_MOTION
	//Go through all the feedback devices too
	if (AK_EXPECT_FALSE(IsFeedbackEnabled()))
		m_pDeviceMgr->HandleStarvation();
#endif // AK_MOTION
}

#ifdef AK_MOTION
//-----------------------------------------------------------------------------
// Name: EnableFeedbackPipeline
// Desc: Enable the feedback pipeline.  This should be called only if there are
//		 devices to drive.  This can be called many times and once enabled, it
//		 stays enabled.
//
// Parameters:None.
// Return: None.
//-----------------------------------------------------------------------------

void CAkLEngine::EnableFeedbackPipeline()
{
	m_pDeviceMgr = CAkFeedbackDeviceMgr::Get();
}

bool CAkLEngine::IsFeedbackEnabled()
{
	return m_pDeviceMgr != NULL && m_pDeviceMgr->IsFeedbackEnabled();
}
#endif // AK_MOTION

void CAkLEngine::ReevaluateBussesForDevice(AkOutputDeviceID in_uDevice, AkUInt32 in_uListenerMask)
{
	// Go through all sources and check if they output on the given device
	// Check also that the listener mask
	// There are 4 cases
	// A) The source already outputs on that device (has a VPL) and the new listeners also match the game object's and device's (Nothing to do)
	// B) The source already outputs on that device (has a VPL) and the new listeners don't match the game object's and device's (remove the VPL)
	// C) The source doesn't output on that device (no VPL) but the new listeners also match the game object's and device's (Add VPL) 
	// D) The source doesn't output on that device (no VPL) but the new listeners don't match the game object's and device's (Nothing to do) 

	for ( AkListVPLSrcs::IteratorEx itSrc = m_Sources.BeginEx(); itSrc != m_Sources.End(); ++itSrc)
	{
		CAkVPLSrcCbxNode* pCbx = *itSrc;
		CAkPBI * pCtx = pCbx->GetContext();
		bool bListenerHears = (pCtx->GetGameObjectPtr()->GetListenerMask() & in_uListenerMask) != 0;

		bool bFound = false;
		AkVPL* pVPL = NULL;
		AkDeviceInfoList::IteratorEx it = pCbx->BeginBusEx();
		while(it != pCbx->EndBus())
		{
			if ((*it)->uDeviceID == in_uDevice)
			{
				pVPL = (*it)->pMixBus;
				if (pVPL && !bListenerHears)
				{
					bFound = true;
					//Case B.  Remove the VPL			
					it = pCbx->RemoveOutputBus(it);			

	#ifndef AK_OPTIMIZED
					if (!pCbx->HasOutputDevice())
					{
						pCtx->Monitor(AkMonitorData::NotificationReason_VirtualNoListener);
					}
	#endif
					continue;
				}
			}
			++it;
		}

		if (!bFound && bListenerHears)
		{
			//Case C, not playing on device but should
			ConnectBusForDevice(pCbx, pCtx, in_uDevice, pVPL);
		}
	}
}

void CAkLEngine::ReevaluateBussesForGameObject(CAkRegisteredObj* in_pObj, AkUInt32 in_uOldMask, AkUInt32 in_uNewMask)
{
	// If a game object listener mask changes and makes the object drop or appear on a different output, we must do something with the VPL.
	// A) The old GameObject mask, new mask and Device mask all have common bits: The VPL still plays on the device, nothing to do
	// B) The old GameObject mask, new mask and Device mask have no common bits: The VPL doesn't play on this device anymore, kill the VPL
	// C) The new mask and Device mask have common bits, but not the old mask: The sound is now heard by a new device.  New VPL needed.
	
	AkUInt32 uOldMask = in_pObj->GetListenerMask();
	
	for ( AkListVPLSrcs::IteratorEx itSrc = m_Sources.BeginEx(); itSrc != m_Sources.End(); ++itSrc)
	{
		CAkVPLSrcCbxNode* pCbx = *itSrc;
		CAkPBI * pCtx = pCbx->GetContext();
		if (pCtx->GetGameObjectPtr() != in_pObj)
			continue;	//Ignore sources that don't play on this object.

		for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		{
			AkUInt32 uNewCommon = (*it).uListeners & in_uNewMask;
			AkUInt32 uOldCommon = (*it).uListeners & uOldMask;
			if (uNewCommon)
			{
				// Case A or C, the device hears the object with the new masks
				if (uOldCommon == 0)
				{
					//Case C.  New VPL needed.
					AkVPL *pVPL;
					ConnectBusForDevice(pCbx, pCtx, (*it).uDeviceID, pVPL);
				}
			}
			else if (uOldCommon)
			{
				//Case B.  It was playing on this device but doesn't now (it would have gone in the previous branch)
				//Remove the VPL.
				AkVPL* pVPL = NULL;
				AkDeviceInfoList::IteratorEx itVPL = pCbx->BeginBusEx();
				while (itVPL != pCbx->EndBus())
				{
					if ((*itVPL)->uDeviceID == (*it).uDeviceID)
						itVPL = pCbx->RemoveOutputBus(itVPL);
					else
						++itVPL;
				}				
			}
		}

#ifndef AK_OPTIMIZED
		if (!pCbx->HasOutputDevice())
		{
			pCtx->Monitor(AkMonitorData::NotificationReason_VirtualNoListener);
		}
#endif
	}
}


void CAkLEngine::FinishRun( CAkVPLSrcCbxNode * in_pCbx, AkVPLState & io_state )
{
	if ( io_state.result == AK_NoMoreData && !io_state.bStop )
	{
		// This sample-accurate voice could not be handled sample-accurately; do it here for
		// next frame.

		CAkVPLSrcNode * pNextSrc = in_pCbx->m_pSources[ 1 ];
		if ( pNextSrc )
		{
			in_pCbx->m_pSources[ 1 ] = NULL;

			in_pCbx->RemovePipeline( CtxDestroyReasonFinished );

			AKRESULT eAddResult = in_pCbx->AddSrc( pNextSrc, true, false );

			if ( eAddResult == AK_Success )
				eAddResult = in_pCbx->AddPipeline();

			if ( eAddResult == AK_Success )
			{
				pNextSrc->Start();
			}
			else 
			{
				if ( eAddResult == AK_FormatNotReady )
				{
					// Missed the boat. Too bad. Return as NoMoreData (what we stitched so far)
					// REVIEW: this will cause this sound to be skipped, but we can't return DataReady
					// as the pipeline doesn't support running voices that are not IsIOReady()
					MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_TransitionNotAccurateStarvation, pNextSrc->GetContext() );
				}

				in_pCbx->Stop();
			}
		}
		else
		{
			in_pCbx->Stop();
		}
	}
	else if ( io_state.result == AK_Fail || io_state.bStop )
	{
		in_pCbx->Stop();

	}
	else if ( io_state.bPause )
		in_pCbx->Pause();
}

// TO DO: Make this a method of AkSIMDSpeakerVolumes
static AkForceInline void ApplyVolumes( AkSIMDSpeakerVolumes& out_Volumes, const AkSIMDSpeakerVolumes& in_Volumes, AkReal32 in_fControlValue )
{
#if defined( AK_LFECENTER ) && defined( AK_REARCHANNELS )
#ifdef AKSIMD_V2F32_SUPPORTED
	AKSIMD_V2F32 vTmp = AKSIMD_SET_V2F32(in_fControlValue);
	out_Volumes.m_vector[0] = AKSIMD_MUL_V2F32( in_Volumes.m_vector[0], vTmp );
	out_Volumes.m_vector[1] = AKSIMD_MUL_V2F32( in_Volumes.m_vector[1], vTmp );
	out_Volumes.m_vector[2] = AKSIMD_MUL_V2F32( in_Volumes.m_vector[2], vTmp );
#ifdef AK_71AUDIO
	out_Volumes.m_vector[3] = AKSIMD_MUL_V2F32( in_Volumes.m_vector[3], vTmp );
#endif
#else
	AKSIMD_V4F32 vTmp = AKSIMD_LOAD1_V4F32( in_fControlValue ); 
	*((AKSIMD_V4F32*)&out_Volumes.vector[0]) = AKSIMD_MUL_V4F32( *((AKSIMD_V4F32*)&in_Volumes.vector[0]), vTmp );
	*((AKSIMD_V4F32*)&out_Volumes.vector[1]) = AKSIMD_MUL_V4F32( *((AKSIMD_V4F32*)&in_Volumes.vector[1]), vTmp );
#endif //AKSIMD_V2F32_SUPPORTED
#else // defined( AK_LFECENTER ) && defined( AK_REARCHANNELS )
#ifdef AKSIMD_V2F32_SUPPORTED
	AKSIMD_V2F32 vTmp = AKSIMD_SET_V2F32( in_fControlValue ); 
	out_Volumes.m_vector = AKSIMD_MUL_V2F32( in_Volumes.m_vector, vTmp );
#else
	out_Volumes.volumes.fFrontLeft = in_Volumes.volumes.fFrontLeft * in_fControlValue;
	out_Volumes.volumes.fFrontRight = in_Volumes.volumes.fFrontRight * in_fControlValue;
#endif
#endif // defined( AK_LFECENTER ) && defined( AK_REARCHANNELS )
}

void CAkLEngine::AnalyzeMixingGraph()
{
	// First pass: Compute cumulative bus gains, setup voice volume rays and send values, create aux busses.
	{
#ifndef AK_OPTIMIZED

		//TODO!  Watch and meter all final busses.  For now, continue to watch only the main master bus.
		CAkVPLFinalMixNode* pMainFinalMix = CAkOutputMgr::GetDevice(AK_MAIN_OUTPUT_DEVICE)->pFinalMix;
		pMainFinalMix->ResetMixingVoiceCount();

		if ( AkMonitor::GetAndClearMeterWatchesDirty() )
		{
			// Final mix node.
			if ( g_MasterBusCtx.GetBus() )
				pMainFinalMix->RefreshMeterWatch( g_MasterBusCtx.GetBus()->ID() );

			// Other mix nodes.
			AkArrayVPL::Iterator it = m_arrayVPLs.Begin();
			while ( it != m_arrayVPLs.End() )
			{
				/// TODO Remove argument and get bus ID from inside RefreshMeterWatch(). 
				/// Currently this way to work around final bus specificity.
				AkUniqueID busID = (*it)->m_MixBus.GetBusID();
				if( busID == AK_INVALID_UNIQUE_ID )
					busID = g_MasterBusCtx.GetBus()->ID();
				///

				(*it)->m_MixBus.RefreshMeterWatch( busID );
				++it;
			}
		}
#endif
		// Compute volume for sources.  Aux Bus pertaining to each source will be updated too.
		for ( AkListVPLSrcs::IteratorEx itSrc = m_Sources.BeginEx(); itSrc != m_Sources.End(); )
		{
			if ( (*itSrc)->GetState() == NodeStatePlay )
			{
				if (!(*itSrc)->ComputeVolumeRays() /*ComputeVolumeRays return true if the source uses aux busses*/)
					(*itSrc)->CleanupAuxBusses();	//This source doesn't use aux busses anymore.  Make sure that there is no leftovers.
			}	
			else
				(*itSrc)->CleanupAuxBusses();	//This source doesn't play, make sure that there is no leftovers.				

			++itSrc;
		}

		AkUInt32 uVPL = 0;
		while ( uVPL < m_arrayVPLs.Length() )
		{
			AkVPL * pVPL = m_arrayVPLs[ uVPL ];

			pVPL->m_fDownstreamGain = pVPL->m_MixBus.GetNextVolume();
			if (pVPL->GetParent())
				pVPL->m_fDownstreamGain *= pVPL->GetParent()->m_fDownstreamGain;
			else 
			{
				AkDevice * pDevice = CAkOutputMgr::GetDevice(pVPL->m_uDevice);
				if (pDevice)
					pVPL->m_fDownstreamGain *= pDevice->pFinalMix->GetNextVolume();
			}

#ifndef AK_OPTIMIZED
			pVPL->m_MixBus.ResetMixingVoiceCount();
#endif
			++uVPL;
		}				
	}

	// Second pass, upwards: Voice setup (max volume) and HDR if applicable.

	// Process all sources
	for ( AkListVPLSrcs::IteratorEx itSrc = m_Sources.BeginEx(); itSrc != m_Sources.End(); )
	{
		if ( (*itSrc)->GetState() == NodeStatePlay )
			(*itSrc)->ComputeMaxVolume();
		++itSrc;
	}

	for ( int iVPL = m_arrayVPLs.Length() - 1; iVPL >= 0; --iVPL )
	{
		AkVPL * pVPL = m_arrayVPLs[ iVPL ];
		if ( pVPL->IsHDR() ) 
			((AkHdrBus*)pVPL)->ComputeHdrAttenuation();
	}
}

// -----------------------------------------------------------------
// AkVPL
// -----------------------------------------------------------------
AkVPL::~AkVPL()
{
	if ( GetParent() )
		GetParent()->m_MixBus.Disconnect();
}

// -----------------------------------------------------------------
// AkHdrBus
// -----------------------------------------------------------------
AkHdrBus::AkHdrBus( CAkBusCtx in_BusCtx )
: m_fHdrMaxVoiceVolume( AK_SAFE_MINIMUM_VOLUME_LEVEL )
, m_fHdrWinTopState( AK_SAFE_MINIMUM_VOLUME_LEVEL )	// Use very small value to avoid releasing right from the start.
, m_fReleaseCoef( 0 )
{ 
	m_bIsHDR = true;
	
	CAkBus * pBus = in_BusCtx.GetBus();
	AKASSERT( pBus );

	// Cache gain computer the first time.
	AkReal32 fRatio;
	pBus->GetHdrGainComputer( m_fThreshold, fRatio );
	m_fGainFactor = ( 1 - ( 1.f / fRatio ) );

	// Cache ballistics the first time.
	AkReal32 fReleaseTime;
	bool bReleaseModeExponential;
	pBus->GetHdrBallistics( fReleaseTime, bReleaseModeExponential );
	m_bExpMode = bReleaseModeExponential;
	if ( fReleaseTime > 0 )
		m_fReleaseCoef = exp( -AK_NUM_VOICE_REFILL_FRAMES / ( DEFAULT_NATIVE_FREQUENCY * fReleaseTime ) );
	else
		m_fReleaseCoef = 0;
}

#define AK_EPSILON_HDR_RELEASE	(0.5f)	// dBs
void AkHdrBus::ComputeHdrAttenuation()
{
	CAkBus * pBus = m_MixBus.GetBusContext().GetBus();

	// Compute and cache downstream gain in dB
	AkReal32 fDownstreamGainDB = AkMath::FastLinTodB( m_fDownstreamGain );
	m_fDownstreamGainDB = fDownstreamGainDB;

	// Derive gain computer only if necessary
	AkReal32 fHdrThreshold, fRatio;
	AkReal32 fGainFactor;
	if ( !pBus->GetHdrGainComputer( fHdrThreshold, fRatio ) )
		fGainFactor = m_fGainFactor;
	else
	{
		m_fThreshold = fHdrThreshold;
		fGainFactor = ( 1 - ( 1.f / fRatio ) );
		m_fGainFactor = fGainFactor;
	}

	// Compute window top target.
	// m_fHdrWinTopTarget is expressed globally (ie comprises downstream gain). Compute window target locally 
	// for this bus. Very important: we must not filter gain changes of downstream mix stages!
	AkReal32 fMaxVoiceVolume = m_fHdrMaxVoiceVolume - fDownstreamGainDB - fHdrThreshold;
	AkReal32 fWinTopTarget = fHdrThreshold;
	if ( fMaxVoiceVolume > 0 )
		fWinTopTarget += ( fGainFactor * fMaxVoiceVolume );

	// Derive ballistics only if necessary.
	AkReal32 fReleaseTime;
	bool bReleaseModeExponential;
	if ( pBus->GetHdrBallistics( fReleaseTime, bReleaseModeExponential ) )
	{
		if ( fReleaseTime > 0 )
			m_fReleaseCoef = exp( -AK_NUM_VOICE_REFILL_FRAMES / ( DEFAULT_NATIVE_FREQUENCY * fReleaseTime ) );
		else
			m_fReleaseCoef = 0;

#ifndef AK_OPTIMIZED
		if ( bReleaseModeExponential != m_bExpMode )
		{
			// Release mode changed (live edit only). Ensure we take the "attack" path.
			m_bExpMode = bReleaseModeExponential;
			m_fHdrWinTopState = AK_SAFE_MINIMUM_VOLUME_LEVEL;
		}
#endif
	}
	AkReal32 fReleaseCoef = m_fReleaseCoef;
	
	// This is the target. Now, get the real one.
	AkReal32 fWindowTop;
	if ( bReleaseModeExponential )
	{
		// This is the target. Now, get the real one.
		if ( fWinTopTarget >= m_fHdrWinTopState )
		{
			// Instantaneous attack.
			m_fHdrWinTopState = fWinTopTarget;
		}
		else
		{
			// Releasing.
			m_fHdrWinTopState = fReleaseCoef * m_fHdrWinTopState + ( 1 - fReleaseCoef ) * fWinTopTarget;

			// Force us to remain alive if we're more than AK_EPSILON_HDR_RELEASE dB away from target value.
			if ( ( m_fHdrWinTopState - fWinTopTarget ) >= AK_EPSILON_HDR_RELEASE )
				m_bReferenced = true;
		}

		fWindowTop = m_fHdrWinTopState;
	}
	else
	{
		AkReal32 fWindowTopLin = AkMath::dBToLin( fWinTopTarget );

		if ( fWindowTopLin >= m_fHdrWinTopState )
		{
			// Instantaneous attack.
			m_fHdrWinTopState = fWindowTopLin;
			fWindowTop = fWinTopTarget;
		}
		else
		{
			// Releasing.
			m_fHdrWinTopState = fReleaseCoef * m_fHdrWinTopState + ( 1 - fReleaseCoef ) * fWindowTopLin;
			fWindowTop = AkMath::FastLinTodB( m_fHdrWinTopState );

			// Force us to remain alive if we're more than AK_EPSILON_HDR_RELEASE dB away from target value.
			if ( ( fWindowTop - fWinTopTarget ) >= AK_EPSILON_HDR_RELEASE )
				m_bReferenced = true;
		}
	}

	// Cache result for voices (global value). Push real window top to behavioral engine.
	m_fHdrWinTop = fWindowTop + fDownstreamGainDB;
	pBus->NotifyHdrWindowTop( fWindowTop );

	// Reset for next frame.
	m_fHdrMaxVoiceVolume = AK_SAFE_MINIMUM_VOLUME_LEVEL;
}

// -----------------------------------------------------------------

void CAkLEngine::RemoveMixBusses()
{
	// Destroy from end of array so that child busses are destroyed before their parents
	for ( int iVPL = m_arrayVPLs.Length() - 1; iVPL >= 0; --iVPL )
	{
		AkVPL * pVPL = m_arrayVPLs[ iVPL ];

		// Delete the bus if it is not in tail processing, has no actives sources and no child busses.
		if( pVPL->CanDestroy() )
		{
			AkDelete( g_LEngineDefaultPoolId, pVPL );
			m_arrayVPLs.Erase( iVPL ); // Need to preserve ordering of busses
		}
		else
		{
			pVPL->OnFrameEnd();
		}
	}
}

#if ! defined( AK_PS3 )

// Optimized version of single-pipeline execution.
void CAkLEngine::RunVPL( AkRunningVPL & io_runningVPL )
{
	CAkVPLSrcCbxNode * AK_RESTRICT pCbx = io_runningVPL.pCbx;
	AkVPLSrcCbxRec & cbxRec = pCbx->m_cbxRec;

	AkUInt32 uFXIndex = AK_NUM_EFFECTS_PER_OBJ;

GetFilter:
	while ( uFXIndex > 0 )
	{
		CAkVPLFilterNodeBase * pFilter = cbxRec.m_pFilter[ --uFXIndex ];
		if ( pFilter )
		{
			pFilter->GetBuffer( io_runningVPL.state );
			if ( io_runningVPL.state.result != AK_DataNeeded )
			{
				if ( io_runningVPL.state.result == AK_DataReady
					|| io_runningVPL.state.result == AK_NoMoreData )
				{
					++uFXIndex;
					goto ConsumeFilter;
				}
				else
				{
					return;
				}
			}
		}
	}

	// Pitch-FmtConv-Source loop
GetPitch:
	cbxRec.m_Pitch.GetBuffer( io_runningVPL.state );
	if ( io_runningVPL.state.result != AK_DataNeeded )
	{
		if ( io_runningVPL.state.result == AK_DataReady
			|| io_runningVPL.state.result == AK_NoMoreData )
		{
			goto ConsumeFilter;
		}
		else
		{
			return;
		}
	}

	do
	{
#ifdef AK_MOTION
		if (io_runningVPL.bFeedbackVPL)
			io_runningVPL.state.SetRequestSize( AK_FEEDBACK_MAX_FRAMES_PER_BUFFER );
		else
#endif // AK_MOTION
			io_runningVPL.state.SetRequestSize( LE_MAX_FRAMES_PER_BUFFER );

		pCbx->m_pSources[ 0 ]->GetBuffer( io_runningVPL.state );
#ifndef AK_OPTIMIZED
		if ( io_runningVPL.state.result == AK_DataReady 
			|| io_runningVPL.state.result == AK_NoMoreData )
		{
			if( pCbx->m_iWasStarvationSignaled )
			{
				--pCbx->m_iWasStarvationSignaled;
			}
		}
		else 
#endif
		{
			/// IMPORTANT: This needs to be done in Release also.
			if ( io_runningVPL.state.result == AK_NoDataReady )
				pCbx->HandleSourceStarvation();
		}

		if ( io_runningVPL.state.result != AK_DataReady
			&& io_runningVPL.state.result != AK_NoMoreData )
		{
			return;
		}

		cbxRec.m_Pitch.ConsumeBuffer( io_runningVPL.state );
		if ( io_runningVPL.state.result != AK_DataNeeded )
		{
			if ( io_runningVPL.state.result == AK_DataReady
				|| io_runningVPL.state.result == AK_NoMoreData )
			{
				goto ConsumeFilter;
			}
			else
			{
				return;
			}
		}
	}
	while(1);

ConsumeFilter:
	while ( uFXIndex < AK_NUM_EFFECTS_PER_OBJ )
	{
		CAkVPLFilterNodeBase * pFilter = cbxRec.m_pFilter[ uFXIndex ];
		if ( pFilter )
		{
			pFilter->ConsumeBuffer( io_runningVPL.state );
			if ( io_runningVPL.state.result == AK_DataNeeded )
			{
				if ( uFXIndex > 0 )
				{
					goto GetFilter;
				}
				else
				{
					goto GetPitch;
				}
			}
			else if ( io_runningVPL.state.result != AK_DataReady
 				&& io_runningVPL.state.result != AK_NoMoreData )
 			{
 				return;
			}
		}
		uFXIndex++;
	}

	cbxRec.m_LPF.ConsumeBuffer( io_runningVPL.state );
	AKASSERT( io_runningVPL.state.result == AK_DataReady
		|| io_runningVPL.state.result == AK_NoMoreData ); // LPF has no failure case

	pCbx->ConsumeBuffer( io_runningVPL.state );

	if ( io_runningVPL.state.result != AK_DataReady
		&& io_runningVPL.state.result != AK_NoMoreData )
	{
		return;
	}

	//Notify markers
	g_pPlayingMgr->NotifyMarkers( io_runningVPL.state );

	CAkPBI * AK_RESTRICT pPBI = pCbx->GetContext();

#ifdef AK_MOTION
	//Route audio to the feedback pipeline, if needed.
	if( pPBI->GetFeedbackParameters()
		&& pCbx->GetNumDevices() > 0 )	/// REVIEW Only if audible?
	{
		if (m_pDeviceMgr->PrepareAudioProcessing(io_runningVPL))
		{
			m_pDeviceMgr->ApplyMotionLPF(io_runningVPL);
			m_pDeviceMgr->ConsumeVPL(io_runningVPL);
			m_pDeviceMgr->CleanupAudioVPL(io_runningVPL);
		}
		else
			m_pDeviceMgr->ConsumeVPL(io_runningVPL);
	}
#endif // AK_MOTION

	// skip the rest of the processing chain if not audible.
	if ( !io_runningVPL.state.bAudible )
		return;

	AkUInt32 uNumChannels = io_runningVPL.state.NumChannels();
	if ( io_runningVPL.state.bIsAuxRoutable )
	{
		AkUInt8 uNumSends;
		const AkMergedEnvironmentValue * pSends = pCbx->GetSendValues( uNumSends );
		for( int i = 0; i < uNumSends; i++ )
		{
			AkAudioMix mixAux[AK_VOICE_MAX_NUM_CHANNELS];
			//Mix the voice into all output bus hierarchies
			for (AkDeviceVPLArray::Iterator it = pSends[i].PerDeviceAuxBusses.Begin(); it != pSends[i].PerDeviceAuxBusses.End(); ++it)
			{
				CAkVPLMixBusNode &rBus = (*it).item->m_MixBus;
#ifndef AK_OPTIMIZED
				if( CheckBusMonitoringMute( rBus.GetBusContext().GetBus() ) )
				{
					rBus.ConsumeBufferMute( io_runningVPL.state );
				}
				else
#endif
				{
					// Copy "send" device volumes onto mixAux and apply control values.
					const AkDeviceInfo * pMix = pCbx->GetDeviceInfo( (*it).key );
					//AKASSERT(pMix);
					if (pMix)
					{
						if( pSends[i].eAuxType == AkAuxType_GameDef )
						{
							for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
							{
								ApplyVolumes(mixAux[iChannel].Next,		pMix->mxDirect[iChannel].Next,		pMix->mxAttenuations.gameDef.fNext * pSends[i].fControlValue);
								ApplyVolumes(mixAux[iChannel].Previous, pMix->mxDirect[iChannel].Previous,	pMix->mxAttenuations.gameDef.fPrev * pSends[i].fLastControlValue);
							}
						}
						else
						{
							for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
							{
								ApplyVolumes(mixAux[iChannel].Next,		pMix->mxDirect[iChannel].Next,		pMix->mxAttenuations.userDef.fNext * pSends[i].fControlValue);
								ApplyVolumes(mixAux[iChannel].Previous, pMix->mxDirect[iChannel].Previous,	pMix->mxAttenuations.userDef.fPrev * pSends[i].fLastControlValue);
							}
						}

						// Add buffer to environmental bus				
						rBus.ConsumeBuffer( io_runningVPL.state, mixAux );
					}
				}
			}
		}
	}

	pCbx->ConsumeBufferForObstruction( io_runningVPL.state );

#ifdef AK_MOTION
	if (!io_runningVPL.bFeedbackVPL)
#endif // AK_MOTION

	//Mix the voice into all output bus hierarchies
	for (AkDeviceInfoList::Iterator it = pCbx->BeginBus(); it != pCbx->EndBus(); ++it)
	{
		AkAudioMix mixTemp[AK_VOICE_MAX_NUM_CHANNELS];
		// Copy device volumes onto mixTemp and apply control values.
		const AkDeviceInfo * pMix = (*it);
		if (!pMix->bCrossDeviceSend)	//Don't mix into VPL that go in different devices, it should be processed in the Aux Bus loop above.
		{
			for(unsigned int iChannel=0; iChannel<uNumChannels; iChannel++)
			{
				ApplyVolumes(mixTemp[iChannel].Next,	 pMix->mxDirect[iChannel].Next,		pMix->mxAttenuations.dry.fNext );
				ApplyVolumes(mixTemp[iChannel].Previous, pMix->mxDirect[iChannel].Previous,	pMix->mxAttenuations.dry.fPrev );
			}

	#ifndef AK_OPTIMIZED
			if ( CheckBusMonitoringMute( pPBI->GetControlBus() )  )
				pMix->pMixBus->m_MixBus.ConsumeBufferMute( io_runningVPL.state );
			else
	#endif
				pMix->pMixBus->m_MixBus.ConsumeBuffer( io_runningVPL.state, mixTemp );
		}
	}
}


// Note: If you change this function BEWARE!  Some platforms don't use it!  Look for CAkLEngine::Perform.
void CAkLEngine::SoftwarePerform()
{
	CAkLEngineCmds::ProcessAllCommands();
	HandleStarvation();

	GetBuffer();

#ifdef AK_MOTION
	//ALWAYS call RenderData.  Even if we didn't process any VPL, there might be data coming from the audio pipeline.
	if(IsFeedbackEnabled())
	{
		AK_START_TIMER_FEEDBACK();
		m_pDeviceMgr->RenderData();
		AK_STOP_TIMER_FEEDBACK();
	}
#endif

	RemoveMixBusses();
} // Perform

void CAkLEngine::GetBuffer()
{
	g_pPositionRepository->UpdateTime();

	CAkLEngineCmds::ProcessDisconnectedSources( LE_MAX_FRAMES_PER_BUFFER );

	AnalyzeMixingGraph();

	// Voice processing.  Process each "sequence" as a whole.	
	for ( AkListVPLSrcs::IteratorEx itSrc = m_Sources.BeginEx(); itSrc != m_Sources.End(); )
	{
		CAkVPLSrcCbxNode * pCbx = (CAkVPLSrcCbxNode *)*itSrc;
		AkRunningVPL runningVPL;

		//Do not call EMPTY_BUFFER() to avoid setting fields in case of virtual voice
		runningVPL.state.SetRequestSize( AK_NUM_VOICE_REFILL_FRAMES );
		
		runningVPL.state.bPause = false;
		runningVPL.state.bStop = false;
		runningVPL.state.result = AK_DataNeeded;
#ifdef AK_MOTION
		runningVPL.bFeedbackVPL = pCbx->GetContext()->IsForFeedbackPipeline();
		runningVPL.pFeedbackData = NULL;
		if(runningVPL.bFeedbackVPL)
			runningVPL.state.SetRequestSize( AK_FEEDBACK_MAX_FRAMES_PER_BUFFER );
#endif // AK_MOTION

		if ( pCbx->GetState() == NodeStatePlay ) 
		{
			if ( pCbx->StartRun( runningVPL.state ) )
			{
				runningVPL.pCbx = pCbx;

				// Clear only necessary fields of buffer
				// Keep request size ("max frames").
				AKASSERT( !runningVPL.state.HasData() );
				runningVPL.state.uValidFrames = 0;
				runningVPL.state.uNumMarkers = 0;
				runningVPL.state.pMarkers = NULL;
				runningVPL.state.posInfo.uStartPos = (AkUInt32)-1;

#ifdef AK_MOTION
				if (runningVPL.bFeedbackVPL)
				{
					AK_START_TIMER_FEEDBACK();
					RunVPL( runningVPL );
					AK_STOP_TIMER_FEEDBACK();
				}
				else
#endif // AK_MOTION
					RunVPL( runningVPL );

				// Release buffer in all cases, except if the source has starved.
				// In such a case it is kept for next LEngine pass.
				if ( runningVPL.state.result != AK_NoDataReady )
				{
					runningVPL.pCbx->ReleaseBuffer();
				}
				else
				{
					// We already computed new volumes, but did not use it because data is not ready.
					// Restore them for next time.
					runningVPL.pCbx->RestorePreviousVolumes( &runningVPL.state );
				}
			}
		}
		else
		{
			// Ensure PBI stopped state is looked at, even when not playing.
			CAkPBI * pCtx = pCbx->GetContext();
			if ( pCtx && pCtx->WasStopped() )
				runningVPL.state.bStop = true;
		}
		
		FinishRun( pCbx, runningVPL.state );

		// Destroy voice when stopped
		if( pCbx->GetState() == NodeStateStop )
		{
			itSrc = m_Sources.Erase( itSrc );

			CAkLEngineCmds::DeleteAllCommandsForSource( pCbx );
			VPLDestroySource( pCbx );
		}
		else
		{
			++itSrc;
		}
	}// Voices

	// Bus tree begin.
	for ( int iVPL = m_arrayVPLs.Length() - 1; iVPL >= 0; --iVPL )
	{
		AkVPL * pVPL = m_arrayVPLs[ iVPL ];

		// Push the normal bus buffer to the final mix or their parent mix.
#ifdef AK_MOTION
		if (!pVPL->m_bFeedback)
#endif // AK_MOTION
		{
			TransferBuffer( pVPL );

			// We can release the buffer right away since the final mixer works in incremental mode (+=)
			pVPL->m_MixBus.ReleaseBuffer();
		}
	}// Bus tree end.

	// Final Mix!
	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		(*it).PushData();

#ifdef AK_WIN
	//Support for Merge To Main sinks.	
	g_pAkSink->FinishMix();
#endif

	//Release the mixing buffers after all data has been pushed.  Some platform use the final buffers across many sinks.
	for(AkDeviceArray::Iterator it = CAkOutputMgr::OutputBegin(); it != CAkOutputMgr::OutputEnd(); ++it)
		(*it).pFinalMix->ReleaseBuffer();
}

#endif // ! defined( AK_PS3 )
