/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionSpeedEngineValInstance;

///////////////////////////////////////////////////////////////////////////////
// Definition
class CBehTreeNodeConditionSpeedEngineValDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionSpeedEngineValDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionSpeedEngineValInstance, ConditionMoveSpeedEngineVal );
protected:	
	CBehTreeValFloat			m_speed;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionSpeedEngineValDefinition() 
		: m_speed( 0.5f )												{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionSpeedEngineValDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_speed, TXT("Reference engine val (behavior graph speed input)") );
END_CLASS_RTTI();


///////////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionSpeedEngineValInstance : public CBehTreeNodeConditionInstance
{
private:
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Float					m_speed;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionSpeedEngineValDefinition Definition;

	CBehTreeNodeConditionSpeedEngineValInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_speed( def.m_speed.GetVal( context ) )								{}
};