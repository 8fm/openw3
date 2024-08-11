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



#include "StateMgrProxyConnected.h"
#include "CommandData.h"
#include "CommandDataSerializer.h"

void StateMgrProxyConnected::HandleExecute( CommandDataSerializer& in_rSerializer, CommandDataSerializer& out_rReturnSerializer )
{
	StateMgrProxyCommandData::CommandData cmdData;

	{
		CommandDataSerializer::AutoSetDataPeeking peekGate( in_rSerializer );
		in_rSerializer.Get( cmdData );
	}

	switch( cmdData.m_methodID )
	{
	case IStateMgrProxy::MethodAddStateGroup:
		{
			StateMgrProxyCommandData::AddStateGroup addStateGroup;
			if( in_rSerializer.Get( addStateGroup ) )
					m_localProxy.AddStateGroup( addStateGroup.m_groupID );

			break;
		}

	case IStateMgrProxy::MethodRemoveStateGroup:
		{
			StateMgrProxyCommandData::RemoveStateGroup removeStateGroup;
			if( in_rSerializer.Get( removeStateGroup ) )
					m_localProxy.RemoveStateGroup( removeStateGroup.m_groupID );

			break;
		}

	case IStateMgrProxy::MethodAddStateTransition:
		{
			StateMgrProxyCommandData::AddStateTransition addStateTransition;
			if( in_rSerializer.Get( addStateTransition ) )
					m_localProxy.AddStateTransition( addStateTransition.m_groupID, 
													addStateTransition.m_stateID1, 
													addStateTransition.m_stateID2, 
													addStateTransition.m_transitionTime, 
													addStateTransition.m_bIsShared );

			break;
		}

	case IStateMgrProxy::MethodRemoveStateTransition:
		{
			StateMgrProxyCommandData::RemoveStateTransition removeStateTransition;
			if( in_rSerializer.Get( removeStateTransition ) )
					m_localProxy.RemoveStateTransition( removeStateTransition.m_groupID, 
													removeStateTransition.m_stateID1, 
													removeStateTransition.m_stateID2, 
													removeStateTransition.m_bIsShared );

			break;
		}

	case IStateMgrProxy::MethodClearStateTransitions:
		{
			StateMgrProxyCommandData::ClearStateTransitions clearStateTransitions;
			if( in_rSerializer.Get( clearStateTransitions ) )
					m_localProxy.ClearStateTransitions( clearStateTransitions.m_groupID );

			break;
		}

	case IStateMgrProxy::MethodSetDefaultTransitionTime:
		{
			StateMgrProxyCommandData::SetDefaultTransitionTime setDefaultTransitionTime;
			if( in_rSerializer.Get( setDefaultTransitionTime ) )
					m_localProxy.SetDefaultTransitionTime( setDefaultTransitionTime.m_groupID, setDefaultTransitionTime.m_transitionTime );

			break;
		}

	case IStateMgrProxy::MethodSetState:
		{
			StateMgrProxyCommandData::SetState setState;
			if( in_rSerializer.Get( setState ) )
					m_localProxy.SetState( setState.m_groupID, setState.m_stateID );

			break;
		}

	case IStateMgrProxy::MethodGetState:
		{
			StateMgrProxyCommandData::GetState getState;
			if( in_rSerializer.Get( getState ) )
					out_rReturnSerializer.Put( m_localProxy.GetState( getState.m_groupID ) );

			break;
		}


	default:
		AKASSERT( !"Unsupported command." );
	}
}
#endif // #ifndef AK_OPTIMIZED
