#pragma once

#include "behTreeNodeCondition.h"

class IBehTreeNodeConditionCheckRotationInstance;
class CBehTreeNodeConditionCheckRotationToCombatTargetInstance;
class CBehTreeNodeConditionCheckRotationToActionTargetInstance;
class CBehTreeNodeConditionCheckRotationToNamedTargetInstance;

////////////////////////////////////////////////////////////////////////
// Base class definition
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeConditionCheckRotationDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeConditionCheckRotationDefinition, CBehTreeNodeConditionDefinition, IBehTreeNodeConditionCheckRotationInstance, ConditionCheckRotationToTarget )
protected:
	Float					m_rotationTolerance;
public:
	IBehTreeNodeConditionCheckRotationDefinition()
		: m_rotationTolerance( 5.f )									{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeConditionCheckRotationDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
	PROPERTY_EDIT( m_rotationTolerance, TXT("Accepted angle tolerance for rotation check") )
END_CLASS_RTTI()

class IBehTreeNodeConditionCheckRotationInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	Float					m_rotationTolerance;

public:
	typedef IBehTreeNodeConditionCheckRotationDefinition Definition;

	IBehTreeNodeConditionCheckRotationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_rotationTolerance( def.m_rotationTolerance )				{}
};

////////////////////////////////////////////////////////////////////////
// Combat target check
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionCheckRotationToCombatTargetDefinition : public IBehTreeNodeConditionCheckRotationDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionCheckRotationToCombatTargetDefinition, IBehTreeNodeConditionCheckRotationDefinition, CBehTreeNodeConditionCheckRotationToCombatTargetInstance, ConditionCheckRotationToCombatTarget );
protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionCheckRotationToCombatTargetDefinition()		{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionCheckRotationToCombatTargetDefinition)
	PARENT_CLASS( IBehTreeNodeConditionCheckRotationDefinition )
END_CLASS_RTTI()


class CBehTreeNodeConditionCheckRotationToCombatTargetInstance : public IBehTreeNodeConditionCheckRotationInstance
{
	typedef IBehTreeNodeConditionCheckRotationInstance Super;
protected:
	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionCheckRotationToCombatTargetDefinition Definition;

	CBehTreeNodeConditionCheckRotationToCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// Action target check
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionCheckRotationToActionTargetDefinition : public IBehTreeNodeConditionCheckRotationDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionCheckRotationToActionTargetDefinition, IBehTreeNodeConditionCheckRotationDefinition, CBehTreeNodeConditionCheckRotationToActionTargetInstance, ConditionCheckRotationToActionTarget );
protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionCheckRotationToActionTargetDefinition()		{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionCheckRotationToActionTargetDefinition )
	PARENT_CLASS( IBehTreeNodeConditionCheckRotationDefinition )
END_CLASS_RTTI()


class CBehTreeNodeConditionCheckRotationToActionTargetInstance : public IBehTreeNodeConditionCheckRotationInstance
{
	typedef IBehTreeNodeConditionCheckRotationInstance Super;
protected:
	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionCheckRotationToActionTargetDefinition Definition;

	CBehTreeNodeConditionCheckRotationToActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};


////////////////////////////////////////////////////////////////////////
// Named target check
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionCheckRotationToNamedTargetDefinition : public IBehTreeNodeConditionCheckRotationDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionCheckRotationToNamedTargetDefinition, IBehTreeNodeConditionCheckRotationDefinition, CBehTreeNodeConditionCheckRotationToNamedTargetInstance, ConditionCheckRotationToNamedTarget );

protected:
	CBehTreeValCName			m_targetName;

protected:	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionCheckRotationToNamedTargetDefinition()		{}

	String GetNodeCaption() const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionCheckRotationToNamedTargetDefinition )
	PARENT_CLASS( IBehTreeNodeConditionCheckRotationDefinition )
	PROPERTY_EDIT( m_targetName		, TXT( "" ) );
END_CLASS_RTTI()


class CBehTreeNodeConditionCheckRotationToNamedTargetInstance : public IBehTreeNodeConditionCheckRotationInstance
{
	typedef IBehTreeNodeConditionCheckRotationInstance Super;
protected:
	CName m_targetName;

protected:
	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionCheckRotationToNamedTargetDefinition Definition;

	CBehTreeNodeConditionCheckRotationToNamedTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_targetName( def.m_targetName.GetVal( context ) )
	{}
};