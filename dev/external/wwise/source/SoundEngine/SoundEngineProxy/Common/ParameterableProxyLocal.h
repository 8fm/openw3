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

#include "ObjectProxyLocal.h"
#include "IParameterableProxy.h"

class ParameterableProxyLocal : public ObjectProxyLocal
								, virtual public IParameterableProxy
{
public:
	virtual ~ParameterableProxyLocal();

	// IParameterableProxy members
	virtual void AddChild( WwiseObjectIDext in_id );
	virtual void RemoveChild( WwiseObjectIDext in_id );

	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax );
	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax );

	virtual void PriorityApplyDistFactor( bool in_bApplyDistFactor );
	virtual void PriorityOverrideParent( bool in_bOverrideParent );

	virtual void AddStateGroup( AkStateGroupID in_stateGroupID );
	virtual void RemoveStateGroup( AkStateGroupID in_stateGroupID );
	virtual void UpdateStateGroups(AkUInt32 in_uGroups, AkStateGroupUpdate* in_pGroups, AkStateUpdate* in_pUpdates);
	virtual void AddState( AkStateGroupID in_stateGroupID, AkUniqueID in_stateInstanceID, AkStateID in_stateID );
	virtual void RemoveState( AkStateGroupID in_stateGroupID, AkStateID in_stateID );
	virtual void UseState( bool in_bUseState );
	virtual void SetStateSyncType( AkStateGroupID in_StateGroupID, AkUInt32/*AkSyncType*/ in_eSyncType );

	virtual void SetFX( AkUInt32 in_uFXIndex, AkUniqueID in_uID, bool in_bShareSet );
	virtual void RemoveFX( AkUInt32 in_uFXIndex );
	virtual void UpdateEffects( AkUInt32 in_uCount, AkEffectUpdate* in_pUpdates );

	virtual void BypassAllFX( bool in_bBypass );
	virtual void BypassFX( AkUInt32 in_uFXIndex, bool in_bBypass );

	virtual void SetRTPC( AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion = NULL, AkUInt32 in_ulConversionArraySize = 0 );
	virtual void UnsetRTPC( AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID );

	virtual void MonitoringSolo( bool in_bSolo );
	virtual void MonitoringMute( bool in_bMute );

	virtual void PosSetPositioningType( bool in_bOverride, bool in_bRTPC, AkPannerType in_ePanner, AkPositionSourceType in_ePosSource );
	virtual void PosSetPannerEnabled( bool in_bIsPannerEnabled );
	virtual void SetPositioningEnabled( bool in_bIsPosEnabled );
	
};

#endif
#endif // #ifndef AK_OPTIMIZED
