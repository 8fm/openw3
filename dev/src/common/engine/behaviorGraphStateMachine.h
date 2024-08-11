/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphContainerNode.h"

class CBehaviorGraphStartingStateNode;
class CBehaviorGraphStateNode;
class CBehaviorSyncInfo;
struct SBehaviorSyncTags;

class CBehaviorGraphStateMachineNode : public CBehaviorGraphContainerNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateMachineNode, CBehaviorGraphContainerNode, "State machine", "State machine" );	
	
	TDynArray< CBehaviorGraphStateNode* >						m_states;
	TDynArray< CBehaviorGraphStateTransitionNode* >				m_transitions;
	TDynArray< CBehaviorGraphStateTransitionGlobalBlendNode* >	m_globalTransitions;

	CBehaviorGraphNode*											m_defaultNode;
	Bool														m_resetStateOnExit;
	Bool														m_applySyncTags;

	static const Uint32											MAX_SIMULTANEOUS_ACTIVE_TRANSITIONS = 16;

protected:
	TInstanceVar< TDynArray< CBehaviorGraphStateTransitionGlobalBlendNode* > >	i_activeForceTransitions;
	TInstanceVar< CBehaviorGraphNode* >											i_currentNode;
	TInstanceVar< CAllocatedBehaviorGraphOutput >								i_outputPose;

public:
	CBehaviorGraphStateMachineNode();	

	//! serialize graph
	virtual void OnSerialize( IFile& file );

	//! on pose load
	virtual void OnPostLoad();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! rebuild node sockets
	virtual void OnRebuildSockets();

	//! get block caption
	virtual String GetCaption() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

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

	//! push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! called on adding new node
	virtual void OnChildNodeAdded( CGraphBlock *node );

	//! remove child node
	virtual void RemoveChildNode( CGraphBlock *node );

	//! check if given class can be child node
	virtual Bool ChildNodeClassSupported( CClass *nodeClass );
#endif

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! process force event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! called on activation of node
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of node
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

public:
	//! switch to given state
	virtual void SwitchToState( CBehaviorGraphNode *node, CBehaviorGraphInstance& instance ) const;

	//! inform about switching current state
	void InformAboutCurrentState( CBehaviorGraphNode* willBeNoLongerCurrent, CBehaviorGraphNode* willBecomeCurrent, CBehaviorGraphInstance& instance ) const;

	//! set default state
	virtual void SetDefaultState( CBehaviorGraphStateNode *node );

	//! access default state
	virtual CBehaviorGraphNode* GetDefaultState() const;

	//! get current state
	virtual CBehaviorGraphNode* GetCurrentState( const CBehaviorGraphInstance& instance ) const;

	//! get current state name
	String GetCurrentStateName( const CBehaviorGraphInstance& instance ) const;

	//! Has global transition connected to
	Bool HasGlobalTransitionConnectedTo( const CBehaviorGraphStateNode* state ) const;

	//! Get cached output pose (from last sample)
	const SBehaviorGraphOutput* GetCachedOutputPose( const CBehaviorGraphInstance& instance ) const;

public:
	//! Behavior graph instance synchronization
	Bool GetSyncInfoForInstance( CBehaviorSyncInfo& info, CBehaviorGraphInstance& instance ) const;
	Bool SynchronizeInstanceTo( const CBehaviorSyncInfo& info, CBehaviorGraphInstance& instance ) const;
	Bool IsSynchronizing( CBehaviorGraphInstance& instance ) const;

	Bool GetOutboundSyncTags( SBehaviorSyncTags& tags, CBehaviorGraphInstance& instance ) const;
	Bool ApplyInboundSyncTags( SBehaviorSyncTags& tags, CBehaviorGraphInstance& instance ) const;

protected:
	//! process transitions that can occur at the moment
	void ProcessTransitions( CBehaviorGraphInstance& instance ) const;

	Bool ProcessForceTransition( CBehaviorGraphInstance& instance ) const;

	void GlobalTransitionsStartUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	void GlobalTransitionsStartBlockActivated( CBehaviorGraphInstance& instance ) const;

	void GlobalTransitionsStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;

	//void ActivateGlobalTransition( CBehaviorGraphInstance& instance, CBehaviorGraphNode *node ) const;

	//void DeactivateGlobalTransition( CBehaviorGraphInstance& instance, CBehaviorGraphNode *node ) const;

	//! reset all inactive global transitions - composite condition called reset only on last condition. Called in every tick.
	void ResetInactiveGlobalTransitions( CBehaviorGraphInstance& instance ) const;

	//! reset all global transitions - composite condition called reset all conditions. Called after global transition was activate.
	void ForceResetGlobalTransitions( CBehaviorGraphInstance& instance ) const;

	//! process sampled pose to transitions
	void ProcessSampledPoseToTransitions( CBehaviorGraphInstance& instance , const SBehaviorGraphOutput& pose ) const;

protected:
	//! Add all missing states and transitions that are not there
	void AddMissingNodes();

protected:
	void InternalActivate( CBehaviorGraphInstance& instance ) const;
	void InternalDeactivate( CBehaviorGraphInstance& instance ) const;

	void UpdateCurrentState( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	void UpdateTransitions( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	void CacheOutputPose( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void DestroyOutputPose( CBehaviorGraphInstance& instance ) const;

public:
	// Editor only
	void SetStateActive( CBehaviorGraphNode *node, CBehaviorGraphInstance& instance ) const;

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateMachineNode );
	PARENT_CLASS( CBehaviorGraphContainerNode );
	PROPERTY( m_globalTransitions );
	PROPERTY_EDIT( m_resetStateOnExit, TXT("") );
	PROPERTY_EDIT( m_applySyncTags, TXT("") );
END_CLASS_RTTI();
