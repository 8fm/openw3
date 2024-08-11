/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "allocatedBehaviorGraphOutput.h"
#include "behaviorGraphStateNode.h"

class CBehaviorGraphFrozenStateNode : public CBehaviorGraphStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphFrozenStateNode, CBehaviorGraphStateNode, "State machine.Entry states", "Freezing" );

	TInstanceVar< CAllocatedBehaviorGraphOutput > i_pose;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
	virtual Bool CanBeExpanded() const;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

protected:
	void CreateAndCachePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphFrozenStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPoseMemoryNode : public CBehaviorGraphBaseNode, public IBehaviorGraphProperty
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseMemoryNode, CBehaviorGraphBaseNode, "Misc", "Pose Memory" );

protected:
	CName	m_blendOutEvent;
	Float	m_blendOutDuration;

protected:
	TInstanceVar< CAllocatedBehaviorGraphOutput >	i_pose;
	TInstanceVar< Uint32 >							i_blendOutEventId;
	TInstanceVar< Float >							i_blendOutTimer;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Memory"); }
#endif

	CBehaviorGraphPoseMemoryNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override; 

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const override;

protected:
	void CreateAndCachePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
	void InternalReset( CBehaviorGraphInstance& instance ) const;
	void StartBlendOutTimer( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseMemoryNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_blendOutEvent, TXT("Event to start blend out"), TXT("BehaviorEventEdition") );
	PROPERTY_EDIT( m_blendOutDuration, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPoseMemoryNode_Mimic : public CBehaviorGraphPoseMemoryNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseMemoryNode_Mimic, CBehaviorGraphPoseMemoryNode, "Mimic", "Pose Memory" );

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif
	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseMemoryNode_Mimic );
	PARENT_CLASS( CBehaviorGraphPoseMemoryNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
