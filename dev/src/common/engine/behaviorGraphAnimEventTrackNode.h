/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphAnimEventTrackNode : public CBehaviorGraphValueNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimEventTrackNode, CBehaviorGraphValueNode, "Misc", "Anim Event Track" );

protected:
	CName					m_eventName;

protected:
	TInstanceVar< Float >	i_value;

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
	CBehaviorGraphAnimEventTrackNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimEventTrackNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_eventName, TXT("Event name"));
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();
