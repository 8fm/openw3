/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodePlayerStateSelector.h"
#include "r6behTreeInstance.h"

IBehTreeNodeCompositeInstance* CBehTreeNodePlayerStateSelectorDefinition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}

CBehTreeNodePlayerStateSelectorInstance::CBehTreeNodePlayerStateSelectorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) 
	: IBehTreeNodeCompositeInstance( def, owner, context, parent )
{
}

Bool CBehTreeNodePlayerStateSelectorInstance::Activate()
{
	if( Think() )
	{
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}

void CBehTreeNodePlayerStateSelectorInstance::OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome )
{
	m_activeChild = INVALID_CHILD;
	if ( false == Think() )
	{
		Complete( BTTO_FAILED );
	}
}

Bool CBehTreeNodePlayerStateSelectorInstance::IsAvailable()
{
	R6_ASSERT( m_owner->IsA< CR6BehTreeInstance >() );
	CR6BehTreeInstance* behTreeInstance = static_cast< CR6BehTreeInstance* > ( m_owner );

	if ( GGame->GetPlayerEntity() == behTreeInstance->FindParent< CEntity >() )
	{
		// our parent entity is currently a player
		// node is available if any of the children (any state) is available
		for ( Uint32 i = 0; i < m_children.Size(); ++i )
		{
			if ( m_children[ i ]->IsAvailable() )
			{
				return true;
			}
		}
	}
	
	DebugNotifyAvailableFail();
	return false;
}

void CBehTreeNodePlayerStateSelectorInstance::Update()
{
	// basicly, re-evaluate every time
	if ( false == Think() )
	{
		return;
	}

	Super::Update();
}

Bool CBehTreeNodePlayerStateSelectorInstance::Think()
{
	Uint32 bestChild = SelectChild();
	if ( bestChild != m_activeChild )
	{
		if ( m_activeChild != INVALID_CHILD )
		{
			if ( false == m_children[ m_activeChild ]->Interrupt() )
			{
				return true;
			}
		}
		
		if ( bestChild != INVALID_CHILD && m_children[ bestChild ]->Activate() )
		{
			m_activeChild = bestChild;
			return true;
		}
		else
		{
			m_activeChild = INVALID_CHILD;
			return false;
		}
	}

	return bestChild != INVALID_CHILD;
}

Uint32 CBehTreeNodePlayerStateSelectorInstance::SelectChild()
{
	for ( Uint32 i = 0; i < m_children.Size(); ++i )
	{
		if ( m_children[ i ]->IsAvailable() )
		{
			return i;
		}
	}

	return INVALID_CHILD;
}

