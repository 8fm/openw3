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
#include "AkSplitterBus.h"
#include "AkSink.h"
#include "AkVPLMixBusNode.h"
#include "AkFeedbackMgr.h"
#include "AkLEngine.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>

CAkSplitterBus::PlayerSlot::PlayerSlot()
{
	m_pFeedbackMixBus = NULL; 
	m_pAudioMixBus = NULL; 
	m_DeviceID = 0;
}

void CAkSplitterBus::PlayerSlot::Term()
{
	if (m_pFeedbackMixBus != NULL)
	{
#if !defined AK_WII_FAMILY_HW
		m_pFeedbackMixBus->Disconnect();
#endif
		AkDelete(g_LEngineDefaultPoolId, m_pFeedbackMixBus);
		m_pFeedbackMixBus = NULL;
	}

	if (m_pAudioMixBus != NULL)
	{
#if !defined AK_WII_FAMILY_HW
		m_pAudioMixBus->Disconnect();
#endif
		AkDelete(g_LEngineDefaultPoolId, m_pAudioMixBus);
		m_pAudioMixBus = NULL;
	}
}

CAkSplitterBus::CAkSplitterBus()
{
	m_iMaxPlayers = 0;
	m_iMaxDevices = 0;
}

CAkSplitterBus::~CAkSplitterBus()
{	
	Term();
}

AKRESULT CAkSplitterBus::Term()
{	
	for(AkUInt32 i = 0; i < m_aBusses.Length(); i++)
	{
		m_aBusses[i].Term();
	}
	m_aBusses.Term();
	return AK_Success;
}

AKRESULT CAkSplitterBus::AddBus( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID, AkChannelMask in_MixingFormat )
{
	//Check if the player is known
	bool bNeedNewPlayer = in_iPlayerID >= m_iMaxPlayers;

	//Check if we know this device.
	AkUInt32 iDevice = 0;
	for(iDevice = 0; iDevice < m_iMaxDevices && m_aBusses[iDevice].m_DeviceID != in_iDeviceID; iDevice++)
		/* Searching for the index of the device */;

	bool bNeedNewDevice = iDevice == m_iMaxDevices;
	if (bNeedNewPlayer || bNeedNewDevice)
	{
		AkInt32 iOldPlayers = (AkInt32)m_iMaxPlayers;
		AkInt32 iOldDevices = (AkInt32)m_iMaxDevices;

		m_iMaxPlayers = AkMax(m_iMaxPlayers, in_iPlayerID + 1);
		if (bNeedNewDevice)
			m_iMaxDevices++; 

		if (!m_aBusses.Resize(m_iMaxPlayers * m_iMaxDevices))
			return AK_Fail;	//No more memory;

		if (bNeedNewDevice)
		{
			//Copy the data at the right places.  The array is basically a 2D array with the Player
			//number as the Y value and the device as the X.  So the layout looks like this
			// P0D0, P0D1, P1D0, P1D1 .... (where PyDx is player Y-device X)
			//Now that we want to add a device or a player we need to space the actual data differently.
			AkInt32 i = iOldPlayers - 1;
			for(; i >= 0; i--)
			{
				memcpy(&m_aBusses[i * m_iMaxDevices], &m_aBusses[i * iOldDevices], sizeof(PlayerSlot) * iOldDevices);
			}

			//Put the new device ID in the last slot for all players
			for(i = 0; i < m_iMaxPlayers; i++)
			{
				m_aBusses[i*m_iMaxDevices + (m_iMaxDevices-1)].m_DeviceID = in_iDeviceID;
			}
		}

		if (bNeedNewPlayer)
		{
			//Init the new player devices like the ones we know
			AkUInt32 iLast = (m_iMaxPlayers - 1) * m_iMaxDevices;
			for(AkUInt32 iDevice = 0; iDevice < m_iMaxDevices; iDevice++)
				m_aBusses[iLast + iDevice].m_DeviceID = m_aBusses[iDevice].m_DeviceID;
		}
	}

	//Once we make it here, we know there is a slot for this combination of player/device
	PlayerSlot &rSlot = m_aBusses[in_iPlayerID*m_iMaxDevices + iDevice];
	if (rSlot.m_pFeedbackMixBus != NULL)
	{
		return AK_Success;	//Already initialized
	}

	//First initialization
	CAkFeedbackMixBus* pMixBus = NULL;
	AkNew2( pMixBus, g_LEngineDefaultPoolId, CAkFeedbackMixBus, CAkFeedbackMixBus() );
	if (pMixBus == NULL)
		return AK_Fail;

	rSlot.m_MixingFormat = in_MixingFormat;
	AK::CAkBusCtx dummyBusContext;
	AKRESULT akr = pMixBus->Init(in_MixingFormat, in_MixingFormat, AK_FEEDBACK_MAX_FRAMES_PER_BUFFER, dummyBusContext);
	if (akr != AK_Success)
	{
		AkDelete(g_LEngineDefaultPoolId, pMixBus);
		return akr;
	}

#if !defined AK_WII_FAMILY_HW
	pMixBus->Connect();
#endif

	rSlot.m_pFeedbackMixBus = pMixBus;

	return AK_Success;
}

AKRESULT CAkSplitterBus::RemoveBus( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID )
{
	//Here we don't remove anything from the array.  We just uninitialize.
	if (in_iPlayerID >= m_iMaxPlayers)
		return AK_Fail;

	//Check if we know this device.
	AkUInt32 iDevice = 0;
	for(iDevice = 0; iDevice < m_iMaxDevices && m_aBusses[iDevice].m_DeviceID != in_iDeviceID; iDevice++)
		/* Searching for the index of the device */;

	if (iDevice == m_iMaxDevices)
		return AK_Fail;
	
	m_aBusses[in_iPlayerID*m_iMaxDevices + iDevice].Term();

	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////////
// MixFeedbackBuffer: Adds a feedback source to the mix
// Params:
///////////////////////////////////////////////////////////////////////////////
void CAkSplitterBus::MixFeedbackBuffer( AkRunningVPL & io_runningVPL, AkUInt32 in_uPlayers )
{
	AkUInt32 iSlot = 0;
	CAkPBI *pPBI = io_runningVPL.pCbx->GetContext();
	AkFeedbackParams *pParams = pPBI->GetFeedbackParameters();
	bool b3DGameDef = pPBI->GetPositionSourceType() == AkGameDef && pPBI->GetPannerType() == Ak3D;

	AkVPLState state;
	state = io_runningVPL.state;

	AkUInt32 uPlayerVolSize = AK::GetNumChannels(pParams->m_uChannelMask);
	AkAudioMix *aMixes;
#ifdef AK_PS3
	aMixes = io_runningVPL.pMotionMixes;
#else
	aMixes = (AkAudioMix *)AkAlloca(m_iMaxPlayers * uPlayerVolSize*sizeof(AkAudioMix));
#endif

	//If not in 3D, assume player 0 and compute the volumes only for one player
	if (!b3DGameDef)
		pParams->CopyVolumes(0, aMixes);

	AkUInt32 keyDevice = MAKE_DEVICE_KEY(pParams->m_usCompanyID, pParams->m_usPluginID);
	for(AkUInt8 iPlayer = 0; iPlayer < m_iMaxPlayers; iPlayer++)
	{
		//Check if this player feels this source.
		if ((in_uPlayers & (1 << iPlayer)) == 0)
		{
			iSlot += m_iMaxDevices;
			continue;
		}

		//Change the volumes based on the target player (if 3D) and apply the master player volume.  
		if (b3DGameDef)
			pParams->CopyVolumes(iPlayer, aMixes + iPlayer*uPlayerVolSize);

		for(AkUInt32 iDevice = 0; iDevice < m_iMaxDevices; iDevice++)
		{
			PlayerSlot& rSlot = m_aBusses[iPlayer * m_iMaxDevices + iDevice];

			if (rSlot.m_DeviceID == keyDevice)
			{
				if (rSlot.m_pFeedbackMixBus != NULL)
				{
					AkUInt32 iMixIndex = !b3DGameDef ? 0 : iPlayer;
					rSlot.m_pFeedbackMixBus->ConsumeBuffer(state, aMixes + iMixIndex*uPlayerVolSize);
				}
				break;	//Found it.
			}
		}
	}

	pParams->StampOldVolumes();
}

///////////////////////////////////////////////////////////////////////////////
// MixAudioBuffer: Adds an audio source to the audio mix to be converted to
//				   vibrations.
// Params:
///////////////////////////////////////////////////////////////////////////////
void CAkSplitterBus::MixAudioBuffer( AkRunningVPL & io_runningVPL, AkUInt32 in_uPlayers )
{
	CAkPBI *pPBI = io_runningVPL.pCbx->GetContext();
	AkFeedbackParams *pParams = pPBI->GetFeedbackParameters();

	AkUInt32 uPlayerVolSize = AK::GetNumChannels(pParams->m_uChannelMask);
	AkAudioMix *aMixes;
#ifdef AK_PS3
	aMixes = io_runningVPL.pMotionMixes;
#else
	aMixes = (AkAudioMix *)AkAlloca(m_iMaxPlayers * uPlayerVolSize*sizeof(AkAudioMix));
#endif

#if !defined AK_WII_FAMILY_HW

	AkVPLState *pState;
	bool b3DGameDef = pPBI->GetPositionSourceType() == AkGameDef && pPBI->GetPannerType() == Ak3D;

#ifndef AK_PS3
	AkVPLState stateTemp;

	//If we processed the LPF, use that audio buffer.
	if (io_runningVPL.pFeedbackData->LPFBuffer.HasData())
		stateTemp.AttachContiguousDeinterleavedData(io_runningVPL.pFeedbackData->LPFBuffer.GetContiguousDeinterleavedData(), 
															io_runningVPL.pFeedbackData->LPFBuffer.MaxFrames(),
															io_runningVPL.pFeedbackData->LPFBuffer.uValidFrames,
															pParams->m_uChannelMask);
	else
		stateTemp.AttachContiguousDeinterleavedData( io_runningVPL.state.GetContiguousDeinterleavedData(), 
															io_runningVPL.state.MaxFrames(),
															io_runningVPL.state.uValidFrames,
															pParams->m_uChannelMask);
	pState = &stateTemp;
	//For PS3, the buffer was already copied in PrepareAudioProcessing.

	//Only "3D game defined" has different volumes for each player. 
	//If not in 3D, assume player 0 and compute them only once.
	if (!b3DGameDef)
		pParams->CopyVolumes(0, aMixes);
#endif

	AkUInt32 iSlot = 0;
	for(AkUInt8 iPlayer = 0; iPlayer < m_iMaxPlayers; iPlayer++)
	{
		if ((in_uPlayers & (1 << iPlayer)) == 0)
		{
			iSlot += m_iMaxDevices;	//Skip this player entirely
			continue;
		}

#ifdef AK_PS3
		//On the PS3, each player has its own buffer for mixing.
		pState = &io_runningVPL.pFeedbackData->pStates[iPlayer];
#else
		//Change the volumes based on the target player (if 3D) and apply the master player volume.  (Except on PS3 since we need a set of volumes for each).
		if (b3DGameDef)
#endif
			pParams->CopyVolumes(iPlayer, aMixes + iPlayer*uPlayerVolSize);

		for(AkUInt32 iDevice = 0; iDevice < m_iMaxDevices; iDevice++)
		{
			PlayerSlot& rSlot = m_aBusses[iSlot];

			//Check if the audio mix node exist, if not create only if this slot is active.
			if (rSlot.m_pFeedbackMixBus != NULL)
			{
				if(rSlot.m_pAudioMixBus == NULL)
				{
					AkNew2( rSlot.m_pAudioMixBus, g_LEngineDefaultPoolId, CAkAudioToFeedbackMixBus, CAkAudioToFeedbackMixBus() );
					if (rSlot.m_pAudioMixBus == NULL)
						continue ;		//Don't care about this failure.

					CAkBusCtx dummyBusContext;
					if(rSlot.m_pAudioMixBus->Init(rSlot.m_MixingFormat, rSlot.m_MixingFormat, LE_MAX_FRAMES_PER_BUFFER, dummyBusContext) != AK_Success)
					{
						AkDelete(g_LEngineDefaultPoolId, rSlot.m_pAudioMixBus);
						rSlot.m_pAudioMixBus = NULL;
						continue;
					}
					rSlot.m_pAudioMixBus->Connect();
				}
				AkUInt32 iMixIndex = !b3DGameDef ? 0 : iPlayer;
				rSlot.m_pAudioMixBus->ConsumeBuffer(*pState, aMixes + iMixIndex*uPlayerVolSize
#ifdef AK_PS3
					, -1, NULL
#endif
					);
			}
			iSlot++;
		}
	}
#endif

	pParams->StampOldVolumes();
}


void CAkSplitterBus::GetBuffer( AkUInt8 in_iPlayerID, AkUInt32 in_iDeviceID, AkAudioBufferBus *& out_pAudioBuffer, AkAudioBufferBus *& out_pFeedbackBuffer )
{
	out_pAudioBuffer = NULL;
	out_pFeedbackBuffer = NULL;

	if (in_iPlayerID >= m_iMaxPlayers)
		return;

	//Check if we know this device.
	AkUInt32 iDevice = 0;
	for(iDevice = 0; iDevice < m_iMaxDevices && m_aBusses[iDevice].m_DeviceID != in_iDeviceID; iDevice++)
		/* Searching for the index of the device */;

	if (iDevice == m_iMaxDevices)
		return;

	PlayerSlot & rSlot = m_aBusses[in_iPlayerID*m_iMaxDevices + iDevice];
	if (rSlot.m_pAudioMixBus != NULL)
		rSlot.m_pAudioMixBus->GetResultingBuffer(out_pAudioBuffer);

	if (rSlot.m_pFeedbackMixBus != NULL)
		rSlot.m_pFeedbackMixBus->GetResultingBuffer(out_pFeedbackBuffer);
}

void CAkSplitterBus::ReleaseBuffers()
{
	for(AkUInt32 i = 0; i < m_aBusses.Length(); ++i)
	{
		PlayerSlot& rSlot = m_aBusses[i];

		if (rSlot.m_pAudioMixBus != NULL)
			rSlot.m_pAudioMixBus->ReleaseBuffer();

		if (rSlot.m_pFeedbackMixBus != NULL)
			rSlot.m_pFeedbackMixBus->ReleaseBuffer();
	}
}

