
#pragma once

#include "storySceneDebug.h"

#ifdef USE_STORY_SCENE_LOADING_STATS
#define BEGIN_SS_LOADING( s, x ) s.StartBlock( TXT( x ) );
#define BEGIN_SS_LOADING_PTR( s, x ) if ( s ) s->StartBlock( TXT( x ) );
#define BEGIN_SS_LOADING_ONCE( s, x ) s.StartBlockOnce( TXT( x ) );
#define END_SS_LOADING( s, x ) s.EndBlock( TXT( x ) );
#define END_SS_LOADING_PTR( s, x ) if (s ) s->EndBlock( TXT( x ) );
#define SS_LOADING_ADD_COUNTER( s, x ) s.AddCounterToBlock( TXT( x ) );
#else
#define BEGIN_SS_LOADING( s, x )
#define BEGIN_SS_LOADING_PTR( s, x )
#define BEGIN_SS_LOADING_ONCE( s, x )
#define END_SS_LOADING( s, x )
#define END_SS_LOADING_PTR( s, x )
#define SS_LOADING_ADD_COUNTER( s, x )
#endif

#ifdef USE_STORY_SCENE_LOADING_STATS

class CStorySceneLoadingStat;

//////////////////////////////////////////////////////////////////////////

class CStorySceneLoadingStatScope
{
	CStorySceneLoadingStat* m_stats;
	String					m_name;

public:
	CStorySceneLoadingStatScope( CStorySceneLoadingStat* s, const String& name );
	~CStorySceneLoadingStatScope();
};

//////////////////////////////////////////////////////////////////////////

class CStorySceneLoadingStat
{
	struct Record 
	{
		String	m_name;
		Float	m_startTime;
		Float	m_duration;
		Int32	m_depth;
		Int32	m_counter;

		Record() : m_startTime( 0.f ), m_duration( 0.f ), m_depth( 0 ), m_counter( 0 ) {}
		Record( const String& name, Float startTime, Int32 depth ) : m_name( name ), m_startTime( startTime ), m_duration( 0.f ), m_depth( depth ), m_counter( 0 ) {}
	};

	String						m_sectionName;
	TDynArray< Record >			m_records;
	Int32						m_currDepth;

	CTimeCounter				m_timer;
	Float						m_durationPlan;
	Double						m_startA;
	Double						m_startB;
	Double						m_durationA;
	Double						m_durationB;

public:
	CStorySceneLoadingStat();

	void Init( const String& section );
	void Deinit();
	Bool IsInitialized() const;

	void StartA();
	void StartB();
	void StopA();
	void StopB();
	
	void StartBlock( const String& name );
	void StartBlockOnce( const String& name );
	void EndBlock( const String& name );

	void AddCounterToBlock( const String& name );
	void SetDurationForPlan( Float d );

	void PrintToLog();

private:
	Record* FindRecord( const String& name );
};
#endif

#ifdef USE_STORY_SCENE_LOADING_STATS
#define SS_LOADING_SCOPE( s, x ) CStorySceneLoadingStatScope scope( &s, TXT( x ) );
#define SS_LOADING_SCOPE_PTR( s, x ) CStorySceneLoadingStatScope scope( s, TXT( x ) );
#else
#define SS_LOADING_SCOPE( s, x )
#define SS_LOADING_SCOPE_PTR( s, x )
#endif

//////////////////////////////////////////////////////////////////////////

struct SStoryScenePlayerDebugStats
{
	static Float DISP_DURATION;

	Float	m_currDeltaTime;
	Float	m_prevDeltaTime;
	Float	m_timeDeltaRatio;
	Float	m_displayTimer;
	Float	m_lastChangeTimer;

	SStoryScenePlayerDebugStats();

	void SetCurrDeltaTime( Float dt );
	void Reset();

	Uint8 GetAlphaAsChar() const;
	Bool ShowMsg( String& outStr ) const;
};

//////////////////////////////////////////////////////////////////////////
