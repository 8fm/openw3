#pragma once
#include "../../common/game/behTreeNodeAtomicAction.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeAtomicForgetCombatTargetDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeInstanceForgetCombatTargetInstance;
class CBehTreeAtomicForgetCombatTargetDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeAtomicForgetCombatTargetDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeInstanceForgetCombatTargetInstance, AtomicForgetCombatTarget );
protected:

public:
	CBehTreeAtomicForgetCombatTargetDefinition() {}
	
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeAtomicForgetCombatTargetDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeInstanceForgetCombatTargetInstance
class CBehTreeInstanceForgetCombatTargetInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	

public:
	typedef CBehTreeAtomicForgetCombatTargetDefinition Definition;

	CBehTreeInstanceForgetCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: CBehTreeNodeAtomicActionInstance( def, owner, context, parent ) 
	{}

	void Update() override;
};
