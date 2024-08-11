/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicAction.h"
#include "behTreeVarsEnums.h"
#include "behTreeSteeringGraphBase.h"
#include "behTreeCustomMoveData.h"
#include "behTreeDynamicNodeBase.h"

class CBehTreeNodeAtomicMoveToInstance;

//////////////////////////////////////////////////////////////////////////
// Generic move to
class CBehTreeNodeAtomicMoveToDefinition : public CBehTreeNodeAtomicActionDefinition, public CBehTreeSteeringGraphCommonDef
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicMoveToInstance, AtomicMoveTo );
protected:
	CBehTreeValFloat		m_maxDistance;
	CBehTreeValFloat		m_moveSpeed;
	CBehTreeValEMoveType	m_moveType;
	Float					m_angularTolerance;
	CBehTreeValFloat		m_pathfindingTolerance;
	CBehTreeValBool			m_keepPreviousMoveData;
	CBehTreeValBool			m_rotateAfterwards;
	CBehTreeValBool			m_preciseArrival;
public:
	CBehTreeNodeAtomicMoveToDefinition()
		: m_maxDistance( 1.f )
		, m_moveSpeed( 1.f )
		, m_moveType( MT_Run )
		, m_angularTolerance( 15.f )
		, m_pathfindingTolerance( 0.f )
		, m_keepPreviousMoveData( false )
		, m_rotateAfterwards( false )
		, m_preciseArrival( false )											{}
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeAtomicMoveToDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
	PROPERTY_EDIT( m_steeringGraph, TXT("Custom steering graph resource") );
	PROPERTY_EDIT( m_maxDistance, TXT("Range limit"));
	PROPERTY_EDIT( m_moveSpeed, TXT("Move speed"));
	PROPERTY_EDIT( m_pathfindingTolerance, TXT("Tolerance for destination node pathfinding") );
	PROPERTY_EDIT( m_moveType, TXT("Type of move motion"));
	PROPERTY_EDIT( m_angularTolerance, TXT("Tolerance for rotation if rotateAfterwards flag is set") );
	PROPERTY_EDIT( m_keepPreviousMoveData, TXT("If should keep previous speed and type of movement") )
	PROPERTY_EDIT( m_rotateAfterwards, TXT("Rotate after reaching target") );
	PROPERTY_EDIT( m_preciseArrival, TXT("Support final step") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicMoveToInstance : public CBehTreeNodeAtomicActionInstance, public CBehTreeSteeringGraphCommonInstance, public IBehTreeDynamicNodeBase
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	enum EState
	{
		E_INVALID,
		E_SUBBEHAVIOR_ACTIVE,
		E_MOVE,
		E_ROTATE
	};

	Float		m_maxDistance;
	Float		m_moveSpeed;
	Float		m_angularTolerance;
	Vector		m_target;
	Float		m_heading;
	Float		m_pathfindingTolerance;
	EMoveType	m_moveType				: 8;
	EState		m_state					: 8;
	Bool		m_keepPreviousMoveData	: 1;
	Bool		m_rotateAfterwards		: 1;
	Bool		m_preciseArrival		: 1;
	
	virtual Bool		StartActorMoveTo();	// Decides what parameters to pass to the Actor::moveTo call
	virtual Bool		ComputeTargetAndHeading() = 0;
	virtual Bool		OnDestinationReached();							// quite internal function. Run when we reached our destination. Can be used to automatically update target and reissue move command
	Bool				UpdateTargetAndHeading();
public:
	typedef CBehTreeNodeAtomicMoveToDefinition Definition;

	CBehTreeNodeAtomicMoveToInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void				Update() override;
	Bool				Activate() override;
	void				Deactivate() override;

	// Stuff important for dynamic node
	void				OnSubgoalCompleted( IBehTreeNodeInstance::eTaskOutcome outcome ) override;
	Bool				OnEvent( CBehTreeEvent& e ) override;
	Bool				Interrupt() override;
	Int32				GetNumChildren() const override;
	Int32				GetNumPersistantChildren() const override;
	IBehTreeNodeInstance*	GetChild( Int32 index ) const override;
	IBehTreeNodeInstance*	GetActiveChild() const override;

	void				OnGenerateDebugFragments( CRenderFrame* frame ) override;
};

//////////////////////////////////////////////////////////////////////////
// Action target move to
class CBehTreeNodeAtomicMoveToActionTargetInstance;

class CBehTreeNodeAtomicMoveToActionTargetDefinition : public CBehTreeNodeAtomicMoveToDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMoveToActionTargetDefinition, CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeAtomicMoveToActionTargetInstance, AtomicMoveToActionTarget );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMoveToActionTargetDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicMoveToDefinition);
END_CLASS_RTTI();

class CBehTreeNodeAtomicMoveToActionTargetInstance : public CBehTreeNodeAtomicMoveToInstance
{
private:
	typedef CBehTreeNodeAtomicMoveToInstance Super;
protected:
	Bool ComputeTargetAndHeading() override;
public:
	CBehTreeNodeAtomicMoveToActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )								{}
};



//////////////////////////////////////////////////////////////////////////
// Combat move to
class CBehTreeNodeAtomicMoveToCombatTargetInstance;

class CBehTreeNodeAtomicMoveToCombatTargetDefinition : public CBehTreeNodeAtomicMoveToDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMoveToCombatTargetDefinition, CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeAtomicMoveToCombatTargetInstance, AtomicMoveToCombatTarget );
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMoveToCombatTargetDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicMoveToDefinition);
END_CLASS_RTTI();

class CBehTreeNodeAtomicMoveToCombatTargetInstance : public CBehTreeNodeAtomicMoveToInstance
{
private:
	typedef CBehTreeNodeAtomicMoveToInstance Super;
protected:
	Bool ComputeTargetAndHeading() override;
public:
	CBehTreeNodeAtomicMoveToCombatTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )								{}
};

//////////////////////////////////////////////////////////////////////////
// Custom move to action, using AI storage data
class CBehTreeNodeAtomicMoveToCustomPointInstance;

class CBehTreeNodeAtomicMoveToCustomPointDefinition : public CBehTreeNodeAtomicMoveToDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMoveToCustomPointDefinition, CBehTreeNodeAtomicMoveToDefinition, CBehTreeNodeAtomicMoveToCustomPointInstance, AtomicMoveToCustomPoint );
protected:

public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMoveToCustomPointDefinition );
PARENT_CLASS( CBehTreeNodeAtomicMoveToDefinition );

END_CLASS_RTTI();

class CBehTreeNodeAtomicMoveToCustomPointInstance : public CBehTreeNodeAtomicMoveToInstance
{
private:
	typedef CBehTreeNodeAtomicMoveToInstance Super;
protected:
	CBehTreeCustomMoveDataPtr	m_customMoveData;

	Bool ComputeTargetAndHeading() override;
public:
	CBehTreeNodeAtomicMoveToCustomPointInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

