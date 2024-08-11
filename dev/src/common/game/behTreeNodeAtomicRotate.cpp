/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeAtomicRotate.h"
#include "behTreeNode.h"
#include "behTreeInstance.h"

//////////////////////////////////////////////////////////////////////////
////////////////////////       DEFINITION       //////////////////////////
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeBaseRotateToTargetDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
////////////////////////        INSTANCE        //////////////////////////
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeBaseRotateToTargetInstance::CBehTreeNodeBaseRotateToTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_completeOnTargetReached( def.m_completeOnTargetReached )
	, m_currentTarget()
{

}

Bool CBehTreeNodeBaseRotateToTargetInstance::IsAvailable()
{
	if( Check() )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}

Bool CBehTreeNodeBaseRotateToTargetInstance::Check()
{
	if ( m_completeOnTargetReached )
	{
		CNode* target = GetTarget();
		CActor* actor = m_owner->GetActor();
		if ( target == nullptr || actor->IsRotatedTowards( target, 10.f ) )
		{
			return false;
		}
	}
	return true;
}

Bool CBehTreeNodeBaseRotateToTargetInstance::Activate()
{
	CNode* target = GetTarget();
	CActor* actor = m_owner->GetActor();

	if ( target == nullptr )
	{
		DebugNotifyActivationFail();
		return false;
	}
	
	m_currentTarget = target;

	if ( actor->ActionRotateTo( target->GetWorldPositionRef(), m_completeOnTargetReached ) == false )
	{
		DebugNotifyActivationFail();
		return false;
	}
	
	if ( Super::Activate() == false )
	{
		actor->ActionCancelAll();
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}

void CBehTreeNodeBaseRotateToTargetInstance::Update()
{
	const CNode*const target = m_currentTarget.Get();
	if ( target == nullptr )
	{
		Complete( BTTO_FAILED );
		return;
	}
	CActor*const actor = m_owner->GetActor();
	if ( actor->GetActionType() != ActorAction_Rotating )
	{
		Complete( BTTO_SUCCESS );
		return;
	}

	if ( actor->ActionRotateTo_Update( target->GetWorldPositionRef(), m_completeOnTargetReached ) == false )
	{
		Complete( BTTO_SUCCESS );
	}
}

void CBehTreeNodeBaseRotateToTargetInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	actor->ActionCancelAll();

	Super::Deactivate();
}

//////////////////////////////////////////////////////////////////////////
//////////   CBehTreeNodeAtomicRotateToTargetDefinition      
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeAtomicRotateToTargetDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
///////////       CBehTreeNodeAtomicRotateToTargetInstance     
//////////////////////////////////////////////////////////////////////////

CBehTreeNodeAtomicRotateToTargetInstance::CBehTreeNodeAtomicRotateToTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_useCombatTarget( def.m_useCombatTarget )
{

}
CNode*const CBehTreeNodeAtomicRotateToTargetInstance::GetTarget()const 
{
	CActor* actor = m_owner->GetActor();
	CNode* target = NULL;
	if ( m_useCombatTarget )
	{
		if ( CNewNPC* npc = Cast< CNewNPC >( actor ) )
		{
			target = npc->GetTarget();
		}
	}
	else
	{
		target = m_owner->GetActionTarget().Get();
	}
	return target;
}
//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicMatchActionTargetRotationDefinition
//////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeAtomicMatchActionTargetRotationDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent /*= NULL */ ) const 
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicMatchActionTargetRotationInstance
//////////////////////////////////////////////////////////////////////////
CBehTreeAtomicMatchActionTargetRotationInstance::CBehTreeAtomicMatchActionTargetRotationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_completeOnTargetReached( def.m_completeOnTargetReached )
{

}


CNode*const CBehTreeAtomicMatchActionTargetRotationInstance::GetTarget()const 
{
	CNode* target = m_owner->GetActionTarget().Get();
	return target;
}
Bool CBehTreeAtomicMatchActionTargetRotationInstance::IsAvailable()
{
	if( Check() )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}
Bool CBehTreeAtomicMatchActionTargetRotationInstance::Check()
{
	if ( m_completeOnTargetReached )
	{
		CNode* target = GetTarget();
		CActor* actor = m_owner->GetActor();
		if ( target == nullptr || actor->IsRotatedTowards( target, 10.f ) )
		{
			return false;
		}
	}
	return true;
}

Bool CBehTreeAtomicMatchActionTargetRotationInstance::Activate()
{
	CNode* target = GetTarget();
	CActor* actor = m_owner->GetActor();

	if ( target == nullptr )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( Check() == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	m_currentTarget			= target;
	const Float targetYaw	= target->GetWorldYaw();
	if ( actor->ActionSetOrientation( targetYaw, m_completeOnTargetReached ) == false )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( Super::Activate() == false )
	{
		actor->ActionCancelAll();
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}
void CBehTreeAtomicMatchActionTargetRotationInstance::Deactivate()
{
	CActor* actor = m_owner->GetActor();
	actor->ActionCancelAll();

	Super::Deactivate();
}

void CBehTreeAtomicMatchActionTargetRotationInstance::Update()
{
	const CNode*const target = m_currentTarget.Get();
	if ( target == nullptr )
	{
		Complete( BTTO_FAILED );
		return;
	}
	CActor*const actor = m_owner->GetActor();
	if ( actor->GetActionType() != ActorAction_Rotating )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	const Float targetYaw	= target->GetWorldYaw();
	if ( actor->ActionSetOrientation_Update( targetYaw, m_completeOnTargetReached ) == false )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
}