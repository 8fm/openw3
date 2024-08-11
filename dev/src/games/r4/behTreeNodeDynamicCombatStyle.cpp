/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDynamicCombatStyle.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeDynamicCombatStyleDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDynamicCombatStyleDefinition
///////////////////////////////////////////////////////////////////////////////
CName CBehTreeNodeDynamicCombatStyleDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return CNAME( AI_DynamicCombatStyle );
}