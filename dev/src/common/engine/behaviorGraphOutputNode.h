/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphNode.h"

// class defining output node for given graph stage/element etc
class CBehaviorGraphOutputNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphOutputNode, CBehaviorGraphNode, "Misc", "Output" )		

protected:
	CBehaviorGraphNode*							m_cachedInputNode;
	TDynArray< CBehaviorGraphValueNode* >		m_cachedCustomInputNodes;
	TDynArray< CBehaviorGraphValueNode* >		m_cachedFloatInputNodes;

public:
	CBehaviorGraphOutputNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Output"); }
	virtual Color GetTitleColor() const;

#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void SetCustomFloatSocketCaptions();
	void SetFloatSocketCaptions();
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphOutputNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedCustomInputNodes );
	PROPERTY( m_cachedFloatInputNodes );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphOverrideFloatTracksNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphOverrideFloatTracksNode, CBehaviorGraphBaseNode, "Misc", "Float track override" );

protected:
	TDynArray< CBehaviorGraphValueNode* >		m_cachedFloatInputNodes;
	Bool	m_overrideZeros;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Float track override"); }
#endif
	CBehaviorGraphOverrideFloatTracksNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphOverrideFloatTracksNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedFloatInputNodes );
	PROPERTY_EDIT( m_overrideZeros, TXT( "Should override with zero values" ) );
END_CLASS_RTTI();
