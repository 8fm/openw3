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

#include "ParameterNodeProxyLocal.h"
#include "AkParameterNode.h"
#include "AkCritical.h"

void ParameterNodeProxyLocal::PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetSpatializationEnabled( in_bIsSpatializationEnabled );
	}
}

void ParameterNodeProxyLocal::PosSetAttenuationID( AkUniqueID in_AttenuationID )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetAttenuationID( in_AttenuationID );
	}
}

void ParameterNodeProxyLocal::PosSetIsPositionDynamic( bool in_bIsDynamic )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetIsPositionDynamic( in_bIsDynamic );
	}
}

void ParameterNodeProxyLocal::PosSetFollowOrientation( bool in_bFollow )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetFollowOrientation( in_bFollow );
	}
}

void ParameterNodeProxyLocal::PosSetPathMode( AkPathMode in_ePathMode )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetPathMode( in_ePathMode );
	}
}

void ParameterNodeProxyLocal::PosSetIsLooping( bool in_bIsLooping )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetIsLooping( in_bIsLooping );
	}
}

void ParameterNodeProxyLocal::PosSetTransition( AkTimeMs in_TransitionTime )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetTransition( in_TransitionTime );
	}
}

void ParameterNodeProxyLocal::PosSetPath(
	AkPathVertex*           in_pArayVertex, 
	AkUInt32                 in_ulNumVertices, 
	AkPathListItemOffset*   in_pArrayPlaylist, 
	AkUInt32                 in_ulNumPlaylistItem 
	)
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // WG-4418 eliminate potential problems
		pIndexable->PosSetPath( in_pArayVertex, in_ulNumVertices, in_pArrayPlaylist, in_ulNumPlaylistItem );
	}
}

void ParameterNodeProxyLocal::PosUpdatePathPoint(
	AkUInt32 in_ulPathIndex,
	AkUInt32 in_ulVertexIndex,
	const AkVector& in_ptPosition,
	AkTimeMs in_DelayToNext
	)
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical; // WG-4418 eliminate potential problems

		pIndexable->PosUpdatePathPoint( in_ulPathIndex, in_ulVertexIndex, in_ptPosition, in_DelayToNext );
	}
}

void ParameterNodeProxyLocal::PosSetPathRange(AkUInt32 in_ulPathIndex, AkReal32 in_fXRange, AkReal32 in_fYRange)
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->PosSetPathRange( in_ulPathIndex, in_fXRange, in_fYRange);
	}
}

void ParameterNodeProxyLocal::OverrideFXParent( bool in_bIsFXOverrideParent )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->OverrideFXParent( in_bIsFXOverrideParent );
	}
}

void ParameterNodeProxyLocal::SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetBelowThresholdBehavior( in_eBelowThresholdBehavior );
	}
}

void ParameterNodeProxyLocal::SetMaxNumInstancesOverrideParent( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetMaxNumInstOverrideParent( in_bOverride ); 
	}
}

void ParameterNodeProxyLocal::SetVVoicesOptOverrideParent( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetVVoicesOptOverrideParent( in_bOverride );
	}
}

void ParameterNodeProxyLocal::SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetMaxNumInstances( in_u16MaxNumInstance );
	}
}

void ParameterNodeProxyLocal::SetIsGlobalLimit( bool in_bIsGlobalLimit )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetIsGlobalLimit( in_bIsGlobalLimit );
	}
}

void ParameterNodeProxyLocal::SetMaxReachedBehavior( bool in_bKillNewest )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetMaxReachedBehavior( in_bKillNewest );
	}
}

void ParameterNodeProxyLocal::SetOverLimitBehavior( bool in_bUseVirtualBehavior )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetOverLimitBehavior( in_bUseVirtualBehavior );
	}
}

void ParameterNodeProxyLocal::SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetVirtualQueueBehavior( in_eBehavior );
	}
}

void ParameterNodeProxyLocal::SetAuxBusSend( AkUniqueID in_AuxBusID, AkUInt32 in_ulIndex )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetAuxBusSend( in_AuxBusID, in_ulIndex );
	}
}

void ParameterNodeProxyLocal::SetOverrideGameAuxSends( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetOverrideGameAuxSends( in_bOverride );
	}
}

void ParameterNodeProxyLocal::SetUseGameAuxSends( bool in_bUse )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetUseGameAuxSends( in_bUse );
	}
}

void ParameterNodeProxyLocal::SetOverrideUserAuxSends( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetOverrideUserAuxSends( in_bOverride );
	}
}

void ParameterNodeProxyLocal::SetOverrideHdrEnvelope( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetOverrideHdrEnvelope( in_bOverride );
	}
}

void ParameterNodeProxyLocal::SetOverrideAnalysis( bool in_bOverride )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetOverrideAnalysis( in_bOverride );
	}
}

void ParameterNodeProxyLocal::SetNormalizeLoudness( bool in_bNormalizeLoudness )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetNormalizeLoudness( in_bNormalizeLoudness );
	}
}

void ParameterNodeProxyLocal::SetEnableEnvelope( bool in_bEnableEnvelope )
{
	CAkParameterNode* pIndexable = static_cast<CAkParameterNode*>( GetIndexable() );
	if( pIndexable )
	{
		return pIndexable->SetEnableEnvelope( in_bEnableEnvelope );
	}
}

#endif
#endif // #ifndef AK_OPTIMIZED
