#pragma once

#include "behTreeDecorator.h"
#include "behTreeCustomMoveData.h"

class CBehTreeDecoratorSelectFleeDestinationInstance;

class CBehTreeDecoratorSelectFleeDestinationDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorSelectFleeDestinationDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeDecoratorSelectFleeDestinationInstance, SelectFleeDestination );	

protected:
	CBehTreeValFloat m_fleeRadius;
	CBehTreeValFloat m_minDistanceFromDanger;

public:
	CBehTreeDecoratorSelectFleeDestinationDefinition()
		: m_fleeRadius( 80.0f )
		, m_minDistanceFromDanger( 20.0f )
	{}

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeDecoratorSelectFleeDestinationDefinition );
PARENT_CLASS(  IBehTreeNodeDecoratorDefinition );
PROPERTY_EDIT( m_fleeRadius, TXT("Radius for flee destination selection") );
PROPERTY_EDIT( m_minDistanceFromDanger, TXT("Distance from danger (attackers) to trigger flee") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////////////////////////////////

class CBehTreeDecoratorSelectFleeDestinationInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCustomMoveDataPtr	m_customMoveData;
	Float						m_fleeRadius;
	Float						m_minDistanceFromDangerSq;

public:
	typedef CBehTreeDecoratorSelectFleeDestinationDefinition Definition;

	CBehTreeDecoratorSelectFleeDestinationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool IsAvailable() override;
	Bool Activate() override;
	void Deactivate() override;
	void OnSubgoalCompleted( eTaskOutcome outcome ) override;
	Bool SelectNextPoint();
};

