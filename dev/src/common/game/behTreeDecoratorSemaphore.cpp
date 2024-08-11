/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorSemaphore.h"

#include "behTreeInstance.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorSemaphoreDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSemaphoreDefinition
///////////////////////////////////////////////////////////////////////////////
String CBehTreeNodeDecoratorSemaphoreDefinition::GetNodeCaption() const
{
	return String::Printf( TXT("Semaphore '%ls' %s"), m_semaphoreName.AsChar(), m_raise ? TXT("raise") : TXT("lower") );
}

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorSemaphoreInstance
///////////////////////////////////////////////////////////////////////////////
CBehTreeNodeDecoratorSemaphoreInstance::CBehTreeNodeDecoratorSemaphoreInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_ptr( owner, def.m_semaphoreName )
	, m_raise( def.m_raise )
{
}

Bool CBehTreeNodeDecoratorSemaphoreInstance::Activate()
{
	if ( Super::Activate() )
	{
		m_ptr->ModifyCounter( m_raise ? 1 : -1 );
		return true;
	}
	return false;
}

void CBehTreeNodeDecoratorSemaphoreInstance::Deactivate()
{
	m_ptr->ModifyCounter( m_raise ? -1 : 1 );
	Super::Deactivate();
}