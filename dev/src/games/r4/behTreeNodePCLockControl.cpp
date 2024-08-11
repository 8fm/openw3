/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodePCLockControl.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodePCLockControlDecoratorDefinition )


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodePCLockControlDecoratorInstance
///////////////////////////////////////////////////////////////////////////////

Bool CBehTreeNodePCLockControlDecoratorInstance::Activate()
{
	return Super::Activate();
}
void CBehTreeNodePCLockControlDecoratorInstance::Deactivate()
{
	Super::Deactivate();
}