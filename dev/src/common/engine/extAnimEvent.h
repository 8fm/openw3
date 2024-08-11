/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../core/hashset.h"
#include "animationEvent.h"

struct CAnimationEventFired;
class CEntity;

class CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimEvent )

public:

	class StatsCollector
	{
	public:
		struct EventStats
		{
			CName m_name;
			Uint32 m_count;
			Float m_timeMS;

			EventStats( CName name = CName::NONE )
				: m_name( name )
				, m_count( 0 )
				, m_timeMS( 0.0 )
			{}

			struct HashFunc
			{
				static RED_FORCE_INLINE Uint32 GetHash( const EventStats& stats ) { return GetHash( stats.m_name ); }
				static RED_FORCE_INLINE Uint32 GetHash( const CName name ) { return ::GetHash( name ); }
			};
			struct EqualFunc
			{
				static RED_INLINE Bool Equal( const EventStats& a, const EventStats& b ) { return a.m_name == b.m_name; }
				static RED_INLINE Bool Equal( const EventStats& a, const CName b ) { return a.m_name == b; }
			};
		};

		typedef THashSet< EventStats, EventStats::HashFunc, EventStats::EqualFunc > EventStatsMap;

	private:
		Float m_totalEventTimeMS;
		EventStatsMap m_stats;
		static StatsCollector s_statsCollector;

	public:
		const EventStatsMap& GetStats() const { return m_stats; }
		Float GetTotalEventTime() const { return m_totalEventTimeMS; }

		void Reset();
		void OnEvent( CName name, Float timeMS );
		
		static StatsCollector& GetInstance();
	};

	//////////////////////////////////////////////////////////////////////////

	struct Sorter
	{
		static RED_INLINE Bool Less( CExtAnimEvent* const & a, CExtAnimEvent* const & b )
		{
			RED_ASSERT( a != NULL );
			RED_ASSERT( b != NULL );
			return a->GetStartTime() < b->GetStartTime();
		}
	};

	//////////////////////////////////////////////////////////////////////////

	static void Serialize( TDynArray< CExtAnimEvent* >& events, IFile& file );

	CExtAnimEvent();
	CExtAnimEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimEvent();

	RED_INLINE const CName& GetEventName() const
	{ return m_eventName; }

	void SetEventName( const CName& name );

#ifdef USE_EXT_ANIM_EVENTS

	RED_INLINE const CName& GetAnimationName() const
	{ return m_animationName; }

	void SetAnimationName( const CName& animationName );

#endif

	RED_INLINE Float GetStartTime() const
	{ return m_startTime; }
	
	void SetStartTime( Float startTime );

	RED_INLINE Bool ReportToScript( Float onWeight) const
	{ return m_reportToScript && onWeight >= m_reportToScriptMinWeight; }

	virtual Bool ReportToAI() const { return true; }

#ifndef NO_EDITOR

	RED_INLINE const String& GetTrackName() const
	{ return m_trackName; }

	void SetTrackName( const String& trackName )
	{ m_trackName = trackName; }

	virtual void OnPropertyPostChanged( const CName& propertyName ) {}

#endif

	virtual Float GetEndTimeWithoutClamp() const
	{ return m_startTime; }

	virtual Float VGetDuration() const
	{ return 0.f; }

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

	virtual void ProcessPostponed( const CAnimationEventFired& info, CAnimatedComponent* component ) const {}

	// this is called when names are the same and classes are the same too
	virtual Bool CanBeMergedWith( const CExtAnimEvent* /*_with*/ ) const
	{ return true; }

	void CheckReportToScriptFlag();

protected:
	CName					m_eventName;
	Float					m_startTime;
	Bool					m_reportToScript;
	Float					m_reportToScriptMinWeight;
#ifdef USE_EXT_ANIM_EVENTS
	CName					m_animationName;
#endif
#ifndef NO_EDITOR
	String					m_trackName;		//<! Only important in editor
#endif
};

BEGIN_CLASS_RTTI( CExtAnimEvent )
	PROPERTY_EDIT( m_eventName, TXT( "Event name" ) )
	PROPERTY_EDIT( m_startTime, TXT( "Start time" ) )
	PROPERTY_RO( m_reportToScript, TXT("Report to script") )
	PROPERTY_EDIT( m_reportToScriptMinWeight, TXT("Report to script minimal weight") )
#ifdef USE_EXT_ANIM_EVENTS
	PROPERTY( m_animationName )
#endif
#ifndef NO_EDITOR
	PROPERTY_NOT_COOKED( m_trackName )
#endif
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class IEventsContainer
{
public:
	virtual void AddEvent( CExtAnimEvent* event ) = 0;
	virtual void RemoveEvent( CExtAnimEvent* event ) = 0;
	virtual void GetEventsForAnimation( const CName& animName, TDynArray< CExtAnimEvent* >& events ) = 0;

	virtual const CResource* GetParentResource() const = 0;

	CResource* GetParentResource()
	{
		return const_cast< CResource* >( static_cast< const IEventsContainer* >( this )->GetParentResource() );
	}
};

//////////////////////////////////////////////////////////////////////////

class CAnimEventSerializer : public CObject
{
	DECLARE_ENGINE_CLASS( CAnimEventSerializer, CObject, 0 );

public:
	TSortedArray< CExtAnimEvent*, CExtAnimEvent::Sorter >	m_events;

	CAnimEventSerializer();
	~CAnimEventSerializer();

	virtual void OnSerialize( IFile& file );
};

BEGIN_CLASS_RTTI( CAnimEventSerializer )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()
