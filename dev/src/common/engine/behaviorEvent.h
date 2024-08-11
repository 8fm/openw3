/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "../core/object.h"

class CBehaviorGraph;

//////////////////////////////////////////////////////////////////////////
//
// TODO:
// Binary search + operator < dla CBehaviorEventDescription
//

class CBehaviorEventDescription : public CObject
{
	friend class CBehaviorEventsList;

	DECLARE_ENGINE_CLASS( CBehaviorEventDescription, CObject, 0 );

public:
	CName			m_eventName;	
	Bool			m_isModifiableByEffect;

public:
	CBehaviorEventDescription();

	const CName& GetEventName() const { return m_eventName; }

	void Set( const CBehaviorEventDescription* rhs )
	{
		m_eventName = rhs->m_eventName;
		m_isModifiableByEffect = rhs->m_isModifiableByEffect;
	}
};

BEGIN_CLASS_RTTI( CBehaviorEventDescription );
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_eventName, TXT("Event name") );	
	PROPERTY_EDIT( m_isModifiableByEffect, TXT("Modifiable by effect") );
END_CLASS_RTTI();


//! helper class for holding 
class CBehaviorEventsList
{
	//! owning behavior graph
	CBehaviorGraph	*m_behaviorGraph;

	//! events list
	typedef TDynArray< CBehaviorEventDescription* > TContainerType;
	TContainerType	m_events;

public:
	CBehaviorEventsList();

	void SetBehaviorGraph( CBehaviorGraph* graph ) { m_behaviorGraph = graph; }

	Uint32 AddEvent( const CName& name );

	CBehaviorEventDescription* GetEvent( const CName& name ) const;	

	Uint32 GetEventId( const CName& name ) const;

	void RemoveEvent( const CName& name );

	Uint32 GetNumEvents() const;

	const CName& GetEventName( Uint32 index ) const;

	const CName& FindEventNameById( Uint32 id ) const;

	CBehaviorEventDescription* GetEvent( Uint32 index ) const;
	
	Bool DuplicationTestName() const;

	Uint32 GetSize() const;

	void GetEvents( TDynArray< CBehaviorEventDescription* >& events ) const;

	CBehaviorEventsList& operator=( const CBehaviorEventsList& rhs );

	friend IFile& operator<<( IFile &file, CBehaviorEventsList &list );

	static const Uint32 NO_EVENT = 0xFFFFFFFF;

protected:
	TContainerType::const_iterator FindEventByName( const CName& name ) const;

	TContainerType::iterator FindEventByName( const CName& name );	

private:
	typedef Bool (*FUNCTION_EVENT_EVENT)( const CBehaviorEventDescription*, const CBehaviorEventDescription* );
	Bool DuplicationTest( FUNCTION_EVENT_EVENT testFun, FUNCTION_EVENT_EVENT messageFun ) const;
};

class CBehaviorEvent
{
	Uint32	m_eventID;

public:
	CBehaviorEvent( Uint32 eventID ) : m_eventID( eventID ) {}	

	RED_INLINE Uint32 GetEventID() const { return m_eventID; }
};

#include "behaviorGraphNotifier.h"

// Event info
struct BehaviorEventInfo
{
	EBehaviorGraphNotificationType	m_type;
	CName							m_eventName;
	CAnimatedComponent*				m_activator;

	// Default constructor
	RED_INLINE BehaviorEventInfo()
		: m_type( BGNT_Activation )
		, m_activator( NULL )
	{};
};



