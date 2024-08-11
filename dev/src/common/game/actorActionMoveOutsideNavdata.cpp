/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorActionMoveOutsideNavdata.h"

#include "movementTargeter.h"
#include "moveSteeringLocomotionSegment.h"
#include "../physics/PhysicsWorld.h"
#include "../physics/physicsWorldUtils.h"

///////////////////////////////////////////////////////////////////////////////

static Bool IsReachable( const CMovingAgentComponent* const mac, const Vector& fromPos, const Vector& toPos )
{
	if ( CWorld* world = mac->GetWorld() )
	{
		CPhysicsWorld* physicsWorld = nullptr;
		if ( world->GetPhysicsWorld( physicsWorld ) )
		{
			CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
			CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) );

			SPhysicsContactInfo contactInfo;
			return physicsWorld->RayCastWithSingleResult( fromPos, toPos, include, exclude, contactInfo ) != TRV_Hit;
		}
	}

	return true;
}

ActorActionMoveOutsideNavdata::ActorActionMoveOutsideNavdata( CActor* actor, EActorActionType type )
	: ActorAction( actor, type )
	, m_activeSegment( nullptr )
	, m_targetMAC( nullptr )
	, m_desiredDistance( 0.f )
	, m_isMoving( false )
{
}

ActorActionMoveOutsideNavdata::~ActorActionMoveOutsideNavdata()
{
}

void ActorActionMoveOutsideNavdata::SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const
{
	mac.SetMoveType( moveType, absSpeed );
}

Bool ActorActionMoveOutsideNavdata::IsCancelingAllowed() const
{
	CMovingAgentComponent* const ac =  m_actor->GetMovingAgentComponent();
	return ac->CanCancelMovement();
}

Bool ActorActionMoveOutsideNavdata::StartMove( const CNode* target, EMoveType moveType, Float absSpeed, Float radius )
{
	PC_SCOPE( AI_ActorActionMoveOutsideNavdata );

	CMovingAgentComponent* const mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	// if the target's got a CMovingAgentComponent, cash its instance for future reference
	m_targetMAC = nullptr;
	
	if ( const CEntity* const targetEntity = Cast< CEntity >( target ) )
	{
		for( ComponentIterator< CMovingAgentComponent > it( targetEntity ); it; ++it )
		{
			// get the first target MAC there is
			m_targetMAC = *it;
			if ( m_targetMAC )
			{
				break;
			}
		}
	}
	SetMoveSpeed( *mac, moveType, absSpeed );

	m_desiredDistance = radius;
	m_target = const_cast< CNode* >( target );

	mac->AttachLocomotionController( *this );

	return true;
}

void ActorActionMoveOutsideNavdata::Cleanup()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}

	m_target = nullptr;
	m_targetMAC = nullptr;
	m_isMoving = false;
}

void ActorActionMoveOutsideNavdata::Stop()
{
	if ( CMovingAgentComponent* const mac = m_actor->GetMovingAgentComponent() )
	{
		mac->DetachLocomotionController( *this );
	}

	Cleanup();
}

void ActorActionMoveOutsideNavdata::OnSegmentFinished( EMoveStatus status )
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
	if ( CanUseLocomotion() )
	{
		m_activeSegment = &locomotion().PushSteeringSegment();
		m_activeSegment->AddTargeter( this );
		m_activeSegment->AddRef();
	}
}

void ActorActionMoveOutsideNavdata::OnControllerAttached()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
	m_activeSegment = &locomotion().PushSteeringSegment();
	m_activeSegment->AddTargeter( this );
	m_activeSegment->AddRef();
}

void ActorActionMoveOutsideNavdata::OnControllerDetached()
{
	if ( m_activeSegment )
	{
		m_activeSegment->Release();
		m_activeSegment = nullptr;
	}
}

Bool ActorActionMoveOutsideNavdata::Update( Float timeDelta )
{
	PC_SCOPE( AI_ActorActionMoveOutsideNavdata );

	CMovingAgentComponent* const mac = m_actor->GetMovingAgentComponent();
	CNode* const targetNode = m_target.Get();
	if ( !targetNode )
	{
		m_actor->ActionEnded( ActorAction_MovingOutNavdata, ActorActionResult_Failed );
		Stop();
		Cleanup();
		return false;
	}

	// check success 
	if ( m_activeSegment && m_activeSegment->IsDestinationReached() )
	{
		// movement completed
		m_actor->ActionEnded( ActorAction_MovingOutNavdata, ActorActionResult_Succeeded );

		Stop();
		Cleanup();
		return false;
	}

	if ( !m_isMoving )
	{
		const Vector3 targetPos = m_targetMAC ? m_targetMAC->GetAgentPosition().AsVector3() : targetNode->GetWorldPosition().AsVector3();

		// success check
		const Vector& actorPos = mac->GetAgentPosition();
		const Float distSq = ( actorPos.AsVector2() - targetPos.AsVector2() ).SquareMag();
		if ( distSq <= m_desiredDistance * m_desiredDistance && Abs( actorPos.Z - targetPos.Z ) < 2.f )
		{
			// movement completed
			m_actor->ActionEnded( ActorAction_MovingOutNavdata, ActorActionResult_Succeeded );

			Stop();
			Cleanup();
			return false;
		}

		const Vector startPos = actorPos + Vector( 0.f, 0.f, 2.f );
		const Vector endPos = targetPos + Vector( 0.f, 0.f, 2.f );
		m_isMoving = IsReachable( mac, startPos, endPos );
		if ( !m_isMoving )
		{
			m_actor->ActionEnded( ActorAction_MovingOutNavdata, ActorActionResult_Failed );
			Stop();
			Cleanup();
			return false;
		}
	}
	return true;
}

void ActorActionMoveOutsideNavdata::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	struct Local
	{
		static void Stop( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal )
		{
			goal.SetHeadingGoal( agent, Vector2( 0.f, 0.f ) );
			goal.SetDistanceToGoal( 0.f );
		}
	};
	goal.SetFulfilled( false );

	if ( m_isMoving )
	{
		CNode* const targetNode = m_target.Get();
		if ( !targetNode )
		{
			goal.SetFulfilled( true );
			return;
		}

		const Vector3 targetPos = m_targetMAC ? m_targetMAC->GetAgentPosition().AsVector3() : targetNode->GetWorldPosition().AsVector3();
		const Vector& sourcePos = agent.GetAgentPosition();
		
		const Float distSq = ( sourcePos.AsVector2() - targetPos.AsVector2() ).SquareMag();
		if ( distSq <= m_desiredDistance * m_desiredDistance && Abs( sourcePos.Z - targetPos.Z ) < 2.f )
		{
			m_isMoving = false;
			Local::Stop( agent, goal );
			return;
		}

		// calculate movement parameters
		const Vector2 diff = targetPos.AsVector2() - sourcePos.AsVector2();
		const Float destinationDistance = diff.Mag();

		const Vector2 velocity = destinationDistance > 0.f ? ( diff * ( 1.f / destinationDistance ) ) : Vector2( 0.f, 0.f );
		const Float orientation = EulerAngles::YawFromXY( velocity.X, velocity.Y );

		goal.SetOrientationGoal( agent, orientation );
		goal.SetHeadingGoal( agent, velocity );
		goal.SetGoalPosition( targetPos );
		goal.SetGoalTolerance( m_desiredDistance );
		goal.SetDistanceToGoal( destinationDistance );
		goal.SetDestinationPosition( targetPos );
		if ( Abs( targetPos.Z - sourcePos.Z ) < 2.f )
		{
			goal.SetDistanceToDestination( destinationDistance );
		}
		goal.MatchOrientationWithHeading( true );
		goal.SnapToMinimalSpeed( true );
		goal.SetGoalTargetNode( m_target );
	}
}

String ActorActionMoveOutsideNavdata::GetDescription() const
{
	if ( const CNode* const target = m_target.Get() )
	{
		return String::Printf( TXT( "Trying to move to target %s not being on navdata" ), target->GetFriendlyName().AsChar() );
	}
	else
	{
		return TXT( "Moving outside navdata UNSTREAMED node!" );
	}
}

Bool CActor::ActionMoveOutsideNavdata( const CNode* target, EMoveType moveType, Float absSpeed, Float radius )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionMoveOutsideNavdata.StartMove( target, moveType, absSpeed, radius ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionMoveOutsideNavdata;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_MovingOutNavdata;

	OnActionStarted( ActorAction_MovingOutNavdata );

	return true;
}