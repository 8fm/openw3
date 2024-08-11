
#include "build.h"
#include "storyScenePlayerStats.h"
#include "storySceneIncludes.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

#ifdef USE_STORY_SCENE_LOADING_STATS

//////////////////////////////////////////////////////////////////////////

CStorySceneLoadingStatScope::CStorySceneLoadingStatScope( CStorySceneLoadingStat* s, const String& name )
	: m_stats( s )
	, m_name( name )
{
	if ( m_stats )
	{
		m_stats->StartBlock( m_name );
	}
}

CStorySceneLoadingStatScope::~CStorySceneLoadingStatScope()
{
	if ( m_stats )
	{
		m_stats->EndBlock( m_name );
	}
}

//////////////////////////////////////////////////////////////////////////

CStorySceneLoadingStat::CStorySceneLoadingStat()
	: m_currDepth( 0 )
	, m_durationA( 0.0 )
	, m_durationB( 0.0 )
	, m_startA( 0.0 )
	, m_startB( 0.0 )
	, m_durationPlan( 0.f )
{

}

void CStorySceneLoadingStat::Init( const String& section )
{
	m_sectionName = section;

	m_currDepth = 0;
	m_startA = 0.0;
	m_startB = 0.0;
	m_durationA = 0.0;
	m_durationB = 0.0;
	m_durationPlan = 0.f;
	//m_timer.ResetTimer();
	m_records.Clear();

	SCENE_ASSERT( IsInitialized() );
}

void CStorySceneLoadingStat::Deinit()
{
	SCENE_ASSERT( m_currDepth == 0 );

	m_sectionName = String::EMPTY;

	SCENE_ASSERT( !IsInitialized() );
}

void CStorySceneLoadingStat::StartA()
{
	SCENE_ASSERT( m_currDepth == 0 );
	SCENE_ASSERT( IsInitialized() );

	m_startA = m_timer.GetTimePeriodMS();
}

void CStorySceneLoadingStat::StartB()
{
	SCENE_ASSERT( m_currDepth == 0 );
	SCENE_ASSERT( IsInitialized() );

	m_startB = m_timer.GetTimePeriodMS();
}

void CStorySceneLoadingStat::StopA()
{
	SCENE_ASSERT( m_currDepth == 0 );

	m_durationA = m_timer.GetTimePeriodMS() - m_startA;
}

void CStorySceneLoadingStat::StopB()
{
	SCENE_ASSERT( m_currDepth == 0 );

	m_durationB = m_timer.GetTimePeriodMS() - m_startB;
}

Bool CStorySceneLoadingStat::IsInitialized() const
{
	return !m_sectionName.Empty();
}

void CStorySceneLoadingStat::SetDurationForPlan( Float d )
{
	m_durationPlan = d;
}

void CStorySceneLoadingStat::StartBlockOnce( const String& name )
{
	Record* r = FindRecord( name );
	if ( !r )
	{
		StartBlock( name );
	}
}

void CStorySceneLoadingStat::StartBlock( const String& name )
{
	SCENE_ASSERT( FindRecord( name ) == nullptr );

	new ( m_records ) Record( name, (Float)m_timer.GetTimePeriodMS(), m_currDepth );

	m_currDepth++;
}

void CStorySceneLoadingStat::EndBlock( const String& name )
{
	Record* r = FindRecord( name );
	SCENE_ASSERT( r );

	r->m_duration = (Float)m_timer.GetTimePeriodMS() - r->m_startTime;

	m_currDepth--;
}

void CStorySceneLoadingStat::AddCounterToBlock( const String& name )
{
	Record* r = FindRecord( name );
	SCENE_ASSERT( r );

	r->m_counter++;
}

CStorySceneLoadingStat::Record* CStorySceneLoadingStat::FindRecord( const String& name )
{
	for ( Record& r : m_records )
	{
		if ( r.m_name == name )
		{
			return &r;
		}
	}

	return nullptr;
}

void CStorySceneLoadingStat::PrintToLog()
{
	RED_LOG( SceneLoading, TXT("****************************************************") );
	RED_LOG( SceneLoading, TXT("Story Scene Loading Stats") );
	RED_LOG( SceneLoading, TXT("Section: '%ls'"), m_sectionName.AsChar() );
	RED_LOG( SceneLoading, TXT("Stage A: [%.5f] ms"), (Float)m_durationA );
	RED_LOG( SceneLoading, TXT("Stage B: [%.5f] ms"), (Float)m_durationB );
	RED_LOG( SceneLoading, TXT("Plan  W: [%.5f] ms"), m_durationPlan );
	
	for ( Record& r : m_records )
	{
		String msg;
		for ( Int32 i=0; i<r.m_depth; ++i )
		{
			msg += TXT("  ");
		}
		msg += TXT(">");
		if ( r.m_counter == 0 )
		{
			RED_LOG( SceneLoading, TXT("%ls%ls: %.5f ms"), msg.AsChar(), r.m_name.AsChar(), r.m_duration );
		}
		else
		{
			RED_LOG( SceneLoading, TXT("%ls%ls: %.5f ms [%d]"), msg.AsChar(), r.m_name.AsChar(), r.m_duration, r.m_counter );
		}
	}

	RED_LOG( SceneLoading, TXT("****************************************************") );
}

#endif

//////////////////////////////////////////////////////////////////////////

Float SStoryScenePlayerDebugStats::DISP_DURATION = 0.5f;

SStoryScenePlayerDebugStats::SStoryScenePlayerDebugStats()
	: m_currDeltaTime( 0.f )
	, m_prevDeltaTime( 0.f )
	, m_displayTimer( 0.f )
	, m_timeDeltaRatio( 1.f )
	, m_lastChangeTimer( 0.f )
{

}

void SStoryScenePlayerDebugStats::SetCurrDeltaTime( Float dt )
{
	m_prevDeltaTime = m_currDeltaTime;
	m_currDeltaTime = dt;

	const Float warnRatio = 2.f;

	const Float ratioA = m_prevDeltaTime > 0.f && m_currDeltaTime > 0.f ? m_currDeltaTime / m_prevDeltaTime : 1.f;
	const Float ratioB = m_prevDeltaTime > 0.f && m_currDeltaTime > 0.f ? m_prevDeltaTime / m_currDeltaTime : 1.f;

	if ( ratioA > warnRatio || ratioB > warnRatio )
	{
		const Float ratio = Max( ratioA, ratioB );

		if ( m_lastChangeTimer > 0.f )
		{
			if ( ratio > m_timeDeltaRatio )
			{
				m_timeDeltaRatio = ratio;
			}
		}
		else
		{
			m_timeDeltaRatio = ratio;
			m_lastChangeTimer = DISP_DURATION;
		}

		m_displayTimer = DISP_DURATION;

		SCENE_LOG( TXT("Time delta signal is not smooth - curr %1.3f, prev %1.3f, ratio %1.1f"), m_currDeltaTime, m_prevDeltaTime, m_timeDeltaRatio );
	}
	else
	{
		m_displayTimer -= dt;
	}

	m_lastChangeTimer -= dt;
}

void SStoryScenePlayerDebugStats::Reset()
{
	m_currDeltaTime = 0.f;
	m_prevDeltaTime = 0.f;
	m_displayTimer = 0.f;
	m_lastChangeTimer = 0.f;
}

Uint8 SStoryScenePlayerDebugStats::GetAlphaAsChar() const
{
	SCENE_ASSERT( m_displayTimer > 0.f );
	return (Uint8)(( m_displayTimer / DISP_DURATION ) * 255);
}

Bool SStoryScenePlayerDebugStats::ShowMsg( String& outStr ) const
{
	Bool ret = false;

	if ( m_displayTimer > 0.f )
	{
		outStr = String::Printf( TXT("%1.1f"), m_timeDeltaRatio );
		ret = true;
	}

	return ret;
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
