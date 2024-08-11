#pragma once

#include "../../common/game/behTreeDecorator.h"

class CBehTreeNodeConditionInstance;

RED_DECLARE_NAME( VitalSpotActivator );

struct SActiveVitalSpotEventData
{
	DECLARE_RTTI_STRUCT( SActiveVitalSpotEventData );
	CName	m_VSActivatorNodeName;
	Bool	m_result;
};

BEGIN_CLASS_RTTI( SActiveVitalSpotEventData );
END_CLASS_RTTI();

class CBehTreeNodeVitalSpotActiveInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeVitalSpotActiveDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeVitalSpotActiveDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeVitalSpotActiveInstance, VitalSpotActivator );
public:
	CName								m_VSActivatorNodeName;
	Bool								m_activateVitalSpot;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeVitalSpotActiveDefinition);
	PARENT_CLASS(IBehTreeNodeDecoratorDefinition);	
	PROPERTY_EDIT( m_activateVitalSpot, TXT("Activate or deactivated vital spot if onde is active ") );
	PROPERTY_EDIT( m_VSActivatorNodeName, TXT("Name of the activator node") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeVitalSpotActiveInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:


public:
	typedef CBehTreeNodeVitalSpotActiveDefinition Definition;

	CBehTreeNodeVitalSpotActiveInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: IBehTreeNodeDecoratorInstance( def, owner, context, parent ) , m_VSActivatorNodeName( def.m_VSActivatorNodeName ),	m_activateVitalSpot( def.m_activateVitalSpot )
	{}

	CName								m_VSActivatorNodeName;
	Bool								m_activateVitalSpot;
	Bool OnEvent( CBehTreeEvent& e );
};

