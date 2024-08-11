/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

class CBehaviorGraphValueNode;

#include "../engine/behaviorGraphNode.h"
#include "../engine/behaviorIkApplyRotation.h"

enum EBehaviorGraphAnimatedRagdollNodeState
{
	BGARN_Ragdoll,
	BGARN_Flying,
	BGARN_HitWall,
	BGARN_HitGround,
	BGARN_MAX // does not need to be in enum
};

BEGIN_ENUM_RTTI( EBehaviorGraphAnimatedRagdollNodeState );
	ENUM_OPTION( BGARN_Ragdoll );
	ENUM_OPTION( BGARN_Flying );
	ENUM_OPTION( BGARN_HitWall );
	ENUM_OPTION( BGARN_HitGround );
END_ENUM_RTTI();

//

struct SBehaviorGraphAnimatedRagdollDirReplacement
{
	DECLARE_RTTI_STRUCT( SBehaviorGraphAnimatedRagdollDirReplacement );

	Float m_probability;
	Uint32 m_index;

	SBehaviorGraphAnimatedRagdollDirReplacement()
	: m_probability( 0.3f )
	, m_index( 0 )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorGraphAnimatedRagdollDirReplacement );
	PROPERTY_EDIT( m_probability, TXT("") );
	PROPERTY_EDIT( m_index, TXT("") );
END_CLASS_RTTI();

//

struct SBehaviorGraphAnimatedRagdollDirDefinition
{
	DECLARE_RTTI_STRUCT( SBehaviorGraphAnimatedRagdollDirDefinition );

	Float m_dirIndexValue;
	Float m_relativeAngle;
	Float m_applyAngleToMovement;
	TDynArray<SBehaviorGraphAnimatedRagdollDirReplacement> m_replacements;

	SBehaviorGraphAnimatedRagdollDirDefinition()
	: m_dirIndexValue( 0.0f )
	, m_relativeAngle( 0.0f )
	, m_applyAngleToMovement( 0.0f )
	{
	}
};

BEGIN_CLASS_RTTI( SBehaviorGraphAnimatedRagdollDirDefinition );
	PROPERTY_EDIT( m_dirIndexValue, TXT("") );
	PROPERTY_EDIT( m_relativeAngle, TXT("") );
	PROPERTY_EDIT( m_applyAngleToMovement, TXT("") );
	PROPERTY_EDIT( m_replacements, TXT("") );
END_CLASS_RTTI();

//

class CBehaviorGraphAnimatedRagdollNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimatedRagdollNode, CBehaviorGraphValueNode, "Ragdoll", "Animated \"ragdoll\"" );

private:
	Float m_chanceToGoToRagdoll; // At every change state
	Float m_stateBlendTime; // How quickly states blend - note that it doesn't blend to ragdoll, just switches
	Vector2 m_maxFlightTime; // Max flight time before switches to ragdoll
	Vector2 m_initialVelocityBoostZ;
	Float m_gravity;
	Float m_topVerticalVelocity;
	CName m_switchAnimatedRagdollToRagdollEvent;
	SApplyRotationIKSolverData m_poseRotateIK;
	TDynArray< SBehaviorGraphAnimatedRagdollDirDefinition > m_dirIndices;

private:
	TInstanceVar< Float >									i_timeDelta;
	TInstanceVar< Vector >									i_velocityWS; 
	TInstanceVar< EBehaviorGraphAnimatedRagdollNodeState >	i_state;
	TInstanceVar< EBehaviorGraphAnimatedRagdollNodeState >	i_prevState;
	TInstanceVar< Float >									i_stateWeight;
	TInstanceVar< Float >									i_timeInState;
	TInstanceVar< Float >									i_flightTime;
	TInstanceVar< Bool >									i_switchToRagdoll;
	TInstanceVar< SApplyRotationIKSolver >					i_poseRotateIK;
	TInstanceVar< Vector >									i_hitGroundNormalMS;
	TInstanceVar< Vector >									i_hitWallNormalMS;
	TInstanceVar< Float >									i_physicalRadius;
	TInstanceVar< Float >									i_dirIndex;
	TInstanceVar< Float >									i_dirIndexApplyAngle;

private:
	TDynArray<CBehaviorGraphNode*> m_cachedNodes;

public:
	CBehaviorGraphAnimatedRagdollNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Animated \"ragdoll\"") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

private:
	void ChangeState( CBehaviorGraphInstance& instance, EBehaviorGraphAnimatedRagdollNodeState newState, Bool initial = false ) const;
	void ApplyVelocity( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	void ApplyAdjustmentsToOutput( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, EBehaviorGraphAnimatedRagdollNodeState state ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimatedRagdollNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_chanceToGoToRagdoll, TXT("At every change state") );
	PROPERTY_EDIT( m_stateBlendTime, TXT("How quickly states blend - note that it doesn't blend to ragdoll, just switches") );
	PROPERTY_EDIT( m_maxFlightTime, TXT("") );
	PROPERTY_EDIT( m_initialVelocityBoostZ, TXT("") );
	PROPERTY_EDIT( m_gravity, TXT("") );
	PROPERTY_EDIT( m_topVerticalVelocity, TXT("") );
	PROPERTY_EDIT( m_switchAnimatedRagdollToRagdollEvent, TXT("") );
	PROPERTY_INLINED( m_poseRotateIK, String::EMPTY );
	PROPERTY_EDIT( m_dirIndices, TXT("") );
	PROPERTY( m_cachedNodes );
END_CLASS_RTTI();
