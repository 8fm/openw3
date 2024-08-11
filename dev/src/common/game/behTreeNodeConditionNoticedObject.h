#pragma once

#include "behTreeNodeCondition.h"

class CBehTreeNodeConditionTargetNoticedInstance;
class CBehTreeNodeConditionCombatTargetNoticedInstance;



////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionTargetNoticedDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionTargetNoticedDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionTargetNoticedInstance, ConditionTargetNoticed );

protected:	

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
};


BEGIN_CLASS_RTTI(CBehTreeNodeConditionTargetNoticedDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);
END_CLASS_RTTI();

class CBehTreeNodeConditionTargetNoticedInstance : public CBehTreeNodeConditionInstance
{
protected:	

	Bool ConditionCheck() override;

	virtual CActor* GetActorToTest() { return nullptr; };
public:
	typedef CBehTreeNodeConditionTargetNoticedDefinition Definition;
	CBehTreeNodeConditionTargetNoticedInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );	
};



////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionCombatTargetNoticedDefinition : public CBehTreeNodeConditionTargetNoticedDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionCombatTargetNoticedDefinition, CBehTreeNodeConditionTargetNoticedDefinition, CBehTreeNodeConditionCombatTargetNoticedInstance, ConditionCombatTargetNoticed );

protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

};


BEGIN_CLASS_RTTI(CBehTreeNodeConditionCombatTargetNoticedDefinition);
PARENT_CLASS(CBehTreeNodeConditionTargetNoticedDefinition);
END_CLASS_RTTI();

class CBehTreeNodeConditionCombatTargetNoticedInstance : public CBehTreeNodeConditionTargetNoticedInstance
{
protected:	
	virtual CActor* GetActorToTest() override;

public:
	typedef CBehTreeNodeConditionCombatTargetNoticedDefinition Definition;
	CBehTreeNodeConditionCombatTargetNoticedInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );	
};