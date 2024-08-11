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
#ifndef PROXYCENTRAL_CONNECTED

#include "ParameterableProxyLocal.h"
#include "IParameterNodeProxy.h"

class CAkParameterNode;

class ParameterNodeProxyLocal : public ParameterableProxyLocal
								, virtual public IParameterNodeProxy
{
public:
	// IParameterableProxy members
	virtual void PosSetSpatializationEnabled( bool in_bSpatializationEnabled );
	virtual void PosSetAttenuationID( AkUniqueID in_AttenuationID );

	virtual void PosSetIsPositionDynamic( bool in_bIsDynamic );
	virtual void PosSetFollowOrientation( bool in_bFollow );

	virtual void PosSetPathMode( AkPathMode in_ePathMode );
	virtual void PosSetIsLooping( bool in_bIsLooping );
	virtual void PosSetTransition( AkTimeMs in_TransitionTime );

	virtual void PosSetPath(
		AkPathVertex*           in_pArayVertex, 
		AkUInt32                 in_ulNumVertices, 
		AkPathListItemOffset*   in_pArrayPlaylist, 
		AkUInt32                 in_ulNumPlaylistItem 
		);

	virtual void PosUpdatePathPoint(
		AkUInt32 in_ulPathIndex,
		AkUInt32 in_ulVertexIndex,
		const AkVector& in_ptPosition,
		AkTimeMs in_DelayToNext
		);

	virtual void PosSetPathRange(AkUInt32 in_ulPathIndex, AkReal32 in_fXRange, AkReal32 in_fYRange);

	virtual void OverrideFXParent( bool in_bIsFXOverrideParent );

	virtual void SetBelowThresholdBehavior( AkBelowThresholdBehavior in_eBelowThresholdBehavior );
	virtual void SetMaxNumInstancesOverrideParent( bool in_bOverride );
	virtual void SetVVoicesOptOverrideParent( bool in_bOverride );
	virtual void SetMaxNumInstances( AkUInt16 in_u16MaxNumInstance );
	virtual void SetIsGlobalLimit( bool in_bIsGlobalLimit );
	virtual void SetMaxReachedBehavior( bool in_bKillNewest );
	virtual void SetOverLimitBehavior( bool in_bUseVirtualBehavior );
	virtual void SetVirtualQueueBehavior( AkVirtualQueueBehavior in_eBehavior );
	
	virtual void SetAuxBusSend( AkUniqueID in_AuxBusID, AkUInt32 in_ulIndex );
	virtual void SetOverrideGameAuxSends( bool in_bOverride );
	virtual void SetUseGameAuxSends( bool in_bUse );
	virtual void SetOverrideUserAuxSends( bool in_bOverride );
	
	virtual void SetOverrideHdrEnvelope( bool in_bOverride );
	virtual void SetOverrideAnalysis( bool in_bOverride );
	virtual void SetNormalizeLoudness( bool in_bNormalizeLoudness );
	virtual void SetEnableEnvelope( bool in_bEnableEnvelope );
};
#endif
#endif // #ifndef AK_OPTIMIZED
