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
#ifndef PROXYCENTRAL_CONNECTED



#include "ParameterableProxyLocal.h"
#include "AkParameterNodeBase.h"
#include "AkCritical.h"
#include "AkAudioLib.h"
#include "AkCommon.h"
#include "AkURenderer.h"

ParameterableProxyLocal::~ParameterableProxyLocal()
{
}

void ParameterableProxyLocal::AddChild( WwiseObjectIDext in_id )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddChild( in_id );
	}
}

void ParameterableProxyLocal::RemoveChild( WwiseObjectIDext in_id )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveChild( in_id );
	}
}

void ParameterableProxyLocal::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetAkProp( in_eProp, in_fValue, in_fMin, in_fMax );
	}
}

void ParameterableProxyLocal::SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetAkProp( in_eProp, in_iValue, in_iMin, in_iMax );
	}
}

void ParameterableProxyLocal::PriorityApplyDistFactor( bool in_bApplyDistFactor )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetPriorityApplyDistFactor( in_bApplyDistFactor );
	}
}

void ParameterableProxyLocal::PriorityOverrideParent( bool in_bOverrideParent )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetPriorityOverrideParent( in_bOverrideParent );
	}
}

void ParameterableProxyLocal::AddStateGroup( AkStateGroupID in_stateGroupID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddStateGroup( in_stateGroupID );
	}
}

void ParameterableProxyLocal::RemoveStateGroup( AkStateGroupID in_stateGroupID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveStateGroup( in_stateGroupID );
	}
}

void ParameterableProxyLocal::UpdateStateGroups(AkUInt32 in_uGroups, AkStateGroupUpdate* in_pGroups, AkStateUpdate* in_pUpdates)
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->UpdateStateGroups( in_uGroups, in_pGroups, in_pUpdates);
	}
}

void ParameterableProxyLocal::AddState( AkStateGroupID in_stateGroupID, AkUniqueID in_stateInstanceID, AkStateID in_stateID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->AddState( in_stateGroupID, in_stateInstanceID, in_stateID );
	}
}

void ParameterableProxyLocal::RemoveState( AkStateGroupID in_stateGroupID, AkStateID in_stateID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveState( in_stateGroupID, in_stateID );
	}
}

void ParameterableProxyLocal::UseState( bool in_bUseState )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->UseState( in_bUseState );
	}
}

void ParameterableProxyLocal::SetStateSyncType( AkStateGroupID in_StateGroupID, AkUInt32/*AkSyncType*/ in_eSyncType )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetStateSyncType( in_StateGroupID, in_eSyncType );
	}
}

void ParameterableProxyLocal::SetFX( AkUInt32 in_uFXIndex, AkUniqueID in_uID, bool in_bShareSet )
{
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // AK_WII

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetFX( in_uFXIndex, in_uID, in_bShareSet );
	}
}


void ParameterableProxyLocal::BypassAllFX( bool in_bBypass )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->MainBypassFX( ( in_bBypass ? 1 : 0 ) << AK_NUM_EFFECTS_BYPASS_ALL_FLAG, 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG );
	}
}

void ParameterableProxyLocal::BypassFX( AkUInt32 in_uFXIndex, bool in_bBypass )
{
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // AK_WII

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->MainBypassFX( ( in_bBypass ? 1 : 0 ) << in_uFXIndex, 1 << in_uFXIndex );
	}
}

void ParameterableProxyLocal::RemoveFX( AkUInt32 in_uFXIndex )
{
#if defined(AK_WII_FAMILY_HW) || defined(AK_3DS)
	if( in_uFXIndex >= AK_NUM_EFFECTS_PER_OBJ )
		return;
#endif // AK_WII

	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->RemoveFX( in_uFXIndex );
	}
}

void ParameterableProxyLocal::UpdateEffects( AkUInt32 in_uCount, AkEffectUpdate* in_pUpdates )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->UpdateEffects( in_uCount, in_pUpdates );
	}
}

void ParameterableProxyLocal::SetRTPC( AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion /*= NULL*/, AkUInt32 in_ulConversionArraySize /*= 0*/ )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->SetRTPC( in_RTPC_ID, in_ParamID, in_RTPCCurveID, in_eScaling, in_pArrayConversion, in_ulConversionArraySize );
	}
}

void ParameterableProxyLocal::UnsetRTPC( AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->UnsetRTPC( in_ParamID, in_RTPCCurveID );
	}
}

void ParameterableProxyLocal::MonitoringSolo( bool in_bSolo )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->MonitoringSolo( in_bSolo );
	}
}

void ParameterableProxyLocal::MonitoringMute( bool in_bMute )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;

		pIndexable->MonitoringMute( in_bMute );
	}
}

void ParameterableProxyLocal::PosSetPositioningType( bool in_bOverride, bool in_bRTPC, AkPannerType in_ePanner, AkPositionSourceType in_ePosSource )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetPositioningType( in_bOverride, in_bRTPC, in_ePanner, in_ePosSource );
	}
}

void ParameterableProxyLocal::PosSetPannerEnabled( bool in_bIsPannerEnabled )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetPannerEnabled( in_bIsPannerEnabled );
	}
}

void ParameterableProxyLocal::SetPositioningEnabled( bool in_bIsPositioningEnabled )
{
	CAkParameterNodeBase* pIndexable = static_cast<CAkParameterNodeBase*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetPositioningEnabled( in_bIsPositioningEnabled );
	}
}

#endif
#endif
