/*
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "../../common/game/behTreeNodeTemplate.h"


class CBehTreeMetanodeDecorateOnCombatstyle : public IBehTreeNodeConditionalBaseNodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeMetanodeDecorateOnCombatstyle, IBehTreeNodeConditionalBaseNodeDefinition, IBehTreeNodeInstance, MetanodeDecorateOnCombatstyle );
protected:
	CName							m_behaviorGraphVarName;
	Int32							m_combatStyleId;

	Bool							CheckCondition( CBehTreeSpawnContext& context ) const override;
public:
	CBehTreeMetanodeDecorateOnCombatstyle()
		: m_combatStyleId( 0 )														{}

	String							GetNodeCaption() const override;
};


BEGIN_CLASS_RTTI( CBehTreeMetanodeDecorateOnCombatstyle )
	PARENT_CLASS( IBehTreeNodeConditionalBaseNodeDefinition )
	PROPERTY_EDIT( m_behaviorGraphVarName, TXT("Combat style id variable name") )
	PROPERTY_EDIT( m_combatStyleId, TXT("Combat style id") )
END_CLASS_RTTI()