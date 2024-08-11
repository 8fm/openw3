/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphContainerNode.h"

class CBehaviorGraphTopLevelNode : public CBehaviorGraphContainerNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphTopLevelNode, CBehaviorGraphContainerNode, "Misc", "Top level node" );		

protected:
	CBehaviorGraphNode*		m_rootNode;
	CBehaviorGraphNode*		m_inputNode;

public:	
	//! Get the root node
	RED_INLINE const CBehaviorGraphNode* GetRootNode() const { return m_rootNode; }

	//! Get the root node
	RED_INLINE CBehaviorGraphNode* GetRootNode() { return m_rootNode; }

public:
	CBehaviorGraphTopLevelNode();	

public:
	//! serialize graph
	virtual void OnSerialize( IFile& file );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	void OnSpawned( const GraphBlockSpawnInfo& info );
#endif

public:
	//! get block caption
	virtual String GetCaption() const { return String( TXT("Top level node") ); }

	//! update stage
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! sample stage
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	//! push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! get sync info
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;

	//! synchronize to given sync info
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! called on activation of node
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivatio of node
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! cache connections
	virtual void CacheConnections();

	//! Remove sockets and connections ( after caching )
	virtual void RemoveConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphTopLevelNode );
	PARENT_CLASS( CBehaviorGraphContainerNode );
END_CLASS_RTTI();

