
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphAnimationManualSlot.h"

#include "../core/engineQsTransform.h"

//////////////////////////////////////////////////////////////////////////

struct SAnimationFullState
{
	DECLARE_RTTI_STRUCT( SAnimationFullState )

public:
	SAnimationState					m_state;

private:
	CSkeletalAnimationSetEntry*		m_animation;

public:
	Float							m_blendTimer;

	Float							m_weight;
	Bool							m_motion;
	Bool							m_fakeMotion;
	Bool							m_extractTrajectory;
	Bool							m_gatherEvents;
	Bool							m_gatherSyncTokens;
	Bool							m_muteSoundEvents;
	Bool							m_allowPoseCorrection;

	EAdditiveType					m_additiveType;
	Bool							m_convertToAdditive;

	TDynArray< Int32 >				m_bonesIdx;
	TDynArray< Float >				m_bonesWeight;

	CGUID							m_ID;

	Bool							m_fullEyesWeight; // Custom

	SAnimationFullState();
	SAnimationFullState(SAnimationFullState const & state);
	~SAnimationFullState();

	void Reset();

	void SetAnimation( CSkeletalAnimationSetEntry* animation );
	CSkeletalAnimationSetEntry* GetAnimation() const { return m_animation; }

	SAnimationFullState & operator = (SAnimationFullState const & state);
};

BEGIN_CLASS_RTTI( SAnimationFullState );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ESAnimationMappedPoseMode
{
	AMPM_Default,
	AMPM_Additive,
	AMPM_Override,
};

BEGIN_ENUM_RTTI( ESAnimationMappedPoseMode );
	ENUM_OPTION( AMPM_Default );
	ENUM_OPTION( AMPM_Additive );
	ENUM_OPTION( AMPM_Override );
END_ENUM_RTTI();

struct SAnimationMappedPose
{
	DECLARE_RTTI_STRUCT( SAnimationMappedPose )

	TEngineQsTransformArray			m_bones;
	TDynArray< Float >				m_tracks;
	TDynArray< Int32 >				m_bonesMapping;
	TDynArray< Int32 >				m_tracksMapping;
	Float							m_weight;
	ESAnimationMappedPoseMode		m_mode;
	CGUID							m_correctionID;
	CName							m_correctionIdleID;

	SAnimationMappedPose() : m_mode( AMPM_Default ), m_weight( 1.f ), m_correctionID( CGUID::ZERO ) {}
};

BEGIN_CLASS_RTTI( SAnimationMappedPose );
	PROPERTY( m_bones );
	PROPERTY( m_tracks );
	PROPERTY( m_bonesMapping );
	PROPERTY( m_tracksMapping );
	PROPERTY( m_weight );
	PROPERTY( m_mode );
	PROPERTY( m_correctionID );
	PROPERTY( m_correctionIdleID );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SAnimationMixerVisualState
{
	struct AnimState
	{
		CName	m_name;
		Float	m_weight;

		AnimState() : m_weight( 0.f ) {}

		AnimState( const SAnimationFullState& fullState )
		{
			m_name = fullState.m_state.m_animation;
			m_weight = fullState.m_weight;
		}

		Bool IsEqual( const AnimState& rhs ) const
		{
			return m_name == rhs.m_name && MAbs( m_weight - rhs.m_weight ) < 0.01f;
		}
	};

	TDynArray< AnimState >	m_animationsData;
	TDynArray< AnimState >	m_additiveData;
	TDynArray< AnimState >	m_overrideData;

	AnimState				m_idleA;
	AnimState				m_idleB;

	static Bool IsNotEqual( const SAnimationMixerVisualState& stateA, const SAnimationMixerVisualState& stateB );
};

//////////////////////////////////////////////////////////////////////////

class CAnimationMixerAnimSynchronizer
{
#ifdef RED_ASSERTS_ENABLED
	Bool	m_initialized;
#endif
	Bool	m_isSetterMode; // true = setter
	
	Float	m_prevTimeA;
	Float	m_currTimeA;
	Float	m_prevTimeB;
	Float	m_currTimeB;

public:
	CAnimationMixerAnimSynchronizer();

	void SetSetterMode();
	void SetGetterMode();

	Bool IsGetterMode() const;
	Bool IsSetterMode() const;

	void Set( Float prevTimeA, Float currTimeA, Float prevTimeB, Float currTimeB );
	void GetA( Float& prevTime, Float& currTime ) const;
	void GetB( Float& prevTime, Float& currTime ) const;
};

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationMixerSlotNode : public CBehaviorGraphBaseNode, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationMixerSlotNode, CBehaviorGraphBaseNode, "Slots.Animation", "Mixer slot" );

protected:
	Bool												m_bodyOrMimicMode;
	Bool												m_canUseIdles;
	EAdditiveType										m_postIdleAdditiveType;
	EAdditiveType										m_postAllAdditiveType;
	TDynArray< CName >									m_fullEyesWeightMimicsTracks;

#ifndef NO_EDITOR
	Bool												m_debugOverride;
#endif

protected:
	TInstanceVar< Float >								i_timeDelta;

	TInstanceVar< TDynArray< SAnimationFullState > >	i_animationsData;
	TInstanceVar< TDynArray< SAnimationFullState > >	i_additiveData;
	TInstanceVar< TDynArray< SAnimationFullState > >	i_overrideData;

	TInstanceVar< TDynArray< CName > >							i_cachedPosesNames;
	TInstanceVar< TDynArray< CAllocatedBehaviorGraphOutput > >	i_cachedPosesData;

	TInstanceVar< TDynArray< Uint32 > >					i_mappedPosesIds;
	TInstanceVar< TDynArray< SAnimationMappedPose > >	i_mappedPosesData;

	TInstanceVar< SAnimationFullState >					i_idleDataA;
	TInstanceVar< SAnimationFullState >					i_idleDataB;
	TInstanceVar< Float >								i_idleDataBlendWeight;

	TInstanceVar< Bool >								i_gatheredSyncTokens;
	TInstanceVar< TDynArray< CAnimationSyncToken* > >	i_syncTokens;

	TInstanceVar< TDynArray< Int32 > >					i_fullEyesWeightMimicsTracks;

protected:
	CBehaviorGraphNode*									m_cachedPostIdleNodeA;
	CBehaviorGraphNode*									m_cachedPostIdleNodeB;
	CBehaviorGraphNode*									m_cachedPostAllNodeA;
	CBehaviorGraphNode*									m_cachedPostAllNodeB;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
	virtual void OnRebuildSockets();
#endif

public:
	CBehaviorGraphAnimationMixerSlotNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

public:
	void OpenMixer( CBehaviorGraphInstance& instance ) const;
	void AddAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const;
	void AddAdditiveAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const;
	void AddOverrideAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const;
	Bool RemoveAnimation( CBehaviorGraphInstance& instance, const CGUID& id ) const;
	void SetIdleAnimationToSample( CBehaviorGraphInstance& instance, const SAnimationFullState& animationA, const SAnimationFullState& animationB, Float blendWeight, Bool canRandAnimStartTime, CAnimationMixerAnimSynchronizer* synchronizer ) const;
	void ResetIdleAnimation( CBehaviorGraphInstance& instance ) const;
	void AddPoseToSample( CBehaviorGraphInstance& instance, Uint32 poseId, const SAnimationMappedPose& pose ) const;
	void RemovePose( CBehaviorGraphInstance& instance, Uint32 poseId ) const;
	void RemoveAllPoses( CBehaviorGraphInstance& instance ) const;
	void CloseMixer( CBehaviorGraphInstance& instance ) const;
	void RemoveAllAnimations( CBehaviorGraphInstance& instance ) const;

	Bool ResamplePose( const CBehaviorGraphInstance& instance, SBehaviorGraphOutput& temp, Int32 boneIdx, Matrix& outBoneMS ) const;
	void GetState( const CBehaviorGraphInstance& instance, SAnimationMixerVisualState& state ) const;

#ifndef NO_EDITOR
	const SAnimationFullState& GetIdleA( CBehaviorGraphInstance& instance ) const;
	const SAnimationFullState& GetIdleB( CBehaviorGraphInstance& instance ) const;
	const SAnimationFullState* GetAnimationState( CBehaviorGraphInstance& instance, CName animationName ) const;
#endif

private:
	CSkeletalAnimationSetEntry* FindAnimation( CBehaviorGraphInstance& instance, const CName& animation ) const;

	void SampleAnimation( SAnimationFullState& animation, SBehaviorGraphOutput* pose, SBehaviorSampleContext& context, const CAnimatedComponent* ac, Float timeDelta ) const;
	void SampleAnimationWithCorrection( CBehaviorGraphInstance& instance, SAnimationFullState& animation, SBehaviorGraphOutput* pose, SBehaviorSampleContext& context, const CAnimatedComponent* ac, Float timeDelta ) const;

	Bool ShouldSamplePostIdleA() const;
	Bool ShouldSamplePostIdleB() const;
	void SamplePostIdleA( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SamplePostIdleB( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	Bool ShouldSamplePostAllA() const;
	Bool ShouldSamplePostAllB() const;
	void SamplePostAllA( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SamplePostAllB( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	Bool IsPoseForIdle( CBehaviorGraphInstance& instance, const SAnimationMappedPose& pose, Float& idleWeight ) const;
	void BlendAdditiveMappedPose( SBehaviorGraphOutput &output, const SAnimationMappedPose& pose, Float parentWeight ) const;
	void BlendOverrideMappedPose( SBehaviorGraphOutput &output, const SAnimationMappedPose& pose, Float parentWeight ) const;

	void AddAnimation( CBehaviorGraphInstance& instance, const TInstanceVar< TDynArray< SAnimationFullState > >& arr, const SAnimationFullState& animation ) const;
	
	Bool HasAnimationWithID( CBehaviorGraphInstance& instance, const TInstanceVar< TDynArray< SAnimationFullState > >& arr, const CGUID& id ) const;
	Bool HasAnyAnimationWithID( CBehaviorGraphInstance& instance, const CGUID& id ) const;
	Bool HasAnyIdleWithID( CBehaviorGraphInstance& instance, const CName& idleId ) const;

	Bool HasAnyCachedPose( CBehaviorGraphInstance& instance ) const;
	Int32 FindCachedPose( CBehaviorGraphInstance& instance, const CName& animation ) const;
	Int32 AddCachedPose( CBehaviorGraphInstance& instance, const SAnimationFullState& animation ) const;
	SBehaviorGraphOutput* GetCachedPose( CBehaviorGraphInstance& instance, Int32 index ) const;

	Int32 FindMappedPoseByCorrection( CBehaviorGraphInstance& instance, CGUID correctionID ) const;
	Int32 FindMappedPose( CBehaviorGraphInstance& instance, Uint32 poseId ) const;
	Int32 AddMappedPose( CBehaviorGraphInstance& instance, Uint32 poseId, const SAnimationMappedPose& pose ) const;
	SAnimationMappedPose* GetMappedPose( CBehaviorGraphInstance& instance, Int32 index ) const;

	void RemoveAllNotCorrectionPoses( CBehaviorGraphInstance& instance ) const;
	void RemoveAllUnusedCorrectionPoses( CBehaviorGraphInstance& instance ) const;

	void FilterAdditiveAnimation( const SAnimationFullState& animation, SBehaviorGraphOutput& pose ) const;

	void UpdateIdleAnimation( SAnimationFullState& animationState, Float timeDelta ) const;

	void RandAnimTime( SAnimationFullState& animation ) const;

	Bool IsBodyMode() const;
	Bool IsMimicsMode() const;

	Bool HasSomethingToSampled( CBehaviorGraphInstance& instance ) const;

private:
	void GatherSyncTokens( CBehaviorGraphInstance& instance ) const;
	void ClearSyncTokens( CBehaviorGraphInstance& instance ) const;
	void SyncTokens( CBehaviorGraphInstance& instance, const SAnimationFullState& animData, Float accWeight ) const;

	void MuteSoundEvents( SBehaviorGraphOutput& pose) const;
public:
	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationMixerSlotNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_bodyOrMimicMode, TXT("true = body, false = mimic") );
	PROPERTY_EDIT( m_canUseIdles, TXT("true = body, false = mimic") );
	PROPERTY_EDIT( m_postIdleAdditiveType, TXT("") );
	PROPERTY_EDIT( m_postAllAdditiveType, TXT("") );
	PROPERTY_EDIT( m_fullEyesWeightMimicsTracks, TXT("") );
	PROPERTY( m_cachedPostIdleNodeA );
	PROPERTY( m_cachedPostIdleNodeB );
	PROPERTY( m_cachedPostAllNodeA );
	PROPERTY( m_cachedPostAllNodeB );
#ifndef NO_EDITOR
	PROPERTY_EDIT( m_debugOverride, TXT("") );
#endif
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorMixerSlotInterface
{
	CBehaviorGraphAnimationMixerSlotNode*	m_slot;
	CBehaviorGraphInstance*					m_instance;
	THandle< CBehaviorGraphInstance >		m_instanceH;

public:
	CBehaviorMixerSlotInterface();
	~CBehaviorMixerSlotInterface();

	void Init( CBehaviorGraphAnimationMixerSlotNode* slot, CBehaviorGraphInstance* instance );
	void Clear();

	Bool IsValid() const;
	CName GetInstanceName() const;

	void OpenMixer();
	void AddAnimationToSample( const SAnimationFullState& animation );
	void AddAdditiveAnimationToSample( const SAnimationFullState& animation );
	void AddOverrideAnimationToSample( const SAnimationFullState& animation );
	Bool RemoveAnimation( const CGUID& id );
	void SetIdleAnimationToSample( const SAnimationFullState& animationA, const SAnimationFullState& animationB, Float blendWeight, Bool canRandAnimStartTime = false, CAnimationMixerAnimSynchronizer* synchronizer = nullptr );
	void ResetIdleAnimation();
	void AddPoseToSample( Uint32 poseId, const SAnimationMappedPose& pose );
	void RemovePose( Uint32 poseId );
	void RemoveAllPoses();
	void CloseMixer();
	void RemoveAllAnimations();

	Bool ResamplePose( SBehaviorGraphOutput& temp, Int32 boneIdx, Matrix& outBoneMS ) const;
	void GetState( SAnimationMixerVisualState& state ) const;

#ifndef NO_EDITOR
	const SAnimationFullState* GetIdleA() const;
	const SAnimationFullState* GetIdleB() const;
	const SAnimationFullState* GetAnimationState( CName animationName ) const;
#endif
};
