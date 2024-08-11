/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeCounterData.h"
#include "../../common/game/behTreeDecorator.h"

class CBehTreeNodeProlongHLCombatDecoratorInstance;
class CBehTreeNodeNotifyCombatActivationDecoratorInstance;
////////////////////////////////////////////////////////////////////////
// Decorator that 'locks' high level combat state so it will be available
// as long as this decorator is running
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeProlongHLCombatDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeProlongHLCombatDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeProlongHLCombatDecoratorInstance, ProlongHLCombat );
protected:
	Bool						m_requestCombatActivationOnEvent;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeProlongHLCombatDecoratorDefinition()
		: m_requestCombatActivationOnEvent( false )						{}

};

BEGIN_CLASS_RTTI( CBehTreeNodeProlongHLCombatDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_requestCombatActivationOnEvent, TXT("If set, this node will make combat available for one entry when its subnode return true on listened event handling"))
END_CLASS_RTTI();

class CBehTreeNodeProlongHLCombatDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	CBehTreeCounterDataPtr		m_combatLock;
	Bool						m_lockIsUp;

	void LockUp();
	void LockDown();

public:
	typedef CBehTreeNodeProlongHLCombatDecoratorDefinition Definition;

	CBehTreeNodeProlongHLCombatDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	Bool Activate();
	void Deactivate();

	Bool IsAvailable() override;
	Int32 Evaluate() override;
};

class CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance : public CBehTreeNodeProlongHLCombatDecoratorInstance
{
	typedef CBehTreeNodeProlongHLCombatDecoratorInstance Super;
public:
	typedef CBehTreeNodeProlongHLCombatDecoratorDefinition Definition;

	CBehTreeNodeProlongHLCombatDecoratorAndRequestCombatActivationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void MarkDirty() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	void OnDestruction() override;
};

class CBehTreeNodeNotifyCombatActivationDecoratorDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeNotifyCombatActivationDecoratorDefinition, IBehTreeNodeDecoratorDefinition, CBehTreeNodeNotifyCombatActivationDecoratorInstance, CombatActivatedNotifier );

protected:
	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeNotifyCombatActivationDecoratorDefinition() {}
};

BEGIN_CLASS_RTTI( CBehTreeNodeNotifyCombatActivationDecoratorDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI();

class CBehTreeNodeNotifyCombatActivationDecoratorInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;

public:
	typedef CBehTreeNodeNotifyCombatActivationDecoratorDefinition Definition;

	CBehTreeNodeNotifyCombatActivationDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}

	Bool Activate();
};