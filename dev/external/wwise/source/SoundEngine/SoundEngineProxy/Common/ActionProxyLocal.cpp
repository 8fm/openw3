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

#include "ActionProxyLocal.h"
#include "AkActions.h"
#include "AkAudioLib.h"
#include "AkCritical.h"

ActionProxyLocal::ActionProxyLocal( AkActionType in_actionType, AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_Action );

	SetIndexable( pIndexable != NULL ? pIndexable : CAkAction::Create( in_actionType, in_id ) );
}

void ActionProxyLocal::SetElementID( WwiseObjectIDext in_elementID )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		static_cast<CAkAction*>( GetIndexable() )->SetElementID( in_elementID );
	}
}

void ActionProxyLocal::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->SetAkProp( in_eProp, in_fValue, in_fMin, in_fMax );
	}
}

void ActionProxyLocal::SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		if ( in_eProp == AkPropID_DelayTime )
			pIndexable->SetAkProp( in_eProp, 
				AkTimeConv::MillisecondsToSamples( in_iValue ), 
				AkTimeConv::MillisecondsToSamples( in_iMin ), 
				AkTimeConv::MillisecondsToSamples( in_iMax ) );
		else
			pIndexable->SetAkProp( in_eProp, in_iValue, in_iMin, in_iMax );
	}
}

void ActionProxyLocal::CurveType( const AkCurveInterpolation in_eCurveType )
{
	CAkAction* pIndexable = static_cast<CAkAction*>( GetIndexable() );
	if( pIndexable )
	{
		CAkFunctionCritical SpaceSetAsCritical;
		pIndexable->CurveType( in_eCurveType );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionExceptProxyLocal::AddException( WwiseObjectIDext in_elementID )
{
	CAkActionExcept* pIndexable = static_cast<CAkActionExcept*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->AddException( in_elementID );
	}
}

void ActionExceptProxyLocal::RemoveException( WwiseObjectIDext in_elementID )
{
	CAkActionExcept* pIndexable = static_cast<CAkActionExcept*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->RemoveException( in_elementID );
	}
}

void ActionExceptProxyLocal::ClearExceptions()
{
	CAkActionExcept* pIndexable = static_cast<CAkActionExcept*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->ClearExceptions();
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionPauseProxyLocal::IncludePendingResume( bool in_bIncludePendingResume )
{
	CAkActionPause* pIndexable = static_cast<CAkActionPause*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IncludePendingResume( in_bIncludePendingResume );
	}
}
// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionResumeProxyLocal::IsMasterResume( bool in_bIsMasterResume )
{
	CAkActionResume* pIndexable = static_cast<CAkActionResume*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->IsMasterResume( in_bIsMasterResume );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetAkPropProxyLocal::SetValue( AkReal32 in_fValue, AkValueMeaning in_eValueMeaning, AkReal32 in_fMin, AkReal32 in_fMax )
{
	CAkActionSetAkProp* pIndexable = static_cast<CAkActionSetAkProp*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetValue( in_fValue, in_eValueMeaning, in_fMin, in_fMax );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetStateProxyLocal::SetGroup( AkStateGroupID in_groupID )
{
	CAkActionSetState* pIndexable = static_cast<CAkActionSetState*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetStateGroup( in_groupID );
	}
}

void ActionSetStateProxyLocal::SetTargetState( AkStateID in_stateID )
{
	CAkActionSetState* pIndexable = static_cast<CAkActionSetState*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetTargetState( in_stateID );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetSwitchProxyLocal::SetSwitchGroup( const AkSwitchGroupID in_ulSwitchGroupID )
{
	CAkActionSetSwitch* pIndexable = static_cast<CAkActionSetSwitch*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSwitchGroup( in_ulSwitchGroupID );
	}
}

void ActionSetSwitchProxyLocal::SetTargetSwitch( const AkSwitchStateID in_ulSwitchID )
{
	CAkActionSetSwitch* pIndexable = static_cast<CAkActionSetSwitch*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetTargetSwitch( in_ulSwitchID );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetGameParameterProxyLocal::SetValue( AkReal32 in_value, AkValueMeaning in_eValueMeaning, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkActionSetGameParameter* pIndexable = static_cast<CAkActionSetGameParameter*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetValue( in_value, in_eValueMeaning, in_rangeMin, in_rangeMax );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionUseStateProxyLocal::UseState( bool in_bUseState )
{
	CAkActionUseState* pIndexable = static_cast<CAkActionUseState*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->UseState( in_bUseState );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionBypassFXProxyLocal::BypassFX( bool in_bBypassFX )
{
	CAkActionBypassFX* pIndexable = static_cast<CAkActionBypassFX*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->Bypass( in_bBypassFX );
	}
}

void ActionBypassFXProxyLocal::SetBypassTarget( bool in_bBypassAllFX, AkUInt8 in_ucEffectsMask )
{
	CAkActionBypassFX* pIndexable = static_cast<CAkActionBypassFX*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetBypassTarget( in_bBypassAllFX, in_ucEffectsMask );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSeekProxyLocal::SetSeekPositionPercent( AkReal32 in_position, AkReal32 in_rangeMin, AkReal32 in_rangeMax )
{
	CAkActionSeek* pIndexable = static_cast<CAkActionSeek*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSeekPositionPercent( in_position, in_rangeMin, in_rangeMax );
	}
}

void ActionSeekProxyLocal::SetSeekPositionTimeAbsolute( AkTimeMs in_position, AkTimeMs in_rangeMin, AkTimeMs in_rangeMax )
{
	CAkActionSeek* pIndexable = static_cast<CAkActionSeek*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSeekPositionTimeAbsolute( in_position, in_rangeMin, in_rangeMax );
	}
}

void ActionSeekProxyLocal::SetSeekToNearestMarker( bool in_bSeekToNearestMarker )
{
	CAkActionSeek* pIndexable = static_cast<CAkActionSeek*>( GetIndexable() );
	if( pIndexable )
	{
		pIndexable->SetSeekToNearestMarker( in_bSeekToNearestMarker );
	}
}

#endif // #ifndef PROXYCENTRAL_CONNECTED
#endif // #ifndef AK_OPTIMIZED
