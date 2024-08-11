/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicMove.h"

#include "../engine/renderFrame.h"

#include "behTreeInstance.h"
#include "movableRepresentationPathAgent.h"
#include "movingAgentComponent.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicMoveToActionTargetDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicMoveToCombatTargetDefinition )
BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeAtomicMoveToCustomPointDefinition )

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE TARGET       //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicMoveToInstance::CBehTreeNodeAtomicMoveToInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
	, CBehTreeSteeringGraphCommonInstance( def, owner, context )
	, m_maxDistance( def.m_maxDistance.GetVal( context ) )
	, m_moveSpeed( def.m_moveSpeed.GetVal( context ) )
	, m_angularTolerance( def.m_angularTolerance )
	, m_pathfindingTolerance( def.m_pathfindingTolerance.GetVal( context ) )
	, m_target( Vector::ZEROS )
	, m_heading( 0.0f )
	, m_moveType( def.m_moveType.GetVal( context ) )
	, m_keepPreviousMoveData( def.m_keepPreviousMoveData.GetVal( context ) )
	, m_rotateAfterwards( def.m_rotateAfterwards.GetVal( context ) )
	, m_preciseArrival( def.m_preciseArrival.GetVal( context ) )
{
}

Bool CBehTreeNodeAtomicMoveToInstance::Activate()
{
	if( m_keepPreviousMoveData )
	{
		CActor* actor = m_owner->GetActor();
		if( actor )
		{
			CMovingAgentComponent*  mac = actor->GetMovingAgentComponent();
			if( mac )
			{
				m_moveType = mac->GetMovementType();
				m_moveSpeed = mac->GetMovementSpeed();
			}			
		}		
	}
	if( !UpdateTargetAndHeading() )
	{
		DebugNotifyActivationFail();
		return false;
	}
	ActivateSteering( m_owner );
	return IBehTreeNodeInstance::Activate();
}

Bool CBehTreeNodeAtomicMoveToInstance::UpdateTargetAndHeading()
{
	if( m_childNode != nullptr )
	{
		return false;
	}
	if ( !ComputeTargetAndHeading() )
	{
		return false;
	}

	if ( StartActorMoveTo() == false )
	{
		return false;
	}
	
	m_owner->OnGameplayEvent( MovementStartedEventName() );
	m_state = E_MOVE;	
	return true;
}

Bool CBehTreeNodeAtomicMoveToInstance::StartActorMoveTo()
{
	CActor* actor = m_owner->GetActor();

	if ( m_state == E_MOVE )
	{
		return actor->ActionMoveToChangeTarget( m_target, m_moveType, m_moveSpeed, m_maxDistance ); 
	}

	Uint16 moveFlags = ActorActionMoveTo::FLAGS_DEFAULT;
	if ( m_preciseArrival )
	{
		moveFlags |= ActorActionMoveTo::FLAG_PRECISE_ARRIVAL;
	}

	return actor->ActionMoveTo(m_target, m_heading, m_moveType, m_moveSpeed, m_maxDistance, MFA_EXIT, moveFlags, m_pathfindingTolerance );
}

Bool CBehTreeNodeAtomicMoveToInstance::OnDestinationReached()
{
	if ( m_rotateAfterwards )
	{
		CActor* actor = m_owner->GetActor();
		const Vector& actorPosition = actor->GetWorldPositionRef();
		actor->ActionRotateTo( actorPosition + EulerAngles::YawToVector( m_heading ) * 4096.f, true, 15.f );
		m_state = E_ROTATE;
		CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
		mac->ForceSetRelativeMoveSpeed( 0 );
		mac->SetMoveRotation( 0 );

		return false;
	}
	else
	{
		CActor* actor				= m_owner->GetActor();
		CMovingAgentComponent* mac	= actor->GetMovingAgentComponent();
		mac->RequestAbsoluteMoveSpeed( 0.0f );
		Complete( BTTO_SUCCESS );
		return true;
	}
}

void CBehTreeNodeAtomicMoveToInstance::Update()
{
	CActor* actor = m_owner->GetActor();
	const Vector& actorPosition = actor->GetWorldPositionRef();
	if ( m_state == E_MOVE )
	{
		if ( !actor->IsMoving() )
		{
			if ( m_target.DistanceSquaredTo2D( actorPosition ) <= (m_maxDistance*m_maxDistance) && Abs( m_target.Z - actorPosition.Z ) < 2.f )
			{
				if ( OnDestinationReached() )
				{
					return;
				}
				Complete( BTTO_SUCCESS );
				return;
			}

			Complete( BTTO_FAILED );
			return;
		}
	}
	if ( m_state == E_ROTATE )
	{
		if ( Abs( EulerAngles::AngleDistance( actor->GetWorldYaw(), m_heading ) ) <= m_angularTolerance )
		{
			Complete( BTTO_SUCCESS );
			return;
		}
		else if ( actor->GetCurrentActionType() != ActorAction_Rotating )
		{
			Complete( BTTO_FAILED );
			return;
		}
	}
	else if ( m_state == E_SUBBEHAVIOR_ACTIVE )
	{
		ASSERT( m_childNode );
		IBehTreeDynamicNodeBase::Update();
	}
}

void CBehTreeNodeAtomicMoveToInstance::Deactivate()
{
	IBehTreeDynamicNodeBase::Deactivate();

	DeactivateSteering( m_owner );

	CActor* actor = m_owner->GetActor();
	actor->ActionCancelAll();

	IBehTreeNodeInstance::Deactivate();
}

void CBehTreeNodeAtomicMoveToInstance::OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome )
{
	DespawnChildNode();

	if ( !ComputeTargetAndHeading() )
	{
		Complete( BTTO_FAILED );
		return;
	}

	if ( !StartActorMoveTo() )
	{
		Complete( BTTO_FAILED );
		return;
	}
	m_state = E_MOVE;
}
Bool CBehTreeNodeAtomicMoveToInstance::OnEvent( CBehTreeEvent& e )
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
Bool CBehTreeNodeAtomicMoveToInstance::Interrupt()
{
	if ( !IBehTreeDynamicNodeBase::Interrupt() )
	{
		return false;
	}
	Deactivate();
	return true;
}
Int32 CBehTreeNodeAtomicMoveToInstance::GetNumChildren() const
{
	return IBehTreeDynamicNodeBase::GetNumChildren();
}
Int32 CBehTreeNodeAtomicMoveToInstance::GetNumPersistantChildren() const
{
	return 0;
}
IBehTreeNodeInstance* CBehTreeNodeAtomicMoveToInstance::GetChild( Int32 index ) const
{
	return IBehTreeDynamicNodeBase::GetChild( index );
}
IBehTreeNodeInstance* CBehTreeNodeAtomicMoveToInstance::GetActiveChild() const
{
	return IBehTreeDynamicNodeBase::GetActiveChild();
}

void CBehTreeNodeAtomicMoveToInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	CActor* actor = m_owner->GetActor();
	Vector pos = actor->GetWorldPosition();

	const Char* state = TXT("");

	switch ( m_state )
	{
	case E_SUBBEHAVIOR_ACTIVE:
		state = TXT("Exploration");
		break;
	case E_MOVE:
		state = TXT("Move");
		break;
	case E_ROTATE:
		state = TXT("Rotate");
		break;
	}

	Float dist = (m_target - pos).Mag3();

	String text =
		String::Printf( TXT( "%s dist: %0.2f/%0.2f" )
		, state
		, dist
		, m_maxDistance);

	frame->AddDebugText( pos, text, -200, -5 );
}


Bool CBehTreeNodeAtomicMoveToActionTargetInstance::ComputeTargetAndHeading()
{
	CNode* target = m_owner->GetActionTarget().Get();
	if ( target )
	{
		m_target = target->GetWorldPositionRef();
		m_heading = target->GetWorldYaw();
		return true;
	}
	return false;
}


Bool CBehTreeNodeAtomicMoveToCombatTargetInstance::ComputeTargetAndHeading()
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( npc )
	{
		CActor* target = npc->GetTarget();
		if ( target )
		{
			m_target = target->GetWorldPositionRef();
			m_heading = target->GetWorldYaw();
			return true;
		}
	}
	return false;
}

CBehTreeNodeAtomicMoveToCustomPointInstance::CBehTreeNodeAtomicMoveToCustomPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_customMoveData( owner )
{
}

Bool CBehTreeNodeAtomicMoveToCustomPointInstance::ComputeTargetAndHeading()
{
	if ( m_customMoveData )
	{
		m_target = m_customMoveData->GetTarget();
		m_heading = m_customMoveData->GetHeading();
		return true;
	}
	return false;
}

