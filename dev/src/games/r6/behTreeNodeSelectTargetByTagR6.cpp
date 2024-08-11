/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "behTreeNodeSelectTargetByTagR6.h"
#include "../../common/game/behTreeInstance.h"
#include "../../common/engine/tagManager.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetByTagDecoratorR6Definition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeSelectTargetByTagDecoratorR6Definition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetByTagDecoratorR6Instance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSelectTargetByTagDecoratorR6Instance::IsAvailable()
{
	// try to get the target
	R6_ASSERT( GGame->GetActiveWorld() );
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	R6_ASSERT( tagMgr );
	CNode* target = tagMgr->GetTaggedNode( m_tag );

	// set the target temporarly before checking
	const THandle< CNode > tmp = m_owner->GetActionTarget();
	m_owner->SetActionTarget( target );
	
	// do the test
	const Bool testResult = Super::IsAvailable();

	// restore previous value
	m_owner->SetActionTarget( tmp );

	if( !testResult )
	{
		DebugNotifyAvailableFail();
	}

	// return the result
	return testResult;
}

Int32 CBehTreeNodeSelectTargetByTagDecoratorR6Instance::Evaluate()
{
	// try to get the target
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	CNode* target = tagMgr->GetTaggedNode( m_tag );

	// set the target temporarly before checking
	const THandle< CNode > tmp = m_owner->GetActionTarget();
	m_owner->SetActionTarget( target );
	
	// do the test
	const Int32 testResult = Super::Evaluate();

	// restore previous value
	m_owner->SetActionTarget( tmp );

	if( testResult <= 0 )
	{
		DebugNotifyAvailableFail();
	}

	// return the result
	return testResult;
}

