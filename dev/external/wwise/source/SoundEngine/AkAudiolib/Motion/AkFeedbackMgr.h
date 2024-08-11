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
// AkFeedbackMgr.h
//
// Feedback device manager.  Controls the usage of various devices types
// for each player.
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FEEDBACK_MGR_H_
#define _AK_FEEDBACK_MGR_H_

#include "AkKeyArray.h"
#include "AkSplitterBus.h"

#define MAKE_DEVICE_KEY(__Company, __Plugin) ((__Plugin << 16) | __Company)

class CAkFeedbackDeviceMgr
{
public:
	~CAkFeedbackDeviceMgr();	

	static inline CAkFeedbackDeviceMgr* Get(){return s_pSingleton;}
	static CAkFeedbackDeviceMgr* Create();
	static void Destroy();

	AKRESULT	AddPlayerFeedbackDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDevicePluginID, void * pDevice = NULL);
	void		RemovePlayerFeedbackDevice(AkUInt8 in_iPlayerID, AkUInt32 in_iCompanyID, AkUInt32 in_iDevicePluginID);

	AkForceInline void		SetPlayerVolume(AkUInt8 in_iPlayer, AkReal32 in_fVolume);
	AkForceInline void		SetPlayerListener(AkUInt8 in_iPlayerID, AkUInt8 in_iListener);
	AkForceInline AkUInt8	PlayerToListener(AkUInt8 in_iPlayerID);
	AkForceInline AkUInt8	ListenerToPlayer(AkUInt8 in_iListener);
	AkForceInline AkUInt8	GetPlayerMask();
	AkForceInline AkUInt8	GetPlayerCount();

	bool		IsFeedbackEnabled();
	bool		IsDeviceActive(AkUInt32 in_iDeviceCompanyID, AkUInt32 in_iDevicePluginID);

	bool		PrepareAudioProcessing(AkRunningVPL & io_runningVPL);
	static void	ApplyMotionLPF(AkRunningVPL & io_runningVPL);
	void		ConsumeVPL( AkRunningVPL & io_runningVPL );

	AkUInt32	GetActivePlayers( CAkPBI* pPBI );

	static void	CleanupAudioVPL(AkRunningVPL & io_runningVPL);
	void		RenderData();
	void		CommandTick();

	void		Stop();

	AkReal32	GetPeak();
	void		HandleStarvation();

#ifndef AK_OPTIMIZED
	void		PostDeviceMonitorData();
#endif
	void		StartOutputCapture(const AkOSChar* in_CaptureFileName);
	void		StopOutputCapture();

public:
	// NOTE: Although it is used only inside CAkFeedbackDeviceMgr, DeviceBus must be public in order to 
	// avoid private nested class access using the CAkKeyArray.

	//Map to hold the association player-devices used.
	struct DeviceBus
	{
		DeviceBus();
		~DeviceBus();

		IAkMotionMixBus*	m_pFinalBus;
	};

private:
	CAkFeedbackDeviceMgr();

	CAkSplitterBus* GetSplitter(AkUInt32 in_idBus);
	void UpdatePlayerCount();
	
	//Map of AKBus IDs to MixBusses.
	typedef CAkKeyArray<AkUInt32, CAkSplitterBus *> tBusArray;

	typedef CAkKeyArray<AkUInt32, DeviceBus, 1> tDeviceMap;

	tBusArray				m_aBusses;												//Map of AkBusID to mixing busses
	tDeviceMap				m_aPlayers[AK_MAX_DEVICES];								//Array of devices for each player
	AkUInt8					m_aPlayerToListener[AK_MAX_DEVICES];					//Which listener is used to represent which player
	AkUInt8					m_aListenerToPlayer[AK_NUM_LISTENERS];					//Which player is binded to which listener (the reverse of m_aPlayerToListener)
	AkUInt32				m_uLastStarvationTime;									//Starvation counter
	AkUInt8					m_uPlayerMask;
	AkUInt8					m_uLastPlayerIndex;
	
	static CAkFeedbackDeviceMgr* s_pSingleton;
};

//////////////////////////////////////////////////////////////////////////
//Inlines

void CAkFeedbackDeviceMgr::SetPlayerListener( AkUInt8 in_iPlayerID, AkUInt8 in_iListener )
{
	AKASSERT(in_iPlayerID < AK_MAX_DEVICES);
	AKASSERT(in_iListener < AK_NUM_LISTENERS);
	m_aListenerToPlayer[m_aPlayerToListener[in_iPlayerID]] &= ~(1 << in_iPlayerID);
	m_aPlayerToListener[in_iPlayerID] = in_iListener;
	m_aListenerToPlayer[in_iListener] |= (1 << in_iPlayerID);	
}

AkUInt8 CAkFeedbackDeviceMgr::PlayerToListener(AkUInt8 in_iPlayerID)
{
	return m_aPlayerToListener[in_iPlayerID];
}

AkUInt8	CAkFeedbackDeviceMgr::ListenerToPlayer(AkUInt8 in_iListener)
{
	return m_aListenerToPlayer[in_iListener] & m_uPlayerMask;
}

AkUInt8 CAkFeedbackDeviceMgr::GetPlayerMask()
{
	return m_uPlayerMask;
}

AkUInt8 CAkFeedbackDeviceMgr::GetPlayerCount()
{
	return m_uLastPlayerIndex;
}

void CAkFeedbackDeviceMgr::SetPlayerVolume( AkUInt8 in_iPlayer, AkReal32 in_fVolume )
{
	AkReal32 fLinVal = 0;
	if (in_fVolume > 0)
	{
		fLinVal = AkMath::dBToLin(-in_fVolume);
		if (fLinVal > 0)
			fLinVal = 1/fLinVal;
		else
			fLinVal = 65535;	//Infinite
	}
	else
		fLinVal = AkMath::dBToLin(in_fVolume);

	tDeviceMap &rDevices = m_aPlayers[in_iPlayer];
	for(AkUInt8 i = 0; i < rDevices.Length(); i++)
		rDevices[i].item.m_pFinalBus->SetMasterVolume(fLinVal);
}

#endif
