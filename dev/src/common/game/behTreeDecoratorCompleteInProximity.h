/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"

class CBehTreeNodeDecoratorCompleteInProximityInstance;

///////////////////////////////////////////////////////////////////////////////
// Definition
class CBehTreeNodeDecoratorCompleteInProximityDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorCompleteInProximityDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeDecoratorCompleteInProximityInstance, CompleteInProximity )
protected:
	CBehTreeValFloat									m_distance;
	Bool												m_useCombatTarget;
	Bool												m_3D;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorCompleteInProximityDefinition()
		: m_distance( 1.f )
		, m_useCombatTarget( true )
		, m_3D( false )															{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorCompleteInProximityDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_distance, TXT("Complete in distance") )
	PROPERTY_EDIT( m_useCombatTarget, TXT("Use combat or action target")  )
	PROPERTY_EDIT( m_3D, TXT("3D/2D distance test") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeDecoratorCompleteInProximityInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

protected:
	Float												m_distanceSq;

public:
	typedef CBehTreeNodeDecoratorCompleteInProximityDefinition Definition;

	CBehTreeNodeDecoratorCompleteInProximityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_distanceSq( def.m_distance.GetVal( context ) )						{ m_distanceSq *= m_distanceSq; }
};

template< class FunDistance, class FunTarget >
class TBehTreeNodeDecoratorCompleteInProximityInstance : public CBehTreeNodeDecoratorCompleteInProximityInstance
{
	typedef CBehTreeNodeDecoratorCompleteInProximityInstance Super;
public:
	TBehTreeNodeDecoratorCompleteInProximityInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}

	void Update() override;
};