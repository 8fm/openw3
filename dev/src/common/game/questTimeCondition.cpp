#include "build.h"
#include "questTimeCondition.h"
#include "../engine/gameTime.h"
#include "../engine/gameTimeManager.h"
#include "../core/gameSave.h"

IMPLEMENT_ENGINE_CLASS( CQuestTimeCondition )

Bool CQuestTimeCondition::OnIsFulfilled()
{
	GameTime currGameTime = GGame->GetTimeManager()->GetTime();

	switch ( m_comparison )
	{
	case CF_Equal:			return m_time == currGameTime;
	case CF_NotEqual:		return !(m_time == currGameTime);
	case CF_Less:			return m_time < currGameTime;
	case CF_LessEqual:		return m_time <= currGameTime;
	case CF_Greater:		return m_time > currGameTime;
	case CF_GreaterEqual:	return m_time >= currGameTime;
	default:				return true;
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestTimePeriodCondition )

Bool CQuestTimePeriodCondition::OnIsFulfilled()
{
	GameTime currGameTime = GGame->GetTimeManager()->GetTime();
	GameTime currDayTime( 0, currGameTime.Hours(), currGameTime.Minutes(), currGameTime.Seconds() );

	Bool result = false;
	if ( m_fromTime > m_toTime )
	{
		// i.e from 22:00 -> 01:00
		result = ( currDayTime >= m_fromTime ) || ( currDayTime <= m_toTime );
	}
	else
	{
		// i.e. from 01:00 -> 22:00
		result = ( currDayTime >= m_fromTime ) && ( currDayTime <= m_toTime );
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestWaitForCondition )

void CQuestWaitForCondition::OnActivate()
{
	m_endTime = GGame->GetTimeManager()->GetTime() + m_howLong;
}

Bool CQuestWaitForCondition::OnIsFulfilled()
{
	GameTime currGameTime = GGame->GetTimeManager()->GetTime();

	Bool result = ( currGameTime >= m_endTime );
	return result;
}

void CQuestWaitForCondition::SaveGame( IGameSaver* saver )
{
	TBaseClass::SaveGame( saver );

	GameTime timeRemaining = m_endTime - GGame->GetTimeManager()->GetTime();
	saver->WriteValue< GameTime >( CNAME(timeRemaining), timeRemaining );
}

void CQuestWaitForCondition::LoadGame( IGameLoader* loader )
{
	TBaseClass::LoadGame( loader );

	GameTime timeRemaining;
	loader->ReadValue< GameTime >( CNAME(timeRemaining), timeRemaining );

	m_endTime = GGame->GetTimeManager()->GetTime() + timeRemaining;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestEngineTimeWaitForCondition )

void CQuestEngineTimeWaitForCondition::OnActivate()
{
	EngineTime currTime = GGame->GetEngineTime();
	m_endTime = currTime + m_howLong.ToFloat() / ( 60.0f * GGame->GetTimeManager()->GetHoursPerMinute() );
}

Bool CQuestEngineTimeWaitForCondition::OnIsFulfilled()
{
	EngineTime currGameTime = GGame->GetEngineTime();

	Bool result = ( currGameTime >= m_endTime );
	return result;
}

void CQuestEngineTimeWaitForCondition::SaveGame( IGameSaver* saver )
{
	TBaseClass::SaveGame( saver );

	EngineTime timeRemaining = m_endTime - GGame->GetEngineTime();

	saver->WriteValue< Float >( CNAME(timeRemaining), (Float)timeRemaining );
}

void CQuestEngineTimeWaitForCondition::LoadGame( IGameLoader* loader )
{
	TBaseClass::LoadGame( loader );

	Float timeRemaining;
	loader->ReadValue< Float >( CNAME(timeRemaining), timeRemaining );

	// verify that the time doesn't exceed the required amount
	Float waitPeriod = m_howLong.ToFloat() / ( 60.0f * GGame->GetTimeManager()->GetHoursPerMinute() );
	if ( timeRemaining > waitPeriod )
	{
		timeRemaining = waitPeriod;
	}
	if ( timeRemaining < 0 )
	{
		timeRemaining = 0.0f;
	}

	m_endTime = GGame->GetEngineTime() + EngineTime( timeRemaining );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestRealtimeDelayCondition )

void CQuestRealtimeDelayCondition::OnActivate()
{
	EngineTime currTime = GGame->GetEngineTime();
	m_endTime = currTime + m_howLong.ToFloat();
}

Bool CQuestRealtimeDelayCondition::OnIsFulfilled()
{
	EngineTime currGameTime = GGame->GetEngineTime();

	Bool result = ( currGameTime >= m_endTime );
	return result;
}

void CQuestRealtimeDelayCondition::SaveGame( IGameSaver* saver )
{
	TBaseClass::SaveGame( saver );

	EngineTime timeRemaining = m_endTime - GGame->GetEngineTime();

	saver->WriteValue< Float >( CNAME(timeRemaining), (Float)timeRemaining );
}

void CQuestRealtimeDelayCondition::LoadGame( IGameLoader* loader )
{
	TBaseClass::LoadGame( loader );

	Float timeRemaining;
	loader->ReadValue< Float >( CNAME(timeRemaining), timeRemaining );

	m_endTime = GGame->GetEngineTime() + EngineTime( timeRemaining );
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestHiResRealtimeDelayCondition )

void CQuestHiResRealtimeDelayCondition::OnActivate()
{
	EngineTime currTime = GGame->GetEngineTime();
	Float timeToWait = 60 * ( 60 * m_hours + m_minutes ) + m_seconds + static_cast< Float >( m_miliseconds ) / 1000.0f;
	m_endTime = currTime + timeToWait;
}

Bool CQuestHiResRealtimeDelayCondition::OnIsFulfilled()
{
	EngineTime currGameTime = GGame->GetEngineTime();

	Bool result = ( currGameTime >= m_endTime );
	return result;
}

void CQuestHiResRealtimeDelayCondition::SaveGame( IGameSaver* saver )
{
	TBaseClass::SaveGame( saver );

	EngineTime timeRemaining = m_endTime - GGame->GetEngineTime();

	saver->WriteValue< Float >( CNAME(timeRemaining), (Float)timeRemaining );
}

void CQuestHiResRealtimeDelayCondition::LoadGame( IGameLoader* loader )
{
	TBaseClass::LoadGame( loader );

	Float timeRemaining;
	loader->ReadValue< Float >( CNAME(timeRemaining), timeRemaining );

	m_endTime = GGame->GetEngineTime() + EngineTime( timeRemaining );
}
