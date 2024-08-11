/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAIAction.h"
#include "r6aiSystem.h"

#include "../../common/game/behTreeNode.h"
#include "../../common/game/behTreeInstance.h"
#include "aiTreeComponent.h"
#include "../../common/game/movingPhysicalAgentComponent.h"
#include "../../common/engine/behaviorGraphStack.h"

RED_DEFINE_STATIC_NAME( playerControlled );

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAIActionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= nullptr */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAIActionInstance::CBehTreeNodeAIActionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
	, m_actionInstance( static_cast< CR6BehTreeInstance* > ( m_owner )->SpawnAIActionInstance( def.m_action.GetVal( context ) )->Bind( this ) )
{
}

CBehTreeNodeAIActionInstance::~CBehTreeNodeAIActionInstance()
{
	static_cast< CR6BehTreeInstance* > ( m_owner )->RemoveAIActionInstance( m_actionInstance );	
}

Bool CBehTreeNodeAIActionInstance::Activate()
{
	R6_ASSERT( m_owner->IsA< CR6BehTreeInstance >() );


	{ // set playerControlled variable to false
		CAITreeComponent* ai = m_owner->FindParent< CAITreeComponent > ();
		R6_ASSERT( ai );

		// register as a listener to locomotion events
		// this is here to fill the m_movementSegment when it is changed on a character
		const TDynArray< CComponent* >& performers = ai->GetPerformers();
		for ( Uint32 i = 0; i < performers.Size(); ++i )
		{
			CMovingPhysicalAgentComponent* mac = Cast< CMovingPhysicalAgentComponent > ( performers[ i ] );
			if ( nullptr == mac || false == mac->IsMotionEnabled() || nullptr == mac->GetBehaviorStack() )
			{
				DebugNotifyActivationFail();
				return false;
			}

			mac->GetBehaviorStack()->SetBehaviorVariable( CNAME( playerControlled ), 0.0f );
		}
	}

	EAIActionStatus status = static_cast< CR6BehTreeInstance* > ( m_owner )->PerformAIAction( m_actionInstance );
	if ( status == ACTION_InProgress )
	{
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodeAIActionInstance::Update()
{
	const EAIActionStatus status = m_actionInstance->GetStatus();
	
	R6_ASSERT( status != ACTION_NotStarted );
	
	if ( status == ACTION_Successful )
	{
		Complete( BTTO_SUCCESS );
	}
	else if ( status == ACTION_Failed )
	{
		Complete( BTTO_FAILED );
	}
}

void CBehTreeNodeAIActionInstance::Deactivate()
{
	Super::Deactivate();

	const EAIActionStatus status = m_actionInstance->GetStatus();

	if ( status == ACTION_InProgress )
	{
		R6_ASSERT( m_owner->IsA< CR6BehTreeInstance >() );
		static_cast< CR6BehTreeInstance* > ( m_owner )->StopAIAction( m_actionInstance );
	}
}

Bool CBehTreeNodeAIActionInstance::IsAvailable()
{
	R6_ASSERT( m_owner->IsA< CR6BehTreeInstance >() );
	Bool toRet = ( static_cast< CR6BehTreeInstance* > ( m_owner )->CheckAIActionAvailability( m_actionInstance ) );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}
	return toRet;
}

void CBehTreeNodeAIActionInstance::OnStopAIAction( CAIAction* actionBeingStopped )
{
	R6_ASSERT( m_actionInstance == actionBeingStopped );

	const EAIActionStatus status = m_actionInstance->GetStatus();	
	R6_ASSERT( status != ACTION_NotStarted && status != ACTION_InProgress );
	
	if( m_isActive )
	{
		if ( status == ACTION_Successful )
		{
			Complete( BTTO_SUCCESS );
		}
		else if ( status == ACTION_Failed )
		{
			Complete( BTTO_FAILED );
		}
	}
}

Bool CBehTreeNodeAIActionInstance::Interrupt()
{
	R6_ASSERT( m_actionInstance );

	// ask the action kindly to finish itself
	const EAIActionStatus status = m_actionInstance->RequestInterruption();
	if ( ACTION_InProgress == status )
	{
		// action needs time to be finished
		return false;
	}

	// action is no longer running, so we can interrupt this block
	return Super::Interrupt();
}
