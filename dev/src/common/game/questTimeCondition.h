#pragma once

#include "questCondition.h"

class CQuestTimeCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestTimeCondition, IQuestCondition, 0 )

private:
	GameTime				m_time;
	ECompareFunc			m_comparison;

protected:
	//! IQuestCondition implementation
	virtual Bool OnIsFulfilled();

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "Time: %s, Comparison: %s" ), 
			m_time.ToString().AsChar(), CEnum::ToString( m_comparison ).AsChar() ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQuestTimeCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_time, TXT("Game time."), TXT( "GameTimePropertyEditor" ) )
	PROPERTY_EDIT( m_comparison, TXT("How do we want to compare the specified time.") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestTimePeriodCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestTimePeriodCondition, IQuestCondition, 0 )

private:
	GameTime				m_fromTime;
	GameTime				m_toTime;


protected:
	//! IQuestCondition implementation
	virtual Bool OnIsFulfilled();

public:
#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String	GetDescription() const 
	{ 
		return String::Printf( TXT( "From: %s, To: %s" ), 
			m_fromTime.ToString().AsChar(), m_toTime.ToString().AsChar() ); 
	}
#endif
};

BEGIN_CLASS_RTTI( CQuestTimePeriodCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_fromTime, TXT("Start time."), TXT( "DayTimeEditor" ) )
	PROPERTY_CUSTOM_EDIT( m_toTime, TXT("End time."), TXT( "DayTimeEditor" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestWaitForCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestWaitForCondition, IQuestCondition, 0 )

private:
	GameTime				m_howLong;
	GameTime				m_endTime;

public:
	virtual void SaveGame( IGameSaver* saver );
	virtual void LoadGame( IGameLoader* loader );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestWaitForCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_howLong, TXT("How long should the condition stall for."), TXT( "GameTimePropertyEditor" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestEngineTimeWaitForCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestEngineTimeWaitForCondition, IQuestCondition, 0 )

private:
	GameTime				m_howLong;
	EngineTime				m_endTime;

public:
	virtual void SaveGame( IGameSaver* saver );
	virtual void LoadGame( IGameLoader* loader );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestEngineTimeWaitForCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_howLong, TXT("How long should the condition stall for."), TXT( "GameTimePropertyEditor" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestRealtimeDelayCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestRealtimeDelayCondition, IQuestCondition, 0 )

private:
	GameTime				m_howLong;
	EngineTime				m_endTime;

public:
	virtual void SaveGame( IGameSaver* saver );
	virtual void LoadGame( IGameLoader* loader );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestRealtimeDelayCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_CUSTOM_EDIT( m_howLong, TXT("How long should the condition stall for."), TXT( "DayTimeEditor" ) )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestHiResRealtimeDelayCondition : public IQuestCondition
{
	DECLARE_ENGINE_CLASS( CQuestHiResRealtimeDelayCondition, IQuestCondition, 0 )

private:
	Uint32					m_hours;
	Uint32					m_minutes;
	Uint32					m_seconds;
	Uint32					m_miliseconds;
	EngineTime				m_endTime;

public:
	virtual void SaveGame( IGameSaver* saver );
	virtual void LoadGame( IGameLoader* loader );

protected:
	//! IQuestCondition implementation
	virtual void OnActivate();
	virtual Bool OnIsFulfilled();
};

BEGIN_CLASS_RTTI( CQuestHiResRealtimeDelayCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_hours,       TXT("Hours to wait") )
	PROPERTY_EDIT( m_minutes,     TXT("Minutes to wait") )
	PROPERTY_EDIT( m_seconds,     TXT("Seconds to wait") )
	PROPERTY_EDIT( m_miliseconds, TXT("Miliseconds to wait") )
END_CLASS_RTTI()
