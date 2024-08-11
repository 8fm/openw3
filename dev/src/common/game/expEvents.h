
#pragma once

class CExpSyncEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExpSyncEvent )

	Bool	m_translation;
	Bool	m_rotation;

public:
	CExpSyncEvent();

	CExpSyncEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );
	virtual ~CExpSyncEvent() {}

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
};

BEGIN_CLASS_RTTI( CExpSyncEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_translation, TXT("") );
	PROPERTY_EDIT( m_rotation, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExpSlideEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExpSlideEvent )

	Bool	m_translation;
	Bool	m_rotation;
	Bool	m_toCollision;

public:
	CExpSlideEvent();

	CExpSlideEvent( const CName& eventName, 
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExpSlideEvent() {}

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

public:
	RED_INLINE Bool Translation() const { return m_translation; }
    RED_INLINE Bool Rotation() const { return m_rotation; }
	RED_INLINE Bool ToCollision() const { return m_toCollision; }
};

BEGIN_CLASS_RTTI( CExpSlideEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_translation, TXT("") );
    PROPERTY_EDIT( m_rotation, TXT("") );
	PROPERTY_EDIT( m_toCollision, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
