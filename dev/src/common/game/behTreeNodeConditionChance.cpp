/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionChance.h"

#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionChanceDefinition )


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionChanceInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionChanceInstance::ConditionCheck()
{
	Float time = m_owner->GetLocalTime();
	if ( m_lastTestTimeout < time )
	{
		m_lastTestOutcome = m_chance >= GEngine->GetRandomNumberGenerator().Get< Float >();
		m_lastTestTimeout = time + m_resultValidFor;
	}

	return m_lastTestOutcome;
}