/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEventDuration.h"

namespace StorySceneEventsCollector
{
	struct ActorLookAt;
	struct ActorGameplayLookAt;
}

struct SStorySceneEventLookAtBlinkSettings
{
	DECLARE_RTTI_STRUCT( SStorySceneEventLookAtBlinkSettings )

	Bool		m_canCloseEyes;
	Bool		m_forceCloseEyes;

	CName		m_animationName;

	Float		m_startOffset;
	Float		m_durationPercent;

	Float		m_horizontalAngleDeg;

	SStorySceneEventLookAtBlinkSettings();
};

BEGIN_CLASS_RTTI( SStorySceneEventLookAtBlinkSettings );
	PROPERTY_EDIT( m_canCloseEyes, TXT( "" ) );
	PROPERTY_EDIT( m_forceCloseEyes, TXT( "" ) );
	PROPERTY_EDIT( m_animationName, TXT( "" ) );
	PROPERTY_EDIT( m_startOffset, TXT( "" ) );
	PROPERTY_EDIT( m_durationPercent, TXT( "" ) );
	PROPERTY_EDIT( m_horizontalAngleDeg, TXT( "" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventLookAtDuration	: public CStorySceneEventDuration
										//, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventLookAtDuration, CStorySceneEventDuration )

	friend class CStorySceneEventGenerator;

	static const Vector DEFAULT_STATIC_POINT;

protected:
	CName				m_actor;
	Bool				m_useTwoTargets;

	CName				m_bodyTarget;
	Bool				m_bodyEnabled;
	Bool				m_bodyInstant;
	Float				m_bodyWeight;
	Vector				m_bodyStaticPointWS;
	EDialogLookAtType	m_type;
	ELookAtLevel		m_level;
	//Bool				m_useBodyWeightCurve2;
	//SCurveData			m_bodyWeightCurve;
	Float				m_bodyTransitionWeight;

	CName				m_eyesTarget;
	Bool				m_eyesEnabled;
	Bool				m_eyesInstant;
	Float				m_eyesWeight;
	Float				m_eyesTransitionFactor;
	Vector				m_eyesStaticPointWS;
	Float				m_eyesLookAtConvergenceWeight;
	Bool				m_eyesLookAtIsAdditive;

	Float				m_sceneRange;
	Float				m_gameplayRange;
	Bool				m_limitDeact;

	Float				m_oldLookAtEyesSpeed;
	Float				m_oldLookAtEyesDampScale;

	Bool				m_usesNewTransition;

	EDialogResetClothAndDanglesType		m_resetCloth;
	SStorySceneEventLookAtBlinkSettings	m_blinkSettings;

protected:
	TInstanceVar< Float >	i_blinkAnimationDuration;

public:
	CStorySceneEventLookAtDuration();
	CStorySceneEventLookAtDuration( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& target, Bool enabled, const String& trackName );
	CStorySceneEventLookAtDuration( const CStorySceneEventLookAtDuration& other );

	virtual CStorySceneEventLookAtDuration* Clone() const override;

	virtual void OnPostLoad() override;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

	void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public: // ICurveDataOwner
	//virtual TDynArray< SCurveData* >* GetCurvesData() { return nullptr; }
	//virtual SCurveData* GetCurveData() { return &m_bodyWeightCurve; }

public:
	RED_INLINE virtual CName GetSubject() const { return m_actor; }
	RED_INLINE Bool IsInstant() const { return m_bodyInstant; }
	RED_INLINE Bool UseBlink() const { return false; } //( m_blinkSettings.m_canCloseEyes || m_blinkSettings.m_forceCloseEyes ) && m_blinkSettings.m_animationName; }

#ifndef NO_EDITOR
	RED_INLINE Bool UsesStaticTarget() const { return true; }
	RED_INLINE Bool UsesTwoTargets() const { return m_useTwoTargets; }
	RED_INLINE Vector& GetBodyTargetRef() { return m_bodyStaticPointWS; }
	RED_INLINE Vector& GetEyesTargetRef() { return m_eyesStaticPointWS; }

	//Bool UseBodyWeightCurve() const { return m_useBodyWeightCurve2; }
	//const SCurveData* GetBodyWeightCurve() const { return &m_bodyWeightCurve; }

	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;

private:
	void CalcStaticPointInSS( const CStoryScenePlayer* previewPlayer );
#endif

private:
	void DoLookAt( const CStorySceneInstanceBuffer& instanceBuffer, StorySceneEventsCollector::ActorLookAt& event, const SStorySceneEventTimeInfo& timeInfo, Float weight, const CStoryScenePlayer* scenePlayer ) const;

	void DoOldEyesLookAt( CStoryScenePlayer* scenePlayer, Bool instant, CStorySceneEventsCollector& collector ) const;

	void InitializeWeightCurve();

	Bool ShouldPlayBlink( CStorySceneInstanceBuffer& data, const SStorySceneEventTimeInfo& timeInfo, SAnimationState& outAnimState ) const;
	const CSkeletalAnimationSetEntry* FindMimicAnimation( const CStoryScenePlayer* scenePlayer, const CName& animName ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventLookAtDuration )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) );

	PROPERTY_CUSTOM_EDIT( m_bodyTarget, TXT( "Target" ), TXT( "DialogVoiceTag" ) );
	PROPERTY_EDIT( m_bodyEnabled, TXT( "Enabled or disable look at" ) );
	PROPERTY_EDIT( m_bodyInstant, TXT("Instant") );
	PROPERTY_EDIT_RANGE( m_bodyWeight, TXT("Weight"), 0.f, 1.f );
	PROPERTY_EDIT( m_bodyStaticPointWS, TXT( "Static point for DLT_StaticPoinit look at" ) );
	PROPERTY_EDIT( m_type, TXT( "Look at type" ) );
	PROPERTY_EDIT( m_level, TXT( "Look at level" ) );
	//PROPERTY_EDIT( m_useBodyWeightCurve2, TXT( "" ) );
	//PROPERTY_CUSTOM_EDIT( m_bodyWeightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
	PROPERTY_EDIT( m_bodyTransitionWeight, TXT( "" ) );
	PROPERTY_EDIT( m_usesNewTransition, TXT( "true = new transition, false = old transition from W3" ) );

	PROPERTY_EDIT( m_useTwoTargets, TXT( "Use two targets - one for body, second for eyes" ) );
	PROPERTY_CUSTOM_EDIT( m_eyesTarget, TXT( "Target" ), TXT( "DialogVoiceTag" ) );
	PROPERTY_EDIT( m_eyesEnabled, TXT( "Enabled or disable look at" ) );
	PROPERTY_EDIT( m_eyesInstant, TXT("Instant") );
	PROPERTY_EDIT_RANGE( m_eyesWeight, TXT("Weight"), 0.f, 1.f );
	PROPERTY_EDIT( m_eyesStaticPointWS, TXT( "Static point for DLT_StaticPoinit look at" ) );
	PROPERTY_CUSTOM_EDIT_RANGE( m_eyesLookAtConvergenceWeight, TXT("Use me!"), TXT("Slider"), 0.f, 1.f );
	PROPERTY_EDIT( m_eyesLookAtIsAdditive, TXT("") );

	PROPERTY_EDIT( m_sceneRange, TXT( "Look at range (0-180)" ) );
	PROPERTY_EDIT( m_gameplayRange, TXT( "Look at range to use during gameplay (0-180)" ) );
	PROPERTY_EDIT( m_limitDeact, TXT( "Auto limit deactivation" ) );

	PROPERTY_EDIT( m_resetCloth, TXT( "" ) );

	PROPERTY_EDIT( m_oldLookAtEyesSpeed, TXT( "" ) );
	PROPERTY_EDIT_RANGE( m_oldLookAtEyesDampScale, TXT("higher value means slower look at"), 0.001f, FLT_MAX );

	PROPERTY_EDIT( m_blinkSettings, TXT("") );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventLookAt : public CStorySceneEvent
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventLookAt, CStorySceneEvent )

	friend class CStorySceneEventGenerator;

protected:
	CName				m_actor;
	CName				m_target;
	
	Bool				m_enabled;
	EDialogLookAtType	m_type;
	Bool				m_instant;

	Float				m_speed;
	ELookAtLevel		m_level;

	Float				m_range;
	Float				m_gameplayRange;
	Bool				m_limitDeact;

	Vector				m_staticPoint;

	Float				m_headRotationRatio;
	Float				m_eyesLookAtConvergenceWeight;
	Bool				m_eyesLookAtIsAdditive;
	Float				m_eyesLookAtDampScale;

	EDialogResetClothAndDanglesType m_resetCloth;

public:
	CStorySceneEventLookAt();
	CStorySceneEventLookAt( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& target, Bool enabled, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventLookAt* Clone() const override;

	virtual void OnPostLoad() override;

public:
	void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	virtual CName GetSubject() const override { return m_actor; }

#ifndef NO_EDITOR
	RED_INLINE Bool UsesStaticTarget() const { return m_type == DLT_StaticPoint; }
	RED_INLINE Vector& GetTargetVectorRef() { return m_staticPoint; }

	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
#endif

protected:
	void DoLookAt( CStoryScenePlayer* scenePlayer, Bool instant, CStorySceneEventsCollector& collector ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventLookAt )
	PARENT_CLASS( CStorySceneEvent )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_CUSTOM_EDIT( m_target, TXT( "Target" ), TXT( "DialogVoiceTag" ) )
	PROPERTY_EDIT( m_enabled, TXT( "Enabled or disable look at" ) )
	PROPERTY_EDIT( m_type, TXT( "Look at type" ) )
	PROPERTY_EDIT( m_speed, TXT( "Look at speed. Zero is default speed" ) )
	PROPERTY_EDIT( m_level, TXT( "Look at level" ) )
	PROPERTY_EDIT( m_range, TXT( "Look at range (0-360)" ) )
	PROPERTY_EDIT( m_gameplayRange, TXT( "Look at range to use during gameplay (0-360)" ) )
	PROPERTY_EDIT( m_limitDeact, TXT( "Auto limit deactivation" ) )
	PROPERTY_EDIT( m_instant, TXT( "Instant" ) )
	PROPERTY_EDIT( m_staticPoint, TXT( "Static point for DLT_StaticPoinit look at" ) )
	PROPERTY_EDIT_RANGE( m_headRotationRatio, TXT( "Turn head by a fraction of whole lookat angle " ) , 0.f , 1.f )
	PROPERTY_CUSTOM_EDIT_RANGE( m_eyesLookAtConvergenceWeight, TXT("Use me!"), TXT("Slider"), 0.f, 1.f );
	PROPERTY_EDIT( m_eyesLookAtIsAdditive, TXT("") );
	PROPERTY_EDIT_RANGE( m_eyesLookAtDampScale, TXT("higher value means slower look at"), 0.001f, FLT_MAX );
	PROPERTY_EDIT( m_resetCloth, TXT( "" ) );
END_CLASS_RTTI()

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventGameplayLookAt	: public CStorySceneEventDuration
										, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventGameplayLookAt, CStorySceneEventDuration )

	static const Vector DEFAULT_STATIC_POINT;

protected:
	CName				m_actor;
	CName				m_target;

	Bool				m_enabled;
	Float				m_weight;
	EDialogLookAtType	m_type;
	Bool				m_instant;

	Bool				m_useWeightCurve;
	SCurveData			m_weightCurve;

	Vector				m_staticPoint;

	CName				m_behaviorVarWeight;
	CName				m_behaviorVarTarget;

public:
	CStorySceneEventGameplayLookAt();
	CStorySceneEventGameplayLookAt( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& target, Bool enabled, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventGameplayLookAt* Clone() const override;

public:
	void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() override { return nullptr; }
	virtual SCurveData* GetCurveData() override { return &m_weightCurve; }

public:
	virtual CName GetSubject() const override { return m_actor; }
	Bool IsInstant() const { return m_instant; }

#ifndef NO_EDITOR
	Bool UsesStaticTarget() const { return m_type == DLT_StaticPoint; }
	Vector& GetTargetVectorRef() { return m_staticPoint; }

	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
#endif

private:
	void DoLookAt( const CStorySceneInstanceBuffer& instanceBuffer, StorySceneEventsCollector::ActorGameplayLookAt& event, const SStorySceneEventTimeInfo& timeInfo, const CStoryScenePlayer* scenePlayer ) const;
	void InitializeWeightCurve();

#ifndef NO_EDITOR
	void CalcStaticPointInSS( const CStoryScenePlayer* previewPlayer );
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventGameplayLookAt )
	PARENT_CLASS( CStorySceneEventDuration )
	PROPERTY_CUSTOM_EDIT( m_actor, TXT( "Actor" ), TXT( "DialogVoicePropTag" ) );
	PROPERTY_CUSTOM_EDIT( m_target, TXT( "Target" ), TXT( "DialogVoicePropTag" ) );
	PROPERTY_EDIT( m_enabled, TXT( "Enabled or disable look at" ) );
	PROPERTY_EDIT( m_instant, TXT("Instant") );
	PROPERTY_EDIT_RANGE( m_weight, TXT("Weight"), 0.f, 1.f );
	PROPERTY_EDIT( m_staticPoint, TXT( "Static point for DLT_StaticPoinit look at" ) );
	PROPERTY_EDIT( m_type, TXT( "Look at type" ) );
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
	PROPERTY_EDIT( m_behaviorVarWeight, TXT( "" ) );
	PROPERTY_EDIT( m_behaviorVarTarget, TXT( "" ) );
END_CLASS_RTTI()
