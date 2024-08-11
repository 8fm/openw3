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

#include <AK/Tools/Common/AkArray.h>

class CAkFeedbackBus;
class CAkSrcLpFilter;
class CAkPBI;
struct AkSIMDSpeakerVolumes;
struct AkAudioMix;

#ifdef AK_WIIU
#define AK_MAX_DEVICES 6	//2 DRC and 4 Wiimotes.
#else
#define AK_MAX_DEVICES 4
#endif

class AkFeedbackParams
{

private:
	//Disable default constructor/destructor
	AkFeedbackParams(AkUInt16 in_iPlayers, AkChannelMask in_uChannelMask);
	~AkFeedbackParams();

	void Init(AkUInt16 in_iPlayers, AkUInt16 in_iChannels);
	AkReal32 ComputeNormalizationFactor(AkUInt32 in_iIndex);
	void TermFilter();

public:

	static AkFeedbackParams* Create(AkUInt16 in_iPlayers, AkChannelMask in_uChannelMask, AkPannerType in_ePanType, AkPositionSourceType in_ePosSource);
	void Destroy();

	inline AkUInt32 VolumeIndex(AkUInt32 in_iPlayer, AkUInt32 in_iChannel)
	{
		return in_iPlayer * m_uChannels + in_iChannel;
	}

	void StampOldVolumes();
	void ZeroNewVolumes();
	void ZeroNewAttenuations();
#ifndef AK_OPTIMIZED
	void MonitoringMute();
#endif
	bool CreateLowPassFilter();
	void ComputePlayerVolumes(CAkPBI* in_pPBI);
	void CopyVolumes(AkUInt32 in_iPlayer, AkAudioMix * out_pMix)
	{
		AkUInt32 i = VolumeIndex(in_iPlayer, 0);
		for(AkUInt32 iChannel = 0; iChannel < m_uChannels; iChannel++)
			out_pMix[iChannel] = m_Volumes[i+iChannel];
	}

	CAkFeedbackBus * m_pOutput;		//The output the object is routed to.
	AkVolumeValue	m_NewVolume;	//The motion volume (including the bus)	(in DB)
	AkVolumeValue	m_AudioBusVolume; //The audio bus volumes to remove from the total.
	AkLPFType		m_LPF;			//The low pass value.
	AkPitchValue	m_MotionBusPitch;//Pitch of the motion Bus.
#if !defined AK_WII_FAMILY_HW
	CAkSrcLpFilter * m_pLPFilter;	//Low pass filter memory for feedback audio
#endif	
	AkReal32		m_fNextAttenuation[AK_MAX_DEVICES];	//Attenuation to add on top of the position volumes (linear).
	AkUInt16		m_usCompanyID;	//Device's company ID
	AkUInt16		m_usPluginID;	//Device's plugin ID	
	AkUInt16		m_uChannelMask;	//Channel mask
	AkUInt16		m_uChannels :4;		//Number of channels of the associated source
	AkUInt16		m_uPlayerCount :3;	//Number of players (used for the volume array below)
	AkUInt16		m_bFirst :1;		//Used to know when to use new volumes as previous volumes (at the first buffer processed)
	AkUInt16		m_bSilent:1;		//Tells if no players feels the motion.

	static const AkUInt16 ALL_DEVICES = 0xFFFF;	//m_usPluginID will be set to this to indicate 
	//that the data should be sent to all devices
	static const AkUInt16 UNINITIALIZED = 0;	//m_usPluginID will be set to this to indicate 
	//that the structure is not initialized yet.

	//Volumes from the positioning algorithm.  This is a variable sized structure.  The number of volumes
	//is variable and allocated at the same time as the struct to avoid extra costly malloc/free calls.
	//KEEP AT THE END
	AkAudioMix m_Volumes[1];
};
