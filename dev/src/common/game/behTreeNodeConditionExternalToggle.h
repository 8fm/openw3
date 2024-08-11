/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionExternalToggleInstance;

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionExternalToggleDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionExternalToggleDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionExternalToggleInstance, ConditionExternalToggle );
protected:
	CName						m_switchName;
	CBehTreeValBool				m_initialValue;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionExternalToggleDefinition()
		: m_initialValue( true )										{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionExternalToggleDefinition)
	PARENT_CLASS(CBehTreeNodeConditionDefinition)
	PROPERTY_EDIT( m_switchName, TXT("External switch name (gameplay event name)") )
	PROPERTY_EDIT( m_initialValue, TXT("Initial value") )
END_CLASS_RTTI()


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionExternalToggleInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CName					m_switchName;
	Bool					m_initialValue;
	Bool					m_value;

	Bool ConditionCheck() override;
public:
	enum EEventValue
	{
		TOGGLE_OFF						= false,
		TOGGLE_ON						= true,
		TOGGLE_RESET_TO_DEFAULTS
	};

	typedef CBehTreeNodeConditionExternalToggleDefinition Definition;

	CBehTreeNodeConditionExternalToggleInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnDestruction() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
};