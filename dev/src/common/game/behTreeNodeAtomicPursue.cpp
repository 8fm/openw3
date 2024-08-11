/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicPursue.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "explorationScriptSupport.h"
#include "movingPhysicalAgentComponent.h"
#include "expComponent.h"
#include "movingAgentComponent.h"
#include "movableRepresentationPathAgent.h"

#include "../core/mathUtils.h"

const Float CBehTreeNodeBaseAtomicPursueTargetInstance:: MAX_EXPLORATION_DISTANCE_SQRT = 1.0f * 1.0f;

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeBaseAtomicPursueTargetDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	CBehTreeNodeBaseAtomicPursueTargetInstance* instance = new Instance( *this, owner, context, parent );
	return instance;
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeBaseAtomicPursueTargetInstance::CBehTreeNodeBaseAtomicPursueTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_minDistance( def.m_minDistance.GetVal( context ) )
	, m_moveSpeed( def.m_moveSpeed.GetVal( context ) )
	, m_tolerance( def.m_tolerance.GetVal( context ) )
	, m_moveType( def.m_moveType.GetVal( context ) )
	, m_tryToMoveOutsideNavdata( def.m_moveOutsideNavdata.GetVal( context ) )
{
}

CNode*const CBehTreeNodeBaseAtomicPursueTargetInstance::ComputeTarget()
{
	return nullptr;
}

Bool CBehTreeNodeBaseAtomicPursueTargetInstance::IsInValidRange( CNode* target )
{
	CActor* actor = m_owner->GetActor();
	const Vector& myPos = actor->GetWorldPositionRef();
	const Vector& targetPos = target->GetWorldPositionRef();
	if ( Abs( myPos.Z - targetPos.Z ) < 2.f )
	{
		Float distanceSq = ( myPos.AsVector2() - targetPos.AsVector2() ).SquareMag();
		return distanceSq > (m_minDistance*m_minDistance);
	}
	return true;
}

Bool CBehTreeNodeBaseAtomicPursueTargetInstance::IsAvailable()
{
	CNode* target = ComputeTarget();
	Bool toRet = ( target != nullptr && IsInValidRange(target) );
	if( !toRet )
	{
		DebugNotifyAvailableFail();
	}
	return toRet;
}

Bool CBehTreeNodeBaseAtomicPursueTargetInstance::Activate()
{
	CNode* target = ComputeTarget();
	if ( !target )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_currentTarget = target;
	CActor* actor = m_owner->GetActor();
	if ( !actor )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( !m_tryToMoveOutsideNavdata )
	{
		if ( !actor->ActionMoveToDynamicNode( target, m_moveType, m_moveSpeed, m_minDistance, true, MFA_EXIT, m_tolerance ) )
		{
			DebugNotifyActivationFail();
			return false;
		}
	}
	else
	{
		const CMovingAgentComponent* const movingAgentComponent = actor->GetMovingAgentComponent();
		CPathAgent* const pathAgent = movingAgentComponent->GetPathAgent();
		
		const Bool startPosValid = pathAgent && pathAgent->TestLocation( movingAgentComponent->GetWorldPositionRef() );
		const Bool actionAvailable = startPosValid ? actor->ActionMoveToDynamicNode( target, m_moveType, m_moveSpeed, m_minDistance, true, MFA_EXIT, m_tolerance ) : actor->ActionMoveOutsideNavdata( target, m_moveType, m_moveSpeed, m_minDistance );

		if ( !actionAvailable )
		{
			DebugNotifyActivationFail();
			return false;
		}
	}
	
	m_state = E_MOVE;
	m_owner->OnGameplayEvent( MovementStartedEventName() );

	return Super::Activate();
}

Bool CBehTreeNodeBaseAtomicPursueTargetInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME( AI_MovementAction ) )
	{
		m_owner->GetActor()->ActionCancelAll();
		IAITree* behavior = e.m_gameplayEventData.Get< IAITree >();
		if ( behavior )
		{
			if ( SpawnChildNode( behavior, SBehTreeDynamicNodeEventData::Parameters(), this, m_owner ) && m_childNode->Activate() )
			{
				m_state = E_SUBBEHAVIOR_ACTIVE;
				return false;
			}
		}

		// some problems with ai exploration behavior
		Complete( BTTO_FAILED );
		return false;
	}
	return IBehTreeDynamicNodeBase::OnEvent( e );
}

void CBehTreeNodeBaseAtomicPursueTargetInstance::Update()
{	
	if ( m_state == E_MOVE )
	{	
		CActor* actor = m_owner->GetActor();
		CNode* target = m_currentTarget.Get();

		if ( !target )
		{
			Complete( BTTO_FAILED );
			return;
		}
	
		if( !actor->IsMoving() )
		{
			Complete( actor->GetCurrentActionStatus() == ActorActionResult_Succeeded ? BTTO_SUCCESS : BTTO_FAILED );
			return;
		}

		if ( !IsInValidRange( target ) )
		{
			Complete( BTTO_SUCCESS );
			return;
		}
	}
	else if( m_state == E_SUBBEHAVIOR_ACTIVE )
	{
		ASSERT( m_childNode );
		IBehTreeDynamicNodeBase::Update();
	}
}

void CBehTreeNodeBaseAtomicPursueTargetInstance::OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome )
{
	DespawnChildNode();	
	CActor* actor = m_owner->GetActor();	
	actor->ActionMoveToDynamicNode( m_currentTarget, m_moveType, m_moveSpeed, m_minDistance, true, MFA_EXIT, m_tolerance );		
	m_state = E_MOVE;
}

void CBehTreeNodeBaseAtomicPursueTargetInstance::Deactivate()
{
	IBehTreeDynamicNodeBase::Deactivate();

	CActor* actor = m_owner->GetActor();
	actor->ActionCancelAll();

	Super::Deactivate();
}

Bool CBehTreeNodeBaseAtomicPursueTargetInstance::Interrupt()
{
	/*if( m_state == E_SUBBEHAVIOR_ACTIVE )
	{
		return false;
	}
	*/
	if ( !IBehTreeDynamicNodeBase::Interrupt() )
	{
		return false;
	}
	Deactivate();
	return true;
}

Int32 CBehTreeNodeBaseAtomicPursueTargetInstance::GetNumChildren() const
{
	return IBehTreeDynamicNodeBase::GetNumChildren();
}

Int32 CBehTreeNodeBaseAtomicPursueTargetInstance::GetNumPersistantChildren() const
{
	return 0;
}

IBehTreeNodeInstance* CBehTreeNodeBaseAtomicPursueTargetInstance::GetChild( Int32 index ) const
{
	return IBehTreeDynamicNodeBase::GetChild( index );
}

IBehTreeNodeInstance* CBehTreeNodeBaseAtomicPursueTargetInstance::GetActiveChild() const
{
	return IBehTreeDynamicNodeBase::GetActiveChild();
}

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAtomicPursueTargetDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	CBehTreeNodeAtomicPursueTargetInstance* instance = new Instance( *this, owner, context, parent );
	return instance;
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicPursueTargetInstance::CBehTreeNodeAtomicPursueTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_useCombatTarget( def.m_useCombatTarget )
{
}

CNode*const CBehTreeNodeAtomicPursueTargetInstance::ComputeTarget()
{
	return m_useCombatTarget ?
		m_owner->GetCombatTarget().Get() :
		m_owner->GetActionTarget().Get();
}
