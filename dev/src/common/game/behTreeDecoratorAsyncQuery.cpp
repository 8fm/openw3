/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorAsyncQuery.h"



Bool IBehTreeNodeDecoratorAsyncQueryInstance::IsAvailable()
{
	if ( m_queryTimeout > m_owner->GetLocalTime() )
	{
		return false;
	}
	return Super::IsAvailable();
}
Int32 IBehTreeNodeDecoratorAsyncQueryInstance::Evaluate()
{
	if ( m_queryTimeout > m_owner->GetLocalTime() )
	{
		return -1;
	}
	return Super::Evaluate();
}

Bool IBehTreeNodeDecoratorAsyncQueryInstance::Activate()
{
	switch( StartQuery() )
	{
	case STATUS_IN_PROGRESS:
		break;
	case STATUS_SUCCESS:
		m_queryTimeout = m_queryValidFor + m_owner->GetLocalTime();
		return false;
	default:
		ASSERT( false );
		ASSUME( false );
	case STATUS_FAILURE:
		m_queryTimeout = m_queryFailedDelay + m_owner->GetLocalTime();
		return false;
	}

	return Super::Activate();
}
void IBehTreeNodeDecoratorAsyncQueryInstance::Update()
{
	switch( UpdateQuery() )
	{
	case STATUS_IN_PROGRESS:
		break;
	case STATUS_SUCCESS:
		m_queryTimeout = m_queryValidFor + m_owner->GetLocalTime();
		Complete( BTTO_SUCCESS );
		return;
	default:
		ASSERT( false );
		ASSUME( false );
	case STATUS_FAILURE:
		m_queryTimeout = m_queryFailedDelay + m_owner->GetLocalTime();
		Complete( BTTO_FAILED );
		return;
	}
	Super::Update();
}