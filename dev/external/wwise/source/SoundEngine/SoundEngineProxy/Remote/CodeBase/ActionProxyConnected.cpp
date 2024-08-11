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

#include "AkActions.h"
#include "ActionProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

ActionProxyConnected::ActionProxyConnected( AkActionType in_eActionType, AkUniqueID in_uID )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_uID, AkIdxType_Action );
	if ( !pIndexable )
		pIndexable = CAkAction::Create( in_eActionType, in_uID );

	SetIndexable( pIndexable );
}

ActionProxyConnected::~ActionProxyConnected()
{
}

void ActionProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkAction * pAction = static_cast<CAkAction *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionProxy::MethodSetElementID:
		{
			ActionProxyCommandData::SetElementID setElementID;
			if( in_rSerializer.Get( setElementID ) )
				pAction->SetElementID( setElementID.m_param1 );
			
			break;
		}

	case IActionProxy::MethodSetAkPropF:
		{
			ActionProxyCommandData::SetAkPropF setAkProp;
			if( in_rSerializer.Get( setAkProp ) )
					pAction->SetAkProp( (AkPropID) setAkProp.m_param1, setAkProp.m_param2, setAkProp.m_param3, setAkProp.m_param4 );
			break;
		}

	case IActionProxy::MethodSetAkPropI:
		{
			ActionProxyCommandData::SetAkPropI setAkProp;
			if( in_rSerializer.Get( setAkProp ) )
			{
				if ( setAkProp.m_param1 == AkPropID_DelayTime )
					pAction->SetAkProp( (AkPropID) setAkProp.m_param1, 
						AkTimeConv::MillisecondsToSamples( setAkProp.m_param2 ), 
						AkTimeConv::MillisecondsToSamples( setAkProp.m_param3 ), 
						AkTimeConv::MillisecondsToSamples( setAkProp.m_param4 ) );
				else
					pAction->SetAkProp( (AkPropID) setAkProp.m_param1, setAkProp.m_param2, setAkProp.m_param3, setAkProp.m_param4 );
			}
			break;
		}

	case IActionProxy::MethodCurveType:
		{
			ActionProxyCommandData::CurveType curveType;
			if( in_rSerializer.Get( curveType ) )
				pAction->CurveType( (AkCurveInterpolation) curveType.m_param1 );
			break;
		}

	default:
		AKASSERT( false );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionExceptProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionExcept * pActionExcept = static_cast<CAkActionExcept *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionExceptProxy::MethodAddException:
		{
			ActionExceptProxyCommandData::AddException addException;
			if( in_rSerializer.Get( addException ) )
				pActionExcept->AddException( addException.m_param1 );
			break;
		}

	case IActionExceptProxy::MethodRemoveException:
		{
			ActionExceptProxyCommandData::RemoveException removeException;
			if( in_rSerializer.Get( removeException ) )
				pActionExcept->RemoveException( removeException.m_param1 );
			break;
		}

	case IActionExceptProxy::MethodClearExceptions:
		{
			ActionExceptProxyCommandData::ClearExceptions clearExceptions;
			if( in_rSerializer.Get( clearExceptions ) )
				pActionExcept->ClearExceptions();
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionPauseProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionPause * pActionPause = static_cast<CAkActionPause *>( GetIndexable() );
	
	switch( in_uMethodID )
	{
	case IActionPauseProxy::MethodIncludePendingResume:
		{
			ActionPauseProxyCommandData::IncludePendingResume includePendingResume;
			if( in_rSerializer.Get( includePendingResume ) )
				pActionPause->IncludePendingResume( includePendingResume.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionResumeProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionResume * pActionResume = static_cast<CAkActionResume *>( GetIndexable() );
	
	switch( in_uMethodID )
	{
	case IActionResumeProxy::MethodIsMasterResume:
		{
			ActionResumeProxyCommandData::IsMasterResume isMasterResume;
			if( in_rSerializer.Get( isMasterResume ) )
				pActionResume->IsMasterResume( isMasterResume.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetAkPropProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionSetAkProp * pActionSetAkProp = static_cast<CAkActionSetAkProp *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionSetAkPropProxy::MethodSetValue:
		{
			ActionSetAkPropProxyCommandData::SetValue setValue;
			if( in_rSerializer.Get( setValue ) )
				pActionSetAkProp->SetValue( setValue.m_param1, (AkValueMeaning) setValue.m_param2, setValue.m_param3, setValue.m_param4 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetStateProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionSetState * pActionSetState = static_cast<CAkActionSetState *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionSetStateProxy::MethodSetGroup:
		{
			ActionSetStateProxyCommandData::SetGroup setGroup;
			if( in_rSerializer.Get( setGroup ) )
				pActionSetState->SetStateGroup( setGroup.m_param1 );
			break;
		}

	case IActionSetStateProxy::MethodSetTargetState:
		{
			ActionSetStateProxyCommandData::SetTargetState setTargetState;
			if( in_rSerializer.Get( setTargetState ) )
				pActionSetState->SetTargetState( setTargetState.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetSwitchProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionSetSwitch * pActionSetSwitch = static_cast<CAkActionSetSwitch *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionSetSwitchProxy::MethodSetSwitchGroup:
		{
			ActionSetSwitchProxyCommandData::SetSwitchGroup setSwitchGroup;
			if( in_rSerializer.Get( setSwitchGroup ) )
				pActionSetSwitch->SetSwitchGroup( setSwitchGroup.m_param1 );
			break;
		}

	case IActionSetSwitchProxy::MethodSetTargetSwitch:
		{
			ActionSetSwitchProxyCommandData::SetTargetSwitch setTargetSwitch;
			if( in_rSerializer.Get( setTargetSwitch ) )
				pActionSetSwitch->SetTargetSwitch( setTargetSwitch.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSetGameParameterProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionSetGameParameter * pActionSetGameParameter = static_cast<CAkActionSetGameParameter *>( GetIndexable() );

	switch( in_uMethodID )
	{		
	case IActionSetGameParameterProxy::MethodSetValue:
		{
			ActionSetGameParameterProxyCommandData::SetValue setValue;
			if( in_rSerializer.Get( setValue ) )
				pActionSetGameParameter->SetValue( setValue.m_param1, (AkValueMeaning) setValue.m_param2, setValue.m_param3, setValue.m_param4 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionUseStateProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionUseState * pActionUseState = static_cast<CAkActionUseState *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionUseStateProxy::MethodUseState:
		{
			ActionUseStateProxyCommandData::UseState useState;
			if( in_rSerializer.Get( useState ) )
				pActionUseState->UseState( useState.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionBypassFXProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionBypassFX * pActionBypassFX = static_cast<CAkActionBypassFX *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionBypassFXProxy::MethodBypassFX:
		{
			ActionBypassFXProxyCommandData::BypassFX bypassFX;
			if( in_rSerializer.Get( bypassFX ) )
				pActionBypassFX->Bypass( bypassFX.m_param1 );
			break;
		}
	case IActionBypassFXProxy::MethodSetBypassTarget:
		{
			ActionBypassFXProxyCommandData::SetBypassTarget setBypassTarget;
			if( in_rSerializer.Get( setBypassTarget ) )
				pActionBypassFX->SetBypassTarget( setBypassTarget.m_param1, setBypassTarget.m_param2 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

// -----------------------------------------------------------------------------------------------------
// -----------------------------------------------------------------------------------------------------

void ActionSeekProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkActionSeek * pActionSeek = static_cast<CAkActionSeek *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IActionSeekProxy::MethodSetSeekPositionPercent:
		{
			ActionSeekProxyCommandData::SetSeekPositionPercent setSeekPercent;
			if( in_rSerializer.Get( setSeekPercent ) )
				pActionSeek->SetSeekPositionPercent( setSeekPercent.m_param1, setSeekPercent.m_param2, setSeekPercent.m_param3 );
			break;
		}
	case IActionSeekProxy::MethodSetSeekPositionTimeAbsolute:
		{
			ActionSeekProxyCommandData::SetSeekPositionTimeAbsolute setSeekTimeAbsolute;
			if( in_rSerializer.Get( setSeekTimeAbsolute ) )
				pActionSeek->SetSeekPositionTimeAbsolute( setSeekTimeAbsolute.m_param1, setSeekTimeAbsolute.m_param2, setSeekTimeAbsolute.m_param3 );
			break;
		}
	case IActionSeekProxy::MethodSetSeekToNearestMarker:
		{
			ActionSeekProxyCommandData::SetSeekToNearestMarker setSeekToNearestMarker;
			if( in_rSerializer.Get( setSeekToNearestMarker ) )
				pActionSeek->SetSeekToNearestMarker( setSeekToNearestMarker.m_param1 );
			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}

#endif // #ifndef AK_OPTIMIZED
