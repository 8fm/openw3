/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDynamicFlightIdle.h"

BEHTREE_STANDARD_SPAWNNODE_FUNCTION( CBehTreeNodeFlightIdleDynamicRootDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeFlightIdleDynamicRootDefinition
///////////////////////////////////////////////////////////////////////////////
CName CBehTreeNodeFlightIdleDynamicRootDefinition::StaticEventName()
{
	return CNAME( AI_Load_FlightIdle );
}

CName CBehTreeNodeFlightIdleDynamicRootDefinition::StaticAIParamName()
{
	return CNAME( freeFlight );
}


CName CBehTreeNodeFlightIdleDynamicRootDefinition::GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return StaticEventName();
}
IAITree* CBehTreeNodeFlightIdleDynamicRootDefinition::GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const
{
	return context.GetVal< IAITree* >( StaticAIParamName(), m_defaultIdleTree.Get() );
}