#pragma once

#include "../../common/game/behTreeDynamicNode.h"


//////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseIdleDynamicRootDefinition
class CBehTreeNodeBaseIdleDynamicRootInstance;
class CBehTreeNodeBaseIdleDynamicRootDefinition : public IBehTreeDynamicNodeBaseDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeBaseIdleDynamicRootDefinition, IBehTreeDynamicNodeBaseDefinition, CBehTreeNodeBaseIdleDynamicRootInstance, BaseIdleRootDynamic );
protected:
	THandle< CAITree >		m_defaultIdleTree;
public:
	CBehTreeNodeBaseIdleDynamicRootDefinition() {}
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeBaseIdleDynamicRootDefinition );
	PARENT_CLASS( IBehTreeDynamicNodeBaseDefinition );
	PROPERTY_INLINED( m_defaultIdleTree, TXT("Default idle tree reference (no external parametrization!!!)") );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseIdleDynamicRootInstance
class CBehTreeNodeBaseIdleDynamicRootInstance : public CBehTreeDynamicNodeInstance
{
	typedef CBehTreeDynamicNodeInstance Super;

protected:
	Bool								m_isBeingInterrupted;

	void RequestChildDespawn() override;

public:
	typedef IBehTreeDynamicNodeBaseDefinition Definition;

	CBehTreeNodeBaseIdleDynamicRootInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeDynamicNodeInstance( def, owner, context, parent )	{}

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	void Deactivate() override;
};



//////////////////////////////////////////////////////////////////////
// CBehTreeNodeIdleDynamicRootDefinition
class CBehTreeNodeIdleDynamicRootInstance;
class CBehTreeNodeIdleDynamicRootDefinition : public CBehTreeNodeBaseIdleDynamicRootDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeIdleDynamicRootDefinition, CBehTreeNodeBaseIdleDynamicRootDefinition, CBehTreeNodeIdleDynamicRootInstance, IdleRootDynamic );
protected:
	
public:
	CBehTreeNodeIdleDynamicRootDefinition() {}

	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	IAITree* GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeIdleDynamicRootDefinition );
	PARENT_CLASS( CBehTreeNodeBaseIdleDynamicRootDefinition );
END_CLASS_RTTI();

/////////////////////////////////////////////////////////////////////
// CBehTreeNodeIdleDynamicRootInstance
class CBehTreeNodeIdleDynamicRootInstance : public CBehTreeNodeBaseIdleDynamicRootInstance
{
	typedef CBehTreeNodeBaseIdleDynamicRootInstance Super;
public:
	typedef CBehTreeNodeIdleDynamicRootDefinition Definition;

	CBehTreeNodeIdleDynamicRootInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: CBehTreeNodeBaseIdleDynamicRootInstance( def, owner, context, parent )	{}
};

