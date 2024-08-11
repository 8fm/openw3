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

#include "SwitchContainerProxyConnected.h"
#include "AkSwitchCntr.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"
#include "ISwitchContainerProxy.h"

SwitchContainerProxyConnected::SwitchContainerProxyConnected( AkUniqueID in_id )
{
	CAkIndexable* pIndexable = AK::SoundEngine::GetIndexable( in_id, AkIdxType_AudioNode );
	if ( !pIndexable )
		pIndexable = CAkSwitchCntr::Create( in_id );

	SetIndexable( pIndexable );
}

SwitchContainerProxyConnected::~SwitchContainerProxyConnected()
{
}

void SwitchContainerProxyConnected::HandleExecute( AkUInt16 in_uMethodID, CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	CAkSwitchCntr * pCntr = static_cast<CAkSwitchCntr *>( GetIndexable() );

	switch( in_uMethodID )
	{
	case ISwitchContainerProxy::MethodSetSwitchGroup:
		{
			SwitchContainerProxyCommandData::SetSwitchGroup setSwitchGroup;
			if( in_rSerializer.Get( setSwitchGroup ) )
					pCntr->SetSwitchGroup( setSwitchGroup.m_ulGroup, setSwitchGroup.m_eGroupType );

			break;
		}

	case ISwitchContainerProxy::MethodSetDefaultSwitch:
		{
			SwitchContainerProxyCommandData::SetDefaultSwitch setDefaultSwitch;
			if( in_rSerializer.Get( setDefaultSwitch ) )
					pCntr->SetDefaultSwitch( setDefaultSwitch.m_switch );

			break;
		}

	case ISwitchContainerProxy::MethodClearSwitches:
		{
			SwitchContainerProxyCommandData::ClearSwitches clearSwitches;
			if( in_rSerializer.Get( clearSwitches ) )
					pCntr->ClearSwitches();

			break;
		}

	case ISwitchContainerProxy::MethodAddSwitch:
		{
			SwitchContainerProxyCommandData::AddSwitch addSwitch;
			if( in_rSerializer.Get( addSwitch ) )
					pCntr->AddSwitch( addSwitch.m_switch );

			break;
		}

	case ISwitchContainerProxy::MethodRemoveSwitch:
		{
			SwitchContainerProxyCommandData::RemoveSwitch removeSwitch;
			if( in_rSerializer.Get( removeSwitch ) )
					pCntr->RemoveSwitch( removeSwitch.m_switch );

			break;
		}

	case ISwitchContainerProxy::MethodAddNodeInSwitch:
		{
			SwitchContainerProxyCommandData::AddNodeInSwitch addNodeInSwitch;
			if( in_rSerializer.Get( addNodeInSwitch ) )
					pCntr->AddNodeInSwitch( addNodeInSwitch.m_switch, addNodeInSwitch.m_nodeID );

			break;
		}

	case ISwitchContainerProxy::MethodRemoveNodeFromSwitch:
		{
			SwitchContainerProxyCommandData::RemoveNodeFromSwitch removeNodeFromSwitch;
			if( in_rSerializer.Get( removeNodeFromSwitch ) )
					pCntr->RemoveNodeFromSwitch( removeNodeFromSwitch.m_switch, removeNodeFromSwitch.m_nodeID );

			break;
		}

	case ISwitchContainerProxy::MethodSetContinuousValidation:
		{
			SwitchContainerProxyCommandData::SetContinuousValidation setContinuousValidation;
			if( in_rSerializer.Get( setContinuousValidation ) )
					pCntr->SetContinuousValidation( setContinuousValidation.m_bIsContinuousCheck );

			break;
		}

	case ISwitchContainerProxy::MethodSetContinuePlayback:
		{
			SwitchContainerProxyCommandData::SetContinuePlayback setContinuePlayback;
			if( in_rSerializer.Get( setContinuePlayback ) )
					pCntr->SetContinuePlayback( setContinuePlayback.m_nodeID, setContinuePlayback.m_bContinuePlayback );

			break;
		}

	case ISwitchContainerProxy::MethodSetFadeInTime:
		{
			SwitchContainerProxyCommandData::SetFadeInTime setFadeInTime;
			if( in_rSerializer.Get( setFadeInTime ) )
					pCntr->SetFadeInTime( setFadeInTime.m_nodeID, setFadeInTime.m_time );

			break;
		}

	case ISwitchContainerProxy::MethodSetFadeOutTime:
		{
			SwitchContainerProxyCommandData::SetFadeOutTime setFadeOutTime;
			if( in_rSerializer.Get( setFadeOutTime ) )
					pCntr->SetFadeOutTime( setFadeOutTime.m_nodeID, setFadeOutTime.m_time );

			break;
		}

	case ISwitchContainerProxy::MethodSetOnSwitchMode:
		{
			SwitchContainerProxyCommandData::SetOnSwitchMode setOnSwitchMode;
			if( in_rSerializer.Get( setOnSwitchMode ) )
					pCntr->SetOnSwitchMode( setOnSwitchMode.m_nodeID, setOnSwitchMode.m_bSwitchMode );

			break;
		}

	case ISwitchContainerProxy::MethodSetIsFirstOnly:
		{
			SwitchContainerProxyCommandData::SetIsFirstOnly setIsFirstOnly;
			if( in_rSerializer.Get( setIsFirstOnly ) )
					pCntr->SetIsFirstOnly( setIsFirstOnly.m_nodeID, setIsFirstOnly.m_bIsFirstOnly );

			break;
		}

	default:
		__base::HandleExecute( in_uMethodID, in_rSerializer, out_rReturnSerializer );
	}
}
#endif // #ifndef AK_OPTIMIZED
