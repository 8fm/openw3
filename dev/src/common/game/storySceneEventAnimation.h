/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneEventAnimClip.h"
#include "storySceneIncludes.h"
#include "storySceneEventInterfaces.h"

struct SStorySceneActorAnimationState;

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventAnimation : public CStorySceneEventAnimClip
#ifndef NO_EDITOR
								, public IDialogBodyAnimationFilterInterface
								, public IDialogBodyPartOwner
#endif
								, public ICurveDataOwner
								, public ISSDragAndDropBodyAnimInterface
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventAnimation, CStorySceneEventAnimClip )
	
	RED_ADD_INTERFACE_SUPPORT_1( ISSDragAndDropBodyAnimInterface )

protected:
	CName								m_animationName;
	Bool								m_useMotionExtraction;
	Bool								m_useFakeMotion;
	Bool								m_disableLookAt;
	Float								m_disableLookAtSpeed;
	Bool								m_useLowerBodyPartsForLookAt;
	Bool								m_gatherSyncTokens;
	Bool								m_muteSoundEvents;

#ifndef NO_EDITOR
	String								m_bonesGroupName;
	TDynArray< SBehaviorGraphBoneInfo >	m_bones;
#endif
	TDynArray< Int32 >					m_bonesIdx;
	TDynArray< Float >					m_bonesWeight;

#ifndef NO_EDITOR
	CName								m_status;
	CName								m_emotionalState;
	CName								m_poseName;
	CName								m_typeName;
	String								m_friendlyName;
#endif

	EStorySceneAnimationType			m_animationType;
	Bool								m_addConvertToAdditive;
	EAdditiveType						m_addAdditiveType;

	Bool								m_useWeightCurve;
	SCurveData							m_weightCurve;
	Bool								m_weightCurveChanged;

	Bool								m_supportsMotionExClipFront;

#ifndef NO_EDITOR
	Bool								m_recacheWeightCurve;
#endif

protected:
	TInstanceVar< CSkeletalAnimationSetEntry* > i_animation;
	TInstanceVar< Bool >						i_hasMotion;

public:
	CStorySceneEventAnimation();
	CStorySceneEventAnimation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& animName, const String& trackName );
	CStorySceneEventAnimation( const CStorySceneEventAnimation& other );

	virtual CStorySceneEventAnimation* Clone() const override;

	virtual const CName& GetAnimationName() const override { return m_animationName; }

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;

	virtual void OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const override;

	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;

	//++ do wywalenia
	RED_INLINE Bool ShouldDisableLookAt() const { return m_disableLookAt; }
	RED_INLINE Float GetDisableLookAtSpeed() const { return m_disableLookAtSpeed; }
	//--

	virtual void OnSerialize( IFile& file ) override;

public: // ISSDragAndDropAnimInterface
	virtual void Interface_SetDragAndDropBodyAnimation( const TDynArray< CName >& animData, Float animDuration );

public: // ICurveDataOwner
	virtual TDynArray< SCurveData* >* GetCurvesData()	{ return nullptr; }
	virtual SCurveData* GetCurveData()					{ return &m_weightCurve; }
	virtual void OnCurveChanged();

#ifndef  NO_EDITOR
public: // IDialogBodyAnimationFilterInterface
	virtual const CName& GetBodyFilterStatus() const override			{ return m_status; }
	virtual const CName& GetBodyFilterEmotionalState() const override	{ return m_emotionalState; }
	virtual const CName& GetBodyFilterPoseName() const override			{ return m_poseName; }
	virtual const CName& GetBodyFilterTypeName() const override			{ return m_typeName; }
	virtual const CName& GetBodyFilterActor() const	override			{ return m_actor; }

	RED_INLINE const String& GetAnimationFriendlyName() const { return m_friendlyName; }

	RED_INLINE void SetBodyFilterStatus( const CName& name ) 			{ m_status = name; }
	RED_INLINE void SetBodyFilterEmotionalState( const CName& name )	{ m_emotionalState = name; }
	RED_INLINE void SetBodyFilterPoseName( const CName& name ) 		{ m_poseName = name; }
	RED_INLINE void SetBodyFilterTypeName( const CName& name ) 		{ m_typeName = name; }

	RED_INLINE void SetAnimationFriendlyName( const String& name ) { m_friendlyName = name; }

public: // IDialogBodyPartOwner
	virtual TDynArray< SBehaviorGraphBoneInfo >* GetBodyPartBones() { return &m_bones; }
	virtual void OnBodyPartsChanged();

public:
	virtual const CName& Interface_GetAnimation() { return m_animationName; }

public: // IStorySceneAnimEventInterface
	void SetAnimationName( const CName& animName ) { m_animationName = animName; }

	virtual void OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName ) override;

	void SuckDataFromActorState( const SStorySceneActorAnimationState& state );
	void SetAnimationState( const TDynArray< CName >& state );

	void CopyFrom( const CStorySceneEventAnimation* rhs );

	void OnBonesListChanged();

	Bool UseWeightCurve() const { return m_useWeightCurve; }
	const SCurveData* GetWeightCurve() const { return &m_weightCurve; }
#endif

protected:
	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const override;
	virtual Float CalculateBlendWeight( Float eventTime, Float eventDuration ) const override;

private:
	CSkeletalAnimationSetEntry* FindAnimation( const CStoryScenePlayer* scenePlayer ) const;

	Bool HasAnimationMotion( const CSkeletalAnimationSetEntry* animation ) const;

	void CopyBlendsValsToWeightCurve( Bool force = false );

	void ApplyAnimationMotion( CStorySceneInstanceBuffer& data, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const;
};

BEGIN_CLASS_RTTI( CStorySceneEventAnimation )
	PARENT_CLASS( CStorySceneEventAnimClip )
	PROPERTY_CUSTOM_EDIT( m_animationName, TXT("Animation name"), TXT("DialogAnimationSelection") );
	PROPERTY_EDIT( m_useMotionExtraction, TXT("Use motion extraction from animation") );
	PROPERTY_EDIT( m_useFakeMotion, TXT("Add motion extraction to root bone") );
	PROPERTY_EDIT( m_gatherSyncTokens, TXT("") );
	PROPERTY_EDIT( m_muteSoundEvents, TXT( "Mute sound events" ) );
	PROPERTY_EDIT( m_disableLookAt, TXT( "Should disable look at when starting gesture" ) )
	PROPERTY_EDIT( m_disableLookAtSpeed, TXT( "Disable look speed" ) )
	PROPERTY_EDIT( m_useLowerBodyPartsForLookAt, TXT( "Use lower body parts for look at" ) )
#ifndef NO_EDITOR
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_bonesGroupName, String::EMPTY, TXT("DialogBodyPartSelection_GroupName") );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_bones, String::EMPTY, TXT("DialogBodyPartSelection_Bones") );
#endif
	PROPERTY_RO( m_bonesIdx, TXT( "" ) );
	PROPERTY_RO( m_bonesWeight, TXT( "" ) );
#ifndef NO_EDITOR
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_status, TXT( "Status" ), TXT( "DialogBodyAnimation_Status" ) );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_emotionalState, TXT( "Emotional state" ), TXT( "DialogBodyAnimation_EmotionalState" ) );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_poseName, TXT( "Pose name" ), TXT( "DialogBodyAnimation_Pose" ) );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_typeName, TXT( "Type name" ), TXT( "DialogBodyAnimation_Type" ) );
	PROPERTY_CUSTOM_EDIT_NOT_COOKED( m_friendlyName, TXT( "Animation friendly name" ), TXT( "DialogBodyAnimation_FriendlyName" ) );
#endif
	PROPERTY_EDIT( m_animationType, TXT("Animation type") );
	PROPERTY_EDIT( m_addConvertToAdditive, TXT( "Property is valid if animation type is additive" ) );
	PROPERTY_EDIT( m_addAdditiveType, TXT( "Property is valid if animation type is additive" ) );
#ifndef NO_EDITOR
	PROPERTY_EDIT_NOT_COOKED( m_recacheWeightCurve, TXT( "" ) );
#endif
	PROPERTY_EDIT( m_useWeightCurve, TXT( "" ) );
	PROPERTY_CUSTOM_EDIT( m_weightCurve, String::EMPTY, TXT("BaseCurveDataEditor") );
	PROPERTY_RO( m_weightCurveChanged, TXT( "" ) );
	PROPERTY_EDIT( m_supportsMotionExClipFront, TXT("True means that motion extraction will be supported for clip front option") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventAdditiveAnimation : public CStorySceneEventAnimation
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventAdditiveAnimation, CStorySceneEventAnimation )

protected:
	Bool			m_convertToAdditive;	
	EAdditiveType	m_additiveType;

public:
	CStorySceneEventAdditiveAnimation();
	CStorySceneEventAdditiveAnimation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& animName, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventAdditiveAnimation* Clone() const override;

protected:
	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventAdditiveAnimation )
	PARENT_CLASS( CStorySceneEventAnimation );
	PROPERTY_EDIT( m_convertToAdditive, TXT( "" ) );
	PROPERTY_EDIT( m_additiveType, TXT( "" ) );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CStorySceneEventOverrideAnimation : public CStorySceneEventAnimation
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventOverrideAnimation, CStorySceneEventAnimation )

	Bool m_fakeProp;

public:
	CStorySceneEventOverrideAnimation();
	CStorySceneEventOverrideAnimation( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& animName, const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventOverrideAnimation* Clone() const override;

protected:
	virtual void OnAddExtraDataToEvent( StorySceneEventsCollector::BodyAnimation& event ) const override;
};

BEGIN_CLASS_RTTI( CStorySceneEventOverrideAnimation )
	PARENT_CLASS( CStorySceneEventAnimation )
	PROPERTY_RO( m_fakeProp, String::EMPTY );
END_CLASS_RTTI();
