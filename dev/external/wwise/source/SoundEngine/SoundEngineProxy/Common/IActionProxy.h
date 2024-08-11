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

#include "AkActions.h"

class IActionProxy : virtual public IObjectProxy
{
	DECLARE_BASECLASS( IObjectProxy );
public:
	virtual void SetElementID( WwiseObjectIDext in_elementID ) = 0;
	virtual void SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax ) = 0;
	virtual void SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax ) = 0;
	virtual void CurveType( const AkCurveInterpolation in_eCurveType ) = 0;

	enum MethodIDs
	{
		MethodSetElementID = __base::LastMethodID,
		MethodSetAkPropF,
		MethodSetAkPropI,
		MethodCurveType,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionExceptProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void AddException( WwiseObjectIDext in_elementID ) = 0;
	virtual void RemoveException( WwiseObjectIDext in_elementID ) = 0;
	virtual void ClearExceptions() = 0;

	enum MethodIDs
	{
		MethodAddException = __base::LastMethodID,
		MethodRemoveException,
		MethodClearExceptions,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionPauseProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void IncludePendingResume( bool in_bIncludePendingResume ) = 0;

	enum MethodIDs
	{
		MethodIncludePendingResume = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionResumeProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void IsMasterResume( bool in_bIsMasterResume ) = 0;

	enum MethodIDs
	{
		MethodIsMasterResume = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionMuteProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:

	enum MethodIDs
	{
		LastMethodID = __base::LastMethodID
	};
};

class IActionSetAkPropProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void SetValue( AkReal32 in_fValue, AkValueMeaning in_eValueMeaning, AkReal32 in_fMin = 0, AkReal32 in_fMax = 0 ) = 0;

	enum MethodIDs
	{
		MethodSetValue = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetStateProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void SetGroup( AkStateGroupID in_groupID ) = 0;
	virtual void SetTargetState( AkStateID in_stateID ) = 0;

	enum MethodIDs
	{
		MethodSetGroup = __base::LastMethodID,
		MethodSetTargetState,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetSwitchProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID ) = 0;
	virtual void SetTargetSwitch( const AkSwitchStateID in_ulSwitchID ) = 0;

	enum MethodIDs
	{
		MethodSetSwitchGroup = __base::LastMethodID,
		MethodSetTargetSwitch,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSetGameParameterProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax ) = 0;

	enum MethodIDs
	{
		MethodSetValue = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionUseStateProxy : virtual public IActionProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void UseState( bool in_bUseState ) = 0;

	enum MethodIDs
	{
		MethodUseState = __base::LastMethodID,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionBypassFXProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionExceptProxy );
public:
	virtual void BypassFX( bool in_bBypassFX ) = 0;
	virtual void SetBypassTarget( bool in_bBypassAllFX, AkUInt8 in_ucEffectsMask ) = 0;

	enum MethodIDs
	{
		MethodBypassFX = __base::LastMethodID,
		MethodSetBypassTarget,

		LastMethodID
	};
};

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

class IActionSeekProxy : virtual public IActionExceptProxy
{
	DECLARE_BASECLASS( IActionProxy );
public:
	virtual void SetSeekPositionPercent( AkReal32 in_position, // Position expressed as percentage of sound [0,1].
		AkReal32 in_rangeMin = 0, AkReal32 in_rangeMax = 0 ) = 0;
	virtual void SetSeekPositionTimeAbsolute( AkTimeMs in_position, // Position in milliseconds.
		AkTimeMs in_rangeMin = 0, AkTimeMs in_rangeMax = 0 ) = 0;
	virtual void SetSeekToNearestMarker( bool in_bSeekToNearestMarker ) = 0;

	enum MethodIDs
	{
		MethodSetSeekPositionPercent = __base::LastMethodID,
		MethodSetSeekPositionTimeAbsolute,
		MethodSetSeekToNearestMarker,

		LastMethodID
	};
};

#endif // #ifndef AK_OPTIMIZED
