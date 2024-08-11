/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "../engine/behaviorGraphNode.h"

enum EBehaviorGraphRagdollNodeState
{
	BGRN_Inactive,
	BGRN_Kinematic,
	BGRN_Ragdoll,
	BGRN_SwitchingFromRagdoll
};

BEGIN_ENUM_RTTI( EBehaviorGraphRagdollNodeState );
	ENUM_OPTION( BGRN_Inactive );
	ENUM_OPTION( BGRN_Kinematic );
	ENUM_OPTION( BGRN_Ragdoll );
	ENUM_OPTION( BGRN_SwitchingFromRagdoll );
END_ENUM_RTTI();

class CBehaviorGraphRagdollNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRagdollNode, CBehaviorGraphBaseNode, "Ragdoll", "Ragdoll" );

protected:
	Bool m_allowToProvidePreRagdollPose;
	Bool m_updateAndSampleInputIfPreRagdollWeightIsNonZero;
	Bool m_keepInFrozenRagdollPose;
	Bool m_switchToSwimming;

protected:
	TInstanceVar< Bool >							i_ragdollExists;
	TInstanceVar< Float	>							i_controlValue;
	TInstanceVar< Float	>							i_prevControlValue;
	TInstanceVar< Float	>							i_startedToSwitchAtControlValue;
	TInstanceVar< Vector >							i_impulse;
	TInstanceVar< EBehaviorGraphRagdollNodeState >	i_state;
	TInstanceVar< CAllocatedBehaviorGraphOutput >	i_ragdolledPose;

protected:
	CBehaviorGraphValueNode*		m_cachedControlVariableNode;
	CBehaviorGraphVectorValueNode*	m_cachedRootBoneImpulseVariable;

public:
	CBehaviorGraphRagdollNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Ragdoll") ); }

#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnReleaseInstance( CBehaviorGraphInstance& instance ) const;
	virtual Bool IsOnReleaseInstanceManuallyOverridden() const override { return true; }

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
	void FastPoseReset( SBehaviorGraphOutput* pose ) const;
	void UpdateControlValues( CBehaviorGraphInstance& instance ) const;

	void ChangeState( CBehaviorGraphInstance& instance, EBehaviorGraphRagdollNodeState newState ) const;

	void CreateAndCacheRagdolledPose( CBehaviorGraphInstance& instance ) const;
	void DestroyRagdolledPose( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRagdollNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_allowToProvidePreRagdollPose, TXT("") );
	PROPERTY_EDIT( m_updateAndSampleInputIfPreRagdollWeightIsNonZero, TXT("") );
	PROPERTY_EDIT( m_keepInFrozenRagdollPose, TXT("") );
	PROPERTY_EDIT( m_switchToSwimming, TXT("") );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedRootBoneImpulseVariable );
END_CLASS_RTTI();
