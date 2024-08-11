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

#include "IParameterableProxy.h"
#include "AkSharedEnum.h"
#include "AkParameters.h"

struct AkPathVertex;
struct AkPathListItemOffset;

class IParameterNodeProxy : virtual public IParameterableProxy
{
	DECLARE_BASECLASS( IParameterableProxy );
public:
	virtual void PosSetSpatializationEnabled( bool in_bIsSpatializationEnabled ) = 0;
	virtual void PosSetAttenuationID( AkUniqueID in_AttenuationID ) = 0;

	virtual void PosSetIsPositionDynamic( bool in_bIsDynamic ) = 0;
	virtual void PosSetFollowOrientation( bool in_bFollow ) = 0;

	virtual void PosSetPathMode( AkPathMode in_ePathMode ) = 0;
	virtual void PosSetIsLooping( bool in_bIsLooping ) = 0;
	virtual void PosSetTransition( AkTimeMs in_TransitionTime ) = 0;

	virtual void PosSetPath(
		AkPathVertex*           in_pArayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		) = 0;

	virtual void PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		const AkVector& in_ptPosition,
		AkTimeMs in_DelayToNext
		) = 0;

	virtual void PosSetPathRange(AkUInt32 in_ulPathIndex, AkReal32 in_fXRange, AkReal32 in_fYRange) = 0;

	virtual void OverrideFXParent( bool in_bIsFXOverrideParent ) = 0;

	virtual void SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior ) = 0;
	virtual void SetMaxNumInstancesOverrideParent( bool in_bOverride ) = 0;
	virtual void SetVVoicesOptOverrideParent( bool in_bOverride ) = 0;
	virtual void SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance ) = 0;
	virtual void SetIsGlobalLimit( bool in_bIsGlobalLimit ) = 0;
	virtual void SetMaxReachedBehavior( bool in_bKillNewest ) = 0;
	virtual void SetOverLimitBehavior( bool in_bUseVirtualBehavior ) = 0;
	virtual void SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior ) = 0;
	
	virtual void SetAuxBusSend( AkUniqueID in_AuxBusID, AkUInt32 in_ulIndex ) = 0;
	virtual void SetOverrideGameAuxSends( bool in_bOverride ) = 0;
	virtual void SetUseGameAuxSends( bool in_bUse ) = 0;
	virtual void SetOverrideUserAuxSends( bool in_bOverride ) = 0;

	virtual void SetOverrideHdrEnvelope( bool in_bOverride ) = 0;
	virtual void SetOverrideAnalysis( bool in_bOverride ) = 0;
	virtual void SetNormalizeLoudness( bool in_bNormalizeLoudness ) = 0;
	virtual void SetEnableEnvelope( bool in_bEnableEnvelope ) = 0;
	
	enum MethodIDs
	{
		MethodPosSetSpatializationEnabled = __base::LastMethodID,
		MethodPosSetAttenuationID,

		MethodPosSetIsPositionDynamic,
		MethodPosSetFollowOrientation,

		MethodPosSetPathMode,
		MethodPosSetIsLooping,
		MethodPosSetTransition,

		MethodPosSetPath,
		MethodPosUpdatePathPoint,
		MethodPosSetPathRange,

		MethodOverrideFXParent,

		MethodSetBelowThresholdBehavior,
		MethodSetMaxNumInstancesOverrideParent,
		MethodSetVVoicesOptOverrideParent,
		MethodSetMaxNumInstances,
		MethodSetIsGlobalLimit,
		MethodSetMaxReachedBehavior,
		MethodSetOverLimitBehavior,
		MethodSetVirtualQueueBehavior,
		
		MethodSetAuxBusSend,
		MethodSetOverrideGameAuxSends,
		MethodSetUseGameAuxSends,
		MethodSetOverrideUserAuxSends,

		MethodSetOverrideHdrEnvelope,
		MethodSetOverrideAnalysis,
		MethodSetNormalizeLoudness,
		MethodSetEnableEnvelope,

		LastMethodID
	};
};
#endif // #ifndef AK_OPTIMIZED
