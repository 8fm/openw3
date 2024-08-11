
#pragma once

#include "storySceneEventDuration.h"
#include "storyScenePlayer.h"
#include "voiceWeightCurve.h"
#include "storySceneEventInterfaces.h"

class CStorySceneEventAnimClip	: public CStorySceneEventDuration
								, public ISSAnimClipInterface
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventAnimClip, CStorySceneEventDuration );

	RED_DECL_INTERFACE_SUPPORT_1( ISSAnimClipInterface )

protected:
	CName		m_actor;
	Float		m_blendIn;
	Float		m_blendOut;
	Float		m_clipFront;
	Float		m_clipEnd;
	Float		m_stretch;
	Float		m_weight;
	ELookAtLevel m_allowLookatsLevel;

	Bool		m_forceAnimationTimeFlag;
	Float		m_forceAnimationTime;

	SVoiceWeightCurve m_voiceWeightCurve;

	Bool		m_allowPoseCorrection;

#ifndef NO_EDITOR
protected:
	Float	m_cachedAnimationDuration;
#endif

public:
	CStorySceneEventAnimClip();
	CStorySceneEventAnimClip( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

public:
	virtual void OnDeinit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	RED_INLINE const CName& GetActor() const			{ return m_actor; }
	RED_INLINE Float GetAnimationClipStart() const	{ return m_clipFront; }
	RED_INLINE Float GetAnimationClipEnd() const		{ return m_clipEnd; }
	RED_INLINE Float GetAnimationBlendIn() const		{ return m_blendIn; }
	RED_INLINE Float GetAnimationBlendOut() const		{ return m_blendOut; }
	RED_INLINE Float GetAnimationStretch() const		{ return m_stretch; }
	RED_INLINE Bool HasAnimation() const				{ return GetAnimationName() != CName::NONE; }

	virtual const CName& GetAnimationName() const		{ return CName::NONE; }
	virtual const CName& GetAnimationSlotName() const	{ return CNAME( MANUAL_DIALOG_SLOT ); }
	
	virtual const CAnimatedComponent* GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const;

	virtual CName GetSubject() const override { return GetActor(); }

	virtual void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const override;

protected:
	virtual Float CalculateBlendWeight( Float eventTime, Float eventDuration ) const;

	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const {}
	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::MimicsAnimation& event ) const {}

	virtual Bool ShouldPlayEmptyAnimation() const { return true; }

	virtual Bool IsBodyOrMimicMode() const { return true; } // Body = true, Mimics = false

	virtual void AddEventToCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, SAnimationState& dialogAnimationState, const SStorySceneEventTimeInfo& timeInfo, Float blendWeight ) const;
	virtual void RemoveEventFromCollector( CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const;

#ifndef  NO_EDITOR
public:
	RED_INLINE void SetActor( const CName& actor )			{ m_actor = actor; }
	
	void SetAnimationClipStart( Float clipFront );
	void SetAnimationClipEnd( Float clipEnd );
	void SetAnimationStretch( Float stretch );

	RED_INLINE void SetAnimationBlendIn( Float blendIn )		{ m_blendIn = blendIn; }
	RED_INLINE void SetAnimationBlendOut( Float blendOut )	{ m_blendOut = blendOut; }
	RED_INLINE void SetAnimationWeight( Float weight )		{ m_weight = weight; }
	RED_INLINE SVoiceWeightCurve& GetVoiceWeightCurve()		{ return m_voiceWeightCurve; }

	Float RefreshDuration( Float animationDuration );
	Float RefreshDuration( const CStoryScenePlayer* previewPlayer );
	Float GetAnimationDuration( const CStoryScenePlayer* previewPlayer ) const;
	Float GetCachedAnimationDuration() const { return m_cachedAnimationDuration; }
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
	ELookAtLevel GetAllowedLookatLevel() const { return m_allowLookatsLevel; }
#endif

protected:
	void DoBakeScaleImpl( Float scalingFactor ) override;
};

BEGIN_CLASS_RTTI( CStorySceneEventAnimClip );
	PARENT_CLASS( CStorySceneEventDuration );
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoicePropTag" ) );
	PROPERTY_EDIT( m_blendIn, TXT( "Blend time of animation start") );
	PROPERTY_EDIT( m_blendOut, TXT( "Blend time of animation end") );
	PROPERTY_EDIT( m_clipFront, TXT( "Clip time of animation start") );
	PROPERTY_EDIT( m_clipEnd, TXT( "Clip time of animation end") );
	PROPERTY_EDIT( m_stretch, TXT( "Animation stretch factor" ) );
	PROPERTY_EDIT( m_allowLookatsLevel, TXT( "Clip lookat poses to this level" ) );
	PROPERTY_EDIT_RANGE( m_weight, TXT("Animation weight factor [0,1]"), 0.f, 1.f );
#ifndef NO_EDITOR
	PROPERTY_NOT_COOKED( m_cachedAnimationDuration );
#endif
	PROPERTY_EDIT( m_forceAnimationTimeFlag, TXT( "" ) );
	PROPERTY_EDIT( m_forceAnimationTime, TXT( "" ) );
	PROPERTY_EDIT( m_voiceWeightCurve, TXT( "" ) );
	PROPERTY_EDIT( m_allowPoseCorrection, TXT( "" ) );
END_CLASS_RTTI();
