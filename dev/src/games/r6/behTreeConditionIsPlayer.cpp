/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeConditionIsPlayer.h"
#include "r6behTreeInstance.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsPlayerDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance*	CBehTreeNodeConditionIsPlayerDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionIsPlayerInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeConditionIsPlayerInstance::CBehTreeNodeConditionIsPlayerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodeConditionInstance( def, owner, context, parent )
{
}

Bool CBehTreeNodeConditionIsPlayerInstance::ConditionCheck()
{
	return GGame->GetPlayerEntity() == m_owner->FindParent< CEntity >();
}

