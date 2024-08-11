/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "behTreeNodeAtomicAction.h"

class CBehTreeNodeAtomicPlayAnimationEventInstance;

class CBehTreeNodeAtomicPlayAnimationEventDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeAtomicPlayAnimationEventDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicPlayAnimationEventInstance, PlayAnimationEvent );
protected:
	Bool				m_shouldForceEvent;
	CBehTreeValCName	m_eventStateName;
	CName				m_eventResetTriggerName;
public:
	CBehTreeNodeAtomicPlayAnimationEventDefinition()
		: m_shouldForceEvent( true ) 
		, m_eventResetTriggerName( CName::NONE )
			{}

	virtual IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const;
};

BEGIN_CLASS_RTTI( CBehTreeNodeAtomicPlayAnimationEventDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition);
    PROPERTY_EDIT( m_shouldForceEvent, TXT("Force Event"));
    PROPERTY_EDIT( m_eventStateName, TXT("Animation event state name"));
    PROPERTY_EDIT( m_eventResetTriggerName, TXT("Trigger name for invoking event reboot"));
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehTreeNodeAtomicPlayAnimationEventInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	CName	m_eventStateName;
	CName	m_eventResetTriggerName;
	Int32	m_eventDelay;
	Bool	m_eventRunning;
    Bool	m_shouldForceEvent;
	Bool	m_eventRepeated;
	Bool	m_stateCompleted;
public:

	typedef CBehTreeNodeAtomicPlayAnimationEventDefinition Definition;

	CBehTreeNodeAtomicPlayAnimationEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	////////////////////////////////////////////////////////////////////
	// Base interface
	void Update() override;
	Bool Activate() override;
	void Deactivate() override;

	Bool OnEvent( CBehTreeEvent& e ) override;

protected:
	Bool FireEvent();
};