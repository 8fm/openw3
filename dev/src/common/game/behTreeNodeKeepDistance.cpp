/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeKeepDistance.h"

#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "movementGoal.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeKeepDistanceDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeKeepDistanceInstance::CBehTreeNodeKeepDistanceInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeCustomSteeringInstance( def, owner, context, parent )
	, m_distance( def.m_distance.GetVal( context ) )
	, m_notFacingTarget( def.m_notFacingTarget.GetVal( context ) )
	, m_exit( false )
{
}

Bool CBehTreeNodeKeepDistanceInstance::Activate()
{
	if( !IsInRange() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	return Super::Activate();
}

void CBehTreeNodeKeepDistanceInstance::Update()
{
	if( m_exit )
	{
		m_exit = false;
		Complete( BTTO_SUCCESS );
	}
}

void CBehTreeNodeKeepDistanceInstance::Deactivate()
{
	Super::Deactivate();
}

Bool CBehTreeNodeKeepDistanceInstance::IsInRange() const
{
	CNewNPC* npc = m_owner->GetNPC();
	if ( npc )
	{
		CActor* target = npc->GetTarget();

		if(	target )
		{
			Float distance = target->GetPosition().DistanceTo(npc->GetPosition());
			if( distance <= m_distance)
			{
				return true;
			}
		}
	}

	return false;
}

Bool CBehTreeNodeKeepDistanceInstance::IsAvailable()
{
	return true;
}

Bool CBehTreeNodeKeepDistanceInstance::IsFinished() const
{
	return false;
}

void CBehTreeNodeKeepDistanceInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFulfilled( false );

	CNewNPC* npc = m_owner->GetNPC();
	if( npc && npc->GetTarget() )
	{
		CActor* target = npc->GetTarget();
		Vector position = npc->GetPosition();

		Vector2 diff = target->GetPosition().AsVector2() - position.AsVector2();
		Float targetDistance = diff.Mag();

		// Distance
		Vector2 keepDistanceVector( 0.0f, 0.0f );
		if ( targetDistance > m_distance )
		{
			goal.SetFulfilled(true);
			keepDistanceVector = Vector::ZEROS;
			m_exit = true;
		}	 
		else if ( targetDistance < m_distance )
		{
			keepDistanceVector = -diff.Normalized();
		}
		
		// Orientation - always face the enemy
		Vector vec = target->GetWorldPosition() - npc->GetWorldPosition();
		if( m_notFacingTarget )
		{
			vec *= -1;
		}
		EulerAngles rot = vec.ToEulerAngles();
		Float orientation = rot.Yaw;

		Float mag = keepDistanceVector.Mag();
		goal.SetSpeedGoal( keepDistanceVector == Vector::ZEROS ? 0.0f :  m_moveSpeed );
		goal.SetOrientationGoal( agent, orientation );
		goal.SetHeadingGoal( agent, keepDistanceVector, false );
		goal.SetDistanceToGoal( mag );
	}
	else
	{
		goal.SetFulfilled( true );
	}
}
