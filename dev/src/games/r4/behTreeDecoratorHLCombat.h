#pragma once

#include "../../common/game/behTreeCounterData.h"
#include "../../common/game/behTreeDecorator.h"
#include "../../common/game/behTreeGuardAreaData.h"
#include "ridingAiStorage.h"


class IBehTreeNodeHLOffenceBaseInstance;
class IBehTreeNodeHLCombatBaseInstance;
class CBehTreeNodeHLCombatInstance;
class CBehTreeNodeHLAnimalCombatInstance;
class CBehTreeDecoratorBaseHLDangerInstance;

////////////////////////////////////////////////////////////////////////
// Base node that stands for combat mode 'guard'
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeHLOffenceBaseDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeHLOffenceBaseDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeHLOffenceBaseInstance, HighLevelOffense );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeHLOffenceBaseDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI();

class IBehTreeNodeHLOffenceBaseInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCounterDataPtr		m_combatLock;								// semaphore that can prolong combat behavior even when high level conditions are not met

	Bool Check();
	virtual Bool ConditionCheck()										= 0;

public:
	typedef IBehTreeNodeHLOffenceBaseDefinition Definition;

	IBehTreeNodeHLOffenceBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void		OnDestruction() override;

	Bool		IsAvailable() override;
	Int32		Evaluate() override;

	Bool		Activate() override;
	void		Deactivate() override;

	Bool		OnListenedEvent( CBehTreeEvent& e ) override;

	static CName GetCombatLockName();
	static CName GetCombatRequestEventName();
};

////////////////////////////////////////////////////////////////////////
// Base node that stands for combat mode 'guard'
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeHLCombatBaseDefinition : public IBehTreeNodeHLOffenceBaseDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeHLCombatBaseDefinition, IBehTreeNodeHLOffenceBaseDefinition, IBehTreeNodeHLCombatBaseInstance, HighLevelCombatBase );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeHLCombatBaseDefinition )
	PARENT_CLASS( IBehTreeNodeHLOffenceBaseDefinition )
END_CLASS_RTTI();

class IBehTreeNodeHLCombatBaseInstance : public IBehTreeNodeHLOffenceBaseInstance
{
	typedef IBehTreeNodeHLOffenceBaseInstance Super;

public:
	typedef IBehTreeNodeHLCombatBaseDefinition Definition;

	IBehTreeNodeHLCombatBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// Base node that stands atop all combat for most combat behaviors
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeHLCombatDefinition : public IBehTreeNodeHLCombatBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeHLCombatDefinition, IBehTreeNodeHLCombatBaseDefinition, CBehTreeNodeHLCombatInstance, HighLevelCombat );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeHLCombatDefinition )
	PARENT_CLASS( IBehTreeNodeHLCombatBaseDefinition )
END_CLASS_RTTI();


class CBehTreeNodeHLCombatInstance : public IBehTreeNodeHLCombatBaseInstance
{
	typedef IBehTreeNodeHLCombatBaseInstance Super;
protected:
	Bool		ConditionCheck() override;

	CBehTreeGuardAreaDataPtr			m_guardAreaDataPtr;
public:
	typedef CBehTreeNodeHLCombatDefinition Definition;

	CBehTreeNodeHLCombatInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool		Activate() override;
	void		Deactivate() override;
};

////////////////////////////////////////////////////////////////////////
// Combat node for animals
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeHLAnimalCombatDefinition : public IBehTreeNodeHLCombatBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeHLAnimalCombatDefinition, IBehTreeNodeHLCombatBaseDefinition, CBehTreeNodeHLAnimalCombatInstance, HighLevelAnimalCombat );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeHLAnimalCombatDefinition )
	PARENT_CLASS( IBehTreeNodeHLCombatBaseDefinition )
END_CLASS_RTTI();


class CBehTreeNodeHLAnimalCombatInstance : public IBehTreeNodeHLCombatBaseInstance
{
	typedef IBehTreeNodeHLCombatBaseInstance Super;
private :
	
protected:
	Bool		ConditionCheck() override;
public:
	typedef CBehTreeNodeHLAnimalCombatDefinition Definition;

	CBehTreeNodeHLAnimalCombatInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )						{}

};



////////////////////////////////////////////////////////////////////////
// IBehTreeDecoratorBaseHLDangerDefinition
////////////////////////////////////////////////////////////////////////
class IBehTreeDecoratorBaseHLDangerDefinition : public IBehTreeNodeHLOffenceBaseDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeDecoratorBaseHLDangerDefinition, IBehTreeNodeHLOffenceBaseDefinition, CBehTreeDecoratorBaseHLDangerInstance, HighLevelDangerBase );
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeDecoratorBaseHLDangerDefinition )
	PARENT_CLASS( IBehTreeNodeHLOffenceBaseDefinition )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// IBehTreeDecoratorBaseHLDangerInstance
class IBehTreeDecoratorBaseHLDangerInstance : public IBehTreeNodeHLOffenceBaseInstance
{
	typedef IBehTreeNodeHLOffenceBaseInstance Super;
protected:
	Bool			m_neutralIsDanger;

	Bool			ConditionCheck() override;

	virtual Bool	GetNeutralIsDanger() const							= 0;
public:
	typedef IBehTreeDecoratorBaseHLDangerDefinition Definition;

	IBehTreeDecoratorBaseHLDangerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}

	Bool			Activate() override;
};

////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHLDangerDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorHLDangerInstance;
class CBehTreeDecoratorHLDangerDefinition : public IBehTreeDecoratorBaseHLDangerDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorHLDangerDefinition, IBehTreeDecoratorBaseHLDangerDefinition, CBehTreeDecoratorHLDangerInstance, HighLevelDanger );
protected:
	CBehTreeValBool m_neutralIsDanger;
public:
	CBehTreeDecoratorHLDangerDefinition() 
		: m_neutralIsDanger( false )	{}
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorHLDangerDefinition )
	PARENT_CLASS( IBehTreeDecoratorBaseHLDangerDefinition )
	PROPERTY_EDIT( m_neutralIsDanger, TXT("Actor with a neutral attitude group will be accounted in the test") )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHLDangerInstance
class CBehTreeDecoratorHLDangerInstance : public IBehTreeDecoratorBaseHLDangerInstance
{
	typedef IBehTreeDecoratorBaseHLDangerInstance Super;
protected:
	Bool m_neutralIsDanger;

	Bool GetNeutralIsDanger()const override;
public:
	typedef CBehTreeDecoratorHLDangerDefinition Definition;

	CBehTreeDecoratorHLDangerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHLDangerTamableDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeDecoratorHLDangerTamableInstance;
class CBehTreeDecoratorHLDangerTamableDefinition : public CBehTreeDecoratorHLDangerDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeDecoratorHLDangerTamableDefinition, CBehTreeDecoratorHLDangerDefinition, CBehTreeDecoratorHLDangerTamableInstance, HighLevelDangerTamable );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
};

BEGIN_CLASS_RTTI( CBehTreeDecoratorHLDangerTamableDefinition )
	PARENT_CLASS( CBehTreeDecoratorHLDangerDefinition )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeDecoratorHLDangerTamableInstance
class CBehTreeDecoratorHLDangerTamableInstance : public CBehTreeDecoratorHLDangerInstance
{
	typedef CBehTreeDecoratorHLDangerInstance Super;
protected:
	Float m_tempNeutralIsDangerTime;
	Bool GetNeutralIsDanger()const override;
public:
	typedef CBehTreeDecoratorHLDangerTamableDefinition Definition;

	CBehTreeDecoratorHLDangerTamableInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );

	void OnDestruction() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
private :
	Bool IsCharmed()const;
};