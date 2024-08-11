#pragma once

/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/


#include "behTreeMetanode.h"
#include "behTreeVars.h"
#include "aiParameters.h"
#include "behTreeNodeTemplate.h"

class CBehTreeNodeConditionalTreeNPCTypeDefinition : public CBehTreeNodeBaseConditionalTreeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionalTreeNPCTypeDefinition, CBehTreeNodeBaseConditionalTreeDefinition, IBehTreeNodeInstance, NPCTypeConditionalTree );
protected:
	ENPCGroupType		m_npcType;

	Bool CheckCondition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;	

public:
	CBehTreeNodeConditionalTreeNPCTypeDefinition()
		: m_npcType( ENGT_Commoner )
		{}

	String							GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionalTreeNPCTypeDefinition );
	PARENT_CLASS( CBehTreeNodeBaseConditionalTreeDefinition );
	PROPERTY_EDIT( m_npcType, TXT("Conditional value") );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();