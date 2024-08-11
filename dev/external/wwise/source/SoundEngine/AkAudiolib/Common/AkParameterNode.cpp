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
// AkParameterNode.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include "AkFxBase.h"
#include "AkStateMgr.h"
#include "AkSIS.h"
#include "AkRegistryMgr.h"
#include "AkActionExcept.h"
#include "AkTransitionManager.h"
#include "AkParentNode.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include "AkRTPCMgr.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkModifiers.h"
#include "AkContinuationList.h"
#include "AkRanSeqCntr.h"
#include "AkSwitchCntr.h"
#include "AkAudioMgr.h"
#include "AkActionPlayAndContinue.h"
#include "AkURenderer.h"
#include "Ak3DParams.h"
#include "AkGen3DParams.h"
#include "AkBus.h"
#include "AkLayer.h"
#include "ActivityChunk.h"
#include "AudiolibLimitations.h"
#include "AkBanks.h"

#ifdef AK_MOTION
#include "AkFeedbackBus.h"
#endif // AK_MOTION

CAkParameterNode::CAkParameterNode(AkUniqueID in_ulID)
:CAkParameterNodeBase(in_ulID)
,m_pMapSIS(NULL)
,m_p3DParameters(NULL)
,m_eVirtualQueueBehavior( AkVirtualQueueBehavior_FromBeginning )
,m_eBelowThresholdBehavior( AkBelowThresholdBehavior_ContinueToPlay )
,m_pAssociatedLayers(NULL)
,m_pAuxChunk(NULL)
,m_ePannerType(Ak2D)
,m_ePosSourceType(AkGameDef)
,m_bOverrideGameAuxSends(false)
,m_bUseGameAuxSends(false)
,m_bOverrideUserAuxSends(false)
,m_bOverrideHdrEnvelope(false)
,m_bOverrideAnalysis(false)
,m_bNormalizeLoudness(false)
,m_bEnableEnvelope(false)
{
}

CAkParameterNode::~CAkParameterNode()
{
	if( m_pMapSIS )
	{
		for( AkMapSIS::Iterator iter = m_pMapSIS->Begin(); iter != m_pMapSIS->End(); ++iter )
		{
			AkDelete( g_DefaultPoolId, (*iter).item );
		}
		m_pMapSIS->Term();
		AkDelete( g_DefaultPoolId, m_pMapSIS );
		m_pMapSIS = NULL; // Just to make sure nobody lower will use it.
	}
	DisablePosParams();

	// get rid of the path played flags if any
	FreePathInfo();

	if( m_pAssociatedLayers )
	{
		m_pAssociatedLayers->Term();
		AkDelete( g_DefaultPoolId, m_pAssociatedLayers );
		// m_pAssociatedLayers = NULL; // not required since in destructor and not using it afterward.
	}

	if ( m_pAuxChunk )
	{
		AkDelete( g_DefaultPoolId, m_pAuxChunk );
	}
}

void CAkParameterNode::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
{
#ifdef AKPROP_TYPECHECK
	AKASSERT( typeid(AkReal32) == *g_AkPropTypeInfo[ in_eProp] );
#endif

	if ( ( in_eProp >= AkPropID_Volume && in_eProp <= AkPropID_BusVolume )
#ifdef AK_MOTION
		|| ( in_eProp >= AkPropID_FeedbackVolume && in_eProp <= AkPropID_FeedbackLPF )
#endif
		|| ( in_eProp >= AkPropID_UserAuxSendVolume0 && in_eProp <= AkPropID_OutputBusLPF )
		)
	{
		AkReal32 fDelta = in_fValue - m_props.GetAkProp( in_eProp, 0.0f ).fValue;
		if ( fDelta != 0.0f )
		{
			Notification( g_AkPropRTPCID[ in_eProp ], fDelta );
			m_props.SetAkProp( in_eProp, in_fValue );
		}
	}
	else if ( in_eProp >= AkPropID_PAN_LR && in_eProp <= AkPropID_CenterPCT )
	{
		AkReal32 fDelta = in_fValue - m_props.GetAkProp( in_eProp, 0.0f ).fValue;
		if ( fDelta != 0.0f )
		{
			PositioningChangeNotification( in_fValue, g_AkPropRTPCID[ in_eProp ], NULL );
			m_props.SetAkProp( in_eProp, in_fValue );
		}
	}
	else
	{
		CAkParameterNodeBase::SetAkProp( in_eProp, in_fValue, in_fMin, in_fMax );
	}

	if( in_fMin || in_fMax || m_ranges.FindProp( in_eProp ) )
	{
		RANGED_MODIFIERS<AkPropValue> range;
		range.m_min.fValue = in_fMin;
		range.m_max.fValue = in_fMax;

		m_ranges.SetAkProp( in_eProp, range );
	}
}

void CAkParameterNode::SetAkProp( 
		AkPropID in_eProp, 
		AkInt32 in_iValue, 
		AkInt32 in_iMin, 
		AkInt32 in_iMax 
		)
{
#ifdef AKPROP_TYPECHECK
	AKASSERT( typeid(AkInt32) == *g_AkPropTypeInfo[ in_eProp ] );
#endif

	CAkParameterNodeBase::SetAkProp( in_eProp, in_iValue, in_iMin, in_iMax );

	if( in_iMin || in_iMax || m_ranges.FindProp( in_eProp ) )
	{
		RANGED_MODIFIERS<AkPropValue> range;
		range.m_min.iValue = in_iMin;
		range.m_max.iValue = in_iMax;

		m_ranges.SetAkProp( in_eProp, range );
	}
}

//====================================================================================================
// check parents for 3D params
//====================================================================================================
void CAkParameterNode::Get3DParams( 
	CAkGen3DParams*& out_rp3DParams, 
	CAkRegisteredObj * in_GameObj,  
	AkPannerType & out_ePannerType,
	AkPositionSourceType & out_ePosType,
	BaseGenParams * io_pBasePosParams 
	)
{
	AKASSERT( in_GameObj );

	// Call parent until m_bPositioningInfoOverrideParent is true.
	// Normally m_bPositioningInfoOverrideParent is set on top-level but maybe some odd soundbank
	// design may decide otherwise.
	CAkParameterNode* pParent = static_cast<CAkParameterNode*>( Parent() );
	if ( m_bPositioningInfoOverrideParent 
		|| !pParent )
	{
		Get3DCloneForObject( out_rp3DParams, out_ePosType );
		out_ePannerType = GetPannerType();

		//Check if there is an RTPC hooked on the positioning type.
		if (m_RTPCBitArray.IsSet(POSID_PositioningType))
			out_ePannerType = (AkPannerType)(AkUInt32)g_pRTPCMgr->GetRTPCConvertedValue(this, POSID_PositioningType, in_GameObj);

		Get2DParams( in_GameObj, io_pBasePosParams );
	}
	else
	{
		pParent->Get3DParams( out_rp3DParams, in_GameObj, out_ePannerType, out_ePosType, io_pBasePosParams );
	}
}

AKRESULT CAkParameterNode::GetStatic3DParams( AkPositioningInfo& out_rPosInfo )
{
	CAkParameterNodeBase* pAudioNode = this;

	memset( &out_rPosInfo, 0, sizeof( AkPositioningInfo ) ); //clean output structure

	while( pAudioNode != NULL )
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		pAudioNode = pAudioNode->Parent();

		if( pParameterNode->m_bPositioningInfoOverrideParent
			|| pAudioNode == NULL )
		{
			//Copy 3D params
			if( pParameterNode->m_p3DParameters )
			{
				const Gen3DParams* p3DParams = pParameterNode->m_p3DParameters->GetParams();
				out_rPosInfo.pannerType = (AkPannerType)m_ePannerType;
				out_rPosInfo.posSourceType = (AkPositionSourceType)m_ePosSourceType;
				out_rPosInfo.bUpdateEachFrame = p3DParams->m_bIsDynamic;
				out_rPosInfo.bUseSpatialization = p3DParams->m_bIsSpatialized;

				//attenuation info
				AkUniqueID AttenuationID = pParameterNode->m_p3DParameters->GetParams()->m_uAttenuationID;
				CAkAttenuation* pAttenuation = g_pIndex->m_idxAttenuations.GetPtrAndAddRef( AttenuationID );
				if( pAttenuation )
				{
					out_rPosInfo.bUseAttenuation = true;
					out_rPosInfo.bUseConeAttenuation = pAttenuation->m_bIsConeEnabled;
					if( pAttenuation->m_bIsConeEnabled )
					{
						out_rPosInfo.fInnerAngle = pAttenuation->m_ConeParams.fInsideAngle;
						out_rPosInfo.fOuterAngle = pAttenuation->m_ConeParams.fOutsideAngle;  //convert to degrees?
						out_rPosInfo.fConeMaxAttenuation = pAttenuation->m_ConeParams.fOutsideVolume; //convert to degrees?
						out_rPosInfo.LPFCone = pAttenuation->m_ConeParams.LoPass;
					}

					CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
					if( pVolumeDryCurve )
					{
						out_rPosInfo.fMaxDistance = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize-1].From;
						out_rPosInfo.fVolDryAtMaxDist = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize-1].To;
					}
					
					CAkAttenuation::AkAttenuationCurve* pVolumeAuxGameDefCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeAuxGameDef );
					if(pVolumeAuxGameDefCurve)
						out_rPosInfo.fVolAuxGameDefAtMaxDist = pVolumeAuxGameDefCurve->m_pArrayGraphPoints[pVolumeAuxGameDefCurve->m_ulArraySize-1].To;

					CAkAttenuation::AkAttenuationCurve* pVolumeAuxUserDefCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeAuxUserDef );
					if(pVolumeAuxUserDefCurve)
						out_rPosInfo.fVolAuxUserDefAtMaxDist = pVolumeAuxUserDefCurve->m_pArrayGraphPoints[pVolumeAuxUserDefCurve->m_ulArraySize-1].To;

					CAkAttenuation::AkAttenuationCurve* pLPFCurve = pAttenuation->GetCurve( AttenuationCurveID_LowPassFilter );
					if(pLPFCurve)
						out_rPosInfo.LPFValueAtMaxDist = pLPFCurve->m_pArrayGraphPoints[pLPFCurve->m_ulArraySize-1].To; 

					pAttenuation->Release();
				}
			}
			else
			{
				out_rPosInfo.pannerType = Ak2D;
			}

			//Copy base params
			out_rPosInfo.fCenterPct = pParameterNode->m_props.GetAkProp( AkPropID_CenterPCT, 0.0f ).fValue / 100.0f;
			return AK_Success;
		}
	}

	return AK_IDNotFound;
}

void CAkParameterNode::UpdateBaseParams( CAkRegisteredObj * in_GameObj, BaseGenParams * io_pBasePosParams, CAkGen3DParams * io_p3DParams )
{
	CAkParameterNodeBase* pAudioNode = this;

	do
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		pAudioNode = pAudioNode->Parent();

		if( pParameterNode->m_bPositioningInfoOverrideParent
			|| pAudioNode == NULL )
		{
			pParameterNode->Get2DParams( in_GameObj, io_pBasePosParams );

			if ( io_p3DParams )
			{
				bool bIsUsingRTPC = pParameterNode->Get3DPanning( in_GameObj, io_p3DParams->GetParams()->m_Position );
				io_p3DParams->SetIsPanningFromRTPC( bIsUsingRTPC );
			}

			return;
		}
	}
	while( pAudioNode != NULL );
}

bool CAkParameterNode::GetMaxRadius( AkReal32 & out_fRadius )
{
	CAkParameterNodeBase* pAudioNode = this;
	out_fRadius = 0.0f;

	do
	{
		CAkParameterNode* pParameterNode = static_cast<CAkParameterNode*>( pAudioNode );

		pAudioNode = pAudioNode->Parent();

		if( pParameterNode->m_bPositioningInfoOverrideParent
			|| pAudioNode == NULL )
		{
			bool bReturnValue = false;

			if ( pParameterNode->m_p3DParameters )
			{
				AkUniqueID AttenuationID = pParameterNode->m_p3DParameters->GetParams()->m_uAttenuationID;
				CAkAttenuation* pAttenuation = g_pIndex->m_idxAttenuations.GetPtrAndAddRef( AttenuationID );
				if( pAttenuation )
				{
					CAkAttenuation::AkAttenuationCurve* pVolumeDryCurve = pAttenuation->GetCurve( AttenuationCurveID_VolumeDry );
					if( pVolumeDryCurve )
					{
						out_fRadius = pVolumeDryCurve->m_pArrayGraphPoints[pVolumeDryCurve->m_ulArraySize - 1].From;
						bReturnValue = true; //has attenuation
					}
					pAttenuation->Release();
				}
			}

			return bReturnValue;
		}
	}
	while( pAudioNode != NULL );

	return false;
}

AKRESULT CAkParameterNode::GetAudioParameters(AkSoundParamsEx &io_Parameters, AkUInt32 in_ulParamSelect, AkMutedMap& io_rMutedMap, CAkRegisteredObj * in_GameObjPtr, bool in_bIncludeRange, AkPBIModValues& io_Ranges, bool in_bDoBusCheck /*= true*/)
{
	AKRESULT eResult = AK_Success;
	AkUInt32 ulParamSelect = in_ulParamSelect;

	GetAudioStateParams( io_Parameters, in_ulParamSelect );

	if(in_ulParamSelect & PT_Volume)
	{
		GetPropAndRTPC( io_Parameters.Volume, AkPropID_Volume, in_GameObjPtr );
		
		// Normalization
		if (!io_Parameters.normalization.bServiced && (m_bOverrideAnalysis || !m_pParentNode))
		{
			io_Parameters.normalization.bNormalizeLoudness = m_bNormalizeLoudness;
			io_Parameters.normalization.bServiced = true;
		}
		GetPropAndRTPC( io_Parameters.normalization.fMakeUpGain, AkPropID_MakeUpGain, in_GameObjPtr );
	}
	if(in_ulParamSelect & PT_Pitch)
	{
		GetPropAndRTPC( io_Parameters.Pitch, AkPropID_Pitch, in_GameObjPtr );
	}
	if(in_ulParamSelect & PT_LPF)
	{
		GetPropAndRTPC( io_Parameters.LPF, AkPropID_LPF, in_GameObjPtr );
	}

	if(in_ulParamSelect & PT_HDR)
	{
		if (!io_Parameters.hdr.bHdrServiced && (m_bOverrideHdrEnvelope || !m_pParentNode))
		{		
			GetPropAndRTPCExclusive( io_Parameters.hdr.fActiveRange, AkPropID_HDRActiveRange, in_GameObjPtr );
			io_Parameters.hdr.bEnableEnvelope = m_bEnableEnvelope;
			io_Parameters.hdr.bHdrServiced = true;
		}
	}
	
	if(m_pGlobalSIS)
	{
		ApplySIS( *m_pGlobalSIS, AkPropID_Volume, io_Parameters.Volume );
		ApplySIS( *m_pGlobalSIS, AkPropID_Pitch, io_Parameters.Pitch );
		ApplySIS( *m_pGlobalSIS, AkPropID_LPF, io_Parameters.LPF );

		AkSISValue * pValue = m_pGlobalSIS->m_values.FindProp( AkPropID_MuteRatio );
		if( pValue && pValue->fValue != AK_UNMUTED_RATIO )
		{
			AkMutedMapItem item;
            item.m_bIsPersistent = false;
			item.m_bIsGlobal = true;
			item.m_Identifier = this;
			io_rMutedMap.Set( item, pValue->fValue );
		}
	}

	if( m_pMapSIS )
	{
		CAkSIS** l_ppSIS = m_pMapSIS->Exists( in_GameObjPtr );
		if( l_ppSIS )
		{
			CAkSIS* pSIS = *l_ppSIS;
			ApplySIS( *pSIS, AkPropID_Volume, io_Parameters.Volume );
			ApplySIS( *pSIS, AkPropID_Pitch, io_Parameters.Pitch );
			ApplySIS( *pSIS, AkPropID_LPF, io_Parameters.LPF );

			AkSISValue * pValue = pSIS->m_values.FindProp( AkPropID_MuteRatio );
			if( pValue && pValue->fValue != AK_UNMUTED_RATIO )
			{
				AkMutedMapItem item;
				item.m_bIsPersistent = false;
				item.m_bIsGlobal = false;
				item.m_Identifier = this;
				io_rMutedMap.Set( item, pValue->fValue );
			}
		}
	}

	if(in_bIncludeRange && !m_ranges.IsEmpty() )
	{
		ApplyRange<AkReal32>( AkPropID_Volume, io_Ranges.VolumeOffset );
		ApplyRange<AkReal32>( AkPropID_Pitch, io_Ranges.PitchOffset );
		ApplyRange<AkReal32>( AkPropID_LPF, io_Ranges.LPFOffset );
	}

	// User Defined
	if( !io_Parameters.bUserDefinedServiced && (m_bOverrideUserAuxSends || !m_pParentNode) )
	{
		io_Parameters.bUserDefinedServiced = true;
		if( m_pAuxChunk )
		{
			GetPropAndRTPC( io_Parameters.aUserAuxSendVolume[0], AkPropID_UserAuxSendVolume0, in_GameObjPtr );
			GetPropAndRTPC( io_Parameters.aUserAuxSendVolume[1], AkPropID_UserAuxSendVolume1, in_GameObjPtr );
			GetPropAndRTPC( io_Parameters.aUserAuxSendVolume[2], AkPropID_UserAuxSendVolume2, in_GameObjPtr );
			GetPropAndRTPC( io_Parameters.aUserAuxSendVolume[3], AkPropID_UserAuxSendVolume3, in_GameObjPtr );
	
			for( int i = 0; i < AK_NUM_AUX_SEND_PER_OBJ; ++i )
			{
				io_Parameters.aAuxSend[i] = m_pAuxChunk->aAux[i];
			}
		}
	}

	// Game Defined
	if( !io_Parameters.bGameDefinedServiced && (m_bOverrideGameAuxSends || !m_pParentNode) )
	{
		io_Parameters.bGameDefinedServiced = true;

		GetPropAndRTPC( io_Parameters.fGameAuxSendVolume, AkPropID_GameAuxSendVolume, in_GameObjPtr );

		io_Parameters.bGameDefinedAuxEnabled= m_bUseGameAuxSends;
	}
	
	if(in_bDoBusCheck && m_pBusOutputNode)
	{
		GetPropAndRTPC( io_Parameters.fOutputBusVolume, AkPropID_OutputBusVolume, in_GameObjPtr );
		GetPropAndRTPC( io_Parameters.fOutputBusLPF, AkPropID_OutputBusLPF, in_GameObjPtr );

		if(m_pParentNode != NULL)
		{
			m_pParentNode->GetAudioParameters(io_Parameters,ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, false);
		}

		m_pBusOutputNode->GetAudioParameters(io_Parameters,ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, false);

	}
	else
	{
		if(m_pParentNode != NULL)
		{
			m_pParentNode->GetAudioParameters(io_Parameters,ulParamSelect, io_rMutedMap, in_GameObjPtr, in_bIncludeRange, io_Ranges, in_bDoBusCheck);
		}
	}

	if( m_pAssociatedLayers )
	{
		for ( LayerList::Iterator it = m_pAssociatedLayers->Begin(), itEnd = m_pAssociatedLayers->End();
			  it != itEnd;
			  ++it )
		{
			(*it)->GetAudioParameters( this, io_Parameters, ulParamSelect, io_rMutedMap, in_GameObjPtr );
		}
	}

	return eResult;
}

void CAkParameterNode::SetAkProp(
	AkPropID in_eProp,
	CAkRegisteredObj * in_GameObjPtr,
	AkValueMeaning in_eValueMeaning,
	AkReal32 in_fTargetValue,
	AkCurveInterpolation in_eFadeCurve,
	AkTimeMs in_lTransitionTime
	)
{
#ifndef AK_OPTIMIZED
	switch ( in_eProp )
	{
	case AkPropID_Pitch:
		{
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_PitchChanged, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_fTargetValue != m_props.GetAkProp( AkPropID_Pitch, 0.0f ).fValue ) ) ) 
			{
				MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_PitchChanged, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	case AkPropID_Volume:
		{
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_VolumeChanged, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && in_fTargetValue != m_props.GetAkProp( AkPropID_Volume, 0.0f ).fValue ) )
			{
				MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_VolumeChanged, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	case AkPropID_LPF:
		{
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_LPFChanged, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_fTargetValue != m_props.GetAkProp( AkPropID_LPF, 0.0f ).fValue ) ) ) 
			{
				MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_LPFChanged, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	default:
		AKASSERT( false );
	}
#endif

	CAkSIS* pSIS = GetSIS( in_GameObjPtr );
	if ( pSIS )
		StartSISTransition( pSIS, in_eProp, in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

void CAkParameterNode::Mute(
		CAkRegisteredObj *	in_GameObjPtr,
		AkCurveInterpolation		in_eFadeCurve /*= AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime /*= 0*/
		)
{
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_Muted, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );
	
	MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_Muted, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT );

	CAkSIS* pSIS = GetSIS( in_GameObjPtr );
	if ( pSIS )
		StartSisMuteTransitions(pSIS,AK_MUTED_RATIO,in_eFadeCurve,in_lTransitionTime);
}

void CAkParameterNode::Unmute( CAkRegisteredObj * in_GameObjPtr, AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	AKASSERT(g_pRegistryMgr);
	
	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_Unmuted, ID(), false, in_GameObjPtr?in_GameObjPtr->ID():AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );

	CAkSIS* pSIS = NULL;
	if( in_GameObjPtr != NULL )
	{
		if( m_pMapSIS )
		{
			CAkSIS** l_ppSIS = m_pMapSIS->Exists( in_GameObjPtr );
			if( l_ppSIS )
			{
				pSIS = *l_ppSIS;
			}
		}
	}
	else
	{
		if( m_pGlobalSIS )
		{
			AkSISValue * pValue = m_pGlobalSIS->m_values.FindProp( AkPropID_MuteRatio );
			if ( pValue && pValue->fValue != AK_UNMUTED_RATIO )
			{
				g_pRegistryMgr->SetNodeIDAsModified( this );
				pSIS = m_pGlobalSIS;
			}
		}
	}
	if(pSIS)
	{
		StartSisMuteTransitions(pSIS,AK_UNMUTED_RATIO,in_eFadeCurve,in_lTransitionTime);
	}
}

void CAkParameterNode::UnmuteAllObj(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	if( m_pMapSIS )
	{
		for( AkMapSIS::Iterator iter = m_pMapSIS->Begin(); iter != m_pMapSIS->End(); ++iter )
		{
			AkSISValue * pValue = (*iter).item->m_values.FindProp( AkPropID_MuteRatio );
			if ( pValue && pValue->fValue != AK_UNMUTED_RATIO )
			{
				Unmute( (*iter).item->m_pGameObj, in_eFadeCurve,in_lTransitionTime );
			}
		}
	}
}

void CAkParameterNode::UnmuteAll(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	Unmute(NULL,in_eFadeCurve,in_lTransitionTime);
	UnmuteAllObj(in_eFadeCurve,in_lTransitionTime);
}

void CAkParameterNode::Unregister(CAkRegisteredObj * in_GameObjPtr)
{
	if( m_pMapSIS )
	{
		AkMapSIS::Iterator iter = m_pMapSIS->Begin();
		while( iter != m_pMapSIS->End() )
		{
			if( (*iter).key == in_GameObjPtr )
			{
				if( (*iter).item )
				{
					AkDelete( g_DefaultPoolId, (*iter).item );
				}
				iter = m_pMapSIS->Erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}
}

void CAkParameterNode::ResetAkProp(
	AkPropID in_eProp, 
	AkCurveInterpolation in_eFadeCurve,
	AkTimeMs in_lTransitionTime
	)
{
	if( m_pMapSIS )
	{
		for( AkMapSIS::Iterator iter = m_pMapSIS->Begin(); iter != m_pMapSIS->End(); ++iter )
		{
			AkSISValue * pValue = (*iter).item->m_values.FindProp( in_eProp );
			if ( pValue && pValue->fValue != 0.0f )
			{
				SetAkProp( in_eProp, (*iter).item->m_pGameObj, AkValueMeaning_Default );
			}
		}
	}

	if( m_pGlobalSIS )
	{
		AkSISValue * pValue = m_pGlobalSIS->m_values.FindProp( in_eProp );
		if ( pValue && pValue->fValue != 0.0f )
		{
			SetAkProp( in_eProp, NULL, AkValueMeaning_Default, 0, in_eFadeCurve, in_lTransitionTime );
		}
	}
}

static CAkActionPlayAndContinue* CreateDelayedAction( ContParams* in_pContinuousParams, AkPBIParams& in_rPBIParams, AkUniqueID in_uTargetElementID )
{
	CAkActionPlayAndContinue* pAction = CAkActionPlayAndContinue::Create( AkActionType_PlayAndContinue, 0, in_pContinuousParams->spContList );
	if(pAction)
	{
		pAction->SetPauseCount( in_pContinuousParams->ulPauseCount );
		pAction->SetHistory( in_rPBIParams.playHistory );
		WwiseObjectID wwiseId( in_uTargetElementID );
		pAction->SetElementID( wwiseId );
		pAction->SetInstigator( in_rPBIParams.pInstigator );
		pAction->SetSAInfo( in_rPBIParams.sequenceID );
		pAction->SetIsFirstPlay( in_rPBIParams.bIsFirst );
		pAction->SetInitialPlaybackState( in_rPBIParams.ePlaybackState );
	}
	return pAction;
}

static AKRESULT CreateDelayedPendingAction( ContParams* in_pContinuousParams, AkPBIParams& in_rPBIParams, AkInt32 iDelaySamples, CAkActionPlayAndContinue* in_pAction )
{
	AKRESULT eResult = AK_InsufficientMemory;

	AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( in_rPBIParams.pGameObj ) );
	if( pPendingAction )
	{
		// copy the transitions we have
		if (
			in_pAction->SetPlayStopTransition( in_pContinuousParams->pPlayStopTransition, in_pContinuousParams->bIsPlayStopTransitionFading, pPendingAction ) == AK_Success
			&&
			in_pAction->SetPauseResumeTransition( in_pContinuousParams->pPauseResumeTransition, in_pContinuousParams->bIsPauseResumeTransitionFading, pPendingAction ) == AK_Success
			)
		{
			in_pAction->SetPathInfo( in_pContinuousParams->pPathInfo );

			eResult = in_pAction->SetAkProp( AkPropID_DelayTime, iDelaySamples, 0, 0 );
			if ( eResult == AK_Success )
			{
				pPendingAction->pAction = in_pAction;
				pPendingAction->UserParam = in_rPBIParams.userParams;

				g_pAudioMgr->EnqueueOrExecuteAction( pPendingAction );
			}
			else
			{
				AkDelete( g_DefaultPoolId, pPendingAction );
				pPendingAction = NULL;
			}
		}
		else
		{
			AkDelete( g_DefaultPoolId, pPendingAction );
			pPendingAction = NULL;
		}
	}
	return eResult;
}

AKRESULT CAkParameterNode::DelayPlayback( AkReal32 in_fDelay, AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_InsufficientMemory;

	ContParams* pContinuousParams = NULL;
	AkPathInfo	PathInfo = {NULL, AK_INVALID_UNIQUE_ID };

    ContParams continuousParams( &PathInfo );
    continuousParams.spContList.Attach( CAkContinuationList::Create() );
	if ( !continuousParams.spContList )
		return AK_Fail;

	if( in_rPBIParams.pContinuousParams )
	{
		pContinuousParams = in_rPBIParams.pContinuousParams;
	}
	else
	{
		// Create a new ContinuousParameterSet.
		continuousParams.spContList.Attach( CAkContinuationList::Create() );
		if ( !continuousParams.spContList )
			return AK_Fail;
		pContinuousParams = &continuousParams;
	}

	CAkActionPlayAndContinue* pAction = CreateDelayedAction( pContinuousParams, in_rPBIParams, ID() );
	if(pAction)
	{
		pAction->m_bSkipDelay = true;
		pAction->m_ePBIType = in_rPBIParams.eType;

		eResult = CreateDelayedPendingAction( pContinuousParams, in_rPBIParams, AkTimeConv::SecondsToSamples( in_fDelay ), pAction );

		// we are done with these
		pAction->Release();
	}

	return eResult;
}

AKRESULT CAkParameterNode::PlayAndContinueAlternate( AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_Fail;

    if( in_rPBIParams.pContinuousParams && in_rPBIParams.pContinuousParams->spContList )
	{
		// Set history ready for next
		AkUInt32& ulrCount = in_rPBIParams.playHistory.HistArray.uiArraySize;

		while( ulrCount )
		{
			if( in_rPBIParams.playHistory.IsContinuous( ulrCount -1 ) )
			{
				break;
			}
			else
			{
				--ulrCount;
			}
		}

		//Determine next
		AkUniqueID			NextElementToPlayID	= AK_INVALID_UNIQUE_ID;
		AkTransitionMode	l_eTransitionMode	= Transition_Disabled;
		AkReal32			l_fTransitionTime	= 0;
		AkUInt16			wPositionSelected	= 0;

		while( !in_rPBIParams.pContinuousParams->spContList->m_listItems.IsEmpty() )
		{
			CAkContinueListItem & item = in_rPBIParams.pContinuousParams->spContList->m_listItems.Last();
			if( !( item.m_pMultiPlayNode ) )
			{
				AkUniqueID uSelectedNodeID_UNUSED;
				CAkParameterNodeBase* pNode = item.m_pContainer->GetNextToPlayContinuous( in_rPBIParams.pGameObj, wPositionSelected, uSelectedNodeID_UNUSED, item.m_pContainerInfo, item.m_LoopingInfo );
				if(pNode)
				{
					in_rPBIParams.playHistory.HistArray.aCntrHist[ in_rPBIParams.playHistory.HistArray.uiArraySize - 1 ] = wPositionSelected;
					NextElementToPlayID = pNode->ID();
					pNode->Release();
					l_eTransitionMode = item.m_pContainer->TransitionMode();
					l_fTransitionTime = item.m_pContainer->TransitionTime( in_rPBIParams.pGameObj );
					break;
				}
				else
				{
					in_rPBIParams.playHistory.RemoveLast();
					while( in_rPBIParams.playHistory.HistArray.uiArraySize
						&& !in_rPBIParams.playHistory.IsContinuous( in_rPBIParams.playHistory.HistArray.uiArraySize - 1 ) )
					{
						in_rPBIParams.playHistory.RemoveLast();
					}
					in_rPBIParams.pContinuousParams->spContList->m_listItems.RemoveLast();
				}
			}
			else // Encountered a switch block
			{
				item.m_pMultiPlayNode->ContGetList( item.m_pAlternateContList, in_rPBIParams.pContinuousParams->spContList );
				in_rPBIParams.pContinuousParams->spContList->m_listItems.RemoveLast();
				
				if( !in_rPBIParams.pContinuousParams->spContList )
				{
					eResult = AK_PartialSuccess;
					break;
				}
			}
		}

		//Then launch next if there is a next
		if( NextElementToPlayID != AK_INVALID_UNIQUE_ID )
		{
			// create the action we need
			CAkActionPlayAndContinue* pAction = CreateDelayedAction( in_rPBIParams.pContinuousParams, in_rPBIParams, NextElementToPlayID );
			if(pAction)
			{
				AkInt32 iDelay;
				AkInt32 iMinimalDelay = AK_NUM_VOICE_REFILL_FRAMES * AK_WAIT_BUFFERS_AFTER_PLAY_FAILED;

				if ( l_eTransitionMode == Transition_Delay )
				{
					iDelay = AkTimeConv::MillisecondsToSamples( l_fTransitionTime );
					if( iDelay < iMinimalDelay )
					{
						// WG-24660 - PAd the situation the user specified a Delay of 0 ms (or any transition under one frame)

						iDelay = iMinimalDelay;
					}
				}
				else
				{
					// WG-2352: avoid freeze on loop
					// WG-4724: Delay must be exactly the size of a
					//         buffer to avoid sample accurate glitches
					//         and Buffer inconsistencies
					iDelay = iMinimalDelay;
				}

				eResult = CreateDelayedPendingAction( in_rPBIParams.pContinuousParams, in_rPBIParams, iDelay, pAction );

				// we are done with these
				pAction->Release();
			}
		}
		if ( in_rPBIParams.pContinuousParams->spContList && eResult != AK_Success && eResult != AK_PartialSuccess )
		{
			in_rPBIParams.pContinuousParams->spContList = NULL;
		}

		if( eResult != AK_Success && eResult != AK_PartialSuccess )
		{
			MONITOR_OBJECTNOTIF( in_rPBIParams.userParams.PlayingID(), in_rPBIParams.pGameObj->ID(), in_rPBIParams.userParams.CustomParam(), AkMonitorData::NotificationReason_ContinueAborted, in_rPBIParams.playHistory.HistArray, ID(), false, 0 );
		}
	}

	return eResult;
}

AKRESULT CAkParameterNode::ExecuteActionExceptParentCheck( ActionParamsExcept& in_rAction )
{
	//This function is to be called when the message passes from a bus to a non-bus children.
	//The first pointed children must them run up the actor mixer hierarchy to find if it should be part of the exception.

	CAkParameterNode* pNode = (CAkParameterNode*)( this->Parent() );
	while( pNode )
	{
		if( IsException( pNode, *(in_rAction.pExeceptionList) ) )
		{
			return AK_Success;// We are part of the exception.
		}
		pNode = (CAkParameterNode*)( pNode->Parent() );
	}

	// Not an exception, continue.
	return ExecuteActionExcept( in_rAction );
}

AKRESULT CAkParameterNode::SetInitialParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{	
	AKRESULT eResult = m_props.SetInitialParams( io_rpData, io_rulDataSize );
	if ( eResult != AK_Success )
		return AK_Fail;

	eResult = m_ranges.SetInitialParams( io_rpData, io_rulDataSize );
	if ( eResult != AK_Success )
		return AK_Fail;

	// Use State is hardcoded to true, not read from banks
	m_bUseState = true;

	return AK_Success;
}

AKRESULT CAkParameterNode::SetInitialFxParams(AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool in_bPartialLoadOnly)
{
	AKRESULT eResult = AK_Success;

	// Read Num Fx
	AkUInt8 bIsOverrideParentFX = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	if(!in_bPartialLoadOnly)
		m_bIsFXOverrideParent = bIsOverrideParentFX ? true : false;

	AkUInt32 uNumFx = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AKASSERT( uNumFx <= AK_NUM_EFFECTS_PER_OBJ );
	if ( uNumFx )
	{
		AkUInt32 bitsFXBypass = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		for ( AkUInt32 uFX = 0; uFX < uNumFx; ++uFX )
		{
			AkUInt32 uFXIndex = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			AkUniqueID fxID = READBANKDATA( AkUniqueID, io_rpData, io_rulDataSize);
			AkUInt8 bIsShareSet = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			AkUInt8 bIsRendered = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			RenderedFX( uFXIndex, ( bIsRendered != 0 ) );

			if ( !bIsRendered )
			{
				// Read Size
				if(fxID != AK_INVALID_UNIQUE_ID && 
				   !in_bPartialLoadOnly)
				{
					eResult = SetFX( uFXIndex, fxID, bIsShareSet != 0 );
				}
			}

			if( eResult != AK_Success )
				break;
		}

		if(!in_bPartialLoadOnly)
			MainBypassFX( bitsFXBypass );
	}

	return eResult;
}

void CAkParameterNode::OverrideFXParent( bool in_bIsFXOverrideParent )
{
#ifndef AK_OPTIMIZED
	if ( m_pFXChunk )
	{
		for( int i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
		{
			if( m_pFXChunk->aFX[ i ].bRendered )
				return;
		}
	}
#endif
	m_bIsFXOverrideParent = in_bIsFXOverrideParent;
}

//////////////////////////////////////////////////////////////////////////////
//Positioning information setting
//////////////////////////////////////////////////////////////////////////////

AKRESULT CAkParameterNode::Enable3DPosParams()
{
	AKRESULT eResult = AK_Success;
	if( !m_p3DParameters )
	{	
		m_bPositioningInfoOverrideParent = true;
		CAkGen3DParamsEx * p3DParams = AkNew( g_DefaultPoolId, CAkGen3DParamsEx() );
		if(!p3DParams)
		{
			eResult = AK_InsufficientMemory;
		}
		else
		{
			p3DParams->SetPathOwner(ID());
			m_p3DParameters = p3DParams;
		}
	}
#ifndef AK_OPTIMIZED
	else
	{
		InvalidateAllPaths();
	}
#endif
	return eResult;
}

void CAkParameterNode::DisablePosParams()
{
#ifndef AK_OPTIMIZED
	InvalidateAllPaths();
#endif
	if( m_p3DParameters )
	{
		FreePathInfo();
		m_p3DParameters->Term();
		AkDelete( g_DefaultPoolId, m_p3DParameters );
		m_p3DParameters = NULL;
	}
}

// WAL entry point for positioning type. 
// in_bRTPC not used: See note below.
void CAkParameterNode::PosSetPositioningType( bool in_bOverride, bool in_bRTPC, AkPannerType in_ePanner, AkPositionSourceType in_ePosSource )
{
	m_bPositioningInfoOverrideParent = in_bOverride;
	if (!in_bOverride)
	{
		DisablePosParams();
		m_ePannerType = Ak2D; // We never want m_ePosType to be undefined.
	}
	else
	{	
		// The authoring tool cannot consistently keep track of RTPC curves in order to push 3D values 
		// only when needed. If this function is called, we can assume that they _might be needed_.
		// So unless this node is overriden, it HAS to be ready to accept 3D data.
		Enable3DPosParams();

		m_ePannerType = in_ePanner;
		m_ePosSourceType = in_ePosSource;
	}

	// IMPORTANT: Do not notify if property is RTPC'd! If sound is playing, notification will be issued 
	// from RTPC Mgr; if not playing, it will be fetched on startup.
	if ( !in_bRTPC )
		PositioningChangeNotification( (AkReal32)m_ePannerType, (AkRTPC_ParameterID) POSID_PositioningType, NULL );
}

void CAkParameterNode::PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled )
{
	if( m_p3DParameters )
	{
		m_p3DParameters->SetSpatializationEnabled( in_bIsSpatializationEnabled );
	}
}

void CAkParameterNode::PosSetAttenuationID( AkUniqueID in_AttenuationID )
{
	if( m_p3DParameters )
	{
		m_p3DParameters->SetAttenuationID( in_AttenuationID );
	}
}

AKRESULT CAkParameterNode::PosSetConeUsage( bool in_bIsConeEnabled )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		m_p3DParameters->SetConeUsage( in_bIsConeEnabled );
		PositioningChangeNotification( (AkReal32)in_bIsConeEnabled, (AkRTPC_ParameterID) POSID_Positioning_Cone_Attenuation_ON_OFF, NULL );
		eResult = AK_Success;
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

void CAkParameterNode::PosSetIsPositionDynamic( bool in_bIsDynamic )
{
	if( m_p3DParameters )
	{
		m_p3DParameters->SetIsPositionDynamic( in_bIsDynamic );
		PositioningChangeNotification( (AkReal32)in_bIsDynamic, (AkRTPC_ParameterID) POSID_IsPositionDynamic, NULL );
	}
}

void CAkParameterNode::PosSetFollowOrientation( bool in_bFollow )
{
	if( m_p3DParameters )
	{
		m_p3DParameters->SetFollowOrientation( in_bFollow );
	}
}

AKRESULT CAkParameterNode::PosSetPathMode( AkPathMode in_ePathMode )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		// get rid of the path played flags if any
		FreePathInfo();

		m_p3DParameters->SetPathMode( in_ePathMode );
		PositioningChangeNotification( (AkReal32)in_ePathMode, (AkRTPC_ParameterID) POSID_PathMode, NULL );
		eResult = AK_Success;
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetIsLooping( bool in_bIsLooping )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		m_p3DParameters->SetIsLooping( in_bIsLooping );
		PositioningChangeNotification( (AkReal32)in_bIsLooping, (AkRTPC_ParameterID) POSID_IsLooping, NULL );
		eResult = AK_Success;
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetTransition( AkTimeMs in_TransitionTime )
{
	AKRESULT eResult;
	if( m_p3DParameters )
	{
		m_p3DParameters->SetTransition( in_TransitionTime );
		PositioningChangeNotification( (AkReal32)in_TransitionTime, (AkRTPC_ParameterID) POSID_Transition, NULL );
		eResult = AK_Success;
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

AKRESULT CAkParameterNode::PosSetPath(
	AkPathVertex*           in_pArrayVertex, 
	AkUInt32                 in_ulNumVertices, 
	AkPathListItemOffset*   in_pArrayPlaylist, 
	AkUInt32                 in_ulNumPlaylistItem 
	)
{
	AKRESULT eResult;
#ifndef AK_OPTIMIZED
	InvalidateAllPaths();
#endif

	if( m_p3DParameters )
	{
		eResult = m_p3DParameters->SetPath( in_pArrayVertex, in_ulNumVertices, in_pArrayPlaylist, in_ulNumPlaylistItem );
	}
	else
	{
		eResult = AK_Fail;
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
	return eResult;
}

#ifndef AK_OPTIMIZED
void CAkParameterNode::PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		AkVector in_newPosition,
		AkTimeMs in_DelayToNext
		)
{
	if( m_p3DParameters )
	{
		m_p3DParameters->UpdatePathPoint( in_ulPathIndex, in_ulVertexIndex, in_newPosition, in_DelayToNext );
	}
	else
	{
		AKASSERT( !"3D Parameters must be enabled before trying to set parameters" );
	}
}
#endif

void CAkParameterNode::PosSetPathRange( AkUInt32 in_ulPathIndex, AkReal32 in_fXRange, AkReal32 in_fYRange )
{
	if( m_p3DParameters && in_ulPathIndex < m_p3DParameters->GetParams()->m_ulNumPlaylistItem )
	{
		m_p3DParameters->GetParams()->m_pArrayPlaylist[in_ulPathIndex].fRangeX = in_fXRange;
		m_p3DParameters->GetParams()->m_pArrayPlaylist[in_ulPathIndex].fRangeY = in_fYRange;
	}
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void CAkParameterNode::Get3DCloneForObject( CAkGen3DParams*& in_rp3DParams, AkPositionSourceType & out_ePosType )
{
	out_ePosType = (AkPositionSourceType)m_ePosSourceType;

	if(m_p3DParameters)
	{
		if(in_rp3DParams == NULL)
		{
			in_rp3DParams = AkNew( g_DefaultPoolId, CAkGen3DParams() );
		}
		if( in_rp3DParams )
		{
			// Copying parameters. Will copy the array pointer for the path.
			*in_rp3DParams = *m_p3DParameters;
		}
	}
}

CAkSIS* CAkParameterNode::GetSIS( CAkRegisteredObj * in_GameObjPtr )
{
	AKASSERT(g_pRegistryMgr);
	CAkSIS* l_pSIS = NULL;
	if( in_GameObjPtr != NULL)
	{
		if( !m_pMapSIS )
		{
			AkNew2( m_pMapSIS, g_DefaultPoolId, AkMapSIS, AkMapSIS() );
			if( !m_pMapSIS )
				return NULL;
		}

		CAkSIS** l_ppSIS = m_pMapSIS->Exists( in_GameObjPtr );
		if(l_ppSIS)
		{
			l_pSIS = *l_ppSIS;
		}
		else
		{
			AkUInt8 bitsFXBypass = m_pFXChunk ? m_pFXChunk->bitsMainFXBypass : 0;

			l_pSIS = AkNew( g_DefaultPoolId, CAkSIS( this, bitsFXBypass, in_GameObjPtr ) );
			if( l_pSIS )
			{
				if( !m_pMapSIS->Set( in_GameObjPtr, l_pSIS ) )
				{
					AkDelete( g_DefaultPoolId, l_pSIS );
					l_pSIS = NULL;
				}
				else
				{
					if( in_GameObjPtr->SetNodeAsModified( this ) != AK_Success )
					{
						m_pMapSIS->Unset( in_GameObjPtr );
						AkDelete( g_DefaultPoolId, l_pSIS );
						l_pSIS = NULL;
					}
				}
			}
		}
	}
	else
	{
		g_pRegistryMgr->SetNodeIDAsModified(this);
		if(!m_pGlobalSIS)
		{
			AkUInt8 bitsFXBypass = m_pFXChunk ? m_pFXChunk->bitsMainFXBypass : 0;

			m_pGlobalSIS = AkNew(g_DefaultPoolId,CAkSIS( this, bitsFXBypass ) );
		}
		l_pSIS = m_pGlobalSIS;
	}
	return l_pSIS;
}
// Returns true if the Context may jump to virtual voices, false otherwise.
AkBelowThresholdBehavior CAkParameterNode::GetVirtualBehavior( AkVirtualQueueBehavior& out_Behavior ) const
{
	if( m_bIsVVoicesOptOverrideParent || Parent() == NULL )
	{
		out_Behavior = (AkVirtualQueueBehavior)m_eVirtualQueueBehavior;
		return (AkBelowThresholdBehavior)m_eBelowThresholdBehavior;
	}
	else
	{
		return static_cast<CAkParameterNode*>( Parent() )->GetVirtualBehavior( out_Behavior );
	}
}

bool CAkParameterNode::Has3DParams()
{
	return m_p3DParameters?true:false;
}

AKRESULT CAkParameterNode::SetPositioningParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = AK_Success;

	AkUInt8 cbPositioningInfoOverrideParent =	READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	m_bPositioningInfoOverrideParent = cbPositioningInfoOverrideParent?true:false;

	if( m_bPositioningInfoOverrideParent )
	{
		AkUInt8 cbIs2DPositioningAvailable =	READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
		AkUInt8 cbIs3DPositioningAvailable =	READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		if( cbIs2DPositioningAvailable )
		{
			//2D
			m_bPositioningEnablePanner =	( READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) > 0 );
			m_ePannerType = Ak2D;
		}
		
		if (cbIs3DPositioningAvailable)
		{
			//3D
			AKASSERT( m_p3DParameters == NULL );

			eResult = Enable3DPosParams();
			m_ePannerType = Ak3D;

			if(eResult == AK_Success)
			{
				Gen3DParams * p3DParams = m_p3DParameters->GetParams();

				m_ePosSourceType = (AkPositionSourceType) READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

				p3DParams->m_uAttenuationID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );

				p3DParams->m_bIsSpatialized = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;

				if( m_ePosSourceType == AkGameDef )
				{
					p3DParams->m_bIsDynamic = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;
				}
				else
				{
					p3DParams->m_ePathMode = (AkPathMode) READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
					p3DParams->m_bIsLooping = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;
					p3DParams->m_TransitionTime = READBANKDATA( AkTimeMs, io_rpData, io_rulDataSize );
					p3DParams->m_bFollowOrientation = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize )?true:false;

					// Paths
					AkPathVertex * pVertices = NULL;

					AkUInt32 ulNumVertices = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
					if ( ulNumVertices )
					{
						pVertices = (AkPathVertex *) io_rpData;
						SKIPBANKBYTES( sizeof(AkPathVertex) * ulNumVertices, io_rpData, io_rulDataSize );
					}

					AkUInt32 ulNumPlayListItem = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
					if ( ulNumPlayListItem )
					{
						AkPathListItemOffset * pPlayListItems = (AkPathListItemOffset*) io_rpData;
						SKIPBANKBYTES( sizeof( AkPathListItemOffset ) * ulNumPlayListItem, io_rpData, io_rulDataSize );

						if ( ulNumVertices ) 
							eResult = PosSetPath( pVertices, ulNumVertices, pPlayListItems, ulNumPlayListItem );
					}

					for(AkUInt32 iPath = 0; iPath < ulNumPlayListItem; iPath++)
					{
						AkReal32 fRangeX = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );
						AkReal32 fRangeY = READBANKDATA( AkReal32, io_rpData, io_rulDataSize );
						PosSetPathRange(iPath, fRangeX, fRangeY);
					}
				}
			}
		}
	}

	return eResult;
}
AKRESULT CAkParameterNode::SetAuxParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	m_bOverrideGameAuxSends = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	m_bUseGameAuxSends = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	m_bOverrideUserAuxSends = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;

	bool bHasAux = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;

	AKRESULT eResult = AK_Success;
	for( int i = 0; i < AK_NUM_AUX_SEND_PER_OBJ; ++i )
	{
		AkUniqueID auxID = AK_INVALID_UNIQUE_ID;
		if( bHasAux )
		{
			auxID = READBANKDATA( AkUniqueID, io_rpData, io_rulDataSize );
		}
		eResult = SetAuxBusSend( auxID, i );
		if( eResult != AK_Success )
			break;
	}

	return eResult;
}

AKRESULT CAkParameterNode::SetAdvSettingsParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AkVirtualQueueBehavior eVirtualQueueBehavior = (AkVirtualQueueBehavior) READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	bool bKillNewest =									READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	bool bUseVirtualBehavior =							READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	// Do not use SetMaxNumInstances() here to avoid useless processing on load bank.
	m_u16MaxNumInstance	=								READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );
	m_bIsGlobalLimit =									READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	AkBelowThresholdBehavior eBelowThresholdBehavior = (AkBelowThresholdBehavior) READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkUInt8 ucMaxNumInstOverrideParent =				READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AkUInt8 ucVVoicesOptOverrideParent =				READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	// HDR. Here in advanced settings. Must be here and not in base (see AKBKParameterNode), and 
	// adding a virtual call for these 4 bits is inefficient and would not be clearer/cleaner.
	m_bOverrideHdrEnvelope =							READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	m_bOverrideAnalysis =								READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	m_bNormalizeLoudness =								READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	m_bEnableEnvelope =									READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	SetVirtualQueueBehavior( eVirtualQueueBehavior );
	SetMaxReachedBehavior( bKillNewest );
	SetOverLimitBehavior( bUseVirtualBehavior );
	SetBelowThresholdBehavior( eBelowThresholdBehavior );
	SetMaxNumInstOverrideParent( ucMaxNumInstOverrideParent != 0 );
	SetVVoicesOptOverrideParent( ucVVoicesOptOverrideParent != 0 );

	return AK_Success;
}

void CAkParameterNode::GetFX(
		AkUInt32	in_uFXIndex,
		AkFXDesc&	out_rFXInfo,
		CAkRegisteredObj *	in_GameObj /*= NULL*/
		)
{
	if( m_bIsFXOverrideParent || Parent() == NULL )
	{
		AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

		if ( m_pFXChunk )
		{
			FXStruct & fx = m_pFXChunk->aFX[ in_uFXIndex ];
			if ( fx.id != AK_INVALID_UNIQUE_ID )
			{
				if ( fx.bShareSet )
					out_rFXInfo.pFx.Attach( g_pIndex->m_idxFxShareSets.GetPtrAndAddRef( fx.id ) );
				else
					out_rFXInfo.pFx.Attach( g_pIndex->m_idxFxCustom.GetPtrAndAddRef( fx.id ) );
			}
			else
			{
				out_rFXInfo.pFx = NULL;
			}

			out_rFXInfo.bIsBypassed = GetBypassFX( in_uFXIndex, in_GameObj );
		}
		else
		{
			out_rFXInfo.pFx = NULL;
			out_rFXInfo.bIsBypassed = false;
		}
	}
	else
	{
		Parent()->GetFX( in_uFXIndex, out_rFXInfo, in_GameObj );
	}
}

void CAkParameterNode::GetFXDataID(
		AkUInt32	in_uFXIndex,
		AkUInt32	in_uDataIndex,
		AkUInt32&	out_rDataID
		)
{
	if( m_bIsFXOverrideParent || Parent() == NULL )
	{
		AKASSERT( in_uFXIndex < AK_NUM_EFFECTS_PER_OBJ );

		out_rDataID = AK_INVALID_SOURCE_ID;

		if ( m_pFXChunk )
		{
			FXStruct & fx = m_pFXChunk->aFX[ in_uFXIndex ];

			CAkFxBase * pFx;
			if ( fx.bShareSet )
				pFx = g_pIndex->m_idxFxShareSets.GetPtrAndAddRef( fx.id );
			else
				pFx = g_pIndex->m_idxFxCustom.GetPtrAndAddRef( fx.id );

			if ( pFx )
			{
				out_rDataID = pFx->GetMediaID( in_uDataIndex );
				pFx->Release();
			}
		}
	}
	else
	{
		Parent()->GetFXDataID( in_uFXIndex, in_uDataIndex, out_rDataID );
	}
}

bool CAkParameterNode::GetBypassFX(
		AkUInt32	in_uFXIndex,
		CAkRegisteredObj * in_GameObjPtr )
{
	if ( !m_pFXChunk )
		return false;

	bool bIsBypass;

	if( m_pFXChunk->aFX[ in_uFXIndex ].id != AK_INVALID_UNIQUE_ID && m_RTPCBitArray.IsSet( RTPC_BypassFX0 + in_uFXIndex ) )
	{
		//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
		bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this),  RTPC_BypassFX0 + in_uFXIndex, in_GameObjPtr ) != 0;
	}
	else
	{
		CAkSIS** l_ppSIS = NULL;

		if( m_pMapSIS )
		{
			l_ppSIS = m_pMapSIS->Exists( in_GameObjPtr );
		}

		if( l_ppSIS )
		{
			bIsBypass = ( (*l_ppSIS)->m_bitsFXBypass >> in_uFXIndex ) & 1;
		}
		else if( m_pGlobalSIS )
		{
			bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> in_uFXIndex ) & 1;
		}
		else
		{
			bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> in_uFXIndex ) & 1;
		}
	}

	return bIsBypass;
}

bool CAkParameterNode::GetBypassAllFX(
		CAkRegisteredObj * in_GameObjPtr )
{
	if( m_bIsFXOverrideParent || Parent() == NULL )
	{
		if ( !m_pFXChunk )
			return false;

		bool bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;

		if( m_RTPCBitArray.IsSet( RTPC_BypassAllFX ) )
		{
			//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
			bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this),  RTPC_BypassAllFX, in_GameObjPtr ) != 0;
		}
		else
		{
			CAkSIS** l_ppSIS = NULL;

			if( m_pMapSIS )
			{
				l_ppSIS = m_pMapSIS->Exists( in_GameObjPtr );
			}

			if( l_ppSIS )
			{
				bIsBypass = ( (*l_ppSIS)->m_bitsFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;
			}
			else if( m_pGlobalSIS )
			{
				bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;
			}
		}
		return bIsBypass;
	}
	else
	{
		return Parent()->GetBypassAllFX( in_GameObjPtr );
	}
}

void CAkParameterNode::ResetFXBypass(
	AkUInt32		in_bitsFXBypass,
	AkUInt32        in_uTargetMask /* = 0xFFFFFFFF */ )
{
	if( m_pGlobalSIS )
	{
		m_pGlobalSIS->m_bitsFXBypass = (AkUInt8) ( ( m_pGlobalSIS->m_bitsFXBypass & ~in_uTargetMask ) | ( in_bitsFXBypass & in_uTargetMask ) );
	}

	if( m_pMapSIS )
	{
		for( AkMapSIS::Iterator iter = m_pMapSIS->Begin(); iter != m_pMapSIS->End(); ++iter )
		{
			(*iter).item->m_bitsFXBypass = (AkUInt8) ( ( (*iter).item->m_bitsFXBypass & ~in_uTargetMask ) | ( in_bitsFXBypass & in_uTargetMask ) );
		}
	}
}

AKRESULT CAkParameterNode::AssociateLayer( CAkLayer* in_pLayer )
{
	if( !m_pAssociatedLayers )
	{
		AkNew2( m_pAssociatedLayers, g_DefaultPoolId, LayerList, LayerList() );
	}

	if ( !m_pAssociatedLayers || !m_pAssociatedLayers->AddLast( in_pLayer ) )
	{
		if( m_pAssociatedLayers && m_pAssociatedLayers->IsEmpty() )
		{
			AkDelete( g_DefaultPoolId, m_pAssociatedLayers );
			m_pAssociatedLayers = NULL;
		}
		return AK_InsufficientMemory;
	}

	RecalcNotification();

	return AK_Success;
}

AKRESULT CAkParameterNode::DissociateLayer( CAkLayer* in_pLayer )
{
	AKRESULT eResult = AK_Fail;

	if( m_pAssociatedLayers )
	{
		eResult = m_pAssociatedLayers->Remove( in_pLayer );

		if ( eResult == AK_Success )
		{
			RecalcNotification();
		}
	}

	return eResult;
}

#ifndef AK_OPTIMIZED
void CAkParameterNode::InvalidatePaths()
{
	//This function is not useless, useful to keep virtual table up when called on destructor
}

void CAkParameterNode::InvalidateAllPaths()
{
	if (g_pAudioMgr)
		g_pAudioMgr->InvalidatePendingPaths(ID());
	InvalidatePaths();
}

#endif

AKRESULT CAkParameterNode::IncrementPlayCount( CounterParameters& io_params )
{
	AKRESULT eResult = IncrementPlayCountValue( io_params.uiFlagForwardToBus );
	// We must continue and go up to the top even if IncrementPlayCountValue failed.
	// Decrement will soon be called and will go to the top too, so it must be done all the way up even in case of failure.

	if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
	{
		if( !(io_params.bMaxConsidered) && eResult == AK_Success )
		{
			if( IsGlobalLimit() )
			{
				eResult = IncrementPlayCountGlobal( io_params.fPriority, io_params.ui16NumKicked, io_params.pAMLimiter );
			}
			else
			{
				eResult = IncrementPlayCountGameObject( io_params.fPriority, io_params.ui16NumKicked, io_params.pGameObj, io_params.pAMLimiter );
			}
		}
		io_params.bMaxConsidered = true;
	}

	// bMaxConsidered must be set to false before passing it to Busses
	// We will reset it later before passing to parents.
	bool bMaxConsideredBackup = io_params.bMaxConsidered;

	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if(m_pBusOutputNode)
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Normal;
			io_params.bMaxConsidered = false;

			AKRESULT newResult = m_pBusOutputNode->IncrementPlayCount( io_params );
			eResult = GetNewResultCodeForVirtualStatus( eResult, newResult );
		}
	}

#ifdef AK_MOTION
	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Motion;
			io_params.bMaxConsidered = false;

			AKRESULT newResult = m_pFeedbackInfo->m_pFeedbackBus->IncrementPlayCount( io_params );
			eResult = GetNewResultCodeForVirtualStatus( eResult, newResult );
		}
	}
#endif // AK_MOTION

	if(m_pParentNode)
	{
		//Restore bMaxConsideredBackup;
		io_params.bMaxConsidered = bMaxConsideredBackup;
		AKRESULT newResult = m_pParentNode->IncrementPlayCount( io_params );
		eResult = GetNewResultCodeForVirtualStatus( eResult, newResult );
	}

	return eResult;
}

void CAkParameterNode::DecrementPlayCount( 
	CounterParameters& io_params
	)
{
	DecrementPlayCountValue();

	if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
	{
		if( !(io_params.bMaxConsidered) && IsActivityChunkEnabled() )
		{
			if( IsGlobalLimit() )
			{
				DecrementPlayCountGlobal();
			}
			else
			{
				DecrementPlayCountGameObject(io_params.pGameObj );
			}
		}
		io_params.bMaxConsidered = true;
	}

	// bMaxConsidered must be set to false before passing it to Busses
	// We will reset it later before passing to parents.
	bool bMaxConsideredBackup = io_params.bMaxConsidered;

	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if(m_pBusOutputNode)
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Normal;
			io_params.bMaxConsidered = false;
			m_pBusOutputNode->DecrementPlayCount( io_params );
		}
	}

#ifdef AK_MOTION
	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Motion;
			io_params.bMaxConsidered = false;
			m_pFeedbackInfo->m_pFeedbackBus->DecrementPlayCount( io_params );
		}
	}
#endif // AK_MOTION

	if(m_pParentNode)
	{
		io_params.bMaxConsidered = bMaxConsideredBackup;
		m_pParentNode->DecrementPlayCount( io_params );
	}
}

void CAkParameterNode::IncrementVirtualCount( 
		CounterParameters& io_params
		)
{
	AKASSERT( IsActivityChunkEnabled() );

	if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
	{
		if( !(io_params.bMaxConsidered) )
		{
			if( IsGlobalLimit() )
			{
				IncrementVirtualCountGlobal();
			}
			else
			{
				IncrementVirtualCountGameObject( io_params.pGameObj );
			}
			io_params.bMaxConsidered = true;
		}
	}

	// bMaxConsidered must be set to false before passing it to Busses
	// We will reset it later before passing to parents.
	bool bMaxConsideredBackup = io_params.bMaxConsidered;

	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if(m_pBusOutputNode)
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Normal;
			io_params.bMaxConsidered = false;
			m_pBusOutputNode->IncrementVirtualCount( io_params );
		}
	}

#ifdef AK_MOTION
	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Motion;
			io_params.bMaxConsidered = false;
			m_pFeedbackInfo->m_pFeedbackBus->IncrementVirtualCount( io_params );
		}
	}
#endif // AK_MOTION

	if(m_pParentNode)
	{
		io_params.bMaxConsidered = bMaxConsideredBackup;
		m_pParentNode->IncrementVirtualCount( io_params );
	}
}

void CAkParameterNode::DecrementVirtualCount( 
		CounterParameters& io_params
		)
{
	if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
	{
		if( !(io_params.bMaxConsidered) && IsActivityChunkEnabled() )
		{
			if( IsGlobalLimit() )
			{
				DecrementVirtualCountGlobal( io_params.ui16NumKicked, io_params.bAllowKick );
			}
			else
			{
				DecrementVirtualCountGameObject( io_params.ui16NumKicked, io_params.bAllowKick, io_params.pGameObj );
			}
		}
		io_params.bMaxConsidered = true;
	}

	// bMaxConsidered must be set to false before passing it to Busses
	// We will reset it later before passing to parents.
	bool bMaxConsideredBackup = io_params.bMaxConsidered;

	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Normal )
	{
		if(m_pBusOutputNode)
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Normal;
			io_params.bMaxConsidered = false;
			m_pBusOutputNode->DecrementVirtualCount( io_params );
		}
	}

#ifdef AK_MOTION
	if( io_params.uiFlagForwardToBus & AK_ForwardToBusType_Motion )
	{
		if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
		{
			io_params.uiFlagForwardToBus &= ~AK_ForwardToBusType_Motion;
			io_params.bMaxConsidered = false;
			m_pFeedbackInfo->m_pFeedbackBus->DecrementVirtualCount( io_params );
		}
	}
#endif // AK_MOTION

	if(m_pParentNode)
	{
		io_params.bMaxConsidered = bMaxConsideredBackup;
		m_pParentNode->DecrementVirtualCount( io_params );
	}
}

void CAkParameterNode::ApplyMaxNumInstances( AkUInt16 in_u16MaxNumInstance, CAkRegisteredObj* in_pGameObj /*= NULL*/, void* in_pExceptArray/*= NULL*/, bool in_bFromRTPC /*= false*/ )
{
	if( in_bFromRTPC )
	{
		if( IsActivityChunkEnabled() )
		{
			if( in_pGameObj != NULL )
			{
				if( IsGlobalLimit() )
				{
					// Ignore. RTPC Game object scoped has no effect on global limit.
				}
				else if( m_bIsMaxNumInstOverrideParent || Parent() == NULL )
				{
					StructMaxInst* pPerObjCount = m_pActivityChunk->m_ListPlayCountPerObj.Exists( in_pGameObj );
					if( pPerObjCount )
					{
						pPerObjCount->SetMax( in_u16MaxNumInstance );
					}
				}
			}
			else
			{
				if( IsGlobalLimit() )
				{
					UpdateMaxNumInstanceGlobal( in_u16MaxNumInstance );
				}
				else if( in_pExceptArray == NULL )
				{
					for( AkPerObjPlayCount::Iterator iterMax = m_pActivityChunk->m_ListPlayCountPerObj.Begin(); 
						iterMax != m_pActivityChunk->m_ListPlayCountPerObj.End(); 
						++iterMax )
					{
						iterMax.pItem->item.SetMax( in_u16MaxNumInstance );
					}
				}
				else
				{
					//This is a global RTPC call with exceptions.

					for( AkPerObjPlayCount::Iterator iterMax = m_pActivityChunk->m_ListPlayCountPerObj.Begin(); 
						iterMax != m_pActivityChunk->m_ListPlayCountPerObj.End(); 
						++iterMax )
					{
						GameObjExceptArray* l_pExceptArray = static_cast<GameObjExceptArray*>( in_pExceptArray );
						bool l_bIsException = false;
						for( GameObjExceptArray::Iterator iter = l_pExceptArray->Begin(); iter != l_pExceptArray->End(); ++iter )
						{
							if( *(iter.pItem) == iterMax.pItem->key )
							{
								l_bIsException = true;
								break;
							}
						}
						if( !l_bIsException )
						{
							iterMax.pItem->item.SetMax( in_u16MaxNumInstance );
						}
					}
				}
			}
		}
	}
	else // Not from RTPC.
	{
		/// Wwise is calling to update the value, and we are not driven by RTPC, so we update every game object.
		if( IsActivityChunkEnabled() )
		{
			AKASSERT( in_pGameObj == NULL && in_pExceptArray == NULL);
			if( IsGlobalLimit() )
			{
				UpdateMaxNumInstanceGlobal( in_u16MaxNumInstance );
			}
			else
			{
				for( AkPerObjPlayCount::Iterator iterMax = m_pActivityChunk->m_ListPlayCountPerObj.Begin(); 
					iterMax != m_pActivityChunk->m_ListPlayCountPerObj.End(); 
					++iterMax )
				{
					iterMax.pItem->item.SetMax( in_u16MaxNumInstance );
				}
			}
		}
		m_u16MaxNumInstance = in_u16MaxNumInstance; // Only set it when not from RTPC.
	}
}

bool CAkParameterNode::IsOrIsChildOf( CAkParameterNodeBase * in_pNodeToTest, bool )
{
	bool bRet = false;
	bool l_bIsBusChecked = false;
	CAkParameterNode* pNode = this;
	
	while( !bRet && pNode )
	{
		bRet = in_pNodeToTest == pNode;
		if( !bRet && !l_bIsBusChecked && pNode->ParentBus() != NULL )
		{
			bRet = static_cast<CAkBus*>( pNode->ParentBus() )->IsOrIsChildOf( in_pNodeToTest );
			l_bIsBusChecked = true;
		}
		pNode = static_cast<CAkParameterNode*>( pNode->Parent() );
	}
	return bRet;
}

void CAkParameterNode::FreePathInfo()
{
	if( m_p3DParameters )
	{
		m_p3DParameters->FreePathInfo();
	}
}

AkPathState* CAkParameterNode::GetPathState()
{
	if( m_p3DParameters )
	{
		return &(m_p3DParameters->m_PathState);
	}
	else if( Parent() )
	{
		return static_cast<CAkParameterNode*>( Parent() )->GetPathState();
	}
	else
	{
		AKASSERT( !"Path not available" );
		return NULL;
	}
}

AKRESULT CAkParameterNode::SetAuxBusSend( AkUniqueID in_AuxBusID, AkUInt32 in_ulIndex )
{
	if ( !m_pAuxChunk && in_AuxBusID != AK_INVALID_UNIQUE_ID)
	{
		AkNew2( m_pAuxChunk, g_DefaultPoolId, AuxChunk, AuxChunk() );
		if ( !m_pAuxChunk )
			return AK_InsufficientMemory;
	}

	if( m_pAuxChunk )
	{
		m_pAuxChunk->aAux[in_ulIndex] = in_AuxBusID;
		RecalcNotification();
	}

	return AK_Success;
}

void CAkParameterNode::SetOverrideGameAuxSends( bool in_bOverride )
{
	m_bOverrideGameAuxSends = in_bOverride;
	RecalcNotification();
}

void CAkParameterNode::SetUseGameAuxSends( bool in_bUse )
{
	m_bUseGameAuxSends = in_bUse;
	RecalcNotification();
}
void CAkParameterNode::SetOverrideUserAuxSends( bool in_bOverride )
{
	m_bOverrideUserAuxSends = in_bOverride;
	RecalcNotification();
}

void CAkParameterNode::SetOverrideHdrEnvelope( bool in_bOverride )
{
	m_bOverrideHdrEnvelope = in_bOverride;
	RecalcNotification();
}

void CAkParameterNode::SetOverrideAnalysis( bool in_bOverride )
{
	m_bOverrideAnalysis = in_bOverride;
	RecalcNotification();
}

void CAkParameterNode::SetNormalizeLoudness( bool in_bNormalizeLoudness )
{
	m_bNormalizeLoudness = in_bNormalizeLoudness;
	RecalcNotification();
}

void CAkParameterNode::SetEnableEnvelope( bool in_bEnableEnvelope )
{
	m_bEnableEnvelope = in_bEnableEnvelope;
	RecalcNotification();
}

#ifdef AK_MOTION
AKRESULT CAkParameterNode::GetFeedbackParameters( AkFeedbackParams &io_Params, CAkSource *in_pSource, CAkRegisteredObj * in_GameObjPtr, bool in_bDoBusCheck /*= true*/ )
{
	AKRESULT akr = CAkParameterNodeBase::GetFeedbackParameters(io_Params, in_pSource, in_GameObjPtr, in_bDoBusCheck);
	if(m_pGlobalSIS)
	{
		ApplySIS( *m_pGlobalSIS, AkPropID_FeedbackVolume, io_Params.m_NewVolume );
	}

	if( m_pMapSIS )
	{
		CAkSIS** l_ppSIS = m_pMapSIS->Exists( in_GameObjPtr );
		if( l_ppSIS )
		{
			ApplySIS( **l_ppSIS, AkPropID_FeedbackVolume, io_Params.m_NewVolume );
		}
	}

	return akr;
}

AkVolumeValue CAkParameterNode::GetEffectiveFeedbackVolume( CAkRegisteredObj * in_GameObjPtr )
{
	AkVolumeValue l_Volume = CAkParameterNodeBase::GetEffectiveFeedbackVolume(in_GameObjPtr);
	ApplyRange<AkReal32>( AkPropID_FeedbackVolume, l_Volume );

	return l_Volume;
}

AkLPFType CAkParameterNode::GetEffectiveFeedbackLPF( CAkRegisteredObj * in_GameObjPtr )
{
	AkLPFType fLPF = CAkParameterNodeBase::GetEffectiveFeedbackLPF(in_GameObjPtr);
	ApplyRange<AkReal32>( AkPropID_FeedbackLPF, fLPF );

	return fLPF;
}
#endif // AK_MOTION

CAkParameterNode::AuxChunk::AuxChunk()
{
	for( int i = 0; i < AK_NUM_AUX_SEND_PER_OBJ; ++i )
	{
		aAux[i] = AK_INVALID_UNIQUE_ID;
	}
}

AKRESULT CAkParameterNode::HandleInitialDelay( AkPBIParams& in_rPBIParams )
{
	AKRESULT eResult = AK_Success;
	if( in_rPBIParams.bSkipDelay )
	{
		in_rPBIParams.bSkipDelay = false;
	}
	else//if( DelayedSafetyBit ) // TODO OptimizationUsefulHere.....
	{
		AkReal32 fDelay = 0.f;
		GetPropAndRTPC( fDelay, AkPropID_InitialDelay, in_rPBIParams.pGameObj );
		ApplyRange<AkReal32>( AkPropID_InitialDelay, fDelay );
		if( fDelay > 0.f )
		{
			if( in_rPBIParams.sequenceID != AK_INVALID_SEQUENCE_ID )
			{
				// Use the frame Offset system when workign with sample accurate sounds.
				AkUInt32 uDelayInSamples = AkTimeConv::SecondsToSamples( fDelay );
				in_rPBIParams.uFrameOffset += uDelayInSamples;
			}
			else
			{
				// Use the existing PlayAndContinue Action system for all tohers situations.
				eResult = DelayPlayback( fDelay, in_rPBIParams );
				if( eResult == AK_Success )
					return AK_PartialSuccess;// Here, partial success means the playback was successfully delayed. Consider as success but dont play it!
			}
		}
	}
	return eResult;
}


