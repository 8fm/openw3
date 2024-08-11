/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"

class IBehTreeNodeDecoratorAsyncQueryInstance;
class IBehTreeNodeDecoratorAsyncResultInstance;


// Common interface for ai asyncronous queries
class IBehTreeNodeDecoratorAsyncQueryDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeDecoratorAsyncQueryDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeDecoratorAsyncQueryInstance, AsyncQuery );
protected:
	CName									m_queryName;
	Float									m_queryValidFor;
	Float									m_queryFailedDelay;
}; 

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeDecoratorAsyncQueryDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_queryName ,TXT("AI storage query name") )
	PROPERTY_EDIT( m_queryValidFor, TXT("Time that query is valid") )
END_CLASS_RTTI()


class IBehTreeNodeDecoratorAsyncQueryInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	enum EQueryStatus
	{
		STATUS_IDLE,
		STATUS_IN_PROGRESS,
		STATUS_SUCCESS,
		STATUS_FAILURE,
	};

	// setup
	Float									m_queryValidFor;
	Float									m_queryFailedDelay;
	// runtime
	Float									m_queryTimeout;

	virtual EQueryStatus					StartQuery()							= 0;
	virtual EQueryStatus					UpdateQuery()							= 0;
public:
	typedef IBehTreeNodeDecoratorAsyncQueryDefinition Definition;

	IBehTreeNodeDecoratorAsyncQueryInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_queryValidFor( def.m_queryValidFor )
		, m_queryFailedDelay( def.m_queryFailedDelay )
		, m_queryTimeout( 0.f )														{}

	Bool									IsAvailable() override;
	Int32									Evaluate() override;

	Bool									Activate() override;
	void									Update() override;

};

// Common interface for ai asynchronous query collect result nodes
class IBehTreeNodeDecoratorAsyncResultDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeDecoratorAsyncResultDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeDecoratorAsyncResultInstance, AsyncResult );
protected:
	CName									m_queryName;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeDecoratorAsyncResultDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_queryName ,TXT("AI storage query name")  )
END_CLASS_RTTI()

class IBehTreeNodeDecoratorAsyncResultInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
public:
	typedef IBehTreeNodeDecoratorAsyncResultDefinition Definition;

	IBehTreeNodeDecoratorAsyncResultInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )										{}
};