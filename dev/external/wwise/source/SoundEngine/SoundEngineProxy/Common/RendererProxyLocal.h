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

#include "IRendererProxy.h"

class RendererProxyLocal : public AK::Comm::IRendererProxy
{
public:
	RendererProxyLocal();
	virtual ~RendererProxyLocal();

	// IRendererProxy members
	virtual AkPlayingID PostEvent( AkUniqueID in_eventID, AkWwiseGameObjectID in_gameObjectPtr, unsigned int in_cookie ) const;
	virtual AKRESULT ExecuteActionOnEvent( AkUniqueID in_eventID, AK::SoundEngine::AkActionOnEventType in_ActionType, AkWwiseGameObjectID in_gameObjectID = AK_INVALID_GAME_OBJECT, AkTimeMs in_uTransitionDuration = 0, AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

	//About RTPC Manager
	virtual AKRESULT RegisterGameObj( AkWwiseGameObjectID in_GameObj, const char* in_pszObjectName = "" );
	virtual AKRESULT UnregisterGameObj( AkWwiseGameObjectID in_GameObj );
	virtual AKRESULT SetActiveListeners( AkWwiseGameObjectID in_GameObjectID, AkUInt32 in_uListenerMask );
	virtual AKRESULT SetAttenuationScalingFactor( AkWwiseGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor );
	virtual void SetRelativePosition( const AkSoundPosition & in_rPosition );
	virtual AKRESULT SetPosition( AkWwiseGameObjectID in_GameObj, const AkSoundPosition& in_rPosition );
	virtual AKRESULT SetMultiplePositions( AkWwiseGameObjectID in_GameObjectID, const AkSoundPosition * in_pPositions, AkUInt16 in_NumPositions, AK::SoundEngine::MultiPositionType in_eMultiPositionType = AK::SoundEngine::MultiPositionType_MultiDirections );
	virtual AKRESULT SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_ulIndex = 0 );
	virtual AKRESULT SetListenerScalingFactor( AkUInt32 in_ulIndex, AkReal32 in_fAttenuationScalingFactor );
	virtual AKRESULT SetListenerSpatialization( AkUInt32 in_uIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets = NULL );
	virtual void SetRTPC( AkRtpcID in_RTPCid, AkReal32 in_Value, AkWwiseGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT );
	virtual void SetDefaultRTPCValue( AkRtpcID in_RTPCid, AkReal32 in_defaultValue );
	virtual void SetPanningRule( AkPanningRule in_panningRule );
	virtual AKRESULT ResetRTPC( AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );
	virtual void ResetRTPCValue( AkRtpcID in_RTPCid, AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT SetSwitch( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, AkWwiseGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT ResetSwitches( AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );
	virtual AKRESULT PostTrigger( AkTriggerID in_Trigger, AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT );

	virtual AKRESULT ResetAllStates();
	virtual AKRESULT ResetRndSeqCntrPlaylists();

	virtual AKRESULT SetGameObjectAuxSendValues( AkWwiseGameObjectID in_gameObjectID, AkAuxSendValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues);
	virtual AKRESULT SetGameObjectOutputBusVolume( AkWwiseGameObjectID in_gameObjectID, AkReal32 in_fControlValue );
	virtual AKRESULT SetObjectObstructionAndOcclusion( AkWwiseGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel );

	virtual AKRESULT SetObsOccCurve( int in_curveXType, int in_curveYType, AkUInt32 in_uNumPoints, AkRTPCGraphPoint * in_apPoints, AkCurveScaling in_eScaling );
	virtual AKRESULT SetObsOccCurveEnabled( int in_curveXType, int in_curveYType, bool in_bEnabled );

	virtual AKRESULT AddSwitchRTPC( AkSwitchGroupID in_switchGroup, AkRtpcID in_RTPC_ID, AkRTPCGraphPointInteger* in_pArrayConversion, AkUInt32 in_ulConversionArraySize );
	virtual void	 RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup );

	virtual void	 SetVolumeThreshold( AkReal32 in_VolumeThreshold );
	virtual void	 SetMaxNumVoicesLimit( AkUInt16 in_maxNumberVoices );

	virtual AKRESULT PostMsgMonitor( const AkUtf16* in_pszMessage );

	virtual AKRESULT PostMsgMonitor( const char* in_pszMessage );

	virtual void StopAll( AkWwiseGameObjectID in_GameObjPtr = AK_INVALID_GAME_OBJECT );
	virtual void StopPlayingID( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration = 0, AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear );

	virtual void SetLoudnessFrequencyWeighting( AkLoudnessFrequencyWeighting in_eLoudnessFrequencyWeighting );
	virtual void EnableSecondaryOutput( bool in_bEnable );
};

#endif // #ifndef AK_OPTIMIZED
