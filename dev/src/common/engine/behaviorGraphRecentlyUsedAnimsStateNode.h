/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphOutput.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphSimplePlaybackUtils.h"
#include "allocatedBehaviorGraphOutput.h"

class CBehaviorGraphRecentlyUsedAnimsStateNode : public CBehaviorGraphStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRecentlyUsedAnimsStateNode, CBehaviorGraphStateNode, "State machine.Entry states", "Recently Used Anims" );

protected:
	Float m_poseBlendOutTime;
	Bool m_applyMotion;

protected:
	TInstanceVar< CAllocatedBehaviorGraphOutput > i_pose; // pose is used to mask missing additive animations, we will quickly blend out from the pose into playback and continue with playback
	TInstanceVar< Float > i_poseWeight;
	TInstanceVar< SSimpleAnimationPlaybackSet > i_playbacks;

public:
	CBehaviorGraphRecentlyUsedAnimsStateNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
	virtual Bool CanBeExpanded() const;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;

protected:
	void CreateAndCachePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRecentlyUsedAnimsStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
	PROPERTY_EDIT( m_poseBlendOutTime, TXT( "Time used to blend out from initial pose, -1 means blend from frozen prev frame pose" ) )
	PROPERTY_EDIT( m_applyMotion, TXT( "We should apply motion" ) )
END_CLASS_RTTI();
