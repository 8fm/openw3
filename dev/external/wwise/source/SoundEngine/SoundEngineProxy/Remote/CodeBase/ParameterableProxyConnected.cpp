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

#include "ParameterableProxyConnected.h"
#include "AkParameterNodeBase.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

ParameterableProxyConnected::ParameterableProxyConnected()
{
}

ParameterableProxyConnected::~ParameterableProxyConnected()
{
}

void ParameterableProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& /*out_rReturnSerializer*/ )
{
	CAkParameterNodeBase * pNode = static_cast<CAkParameterNodeBase *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case IParameterableProxy::MethodAddChild:
		{
			ParameterableProxyCommandData::AddChild addChild;
			if( in_rSerializer.Get( addChild ) )
				pNode->AddChild( addChild.m_param1 );
			break;
		}

	case IParameterableProxy::MethodRemoveChild:
		{
			ParameterableProxyCommandData::RemoveChild removeChild;
			if( in_rSerializer.Get( removeChild ) )
				pNode->RemoveChild( removeChild.m_param1 );
			break;
		}

	case IParameterableProxy::MethodSetAkPropF:
		{
			ParameterableProxyCommandData::SetAkPropF setAkProp;
			if( in_rSerializer.Get( setAkProp ) )
					pNode->SetAkProp( (AkPropID) setAkProp.m_param1, setAkProp.m_param2, setAkProp.m_param3, setAkProp.m_param4 );
			break;
		}

	case IParameterableProxy::MethodSetAkPropI:
		{
			ParameterableProxyCommandData::SetAkPropI setAkProp;
			if( in_rSerializer.Get( setAkProp ) )
					pNode->SetAkProp( (AkPropID) setAkProp.m_param1, setAkProp.m_param2, setAkProp.m_param3, setAkProp.m_param4 );
			break;
		}

	case IParameterableProxy::MethodPriorityApplyDistFactor:
		{
			ParameterableProxyCommandData::PriorityApplyDistFactor priorityApplyDistFactor;
			if( in_rSerializer.Get( priorityApplyDistFactor ) )
					pNode->SetPriorityApplyDistFactor( priorityApplyDistFactor.m_param1 );
			break;
		}

	case IParameterableProxy::MethodPriorityOverrideParent:
		{
			ParameterableProxyCommandData::PriorityOverrideParent priorityOverrideParent;
			if( in_rSerializer.Get( priorityOverrideParent ) )
					pNode->SetPriorityOverrideParent( priorityOverrideParent.m_param1 );
			break;
		}

	case IParameterableProxy::MethodAddStateGroup:
		{
			ParameterableProxyCommandData::AddStateGroup addStateGroup;
			if( in_rSerializer.Get( addStateGroup ) )
					pNode->AddStateGroup( addStateGroup.m_param1 );
			break;
		}

	case IParameterableProxy::MethodRemoveStateGroup:
		{
			ParameterableProxyCommandData::RemoveStateGroup removeStateGroup;
			if( in_rSerializer.Get( removeStateGroup ) )
					pNode->RemoveStateGroup( removeStateGroup.m_param1 );
			break;
		}
	case IParameterableProxy::MethodUpdateStateGroups:
		{
			ParameterableProxyCommandData::UpdateStateGroups updateStateGroups;
			if( in_rSerializer.Get( updateStateGroups ) )
				pNode->UpdateStateGroups( updateStateGroups.m_uGroups, updateStateGroups.m_pGroups, updateStateGroups.m_pStates );
			break;
		}

	case IParameterableProxy::MethodAddState:
		{
			ParameterableProxyCommandData::AddState addState;
			if( in_rSerializer.Get( addState ) )
					pNode->AddState( addState.m_param1, addState.m_param2, addState.m_param3 );

			break;
		}

	case IParameterableProxy::MethodRemoveState:
		{
			ParameterableProxyCommandData::RemoveState removeState;
			if( in_rSerializer.Get( removeState ) )
					pNode->RemoveState( removeState.m_param1, removeState.m_param2 );

			break;
		}

	case IParameterableProxy::MethodUseState:
		{
			ParameterableProxyCommandData::UseState useState;
			if( in_rSerializer.Get( useState ) )
					pNode->UseState( useState.m_param1 );
			break;
		}

	case IParameterableProxy::MethodSetStateSyncType:
		{
			ParameterableProxyCommandData::SetStateSyncType setStateSyncType;
			if( in_rSerializer.Get( setStateSyncType ) )
					pNode->SetStateSyncType( setStateSyncType.m_param1, setStateSyncType.m_param2 );

			break;
		}

	case IParameterableProxy::MethodSetFX:
		{
			ParameterableProxyCommandData::SetFX setFX;
			if( in_rSerializer.Get( setFX ) )
					pNode->SetFX( setFX.m_param1, setFX.m_param2, setFX.m_param3 );

			break;
		}

	case IParameterableProxy::MethodBypassAllFX:
		{
			ParameterableProxyCommandData::BypassAllFX bypassAllFX;
			if( in_rSerializer.Get( bypassAllFX ) )
			{
					AkUInt32 uBits = ( bypassAllFX.m_param1 ? 1 : 0 ) << AK_NUM_EFFECTS_BYPASS_ALL_FLAG;
					pNode->MainBypassFX( uBits, 1 << AK_NUM_EFFECTS_BYPASS_ALL_FLAG );
			}

			break;
		}

	case IParameterableProxy::MethodBypassFX:
		{
			ParameterableProxyCommandData::BypassFX bypassFX;
			if( in_rSerializer.Get( bypassFX ) )
			{
				AkUInt32 uBits = ( bypassFX.m_param2 ? 1 : 0 ) << bypassFX.m_param1;
				pNode->BypassFX( uBits, 1 << bypassFX.m_param1 );
			}

			break;
		}

	case IParameterableProxy::MethodRemoveFX:
		{
			ParameterableProxyCommandData::RemoveFX removeFX;
			if( in_rSerializer.Get( removeFX ) )
				pNode->RemoveFX( removeFX.m_param1 );

			break;
		}

	case IParameterableProxy::MethodUpdateEffects:
		{
			ParameterableProxyCommandData::UpdateEffects update;
			if( in_rSerializer.Get( update ) )
				pNode->UpdateEffects( update.m_uCount, update.m_pUpdates );

			break;
		}


	case IParameterableProxy::MethodSetRTPC:
		{
			ParameterableProxyCommandData::SetRTPC setRTPC;
			if( in_rSerializer.Get( setRTPC ) )
					pNode->SetRTPC( setRTPC.m_RTPCID, setRTPC.m_paramID, setRTPC.m_RTPCCurveID, setRTPC.m_eScaling, setRTPC.m_pArrayConversion, setRTPC.m_ulConversionArraySize );

			break;
		}

	case IParameterableProxy::MethodUnsetRTPC:
		{
			ParameterableProxyCommandData::UnsetRTPC unsetRTPC;
			if( in_rSerializer.Get( unsetRTPC ) )
					pNode->UnsetRTPC( (AkRTPC_ParameterID) unsetRTPC.m_param1, unsetRTPC.m_param2 );

			break;
		}

	case IParameterableProxy::MethodMonitoringSolo:
		{
			ParameterableProxyCommandData::MonitoringSolo monitoringSolo;
			if( in_rSerializer.Get( monitoringSolo ) )
					pNode->MonitoringSolo( monitoringSolo.m_param1 );
			break;
		}

	case IParameterableProxy::MethodMonitoringMute:
		{
			ParameterableProxyCommandData::MonitoringMute monitoringMute;
			if( in_rSerializer.Get( monitoringMute ) )
					pNode->MonitoringMute( monitoringMute.m_param1 );
			break;
		}

	case IParameterableProxy::MethodPosSetPositioningType:
		{
			ParameterableProxyCommandData::PosSetPositioningType posSetPositioningType;
			if( in_rSerializer.Get( posSetPositioningType ) )
					pNode->PosSetPositioningType( posSetPositioningType.m_param1, posSetPositioningType.m_param2, (AkPannerType)posSetPositioningType.m_param3, (AkPositionSourceType)posSetPositioningType.m_param4 );

			break;
		}

	case IParameterableProxy::MethodPosSetPannerEnabled:
		{
			ParameterableProxyCommandData::PosSetPannerEnabled posSetPannerEnabled;
			if( in_rSerializer.Get( posSetPannerEnabled ) )
					pNode->PosSetPannerEnabled( posSetPannerEnabled.m_param1 );

			break;
		}

	case IParameterableProxy::MethodSetPositioningEnabled:
		{
			ParameterableProxyCommandData::SetPositioningEnabled setPositioningEnabled;
			if( in_rSerializer.Get( setPositioningEnabled ) )
					pNode->SetPositioningEnabled( setPositioningEnabled.m_param1 );

			break;
		}

	default:
		AKASSERT( false );
	}
}
#endif // #ifndef AK_OPTIMIZED
