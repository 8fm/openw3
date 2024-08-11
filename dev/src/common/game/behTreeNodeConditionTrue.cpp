/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeConditionTrue.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeConditionTrueDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeConditionTrueInstance
///////////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeConditionTrueInstance::ConditionCheck()
{
	return true;
}

