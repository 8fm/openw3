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
#include "AkPBI.h"
#include "AkFeedbackStructs.h"
#if !defined AK_WII
#include "../SoftwarePipeline/AkSrcLpFilter.h"
#endif

#include "AkFeedbackMgr.h"


extern AkReal32 g_fVolumeThreshold;

AkFeedbackParams* AkFeedbackParams::Create(AkUInt16 in_iPlayers, AkChannelMask in_uChannelMask, AkPannerType in_ePanType, AkPositionSourceType in_ePosSource)
{
	//Only Ak3DGameDef needs to have one volume set per player.  All others need only one.
	if (!(in_ePanType == Ak3D && in_ePosSource == AkGameDef))
		in_iPlayers = 1;

	//- Minus one because there is already one in the structure size.
	AkUInt32 uChannels = AK::GetNumChannels(in_uChannelMask & ~AK_SPEAKER_LOW_FREQUENCY);
	if (uChannels == 0)
		return NULL;	//0.1 are not played at all.  Don't waste time on it.

	AkUInt32 uVolumeCount = in_iPlayers * uChannels - 1;

	//Also, the member m_Volumes must be aligned to 16 bytes boundaries.	
	AkUInt32 uSize = sizeof(AkFeedbackParams) + uVolumeCount * sizeof(AkAudioMix);
	AkFeedbackParams *pNew = (AkFeedbackParams*)AkMalign( g_DefaultPoolId, uSize, 16 );
	if (pNew == NULL)
		return NULL;
		
	AkPlacementNew(pNew) AkFeedbackParams(in_iPlayers, in_uChannelMask);

	return pNew;
}

void AkFeedbackParams::Destroy()
{
	this->~AkFeedbackParams();
 	AkFalign(g_DefaultPoolId, this);
}

AkFeedbackParams::AkFeedbackParams(AkUInt16 in_iPlayers, AkChannelMask in_uChannelMask)
{
	m_pOutput = NULL;
	m_NewVolume = 0;
	m_AudioBusVolume = 0;
	m_LPF = 0;
	m_MotionBusPitch = 0;
#if !defined AK_WII
	m_pLPFilter = NULL;
#endif	
	m_usCompanyID = 0;
	m_usPluginID = 0;
	for(AkUInt8 i = 0; i < AK_MAX_DEVICES; i++)
		m_fNextAttenuation[i] = 1.0f;
	m_uPlayerCount = in_iPlayers;

	//We don't want to process the LFE.
	m_uChannelMask = (AkUInt16)(in_uChannelMask & ~AK_SPEAKER_LOW_FREQUENCY);
	m_uChannels = AK::GetNumChannels(m_uChannelMask);
	m_bFirst = true;
}

AkFeedbackParams::~AkFeedbackParams()
{
	TermFilter();
}

bool AkFeedbackParams::CreateLowPassFilter()
{
	bool bSuccess = true;
#if !defined AK_WII
	m_pLPFilter = AkNewAligned(g_DefaultPoolId, CAkSrcLpFilter, AK_SIMD_ALIGNMENT);

	if( !m_pLPFilter || m_pLPFilter->Init( m_uChannelMask, true ) != AK_Success)
	{
		bSuccess = false;
		TermFilter();
	}

#endif	
	return bSuccess;
}


void AkFeedbackParams::StampOldVolumes()
{
	//Take the new volumes and put them in the old volumes.
	AkUInt16 uTableSize = m_uChannels * m_uPlayerCount;
	for(AkUInt16 i = 0; i < uTableSize; i++)
		m_Volumes[i].Previous = m_Volumes[i].Next;
}

void AkFeedbackParams::ZeroNewVolumes()
{
	AkUInt16 uTableSize = m_uChannels * m_uPlayerCount;
	for(AkUInt16 i = 0; i < uTableSize; i++)
		m_Volumes[i].Next.Zero();
}

void AkFeedbackParams::ZeroNewAttenuations()
{
	for ( AkUInt8 iPlayer = 0; iPlayer < AK_MAX_DEVICES; ++iPlayer )
	{
		m_fNextAttenuation[iPlayer] = 0;
	}
}

#ifndef AK_OPTIMIZED
void AkFeedbackParams::MonitoringMute()
{
	AkUInt16 uTableSize = m_uChannels * m_uPlayerCount;
	for(AkUInt16 i = 0; i < uTableSize; i++)
		m_Volumes[i].Next.Zero();

	StampOldVolumes();
}
#endif

static AkReal32 dBToLinWithPositive(AkReal32 in_fVal)
{
	AkReal32 fLinVal = 0;
	if (in_fVal > 0)
	{
		fLinVal = AkMath::dBToLin(-in_fVal);
		if (fLinVal > 0)
			fLinVal = 1/fLinVal;
		else
			fLinVal = 65535;	//Infinite
	}
	else
		fLinVal = AkMath::dBToLin(in_fVal);

	return fLinVal;
}

void AkFeedbackParams::ComputePlayerVolumes(CAkPBI* in_pPBI)
{
	AKASSERT(in_pPBI->GetFeedbackParameters() == this);
	m_bSilent = true;

	//When entering this function m_Volumes contain the POSITIONING volumes ONLY.  At the
	//end of this function,  m_Volume will contain the compounded volume (pos+attenuation+hierarchy).

	AkUInt32 uPlayerMask = CAkFeedbackDeviceMgr::Get()->GetActivePlayers(in_pPBI);
	for(AkUInt16 iPlayer = 0; iPlayer < AK_MAX_DEVICES; iPlayer++)
	{
		if (!((1 << iPlayer) & uPlayerMask))
			continue;

		//Compute feedback offset 
		AkReal32 fMotionVol = m_NewVolume;	//DB
		AkUInt32 iIndex = 0;

		//Reconstruct the volumes from each constituents.  
		//We need to remove the contribution of the AudioBus (if any).
		AkReal32 fNewVol = dBToLinWithPositive( fMotionVol - m_AudioBusVolume ) * m_fNextAttenuation[iPlayer];
		fNewVol *= in_pPBI->ComputeCollapsedVoiceVolume();
		if ( in_pPBI->GetPannerType() == Ak3D )
		{
			//In 3D, each player may be in a different position.  Therefore, the volumes
			//from the positioning computations will be different.  The ones received in the AudioMix
			//array are the ones computed as if the audio was the output (taking the max value).
	
			AKASSERT(iPlayer < m_uPlayerCount);
			iIndex = VolumeIndex(iPlayer, 0);
		}

		//Build a unity matrix for the volumes.
		AkUInt32 uSize = m_uChannels * sizeof(AkAudioMix);
		AkAudioMix *pMix = &m_Volumes[iIndex];
		memset(pMix, 0, uSize);
		pMix[0].Next.volumes.fFrontLeft = fNewVol;
		m_bSilent &= pMix[0].Next.IsLessOrEqual(&g_fVolumeThreshold);

#if !defined AK_WII_FAMILY && !defined AK_3DS
		if (m_uChannels > 1)
		{
			pMix[1].Next.volumes.fFrontRight = fNewVol;
			m_bSilent &= pMix[1].Next.IsLessOrEqual(&g_fVolumeThreshold);
		}
#ifdef AK_XBOXONE
		if (m_uChannels > 3)
		{
			pMix[2].Next.volumes.fRearLeft = fNewVol;
			m_bSilent &= pMix[2].Next.IsLessOrEqual(&g_fVolumeThreshold);
			pMix[3].Next.volumes.fRearRight = fNewVol;
			m_bSilent &= pMix[3].Next.IsLessOrEqual(&g_fVolumeThreshold);
		}
#endif
#endif

		if (m_bFirst)
		{
			//If this is the first volumes computed, initialize previous volumes with new volumes to ensure
			//a valid volume when interpolating from previous to next.
			for(AkUInt8 iChannel = 0; iChannel < m_uChannels; iChannel++)
				pMix[iChannel].Previous = pMix[iChannel].Next;
		}

		//If the source isn't in 3D, all motion occurs at the same position.  Compute only one set of volumes.			
		if (in_pPBI->GetPannerType() == Ak2D)
			break;
	}
}

void AkFeedbackParams::TermFilter()
{
#if !defined AK_WII
	if (m_pLPFilter)
	{
		m_pLPFilter->Term();

		AkDeleteAligned( g_DefaultPoolId, m_pLPFilter );
		m_pLPFilter = NULL;
	}
#endif
}

