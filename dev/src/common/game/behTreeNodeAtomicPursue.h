/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicAction.h"
#include "behTreeVarsEnums.h"
#include "behTreeDynamicNodeBase.h"

//////////////////////////////////////////////////////////////
// CBehTreeNodeBaseAtomicPursueTargetDefinition
class CBehTreeNodeBaseAtomicPursueTargetInstance;
class CBehTreeNodeBaseAtomicPursueTargetDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeBaseAtomicPursueTargetDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeBaseAtomicPursueTargetInstance, BasePursueTarget );

private:									
	CBehTreeValFloat		m_minDistance;
	CBehTreeValFloat		m_moveSpeed;
	CBehTreeValFloat		m_tolerance;
	CBehTreeValEMoveType	m_moveType;
	CBehTreeValBool			m_moveOutsideNavdata;

public:
	CBehTreeNodeBaseAtomicPursueTargetDefinition()
		: m_minDistance( 1.f )
		, m_moveSpeed( 1.f )
		, m_moveType( MT_Run )
		, m_moveOutsideNavdata( false )
	{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeBaseAtomicPursueTargetDefinition )
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition )
	PROPERTY_EDIT( m_minDistance, TXT("Minimum pursue range") )
	PROPERTY_EDIT( m_moveSpeed, TXT("Move speed") )
	PROPERTY_EDIT( m_tolerance, TXT("Pathfinding tolerance") )
	PROPERTY_EDIT( m_moveType, TXT("Type of move motion") )
	PROPERTY_EDIT( m_moveOutsideNavdata, TXT("Try to move even being outside of navdata") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicPursueTargetInstance
class CBehTreeNodeBaseAtomicPursueTargetInstance : public CBehTreeNodeAtomicActionInstance, public IBehTreeDynamicNodeBase
{

	enum EState
	{
		E_MOVE,
		E_SUBBEHAVIOR_ACTIVE
	};

	const static Float	MAX_EXPLORATION_DISTANCE_SQRT;

	typedef CBehTreeNodeAtomicActionInstance Super;

protected:
	Float					m_minDistance;
	Float					m_moveSpeed;
	Float					m_tolerance;
	EMoveType				m_moveType : 16;
	EState					m_state : 8;
	Bool					m_tryToMoveOutsideNavdata;

	THandle< CNode >		m_currentTarget;

	virtual CNode* const	ComputeTarget();

public:
	typedef CBehTreeNodeBaseAtomicPursueTargetDefinition Definition;

	CBehTreeNodeBaseAtomicPursueTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void					Update() override;
	Bool					Activate() override;
	void					Deactivate() override;
	Bool					IsAvailable() override;
	Bool					OnEvent( CBehTreeEvent& e ) override;

	Bool					Interrupt() override;
	Int32					GetNumChildren() const override;
	Int32					GetNumPersistantChildren() const override;
	IBehTreeNodeInstance*	GetChild( Int32 index ) const override;
	IBehTreeNodeInstance*	GetActiveChild() const override;

	void					OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome ) override;

private:
	Bool					IsInValidRange( CNode* target );
};

////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicPursueTargetDefinition
class CBehTreeNodeAtomicPursueTargetInstance;
class CBehTreeNodeAtomicPursueTargetDefinition : public CBehTreeNodeBaseAtomicPursueTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicPursueTargetDefinition, CBehTreeNodeBaseAtomicPursueTargetDefinition, CBehTreeNodeAtomicPursueTargetInstance, PursueTarget );
private:									
	Bool				m_useCombatTarget;
public:
	CBehTreeNodeAtomicPursueTargetDefinition()
		: m_useCombatTarget( true ) {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicPursueTargetDefinition );
	PARENT_CLASS( CBehTreeNodeBaseAtomicPursueTargetDefinition);
	PROPERTY_EDIT( m_useCombatTarget, TXT("Use combat/noncombat target"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicPursueTargetInstance
class CBehTreeNodeAtomicPursueTargetInstance : public CBehTreeNodeBaseAtomicPursueTargetInstance
{
	typedef CBehTreeNodeBaseAtomicPursueTargetInstance Super;
protected:
	Bool					m_useCombatTarget;

	CNode*const ComputeTarget()override;
public:
	typedef CBehTreeNodeAtomicPursueTargetDefinition Definition;

	CBehTreeNodeAtomicPursueTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

