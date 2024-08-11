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
// AkBus.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkBus.h"
#include "AkAudioLibIndex.h"
#include "AkAudioMgr.h"
#include "AkActionDuck.h"
#include "AkTransitionManager.h"
#include "AkMonitor.h"
#include "AudiolibDefs.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>
#include "AkRegistryMgr.h"
#include "AkDuckItem.h"
#include "Ak3DParams.h"
#include "AkSIS.h"
#include "AkURenderer.h"
#include "AkLEngine.h"
#include "AkRTPCMgr.h"
#include "AkBanks.h"
#include "AkEnvironmentsMgr.h"
#include "AkFxBase.h"
#include "AkBankMgr.h"
#include "ActivityChunk.h"

#ifdef AK_MOTION
	#include "AkFeedbackBus.h"
#endif // AK_MOTION

#ifdef AK_XBOX360
#include "Xmp.h"
#endif

#ifdef AK_PS3
#include <sysutil/sysutil_bgmplayback.h>
#endif

#define AK_MIN_DUCK_LIST_SIZE 0
#define AK_MAX_DUCK_LIST_SIZE 100

extern AkPlatformInitSettings g_PDSettings;

CAkBusCtx	g_MasterBusCtx;
CAkBusCtx	g_SecondaryMasterBusCtx;

CAkBus::CAkBus(AkUniqueID in_ulID)
:CAkActiveParent<CAkParameterNodeBase>(in_ulID)
,m_RecoveryTime(0)
,m_uChannelConfig(AK_SUPPORTED_SPEAKER_SETUP)
,m_fMaxDuckVolume(AK_DEFAULT_MAX_BUS_DUCKING)
,m_fEffectiveBusVolume( 0.0f )//dB
,m_fEffectiveVoiceVolume( 0.0f )//dB
,m_bControBusVolumeUpdated( false )
#ifndef AK_OPTIMIZED
,m_bIsMonitoringMute( false )
#endif
,m_eDuckingState(DuckState_OFF)
,m_bHdrReleaseModeExponential( false )
,m_bHdrReleaseTimeDirty( true )
,m_bHdrGainComputerDirty( true )
,m_bMainOutputHierarchy(false)
#if defined(AK_XBOX360) || defined(AK_PS3)
,m_bIsBackgroundMusicBus(false)
,m_bIsBackgroundMusicMuted(false)
#endif
{
}

CAkBus::~CAkBus()
{

#if defined(AK_XBOX360) || defined(AK_PS3)
	UnsetAsBackgroundMusicBus();
#endif

	for( AkDuckedVolumeList::Iterator iter = m_DuckedVolumeList.Begin(); iter != m_DuckedVolumeList.End(); ++iter )
	{
		(*iter).item.Term(); //Terminating the CAkDuckItem in the list
	}
	m_DuckedVolumeList.Term();

	for( AkDuckedVolumeList::Iterator iter = m_DuckedBusVolumeList.Begin(); iter != m_DuckedBusVolumeList.End(); ++iter )
	{
		(*iter).item.Term(); //Terminating the CAkDuckItem in the list
	}
	m_DuckedBusVolumeList.Term();

	m_ToDuckList.Term();

	if ( g_MasterBusCtx.GetBus() == this )
	{
		{
			AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
			g_MasterBusCtx.SetBus(NULL);
		}
		 if( g_pBankManager )
			g_pBankManager->SetIsFirstBusLoaded( false );
		 CAkBankMgr::SignalLastBankUnloaded();
	}
	else if ( g_SecondaryMasterBusCtx.GetBus() == this )
	{
		AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
		g_SecondaryMasterBusCtx.SetBus(NULL);
	}
	m_mapBusChildId.Term();
}

AKRESULT CAkBus::Init()
{
	AKRESULT eResult = CAkActiveParent<CAkParameterNodeBase>::Init();
	if( eResult == AK_Success )
	{
		eResult = m_ToDuckList.Init( AK_MIN_DUCK_LIST_SIZE, AK_MAX_DUCK_LIST_SIZE );
	}
	if( eResult == AK_Success )
	{
		eResult = m_DuckedVolumeList.Init( AK_MIN_DUCK_LIST_SIZE, AK_MAX_DUCK_LIST_SIZE );
	}
	if( eResult == AK_Success )
	{
		eResult = m_DuckedBusVolumeList.Init( AK_MIN_DUCK_LIST_SIZE, AK_MAX_DUCK_LIST_SIZE );
	}
	return eResult;
}

CAkBus* CAkBus::Create( AkUniqueID in_ulID )
{
	CAkBus* pBus = AkNew( g_DefaultPoolId, CAkBus( in_ulID ) );
	if( pBus )
	{
		if( pBus->Init() != AK_Success )
		{
			pBus->Release();
			pBus = NULL;
		}		
	}

	return pBus;
}

AkNodeCategory CAkBus::NodeCategory()
{
	return AkNodeCategory_Bus;
}

AKRESULT CAkBus::ExecuteAction( ActionParams& in_rAction )
{
	if( IsActiveOrPlaying() )
	{
		if( in_rAction.bIsMasterCall )
		{
			bool bPause = in_rAction.eType == ActionParamType_Pause;
			PauseTransitions( bPause );
		}

		// Process children in reverse order in order to avoid breaking the array if 
		// they self-destruct within ExecuteAction().
		in_rAction.bIsFromBus = true;
		AkUInt32 uIndex = m_mapChildId.Length();
		while ( uIndex > 0 )
		{
			m_mapChildId[--uIndex]->ExecuteAction( in_rAction );
			if( uIndex > m_mapChildId.Length() )
			{
				// SYU-98386-513 - WG-24620
				// Patch: During the iteration some of the children were possibly deleted and the index must be reset to its maximum logical value.
				// Without this patch, this code would definitely crash on next iteration, so it is apparently a good thing to do.
				// We at the moment have no idea of how exactly it hapenned, but so far 2 users reported this issue and this patch worked for them...
				// Better send some useless notifications than crashing systematically
				uIndex = m_mapChildId.Length();
			}

		}
		uIndex = m_mapBusChildId.Length();
		while ( uIndex > 0 )
		{
			m_mapBusChildId[--uIndex]->ExecuteAction( in_rAction );
			if( uIndex > m_mapBusChildId.Length() )
			{
				// SYU-98386-513 - WG-24620
				// Patch: During the iteration some of the children were possibly deleted and the index must be reset to its maximum logical value.
				// Without this patch, this code would definitely crash on next iteration, so it is apparently a good thing to do.
				// We at the moment have no idea of how exactly it hapenned, but so far 2 users reported this issue and this patch worked for them...
				// Better send some useless notifications than crashing systematically
				uIndex = m_mapBusChildId.Length();
			}
		}

		// WARNING: this may have self-destructed during last call to pNode->ExecuteAction( in_rAction );
	}
	return AK_Success;
}

AKRESULT CAkBus::ExecuteActionExcept( ActionParamsExcept& in_rAction )
{
	if( in_rAction.pGameObj == NULL )
	{
		bool bPause = in_rAction.eType == ActionParamType_Pause;
		PauseTransitions( bPause );
	}

	in_rAction.bIsFromBus = true;

	// Process children in reverse order in order to avoid breaking the array if 
	// they self-destruct within ExecuteAction().
	in_rAction.bIsFromBus = true;
	AkUInt32 uIndex = m_mapChildId.Length();
	while ( uIndex > 0 )
	{
		CAkParameterNodeBase* pChild = m_mapChildId[--uIndex];
		if(!IsException( pChild, *(in_rAction.pExeceptionList) ) )
			pChild->ExecuteActionExceptParentCheck( in_rAction );
		if( uIndex > m_mapChildId.Length() )
		{
			// SYU-98386-513 - WG-24620
			// Patch: During the iteration some of the children were possibly deleted and the index must be reset to its maximum logical value.
			// Without this patch, this code would definitely crash on next iteration, so it is apparently a good thing to do.
			// We at the moment have no idea of how exactly it hapenned, but so far 2 users reported this issue and this patch worked for them...
			// Better send some useless notifications than crashing systematically
			uIndex = m_mapChildId.Length();
		}
	}

	uIndex = m_mapBusChildId.Length();
	while ( uIndex > 0 )
	{
		CAkParameterNodeBase* pChild = m_mapBusChildId[--uIndex];
		if(!IsException( pChild, *(in_rAction.pExeceptionList) ) )
			pChild->ExecuteActionExceptParentCheck( in_rAction );
		if( uIndex > m_mapBusChildId.Length() )
		{
			// SYU-98386-513 - WG-24620
			// Patch: During the iteration some of the children were possibly deleted and the index must be reset to its maximum logical value.
			// Without this patch, this code would definitely crash on next iteration, so it is apparently a good thing to do.
			// We at the moment have no idea of how exactly it hapenned, but so far 2 users reported this issue and this patch worked for them...
			// Better send some useless notifications than crashing systematically
			uIndex = m_mapBusChildId.Length();
		}
	}
	return AK_Success;
}

AKRESULT CAkBus::PlayToEnd( CAkRegisteredObj * in_pGameObj , CAkParameterNodeBase* in_NodePtr, AkPlayingID in_PlayingID /* = AK_INVALID_PLAYING_ID */ )
{
	AKRESULT eResult = AK_Success;
	for( ChildrenIterator iter(m_mapBusChildId,m_mapChildId); !iter.End(); ++iter )
	{
		CAkParameterNodeBase* pNode = (*iter);

		eResult = pNode->PlayToEnd( in_pGameObj, in_NodePtr, in_PlayingID );
		if(eResult != AK_Success)
		{
			break;
		}
	}
	return eResult;
}

void CAkBus::PriorityNotification( NotifParams& in_rParams )
{
	if( IsActivityChunkEnabled() )
	{
		AkChildArray& rArray = m_pActivityChunk->GetActiveChildren();
		for( AkChildArray::Iterator iter = rArray.Begin(); iter != rArray.End(); ++iter )
		{
			if( !(*iter)->PriorityOverrideParent() )
			{
				if( (*iter)->IsPlaying() )
				{
					(*iter)->PriorityNotification( in_rParams );
				}
			}
		}
	}
}

void CAkBus::ForAllPBI( 
		AkForAllPBIFunc in_funcForAll,
		CAkRegisteredObj * in_pGameObj,
		void * in_pCookie )
{
	if( IsActivityChunkEnabled() )
	{
		AkChildArray& rArray = m_pActivityChunk->GetActiveChildren();
		for( AkChildArray::Iterator iter = rArray.Begin(); iter != rArray.End(); ++iter )
		{
			if( (*iter)->IsPlaying() )
			{
				(*iter)->ForAllPBI( in_funcForAll, in_pGameObj, in_pCookie );
			}
		}
	}
}

void CAkBus::PropagatePositioningNotification(
		AkReal32			in_RTPCValue,	// 
		AkRTPC_ParameterID	in_ParameterID,	// RTPC ParameterID, must be a Positioning ID.
		CAkRegisteredObj*	in_GameObj,		// Target Game Object
		void*				in_pExceptArray /*= NULL*/
		)
{
	if ( !IsTopBus() )
		CAkLEngine::PositioningChangeNotification( ID(), in_RTPCValue, in_ParameterID );
}

AKRESULT CAkBus::GetAudioParameters(AkSoundParamsEx &io_Parameters, AkUInt32 in_ulParamSelect, AkMutedMap& io_rMutedMap, CAkRegisteredObj * in_pGameObj, bool in_bIncludeRange, AkPBIModValues& io_Ranges, bool in_bDoBusCheck /*= true*/)
{
	AKRESULT eResult = AK_Success;

	in_ulParamSelect &= ~(PT_Volume | PT_BusVolume);

	AkUInt32 ulParamSelect = in_ulParamSelect;

	GetAudioStateParams( io_Parameters, in_ulParamSelect );

	if(in_ulParamSelect & PT_Pitch)
	{
		GetPropAndRTPC( io_Parameters.Pitch, AkPropID_Pitch, NULL );
	}
	if(in_ulParamSelect & PT_LPF)
	{
		GetPropAndRTPC( io_Parameters.LPF, AkPropID_LPF, NULL );
	}
	if(m_pGlobalSIS)
	{
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

#if defined(AK_XBOX360) || defined(AK_PS3)
	if(m_bIsBackgroundMusicMuted)
	{
		AkMutedMapItem item;
        item.m_bIsPersistent = false;
		item.m_bIsGlobal = true;
		item.m_Identifier = this;
		io_rMutedMap.Set( item, AK_MUTED_RATIO );//This override possible other mute level
	}
#endif

	if(m_pBusOutputNode != NULL)
	{
		m_pBusOutputNode->GetAudioParameters( io_Parameters, ulParamSelect, io_rMutedMap, in_pGameObj, in_bIncludeRange, io_Ranges );
	}
	return eResult;
}

AkVolumeValue CAkBus::GetBusEffectiveVolume( BusVolumeType in_VolumeType, AkPropID in_eProp )
{
	AKASSERT( in_eProp == AkPropID_BusVolume || in_eProp == AkPropID_Volume );
	AKASSERT( in_eProp != AkPropID_Volume || in_VolumeType != BusVolumeType_ToNextBusWithEffect ); // Would be unexpected...

	AkVolumeValue l_Volume = 0;

	AkSoundParams soundParams;
	soundParams.Clear();

	switch( in_eProp )
	{
	case AkPropID_Volume:
		GetAudioStateParams( soundParams, PT_Volume );
		l_Volume += soundParams.Volume;
		break;
	case AkPropID_BusVolume:
		GetAudioStateParams( soundParams, PT_BusVolume );
		l_Volume += soundParams.BusVolume;
		break;
	}

	GetPropAndRTPC( l_Volume, in_eProp, NULL );

	if(m_pGlobalSIS)
	{
		ApplySIS( *m_pGlobalSIS, in_eProp, l_Volume );
	}

	l_Volume += GetDuckedVolume( in_eProp );

	if( m_pBusOutputNode != NULL )
	{
		if( in_VolumeType == BusVolumeType_IncludeEntireBusTree 
			|| !((CAkBus*)(m_pBusOutputNode))->IsMixingBus() )
		{
			l_Volume += static_cast<CAkBus*>(m_pBusOutputNode)->GetBusEffectiveVolume( in_VolumeType, in_eProp );
		}
	}

	return l_Volume;
}

void CAkBus::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
{
#ifdef AKPROP_TYPECHECK
	AKASSERT( typeid(AkReal32) == *g_AkPropTypeInfo[ in_eProp ] );
#endif

	if ( in_eProp >= AkPropID_Volume && in_eProp <= AkPropID_BusVolume )
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

}

void CAkBus::ParamNotification( NotifParams& in_rParams )
{
	AKASSERT( in_rParams.pGameObj == NULL );

	// Note: master bus and bus volumes are applied lower in the hierarchy when the is no effect, 
	// otherwise they are applied at the proper level to avoid having pre-effect volumes
	in_rParams.bIsFromBus = true;
	if( (in_rParams.eType == RTPC_BusVolume) && IsMixingBus() )
	{
		if ( IsTopBus() && m_bMainOutputHierarchy )
			CAkLEngine::SetMasterBusVolume( m_bMainOutputHierarchy, in_rParams.fValue );
		else
			CAkLEngine::SetBusVolume( ID(), in_rParams.fValue );
	}
	else
	{
		AKASSERT( in_rParams.pExceptObjects == NULL );

#if defined(AK_WII) || defined(AK_3DS)
		// On the Wii, don't check if active or playing on busses volume it has to be constantly updated.
		// WG-19864  SetVolume ResetVolumes not always working on the Wii / 3DS.
		if( in_rParams.eType == RTPC_BusVolume && !IsActiveOrPlaying() )
		{
			AKASSERT( !IsMixingBus() );
			if( !m_bControBusVolumeUpdated )
			{
				UpdateVoiceVolumes();
			}
			else
			{
				m_fEffectiveBusVolume += in_rParams.fValue;
			}

			// iterate only in busses.
			for( AkMapChildID::Iterator iter = m_mapBusChildId.Begin(); iter != m_mapBusChildId.End(); ++iter )
			{
				(*iter)->ParamNotification( in_rParams );
			}
		}
		else
#endif
		{
			if( IsActiveOrPlaying() )
			{
				// The volume is not passed to the actor mixer hierarchy, it stays at the bus level.
				if( in_rParams.eType == RTPC_BusVolume )
				{
					AKASSERT( !IsMixingBus() );
					AKASSERT( m_bControBusVolumeUpdated );
					m_fEffectiveBusVolume += in_rParams.fValue;

					// iterate only in busses as bus volume dont go in AM Hierarchy
					for( AkMapChildID::Iterator iter = m_mapBusChildId.Begin(); iter != m_mapBusChildId.End(); ++iter )
					{
#if defined(AK_WII) || defined(AK_3DS)
					//	if( (*iter)->IsActiveOrPlaying() )
#else
						if( (*iter)->IsActiveOrPlaying() )
#endif
						{
							(*iter)->ParamNotification( in_rParams );
						}
					}
				}
				else if( in_rParams.eType == RTPC_Volume )
				{
					AKASSERT( m_bControBusVolumeUpdated );
					m_fEffectiveVoiceVolume += in_rParams.fValue;

					// iterate only in busses as bus volume dont go in AM Hierarchy
					for( AkMapChildID::Iterator iter = m_mapBusChildId.Begin(); iter != m_mapBusChildId.End(); ++iter )
					{
						if( (*iter)->IsActiveOrPlaying() )
						{
							(*iter)->ParamNotification( in_rParams );
						}
					}
				}
				else if ( in_rParams.eType == RTPC_HDRBusReleaseTime )
				{
					// HDR release time does not need to be notified to children, but dirty flag must be 
					// set to have lower engine recompute filter coefficient.
					m_bHdrReleaseTimeDirty = true;
				}
				else if ( in_rParams.eType == RTPC_HDRBusThreshold
						|| in_rParams.eType == RTPC_HDRBusRatio )
				{
					// HDR gain computer settings do not need to be notified to children, but dirty flag must be 
					// set to have lower engine cache new values.
					m_bHdrGainComputerDirty = true;
				}
				else
				{
					// Propagate notification to all children
					AKASSERT( IsActivityChunkEnabled() );
					{
						AkChildArray& rArray = m_pActivityChunk->GetActiveChildren();
						for( AkChildArray::Iterator iter = rArray.Begin(); iter != rArray.End(); ++iter )
						{
							// Checking IsActiveOrPlaying instead of IsPlaying to also include Sub Busses tails.
							if( (*iter)->IsActiveOrPlaying() )
							{
								(*iter)->ParamNotification( in_rParams );
							}
						}
					}
				}
			}
		}
	}
}

void CAkBus::MuteNotification(AkReal32 in_fMuteRatio, AkMutedMapItem& in_rMutedItem, bool in_bIsFromBus /*= false*/)
{
	if( IsActivityChunkEnabled() )
	{
		AkChildArray& rArray = m_pActivityChunk->GetActiveChildren();
		for( AkChildArray::Iterator iter = rArray.Begin(); iter != rArray.End(); ++iter )
		{
			if( (*iter)->IsPlaying() )
			{
#if defined(AK_XBOX360) || defined(AK_PS3)
				if( m_bIsBackgroundMusicMuted && in_rMutedItem.m_Identifier == this )
				{
					in_fMuteRatio = AK_MUTED_RATIO;
				}
#endif
				(*iter)->MuteNotification(in_fMuteRatio, in_rMutedItem, true);
			}
		}
	}
}

void CAkBus::UpdateBusBypass( AkRTPC_ParameterID in_ParamID )
{
	// WG-21086 
	switch(in_ParamID)
	{
	case RTPC_BypassFX0:
		NotifyBypass( ( GetBypassFX( 0 ) ? 1 : 0 ) << 0, 1 << 0, NULL);
		break;

	case RTPC_BypassFX1:
		NotifyBypass( ( GetBypassFX( 1 ) ? 1 : 0 ) << 1, 1 << 1, NULL);
		break;

	case RTPC_BypassFX2:
		NotifyBypass( ( GetBypassFX( 2 ) ? 1 : 0 ) << 2, 1 << 2, NULL);
		break;

	case RTPC_BypassFX3:
		NotifyBypass( ( GetBypassFX( 3 ) ? 1 : 0 ) << 3, 1 << 3, NULL);
		break;
	
	case RTPC_BypassAllFX:
		NotifyBypass( ( GetBypassAllFX() ? 1 : 0 ) << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, NULL);
		break;
	}
}

void CAkBus::NotifyBypass(
		AkUInt32 in_bitsFXBypass,
		AkUInt32 in_uTargetMask,
		CAkRegisteredObj * in_GameObj,
		void*	in_pExceptArray /* = NULL */)
{
	if ( IsTopBus() && m_bMainOutputHierarchy )
	{
		CAkLEngine::BypassMasterBusFx( in_bitsFXBypass, in_uTargetMask );
	}
	else
	{
		CAkLEngine::BypassBusFx( ID(), in_bitsFXBypass, in_uTargetMask );
	}
}

AKRESULT CAkBus::CanAddChild( CAkParameterNodeBase * in_pAudioNode )
{
	AKASSERT(in_pAudioNode);

	AKRESULT eResult = AK_Success;
	bool bConnectingBus = in_pAudioNode->IsBusCategory();
	if(in_pAudioNode->ParentBus() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if( !bConnectingBus && m_mapChildId.Exists( in_pAudioNode->ID()) )
	{
		eResult = AK_AlreadyConnected;
	}
	else if( bConnectingBus && m_mapBusChildId.Exists( in_pAudioNode->ID()) )
	{
		eResult = AK_AlreadyConnected;
	}
	else if( bConnectingBus && ID() == in_pAudioNode->ID() )
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

AKRESULT CAkBus::AddChildInternal( CAkParameterNodeBase* pAudioNode )
{
	AKRESULT eResult = CanAddChild(pAudioNode);
	if(eResult == AK_Success)
	{
		CAkParameterNodeBase** ppNode = NULL;
		if( pAudioNode->IsBusCategory() )
		{
			ppNode = m_mapBusChildId.AddNoSetKey( pAudioNode->ID() );
		}
		else
		{
			ppNode = m_mapChildId.AddNoSetKey( pAudioNode->ID() );
		}
		if( !ppNode )
		{
			eResult = AK_Fail;
		}
		else
		{
			*ppNode = pAudioNode;
			pAudioNode->ParentBus(this);
			this->AddRef();
		}
	}

	pAudioNode->Release();

	return eResult;
}

AKRESULT CAkBus::AddChild( WwiseObjectIDext in_ulID )
{
	if(!in_ulID.id)
	{
		return AK_InvalidID;
	}
	
	CAkParameterNodeBase* pAudioNode = g_pIndex->GetNodePtrAndAddRef( in_ulID );
	if ( !pAudioNode )
		return AK_IDNotFound;

	return AddChildInternal( pAudioNode );
}

void CAkBus::RemoveChild( CAkParameterNodeBase* in_pChild )
{
	AKASSERT(in_pChild);

	if( in_pChild->ParentBus() == this )
	{
		in_pChild->ParentBus(NULL);
		if( in_pChild->IsBusCategory() )
		{
			m_mapBusChildId.Unset(in_pChild->ID());
		}
		else
		{
			m_mapChildId.Unset(in_pChild->ID());
		}
		this->Release();
	}
}

void CAkBus::RemoveChild( WwiseObjectIDext in_ulID )
{
	AKASSERT(in_ulID.id);

	CAkParameterNodeBase** l_pANPtr = NULL;
	if( in_ulID.bIsBus )
	{
		l_pANPtr = m_mapBusChildId.Exists(in_ulID.id);
	}
	else
	{
		l_pANPtr = m_mapChildId.Exists(in_ulID.id);
	}
	if( l_pANPtr )
	{
		RemoveChild(*l_pANPtr);
	}
}

///////////////////////////////////////////////////////////////////////////////
//                       DUCKING FUNCTIONS
///////////////////////////////////////////////////////////////////////////////
AKRESULT CAkBus::AddDuck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve,
		AkPropID in_TargetProp
		)
{
	AKASSERT(in_BusID);
	
	AkDuckInfo * pDuckInfo = m_ToDuckList.Set( in_BusID );
	if ( pDuckInfo )
	{
		pDuckInfo->DuckVolume = in_DuckVolume;
		pDuckInfo->FadeCurve = in_eFadeCurve;
		pDuckInfo->FadeInTime = in_FadeInTime;
		pDuckInfo->FadeOutTime = in_FadeOutTime;
		pDuckInfo->TargetProp = in_TargetProp;

#ifndef AK_OPTIMIZED
		if ( m_eDuckingState == DuckState_ON 
			||  m_eDuckingState == DuckState_PENDING )
		{
			CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->GetNodePtrAndAddRef( in_BusID, AkNodeType_Bus ));
			if( pBus )
			{
				// Use 0 fade-time to handle case where volume is modified in app (which sends a AddDuck)
				pBus->Duck( ID(), in_DuckVolume, 0, in_eFadeCurve, in_TargetProp );
				pBus->Release();
			}
		}
#endif

		return AK_Success;
	}
	
	return AK_Fail;
}

AKRESULT CAkBus::RemoveDuck(
		AkUniqueID in_BusID
		)
{
	AKASSERT( g_pIndex );

#ifndef AK_OPTIMIZED
	AkDuckInfo* pDuckInfo = m_ToDuckList.Exists( in_BusID );
	if( pDuckInfo )
	{
		CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->GetNodePtrAndAddRef( in_BusID, AkNodeType_Bus ));
		if( pBus )
		{
			// Remove potential duck that may have occured on the Ducked Bus
			pBus->Unduck( ID(), 0, pDuckInfo->FadeCurve, pDuckInfo->TargetProp );
			pBus->Release();
		}
	}
#endif //AK_OPTIMIZED
	m_ToDuckList.Unset(in_BusID);
	return AK_Success;
}

AKRESULT CAkBus::RemoveAllDuck()
{
	m_ToDuckList.RemoveAll();
	return AK_Success;
}

void CAkBus::SetRecoveryTime(AkUInt32 in_RecoveryTime)
{
	m_RecoveryTime = in_RecoveryTime;
}

void CAkBus::SetMaxDuckVolume( AkReal32 in_fMaxDuckVolume )
{
	if ( in_fMaxDuckVolume == m_fMaxDuckVolume )
		return;

	AkReal32 fOldDuckedBusVolume = GetDuckedVolume( AkPropID_BusVolume );
	AkReal32 fOldDuckedVoiceVolume = GetDuckedVolume( AkPropID_Volume );
	m_fMaxDuckVolume = in_fMaxDuckVolume;

	AkReal32 fNewDuckedBusVolume = GetDuckedVolume( AkPropID_BusVolume );
	AkReal32 fNewDuckedVoiceVolume = GetDuckedVolume( AkPropID_Volume );

	Notification( RTPC_BusVolume, fNewDuckedBusVolume - fOldDuckedBusVolume );
	Notification( RTPC_Volume, fNewDuckedVoiceVolume - fOldDuckedVoiceVolume );
}

void CAkBus::Duck(
		AkUniqueID in_BusID,
		AkVolumeValue in_DuckVolume,
		AkTimeMs in_FadeOutTime,
		AkCurveInterpolation in_eFadeCurve,
		AkPropID in_PropID
		)
{
	AKRESULT eResult = AK_Success;
	CAkDuckItem* pDuckItem = NULL;
	AkDuckedVolumeList* pDuckedList = NULL;
	switch( in_PropID )
	{
	case AkPropID_Volume:
		pDuckedList = &m_DuckedVolumeList;
		break;
	case AkPropID_BusVolume:
		pDuckedList = &m_DuckedBusVolumeList;
		break;
	}
	AKASSERT( pDuckedList );

	pDuckItem = pDuckedList->Exists(in_BusID);
	if(!pDuckItem)
	{
		pDuckItem = pDuckedList->Set(in_BusID);
		if ( pDuckItem )
		{
			pDuckItem->Init( this );
		}
		else
		{
			eResult = AK_Fail;
		}
	}
	
	MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Ducked, 0, 0);

	if(eResult == AK_Success)
	{
		StartDuckTransitions(pDuckItem, in_DuckVolume, AkValueMeaning_Offset, in_eFadeCurve, in_FadeOutTime, in_PropID);
	}
}

void CAkBus::Unduck(
		AkUniqueID in_BusID,
		AkTimeMs in_FadeInTime,
		AkCurveInterpolation in_eFadeCurve,
		AkPropID in_PropID
		)
{
	AkDuckedVolumeList* pDuckedList = NULL;
	switch( in_PropID )
	{
	case AkPropID_Volume:
		pDuckedList = &m_DuckedVolumeList;
		break;
	case AkPropID_BusVolume:
		pDuckedList = &m_DuckedBusVolumeList;
		break;
	}
	AKASSERT( pDuckedList );

	CAkDuckItem* pDuckItem = pDuckedList->Exists(in_BusID);
	if(pDuckItem)
	{
		StartDuckTransitions(pDuckItem, 0, AkValueMeaning_Default, in_eFadeCurve, in_FadeInTime, in_PropID);
		CheckDuck();
	}
}

void CAkBus::PauseDuck(
		AkUniqueID in_BusID
		)
{
	CAkDuckItem* pDuckItem = m_DuckedVolumeList.Exists(in_BusID);
	if(pDuckItem)
	{
		//Setting the transition time to zero with initial value, which will freeze the duck value while waiting
		StartDuckTransitions(pDuckItem, pDuckItem->m_EffectiveVolumeOffset, AkValueMeaning_Independent, AkCurveInterpolation_Linear, 0, AkPropID_Volume );
	}
	pDuckItem = m_DuckedBusVolumeList.Exists(in_BusID);
	if(pDuckItem)
	{
		//Setting the transition time to zero with initial value, which will freeze the duck value while waiting
		StartDuckTransitions(pDuckItem, pDuckItem->m_EffectiveVolumeOffset, AkValueMeaning_Independent, AkCurveInterpolation_Linear, 0, AkPropID_BusVolume);
	}
}

void CAkBus::StartDuckTransitions(CAkDuckItem*		in_pDuckItem,
										AkReal32			in_fTargetValue,
										AkValueMeaning	in_eValueMeaning,
										AkCurveInterpolation		in_eFadeCurve,
										AkTimeMs		in_lTransitionTime,
										AkPropID		in_ePropID)
{
	AKASSERT(g_pTransitionManager);

	// have we got one running already ?
	if(in_pDuckItem->m_pvVolumeTransition != NULL)
	{
		// yup, let's change the direction it goes
		g_pTransitionManager->ChangeParameter(in_pDuckItem->m_pvVolumeTransition,
													in_ePropID,
													in_fTargetValue,
													in_lTransitionTime,
													in_eFadeCurve,
													in_eValueMeaning);
	}
	else
	{
		AkReal32 fStartValue = in_pDuckItem->m_EffectiveVolumeOffset;
		AkReal32 fTargetValue = 0.0f;
		switch(in_eValueMeaning)
		{
		case AkValueMeaning_Offset:
		case AkValueMeaning_Independent:
			fTargetValue = in_fTargetValue;
			break;
		case AkValueMeaning_Default:
			break;
		default:
			AKASSERT(!"Invalid Meaning type");
			break;
		}

		// do we really need to start a transition ?
		if((fStartValue == fTargetValue)
			|| (in_lTransitionTime == 0))
		{
			in_pDuckItem->TransUpdateValue( in_ePropID, fTargetValue, true );
		}
		// start it
		else
		{
			TransitionParameters VolumeParams(
				in_pDuckItem,
				in_ePropID,
				fStartValue,
				fTargetValue,
				in_lTransitionTime,
				in_eFadeCurve,
				true,
				true );
			in_pDuckItem->m_pvVolumeTransition = g_pTransitionManager->AddTransitionToList(VolumeParams);
		}
	}
}

void CAkBus::ApplyMaxNumInstances( AkUInt16 in_u16MaxNumInstance, CAkRegisteredObj* in_pGameObj/* = NULL*/, void* in_pExceptArray/* = NULL*/, bool in_bFromRTPC /*= false*/ )
{
	if( in_pGameObj == NULL )
	{
		// Do not apply RTPC modifications if this is disabled
		if( in_bFromRTPC && m_u16MaxNumInstance == 0 )
			return;

		UpdateMaxNumInstanceGlobal( in_u16MaxNumInstance );

		if( !in_bFromRTPC )
			m_u16MaxNumInstance = in_u16MaxNumInstance;
	}
}

void CAkBus::StartDucking()
{
	m_eDuckingState = DuckState_ON;
	UpdateDuckedBus();
}

void CAkBus::StopDucking()
{
	//Must stop Ducking
	if( m_RecoveryTime && !m_ToDuckList.IsEmpty() )
	{
		AKRESULT eResult = RequestDuckNotif();
		if(eResult == AK_Success)
		{
			m_eDuckingState = DuckState_PENDING;
		}
		else
		{
			// If we cannot get a notif, then just skip the notif and ask for fade back right now.
			// Better doing it now than never
			m_eDuckingState = DuckState_OFF;
		}
	}
	else
	{
		m_eDuckingState = DuckState_OFF;
	}

	UpdateDuckedBus();
}

AKRESULT CAkBus::IncrementPlayCount( CounterParameters& io_params )
{
	AKRESULT eResult = IncrementPlayCountValue( io_params.uiFlagForwardToBus );

	
	if( m_bIsMaxNumInstOverrideParent || m_pBusOutputNode == NULL )
	{
		if( !(io_params.bMaxConsidered) && eResult == AK_Success )
		{
			eResult = IncrementPlayCountGlobal( io_params.fPriority, io_params.ui16NumKicked, io_params.pBusLimiter );
		}
		io_params.bMaxConsidered = true;
	}

	if(m_pBusOutputNode)
	{
		AKRESULT newResult = m_pBusOutputNode->IncrementPlayCount( io_params );
		eResult = GetNewResultCodeForVirtualStatus( eResult, newResult );
	}

#ifdef AK_MOTION
	if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
	{
		AKRESULT newResult = m_pFeedbackInfo->m_pFeedbackBus->IncrementPlayCount( io_params );
		eResult = GetNewResultCodeForVirtualStatus( eResult, newResult );
	}
#endif // AK_MOTION

	if(GetPlayCount() == 1)
	{
		//Must start Ducking
		StartDucking();
	}

	return eResult;
}

void CAkBus::DecrementPlayCount( 
	CounterParameters& io_params
	)
{
	DecrementPlayCountValue();

	if( m_bIsMaxNumInstOverrideParent || m_pBusOutputNode == NULL )
	{
		if( !(io_params.bMaxConsidered) )
		{
			DecrementPlayCountGlobal();
			io_params.bMaxConsidered = true;
		}
	}

	if(m_pBusOutputNode)
	{
		m_pBusOutputNode->DecrementPlayCount( io_params );
	}

#ifdef AK_MOTION
	if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
	{
		m_pFeedbackInfo->m_pFeedbackBus->DecrementPlayCount( io_params );
	}
#endif // AK_MOTION

	AKASSERT( !m_pParentNode ); // Should never happen as actually a bus never has parents, just making sure.

	if(GetPlayCount() == 0)
	{
		StopDucking();
	}
}

void CAkBus::IncrementVirtualCount( 
		CounterParameters& io_params
		)
{
	if( m_bIsMaxNumInstOverrideParent || m_pBusOutputNode == NULL )
	{
		if( !(io_params.bMaxConsidered) && IsActivityChunkEnabled() )
		{
			IncrementVirtualCountGlobal();
		}
		io_params.bMaxConsidered = true;
	}

	if(m_pBusOutputNode)
	{
		m_pBusOutputNode->IncrementVirtualCount( io_params );
	}

#ifdef AK_MOTION
	if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
	{
		m_pFeedbackInfo->m_pFeedbackBus->IncrementVirtualCount( io_params );
	}
#endif // AK_MOTION
}

void CAkBus::DecrementVirtualCount( 
		CounterParameters& io_params
		)
{
	if( m_bIsMaxNumInstOverrideParent || m_pBusOutputNode == NULL )
	{
		if( !(io_params.bMaxConsidered) )
		{
			DecrementVirtualCountGlobal( io_params.ui16NumKicked, io_params.bAllowKick );
			io_params.bMaxConsidered = true;
		}
	}

	if(m_pBusOutputNode)
	{
		m_pBusOutputNode->DecrementVirtualCount( io_params );
	}

#ifdef AK_MOTION
	if (m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL)
	{
		m_pFeedbackInfo->m_pFeedbackBus->DecrementVirtualCount( io_params );
	}
#endif // AK_MOTION
}

bool CAkBus::IncrementActivityCount( AkUInt16 in_flagForwardToBus)
{
	bool bIsSuccessful = IncrementActivityCountValue( in_flagForwardToBus );

	if( m_pBusOutputNode )
	{
		bIsSuccessful &= m_pBusOutputNode->IncrementActivityCount();
	}

#ifdef AK_MOTION
	if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
	{
		bIsSuccessful &=m_pFeedbackInfo->m_pFeedbackBus->IncrementActivityCount();
	}
#endif // AK_MOTION

	return bIsSuccessful;
}

void CAkBus::DecrementActivityCount( AkUInt16 )
{
	DecrementActivityCountValue();

	if( m_pBusOutputNode )
	{
		m_pBusOutputNode->DecrementActivityCount();
	}

#ifdef AK_MOTION
	if ( m_pFeedbackInfo != NULL && m_pFeedbackInfo->m_pFeedbackBus != NULL )
	{
		m_pFeedbackInfo->m_pFeedbackBus->DecrementActivityCount();
	}
#endif // AK_MOTION
}

bool CAkBus::IsOrIsChildOf( CAkParameterNodeBase * in_pNodeToTest )
{
	CAkBus* pBus = this;
	while( pBus )
	{
		if( in_pNodeToTest == pBus )
			return true;
		pBus = static_cast<CAkBus*>( pBus->ParentBus() );
	}
	return false;
}

void CAkBus::DuckNotif()
{
	if(m_eDuckingState == DuckState_PENDING)
	{
		m_eDuckingState = DuckState_OFF;
		UpdateDuckedBus();
	}
}

void CAkBus::UpdateDuckedBus()
{
	for( AkToDuckList::Iterator iter = m_ToDuckList.Begin(); iter != m_ToDuckList.End(); ++iter )
	{
		MapStruct<AkUniqueID, AkDuckInfo>&  rItem = *iter;
		CAkBus* pBus = static_cast<CAkBus*>(g_pIndex->GetNodePtrAndAddRef( rItem.key, AkNodeType_Bus ));
		if(pBus)
		{
			switch(m_eDuckingState)
			{
			case DuckState_OFF:
				pBus->Unduck(ID(),rItem.item.FadeInTime, rItem.item.FadeCurve, rItem.item.TargetProp);
				break;

			case DuckState_ON:
				pBus->Duck(ID(),rItem.item.DuckVolume, rItem.item.FadeOutTime, rItem.item.FadeCurve, rItem.item.TargetProp);
				break;

			case DuckState_PENDING:
				pBus->PauseDuck(ID());
				break;

			default:
				AKASSERT(!"Unknown Ducking State");
			}
			pBus->Release();
		}
	}
}

AKRESULT CAkBus::RequestDuckNotif()
{
	AKRESULT eResult = AK_Fail; 
	CAkActionDuck* pActionDuck = AkNew(g_DefaultPoolId, CAkActionDuck(AkActionType_Duck,0));
	if(pActionDuck)
	{
		if ( pActionDuck->SetAkProp( AkPropID_DelayTime, (AkInt32) m_RecoveryTime, 0, 0 ) == AK_Success )
		{
			WwiseObjectID wwiseId( ID(), AkNodeType_Bus );
			pActionDuck->SetElementID( wwiseId );

			AkPendingAction* pPendingAction = AkNew( g_DefaultPoolId, AkPendingAction( NULL ) );
			if(pPendingAction)
			{
				pPendingAction->pAction = pActionDuck;
				g_pAudioMgr->EnqueueOrExecuteAction(pPendingAction);
				eResult = AK_Success;
			}
		}

		pActionDuck->Release();
	}

	return eResult;
}

void CAkBus::CheckDuck()
{
	bool bIsDucked = false;
	for( AkDuckedVolumeList::Iterator iter = m_DuckedVolumeList.Begin(); iter != m_DuckedVolumeList.End(); ++iter )
	{
		// Should be != 0.0f, but the transition result on a DB value, after conversion is innacurate.
		if( (*iter).item.m_EffectiveVolumeOffset < -0.01f )
		{
			bIsDucked = true;
			break;
		}
	}
	if(!bIsDucked)
	{
		for( AkDuckedVolumeList::Iterator iter = m_DuckedBusVolumeList.Begin(); iter != m_DuckedBusVolumeList.End(); ++iter )
		{
			// Should be != 0.0f, but the transition result on a DB value, after conversion is innacurate.
			if( (*iter).item.m_EffectiveVolumeOffset < -0.01f )
			{
				bIsDucked = true;
				break;
			}
		}
	}

	if(!bIsDucked)
	{
		MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Unducked, 0, 0 );
	}
}

AkVolumeValue CAkBus::GetDuckedVolume( AkPropID in_eProp )
{
	AkDuckedVolumeList* pDuckedList = NULL;
	switch(in_eProp)
	{
	case AkPropID_Volume:
		pDuckedList = &m_DuckedVolumeList;
		break;
	case AkPropID_BusVolume:
		pDuckedList = &m_DuckedBusVolumeList;
		break;
	}
	AkVolumeValue l_DuckedVolume = 0;
	for( AkDuckedVolumeList::Iterator iter = pDuckedList->Begin(); iter != pDuckedList->End(); ++iter )
	{
		l_DuckedVolume += (*iter).item.m_EffectiveVolumeOffset;
	}

	if( l_DuckedVolume < m_fMaxDuckVolume )
	{
		l_DuckedVolume = m_fMaxDuckVolume;
	}

	return l_DuckedVolume;
}

void CAkBus::ChannelConfig( AkUInt32 in_uConfig )
{
	m_uChannelConfig = in_uConfig & AK_SUPPORTED_SPEAKER_SETUP;
}

CAkBus* CAkBus::GetMixingBus()
{
	if( IsMixingBus() && this != g_MasterBusCtx.GetBus())
	{
		return this;
	}
	return CAkParameterNodeBase::GetMixingBus();
}

CAkBus* CAkBus::GetLimitingBus()
{
	if( m_bIsMaxNumInstOverrideParent )
	{
		return this;
	}
	return CAkParameterNodeBase::GetLimitingBus();
}

void CAkBus::UpdateFx(
		AkUInt32 in_uFXIndex
		)
{
	if ( IsTopBus() && m_bMainOutputHierarchy )
		CAkLEngine::UpdateMasterBusFX( in_uFXIndex );
	else
		CAkLEngine::UpdateMixBusFX( ID(), in_uFXIndex );
}

bool CAkBus::HasEffect()
{
	if ( m_pFXChunk )
	{
		for ( AkUInt32 i = 0; i < AK_NUM_EFFECTS_PER_OBJ; ++i )
			if ( m_pFXChunk->aFX[i].id != AK_INVALID_UNIQUE_ID )
				return true;
	}

	return false;
}

bool CAkBus::IsMixingBus()
{
	// On real hardware platform we dont spawn Mixing node for master bus, for all the others yes.
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)
	// Note: HDR and bus positioning not supported on hardware platforms.
	return HasEffect() 
		|| (NodeCategory() == AkNodeCategory_AuxBus);
#else
	return HasEffect() 
		|| (NodeCategory() ==  AkNodeCategory_AuxBus) 
		|| ChannelConfig() != AK_CHANNEL_MASK_PARENT
		|| m_bPositioningEnabled
		|| IsTopBus()
		|| IsHdrBus();
#endif
}

bool CAkBus::IsTopBus() 
{ 
	return ParentBus() == NULL;
}

AKRESULT CAkBus::SetInitialParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
{
	AKRESULT eResult = m_props.SetInitialParams( io_rpData, io_rulDataSize );
	if ( eResult != AK_Success )
		return eResult;

	m_bPositioningEnabled				= ( READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) > 0 );
	m_bPositioningEnablePanner			= ( READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) > 0 );

	bool bKillNewest					= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	bool bUseVirtualBehavior			= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize ) != 0;
	SetMaxReachedBehavior( bKillNewest );
	SetOverLimitBehavior( bUseVirtualBehavior );

	m_u16MaxNumInstance					= READBANKDATA( AkUInt16, io_rpData, io_rulDataSize );
	m_bIsMaxNumInstOverrideParent		= READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	ChannelConfig( READBANKDATA( AkUInt16, io_rpData, io_rulDataSize ) );
	
	// Use State is hardcoded to true, not read from banks
	m_bUseState = true;

	// IMPORTANT must read it even if we use it on AK_XBOX360 only... the format is the same
	AkUInt8 l_IsBackgroundMusic = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	// IMPORTANT must read it even if we use it on Wii only... the format is the same
	AkUInt8 l_EnableWiiCompressor = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

	m_bIsHdrBus = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	m_bHdrReleaseModeExponential = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

#if defined AK_WII_FAMILY_HW
	if ( IsTopBus() )
	{
		EnableHardwareCompressor( l_EnableWiiCompressor );
	}
#endif

#if defined(AK_XBOX360) || defined(AK_PS3)
	if( l_IsBackgroundMusic )
	{
		SetAsBackgroundMusicBus();
	}
#endif

	return AK_Success;
}

AKRESULT CAkBus::SetInitialValues(AkUInt8* in_pData, AkUInt32 in_ulDataSize)
{
	AKRESULT eResult = AK_Success;

	//Read ID
	// We don't care about the ID, just skip it
	SKIPBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	AkUniqueID OverrideBusId = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	if(OverrideBusId)
	{
		CAkParameterNodeBase* pBus = g_pIndex->GetNodePtrAndAddRef( OverrideBusId, AkNodeType_Bus );
		if(pBus)
		{
			eResult = pBus->AddChildByPtr( this );
			pBus->Release();
		}
		else
		{
			// It is now an error to not load the bank content in the proper order.
			MONITOR_ERRORMSG( AKTEXT("Master bus structure not loaded: make sure that the first bank to be loaded contains the master bus information") );
			eResult = AK_Fail;
		}
	}
	else
	{
		if ( g_MasterBusCtx.GetBus() == NULL )
		{
			g_MasterBusCtx.SetBus( this );
			m_bMainOutputHierarchy = true;
		}
		else if (g_SecondaryMasterBusCtx.GetBus() == NULL &&
				 g_MasterBusCtx.GetBus() != this )
		{
			g_SecondaryMasterBusCtx.SetBus( this );
			m_bMainOutputHierarchy = false;
		}
	}

	if(eResult == AK_Success)
	{
		eResult = SetInitialParams( in_pData, in_ulDataSize );
	}

	if(eResult == AK_Success)
	{
		m_RecoveryTime = AkTimeConv::MillisecondsToSamples( READBANKDATA( AkTimeMs, in_pData, in_ulDataSize ) );
		m_fMaxDuckVolume = READBANKDATA( AkReal32, in_pData, in_ulDataSize );

		//Process Child list
		AkUInt32 ulDucks = READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
		for(AkUInt32 i = 0; i < ulDucks; ++i)
		{
			AkUniqueID BusID	= READBANKDATA( AkUInt32, in_pData, in_ulDataSize );
			AkReal32 DuckVolume	= READBANKDATA( AkReal32, in_pData, in_ulDataSize );
			AkTimeMs FadeOut	= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
			AkTimeMs FadeIn		= READBANKDATA( AkTimeMs, in_pData, in_ulDataSize );
			AkUInt8 FadeCurve	= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );
			AkUInt8 TargetProp	= READBANKDATA( AkUInt8, in_pData, in_ulDataSize );

			eResult = AddDuck( BusID, DuckVolume, FadeOut, FadeIn, (AkCurveInterpolation)FadeCurve, (AkPropID)TargetProp );

			if(eResult != AK_Success)
			{
				break;
			}
		}

	}

	if(eResult == AK_Success)
	{
		eResult = SetInitialFxParams(in_pData, in_ulDataSize, false);
	}

	if(eResult == AK_Success)
	{
		eResult = SetInitialRTPC(in_pData, in_ulDataSize);
	}

	if(eResult == AK_Success)
	{
		eResult = ReadStateChunk(in_pData, in_ulDataSize);
	}

	if(eResult == AK_Success)
	{
		eResult = ReadFeedbackInfo(in_pData, in_ulDataSize);
	}

	CHECKBANKDATASIZE( in_ulDataSize, eResult );	

	return eResult;
}

AKRESULT CAkBus::SetInitialFxParams( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize, bool /*in_bPartialLoadOnly*/ )
{
	AKRESULT eResult = AK_Success;

	// Read Num Fx
	AkUInt32 uNumFx = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
	AKASSERT(uNumFx <= AK_NUM_EFFECTS_PER_OBJ);
	if ( uNumFx )
	{
		AkUInt32 bitsFXBypass = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );

		for ( AkUInt32 uFX = 0; uFX < uNumFx; ++uFX )
		{
			AkUInt32 uFXIndex = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			AkUniqueID fxID = READBANKDATA( AkUniqueID, io_rpData, io_rulDataSize);
			AkUInt8 bIsShareSet = READBANKDATA( AkUInt8, io_rpData, io_rulDataSize );
			SKIPBANKDATA( AkUInt8, io_rpData, io_rulDataSize ); // bIsRendered

			if(fxID != AK_INVALID_UNIQUE_ID )
			{
				eResult = SetFX( uFXIndex, fxID, bIsShareSet != 0 );
			}

			if( eResult != AK_Success )
				break;
		}

		MainBypassFX( bitsFXBypass );
	}

	return eResult;
}

void CAkBus::_SetInMainHierarchy( bool in_bIsInMainHierarchy )
{
	m_bMainOutputHierarchy = in_bIsInMainHierarchy;
	// Call on all child busses.
	for( AkMapChildID::Iterator iter = m_mapBusChildId.Begin(); iter != m_mapBusChildId.End(); ++iter )
	{
		AKASSERT( (*iter)->NodeCategory() == AkNodeCategory_Bus
				|| (*iter)->NodeCategory() == AkNodeCategory_AuxBus
				|| (*iter)->NodeCategory() == AkNodeCategory_FeedbackBus );

		((CAkBus*)(*iter))->_SetInMainHierarchy( in_bIsInMainHierarchy );
	}
}

void CAkBus::ParentBus(CAkParameterNodeBase* in_pParent)
{
	CAkActiveParent<CAkParameterNodeBase>::ParentBus(in_pParent);
	if (in_pParent != NULL)
	{
		bool bParentIsInMainOutputHierarchy = ((CAkBus*)in_pParent)->IsInMainHierarchy();
		if (m_bMainOutputHierarchy != bParentIsInMainOutputHierarchy)
		{
			//Re-parented!  Go through child busses to make sure they all know of the new flag
			_SetInMainHierarchy( bParentIsInMainOutputHierarchy );
		}
	}
}

void CAkBus::Parent(CAkParameterNodeBase* in_pParent)
{
	AKASSERT( !"Parent of a bus should always be NULL" );
}

bool CAkBus::GetStateSyncTypes( AkStateGroupID in_stateGroupID, CAkStateSyncArray* io_pSyncTypes )
{
	if( CheckSyncTypes( in_stateGroupID, io_pSyncTypes ) )
		return true;

	if( ParentBus() )
	{
		return static_cast<CAkBus*>( ParentBus() )->GetStateSyncTypes( in_stateGroupID, io_pSyncTypes );
	}
	return false;
}

void CAkBus::EnableHardwareCompressor( bool in_Enable )
{
#if defined AK_WII_FAMILY_HW
	AXSetCompressor( in_Enable ? AX_COMPRESSOR_ON : AX_COMPRESSOR_OFF );
#endif
}

void CAkBus::GetFX( AkUInt32 in_uFXIndex, AkFXDesc& out_rFXInfo, CAkRegisteredObj *	)
{
	AKASSERT( in_uFXIndex <  AK_NUM_EFFECTS_PER_OBJ );

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

		out_rFXInfo.bIsBypassed = GetBypassFX( in_uFXIndex );
	}
	else
	{
		out_rFXInfo.pFx = NULL;
		out_rFXInfo.bIsBypassed = false;
	}
}

void CAkBus::GetFXDataID(
		AkUInt32	in_uFXIndex,
		AkUInt32	in_uDataIndex,
		AkUInt32&	out_rDataID
		)
{
	AKASSERT( in_uFXIndex <  AK_NUM_EFFECTS_PER_OBJ );

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

bool CAkBus::GetBypassFX( AkUInt32 in_uFXIndex )
{
	if ( !m_pFXChunk )
		return false;

	bool bIsBypass = ( m_pFXChunk->bitsMainFXBypass >> in_uFXIndex ) & 1;

	//We Use RTPC VALUE PRIOR TO ACTIONS!
	if( m_pFXChunk->aFX[ in_uFXIndex ].id != AK_INVALID_UNIQUE_ID && m_RTPCBitArray.IsSet( RTPC_BypassFX0 + in_uFXIndex ) )
	{
		//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
		bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_BypassFX0 + in_uFXIndex, NULL ) != 0;
	}
	else if( m_pGlobalSIS )
	{
		bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> in_uFXIndex ) & 1;
	}

	return bIsBypass;
}

bool CAkBus::GetBypassAllFX( CAkRegisteredObj * )
{
	bool bIsBypass = m_pFXChunk && ( ( m_pFXChunk->bitsMainFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1) ;

	//We Use RTPC VALUE PRIOR TO ACTIONS!
	if( m_RTPCBitArray.IsSet( RTPC_BypassAllFX ) )
	{
		//Getting RTPC for AK_INVALID_GAME_OBJECT since we are in a Bus.
		bIsBypass = g_pRTPCMgr->GetRTPCConvertedValue( reinterpret_cast<IAkRTPCSubscriber*>(this), RTPC_BypassAllFX, NULL ) != 0;
	}
	else if( m_pGlobalSIS )
	{
		bIsBypass = ( m_pGlobalSIS->m_bitsFXBypass >> AK_NUM_EFFECTS_BYPASS_ALL_FLAG ) & 1;
	}

	return bIsBypass;
}

void CAkBus::ResetFXBypass(
	AkUInt32 in_bitsFXBypass,
	AkUInt32 in_uTargetMask /* = 0xFFFFFFFF */ ) 
{
	if( m_pGlobalSIS )
	{
		m_pGlobalSIS->m_bitsFXBypass = (AkUInt8) ( ( m_pGlobalSIS->m_bitsFXBypass & ~in_uTargetMask ) | in_bitsFXBypass );
	}
}

void CAkBus::SetAkProp(
	AkPropID in_eProp, 
	CAkRegisteredObj * in_pGameObj,
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
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_PitchChanged, ID(), true, AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_fTargetValue != m_props.GetAkProp( AkPropID_Pitch, 0.0f ).fValue ) ) ) 
			{
				MONITOR_PARAMCHANGED(AkMonitorData::NotificationReason_PitchChanged, ID(), true, AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	case AkPropID_Volume:
		{
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_VolumeChanged, ID(), true, AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && in_fTargetValue != m_props.GetAkProp( AkPropID_Volume, 0.0f ).fValue ) )
			{
				MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_VolumeChanged, ID(), true, AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	case AkPropID_LPF:
		{
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_LPFChanged, ID(), true, AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && ( in_fTargetValue != m_props.GetAkProp( AkPropID_LPF, 0.0f ).fValue ) ) ) 
			{
				MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_LPFChanged, ID(), true, AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	case AkPropID_BusVolume:
		{
			MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_VolumeChanged, ID(), true, AK_INVALID_GAME_OBJECT , in_fTargetValue, in_eValueMeaning, in_lTransitionTime );

			if(    ( in_eValueMeaning == AkValueMeaning_Offset && in_fTargetValue != 0 )
				|| ( in_eValueMeaning == AkValueMeaning_Independent && in_fTargetValue != m_props.GetAkProp( AkPropID_BusVolume, 0.0f ).fValue ) )
			{
				MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_VolumeChanged, ID(), true, AK_INVALID_GAME_OBJECT );
			}
		}
		break;
	default:
		AKASSERT( false );
	}
#endif

	CAkSIS * pSIS = GetSIS();
	if ( pSIS )
		StartSISTransition( pSIS, in_eProp, in_fTargetValue, in_eValueMeaning, in_eFadeCurve, in_lTransitionTime );
}

// Reset a runtime property value (SIS)
void CAkBus::ResetAkProp(
	AkPropID in_eProp, 
	AkCurveInterpolation in_eFadeCurve,
	AkTimeMs in_lTransitionTime
	)
{
	if( m_pGlobalSIS )
	{
		AkSISValue * pValue = m_pGlobalSIS->m_values.FindProp( in_eProp );
		if ( pValue && pValue->fValue != 0.0f )
		{
			SetAkProp( in_eProp, NULL, AkValueMeaning_Default, 0, in_eFadeCurve, in_lTransitionTime );
		}
	}
}

void CAkBus::Mute(
		CAkRegisteredObj *	in_pGameObj,
		AkCurveInterpolation		in_eFadeCurve /*= AkCurveInterpolation_Linear*/,
		AkTimeMs		in_lTransitionTime /*= 0*/
		)
{
	AKASSERT( g_pRegistryMgr );

	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_Muted, ID(), true, AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );
	
	MONITOR_PARAMCHANGED( AkMonitorData::NotificationReason_Muted, ID(), true, AK_INVALID_GAME_OBJECT );

	MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Muted, 0, 0 );

	if( in_pGameObj == NULL )
	{
		CAkSIS* pSIS = GetSIS();
		if ( pSIS )
			StartSisMuteTransitions( pSIS, AK_MUTED_RATIO, in_eFadeCurve, in_lTransitionTime );
	}
}

void CAkBus::Unmute( CAkRegisteredObj * in_pGameObj, AkCurveInterpolation in_eFadeCurve, AkTimeMs in_lTransitionTime )
{
	AKASSERT(g_pRegistryMgr);

	MONITOR_SETPARAMNOTIF_FLOAT( AkMonitorData::NotificationReason_Unmuted, ID(), true, AK_INVALID_GAME_OBJECT, 0, AkValueMeaning_Default, in_lTransitionTime );

	MONITOR_BUSNOTIFICATION( ID(), AkMonitorData::BusNotification_Unmuted, 0, 0 );

	if( in_pGameObj == NULL && m_pGlobalSIS )
	{
		AkSISValue * pValue = m_pGlobalSIS->m_values.FindProp( AkPropID_MuteRatio );
		if ( pValue && pValue->fValue != AK_UNMUTED_RATIO )
		{
			g_pRegistryMgr->SetNodeIDAsModified(this);
			StartSisMuteTransitions( m_pGlobalSIS, AK_UNMUTED_RATIO, in_eFadeCurve, in_lTransitionTime );
		}
	}
}

void CAkBus::UnmuteAll(AkCurveInterpolation in_eFadeCurve,AkTimeMs in_lTransitionTime)
{
	Unmute(NULL,in_eFadeCurve,in_lTransitionTime);
}

CAkSIS* CAkBus::GetSIS( CAkRegisteredObj * )
{
	AKASSERT( g_pRegistryMgr );
	g_pRegistryMgr->SetNodeIDAsModified(this);
	if(!m_pGlobalSIS)
	{
		AkUInt8 bitsBypass = 0;
		for ( AkUInt32 uFXIndex = 0; uFXIndex < AK_NUM_EFFECTS_PER_OBJ; ++uFXIndex )
			bitsBypass |= ( GetBypassFX( uFXIndex ) ? 1 : 0 ) << uFXIndex;
		m_pGlobalSIS = AkNew( g_DefaultPoolId, CAkSIS( this, bitsBypass ) );
	}

	return m_pGlobalSIS;
}

void CAkBus::RecalcNotification()
{
	if( IsMixingBus() )
	{
		AkReal32 fVolume = GetBusEffectiveVolume( BusVolumeType_ToNextBusWithEffect, AkPropID_BusVolume );
		
		m_bHdrReleaseTimeDirty = true;

		if ( ParentBus() == NULL )	//No parent?  Must be a master bus.
		{
			CAkLEngine::ResetMasterBusVolume( m_bMainOutputHierarchy, fVolume );
		}
		else
		{
			CAkLEngine::ResetBusVolume( ID(), fVolume );
		}
	}

	ResetControlBusVolume();

	if( IsActivityChunkEnabled() )
	{
		AkChildArray& rArray = m_pActivityChunk->GetActiveChildren();
		for( AkChildArray::Iterator iter = rArray.Begin(); iter != rArray.End(); ++iter )
		{
			if( (*iter)->IsActiveOrPlaying() )
			{
				(*iter)->RecalcNotification();
			}
		}
	}
}

CAkRTPCMgr::SubscriberType CAkBus::GetRTPCSubscriberType() const
{
	return CAkRTPCMgr::SubscriberType_CAkBus;
}

CAkBus * CAkBus::GetPrimaryMasterBusAndAddRef()
{
	AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
	CAkBus* pMasterBus = GetPrimaryMasterBus();
	if (pMasterBus != NULL)
		pMasterBus->AddRef();

	return pMasterBus;
}

CAkBus * CAkBus::GetSecondaryMasterBusAndAddRef()
{
	AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
	CAkBus* pMasterBus = GetSecondaryMasterBus();
	if (pMasterBus != NULL)
		pMasterBus->AddRef();

	return pMasterBus;
}

void CAkBus::ClearMasterBus()
{
	AkAutoLock<CAkLock> lock(g_pIndex->GetNodeLock( AkNodeType_Bus ));
	g_MasterBusCtx.SetBus( NULL );
	g_SecondaryMasterBusCtx.SetBus(NULL);
}

void CAkBus::UpdateVoiceVolumes()
{
	if( !IsMixingBus() )
		m_fEffectiveBusVolume = GetBusEffectiveVolume( BusVolumeType_ToNextBusWithEffect, AkPropID_BusVolume );
	else
		m_fEffectiveBusVolume = 0.f;

	m_fEffectiveVoiceVolume = GetBusEffectiveVolume( BusVolumeType_IncludeEntireBusTree, AkPropID_Volume );

	m_bControBusVolumeUpdated = true;
}

// Lower engine calls this when HDR window top has been calculated.
void CAkBus::NotifyHdrWindowTop( AkReal32 in_fWindowTop )
{
	// Set game param value.
	AkUniqueID gameParamID = (AkUniqueID)m_props.GetAkProp( AkPropID_HDRBusGameParam, (AkInt32)AK_INVALID_UNIQUE_ID ).iValue;
	if ( gameParamID != AK_INVALID_UNIQUE_ID )
	{
		TransParams transParams;
		transParams.TransitionTime = 0;
		transParams.eFadeCurve = AkCurveInterpolation_Linear;

		AkReal32 fMin = m_props.GetAkProp( AkPropID_HDRBusGameParamMin, g_AkPropDefault[AkPropID_HDRBusGameParamMin]).fValue;
		AkReal32 fMax = m_props.GetAkProp( AkPropID_HDRBusGameParamMax, g_AkPropDefault[AkPropID_HDRBusGameParamMax]).fValue;
		in_fWindowTop = AkClamp( in_fWindowTop, fMin, fMax );

		g_pRTPCMgr->SetRTPCInternal( gameParamID, in_fWindowTop, NULL, transParams );
#ifdef WWISE_AUTHORING
		CAkRegisteredObj * pTransportGameObj = g_pRegistryMgr->GetObjAndAddref( 0 ); // GameObjects::GO_Transport
		if ( pTransportGameObj )
		{
			g_pRTPCMgr->SetRTPCInternal( gameParamID, in_fWindowTop, pTransportGameObj, transParams );
			pTransportGameObj->Release();
		}
#endif
	}
}

#ifndef AK_OPTIMIZED
void CAkBus::MonitoringSolo( bool in_bSolo )
{
	CAkParameterNodeBase::_MonitoringSolo( in_bSolo, g_uSoloCount_bus );
}

void CAkBus::MonitoringMute( bool in_bMute )
{
	CAkParameterNodeBase::_MonitoringMute( in_bMute, g_uMuteCount_bus );
}

void CAkBus::GetMonitoringMuteSoloState( 
	bool /*in_bCheckBus*/,	// Pass true. When an overridden bus is found, it is set to false.
	bool & io_bSolo,	// Pass false. Bit is OR'ed against each node of the signal flow.
	bool & io_bMute		// Pass false. Bit is OR'ed against each node of the signal flow.
	)
{
	io_bSolo = io_bSolo || m_bIsSoloed;
	io_bMute = io_bMute || m_bIsMuted;
	if ( io_bMute )	// Mute wins. Bail out.
		return;
	
	if ( m_pBusOutputNode )
	{
		// Yes. Check master-mixer hierarchy.
		m_pBusOutputNode->GetMonitoringMuteSoloState( false, io_bSolo, io_bMute );
	}
}

// Walk through hierarchy to determine status of monitoring mute.
void CAkBus::RefreshMonitoringMute()	// True if at least one node is soloed in the hierarchy.
{
	if ( !CAkBus::IsMonitoringMuteSoloActive_bus() )
		m_bIsMonitoringMute = false;
	else
	{
		bool bSolo = false;
		bool bMute = false;
		GetMonitoringMuteSoloState( false, bSolo, bMute );
		
		m_bIsMonitoringMute = ( bMute || ( CAkBus::IsMonitoringSoloActive_bus() && !bSolo ) );
	}
}

#endif

#if defined( AK_XBOX360 ) || defined( AK_PS3 )

CAkBus* CAkBus::m_pBackgroundMusicBus = NULL;
CAkLock CAkBus::m_BackgroundMusicLock;

void CAkBus::MuteBackgroundMusic()
{
	AkAutoLock<CAkLock> gate( m_BackgroundMusicLock );
	if( m_pBackgroundMusicBus )
	{
		m_pBackgroundMusicBus->BackgroundMusic_Mute();
	}
}
void CAkBus::UnmuteBackgroundMusic()
{
	AkAutoLock<CAkLock> gate( m_BackgroundMusicLock );
	if( m_pBackgroundMusicBus )
	{
		m_pBackgroundMusicBus->BackgroundMusic_Unmute();
	}
}

void CAkBus::BackgroundMusic_Mute()
{
	m_bIsBackgroundMusicMuted = true;

	AkMutedMapItem item;
    item.m_bIsPersistent = false;
	item.m_bIsGlobal = true;
	item.m_Identifier = this;
	MuteNotification( AK_MUTED_RATIO, item, true ); 
}

void CAkBus::BackgroundMusic_Unmute()
{
	m_bIsBackgroundMusicMuted = false;

	AkMutedMapItem item;
    item.m_bIsPersistent = false;
	item.m_bIsGlobal = true;
	item.m_Identifier = this;

	AkReal32 fMuteRatio = AK_UNMUTED_RATIO;

	if(m_pGlobalSIS)
	{
		AkSISValue * pValue = m_pGlobalSIS->m_values.FindProp( AkPropID_MuteRatio );
		if( pValue )
			fMuteRatio = pValue->fValue;
	}

	MuteNotification( fMuteRatio, item, true );
}

void CAkBus::SetAsBackgroundMusicBus()
{
	AkAutoLock<CAkLock> gate( m_BackgroundMusicLock );

	if( !m_bIsBackgroundMusicBus )
	{
		if( m_pBackgroundMusicBus == NULL )
		{
			m_pBackgroundMusicBus = this;
			m_bIsBackgroundMusicBus = true;

#ifdef AK_XBOX360
			BOOL bHasTitleXMPControl;
			// Query the system to get the initial muted state of this bus
			XMPTitleHasPlaybackControl( &bHasTitleXMPControl );
			m_bIsBackgroundMusicMuted = !(bHasTitleXMPControl);
#endif

#ifdef AK_PS3
			m_bIsBackgroundMusicMuted = false;
			if ( g_PDSettings.bBGMEnable )
			{
				CellSysutilBgmPlaybackStatus status;
				int ret = cellSysutilGetBgmPlaybackStatus( &status );
				if ( ret == CELL_SYSUTIL_BGMPLAYBACK_OK )
				{
					// straight out of the sony doc: 
					// BGM on the application side can only be played back when playerState is CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP
					m_bIsBackgroundMusicMuted = (status.playerState != CELL_SYSUTIL_BGMPLAYBACK_STATUS_STOP); 
				}
			}
#endif

			if( m_bIsBackgroundMusicMuted )
				BackgroundMusic_Mute(); 
		}
		else
		{
			AKASSERT( !"Illegal to have more than one BGM bus" );
		}
	}
}

void CAkBus::UnsetAsBackgroundMusicBus()
{
	AkAutoLock<CAkLock> gate( m_BackgroundMusicLock );

	if( m_bIsBackgroundMusicBus )
	{
		// If this assert pops, it means somebody else registered an XMP bus
		AKASSERT( m_pBackgroundMusicBus == this );
		m_pBackgroundMusicBus = NULL;
		m_bIsBackgroundMusicMuted = false;
		m_bIsBackgroundMusicBus = false;
		BackgroundMusic_Unmute();
	}
}

#endif
