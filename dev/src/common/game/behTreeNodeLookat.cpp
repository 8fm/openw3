/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeLookat.h"

#include "behTreeInstance.h"

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLookAtDefinition
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeDecoratorLookAtDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_player )
	{
		return new CBehTreeNodeDecoratorLookAtPlayerInstance( *this, owner, context, parent );
	}
	if ( m_actionTarget )
	{
		return new CBehTreeNodeDecoratorLookAtActionTargetInstance( *this, owner, context, parent );
	}
	if ( m_combatTarget )
	{
		return new CBehTreeNodeDecoratorLookAtCombatTargetInstance( *this, owner, context, parent );
	}
	if ( m_reactionTarget )
	{
		return new CBehTreeNodeDecoratorLookAtReactionTargetInstance( *this, owner, context, parent );
	}
	return new CBehTreeNodeDecoratorLookAtNamedTargetInstance( *this, owner, context, parent );
}



///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeDecoratorLookAtInstance
///////////////////////////////////////////////////////////////////////////////

Bool IBehTreeNodeDecoratorLookAtInstance::Activate()
{
	if ( Super::Activate() )
	{
		CNode* node = GetLookatTarget();

		if ( node )
		{
			m_owner->GetActor()->EnableDynamicLookAt( node, 1024.f );
		}

		return true;
	}
	return false;
}
void IBehTreeNodeDecoratorLookAtInstance::Deactivate()
{
	CNode* node = GetLookatTarget();

	if ( node )
	{
		m_owner->GetActor()->EnableDynamicLookAt( node, m_durationPostDeactivation );
	}

	Super::Deactivate();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLookAtPlayerInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeDecoratorLookAtPlayerInstance::GetLookatTarget()
{
	return GGame->GetPlayerEntity();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLookAtActionTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeDecoratorLookAtActionTargetInstance::GetLookatTarget()
{
	return m_owner->GetActionTarget();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLookAtCombatTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeDecoratorLookAtCombatTargetInstance::GetLookatTarget()
{
	return m_owner->GetCombatTarget();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLookAtReactionTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeDecoratorLookAtReactionTargetInstance::GetLookatTarget()
{
	return m_owner->GetNamedTarget( CNAME( ReactionTarget ) );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorLookAtNamedTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeDecoratorLookAtNamedTargetInstance::GetLookatTarget()
{
	return m_owner->GetNamedTarget( m_namedTarget );
}



///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicLookAtDefinition
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeAtomicLookAtDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	if ( m_player )
	{
		return new CBehTreeNodeAtomicLookAtPlayerInstance( *this, owner, context, parent );
	}
	if ( m_actionTarget )
	{
		return new CBehTreeNodeAtomicLookAtActionTargetInstance( *this, owner, context, parent );
	}
	if ( m_combatTarget )
	{
		return new CBehTreeNodeAtomicLookAtCombatTargetInstance( *this, owner, context, parent );
	}
	if ( m_reactionTarget )
	{
		return new CBehTreeNodeAtomicLookAtReactionTargetInstance( *this, owner, context, parent );
	}
	return new CBehTreeNodeAtomicLookAtNamedTargetInstance( *this, owner, context, parent );
}



///////////////////////////////////////////////////////////////////////////////
// IBehTreeNodeAtomicLookAtInstance
///////////////////////////////////////////////////////////////////////////////

void IBehTreeNodeAtomicLookAtInstance::Update()
{
	CNode* node = GetLookatTarget();
	if ( node )
	{
		m_owner->GetActor()->EnableDynamicLookAt( node, m_duration );
	}
	Super::Update();
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicLookAtPlayerInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeAtomicLookAtPlayerInstance::GetLookatTarget()
{
	return GGame->GetPlayerEntity();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicLookAtActionTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeAtomicLookAtActionTargetInstance::GetLookatTarget()
{
	return m_owner->GetActionTarget();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicLookAtCombatTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeAtomicLookAtCombatTargetInstance::GetLookatTarget()
{
	return m_owner->GetCombatTarget();
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicLookAtReactionTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeAtomicLookAtReactionTargetInstance::GetLookatTarget()
{
	return m_owner->GetNamedTarget( CNAME( ReactionTarget ) );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicLookAtNamedTargetInstance
///////////////////////////////////////////////////////////////////////////////

CNode* CBehTreeNodeAtomicLookAtNamedTargetInstance::GetLookatTarget()
{
	return m_owner->GetNamedTarget( m_namedTarget );
}

