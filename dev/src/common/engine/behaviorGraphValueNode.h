/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphNode.h"

//! graph node with value semantic
class CBehaviorGraphValueNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphValueNode, CBehaviorGraphNode );

public:
	CBehaviorGraphValueNode();

	//! Get title color
	virtual Color GetTitleColor() const;

	//! Get value
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const = 0;

	//! Value node hasn't got synchronization
	virtual void GetSyncInfo( CBehaviorGraphInstance& /*instance*/, CSyncInfo &/*info*/ ) const {}
	virtual void SynchronizeTo( CBehaviorGraphInstance& /*instance*/, const CSyncInfo &/*info*/ ) const {}
	virtual Bool ProcessEvent( CBehaviorGraphInstance& /*instance*/, const CBehaviorEvent &/*event*/ ) const { return false; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphValueNode );
	PARENT_CLASS( CBehaviorGraphNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphValueBaseNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphValueBaseNode, CBehaviorGraphValueNode );

protected:
	TInstanceVar< Float >		i_value;

protected:
	CBehaviorGraphValueNode*	m_cachedInputNode;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& /*context*/, CBehaviorGraphInstance& /*instance*/, SBehaviorGraphOutput &/*output*/ ) const {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void CacheConnections();
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphValueBaseNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

//! graph node with vector value semantic
class CBehaviorGraphVectorValueNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphVectorValueNode, CBehaviorGraphValueNode );

public:
	//! get title color
	virtual Color GetTitleColor() const;

	//! get value
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const = 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphVectorValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

//! graph node with vector value semantic
class CBehaviorGraphVectorValueBaseNode : public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_ABSTRACT_CLASS( CBehaviorGraphVectorValueBaseNode, CBehaviorGraphVectorValueNode );

protected:
	TInstanceVar< Vector >			i_value;

protected:
	CBehaviorGraphVectorValueNode*	m_cachedInputNode;

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& /*context*/, CBehaviorGraphInstance& /*instance*/, SBehaviorGraphOutput &/*output*/ ) const {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void CacheConnections();
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehaviorGraphVectorValueBaseNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();
