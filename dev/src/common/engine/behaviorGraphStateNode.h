/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once
#include "behaviorGraphContainerNode.h"

#include "../core/tagList.h"

class CBehaviorGraphInstance;
class CBehaviorSyncInfo;
struct SBehaviorSyncTags;

///////////////////////////////////////////////////////////////////////////////
//
//	Contains sync tags used for synchronization between behavior graphs.
//	For more info, check CBehaviorGraphStateMachineNode's GetOutboundSyncTags
//	and ApplyInboundSyncTags
//

struct SBehaviorGraphStateBehaviorGraphSyncInfo
{
	DECLARE_RTTI_STRUCT( SBehaviorGraphStateBehaviorGraphSyncInfo );

	TDynArray< CName >		m_outSyncTags;				//!< Tags used for outbound synchronization
	TDynArray< CName >		m_inSyncTags;				//!< Tags used for inbound synchronization
	Int32					m_inSyncPriority;			//!< Inbound synchronization priority (greater number means more important prio)
	Bool					m_inAllSyncTagsRequired;	//!< All tags used for inbound synchronization are required

	SBehaviorGraphStateBehaviorGraphSyncInfo();

	Int32 GetMatchingInboundTagCount( const SBehaviorSyncTags& syncTags) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	String GetInListAsString() const;
#endif
};

BEGIN_CLASS_RTTI( SBehaviorGraphStateBehaviorGraphSyncInfo );
	PROPERTY_EDIT_NAME( m_outSyncTags, TXT("Outbound sync tags"), TXT("Tags used for outbound synchronization") )
	PROPERTY_EDIT_NAME( m_inSyncTags, TXT("Inbound sync tags"), TXT("Tags used for inbound synchronization") )
	PROPERTY_EDIT_NAME( m_inSyncPriority, TXT("Inbound sync priority"), TXT("Inbound synchronization priority (greater number means more important prio)") )
	PROPERTY_EDIT_NAME( m_inAllSyncTagsRequired, TXT("All inbound tags required?"), TXT("All tags used for inbound synchronization are required") )
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
//
// A state in a behavior state machine
//

class CBehaviorGraphStateNode : public CBehaviorGraphContainerNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateNode, CBehaviorGraphContainerNode, "State machine", "State" );	

protected:
	CBehaviorGraphNode*									m_rootNode;
	TagList												m_groups;
	SBehaviorGraphStateBehaviorGraphSyncInfo			m_behaviorGraphSyncInfo;	//!< Used to synchronize one graph to another

protected:
	TDynArray< CBehaviorGraphStateTransitionNode* >		m_cachedStateTransitions;

public:
	CBehaviorGraphStateNode();

	void OnSerialize( IFile& file );

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! block was spawned
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! recreate sockets
	virtual void OnRebuildSockets();

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get block depth group
	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const;

	//! get block caption
	virtual String GetCaption() const;

	void OnCopyAllTransitionsFrom( CBehaviorGraphStateNode *const stateNode );
#endif

	//! update stage
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! sample stage
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	//! get sync info
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;

	//! synchronize to given sync info
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	//! push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! process external event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! cache connections
	virtual void CacheConnections();

	//! called on activation of node
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of node
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	Bool GetSyncInfoForInstance( CBehaviorGraphInstance& instance, CBehaviorSyncInfo& info ) const;

	//! preload animations
	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

	//! called on becoming current state
	virtual void OnBecomesCurrentState( CBehaviorGraphInstance& instance ) const {};

	//! called on becoming being no longer current state
	virtual void OnNoLongerCurrentState( CBehaviorGraphInstance& instance ) const {};

	//! called whend state is fully blended in, weight is 1.0
	virtual void OnFullyBlendedIn( CBehaviorGraphInstance& instance ) const {};

public:
	//! get number of outgoing transitions connected to given node
	Uint32 GetNumConnectedTransitions() const;

	//! get i-th outgoing transition
	CBehaviorGraphStateTransitionNode* GetConnectedTransition( Uint32 index ) const;

	//! get group list
	const TagList& GetGroupList() const;

protected:
	//! notify transitions about activation of their start block
	void TransitionsStartBlockActivated( CBehaviorGraphInstance& instance ) const;

	//! notify transitions about deactivation of their start block
	void TransitionsStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;

	//! notify transitions about update of their start block
	void TransitionsStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

public:
	//! gather sync tags for synchronization between behavior graphs
	Bool GetOutboundSyncTags( SBehaviorSyncTags& tags ) const;

	const SBehaviorGraphStateBehaviorGraphSyncInfo& GetBehaviorGraphSyncInfo() const { return m_behaviorGraphSyncInfo; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateNode );
	PARENT_CLASS( CBehaviorGraphContainerNode );
	PROPERTY_EDIT( m_groups, TXT("") );
	PROPERTY_EDIT( m_behaviorGraphSyncInfo, TXT("Used for intergraph synchronization") );
	PROPERTY( m_cachedStateTransitions );
END_CLASS_RTTI();

