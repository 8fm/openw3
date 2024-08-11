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

#include "IObjectProxy.h"

#include "AkParameters.h"
#include "AkRTPC.h"
#include "AkParameterNodeBase.h"

class IParameterableProxy : virtual public IObjectProxy
{
	DECLARE_BASECLASS( IObjectProxy );
public:
	virtual void AddChild( WwiseObjectIDext in_id ) = 0;
	virtual void RemoveChild( WwiseObjectIDext in_id ) = 0;

	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax ) = 0;
	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax ) = 0;

	virtual void PriorityApplyDistFactor( bool in_bApplyDistFactor ) = 0;
	virtual void PriorityOverrideParent( bool in_bOverrideParent ) = 0;

	virtual void AddStateGroup( AkStateGroupID in_stateGroupID ) = 0;
	virtual void RemoveStateGroup( AkStateGroupID in_stateGroupID ) = 0;
	virtual void UpdateStateGroups(AkUInt32 in_uGroups, AkStateGroupUpdate* in_pGroups, AkStateUpdate* in_pUpdates) = 0;	
	virtual void AddState( AkStateGroupID in_stateGroupID, AkUniqueID in_stateInstanceID, AkStateID in_stateID ) = 0;
	virtual void RemoveState( AkStateGroupID in_stateGroupID, AkStateID in_stateID ) = 0;
	virtual void UseState( bool in_bUseState ) = 0;
	virtual void SetStateSyncType( AkStateGroupID in_StateGroupID, AkUInt32/*AkSyncType*/ in_eSyncType ) = 0;
	

	virtual void SetFX( AkUInt32 in_uFXIndex, AkUniqueID in_uID, bool in_bShareSet ) = 0;
	virtual void RemoveFX( AkUInt32 in_uFXIndex ) = 0;
	virtual void UpdateEffects( AkUInt32 in_uCount, AkEffectUpdate* in_pUpdates ) = 0;

	virtual void BypassAllFX( bool in_bBypass ) = 0;
	virtual void BypassFX( AkUInt32 in_uFXIndex, bool in_bBypass ) = 0;

	virtual void SetRTPC( AkRtpcID in_RTPC_ID, AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID, AkCurveScaling in_eScaling, AkRTPCGraphPoint* in_pArrayConversion = NULL, AkUInt32 in_ulConversionArraySize = 0 ) = 0;
	virtual void UnsetRTPC( AkRTPC_ParameterID in_ParamID, AkUniqueID in_RTPCCurveID ) = 0;

	virtual void MonitoringSolo( bool in_bSolo ) = 0;
	virtual void MonitoringMute( bool in_bMute ) = 0;

	virtual void PosSetPositioningType( bool in_bOverride, bool in_bRTPC, AkPannerType in_ePanner, AkPositionSourceType in_ePosSource ) = 0;
	virtual void PosSetPannerEnabled( bool in_bIsPannerEnabled ) = 0;

	virtual void SetPositioningEnabled( bool in_bIsPosEnabled ) = 0;

	enum MethodIDs
	{
		MethodAddChild  = __base::LastMethodID,
		MethodRemoveChild,
		MethodSetAkPropF,
		MethodSetAkPropI,
		MethodPriorityApplyDistFactor,
		MethodPriorityOverrideParent,
		MethodAddStateGroup,
		MethodRemoveStateGroup,
		MethodUpdateStateGroups,
		MethodAddState,
		MethodRemoveState,
		MethodUseState,
		MethodLinkStateToStateDefault,
		MethodSetStateSyncType,

		MethodSetFX,
		MethodBypassAllFX,
		MethodBypassFX,
		MethodRemoveFX,
		MethodUpdateEffects,

		MethodSetRTPC,
		MethodUnsetRTPC,

		MethodMonitoringSolo,
		MethodMonitoringMute,

		MethodPosSetPositioningType,
		MethodPosSetPannerEnabled,
		MethodSetPositioningEnabled,

		LastMethodID
	};
};
#endif // #ifndef AK_OPTIMIZED
