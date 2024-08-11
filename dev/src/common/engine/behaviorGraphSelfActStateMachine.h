/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphStateMachine.h"
#include "behaviorGraphStateNode.h"
#include "behaviorGraphTransitionBlend.h"

class CBehaviorGraphDefaultSelfActStateNode : public CBehaviorGraphStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDefaultSelfActStateNode, CBehaviorGraphStateNode, "State machine", "Invalid State (CBehaviorGraphDefaultSelfActStateNode)" );	

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool CanBeExpanded() const;
#endif

	virtual Bool IsManualCreationAllowed() const { return false; }

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphDefaultSelfActStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSelfActivatingStateMachineNode : public CBehaviorGraphStateMachineNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSelfActivatingStateMachineNode, CBehaviorGraphStateMachineNode, "State machine", "State machine - self act" );	

protected:
	TInstanceVar< Bool >	i_activateMachine;
	TInstanceVar< Bool >	i_running;

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
	CBehaviorGraphSelfActivatingStateMachineNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

protected:
	virtual CBehaviorGraphStateNode* CreateDefaultStateNode();
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

	//! process event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! process force event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! called on activation of node
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! called on deactivation of node
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

public:
	Bool IsRunning( CBehaviorGraphInstance& instance ) const;

	CBehaviorGraphNode* GetInputNode() const;

protected:
	void InternalReset( CBehaviorGraphInstance& instance ) const;

	void RequestActivateMachine( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSelfActivatingStateMachineNode );
	PARENT_CLASS( CBehaviorGraphStateMachineNode );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSelfActivatingOverrideStateMachineNode : public CBehaviorGraphSelfActivatingStateMachineNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSelfActivatingOverrideStateMachineNode, CBehaviorGraphSelfActivatingStateMachineNode, "State machine", "State machine - self act with override" );	

protected:
	TDynArray< SBehaviorGraphBoneInfo >	m_bones;
	Bool								m_overrideFloatTracks;
	Bool								m_overrideCustomTracks;
	Bool								m_mergeEvents;
	Bool								m_overrideDeltaMotion;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

	virtual TDynArray< SBehaviorGraphBoneInfo >* GetBonesProperty();

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSelfActivatingOverrideStateMachineNode );
	PARENT_CLASS( CBehaviorGraphSelfActivatingStateMachineNode );
	PROPERTY_CUSTOM_EDIT( m_bones, TXT("Bones with weights"), TXT("BehaviorBoneMultiSelectionWithWeight") );
	PROPERTY_EDIT( m_overrideFloatTracks, TXT("Override float tracks") );
	PROPERTY_EDIT( m_overrideCustomTracks, TXT("Override custom tracks") );
	PROPERTY_EDIT( m_mergeEvents, TXT("Merge events") );
	PROPERTY_EDIT( m_overrideDeltaMotion, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSyncOverrideStateMachineNode : public CBehaviorGraphSelfActivatingStateMachineNode, public IBehaviorGraphBonesPropertyOwner
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSyncOverrideStateMachineNode, CBehaviorGraphSelfActivatingStateMachineNode, "State machine", "State machine - self act with sync override" );	

protected:
	String								m_rootBoneName;
	Bool								m_blendRootParent;
	Float								m_defaultWeight;
	Bool								m_mergeEvents;

protected:
	TInstanceVar< TDynArray< Int32 > >	i_bones;
	TInstanceVar< Int32 >				i_boneRoot;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

private:
	void FillBones( Int32 rootBone, TDynArray< Int32 >& bones, CBehaviorGraphInstance& instance ) const;
	void ConnectTwoPoses( SBehaviorGraphOutput &a, SBehaviorGraphOutput &b, Uint32 bone, CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSyncOverrideStateMachineNode );
	PARENT_CLASS( CBehaviorGraphSelfActivatingStateMachineNode );
	PROPERTY_CUSTOM_EDIT( m_rootBoneName, TXT("Magic bone name"), TXT("BehaviorBoneSelection") );
	PROPERTY_EDIT( m_blendRootParent, TXT("") );
	PROPERTY_EDIT( m_defaultWeight, TXT("") );
	PROPERTY_EDIT( m_mergeEvents, TXT("Merge events") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphDefaultSelfActAdditiveStateNode : public CBehaviorGraphStateNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDefaultSelfActAdditiveStateNode, CBehaviorGraphStateNode, "State machine", "Invalid State (CBehaviorGraphDefaultSelfActAdditiveStateNode)" );	

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual Bool CanBeExpanded() const;
#endif

	virtual void CacheConnections();

	virtual Bool IsManualCreationAllowed() const { return false; }

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphDefaultSelfActAdditiveStateNode );
	PARENT_CLASS( CBehaviorGraphStateNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStateAdditiveTransitionNode : public CBehaviorGraphStateTransitionBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateAdditiveTransitionNode, CBehaviorGraphStateTransitionBlendNode, "State machine", "Blend transition (additive)" );

protected:
	CBehaviorGraphNode*		m_cachedAdditiveNode;

public:
	virtual Bool IsManualCreationAllowed() const { return false; }

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateAdditiveTransitionNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY( m_cachedAdditiveNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphSelfActivatingAdditiveStateMachineNode : public CBehaviorGraphSelfActivatingStateMachineNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSelfActivatingAdditiveStateMachineNode, CBehaviorGraphSelfActivatingStateMachineNode, "State machine", "State machine - self act additive" );	

protected:
	EAdditiveType	m_type;
	Bool			m_mergeEvents;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;

protected:
	virtual CBehaviorGraphStateNode* CreateDefaultStateNode();
#endif

	CBehaviorGraphSelfActivatingAdditiveStateMachineNode();

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSelfActivatingAdditiveStateMachineNode );
	PARENT_CLASS( CBehaviorGraphSelfActivatingStateMachineNode );
	PROPERTY_EDIT( m_type, TXT("") );
	PROPERTY_EDIT( m_mergeEvents, TXT("Merge events") );
END_CLASS_RTTI();
