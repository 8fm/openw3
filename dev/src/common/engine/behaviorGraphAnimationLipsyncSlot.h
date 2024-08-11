
#pragma once

#include "behaviorGraphAnimationAdditiveSlot.h"
#include "behaviorGraphAnimationSlotNode.h"
#include "poseHandle.h"

class CPoseProvider;

//////////////////////////////////////////////////////////////////////////

class CAllocatedLipsyncBehaviorGraphOutput
{
	DECLARE_RTTI_SIMPLE_CLASS( CAllocatedLipsyncBehaviorGraphOutput );

	CPoseHandle	m_pose;
	CBehaviorGraphInstance*	m_instance; // we need to keep those to allow copying

public:
	CAllocatedLipsyncBehaviorGraphOutput();
	~CAllocatedLipsyncBehaviorGraphOutput();

	void Create( CBehaviorGraphInstance& instance );
	void Free( CBehaviorGraphInstance& instance );

	RED_INLINE SBehaviorGraphOutput* GetPose() { return m_pose.Get(); }
	RED_INLINE Bool HasPose() const { return m_pose; }

private:
	CAllocatedLipsyncBehaviorGraphOutput( const CAllocatedLipsyncBehaviorGraphOutput& );

	CPoseProvider* GetAlloc() const;

public:
	CAllocatedLipsyncBehaviorGraphOutput& operator=( const CAllocatedLipsyncBehaviorGraphOutput& rhs );
};

BEGIN_CLASS_RTTI( CAllocatedLipsyncBehaviorGraphOutput );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationLipsyncSlotNode : public CBehaviorGraphAnimationBaseSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationLipsyncSlotNode, CBehaviorGraphAnimationBaseSlotNode, "Slots.Animation", "Lipsync slot" );

protected:
	TInstanceVar< CAllocatedLipsyncBehaviorGraphOutput >	i_pose;

protected:
	CBehaviorGraphNode*		m_cachedBaseAnimInputNode;
	CBehaviorGraphNode*		m_cachedAdditiveAnimInputNode;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

	CBehaviorGraphAnimationLipsyncSlotNode();

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

public:
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, const CName& animation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool PlayAnimation( CBehaviorGraphInstance& instance, CSkeletalAnimationSetEntry* skeletalAnimation, const SBehaviorSlotSetup* slotSetup = NULL ) const;
	virtual Bool StopAnimation( CBehaviorGraphInstance& instance, Float blendOutTime = 0.0f ) const override;

protected:
	virtual Bool UseFovTrack( CBehaviorGraphInstance& instance ) const;
	virtual Bool UseDofTrack( CBehaviorGraphInstance& instance ) const;
	virtual void OnAnimationFinished( CBehaviorGraphInstance& instance ) const;
	virtual Bool ShouldDoPoseCorrection() const;
	virtual Bool ShouldAddAnimationUsage() const;

private:
	void AllocPose( CBehaviorGraphInstance& instance ) const;
	Bool HasAllocedPose( CBehaviorGraphInstance& instance ) const;
	void DeallocPose( CBehaviorGraphInstance& instance ) const;
	SBehaviorGraphOutput* GetAllocedPose( CBehaviorGraphInstance& instance ) const;

	void CacheState( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput& pose ) const;

	void BlendLipsyncToAdditivePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& addPose, const SBehaviorGraphOutput& mimicPose ) const;
	void BlendAddPoseToMainPose( SBehaviorGraphOutput& mainPose, const SBehaviorGraphOutput& addPose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationLipsyncSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationBaseSlotNode );
	PROPERTY( m_cachedBaseAnimInputNode );
	PROPERTY( m_cachedAdditiveAnimInputNode );
END_CLASS_RTTI();
