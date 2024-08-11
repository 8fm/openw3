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

#include "AkRTPC.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

class IConsoleConnector;
struct AkInitSettings;

namespace AK
{
	namespace Comm
	{
		class IRendererProxy
		{
		public:
			virtual AkPlayingID PostEvent( AkUniqueID in_eventID, AkWwiseGameObjectID in_gameObjectPtr = 0, unsigned int in_cookie = 0 ) const = 0;
			virtual AKRESULT ExecuteActionOnEvent( AkUniqueID in_eventID, AK::SoundEngine::AkActionOnEventType in_ActionType, AkWwiseGameObjectID in_gameObjectID = AK_INVALID_GAME_OBJECT, AkTimeMs in_uTransitionDuration = 0, AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear ) = 0;

			//About RTPC Manager
			virtual AKRESULT RegisterGameObj( AkWwiseGameObjectID in_GameObj, const char* in_pszObjectName = "" ) = 0;
			virtual AKRESULT UnregisterGameObj( AkWwiseGameObjectID in_GameObj ) = 0;
			virtual AKRESULT SetActiveListeners( AkWwiseGameObjectID in_GameObjectID, AkUInt32 in_uListenerMask ) = 0;
			virtual AKRESULT SetAttenuationScalingFactor( AkWwiseGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor ) = 0;
			virtual void SetRelativePosition( const AkSoundPosition & in_rPosition ) = 0;
			virtual AKRESULT SetPosition( AkWwiseGameObjectID in_GameObj, const AkSoundPosition& in_rPosition ) = 0;
			virtual AKRESULT SetMultiplePositions( AkWwiseGameObjectID in_GameObjectID, const AkSoundPosition * in_pPositions, AkUInt16 in_NumPositions, AK::SoundEngine::MultiPositionType in_eMultiPositionType = AK::SoundEngine::MultiPositionType_MultiDirections ) = 0;
			virtual AKRESULT SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_ulIndex = 0 ) = 0;
			virtual AKRESULT SetListenerScalingFactor( AkUInt32 in_ulIndex, AkReal32 in_fAttenuationScalingFactor ) = 0;
			virtual AKRESULT SetListenerSpatialization( AkUInt32 in_uIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets = NULL ) = 0;
			virtual void SetRTPC( AkRtpcID in_RTPCid, AkReal32 in_Value, AkWwiseGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT ) = 0;
			virtual void SetDefaultRTPCValue( AkRtpcID in_RTPCid, AkReal32 in_defaultValue ) = 0;
			virtual void SetPanningRule( AkPanningRule in_panningRule ) = 0;
			virtual AKRESULT ResetRTPC( AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual void ResetRTPCValue( AkRtpcID in_RTPCid, AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT SetSwitch( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, AkWwiseGameObjectID in_GameObj = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT ResetSwitches( AkWwiseGameObjectID in_GameObjectPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual AKRESULT PostTrigger( AkTriggerID in_Trigger, AkWwiseGameObjectID in_GameObjPtr = AK_INVALID_GAME_OBJECT ) = 0;

			virtual AKRESULT ResetAllStates() = 0;
			virtual AKRESULT ResetRndSeqCntrPlaylists() = 0;

			virtual AKRESULT SetGameObjectAuxSendValues( AkWwiseGameObjectID in_gameObjectID, AkAuxSendValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues) = 0;
			virtual AKRESULT SetGameObjectOutputBusVolume( AkWwiseGameObjectID in_gameObjectID, AkReal32 in_fControlValue ) = 0;
			virtual AKRESULT SetObjectObstructionAndOcclusion( AkWwiseGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel ) = 0;

			virtual AKRESULT SetObsOccCurve( int in_curveXType, int in_curveYType, AkUInt32 in_uNumPoints, AkRTPCGraphPoint* in_apPoints, AkCurveScaling in_eScaling ) = 0;
			virtual AKRESULT SetObsOccCurveEnabled( int in_curveXType, int in_curveYType, bool in_bEnabled ) = 0;

			virtual AKRESULT AddSwitchRTPC( AkSwitchGroupID in_switchGroup, AkRtpcID in_RTPC_ID, AkRTPCGraphPointInteger* in_pArrayConversion, AkUInt32 in_ulConversionArraySize ) = 0;
			virtual void	 RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup ) = 0;

			virtual void	 SetVolumeThreshold( AkReal32 in_VolumeThreshold ) = 0;
			virtual void	 SetMaxNumVoicesLimit( AkUInt16 in_maxNumberVoices ) = 0;

			virtual AKRESULT PostMsgMonitor( const AkUtf16* in_pszMessage ) = 0;

			virtual AKRESULT PostMsgMonitor( const char* in_pszMessage ) = 0;

			virtual void StopAll( AkWwiseGameObjectID in_GameObjPtr = AK_INVALID_GAME_OBJECT ) = 0;
			virtual void StopPlayingID( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration = 0, AkCurveInterpolation in_eFadeCurve = AkCurveInterpolation_Linear ) = 0;

			virtual void SetLoudnessFrequencyWeighting( AkLoudnessFrequencyWeighting in_eLoudnessFrequencyWeighting ) = 0;

			virtual void EnableSecondaryOutput( bool in_bEnable ) = 0;
			
			enum MethodIDs
			{
				MethodPostEvent = 1,
				MethodExecuteActionOnEvent,
				MethodRegisterGameObject,
				MethodUnregisterGameObject,
				MethodSetActiveListeners,
				MethodSetAttenuationScalingFactor,
				MethodSetRelativePosition,
				MethodSetPosition,
				MethodSetMultiplePositions,
				MethodSetListenerPosition,
				MethodSetListenerScalingFactor,
				MethodSetListenerSpatialization,
				MethodSetPanningRule,
				MethodSetRTPC,
				MethodSetDefaultRTPCValue,
				MethodResetRTPC,
				MethodResetRTPCValue,
				MethodSetSwitch,
				MethodResetSwitches,
				MethodPostTrigger,

				MethodResetAllStates,
				MethodResetRndSeqCntrPlaylists,

				MethodSetGameObjectAuxSendValues,
				MethodSetGameObjectOutputBusVolume,
				MethodSetObjectObstructionAndOcclusion,

				MethodSetObsOccCurve,
				MethodSetObsOccCurveEnabled,
				
				MethodAddSwitchRTPC,
				MethodRemoveSwitchRTPC,

				MethodSetVolumeThreshold,
				MethodSetMaxNumVoicesLimit,

				MethodPostMsgMonitor,

				MethodStopAll,
				MethodStopPlayingID,

				MethodSetLoudnessFrequencyWeighting,

				MethodEnableSecondaryOutput,

				LastMethodID
			};
		};
	}
}
#endif // #ifndef AK_OPTIMIZED
