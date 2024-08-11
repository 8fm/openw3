
#pragma once

class CExtAnimGameplayMimicEvent : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimGameplayMimicEvent )

protected:
	CName	m_animation;

public:
	CExtAnimGameplayMimicEvent();
	CExtAnimGameplayMimicEvent( const CName& eventName, const CName& animationName, Float startTime, Float duration, const String& trackName );
	virtual ~CExtAnimGameplayMimicEvent() {}

	virtual void Start( const CAnimationEventFired& info, CAnimatedComponent* component ) const;
	virtual void Stop( const CAnimationEventFired& info, CAnimatedComponent* component ) const;

#ifndef NO_EDITOR
	virtual void OnPropertyPostChanged( const CName& propertyName ) override;
	static const CSkeletalAnimationSet* GetAnimationSet();
#endif
};

BEGIN_CLASS_RTTI( CExtAnimGameplayMimicEvent );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_CUSTOM_EDIT( m_animation, TXT(""), TXT("GameplayMimicSelection") );
END_CLASS_RTTI();
