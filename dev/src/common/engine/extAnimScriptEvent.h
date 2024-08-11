/*
 * Copyright © 2012 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CExtAnimScriptEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimScriptEvent )

public:
	CExtAnimScriptEvent();
	CExtAnimScriptEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName );

	virtual ~CExtAnimScriptEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const;
};

BEGIN_CLASS_RTTI( CExtAnimScriptEvent );
	PARENT_CLASS( CExtAnimEvent );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct SMultiValue
{
	DECLARE_RTTI_STRUCT( SMultiValue )

	TDynArray< Float >			m_floats;
	TDynArray< Bool >			m_bools;
	TDynArray< SEnumVariant >	m_enums;
	TDynArray< CName >			m_names;
};

BEGIN_CLASS_RTTI( SMultiValue )
	PROPERTY_EDIT( m_floats, TXT("Float variables") )
	PROPERTY_EDIT( m_bools, TXT("Bool variables") )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_enums, TXT("Enum variables"), TXT("EnumVariantSelection") )
	PROPERTY_EDIT( m_names, TXT("Name variables") )
END_CLASS_RTTI()

class CEASMultiValueSimpleEvent : public CExtAnimScriptEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CEASMultiValueSimpleEvent )

protected:
	CName		m_callback;
	SMultiValue	m_properties;

public:
	CEASMultiValueSimpleEvent() : CExtAnimScriptEvent() {};

	CEASMultiValueSimpleEvent( const CName& eventName,
		const CName& animationName, Float startTime, const String& trackName )
		: CExtAnimScriptEvent( eventName, animationName, startTime, trackName )
		, m_callback( CName::NONE ) {};

	virtual void SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const;
};


BEGIN_CLASS_RTTI( CEASMultiValueSimpleEvent )
	PARENT_CLASS( CExtAnimScriptEvent )
	PROPERTY_EDIT( m_callback, TXT("Name of the scripted event to be called (ie. OnAnimEvent())") )
	PROPERTY_INLINED( m_properties, TXT("All variables") )
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CExtAnimScriptDurationEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimScriptDurationEvent )

public:
	CExtAnimScriptDurationEvent();

	CExtAnimScriptDurationEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimScriptDurationEvent();

	virtual void Process( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const;
};

BEGIN_CLASS_RTTI( CExtAnimScriptDurationEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

RED_DECLARE_NAME( OnSlideToTargetAnimEvent );

struct SSlideToTargetEventProps
{
	DECLARE_RTTI_STRUCT( SSlideToTargetEventProps )

	Float	m_minSlideDist;
	Float	m_maxSlideDist;
	Bool	m_slideToMaxDistIfTargetSeen;
	Bool	m_slideToMaxDistIfNoTarget;
};

BEGIN_CLASS_RTTI( SSlideToTargetEventProps );
	PROPERTY_EDIT( m_minSlideDist, TXT("Minimum slide distance") );
	PROPERTY_EDIT( m_maxSlideDist, TXT("Maximum slide distance") );
	PROPERTY_EDIT( m_slideToMaxDistIfTargetSeen, TXT("Should slide if target is seen?") );
	PROPERTY_EDIT( m_slideToMaxDistIfNoTarget, TXT("Shoul slide if no target?") );
END_CLASS_RTTI();

class CEASSlideToTargetEvent : public CExtAnimScriptDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CEASSlideToTargetEvent )

protected:
	SSlideToTargetEventProps	m_properties;

public:
	CEASSlideToTargetEvent() : CExtAnimScriptDurationEvent() {};

	CEASSlideToTargetEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName )
		: CExtAnimScriptDurationEvent( eventName, animationName, startTime, duration, trackName ) {};

	virtual void SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const;
};

BEGIN_CLASS_RTTI( CEASSlideToTargetEvent );
	PARENT_CLASS( CExtAnimScriptDurationEvent );
	PROPERTY_EDIT( m_properties, TXT("Event properties") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

RED_DECLARE_NAME( OnEnumAnimEvent );

struct SEnumVariant
{
	DECLARE_RTTI_STRUCT( SEnumVariant )

	CName	m_enumType;
	Int32		m_enumValue;
};

BEGIN_CLASS_RTTI( SEnumVariant );
	PROPERTY( m_enumType );
	PROPERTY( m_enumValue );
END_CLASS_RTTI();

class CEASEnumEvent : public CExtAnimScriptDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CEASEnumEvent )

protected:
	SEnumVariant	m_enumVariant;

public:
	CEASEnumEvent() : CExtAnimScriptDurationEvent() {};

	CEASEnumEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName )
		: CExtAnimScriptDurationEvent( eventName, animationName, startTime, duration, trackName ) {};

	virtual void SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const;

	virtual Bool CanBeMergedWith( const CExtAnimEvent* _with ) const;

	RED_INLINE const SEnumVariant& GetEnumData() const { return m_enumVariant; }
};

BEGIN_CLASS_RTTI( CEASEnumEvent );
	PARENT_CLASS( CExtAnimScriptDurationEvent );
	PROPERTY_CUSTOM_EDIT( m_enumVariant, TXT("Event enum value"), TXT("EnumVariantSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CEASMultiValueEvent : public CExtAnimScriptDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CEASMultiValueEvent )

protected:
	CName		m_callback;
	SMultiValue	m_properties;

public:
	CEASMultiValueEvent() : CExtAnimScriptDurationEvent() {};

	CEASMultiValueEvent( const CName& eventName,
		const CName& animationName, Float startTime, Float duration, const String& trackName )
		: CExtAnimScriptDurationEvent( eventName, animationName, startTime, duration, trackName )
		, m_callback( CName::NONE ) {};

	virtual void SendEventToScript( CEntity* entity, EAnimationEventType type, const SAnimationEventAnimInfo& animInfo ) const;
};


BEGIN_CLASS_RTTI( CEASMultiValueEvent );
	PARENT_CLASS( CExtAnimScriptDurationEvent );
	PROPERTY_EDIT( m_callback, TXT("Name of the scripted event to be called (ie. OnAnimEvent())") );
	PROPERTY_INLINED( m_properties, TXT("All variables") );
END_CLASS_RTTI();