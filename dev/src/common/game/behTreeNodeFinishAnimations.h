#pragma once

#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeFinishAnimationsInstance;
class CBehTreeNodeBreakAnimationsInstance;

class CBehTreeNodeFinishAnimationsDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeFinishAnimationsDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeFinishAnimationsInstance, FinishAnimations );
public:
	CBehTreeNodeFinishAnimationsDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeFinishAnimationsDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeFinishAnimationsInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
public:
	typedef CBehTreeNodeFinishAnimationsDefinition Definition;

	CBehTreeNodeFinishAnimationsInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}


	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
};

class CBehTreeNodeBreakAnimationsDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeBreakAnimationsDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeBreakAnimationsInstance, BreakAnimations );
public:
	CBehTreeNodeBreakAnimationsDefinition() {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeBreakAnimationsDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeBreakAnimationsInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
public:
	typedef CBehTreeNodeBreakAnimationsDefinition Definition;

	CBehTreeNodeBreakAnimationsInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
};