/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicAction.h"

/////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicRotateToTargetDefinition
class CBehTreeNodeBaseRotateToTargetInstance;
class CBehTreeNodeBaseRotateToTargetDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeBaseRotateToTargetDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeBaseRotateToTargetInstance, BaseRotateToTarget );
private:
	Bool				m_completeOnTargetReached;
public:
	CBehTreeNodeBaseRotateToTargetDefinition()
		: m_completeOnTargetReached( true ) {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeBaseRotateToTargetDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
	PROPERTY_EDIT( m_completeOnTargetReached, TXT("if false will keep on rotating by following target") )
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicRotateToTargetInstance
class CBehTreeNodeBaseRotateToTargetInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	//Float					m_rotateTime;
	Bool					m_completeOnTargetReached;
	THandle< CNode >		m_currentTarget;
public:


	typedef CBehTreeNodeBaseRotateToTargetDefinition Definition;

	CBehTreeNodeBaseRotateToTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	Bool IsAvailable() override;

private:
	virtual CNode*const GetTarget()const {return nullptr;}
	Bool Check();
};

/////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicRotateToTargetDefinition
class CBehTreeNodeAtomicRotateToTargetInstance;
class CBehTreeNodeAtomicRotateToTargetDefinition : public CBehTreeNodeBaseRotateToTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicRotateToTargetDefinition, CBehTreeNodeBaseRotateToTargetDefinition, CBehTreeNodeAtomicRotateToTargetInstance, RotateToTarget );
private:
	Bool					m_useCombatTarget;
public:
	CBehTreeNodeAtomicRotateToTargetDefinition()
		: m_useCombatTarget( false ) {}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicRotateToTargetDefinition );
	PARENT_CLASS( CBehTreeNodeBaseRotateToTargetDefinition);
	PROPERTY_EDIT( m_useCombatTarget, TXT("Use combat or action target") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
// CBehTreeNodeAtomicRotateToTargetInstance
class CBehTreeNodeAtomicRotateToTargetInstance : public CBehTreeNodeBaseRotateToTargetInstance
{
	typedef CBehTreeNodeBaseRotateToTargetInstance Super;
protected:
	Bool					m_useCombatTarget;
public:
	typedef CBehTreeNodeAtomicRotateToTargetDefinition Definition;

	CBehTreeNodeAtomicRotateToTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );


private:
	CNode*const GetTarget()const override;
};


/////
class CBehTreeAtomicMatchActionTargetRotationInstance;

class CBehTreeNodeAtomicMatchActionTargetRotationDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicMatchActionTargetRotationDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeAtomicMatchActionTargetRotationInstance, RotateToMatchActionTargetHeading );
private:
	Bool				m_completeOnTargetReached;
public:
	CBehTreeNodeAtomicMatchActionTargetRotationDefinition()
		: m_completeOnTargetReached( true )	{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicMatchActionTargetRotationDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
	PROPERTY_EDIT( m_completeOnTargetReached, TXT("if false will keep on rotating by matching target heading") )
END_CLASS_RTTI();


//////////////////////////////////////////////////////////////////////////

class CBehTreeAtomicMatchActionTargetRotationInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	Bool					m_completeOnTargetReached;
	THandle< CNode >		m_currentTarget;

public:
	typedef CBehTreeNodeAtomicMatchActionTargetRotationDefinition Definition;

	CBehTreeAtomicMatchActionTargetRotationInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	//! Behavior execution cycle
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;
	Bool IsAvailable() override;
private:
	CNode*const GetTarget()const;
	Bool Check();
};
