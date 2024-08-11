#include "build.h"
#include "behTreeDecoratorUninterruptable.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorUninterruptableDefinition );

Bool CBehTreeDecoratorUninterruptableInstance::Interrupt()
{
	return false;
}

