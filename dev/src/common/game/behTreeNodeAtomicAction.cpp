/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeAtomicAction.h"


Bool CBehTreeNodeAtomicActionDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	if ( !IsMyInstance( node ) )
	{
		return false;
	}

	node->OnSpawn( *this, context );

	return true;
}