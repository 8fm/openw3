
#pragma once

#include "storySceneIncludes.h"

class CSceneAnimationsResourcesManager
{
public:
	CSceneAnimationsResourcesManager();

	//! SceneAnimationsBody
	RED_INLINE Bool LoadSceneAnimationsBody2dArray( const String& filePath ) { return m_resSceneAnimationsBody.Load2dArray( filePath ); }
	RED_INLINE Bool UnloadSceneAnimationsBody2dArray( const String& filePath ) { return m_resSceneAnimationsBody.Unload2dArray( filePath ); }
	
	RED_INLINE const C2dArray& GetSceneAnimationsBody2dArray() const { return m_resSceneAnimationsBody.Get2dArray(); }
	RED_INLINE const C2dArray& ReloadSceneAnimationsBody2dArray() { return m_resSceneAnimationsBody.Reload2dArray(); }
	RED_INLINE void GetFileNameAndOffsetSceneAnimationsBody( Uint32 globalIndex, String& fileName, Uint32& localIndex ) { 
#ifdef NO_EDITOR
		fileName = TXT("scene_body_animations.csv");
		localIndex = globalIndex;
#else
		m_resSceneAnimationsBody.GetFileNameAndOffset( globalIndex, fileName, localIndex );
#endif //! NO_EDITOR
	}

	//! SceneAnimationsMimics
	RED_INLINE Bool LoadSceneAnimationsMimics2dArray( const String& filePath ) { return m_resSceneAnimationsMimics.Load2dArray( filePath ); }
	RED_INLINE Bool UnloadSceneAnimationsMimics2dArray( const String& filePath ) { return m_resSceneAnimationsMimics.Unload2dArray( filePath ); }

	RED_INLINE const C2dArray& GetSceneAnimationsMimics2dArray() const { return m_resSceneAnimationsMimics.Get2dArray(); }
	RED_INLINE const C2dArray& ReloadSceneAnimationsMimics2dArray() { return m_resSceneAnimationsMimics.Reload2dArray(); }
	RED_INLINE void GetFileNameAndOffsetSceneAnimationsMimics( Uint32 globalIndex, String& fileName, Uint32& localIndex ) { 
#ifdef NO_EDITOR
		fileName = TXT("scene_mimics_animations.csv");
		localIndex = globalIndex;
#else
		m_resSceneAnimationsMimics.GetFileNameAndOffset( globalIndex, fileName, localIndex );
#endif //! NO_EDITOR
	}

	//! SceneAnimationsMimicsEmoStates
	RED_INLINE Bool LoadSceneAnimationsMimicsEmoStates2dArray( const String& filePath ) { return m_resSceneAnimationsMimicsEmoStates.Load2dArray( filePath ); }
	RED_INLINE Bool UnloadSceneAnimationsMimicsEmoStates2dArray( const String& filePath ) { return m_resSceneAnimationsMimicsEmoStates.Unload2dArray( filePath ); }

	RED_INLINE const C2dArray& GetSceneAnimationsMimicsEmoStates2dArray() const { return m_resSceneAnimationsMimicsEmoStates.Get2dArray(); }
	RED_INLINE const C2dArray& ReloadSceneAnimationsMimicsEmoStates2dArray() { return m_resSceneAnimationsMimicsEmoStates.Reload2dArray(); }
	RED_INLINE void GetFileNameAndOffsetSceneAnimationsMimicsEmoStates( Uint32 globalIndex, String& fileName, Uint32& localIndex ) {
#ifdef NO_EDITOR
		fileName = TXT("scene_mimics_emotional_states.csv");
		localIndex = globalIndex;
#else
		m_resSceneAnimationsMimicsEmoStates.GetFileNameAndOffset( globalIndex, fileName, localIndex );
#endif //! NO_EDITOR
	}


private:
	C2dArraysResourcesManager m_resSceneAnimationsBody;
	C2dArraysResourcesManager m_resSceneAnimationsMimics;
	C2dArraysResourcesManager m_resSceneAnimationsMimicsEmoStates;
};

typedef TSingleton< CSceneAnimationsResourcesManager, TDefaultLifetime, TCreateUsingNew > SSceneAnimationsResourcesManager;

class CStorySceneAnimationList
{
public:
	struct AnimationBodyData
	{
		CName							m_animationName;
		String							m_friendlyName;
		SStorySceneActorAnimationState* m_transitionTo;
		CName							m_lookAtBodyAnimationName;
		CName							m_lookAtHeadAnimationName;

#ifndef NO_EDITOR
		SStorySceneEventGeneratorAnimationState m_generatorData;
#endif

		AnimationBodyData() : m_transitionTo( nullptr ) {}
	};

	struct AnimationMimicsData
	{
		CName							m_animationName;
		String							m_friendlyName;
		CName							m_friendlyNameAsName;
		CName							m_typeName;
		CName							m_layerName;

#ifndef NO_EDITOR
		SStorySceneEventGeneratorAnimationState m_generatorData;
#endif

		AnimationMimicsData() {}
	};

	struct IdleAndLookAtAnimationData
	{
		CName	m_idle;
		CName	m_lookAtBody;
		CName	m_lookAtHead;
	};

	struct MimicsEmoStatePreset
	{
		CName	m_emoState;
		CName	m_layerEyes;
		CName	m_layerPose;
		CName	m_layerAnimation;
	};

	static const Uint32 LEVEL_BODY_STATUS = 0;
	static const Uint32 LEVEL_BODY_EMOTIONAL_STATE = 1;
	static const Uint32 LEVEL_BODY_POSE = 2;
	static const Uint32 LEVEL_BODY_TYPE = 3;
	static const Uint32 LEVEL_BODY_ANIMATIONS = 4;
	static const Uint32 LEVEL_BODY_TRANSITION = 5;

	static const Uint32 LEVEL_MIMICS_ACTION_TYPE = 0;
	static const Uint32 LEVEL_MIMICS_ANIMATIONS = 1;
	static const Uint32 LEVEL_MIMICS_EMOTIONAL_STATE = 2;
	static const Uint32 LEVEL_MIMICS_LAYER_EYES = 3;
	static const Uint32 LEVEL_MIMICS_LAYER_POSE = 4;
	static const Uint32 LEVEL_MIMICS_LAYER_ANIMATION = 5;

	static const CName&	ANIMATION_TYPE_IDLE;
	static const CName&	ANIMATION_TYPE_TRANSITION;
	
	static const CName&	IDLE_KEYWORD;
	static const CName&	GESTURE_KEYWORD;
	static const CName&	ADDITIVE_KEYWORD;
	
	static const CName&	EXIT_KEYWORD;
	static const CName&	ENTER_KEYWORD;
	
	static const CName&	LAYER_EYES;
	static const CName&	LAYER_POSE;
	static const CName&	LAYER_ANIMATION;
	
	static const CName&	DEFAULT_STATUS;
	static const CName&	DEFAULT_EMO_STATE;
	static const CName&	DEFAULT_POSE;
	static const CName&	DEFAULT_MIMICS_EMO_STATE;
	static const CName&	DEFAULT_MIMICS_LAYER_EYES;
	static const CName&	DEFAULT_MIMICS_LAYER_POSE;
	static const CName&	DEFAULT_MIMICS_LAYER_ANIMATION;

	typedef THashMap< SStorySceneActorAnimationState, TPair< SStorySceneActorAnimationState, TDynArray< AnimationBodyData >> > TAdditionalAdditivesMap;

private:
	typedef CName	TAnimationType;
	typedef CName	TEmotionalState;
	typedef CName	TPose;
	typedef CName	TStatus;
	typedef CName	TActionType;

	typedef TDynArray< AnimationBodyData >								TAnimationBodyLevel;
	typedef TDynArray< AnimationMimicsData >							TAnimationMimicsLevel;
	
	typedef TDynArray< TPair< TAnimationType, TAnimationBodyLevel > >	TBodyTypeLevel;
	typedef TDynArray< TPair< TPose, TBodyTypeLevel > >					TBodyPoseLevel;
	typedef TDynArray< TPair< TEmotionalState, TBodyPoseLevel > >		TBodyEmotionalStateLevel;
	typedef TDynArray< TPair< TStatus, TBodyEmotionalStateLevel > >		TBodyStatusLevel;

	typedef TDynArray< TPair< TActionType, TAnimationMimicsLevel > >	TMimicsEmotionalStateLevel;

	typedef TBodyStatusLevel											TBodyAnimations;
	typedef TMimicsEmotionalStateLevel									TMimicsAnimations;

	typedef TPair< Int32, Int32 >										TMimicIdleMappingRecord;
	typedef TDynArray< TMimicIdleMappingRecord >						TMimicsIdleMapping;
	typedef TDynArray< MimicsEmoStatePreset >							TMimicsEmoStatePresets;

	TBodyAnimations			m_dataBody;
	TMimicsAnimations		m_dataMimics;
	TMimicsIdleMapping		m_dataMimicsIdleMapping;
	TMimicsEmoStatePresets	m_dataMimicsEmoStatePresets;

	TAdditionalAdditivesMap	m_additionalAddittiveAnimMan;
	TAdditionalAdditivesMap	m_additionalAddittiveAnimWoman;
	
public:
	~CStorySceneAnimationList();

	Bool Load();

	void Destroy();

	Bool IsValid() const;

	Bool FindStateByAnimation( const CName& animationName, SStorySceneActorAnimationState& out, CName* outAnimType = nullptr ) const;
	
	Bool RandBodyIdleAnimation( const SStorySceneActorAnimationState& in, IdleAndLookAtAnimationData& dataOut, const CAnimatedComponent* filter = nullptr ) const;
	Bool RandMimicsIdleAnimation( const SStorySceneActorAnimationState& in, CName& animationNameOut, const CAnimatedComponent* filter, const CName& layerName ) const;

	Bool FindBodyTransitions( const SStorySceneActorAnimationState& from, const SStorySceneActorAnimationState& to, TDynArray< CName >& animationNamesOut ) const;
	Bool FindMimicsTransitions( const SStorySceneActorAnimationState& from, const SStorySceneActorAnimationState& to, TDynArray< CName >& animationNamesOut ) const;

	const MimicsEmoStatePreset* FindMimicsAnimationByEmoState( const CName& emoState ) const;

	void CollectBodyIdleAnimations( const SStorySceneActorAnimationState& in, TDynArray< CName >& outAnimations, const CAnimatedComponent* filter = nullptr ) const;

#ifndef NO_EDITOR
public:
	void ShowErrors( IFeedbackSystem* f ) const;

	CName FindBodyAnimationByFriendlyName( const CName& status, const CName& emoState, const CName& pose, const CName& type, const String& friendlyName ) const;
	CName FindMimicsAnimationByFriendlyName( const CName& friendlyName ) const;

	const TAdditionalAdditivesMap& GetManAdditionalAdditives() const { return m_additionalAddittiveAnimMan; };
	const TAdditionalAdditivesMap& GetWomanAdditionalAdditives() const { return m_additionalAddittiveAnimWoman; };
#endif

private:
	// Parsing
	struct SParseParams
	{
		TBodyAnimations*		m_dataBody;
		TMimicsAnimations*		m_dataMimics;
		TMimicsIdleMapping*		m_dataMimicsIdleMapping;
		TMimicsEmoStatePresets*	m_dataMimicsEmoStatePresets;

		IFeedbackSystem*		m_feedback;

#ifndef NO_EDITOR
		TAdditionalAdditivesMap*	m_additionalAddittiveAnimMan;
		TAdditionalAdditivesMap*	m_additionalAddittiveAnimWoman;
#endif
	};

	Bool Parse( SParseParams & pp ) const;

	Int32 FindTypeLevel( const TBodyAnimations& data, Int32 s, Int32 e, Int32 p, const TAnimationType& type ) const;
	Int32 FindOrAddTypeLevel( TBodyAnimations& data, Int32 s, Int32 e, Int32 p, const TAnimationType& type ) const;

	static void ShowErrorInSceneAnimationsBody( IFeedbackSystem* f, const Uint32& index, const String& msg );
	static void ShowErrorInSceneAnimationsMimics( IFeedbackSystem* f, const Uint32& index, const String& msg );
	static void ShowErrorInSceneAnimationsMimicsEmoStates( IFeedbackSystem* f, const Uint32& index, const String& msg );

public:
	//////////////////////////////////////////////////////////////////////////
	// Iterators

	class StatusBodyIterator
	{
	protected:
		const CStorySceneAnimationList&		m_list;
		Uint32								m_index;

	public:
		StatusBodyIterator( const CStorySceneAnimationList& list );

		operator Bool () const;
		void operator++ ();
		const CName& operator*() const;

		Int32 GetCurrentIndex() const;

		const CStorySceneAnimationList&	GetList() const { return m_list; }

	private:
		const StatusBodyIterator& operator=( const StatusBodyIterator& );
	};

	class EmotionalStatesBodyIterator : public StatusBodyIterator
	{
	protected:
		Int32		m_statusIdx;

	public:
		EmotionalStatesBodyIterator( const CStorySceneAnimationList& list, const CName& status );
		EmotionalStatesBodyIterator( const CStorySceneAnimationList& list, Int32 statusIdx = 0 );

		void Restart( const StatusBodyIterator& it );

		operator Bool () const;
		void operator++ ();
		const CName& operator*() const;

		Int32 GetStatusIndex() const;
	};

	class PoseBodyIterator : public EmotionalStatesBodyIterator
	{
	protected:
		Int32		m_emotionalStateIdx;

	public:
		PoseBodyIterator( const CStorySceneAnimationList& list, const CName& status, const CName& emotionalState );
		PoseBodyIterator( const CStorySceneAnimationList& list, Int32 statusIdx = 0, Int32 emotionalStateIdx = 0 );

		void Restart( const EmotionalStatesBodyIterator& it );

		operator Bool () const;
		void operator++ ();
		const CName& operator*() const;

		Int32 GetEmotionalStateIndex() const;
	};

	class TypeBodyIterator : public PoseBodyIterator
	{
	protected:
		Int32		m_poseIdx;

	public:
		TypeBodyIterator( const CStorySceneAnimationList& list, const CName& status, const CName& emotionalState, const CName& pose );
		TypeBodyIterator( const CStorySceneAnimationList& list, Int32 statusIdx = 0, Int32 emotionalStateIdx = 0, Int32 poseIdx = 0 );

		void Restart( const PoseBodyIterator& it );

		operator Bool () const;
		void operator++ ();
		const CName& operator*() const;

		Int32 GetPoseIndex() const;
	};

	class BodyAnimationIterator : public TypeBodyIterator
	{
	protected:
		Int32		m_typeIdx;

	public:
		BodyAnimationIterator( const CStorySceneAnimationList& list, const CName& status, const CName& emotionalState, const CName& pose, const CName& type );
		BodyAnimationIterator( const CStorySceneAnimationList& list, Int32 statusIdx = 0, Int32 emotionalStateIdx = 0, Int32 poseIdx = 0, Int32 typeIdx = 0 );

		void Restart( const TypeBodyIterator& it );

		operator Bool () const;
		void operator++ ();
		const AnimationBodyData& operator*() const;

		Int32 GetTypeIndex() const;

	private:
		const BodyAnimationIterator& operator=( const BodyAnimationIterator& );
	};

	class AllBodyAnimationsIterator
	{
	protected:	
		StatusBodyIterator				m_statusIt;
		EmotionalStatesBodyIterator		m_emoIt;
		PoseBodyIterator				m_poseIt;
		TypeBodyIterator				m_typeIt;
		BodyAnimationIterator			m_animIt;

	public:
		AllBodyAnimationsIterator( const CStorySceneAnimationList& list );

		operator Bool () const;
		void operator++ ();
		const AnimationBodyData& operator*() const;

		const CName& GetStatus() const;
		const CName& GetEmoState() const;
		const CName& GetPose() const;
		const CName& GetTypeName() const;

	private:
		const AllBodyAnimationsIterator& operator=( const AllBodyAnimationsIterator& );
	};

	//////////////////////////////////////////////////////////////////////////

	class IdleAnimationMimicsIterator
	{
	protected:
		const CStorySceneAnimationList&		m_list;
		Uint32								m_index;

	public:
		IdleAnimationMimicsIterator( const CStorySceneAnimationList& list );

		operator Bool () const;
		void operator++ ();
		const AnimationMimicsData& operator*() const;

		const CStorySceneAnimationList&	GetList() const { return m_list; }

	private:
		const IdleAnimationMimicsIterator& operator=( const IdleAnimationMimicsIterator& );
	};

	class EmotionalStateMimicsIterator
	{
	protected:
		const CStorySceneAnimationList&		m_list;
		Uint32								m_index;

	public:
		EmotionalStateMimicsIterator( const CStorySceneAnimationList& list );

		operator Bool () const;
		void operator++ ();
		const MimicsEmoStatePreset& operator*() const;

		const CStorySceneAnimationList&	GetList() const { return m_list; }

	private:
		const EmotionalStateMimicsIterator& operator=( const EmotionalStateMimicsIterator& );
	};

	class LayerMimicsAnimationIterator
	{
	protected:
		const CStorySceneAnimationList&		m_list;
		Int32								m_index;
		CName								m_layerName;

	public:
		LayerMimicsAnimationIterator( const CStorySceneAnimationList& list, const CName& layerName );

		operator Bool () const;
		void operator++ ();
		const AnimationMimicsData& operator*() const;

	private:
		const LayerMimicsAnimationIterator& operator=( const LayerMimicsAnimationIterator& );

		void Next();
	};

	class ActionTypeMimicsIterator
	{
	protected:
		const CStorySceneAnimationList&		m_list;
		Uint32								m_index;

	public:
		ActionTypeMimicsIterator( const CStorySceneAnimationList& list );

		operator Bool () const;
		void operator++ ();
		const CName& operator*() const;

		Int32 GetCurrentIndex() const;

		const CStorySceneAnimationList&	GetList() const { return m_list; }

	private:
		const ActionTypeMimicsIterator& operator=( const ActionTypeMimicsIterator& );
	};

	class MimicsAnimationIteratorByEmoState
	{
	protected:
		Int32								m_index;
		CName								m_emoState;
		const CStorySceneAnimationList&		m_list;

	public:
		MimicsAnimationIteratorByEmoState( const CStorySceneAnimationList& list, const CName& mimicEmoState );

		operator Bool () const;
		void operator++ ();
		const AnimationMimicsData& operator*() const;

	private:
		const MimicsAnimationIteratorByEmoState& operator=( const MimicsAnimationIteratorByEmoState& );

		void Next();
	};

	class MimicsAnimationIteratorByAction : public ActionTypeMimicsIterator
	{
	protected:
		Int32		m_actionIdx;

	public:
		MimicsAnimationIteratorByAction( const CStorySceneAnimationList& list, const CName& actionType );
		MimicsAnimationIteratorByAction( const CStorySceneAnimationList& list, Int32 actionIdx = 0 );

		void Restart( const ActionTypeMimicsIterator& it );

		operator Bool () const;
		void operator++ ();
		const AnimationMimicsData& operator*() const;

		Int32 GetActionTypeIndex() const;

	private:
		const MimicsAnimationIteratorByAction& operator=( const MimicsAnimationIteratorByAction& );
	};

	class AllMimicsAnimationsIterator
	{
	protected:	
		ActionTypeMimicsIterator			m_actionIt;
		MimicsAnimationIteratorByAction		m_animIt;

	public:
		AllMimicsAnimationsIterator( const CStorySceneAnimationList& list );

		operator Bool () const;
		void operator++ ();
		const AnimationMimicsData& operator*() const;

		const CName& GetActionType() const;

	private:
		const AllMimicsAnimationsIterator& operator=( const AllMimicsAnimationsIterator& );
	};
};
