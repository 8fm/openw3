/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"

class IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance;
class CBehTreeNodeConditionWasEventFiredRecentlyInstance;
class CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance;
class CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance;

////////////////////////////////////////////////////////////////////////
// Base event listening class.
////////////////////////////////////////////////////////////////////////
// Definition
class IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition, CBehTreeNodeConditionDefinition, IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance, WasEventFiredRecentlyBase );

protected:	
	Float			m_activationTimeout;
	Float			m_executionTimeout;

public:
	IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition()
		: CBehTreeNodeConditionDefinition( false, false, false, true )
		, m_activationTimeout( 0.1f )
		, m_executionTimeout( -1.f )									{}
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition );
	PARENT_CLASS( CBehTreeNodeConditionDefinition );	
	PROPERTY_EDIT( m_activationTimeout, TXT("Time while behavior can activate. -1 behavior will be always available.") );
	PROPERTY_EDIT( m_executionTimeout, TXT("Time until behavior can be executed. -1 behavior wont complete by itself.") );
END_CLASS_RTTI();

// Instance
class IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance : public CBehTreeNodeConditionInstance
{
	typedef  CBehTreeNodeConditionInstance Super;
protected:
	Float			m_activationTimeout;
	Float			m_lastEvent;

	Bool ConditionCheck() override;

public:
	typedef IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition Definition;

	IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void Deactivate() override;
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
};

////////////////////////////////////////////////////////////////////////
// Listening to single event
////////////////////////////////////////////////////////////////////////
// Definition
class CBehTreeNodeConditionWasEventFiredRecentlyDefinition : public IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionWasEventFiredRecentlyDefinition, IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition, CBehTreeNodeConditionWasEventFiredRecentlyInstance, WasEventFiredRecently );
	friend class CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance;

protected:	
	CBehTreeValCName	m_eventName;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionWasEventFiredRecentlyDefinition()				{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionWasEventFiredRecentlyDefinition );
	PARENT_CLASS( IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition );	
	PROPERTY_EDIT( m_eventName, TXT( "Name of the event to listen to." ) );
END_CLASS_RTTI();


// Instance
class CBehTreeNodeConditionWasEventFiredRecentlyInstance : public IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance
{
	typedef  IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance Super;
protected:
	CName			m_eventName;
public:
	typedef CBehTreeNodeConditionWasEventFiredRecentlyDefinition Definition;

	CBehTreeNodeConditionWasEventFiredRecentlyInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnDestruction() override;
};

// Special instance (with execution timeout)
class CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance : public CBehTreeNodeConditionWasEventFiredRecentlyInstance
{
	typedef  CBehTreeNodeConditionWasEventFiredRecentlyInstance Super;
protected:
	Float			m_executionTimeout;

public:
	CBehTreeNodeConditionWasEventFiredRecentlyWithExecutionTimeoutInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_executionTimeout( def.m_executionTimeout )					{}

	void Update() override;
};

////////////////////////////////////////////////////////////////////////
// Listening to multiple events
////////////////////////////////////////////////////////////////////////
// Definition
class CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition : public IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition, IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition, CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance, WasAnyOfEventsFiredRecently );
	friend class CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyWithExecutionTimeoutInstance;

protected:	
	TDynArray< CName >			m_eventsNames;

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition()				{}
};


BEGIN_CLASS_RTTI( CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition );
	PARENT_CLASS( IBehTreeNodeConditionWasEventFiredRecentlyBaseDefinition );	
	PROPERTY_EDIT( m_eventsNames, TXT( "Name of the event to listen to." ) );
END_CLASS_RTTI();

// Instance 
class CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance : public IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance
{
	typedef  IBehTreeNodeConditionWasEventFiredRecentlyBaseInstance Super;
protected:
	TDynArray< CName >			m_eventsNames;

public:
	typedef CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyDefinition Definition;

	CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void OnDestruction() override;
};


// Special instance (with execution timeout)
class CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyWithExecutionTimeoutInstance : public CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance
{
	typedef  CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyInstance Super;
protected:
	Float			m_executionTimeout;

public:
	CBehTreeNodeConditionWasAnyOfEventsFiredRecentlyWithExecutionTimeoutInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_executionTimeout( def.m_executionTimeout )					{}

	void Update() override;
};