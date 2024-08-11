
#pragma once

#include "behaviorGraphNode.h"
#include "behaviorGraphTransitionBlend.h"
#include "allocatedBehaviorGraphOutput.h"
#include "..\core\engineQsTransform.h"

class CBehaviorGraphStateTransitionMatchToPoseNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateTransitionMatchToPoseNode, CBehaviorGraphStateTransitionBlendNode, "State machine.Transitions", "Match to pose transition" );

	Bool											m_useMathMethod;

protected:
	TInstanceVar< CAllocatedBehaviorGraphOutput >	i_pose;
	TInstanceVar< Bool >							i_poseCached;

public:
	CBehaviorGraphStateTransitionMatchToPoseNode();

	virtual String GetCaption() const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	void CreatePose( CBehaviorGraphInstance& instance ) const;
	void CachePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
	void AddCachePose( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& animPose, const SBehaviorGraphOutput& cachedPose, Float alpha ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateTransitionMatchToPoseNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY_RO( m_useMathMethod, TXT("false = transition will be normal blend transition") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStateTransitionMatchFromPoseNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateTransitionMatchFromPoseNode, CBehaviorGraphStateTransitionBlendNode, "State machine.Transitions", "Match from pose transition" );

private:
	Bool											m_useMathMethod;

private:
	TInstanceVar< CAllocatedBehaviorGraphOutput >	i_pose;
	TInstanceVar< Bool >							i_poseCached;

public:
	CBehaviorGraphStateTransitionMatchFromPoseNode();

	virtual String GetCaption() const;

	RED_INLINE Bool UseMatchMathod() const { return m_useMathMethod; }

	void CacheMatchFromPose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	void CreatePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateTransitionMatchFromPoseNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY_EDIT( m_useMathMethod, TXT("false = transition will be normal blend transition") );
END_CLASS_RTTI();

class CBehaviorGraphMatchFromPoseNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMatchFromPoseNode, CBehaviorGraphBaseNode, "Misc", "Match from pose" );

public:
	Float											m_minDuration;

protected:
	TInstanceVar< CAllocatedBehaviorGraphOutput >	i_pose;
	TInstanceVar< Bool >							i_poseCached;
	TInstanceVar< Float >							i_additiveDuration;
	TInstanceVar< Float >							i_currTime;
	TInstanceVar< Bool >							i_running;

public:
	CBehaviorGraphMatchFromPoseNode();

	virtual String GetCaption() const;

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	void CreatePose( CBehaviorGraphInstance& instance ) const;
	void CachePose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
	void AddCachePose( SBehaviorGraphOutput& output, const SBehaviorGraphOutput& animPose, const SBehaviorGraphOutput& cachedPose, Float alpha ) const;

	Bool SuckPoseFromTransition( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& pose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMatchFromPoseNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_minDuration, TXT("Minimal duration time") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStatePelvisTransitionNode : public CBehaviorGraphStateTransitionBlendNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStatePelvisTransitionNode, CBehaviorGraphStateTransitionBlendNode, "State machine.Transitions", "Pelvis transition" );

	Bool											m_usePelvisBlendMethod;
	String											m_pelvisBoneName;
	EAxis											m_pelvisDirectionFwdLS;

protected:
	TInstanceVar< Int32 >							i_pelvisBoneIdx;
	TInstanceVar< Bool >							i_cacheOffset;
	TInstanceVar< EngineQsTransform >				i_offsetME;
	TInstanceVar< EngineQsTransform >				i_offsetPelvisLS;
	TInstanceVar< Float >							i_timeDelta;

public:
	CBehaviorGraphStatePelvisTransitionNode();

	virtual String GetCaption() const;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void InterpolatePoses( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput &poseB, Float alpha ) const;

	void CalcAndCacheOffset( CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput &poseB, Float alpha ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStatePelvisTransitionNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY_CUSTOM_EDIT( m_pelvisBoneName, TXT(""), TXT("BehaviorBoneSelection"));
	PROPERTY_EDIT( m_pelvisDirectionFwdLS, TXT("") );
	PROPERTY_EDIT( m_usePelvisBlendMethod, TXT("false = transition will be normal blend transition") );
END_CLASS_RTTI();
