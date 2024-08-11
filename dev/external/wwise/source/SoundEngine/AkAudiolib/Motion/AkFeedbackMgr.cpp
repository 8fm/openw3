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
#include "AkFeedbackMgr.h"
#include "AkEffectsMgr.h"
#include "AkLEngine.h"
#include "AkFeedbackBus.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include "AkMonitor.h"
#include "AkAudioMgr.h"
#include "AkFXMemAlloc.h"
#include "Ak3DListener.h"
#include "AkMonitorData.h"
#include "AkURenderer.h"
#if !defined AK_WII_FAMILY_HW
#include "AkSrcLpFilter.h"
#endif


#define MINIMUM_STARVATION_NOTIFICATION_DELAY 8 //in buffers
CAkFeedbackDeviceMgr* CAkFeedbackDeviceMgr::s_pSingleton = NULL;
extern AkPlatformInitSettings g_PDSettings;
extern CAkAudioMgr* g_pAudioMgr;

CAkFeedbackDeviceMgr::DeviceBus::DeviceBus()
{
	m_pFinalBus = NULL;
}

CAkFeedbackDeviceMgr::DeviceBus::~DeviceBus()
{
}

CAkFeedbackDeviceMgr::CAkFeedbackDeviceMgr()
{
	m_uLastStarvationTime = 0;
	m_uPlayerMask = 0;

	//Initialize the listener mapping for each player.

	for(AkInt8 i = 0; i != AK_MAX_DEVICES; i++)
		m_aPlayerToListener[i] = 0;		//By default all players use listener 0.

	for(AkInt8 i = 0; i != AK_NUM_LISTENERS; i++)
		m_aListenerToPlayer[i] = 0;		

	m_aListenerToPlayer[0] = (1 << AK_MAX_DEVICES) - 1;	//By default all players use listener 0.
	m_uLastPlayerIndex = 0;
}

CAkFeedbackDeviceMgr::~CAkFeedbackDeviceMgr()
{
	AkUInt32 i = 0;
	for(i = 0; i < m_aBusses.Length(); i++)
	{
		AkDelete(g_LEngineDefaultPoolId, m_aBusses[i].item);
	}

	for(i = 0; i != AK_MAX_DEVICES; i++)
	{
		tDeviceMap& rDeviceMap = m_aPlayers[i];
		for(AkUInt32 j = 0; j < rDeviceMap.Length(); j++)
		{
			if (rDeviceMap[j].item.m_pFinalBus != NULL)
			{
				AKVERIFY(rDeviceMap[j].item.m_pFinalBus->Term(AkFXMemAlloc::GetLower()));	
				rDeviceMap[j].item.m_pFinalBus = NULL;
			}
		}
		rDeviceMap.Term();
	}

	m_aBusses.Term();
}

CAkFeedbackDeviceMgr* CAkFeedbackDeviceMgr::Create()
{
	if (s_pSingleton == NULL)
	{
		AkNew2( s_pSingleton, g_LEngineDefaultPoolId, CAkFeedbackDeviceMgr, CAkFeedbackDeviceMgr() );
	}

	return s_pSingleton;
}

void CAkFeedbackDeviceMgr::Destroy()
{
	AkDelete(g_LEngineDefaultPoolId, s_pSingleton);
	s_pSingleton = NULL;
}

AKRESULT CAkFeedbackDeviceMgr::AddPlayerFeedbackDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceCompanyID, AkUInt32 in_iDevicePluginID, void * in_pDevice)
{
	//Enable the pipeline.  It doesn't matter if this is done multiple times.
	CAkLEngine::EnableFeedbackPipeline();

	AkUInt32 keyDevice = MAKE_DEVICE_KEY(in_iDeviceCompanyID, in_iDevicePluginID);

	AKASSERT(in_iPlayerID < AK_MAX_DEVICES);
	//Get the devices currently used by this player
	tDeviceMap& rDeviceMap = m_aPlayers[in_iPlayerID];
	DeviceBus* pDevice = rDeviceMap.Set(keyDevice);
	if (pDevice == NULL)
		return AK_Fail;		//No memory 

	if (pDevice->m_pFinalBus != NULL)
		return AK_Success;	//Already exists!

	//Create the final mix bus for this device
	if(CAkEffectsMgr::AllocFeedbackBus(in_iDeviceCompanyID, in_iDevicePluginID, &g_PDSettings, in_iPlayerID, pDevice->m_pFinalBus, in_pDevice) != AK_Success)
	{
		rDeviceMap.Unset(keyDevice);
		return AK_Fail;		//No memory 
	}
	
	//Pass the information to the splitters so they create a new bus for this
	AKRESULT akr = AK_Success;
	tBusArray::Iterator it = m_aBusses.Begin();
	for(; it != m_aBusses.End(); ++it)
	{
		akr = (*it).item->AddBus(in_iPlayerID, keyDevice, pDevice->m_pFinalBus->GetMixingFormat());
		if (akr != AK_Success)
			break;
	}

	if (akr != AK_Success)
	{
		MONITOR_ERRORMSG2( AKTEXT("Error! Initialization of one feedback device failed."), AKTEXT("") );

		RemovePlayerFeedbackDevice(in_iPlayerID, in_iDeviceCompanyID, in_iDevicePluginID);
	}

	UpdatePlayerCount();

	return akr;
}

void CAkFeedbackDeviceMgr::RemovePlayerFeedbackDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceCompanyID, AkUInt32 in_iDevicePluginID)
{
	AkUInt32 keyDevice = MAKE_DEVICE_KEY(in_iDeviceCompanyID, in_iDevicePluginID);

	AKASSERT(in_iPlayerID < AK_MAX_DEVICES);

	//Get the devices currently used by this player
	tDeviceMap& rDeviceMap = m_aPlayers[in_iPlayerID];
	DeviceBus* pDevice = rDeviceMap.Exists(keyDevice);
	if (pDevice == NULL)
		return;		//Was not connected to this device

	//Pass the information to the splitters so they remove busses related to this player/device
	tBusArray::Iterator it = m_aBusses.Begin();
	for(; it != m_aBusses.End(); ++it)
	{
		(*it).item->RemoveBus(in_iPlayerID, keyDevice);
	}

	//Destroy the final bus
	if (pDevice->m_pFinalBus != NULL)
	{
		pDevice->m_pFinalBus->Stop();
		AKVERIFY(pDevice->m_pFinalBus->Term(AkFXMemAlloc::GetLower()));	
		pDevice->m_pFinalBus = NULL;
	}

	rDeviceMap.Unset(keyDevice);

	UpdatePlayerCount();
}

bool CAkFeedbackDeviceMgr::PrepareAudioProcessing(AkRunningVPL & io_runningVPL)
{
#if !defined AK_WII_FAMILY_HW

	//This should be called only for Audio-To-Motion VPLs
	if (io_runningVPL.bFeedbackVPL || io_runningVPL.state.uValidFrames == 0)
		return false;

	//Check if this VPL should be sent to a feedback bus, and which one.
	CAkPBI* pPBI = io_runningVPL.pCbx->GetContext();

	AkFeedbackParams *pParams = pPBI->GetFeedbackParameters();
	if (pParams == NULL || m_uLastPlayerIndex == 0)
		return false;	//Not routed to a motion bus.

	//Check if we need to allocate the FeedbackData in the VPL
	if (GetActivePlayers(pPBI) == 0)
		return false;	//Nobody will get this motion so don't bother.

	void *pSamples = io_runningVPL.state.GetContiguousDeinterleavedData();
	AkUInt16 uFrames = io_runningVPL.state.MaxFrames();

	AkFeedbackVPLData *pData;
#ifdef AK_PS3
	//We will need this data.  Allocate states.
	pData = (AkFeedbackVPLData *)AkAlloc(g_LEngineDefaultPoolId, sizeof(AkFeedbackVPLData) + (m_uLastPlayerIndex - 1) * sizeof(AkVPLState));
#else
	pData = (AkFeedbackVPLData *)AkAlloc(g_LEngineDefaultPoolId, sizeof(AkFeedbackVPLData));
#endif
	if (pData == NULL)
		return false;	//Don't care about memory problems.  We won't process this voice.

	//If we have to process the LPF, allocate the buffer and copy the data.  LPF is processed in-place, unfortunately.
	pData->LPFBuffer.ClearData();
	if (pParams->m_LPF > BYPASSMAXVAL)
	{
		AKRESULT result = pData->LPFBuffer.GetCachedBuffer( uFrames, pParams->m_uChannelMask );
		if ( result == AK_Success )
		{
			memcpy(	pData->LPFBuffer.GetContiguousDeinterleavedData(),
					pSamples, uFrames * sizeof(AkSampleType) * pParams->m_uChannels );

			pData->LPFBuffer.uValidFrames = io_runningVPL.state.uValidFrames;
			pSamples = pData->LPFBuffer.GetContiguousDeinterleavedData();
		}
	}

#ifdef AK_PS3
	//Initialize each of the player states.
	pData->iPlayerCount = m_uLastPlayerIndex;
	for(AkUInt8 i = 0; i < m_uLastPlayerIndex; i++)
	{
		pData->pStates[i].result = AK_DataNeeded;
		pData->pStates[i].Clear();
		pData->pStates[i].AttachContiguousDeinterleavedData(pSamples, uFrames, io_runningVPL.state.uValidFrames, pParams->m_uChannelMask);
		//AudioMix will be initialized later.
	}
#endif

	io_runningVPL.pFeedbackData = pData;
#endif
	return true;
}

void CAkFeedbackDeviceMgr::CleanupAudioVPL(AkRunningVPL & io_runningVPL)
{
#if !defined AK_WII_FAMILY_HW
	if (io_runningVPL.pFeedbackData->LPFBuffer.HasData())
		io_runningVPL.pFeedbackData->LPFBuffer.ReleaseCachedBuffer();

	AkFree(g_LEngineDefaultPoolId, io_runningVPL.pFeedbackData);
	io_runningVPL.pFeedbackData = NULL;

#ifdef AK_PS3
	io_runningVPL.state.result = io_runningVPL.state.resultPrevious;
#endif
#endif
}

void CAkFeedbackDeviceMgr::ApplyMotionLPF(AkRunningVPL & io_runningVPL)
{
#if !defined AK_WII_FAMILY_HW
	CAkPBI* pPBI = io_runningVPL.pCbx->GetContext();
	AkFeedbackParams *pParams = pPBI->GetFeedbackParameters();

	// Current implementation of LPF writes and reads in same buffer. LPF must be applied only for feedback source.
	if (pParams->m_LPF > BYPASSMAXVAL && io_runningVPL.pFeedbackData->LPFBuffer.HasData())
	{
		if (pParams->m_pLPFilter != NULL ||
			pParams->CreateLowPassFilter())
		{
			// Prepare the low pass filter
			pParams->m_pLPFilter->SetLPFPar(pParams->m_LPF);

			//Apply current filter
#ifdef AK_PS3
			pParams->m_pLPFilter->ExecutePS3( &io_runningVPL.pFeedbackData->LPFBuffer, io_runningVPL.state.result );
#else
			pParams->m_pLPFilter->Execute( &io_runningVPL.pFeedbackData->LPFBuffer );
#endif
		}
	}
#endif
}

void CAkFeedbackDeviceMgr::ConsumeVPL( AkRunningVPL & io_runningVPL )
{
	//Check if this VPL should be sent to a feedback bus, and which one.
	CAkPBI* pPBI = io_runningVPL.pCbx->GetContext();
	
	AkFeedbackParams *pParams = pPBI->GetFeedbackParameters();
	if (pParams == NULL || m_uLastPlayerIndex == 0)
		return;	//Nothing to do!

	CAkSplitterBus* pSplitter = GetSplitter(pParams->m_pOutput->ID());
	if (!pSplitter)
		return;

	AkUInt32 uPlayerMask = GetActivePlayers(pPBI);

#ifdef AK_PS3
	if (io_runningVPL.pMotionMixes == NULL)
		io_runningVPL.pMotionMixes = (AkAudioMix*)AkMalign(g_LEngineDefaultPoolId, AK_ALIGN_SIZE_FOR_DMA(m_uLastPlayerIndex * AK::GetNumChannels(pParams->m_uChannelMask)*sizeof(AkAudioMix)), AK_SIMD_ALIGNMENT);
	if (io_runningVPL.pMotionMixes == NULL)
		return;	//Can't process anyway.
#endif

	if (io_runningVPL.bFeedbackVPL)
		pSplitter->MixFeedbackBuffer(io_runningVPL, uPlayerMask);
#if !defined AK_WII_FAMILY_HW
	else if(io_runningVPL.pFeedbackData != NULL)
		pSplitter->MixAudioBuffer(io_runningVPL, uPlayerMask);
#endif
}

CAkSplitterBus* CAkFeedbackDeviceMgr::GetSplitter(AkUInt32 in_idBus)
{
	CAkSplitterBus* pSplitter = NULL;
	CAkSplitterBus** ppSplitter = m_aBusses.Exists(in_idBus);
	if (ppSplitter == NULL)
	{
		//First time we see this mixing bus.  Create the splitter.
		AkNew2( pSplitter, g_LEngineDefaultPoolId, CAkSplitterBus, CAkSplitterBus() );
		if (pSplitter == NULL)
			return NULL;	//Out of memory;

		if (m_aBusses.Set(in_idBus, pSplitter) == NULL)
		{
			//Out of memory.
			AkDelete(g_LEngineDefaultPoolId, pSplitter);
			return NULL;
		}
	}
	else
		pSplitter = *ppSplitter;

	//Now populate the splitter with all the possibilities
	for(AkUInt8 i = 0; i < m_uLastPlayerIndex; i++)
	{
		tDeviceMap& rDevices = m_aPlayers[i];
		tDeviceMap::Iterator itDevice = rDevices.Begin();
		for(; itDevice != rDevices.End(); ++itDevice)
		{
			pSplitter->AddBus(i, itDevice.pItem->key, itDevice.pItem->item.m_pFinalBus->GetMixingFormat());
		}
	}

	return pSplitter;
}

void CAkFeedbackDeviceMgr::RenderData()
{
	AkAudioBufferBus *pAudioBuffer;
	AkAudioBufferBus *pFeedbackBuffer;
	for(AkUInt8 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
			{
				for(AkUInt32 iBus = 0; iBus < m_aBusses.Length(); ++iBus)
				{
					m_aBusses[iBus].item->GetBuffer(iPlayer, rDevices[iDevice].key, pAudioBuffer, pFeedbackBuffer);
					if (pAudioBuffer != NULL && pAudioBuffer->uValidFrames > 0)
						rDevice.m_pFinalBus->MixAudioBuffer(*pAudioBuffer);

					if (pFeedbackBuffer != NULL && pFeedbackBuffer->uValidFrames > 0)
						rDevice.m_pFinalBus->MixFeedbackBuffer(*pFeedbackBuffer);
				}

				rDevice.m_pFinalBus->RenderData();
			}
		}
	}

	for(AkUInt32 iBus = 0; iBus < m_aBusses.Length(); ++iBus)
	{
		m_aBusses[iBus].item->ReleaseBuffers();
	}
}

AkReal32 CAkFeedbackDeviceMgr::GetPeak()
{
	AkReal32 fMax = 0;
	for(AkUInt32 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
			{
				AkReal32 fCurrent = rDevice.m_pFinalBus->GetPeak();
				fMax = AkMax(fMax, fCurrent);
			}
		}
	}
	return fMax;
}

void CAkFeedbackDeviceMgr::HandleStarvation()
{
	bool bStarving = false;
	for(AkUInt32 iPlayer = 0; iPlayer < m_uLastPlayerIndex && !bStarving; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length() && !bStarving; ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
				bStarving = rDevice.m_pFinalBus->IsStarving();
		}
	}

	if (bStarving)
	{
		AkUInt32 uTimeNow = g_pAudioMgr->GetBufferTick();
		if( m_uLastStarvationTime == 0 ||
			uTimeNow - m_uLastStarvationTime > MINIMUM_STARVATION_NOTIFICATION_DELAY )
		{
			MONITOR_ERROR( AK::Monitor::ErrorCode_FeedbackVoiceStarving );
			m_uLastStarvationTime = uTimeNow;
		}
	}
}

//This function drives command driven devices like rumble controllers.  
//It is provided to increase the control rate above the audio buffer rate (called ~93 times per second on PC/XBox/PS3, ~83 times on the Wii)
void CAkFeedbackDeviceMgr::CommandTick()
{
	for(AkUInt32 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
				rDevice.m_pFinalBus->CommandTick();
		}
	}
}

bool CAkFeedbackDeviceMgr::IsFeedbackEnabled()
{
	//The feedback is enabled if we have at least one player and one device which is enabled.  
	//NOTE: later when SetPlayerMotionVolume is implemented we will also need to check the player volume too.
	for(AkUInt8 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		if (m_aPlayers[iPlayer].Length() > 0)
			return true;
	}
	return false;
}

bool CAkFeedbackDeviceMgr::IsDeviceActive(AkUInt32 in_iDeviceCompanyID, AkUInt32 in_iDevicePluginID)
{
	AkUInt32 keyDevice = MAKE_DEVICE_KEY(in_iDeviceCompanyID, in_iDevicePluginID);
	for(AkUInt32 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		DeviceBus* pBus = m_aPlayers[iPlayer].Exists(keyDevice);
		if (pBus != NULL && pBus->m_pFinalBus->IsActive())
			return true;
	}

	return false;
}

void CAkFeedbackDeviceMgr::UpdatePlayerCount()
{
	//The player count is simply the highest active player.  So if there is only
	//one game controller connected but it is in the console port #2, there will be 2 players.
	m_uLastPlayerIndex = 0;
	m_uPlayerMask = 0;
	for(AkUInt8 i = 0; i < AK_MAX_DEVICES; i++)
	{
		if (m_aPlayers[i].Length() > 0)
		{
			m_uLastPlayerIndex = i + 1;	//We want to have the highest index.  Not really the actual count.
			m_uPlayerMask |= (1 << i);
		}
	}

	//Also invalidate all PBIs with motion.  The AkFeedbackStruct needs to be reallocated.
	CAkURenderer::InvalidateAllMotionPBIs();
}

#ifndef AK_OPTIMIZED
void CAkFeedbackDeviceMgr::PostDeviceMonitorData()
{
	//Find the number of registered devices
	AkArray<AkUInt32, AkUInt32, ArrayPoolLEngineDefault> aDevices;
	for(AkUInt8 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			if (aDevices.FindEx(rDevices[iDevice].key) == aDevices.End())
				aDevices.AddLast(rDevices[iDevice].key);
		}
	}

	if (aDevices.Length() == 0)
		return;

	AkUInt32 sizeofItem = offsetof( AkMonitorData::MonitorDataItem, feedbackDevicesData ) + sizeof(AkMonitorData::FeedbackDevicesMonitorData) 
		+ (aDevices.Length() - 1) * sizeof(AkMonitorData::FeedbackDeviceIDMonitorData);	//Minus 1 because there is already one in the struct.

	//Fill the allocated struct
	AkProfileDataCreator creator( sizeofItem );
	if ( creator.m_pData )
	{
		creator.m_pData->eDataType = AkMonitorData::MonitorDataFeedbackDevices;
		creator.m_pData->feedbackDevicesData.usDeviceCount = (AkUInt16)aDevices.Length();

		for(AkUInt32 iDevice = 0; iDevice < aDevices.Length(); ++iDevice)
		{
			AkUInt32 keyDevice = aDevices[iDevice];
			AkMonitorData::FeedbackDeviceIDMonitorData &rData = creator.m_pData->feedbackDevicesData.deviceIDs[iDevice];
			rData.usCompanyID = (AkUInt16)(keyDevice & 0xF);
			rData.usDeviceID = (AkUInt16)(keyDevice >> 16);
			rData.ucPlayerActive = 0;
		}
		
		for(AkUInt8 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
		{
			tDeviceMap &rDevices = m_aPlayers[iPlayer];
			for(AkUInt32 iDevice = 0; iDevice < aDevices.Length(); ++iDevice)
			{
				AkMonitorData::FeedbackDeviceIDMonitorData &rData = creator.m_pData->feedbackDevicesData.deviceIDs[iDevice];
				DeviceBus *pBus = rDevices.Exists(aDevices[iDevice]);
				if (pBus != NULL && pBus->m_pFinalBus != NULL && pBus->m_pFinalBus->IsActive())
					rData.ucPlayerActive = rData.ucPlayerActive | (1 << iPlayer);
			}	
		}
	}

	aDevices.Term();
}

#endif

void CAkFeedbackDeviceMgr::Stop()
{
	for(AkUInt32 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
				rDevice.m_pFinalBus->Stop();
		}
	}
}

AkUInt32 CAkFeedbackDeviceMgr::GetActivePlayers( CAkPBI* pPBI )
{
	//Check which players are going to receive this source.  The uPlayerMask bitfield will contain a bit
	//for each player that will receive the source (0 to 3).  To compute this mask we need to check which listener
	//"hears" the source from the given game object and which listener is assigned to the player.
	//To set the feedback mask, the game programmer must use the SetListenerPipeline function.
	AkUInt32 uPlayerMask = 0;
	AkUInt32 uListenerMask = pPBI->GetGameObjectPtr()->GetListenerMask() & CAkListener::GetFeedbackMask();
	for(AkUInt8 i = 0; i < m_uLastPlayerIndex; i++)
	{
		if ((1 << m_aPlayerToListener[i]) & uListenerMask)
			uPlayerMask |= 1 << i;
	}

	return uPlayerMask;
}

void CAkFeedbackDeviceMgr::StartOutputCapture( const AkOSChar* in_CaptureFileName )
{
	for(AkUInt8 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
				rDevice.m_pFinalBus->StartOutputCapture(in_CaptureFileName);
		}
	}
}

void CAkFeedbackDeviceMgr::StopOutputCapture()
{
	for(AkUInt8 iPlayer = 0; iPlayer < m_uLastPlayerIndex; ++iPlayer)
	{
		tDeviceMap &rDevices = m_aPlayers[iPlayer];
		for(AkUInt32 iDevice = 0; iDevice < rDevices.Length(); ++iDevice)
		{
			DeviceBus &rDevice = rDevices[iDevice].item;
			if (rDevice.m_pFinalBus != NULL)
				rDevice.m_pFinalBus->StopOutputCapture();
		}
	}
}