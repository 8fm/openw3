/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "allocatedBehaviorGraphOutput.h"
#include "behaviorGraphAnimationSlotNode.h"

class CBehaviorGraphAnimationSlotNode : public CBehaviorGraphAnimationBaseSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationSlotNode, CBehaviorGraphAnimationBaseSlotNode, "Slots.Animation", "Blend slot" );

	enum ESlotAction
	{
		SA_None,
		SA_BlendOut,
		SA_Finish,
	};

protected:
	CName					m_stopEvtName;
	CName					m_startEvtName;

protected:
	TInstanceVar< Bool >	i_slotActive;
	TInstanceVar< Int32 >	i_slotAction;

	TInstanceVar< Bool >	i_hasCachedPose;

	TInstanceVar< Bool >	i_blendIn;
	TInstanceVar< Bool >	i_blendOut;

	TInstanceVar< Float >	i_blendInDuration;
	TInstanceVar< Float >	i_blendOutDuration;
	TInstanceVar< Float >	i_finishBlendingDurationLeft;	//!< Stores how much time left for blending (needed for calculating the weight)
	TInstanceVar< Int32 >	i_blendInType;
	TInstanceVar< Int32 >	i_blendOutType;
	TInstanceVar< Int32 >	i_mergeBlendedSlotEvents;

	TInstanceVar< Int32 >	i_firstLoop;

	TInstanceVar< Bool >	i_isInTick;

	TInstanceVar< CAllocatedBehaviorGraphOutput > i_pose;

protected:
	CBehaviorGraphNode*		m_cachedBaseInputNode;

public:
	CBehaviorGraphAnimationSlotNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

public:
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime = 0.0f ) const override;

	virtual Bool IsSlotActive( const CBehaviorGraphInstance& instace ) const;

public:
	Bool IsBlendIn( CBehaviorGraphInstance& instance ) const;
	Bool IsBlendOut( CBehaviorGraphInstance& instance ) const;

	Float GetBlendTimer( CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void CollectAnimationUsageData( const CBehaviorGraphInstance& instance, TDynArray<SBehaviorUsedAnimationData>& collectorArray ) const override;
#endif
	
protected:
	void SetSlotActive( CBehaviorGraphInstance& instance ) const;
	void SetSlotInactive( CBehaviorGraphInstance& instance ) const;

	void UpdateSlotLogic( CBehaviorGraphInstance& instance, Float timeDelta ) const;

	void UpdateSlotAnimation( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void UpdateInputAnimation( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	void StartAnimation( CBehaviorGraphInstance& instance ) const;
	virtual void SetupSlot( CBehaviorGraphInstance& instance, const SBehaviorSlotSetup* setup ) const;
	virtual void SampleSlotAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void SampleInputAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	void BlendPoses( CBehaviorGraphInstance& instance, SBehaviorSampleContext& context, SBehaviorGraphOutput &output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float weight ) const;

	virtual Bool HasCachedPose( CBehaviorGraphInstance& instance ) const;
	virtual void CachePose( CBehaviorGraphInstance& instance ) const;
	virtual void CreatePose( CBehaviorGraphInstance& instance ) const;
	virtual void DestroyPose( CBehaviorGraphInstance& instance ) const;

public:
	Bool IsBlendInStarted( CBehaviorGraphInstance& instance ) const;
	Bool IsBlendOutStarted( CBehaviorGraphInstance& instance ) const;
	Bool IsBlending( CBehaviorGraphInstance& instance ) const;
	Bool IsBlendingWithInput( CBehaviorGraphInstance& instance ) const;
	Bool IsBlendingOnFinish( CBehaviorGraphInstance& instance ) const;
	Bool HasBlendOut( CBehaviorGraphInstance& instance ) const;
	Bool HasBlendIn( CBehaviorGraphInstance& instance ) const;
	Bool IsAnimationSlotFinished( CBehaviorGraphInstance& instance ) const;

private:
	void StopAndDeactivate( CBehaviorGraphInstance& instance ) const;
	void InternalSlotReset( CBehaviorGraphInstance& instance ) const;

	void SetSlotLogicAction( CBehaviorGraphInstance& instance, ESlotAction action ) const;
	void ProcessSlotLogicAction( CBehaviorGraphInstance& instance ) const;

	void CheckInputActivation( CBehaviorGraphInstance& instance ) const;
	void CheckInputDeactivation( CBehaviorGraphInstance& instance ) const;

protected:
	virtual Bool IsLooped( CBehaviorGraphInstance& instance ) const;

	virtual void SlotReset( CBehaviorGraphInstance& instance ) const;

	virtual void AnimationSlotFinished( CBehaviorGraphInstance& instance ) const;
	virtual void AnimationSlotBlendOutStarted( CBehaviorGraphInstance& instance ) const;

	virtual Float GetBlendWeight( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsSlotPoseMimic() const;

	virtual void OnAnimationFinished( CBehaviorGraphInstance& instance ) const;

public:
	Float GetLocalTime( CBehaviorGraphInstance& instace ) const;
	Bool IsBlendingWithCachedPose( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationBaseSlotNode );
	PROPERTY( m_cachedBaseInputNode );
	PROPERTY_EDIT( m_startEvtName, TXT("") );
	PROPERTY_EDIT( m_stopEvtName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicSlotNode : public CBehaviorGraphAnimationSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicSlotNode, CBehaviorGraphAnimationSlotNode, "Slots.Mimic", "Blend slot" );

protected:
	TInstanceVar< Bool >		i_useCachePose;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime = 0.0f ) const override;

	virtual void CacheConnections();

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual Bool IsMimic() const { return true; }

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	virtual Bool HasCachedPose( CBehaviorGraphInstance& instance ) const;
	virtual void CachePose( CBehaviorGraphInstance& instance ) const;
	virtual void CreatePose( CBehaviorGraphInstance& instance ) const;
	virtual void DestroyPose( CBehaviorGraphInstance& instance ) const;

	virtual Bool IsLooped( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsSlotPoseMimic() const;
	virtual Bool IsValid( CBehaviorGraphInstance& instance ) const;
	virtual void SampleSlotAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationSlotNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationSlotWithCurveNode : public CBehaviorGraphAnimationSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationSlotWithCurveNode, CBehaviorGraphAnimationSlotNode, "Slots.Animation", "Blend with curve slot" );

protected:
	CCurve* m_curve;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif

protected:
	virtual Float GetBlendWeight( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationSlotWithCurveNode );
	PARENT_CLASS( CBehaviorGraphAnimationSlotNode );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("CurveSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicSlotWithCurveNode : public CBehaviorGraphMimicSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicSlotWithCurveNode, CBehaviorGraphMimicSlotNode, "Slots.Mimic", "Blend with curve slot" );

protected:
	CCurve* m_curve;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif

protected:
	virtual Float GetBlendWeight( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicSlotWithCurveNode );
	PARENT_CLASS( CBehaviorGraphMimicSlotNode );
	PROPERTY_CUSTOM_EDIT( m_curve, TXT("Curve"), TXT("CurveSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicSlotWithSwapingNode : public CBehaviorGraphMimicSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicSlotWithSwapingNode, CBehaviorGraphMimicSlotNode, "Slots.Mimic", "Blend with swaping" );

protected:
	Uint32	m_from;
	Uint32	m_to;

public:
	CBehaviorGraphMimicSlotWithSwapingNode();

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicSlotWithSwapingNode );
	PARENT_CLASS( CBehaviorGraphMimicSlotNode );
	PROPERTY_EDIT( m_from, TXT("") );
	PROPERTY_EDIT( m_to, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

struct SSlotEventAnim
{
	DECLARE_RTTI_STRUCT( SSlotEventAnim )

	CName	m_event;
	CName	m_animation;
	Float	m_blendIn;
	Float	m_blendOut;
	Bool	m_looped;

	SSlotEventAnim() : m_blendIn( 0.2f ), m_blendOut( 0.2f ), m_looped( false ) {}
};

BEGIN_CLASS_RTTI( SSlotEventAnim );
	PROPERTY_CUSTOM_EDIT( m_event, TXT("Event to trigger transition"), TXT("BehaviorEventEdition") );
	PROPERTY_CUSTOM_EDIT( m_animation, TXT("Animation name"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

class CBehaviorGraphMimicEventSlotNode : public CBehaviorGraphMimicSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicEventSlotNode, CBehaviorGraphMimicSlotNode, "Slots.Mimic", "Event blend slot" );

protected:
	TDynArray< SSlotEventAnim >			m_animations;

protected:
	TInstanceVar< TDynArray< Uint32 > >	i_events;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const override { return TXT("Event blend slot"); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const override;

	virtual void GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicEventSlotNode );
	PARENT_CLASS( CBehaviorGraphMimicSlotNode );
	PROPERTY_EDIT( m_animations, TXT("Slot animations") );
END_CLASS_RTTI();
