/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphTransitionNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphStateNode;

class CBehaviorGraphStateTransitionBlendNode : public CBehaviorGraphStateTransitionNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStateTransitionBlendNode, CBehaviorGraphStateTransitionNode, "State machine.Transitions", "Blend transition" );

protected:
	Float								m_transitionTime;
	Bool								m_synchronize;
	IBehaviorSyncMethod*				m_syncMethod;
	EBehaviorTransitionBlendMotion		m_motionBlendType;

protected:
	TInstanceVar< Float >				i_currentTime;

public:
	CBehaviorGraphStateTransitionBlendNode();

	RED_INLINE Float GetTransitionTime() const { return m_transitionTime; }

	//! get block caption
	virtual String GetCaption() const;

public:
	//! Build block data layout
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	//! Initialize instance buffer
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	//! Build  instance properties
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	//! update stage
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! sample stage
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const; 

	//! reset stage to default state
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	//! process synchronization
	virtual void Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const;

	//! get sync info
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;

	//! synchronize to given sync info
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	//! push activation alpha through graph
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	//! block activated
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	//! block deactivated
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	//! copy data from
	virtual void CopyFrom( const CBehaviorGraphStateTransitionNode* node );

	virtual const CBehaviorGraphStateNode* GetCloserState( CBehaviorGraphInstance& instance ) const;
	using CBehaviorGraphStateTransitionNode::GetCloserState;

	const IBehaviorSyncMethod* GetSyncMethod() const;

protected:
	Float GetAlpha( CBehaviorGraphInstance& instance ) const;

public: // Editor only
	void CreateDefault_FootLeft();
	void CreateDefault_FootRight();
	void CreateDefault_AnimEndAUX();

protected:
	void CreateAnimEventDefault( const CName& eventName );
	void CreateEventDefault( const CName& eventName );

	void DisableSynchronization( CBehaviorGraphInstance& instance ) const;

	virtual void InterpolatePoses( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, const SBehaviorGraphOutput &poseA, const SBehaviorGraphOutput &poseB, Float alpha ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStateTransitionBlendNode );
	PARENT_CLASS( CBehaviorGraphStateTransitionNode );
	PROPERTY_EDIT( m_transitionTime, TXT("Transition time") );
	PROPERTY_EDIT( m_synchronize, TXT("Synchronize child playback") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_motionBlendType, TXT("") );
END_CLASS_RTTI();