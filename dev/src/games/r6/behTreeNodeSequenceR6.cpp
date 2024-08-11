/*
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behTreeNodeSequenceR6.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceCheckAvailabilityR6Definition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeCompositeInstance* CBehTreeNodeSequenceCheckAvailabilityR6Definition::SpawnCompositeInstanceInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSequenceCheckAvailabilityR6Instance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSequenceCheckAvailabilityR6Instance::IsAvailable()
{
	if ( m_children.Empty() )
	{
		DebugNotifyAvailableFail();
		return false;
	}

	const Uint32 childToBeTested = ( m_activeChild == INVALID_CHILD ) ? 0 : m_activeChild;
	return m_children[ childToBeTested ]->IsAvailable();
}

Int32 CBehTreeNodeSequenceCheckAvailabilityR6Instance::Evaluate()
{
	if ( m_children.Empty() )
	{
		DebugNotifyAvailableFail();
		return -1;
	}

	const Uint32 childToBeEvaluated = ( m_activeChild == INVALID_CHILD ) ? 0 : m_activeChild;
	return m_children[ childToBeEvaluated ]->Evaluate();
}

