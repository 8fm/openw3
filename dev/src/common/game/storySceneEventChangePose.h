/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneEventAnimClip.h"
#include "storySceneIncludes.h"

class CStorySceneEventChangePose	: public CStorySceneEventAnimClip
									, public IDialogBodyAnimationFilterInterface
									, public ICurveDataOwner
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventChangePose, CStorySceneEventAnimClip );

protected:
	CName						m_stateName;

	CName						m_status;
	CName						m_emotionalState;
	CName						m_poseName;

	CName						m_transitionAnimation;
	Bool						m_useMotionExtraction;

	CName						m_forceBodyIdleAnimation;

	Bool						m_useWeightCurve;
	SCurveData					m_weightCurve;

	EDialogResetClothAndDanglesType m_resetCloth;

protected:
	TInstanceVar< CSkeletalAnimationSetEntry* > i_animation;
	TInstanceVar< Bool >						i_hasMotion;

public:
	CStorySceneEventChangePose();
	CStorySceneEventChangePose( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName );
	CStorySceneEventChangePose( const CStorySceneEventChangePose& other );

	virtual CStorySceneEventChangePose* Clone() const override;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

public:
	virtual const CName& GetAnimationName() const override { return m_transitionAnimation; }
	virtual void SetAnimationName( CName newName ) { m_transitionAnimation = newName; }
	virtual void OnSerialize( IFile& file ) override;
	void SetAnimationState( const TDynArray< CName >& state );
	RED_INLINE const CName& GetForceBodyIdleAnimation() const { return m_forceBodyIdleAnimation; }

	virtual void CollectUsedAnimations( CStorySceneAnimationContainer& container ) const override;

protected:
	virtual const CName& GetAnimationSlotName() const override { return CNAME( POSE_DIALOG_SLOT ); }

	virtual Bool ShouldPlayEmptyAnimation() const override { return false; }

public: // IDialogBodyAnimationFilterInterface
	virtual const CName& GetBodyFilterStatus() const override			{ return m_status; }
	virtual const CName& GetBodyFilterEmotionalState() const override	{ return m_emotionalState; }
	virtual const CName& GetBodyFilterPoseName() const override			{ return m_poseName; }
	virtual const CName& GetBodyFilterTypeName() const override			{ return CName::NONE; }
	virtual const CName& GetBodyFilterActor() const	override			{ return m_actor; }

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData() { return nullptr; }
	virtual SCurveData* GetCurveData() { return &m_weightCurve; }

#ifndef  NO_EDITOR
public:
	Bool UseWeightCurve() const { return m_useWeightCurve; }
	const SCurveData* GetWeightCurve() const { return &m_weightCurve; }
	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;
#endif

private:
	void FillState( SStorySceneActorAnimationState& state ) const;

	CSkeletalAnimationSetEntry* FindAnimation( const CStoryScenePlayer* scenePlayer ) const;
	Bool HasAnimationMotion( const CSkeletalAnimationSetEntry* animation ) const;
	void ApplyAnimationMotion( CStorySceneInstanceBuffer& data, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventChangePose );
	PARENT_CLASS( CStorySceneEventAnimClip );
	PROPERTY_RO( m_stateName, TXT( "Name of pose" ) );
	PROPERTY_CUSTOM_EDIT( m_status, TXT( "Status" ), TXT( "DialogBodyAnimation_Status" ) );
	PROPERTY_CUSTOM_EDIT( m_emotionalState, TXT( "Emotional state" ), TXT( "DialogBodyAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT( m_poseName, TXT( "Pose name" ), TXT( "DialogBodyAnimation_Pose" ) );
	PROPERTY_CUSTOM_EDIT( m_transitionAnimation, TXT( "Name of transition animation" ), TXT( "DialogBodyAnimation_Transition" )  );
	PROPERTY_EDIT( m_useMotionExtraction, String::EMPTY );
	PROPERTY_CUSTOM_EDIT( m_forceBodyIdleAnimation, TXT("Force body idle animation"), TXT("DialogAnimationSelection") );
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
	PROPERTY_EDIT( m_resetCloth, TXT("") )
END_CLASS_RTTI();
