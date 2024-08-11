
#pragma once

enum EAnimEffectAction
{
	EA_Start,
	EA_Stop
};

BEGIN_ENUM_RTTI( EAnimEffectAction )
	ENUM_OPTION( EA_Start )
	ENUM_OPTION( EA_Stop )
END_ENUM_RTTI()

class CExtAnimEffectEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimEffectEvent )

public:
	CExtAnimEffectEvent();

	CExtAnimEffectEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExtAnimEffectEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

public:
	CName	GetEffectName() const { return m_effectName; }
	EAnimEffectAction GetEffectAction() const { return m_action; }

protected:
	CName				m_effectName;
	EAnimEffectAction	m_action;
};

BEGIN_CLASS_RTTI( CExtAnimEffectEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_effectName, TXT( "Name of the effect to start or stop" ) );
	PROPERTY_EDIT( m_action, TXT( "Should effect be started or stopped" ) );
END_CLASS_RTTI();

class CExtAnimEffectDurationEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimEffectDurationEvent )

public:
	CExtAnimEffectDurationEvent();

	CExtAnimEffectDurationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimEffectDurationEvent();

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

public:
	CName	GetEffectName() const { return m_effectName; }

protected:
	CName				m_effectName;

	void ProcessEffect( const CAnimationEventFired& info, CAnimatedComponent* component, EAnimEffectAction action ) const;
};

BEGIN_CLASS_RTTI( CExtAnimEffectDurationEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_effectName, TXT( "Name of the effect to start or stop" ) );
END_CLASS_RTTI();
