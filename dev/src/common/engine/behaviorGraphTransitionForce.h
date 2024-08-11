/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "allocatedBehaviorGraphOutput.h"
#include "behaviorGraphTransitionBlend.h"

class CBehaviorGraphStateNode;

class CBehaviorGraphStateTransitionGlobalBlendNode : public CBehaviorGraphStateTransitionBlendNode, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateTransitionGlobalBlendNode, CBehaviorGraphStateTransitionBlendNode, "State machine", "Global transition" );

protected:
	TagList		m_includeGroup;
	TagList		m_excludeGroup;
	CName		m_generateEventForDestState;			//!< if set, will work as gate - only allow to transition if not at this state
	CName		m_generateForcedEventForDestState;		//!< if set, will work as gate - only allow to transition if not at this state
	Bool		m_cachePoseFromPrevSampling;			//!< if set, will always cache global pose (as opposite to caching state machine's pose)
	Bool		m_useProgressiveSampilngForBlending;    //!< Use progressive blending instead of NLERP

	TInstanceVar< Float > i_progressiveSamplingAlpha;
	TInstanceVar< CAllocatedBehaviorGraphOutput > i_pose;

public:
	CBehaviorGraphStateTransitionGlobalBlendNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! initialize data instance
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	//! release data instance
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	//! update stage
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;

	//! sample stage
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	//! process external event
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	//! process force event
	virtual Bool ProcessForceEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! get block caption
	virtual String GetCaption() const;

	//! Get block shape
	virtual EGraphBlockShape GetBlockShape() const;

	//! Get block depth group
	virtual EGraphBlockDepthGroup GetBlockDepthGroup() const;

	//! Get block border color
	virtual Color GetBorderColor() const;

	//! Get client area color
	virtual Color GetClientColor() const;

#endif

	//! Called on block's activation
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! Called on block's deactivation
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! Called on activation of parent block's start input
	virtual void OnStartBlockActivated( CBehaviorGraphInstance& instance ) const;

	//! Called on deactivation of parent block's start input
	virtual void OnStartBlockDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Bool IsManualCreationAllowed() const { return false; }

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();

	virtual Bool CanConnectWith( CBehaviorGraphInstance& instance, const CBehaviorGraphStateNode* state ) const;

	void CreateAndCachePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;

protected:
	void UpdateProgressiveSamplingAlpha(CBehaviorGraphInstance& instance, Float currentTime, Float timeDelta) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateTransitionGlobalBlendNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionBlendNode );
	PROPERTY_EDIT( m_includeGroup, TXT("") );
	PROPERTY_EDIT( m_excludeGroup, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_generateEventForDestState, TXT("Generate event for dest state when activated"), TXT("BehaviorEventSelection") );
	PROPERTY_CUSTOM_EDIT( m_generateForcedEventForDestState, TXT("Generate force event for dest state when activated"), TXT("BehaviorEventSelection") );
	PROPERTY_EDIT( m_cachePoseFromPrevSampling, TXT("If set, will always cache global pose (as opposite to caching state machine's pose)") );
	PROPERTY_EDIT( m_useProgressiveSampilngForBlending, TXT("Enables progressive blending.") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

//#define NO_USE_GLOBALS_WITH_WAITING

// I love this stupid names...
class CBehaviorGraphStateTransitionGlobalBlendStreamingNode : public CBehaviorGraphStateTransitionGlobalBlendNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateTransitionGlobalBlendStreamingNode, CBehaviorGraphStateTransitionGlobalBlendNode, "State machine", "Global transition with streaming" );

protected:
	String										m_defaultStateName;		
	CBehaviorGraphStateNode*					m_defaultState;

protected:
	TInstanceVar< Bool >						i_waiting;

public:
	CBehaviorGraphStateTransitionGlobalBlendStreamingNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual Color GetBorderColor() const;
#endif

private:
	void InternalReset( CBehaviorGraphInstance& instance ) const;
	
	const CBehaviorGraphStateNode* GetDefaultNode( CBehaviorGraphInstance& instance ) const;

	void CacheDefaultNode();
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateTransitionGlobalBlendStreamingNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionGlobalBlendNode );
	PROPERTY_EDIT( m_defaultStateName, TXT("") );
	PROPERTY( m_defaultState );
END_CLASS_RTTI();
