/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeDecorator.h"
#include "behTreeNodeAtomicIdle.h"


class IBehTreeNodeDecoratorLookAtInstance;
class CBehTreeNodeDecoratorLookAtPlayerInstance;
class CBehTreeNodeDecoratorLookAtActionTargetInstance;
class CBehTreeNodeDecoratorLookAtCombatTargetInstance;
class CBehTreeNodeDecoratorLookAtReactionTargetInstance;
class CBehTreeNodeDecoratorLookAtNamedTargetInstance;

class IBehTreeNodeAtomicLookAtInstance;

///////////////////////////////////////////////////////////////////////////////
// Lookat decorator that locks lookat to target in given period of time
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeDecoratorLookAtDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeDecoratorLookAtDefinition, IBehTreeNodeDecoratorDefinition, IBehTreeNodeDecoratorLookAtInstance, LookAt )
	friend class CBehTreeNodeDecoratorLookAtNamedTargetInstance;

protected:
	Float					m_durationPostDeactivation;
	// lookat target selection method
	Bool					m_player;
	Bool					m_actionTarget;
	Bool					m_combatTarget;
	Bool					m_reactionTarget;
	CName					m_namedTarget;

	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeDecoratorLookAtDefinition()
		: m_durationPostDeactivation( 0.f )
		, m_player( false )
		, m_actionTarget( true )
		, m_combatTarget( false )
		, m_reactionTarget( false )												{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeDecoratorLookAtDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
	PROPERTY_EDIT( m_durationPostDeactivation, TXT("Duration for which lookat stays after deactivation.") )
	PROPERTY_EDIT( m_player, TXT("Is targeting player? (Exclusive)") )
	PROPERTY_EDIT( m_actionTarget, TXT("Is targeting action target? (Exclusive)") )
	PROPERTY_EDIT( m_combatTarget, TXT("Is targeting combat target? (Exclusive)") )
	PROPERTY_EDIT( m_reactionTarget, TXT("Is targeting reaction target? (Exclusive)") )
	PROPERTY_EDIT( m_namedTarget, TXT("Is targeting named target? (Exclusive)") )
END_CLASS_RTTI()

class IBehTreeNodeDecoratorLookAtInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	Float					m_durationPostDeactivation;

	virtual CNode* GetLookatTarget()											= 0;
public:
	typedef CBehTreeNodeDecoratorLookAtDefinition Definition;

	IBehTreeNodeDecoratorLookAtInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_durationPostDeactivation( def.m_durationPostDeactivation )			{}

	Bool Activate() override;
	void Deactivate() override;
};

///////////////////////////////////////////////////////////////////////////////
// Different implementations for different target extraction mechanics
///////////////////////////////////////////////////////////////////////////////
// Player
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDecoratorLookAtPlayerInstance : public IBehTreeNodeDecoratorLookAtInstance
{
	typedef IBehTreeNodeDecoratorLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeDecoratorLookAtPlayerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};


///////////////////////////////////////////////////////////////////////////////
// Action target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDecoratorLookAtActionTargetInstance : public IBehTreeNodeDecoratorLookAtInstance
{
	typedef IBehTreeNodeDecoratorLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeDecoratorLookAtActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};

///////////////////////////////////////////////////////////////////////////////
// Combat target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDecoratorLookAtCombatTargetInstance : public IBehTreeNodeDecoratorLookAtInstance
{
	typedef IBehTreeNodeDecoratorLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeDecoratorLookAtCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};
///////////////////////////////////////////////////////////////////////////////
// Reaction target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDecoratorLookAtReactionTargetInstance : public IBehTreeNodeDecoratorLookAtInstance
{
	typedef IBehTreeNodeDecoratorLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeDecoratorLookAtReactionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};
///////////////////////////////////////////////////////////////////////////////
// Named target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeDecoratorLookAtNamedTargetInstance : public IBehTreeNodeDecoratorLookAtInstance
{
	typedef IBehTreeNodeDecoratorLookAtInstance Super;
protected:
	CName				m_namedTarget;

	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeDecoratorLookAtNamedTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_namedTarget( def.m_namedTarget )									{}
};

///////////////////////////////////////////////////////////////////////////////
// Lookat atomic node that enables lookat and completes
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicLookAtDefinition : public CBehTreeNodeCompleteImmediatelyDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicLookAtDefinition, CBehTreeNodeCompleteImmediatelyDefinition, IBehTreeNodeAtomicLookAtInstance, AtomicLookAt )
	friend class CBehTreeNodeAtomicLookAtNamedTargetInstance;
protected:
	Float					m_duration;
	// lookat target selection method
	Bool					m_player;
	Bool					m_actionTarget;
	Bool					m_combatTarget;
	Bool					m_reactionTarget;
	CName					m_namedTarget;

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

	CBehTreeNodeAtomicLookAtDefinition()
		: m_duration( 3.f )	
		, m_player( false )
		, m_actionTarget( true )
		, m_combatTarget( false )
		, m_reactionTarget( false )												{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicLookAtDefinition )
	PARENT_CLASS( CBehTreeNodeCompleteImmediatelyDefinition )
	PROPERTY_EDIT( m_duration, TXT("Lookat duration.") )
	PROPERTY_EDIT( m_player, TXT("Is targeting player? (Exclusive)") )
	PROPERTY_EDIT( m_actionTarget, TXT("Is targeting action target? (Exclusive)") )
	PROPERTY_EDIT( m_combatTarget, TXT("Is targeting combat target? (Exclusive)") )
	PROPERTY_EDIT( m_reactionTarget, TXT("Is targeting reaction target? (Exclusive)") )
	PROPERTY_EDIT( m_namedTarget, TXT("Is targeting named target? (Exclusive)") )
END_CLASS_RTTI()

class IBehTreeNodeAtomicLookAtInstance : public CBehTreeNodeCompleteImmediatelyInstance
{
	typedef CBehTreeNodeCompleteImmediatelyInstance Super;
protected:
	Float					m_duration;

	virtual CNode* GetLookatTarget()											= 0;
public:
	typedef CBehTreeNodeAtomicLookAtDefinition Definition;
	IBehTreeNodeAtomicLookAtInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_duration( def.m_duration )											{}

	void Update() override;
};

///////////////////////////////////////////////////////////////////////////////
// Different implementations for different target extraction mechanics
///////////////////////////////////////////////////////////////////////////////
// Player
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicLookAtPlayerInstance : public IBehTreeNodeAtomicLookAtInstance
{
	typedef IBehTreeNodeAtomicLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeAtomicLookAtPlayerInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};


///////////////////////////////////////////////////////////////////////////////
// Action target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicLookAtActionTargetInstance : public IBehTreeNodeAtomicLookAtInstance
{
	typedef IBehTreeNodeAtomicLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeAtomicLookAtActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};

///////////////////////////////////////////////////////////////////////////////
// Combat target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicLookAtCombatTargetInstance : public IBehTreeNodeAtomicLookAtInstance
{
	typedef IBehTreeNodeAtomicLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeAtomicLookAtCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};
///////////////////////////////////////////////////////////////////////////////
// Reaction target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicLookAtReactionTargetInstance : public IBehTreeNodeAtomicLookAtInstance
{
	typedef IBehTreeNodeAtomicLookAtInstance Super;
protected:
	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeAtomicLookAtReactionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};
///////////////////////////////////////////////////////////////////////////////
// Named target
///////////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicLookAtNamedTargetInstance : public IBehTreeNodeAtomicLookAtInstance
{
	typedef IBehTreeNodeAtomicLookAtInstance Super;
protected:
	CName				m_namedTarget;

	CNode* GetLookatTarget() override;
public:
	CBehTreeNodeAtomicLookAtNamedTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_namedTarget( def.m_namedTarget )									{}
};