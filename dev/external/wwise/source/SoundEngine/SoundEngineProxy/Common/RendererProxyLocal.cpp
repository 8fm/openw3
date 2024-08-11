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



#include "RendererProxyLocal.h"

#include "AkAudioLib.h"
#include "AkAudioLibIndex.h"
#include "AkEnvironmentsMgr.h"
#include "AkRTPCMgr.h"
#include "AkStateMgr.h"
#include "AkCritical.h"
#include "AkMonitor.h"
#include "AkPlayingMgr.h"
#include "AkRegistryMgr.h"
#include "AkOutputMgr.h"

RendererProxyLocal::RendererProxyLocal()
{
}

RendererProxyLocal::~RendererProxyLocal()
{
}

AkPlayingID RendererProxyLocal::PostEvent( AkUniqueID in_eventID, AkWwiseGameObjectID in_gameObjectPtr, unsigned int in_cookie ) const
{
	AkCustomParamType customParam;
	customParam.customParam = in_cookie;
	customParam.ui32Reserved = in_cookie ? AK_EVENTWITHCOOKIE_RESERVED_BIT : 0;
	customParam.ui32Reserved |= AK_EVENTFROMWWISE_RESERVED_BIT;
	customParam.pExternalSrcs = NULL;
	AKASSERT( AK::SoundEngine::IsInitialized() );
	return AK::SoundEngine::PostEvent( in_eventID, (AkGameObjectID)in_gameObjectPtr, AK_WWISE_MARKER_NOTIFICATION_FLAGS, NULL, NULL, &customParam );
}

AKRESULT RendererProxyLocal::ExecuteActionOnEvent( AkUniqueID in_eventID, AK::SoundEngine::AkActionOnEventType in_ActionType, AkWwiseGameObjectID in_gameObjectID /* = AK_INVALID_GAME_OBJECT */, AkTimeMs in_uTransitionDuration /* = 0 */, AkCurveInterpolation in_eFadeCurve /* = AkCurveInterpolation_Linear */)
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::ExecuteActionOnEvent( in_eventID, in_ActionType, (AkGameObjectID)in_gameObjectID, in_uTransitionDuration, in_eFadeCurve );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::RegisterGameObj( AkWwiseGameObjectID in_GameObj, const char* in_pszObjectName )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::RegisterGameObj( (AkGameObjectID)in_GameObj, in_pszObjectName );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::UnregisterGameObj( AkWwiseGameObjectID in_GameObj )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::UnregisterGameObj( (AkGameObjectID)in_GameObj );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetActiveListeners( AkWwiseGameObjectID in_GameObjectID, AkUInt32 in_uListenerMask )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetActiveListeners( (AkGameObjectID)in_GameObjectID, in_uListenerMask );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetAttenuationScalingFactor( AkWwiseGameObjectID in_GameObjectID, AkReal32 in_fAttenuationScalingFactor )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetAttenuationScalingFactor( (AkGameObjectID)in_GameObjectID, in_fAttenuationScalingFactor );
	}
	return AK_Fail;
}

void RendererProxyLocal::SetRelativePosition( const AkSoundPosition & in_rPosition )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		g_pRegistryMgr->SetRelativePosition( in_rPosition );
	}
}

AKRESULT RendererProxyLocal::SetPosition( AkWwiseGameObjectID in_GameObj, const AkSoundPosition& in_rPosition )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetPositionInternal( (AkGameObjectID)in_GameObj, in_rPosition );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetMultiplePositions( AkWwiseGameObjectID in_GameObjectID, const AkSoundPosition * in_pPositions, AkUInt16 in_NumPositions, AK::SoundEngine::MultiPositionType in_eMultiPositionType /* = MultiPositionType_MultiDirections */ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetMultiplePositions( (AkGameObjectID)in_GameObjectID, in_pPositions, in_NumPositions, in_eMultiPositionType );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetListenerPosition( const AkListenerPosition& in_rPosition, AkUInt32 in_ulIndex /*= 0*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetListenerPosition( in_rPosition, in_ulIndex );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetListenerScalingFactor( AkUInt32 in_ulIndex, AkReal32 in_fAttenuationScalingFactor )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetListenerScalingFactor( in_ulIndex, in_fAttenuationScalingFactor );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetListenerSpatialization( AkUInt32 in_uIndex, bool in_bSpatialized, AkSpeakerVolumes * in_pVolumeOffsets /*= NULL*/ )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetListenerSpatialization( in_uIndex, in_bSpatialized, in_pVolumeOffsets );
	}
	return AK_Fail;
}

void RendererProxyLocal::SetRTPC( AkRtpcID in_RTPCid, AkReal32 in_Value, AkWwiseGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::SetRTPCValue( in_RTPCid, in_Value, (AkGameObjectID)in_GameObj );
	}
}

void RendererProxyLocal::SetDefaultRTPCValue( AkRtpcID in_RTPCid, AkReal32 in_defaultValue )
{
	if( AK::SoundEngine::IsInitialized() && g_pRTPCMgr )
	{
		g_pRTPCMgr->SetDefaultParamValue(in_RTPCid, in_defaultValue);
	}
}

void RendererProxyLocal::SetPanningRule( AkPanningRule in_panningRule )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		// Set panning rule for main device.
		AK::SoundEngine::SetPanningRule( in_panningRule );
	}
}

AKRESULT RendererProxyLocal::ResetRTPC( AkWwiseGameObjectID in_GameObjectPtr )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::ResetRTPC( (AkGameObjectID)in_GameObjectPtr );
	}
	return AK_Fail;
}

void RendererProxyLocal::ResetRTPCValue(  AkRtpcID in_RTPCid, AkWwiseGameObjectID in_GameObjectPtr )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::ResetRTPCValue( in_RTPCid, (AkGameObjectID)in_GameObjectPtr );
	}
}

AKRESULT RendererProxyLocal::SetSwitch( AkSwitchGroupID in_SwitchGroup, AkSwitchStateID in_SwitchState, AkWwiseGameObjectID in_GameObj /*= AK_INVALID_GAME_OBJECT*/ )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetSwitch( in_SwitchGroup, in_SwitchState, (AkGameObjectID)in_GameObj );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::ResetSwitches( AkWwiseGameObjectID in_GameObjectPtr )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::ResetSwitches( (AkGameObjectID)in_GameObjectPtr );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::PostTrigger( AkTriggerID in_Trigger, AkWwiseGameObjectID in_GameObj  )
{
    if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::PostTrigger( in_Trigger, (AkGameObjectID)in_GameObj );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::ResetAllStates()
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		return g_pStateMgr->ResetAllStates();
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::ResetRndSeqCntrPlaylists()
{
    if( AK::SoundEngine::IsInitialized() )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		return g_pIndex->ResetRndSeqCntrPlaylists();
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetGameObjectAuxSendValues( AkWwiseGameObjectID in_gameObjectID, AkAuxSendValue* in_aEnvironmentValues, AkUInt32 in_uNumEnvValues)
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetGameObjectAuxSendValues( (AkGameObjectID)in_gameObjectID, in_aEnvironmentValues, in_uNumEnvValues );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetGameObjectOutputBusVolume( AkWwiseGameObjectID in_gameObjectID, AkReal32 in_fControlValue )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetGameObjectOutputBusVolume( (AkGameObjectID)in_gameObjectID, in_fControlValue );
	}
	return AK_Fail;
}
	
AKRESULT RendererProxyLocal::SetObjectObstructionAndOcclusion( AkWwiseGameObjectID in_ObjectID, AkUInt32 in_uListener, AkReal32 in_fObstructionLevel, AkReal32 in_fOcclusionLevel )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		return AK::SoundEngine::SetObjectObstructionAndOcclusion( (AkGameObjectID)in_ObjectID, in_uListener, in_fObstructionLevel, in_fOcclusionLevel );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetObsOccCurve( int in_curveXType, int in_curveYType, AkUInt32 in_uNumPoints, AkRTPCGraphPoint * in_apPoints, AkCurveScaling in_eScaling )
{
	if( g_pEnvironmentMgr )
	{
		return g_pEnvironmentMgr->SetObsOccCurve( (CAkEnvironmentsMgr::eCurveXType)in_curveXType, 
												  (CAkEnvironmentsMgr::eCurveYType)in_curveYType, 
												  in_uNumPoints, in_apPoints, in_eScaling );
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::SetObsOccCurveEnabled( int in_curveXType, int in_curveYType, bool in_bEnabled )
{
	if( g_pEnvironmentMgr )
	{
		g_pEnvironmentMgr->SetCurveEnabled( (CAkEnvironmentsMgr::eCurveXType)in_curveXType, 
										    (CAkEnvironmentsMgr::eCurveYType)in_curveYType, 
											in_bEnabled );
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::AddSwitchRTPC( AkSwitchGroupID in_switchGroup, AkRtpcID in_RTPC_ID, AkRTPCGraphPointInteger* in_pArrayConversion, AkUInt32 in_ulConversionArraySize )
{
	if( g_pRTPCMgr )
	{
		return g_pRTPCMgr->AddSwitchRTPC( in_switchGroup, in_RTPC_ID, in_pArrayConversion, in_ulConversionArraySize );
	}
	return AK_Fail;

}

void RendererProxyLocal::RemoveSwitchRTPC( AkSwitchGroupID in_switchGroup )
{
	if( g_pRTPCMgr )
	{
		return g_pRTPCMgr->RemoveSwitchRTPC( in_switchGroup );
	}
}

void RendererProxyLocal::SetVolumeThreshold( AkReal32 in_VolumeThreshold )
{
	AK::SoundEngine::SetVolumeThresholdInternal( in_VolumeThreshold, AK::SoundEngine::AkCommandPriority_WwiseApp );
}

void RendererProxyLocal::SetMaxNumVoicesLimit( AkUInt16 in_maxNumberVoices )
{
	AK::SoundEngine::SetMaxNumVoicesLimitInternal( in_maxNumberVoices, AK::SoundEngine::AkCommandPriority_WwiseApp );
}

AKRESULT RendererProxyLocal::PostMsgMonitor( const AkUtf16* in_pszMessage )
{
	if( AkMonitor::Get() )
	{
		size_t stringSize = AkUtf16StrLen(in_pszMessage) +1;
		AkOSChar* in_pzConverted = (AkOSChar*)AkAlloc( g_DefaultPoolId, sizeof(AkOSChar) * stringSize );
		
		if( in_pzConverted == NULL)
		{
			return AK_Fail;
		}
		
		AK_UTF16_TO_OSCHAR( in_pzConverted, in_pszMessage, stringSize );
		
		AkMonitor::Monitor_PostString( in_pzConverted, AK::Monitor::ErrorLevel_Message );
		AkFree( g_DefaultPoolId, in_pzConverted );
		return AK_Success;
	}
	return AK_Fail;
}

AKRESULT RendererProxyLocal::PostMsgMonitor( const char* in_pszMessage )
{
	AKRESULT eResult = AK_Fail;
	AkUtf16 wideString[ AK_MAX_PATH ];	
	if( AK_CHAR_TO_UTF16( wideString, in_pszMessage, AK_MAX_PATH ) > 0)
		eResult = PostMsgMonitor( wideString );

	return eResult;
}

void RendererProxyLocal::StopAll( AkWwiseGameObjectID in_GameObjPtr )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::StopAll( (AkGameObjectID)in_GameObjPtr );
	}
}

void RendererProxyLocal::StopPlayingID( AkPlayingID in_playingID, AkTimeMs in_uTransitionDuration , AkCurveInterpolation in_eFadeCurve )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::StopPlayingID( in_playingID, in_uTransitionDuration, in_eFadeCurve );
	}
}

void RendererProxyLocal::SetLoudnessFrequencyWeighting( AkLoudnessFrequencyWeighting in_eLoudnessFrequencyWeighting )
{
	if( AK::SoundEngine::IsInitialized() )
	{
		AK::SoundEngine::SetLoudnessFrequencyWeighting( in_eLoudnessFrequencyWeighting );
	}
}

void RendererProxyLocal::EnableSecondaryOutput( bool in_bEnable )
{
	//Used only for authoring app.
#ifdef AK_WIN

	if( !AK::SoundEngine::IsInitialized() )
		return;

	if (in_bEnable)
	{
		AK::SoundEngine::AddSecondaryOutput(0, AkSink_MergeToMain, 0x2);
	}
	else
	{
		AK::SoundEngine::RemoveSecondaryOutput(0, AkSink_MergeToMain);
	}
#endif
}

#endif // #ifndef AK_OPTIMIZED
