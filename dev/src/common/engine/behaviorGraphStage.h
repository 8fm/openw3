/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphContainerNode.h"

class CBehaviorGraph;
class CBehaviorGraphInstance;
class CBehaviorGraphStageNode;
class CBehaviorGraphStageInstance;
class CBehaviorGraphNode;
class CBehaviorGraphNodeInstance;

class CBehaviorGraphStageNode : public CBehaviorGraphContainerNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStageNode, CBehaviorGraphContainerNode, "Misc", "Stage" );		

protected:
	CBehaviorGraphNode*		m_cachedInputNode;
	Bool					m_activeByDefault;

protected:
	TInstanceVar< Bool >	i_stageActive;

protected:
	CBehaviorGraphNode*		m_rootNode;

public:	
	//! Get the stage root node
	RED_INLINE const CBehaviorGraphNode* GetRootNode() const { return m_rootNode; }

	//! Get the stage root node
	RED_INLINE CBehaviorGraphNode* GetRootNode() { return m_rootNode; }

	//! Is the state active
	Bool IsStageActive( CBehaviorGraphInstance& instance ) const;

public:
	CBehaviorGraphStageNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );
#endif
	virtual void OnSerialize( IFile& file );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! rebuild node sockets
	virtual void OnRebuildSockets();

	//! get block caption
	virtual String GetCaption() const;

#endif

	//! update stage
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! sample stage
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	//! reset stage to default state
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	//! get sync info
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;

	//! synchronize to given sync info
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	// process activation alpha
	void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! called on activation of node
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of node
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! cache connections
	virtual void CacheConnections();

	//! activate this stage
	void ActivateStage( CBehaviorGraphInstance& instance ) const;

	//! deactivate this stage
	void DeactivateStage( CBehaviorGraphInstance& instance ) const;

	//! Preload animations
	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStageNode );
	PARENT_CLASS( CBehaviorGraphContainerNode );
	PROPERTY_EDIT( m_activeByDefault, TXT("Is stage active by default") );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

