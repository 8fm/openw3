/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeAreaSelection.h"
#include "behTreeNodeSelectTargetByTag.h"

class CBehTreeNodeSelectTargetByTagInAreaDecoratorInstance;



class CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition : public CBehTreeNodeSelectTargetByTagDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition, CBehTreeNodeSelectTargetByTagDecoratorDefinition, CBehTreeNodeSelectTargetByTagInAreaDecoratorInstance, SelectTargetByTagInAreaDecorator )
protected:
	SBehTreeAreaSelection					m_areaSelection;
	CBehTreeValBool							m_getClosest;
	CBehTreeValBool							m_reselectOnActivate;

	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition()
		: m_getClosest( false )															{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition )
	PARENT_CLASS( CBehTreeNodeSelectTargetByTagDecoratorDefinition )
	PROPERTY_EDIT( m_areaSelection, TXT("Area selection method") )
	PROPERTY_EDIT( m_getClosest, TXT("Get closest or random target") )
	PROPERTY_EDIT( m_reselectOnActivate, TXT("Search for target each time node gets activated") )
END_CLASS_RTTI()

class CBehTreeNodeSelectTargetByTagInAreaDecoratorInstance : public CBehTreeNodeSelectTargetByTagDecoratorInstance
{
	typedef CBehTreeNodeSelectTargetByTagDecoratorInstance Super;
protected:
	SBehTreeSelectedAreaInstance			m_area;
	THandle< CNode >						m_node;										// runtime: cached last target
	Bool									m_getClosest;
	Bool									m_reselectOnActivate;

	CNode*									ComputeTarget();
public:
	typedef CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition Definition;

	CBehTreeNodeSelectTargetByTagInAreaDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_getClosest( def.m_getClosest.GetVal( context ) )
		, m_reselectOnActivate( def.m_reselectOnActivate.GetVal( context ) )			{ def.m_areaSelection.InitInstance( owner, context, m_area ); }

	Bool									Activate() override;

};