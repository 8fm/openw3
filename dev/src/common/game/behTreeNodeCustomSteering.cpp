/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeCustomSteering.h"

#include "behTreeNode.h"
#include "behTreeInstance.h"
#include "movementGoal.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeCustomSteeringDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeCustomSteeringInstance::CBehTreeNodeCustomSteeringInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeAtomicActionInstance( def, owner, context, parent )
	, CBehTreeSteeringGraphCommonInstance( def, owner, context )
	, m_moveType( def.m_moveType.GetVal( context ) )
	, m_moveSpeed( def.m_moveSpeed.GetVal( context ) )
{
}

Bool CBehTreeNodeCustomSteeringInstance::Activate()
{
	CActor* owner = m_owner->GetActor();
	if ( !owner )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( !owner->ActionCustomSteer( this, GetMoveType(), m_moveSpeed, MFA_EXIT ) )
	{
		DebugNotifyActivationFail();
		return false;
	}

	ActivateSteering( m_owner );

	if ( !Super::Activate() )
	{
		owner->ActionCancelAll();
		DeactivateSteering( m_owner );
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}

void CBehTreeNodeCustomSteeringInstance::Update()
{
	CActor* actor = m_owner->GetActor();
	if( !actor->IsMoving() )
	{
		Complete( BTTO_SUCCESS );
	}
}

void CBehTreeNodeCustomSteeringInstance::Deactivate()
{
	Super::Deactivate();

	DeactivateSteering( m_owner );

	CActor* actor = m_owner->GetActor();
	if ( actor )
	{
		actor->ActionCancelAll();
	}
}

void CBehTreeNodeCustomSteeringInstance::UpdateChannels( const CMovingAgentComponent& agent, SMoveLocomotionGoal& goal, Float timeDelta )
{
	goal.SetFulfilled( false );
}

Bool CBehTreeNodeCustomSteeringInstance::IsFinished() const
{
	return false;
}

