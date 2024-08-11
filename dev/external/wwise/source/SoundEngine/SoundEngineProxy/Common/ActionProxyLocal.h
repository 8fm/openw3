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
#include "IActionProxy.h"

class CAkActionPlay;
class CAkActionActive;
class CAkActionSetValue;
class CAkActionMute;
class CAkActionSetPitch;
class CAkActionSetVolume;
class CAkActionSetLPF;
class CAkActionSetState;
class CAkActionUseState;

class ActionProxyLocal : public ObjectProxyLocal
						, virtual public IActionProxy
{
public:
	ActionProxyLocal( AkActionType in_actionType, AkUniqueID in_id );

	// IActionProxy members
	virtual void SetElementID( WwiseObjectIDext in_elementID );
	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax );
	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax );
	virtual void CurveType( const AkCurveInterpolation in_eCurveType );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionExceptProxyLocal : public ActionProxyLocal
							, virtual public IActionExceptProxy
{
public:
	ActionExceptProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void AddException( WwiseObjectIDext in_elementID );
	virtual void RemoveException( WwiseObjectIDext in_elementID );
	virtual void ClearExceptions();
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionPauseProxyLocal : public ActionExceptProxyLocal
								, virtual public IActionPauseProxy
{
public:
	ActionPauseProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void IncludePendingResume( bool in_bIncludePendingResume );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionResumeProxyLocal : public ActionExceptProxyLocal
								, virtual public IActionResumeProxy
{
public:
	ActionResumeProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void IsMasterResume( bool in_bIsMasterResume );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBreakProxyLocal : public ActionProxyLocal
{
public:
	ActionBreakProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionMuteProxyLocal : public ActionExceptProxyLocal
								, virtual public IActionMuteProxy
{
public:
	ActionMuteProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetAkPropProxyLocal : public ActionExceptProxyLocal
								, virtual public IActionSetAkPropProxy
{
public:
	ActionSetAkPropProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void SetValue( AkReal32 in_fValue, AkValueMeaning in_eValueMeaning, AkReal32 in_fMin, AkReal32 in_fMax );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetStateProxyLocal : public ActionProxyLocal
									, virtual public IActionSetStateProxy
{
public:
	ActionSetStateProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void SetGroup( AkStateGroupID in_groupID );
	virtual void SetTargetState( AkStateID in_stateID );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetSwitchProxyLocal : public ActionProxyLocal
									, virtual public IActionSetSwitchProxy
{
public:
	ActionSetSwitchProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID );
	virtual void SetTargetSwitch( const AkSwitchStateID in_ulSwitchID );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSetGameParameterProxyLocal : public ActionExceptProxyLocal
										, virtual public IActionSetGameParameterProxy
{
public:
	ActionSetGameParameterProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}
	
	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionUseStateProxyLocal : public ActionProxyLocal
									, virtual public IActionUseStateProxy
{
public:
	ActionUseStateProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionProxyLocal( in_actionType, in_id ){}

	virtual void UseState( bool in_bUseState );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionBypassFXProxyLocal : public ActionExceptProxyLocal
									, virtual public IActionBypassFXProxy
{
public:
	ActionBypassFXProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void BypassFX( bool in_bBypassFX );
	virtual void SetBypassTarget( bool in_bBypassAllFX, AkUInt8 in_ucEffectsMask );
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class ActionSeekProxyLocal : public ActionExceptProxyLocal
							, virtual public IActionSeekProxy
{
public:
	ActionSeekProxyLocal( AkActionType in_actionType, AkUniqueID in_id ):ActionExceptProxyLocal( in_actionType, in_id ){}

	virtual void SetSeekPositionPercent( AkReal32 in_position, AkReal32 in_rangeMin = 0, AkReal32 in_rangeMax = 0 );
	virtual void SetSeekPositionTimeAbsolute( AkTimeMs in_position, AkTimeMs in_rangeMin = 0, AkTimeMs in_rangeMax = 0 );
	virtual void SetSeekToNearestMarker( bool in_bSeekToNearestMarker );
};

#endif // #ifndef PROXYCENTRAL_CONNECTED
#endif // #ifndef AK_OPTIMIZED
