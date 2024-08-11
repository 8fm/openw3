/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeCustomMoveData.h"
#include "behTreeDecoratorAsyncQuery.h"
#include "behTreeGuardAreaData.h"
#include "behTreeWalkableQueryData.h"

class IBehTreeNodeDecoratorWalkableSpotQueryInstance;
class CBehTreeNodeDecoratorWalkableSpotRingQueryInstance;
class CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance;
class CBehTreeNodeDecoratorWalkableSpotResultInstance;

///////////////////////////////////////////////////////////////////////////////
// General walkable spot async query interface
class IBehTreeNodeDecoratorWalkableSpotQueryDefinition : public IBehTreeNodeDecoratorAsyncQueryDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeDecoratorWalkableSpotQueryDefinition, IBehTreeNodeDecoratorAsyncQueryDefinition, IBehTreeNodeDecoratorWalkableSpotQueryInstance, WalkableSpotQuery );

protected:
	Bool						m_useCombatTargetAsReference;
	Bool						m_useTargetAsSourceSpot;
	Bool						m_stayInGuardArea;
public:
	IBehTreeNodeDecoratorWalkableSpotQueryDefinition()
		: m_useCombatTargetAsReference( false )
		, m_useTargetAsSourceSpot( false )
		, m_stayInGuardArea( false )										{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeDecoratorWalkableSpotQueryDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorAsyncQueryDefinition )
	PROPERTY_EDIT( m_useCombatTargetAsReference, TXT("Use combat or action target as query reference pos") )
	PROPERTY_EDIT( m_useTargetAsSourceSpot, TXT("Use selected referece target (otherwise self) as sourse spot") )
	PROPERTY_EDIT( m_stayInGuardArea, TXT("Only compute result from withing guard area") )
END_CLASS_RTTI();

class IBehTreeNodeDecoratorWalkableSpotQueryInstance : public IBehTreeNodeDecoratorAsyncQueryInstance
{
	typedef IBehTreeNodeDecoratorAsyncQueryInstance Super;
protected:
	Bool							m_useCombatTargetAsReference;
	Bool							m_useTargetAsSourceSpot;
	CBehTreePositioningRequestPtr	m_requestPtr;
	CBehTreeGuardAreaDataPtr		m_guardDataPtr;

	EQueryStatus				UpdateQuery() override;
public:
	typedef IBehTreeNodeDecoratorWalkableSpotQueryDefinition Definition;

	IBehTreeNodeDecoratorWalkableSpotQueryInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

///////////////////////////////////////////////////////////////////////////////
// General walkable spot query in given ring (min-max dist)
class CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition : public IBehTreeNodeDecoratorWalkableSpotQueryDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition, IBehTreeNodeDecoratorWalkableSpotQueryDefinition, CBehTreeNodeDecoratorWalkableSpotRingQueryInstance, WalkableSpotRingQuery );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	SPositioningFilter			m_filter;
	
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorWalkableSpotQueryDefinition )
	PROPERTY_EDIT( m_filter, TXT("Navigation query settings") );
END_CLASS_RTTI();


class CBehTreeNodeDecoratorWalkableSpotRingQueryInstance : public IBehTreeNodeDecoratorWalkableSpotQueryInstance
{
	typedef IBehTreeNodeDecoratorWalkableSpotQueryInstance Super;
protected:
	SPositioningFilter				m_filter;

	EQueryStatus				StartQuery() override;
public:
	typedef CBehTreeNodeDecoratorWalkableSpotRingQueryDefinition Definition;

	CBehTreeNodeDecoratorWalkableSpotRingQueryInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_filter( def.m_filter )												{}
};

///////////////////////////////////////////////////////////////////////////////
// General closest walkable spot from target position
class CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition : public IBehTreeNodeDecoratorWalkableSpotQueryDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition, IBehTreeNodeDecoratorWalkableSpotQueryDefinition, CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance, WalkableSpotClosestQuery );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	SClosestSpotFilter			m_filter;

};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorWalkableSpotQueryDefinition )
	PROPERTY_EDIT( m_filter, TXT("Navigation query settings") );
END_CLASS_RTTI();


class CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance : public IBehTreeNodeDecoratorWalkableSpotQueryInstance
{
	typedef IBehTreeNodeDecoratorWalkableSpotQueryInstance Super;
protected:
	SClosestSpotFilter				m_filter;

	EQueryStatus				StartQuery() override;
public:
	typedef CBehTreeNodeDecoratorWalkableSpotClosestQueryDefinition Definition;

	CBehTreeNodeDecoratorWalkableSpotClosestQueryInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_filter( def.m_filter )												{}
};

///////////////////////////////////////////////////////////////////////////////
// Consume walkable spot query
// Common interface for ai asynchronous query collect result nodes
class CBehTreeNodeDecoratorWalkableSpotResultDefintion : public IBehTreeNodeDecoratorAsyncResultDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorWalkableSpotResultDefintion, IBehTreeNodeDecoratorAsyncResultDefinition, CBehTreeNodeDecoratorWalkableSpotResultInstance, WalkableSpotResult );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorWalkableSpotResultDefintion )
	PARENT_CLASS( IBehTreeNodeDecoratorAsyncResultDefinition )
END_CLASS_RTTI()

class CBehTreeNodeDecoratorWalkableSpotResultInstance : public IBehTreeNodeDecoratorAsyncResultInstance
{
	typedef IBehTreeNodeDecoratorAsyncResultInstance Super;
protected:
	CBehTreePositioningRequestPtr	m_requestPtr;
	CBehTreeCustomMoveDataPtr		m_customMoveDataPtr;
public:
	typedef CBehTreeNodeDecoratorWalkableSpotResultDefintion Definition;

	CBehTreeNodeDecoratorWalkableSpotResultInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	Bool Activate() override;


};
