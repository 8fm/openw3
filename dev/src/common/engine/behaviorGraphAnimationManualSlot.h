
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

struct SRuntimeAnimationData
{
	DECLARE_RTTI_STRUCT( SRuntimeAnimationData )

public:
	SAnimationState					m_state;
	Float							m_blendTimer;
	Uint32							m_blendBonesCount;

private:
	CSkeletalAnimationSetEntry*		m_animation;

public:
	SRuntimeAnimationData();

	void Reset( Bool isActive );

	void AutoUpdateTime( Float timeDelta, Bool looped = false );

	void SetAnimation( CSkeletalAnimationSetEntry* animation, Bool isActive );
	CSkeletalAnimationSetEntry* GetAnimation() const { return m_animation; }
};

BEGIN_CLASS_RTTI( SRuntimeAnimationData );
END_CLASS_RTTI();

class CBehaviorGraphAnimationManualSlotNode : public CBehaviorGraphNode, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationManualSlotNode, CBehaviorGraphNode, "Slots.Animation", "Manual slot" );

	enum EInternalMotionType
	{
		IMT_Anim,
		IMT_Add,
		IMT_Set,
		IMT_Blend,
	};

protected:
	CName										m_defaultAnimation;

protected:
	TInstanceVar< Float >						i_timeDelta;

	TInstanceVar< SRuntimeAnimationData >		i_animationAData;
	TInstanceVar< SRuntimeAnimationData >		i_animationBData;
	TInstanceVar< SRuntimeAnimationData >		i_animationDefaultData;

	TInstanceVar< Float >						i_weight;
	TInstanceVar< Float >						i_nodeWeight;
	
	TInstanceVar< Int32 >						i_motion;
	TInstanceVar< Vector >						i_motionTrans;
	TInstanceVar< Vector >						i_motionQuat;

	TInstanceVar< Float >						i_blendOutTime; // if negative - blend out is inactive
	TInstanceVar< Bool >						i_autoUpdateWhenBlendingOut; // update automatically when blending out

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
	virtual void OnRebuildSockets();

	CName GetAnimationAName( CBehaviorGraphInstance& instance ) const;
	CName GetAnimationBName( CBehaviorGraphInstance& instance ) const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const;
#endif

public:
	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

public:
	Bool PlayAnimation( CBehaviorGraphInstance& instance, const SAnimationState& animation, Float weight ) const;
	Bool PlayAnimations( CBehaviorGraphInstance& instance, const SAnimationState& animationA, const SAnimationState& animationB, Float blendWeight, Float weight ) const;
	void Stop( CBehaviorGraphInstance& instance ) const;
	void BlendOut( CBehaviorGraphInstance& instance, Float blendOutTime, Bool continuePlaying ) const;
#ifdef USE_HAVOK_ANIMATION
	void AddMotion( CBehaviorGraphInstance& instance, const hkQsTransform& motion ) const;
	void SetMotion( CBehaviorGraphInstance& instance, const hkQsTransform& motion ) const;
	void BlendMotion( CBehaviorGraphInstance& instance, const hkQsTransform& motion ) const;
#else
	void AddMotion( CBehaviorGraphInstance& instance, const RedQsTransform& motion ) const;
	void SetMotion( CBehaviorGraphInstance& instance, const RedQsTransform& motion ) const;
	void BlendMotion( CBehaviorGraphInstance& instance, const RedQsTransform& motion ) const;
#endif
	void ResetMotion( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void OnUpdateWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorUpdateContext &context, Float timeDelta ) const;
	virtual void SampleWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorSampleContext& context, SBehaviorGraphOutput & output ) const;
	virtual void PerformPoseCorrection( SBehaviorSampleContext& context, SBehaviorGraphOutput& output ) const;
	Bool SamplePoseFromSlot( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

private:
	void SampleAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const CSkeletalAnimationSetEntry* animation, SRuntimeAnimationData& data ) const;

	Bool SetAnimationState( CBehaviorGraphInstance& instance, SRuntimeAnimationData& animationData, const SAnimationState& inputData ) const;

	CSkeletalAnimationSetEntry* FindAnimation( CBehaviorGraphInstance& instance, const CName& animation ) const;

	void ApplyMotion( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationManualSlotNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_CUSTOM_EDIT( m_defaultAnimation, TXT("Animation name"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationManualWithInputSlotNode : public CBehaviorGraphAnimationManualSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationManualWithInputSlotNode, CBehaviorGraphAnimationManualSlotNode, "Slots.Animation", "Manual with input" );

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void OnRebuildSockets();
#endif

public:
	virtual void CacheConnections();

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void OnUpdateWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorUpdateContext &context, Float timeDelta ) const;
	virtual void SampleWhenNoAnimation( CBehaviorGraphInstance& instance, SBehaviorSampleContext& context, SBehaviorGraphOutput & output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationManualWithInputSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationManualSlotNode );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorManualSlotInterface
{
	CBehaviorGraphAnimationManualSlotNode*	m_slot;
	CBehaviorGraphInstance*					m_instance;

public:
	CBehaviorManualSlotInterface();
	~CBehaviorManualSlotInterface();

	void Init( CBehaviorGraphAnimationManualSlotNode* slot, CBehaviorGraphInstance* instance );
	void Clear();

	Bool IsValid() const;
	Bool IsActive() const;
	CName GetInstanceName() const;
	CBehaviorGraphInstance * GetInstance() const { return m_instance; }

	Bool PlayAnimation( const SAnimationState& animation, Float weight = 1.f );
	Bool PlayAnimations( const SAnimationState& animationA, const SAnimationState& animationB, Float blendWeight, Float weight = 1.f );
	void Stop();
	void BlendOut( Float blendOutTime, Bool continuePlaying );
#ifdef USE_HAVOK_ANIMATION
	void SetMotion( const hkQsTransform& motion );
	void BlendMotion( const hkQsTransform& motion );
	void AddMotion( const hkQsTransform& motion );
#else
	void SetMotion( const RedQsTransform& motion );
	void BlendMotion( const RedQsTransform& motion );
	void AddMotion( const RedQsTransform& motion );
#endif
	void ResetMotion();
};
