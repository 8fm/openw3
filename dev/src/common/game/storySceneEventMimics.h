/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEventAnimClip.h"

class CStorySceneEventMimics	: public CStorySceneEventAnimClip
								, public IDialogMimicsAnimationFilterInterface
								, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventMimics, CStorySceneEventAnimClip )

	// TO REMOVE
	CName			m_stateName;

protected:
	CName			m_mimicsEmotionalState;

	CName			m_mimicsLayer_Eyes;
	CName			m_mimicsLayer_Pose;
	CName			m_mimicsLayer_Animation;

	CName			m_transitionAnimation;
	CName			m_forceMimicsIdleAnimation_Eyes;
	CName			m_forceMimicsIdleAnimation_Pose;
	CName			m_forceMimicsIdleAnimation_Animation;

	Float			m_mimicsPoseWeight;

	Bool			m_useWeightCurve;
	SCurveData		m_weightCurve;

public:
	CStorySceneEventMimics();
	CStorySceneEventMimics( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventMimics* Clone() const override;

public:
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public: // IDialogMimicsAnimationFilterInterface
	virtual const CName& GetMimicsFilterEmotionalState() const override { return m_mimicsEmotionalState; }
	virtual const CName& GetMimicsFilterActor() const override { return m_actor; }

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() { return nullptr; }
	virtual SCurveData* GetCurveData() { return &m_weightCurve; }

public:
	virtual const CName& GetAnimationName() const override { return m_transitionAnimation; }

	virtual const CAnimatedComponent* GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const;

	virtual void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const override;

public:
	RED_INLINE void SetAnimationName( const CName& animName ) { m_stateName = animName; }

	RED_INLINE const CName& GetMimicFilterEyesLayer() const { return m_mimicsLayer_Eyes; }
	RED_INLINE const CName& GetMimicFilterPoseLayer() const { return m_mimicsLayer_Pose; }
	RED_INLINE const CName& GetMimicFilterAnimationLayer() const { return m_mimicsLayer_Animation; }

	virtual void OnSerialize( IFile& file ) override;
	Float GetMimicPoseWeight() const { return m_mimicsPoseWeight; }
	void SetMimicPoseWeight( Float val ) { m_mimicsPoseWeight = val; }
	void SetMimicData( const TDynArray<CName>& data );
protected:
	virtual Bool IsBodyOrMimicMode() const override { return false; } // Body = true, Mimics = false
	virtual Bool ShouldPlayEmptyAnimation() const override { return false; }

private:
	void CacheMimicsLayerFromEmoState( Bool force );

	void SetupEventAnimationData( StorySceneEventsCollector::ActorChangeState& evt ) const;

	Bool DoesForceAnyIdle() const;

#ifndef NO_EDITOR
public:
	Bool UseWeightCurve() const { return m_useWeightCurve; }
	const SCurveData* GetWeightCurve() const { return &m_weightCurve; }
	void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName );

private:
	void SetCustomEmotionalState();
#endif
};

BEGIN_CLASS_RTTI( CStorySceneEventMimics )
	PARENT_CLASS( CStorySceneEventAnimClip )
	PROPERTY_RO( m_stateName, TXT( "Mimics idle animation name" ) )
	PROPERTY_CUSTOM_EDIT( m_mimicsEmotionalState, TXT( "Mimics emotional state" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_mimicsLayer_Eyes, TXT( "Mimics layer eyes" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_mimicsLayer_Pose, TXT( "Mimics layer pose" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_mimicsLayer_Animation, TXT( "Mimics layer animation" ), TXT( "DialogMimicsAnimation_EmotionalState" ) );
	PROPERTY_EDIT( m_mimicsPoseWeight, TXT( "" ) );
	PROPERTY_EDIT( m_transitionAnimation, TXT( "Name of transition animation" ) );
	PROPERTY_CUSTOM_EDIT( m_forceMimicsIdleAnimation_Eyes, TXT("Force mimics idle animation"), TXT("DialogMimicAnimationSelection") );
	PROPERTY_CUSTOM_EDIT( m_forceMimicsIdleAnimation_Pose, TXT("Force mimics idle animation"), TXT("DialogMimicAnimationSelection") );
	PROPERTY_CUSTOM_EDIT( m_forceMimicsIdleAnimation_Animation, TXT("Force mimics idle animation"), TXT("DialogMimicAnimationSelection") );
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
END_CLASS_RTTI()
