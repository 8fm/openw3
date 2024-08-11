#include "build.h"

#include "behTreeDecoratorActivePriority.h"
#include "behTreeInstance.h"


BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeDecoratorActivePriorityDefinition )
////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorActivePriorityDefinition
////////////////////////////////////////////////////////////////////////
String CBehTreeDecoratorActivePriorityDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("Active priority %d"), m_priority.m_value );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorActivePriorityInstance
////////////////////////////////////////////////////////////////////////
Int32 CBehTreeDecoratorActivePriorityInstance::Evaluate()
{
	// Active? Return child's priority
	if( m_isActive )
	{
		if( m_evaluateChildWhenActive )
		{
			Int32 childPriority = Super::Evaluate();
			if( childPriority == -1 )
			{
				return -1;
			}
		}
		return m_priority;
	}

	// Not active yet? Give them our priority
	return Super::Evaluate();
}

Bool CBehTreeDecoratorActivePriorityInstance::IsAvailable()
{
	if ( m_isActive )
	{
		if( m_priority > 0 )
		{
			return true;
		}
		else
		{
			DebugNotifyAvailableFail();
			return false;
		}
	}

	return Super::IsAvailable();
}