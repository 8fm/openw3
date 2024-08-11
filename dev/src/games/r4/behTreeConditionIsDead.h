#pragma once
#include "../../common/game/behTreeNodeCondition.h"

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeConditionIsDeadInstance;
class CBehTreeNodeConditionIsDeadDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsDeadDefinition, CBehTreeNodeConditionDefinition, CBehTreeConditionIsDeadInstance, ConditionIsDead );
protected:
	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionIsDeadDefinition( ) {}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionIsDeadDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeConditionIsDeadInstance : public CBehTreeNodeConditionInstance
{
protected:
	typedef CBehTreeNodeConditionInstance Super;

	Bool	m_isDead;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsDeadDefinition Definition;

	CBehTreeConditionIsDeadInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	void OnDestruction() override;

	Bool OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	// state saving
	Bool IsSavingState() const override;
	void SaveState( IGameSaver* writer ) override;
	Bool LoadState( IGameLoader* reader ) override;
};