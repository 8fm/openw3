/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"

class CBehaviorGraphInputNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphInputNode, CBehaviorGraphNode, "Misc", "Input" );	

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const {}

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const { return false; }

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();

	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;

#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphInputNode );
	PARENT_CLASS( CBehaviorGraphNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphTPoseNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphTPoseNode, CBehaviorGraphNode, "Misc", "TPose" );	

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const {}

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const { return false; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphTPoseNode );
	PARENT_CLASS( CBehaviorGraphNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphIdentityPoseNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphIdentityPoseNode, CBehaviorGraphNode, "Misc", "Identity Pose" );	

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const {}

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const { return false; }

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphIdentityPoseNode );
	PARENT_CLASS( CBehaviorGraphNode );
END_CLASS_RTTI();
