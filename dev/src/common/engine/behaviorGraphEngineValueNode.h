/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphVariableNode.h"
#include "behaviorGraphVectorVariableNode.h"

/// Engine value types
enum EBehaviorEngineValueType
{
	BEVT_TimeDelta,					//!< Time delta
	BEVT_ActorToCameraAngle,		//!< Angle between actor direction and camera direction
	BEVT_ActorSpeed,				//!< Actors speed
	BEVT_ActorMoveDirection,		//!< Actors move direction
	BEVT_ActorHeading,				//!< Actors speed
	BEVT_ActorRotationSpeed,		//!< Actors rotation speed
	BEVT_ActorRelativeDirection,	//!< Actor relative direction
	BEVT_ActorRotation,				//!< Actors rotation
	BEVT_ActorRawDesiredRotation,	//!< Actors raw desired rotation not blended anyhow
	BEVT_CameraFollowAngle,			//!< Angle between camera follow target and camera direction
	BEVT_ActorLookAtLevel,			//!< Actors look at level
	BEVT_ActorLookAtEnabled,		//!< Actors look at enabled
	BEVT_ActorEyesLookAtConvergenceWeight,
	BEVT_Pad,						//!< Game uses pad inputs
	BEVT_ActorAnimState,			//!< Actor's animation state
	BEVT_ActorMoveDirToFacingDiff,  //!< angular distance between actor's movement direction and facing.
	BEVT_AnimationMultiplier,		//!< Animation multiplier
	BEVT_IsActorInScene,			//!< Is actor in not gameplay scene
	BEVT_CurrentBehaviorGraphInstanceTimeActive,	//!< How long current behavior graph instance is active
};

BEGIN_ENUM_RTTI( EBehaviorEngineValueType );
	ENUM_OPTION( BEVT_TimeDelta );
	ENUM_OPTION( BEVT_ActorToCameraAngle );
	ENUM_OPTION( BEVT_ActorSpeed );
	ENUM_OPTION( BEVT_ActorMoveDirection );
	ENUM_OPTION( BEVT_ActorHeading );
	ENUM_OPTION( BEVT_ActorRotationSpeed );
	ENUM_OPTION( BEVT_ActorRelativeDirection );
	ENUM_OPTION( BEVT_ActorRotation );
	ENUM_OPTION( BEVT_ActorRawDesiredRotation );
	ENUM_OPTION( BEVT_CameraFollowAngle );
	ENUM_OPTION( BEVT_ActorLookAtLevel );
	ENUM_OPTION( BEVT_ActorLookAtEnabled );
	ENUM_OPTION( BEVT_Pad );
	ENUM_OPTION( BEVT_ActorAnimState );
	ENUM_OPTION( BEVT_ActorMoveDirToFacingDiff );
	ENUM_OPTION( BEVT_AnimationMultiplier );
	ENUM_OPTION( BEVT_IsActorInScene );
	ENUM_OPTION( BEVT_CurrentBehaviorGraphInstanceTimeActive );
END_ENUM_RTTI()

/// Engine vector value types
enum EBehaviorEngineVectorValueType
{
	BEVVT_ActorLookAtTarget,
	BEVVT_ActorLookAtCompressedData,
	BEVVT_ActorLookAtBodyPartWeights,
	BEVVT_ActorEyesLookAtData,
};

BEGIN_ENUM_RTTI( EBehaviorEngineVectorValueType );
	ENUM_OPTION( BEVVT_ActorLookAtTarget );
	ENUM_OPTION( BEVVT_ActorLookAtCompressedData );
	ENUM_OPTION( BEVVT_ActorLookAtBodyPartWeights );
	ENUM_OPTION( BEVVT_ActorEyesLookAtData );
END_ENUM_RTTI()

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphEngineValueNode : public  CBehaviorGraphVariableNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphEngineValueNode, CBehaviorGraphVariableNode, "Float", "Engine value" );		

protected:
	Bool						m_manualControl;
	EBehaviorEngineValueType	m_engineValueType;

protected:
	TInstanceVar< Float >		i_value;
	TInstanceVar< Uint64 >		i_updateID;

public:
	CBehaviorGraphEngineValueNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	Bool IsManuallyControlled() const { return m_manualControl; }
	void SetManualControl( Bool state ) { m_manualControl = state; }

	String GetType() const;
	virtual String GetCaption() const;

public:
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

protected:
	Float GetValueInternal( CBehaviorGraphInstance& instance, Float timeDelta ) const;

	Float GetActorToCameraAngle( CBehaviorGraphInstance& instance ) const;
	Float GetRelativeMoveSpeed( CBehaviorGraphInstance& instance ) const;
	Float GetActorMoveDirection( CBehaviorGraphInstance& instance ) const;
	Float GetActorHeading( CBehaviorGraphInstance& instance ) const;
	Float GetActorRotationSpeed( CBehaviorGraphInstance& instance ) const;
	Float GetRelativeDirection( CBehaviorGraphInstance& instance ) const;
	Float GetActorMoveDirToFacingDiff( CBehaviorGraphInstance& instance ) const;
	Float GetActorRotation( CBehaviorGraphInstance& instance ) const;
	Float GetActorRawDesiredRotation( CBehaviorGraphInstance& instance ) const;
	Float GetCameraFollowAngle( CBehaviorGraphInstance& instance ) const;
	Float GetActorLookAtLevel( CBehaviorGraphInstance& instance ) const;
	Float GetActorLookAtEnabled( CBehaviorGraphInstance& instance ) const;
	Float GetPad( CBehaviorGraphInstance& instance ) const;
	Float GetActorAnimState( CBehaviorGraphInstance& instance ) const;
	Float GetAnimationMultiplier( CBehaviorGraphInstance& instance ) const;
	Float IsActorInScene( CBehaviorGraphInstance& instance ) const;
	Float GetCurrentBehaviorGraphInstanceTimeActive( CBehaviorGraphInstance& instance ) const;

private:
	RED_INLINE Float ToBehaviorAngle( Float engineAngle ) const { return -engineAngle / 180.0f; }
};

BEGIN_CLASS_RTTI( CBehaviorGraphEngineValueNode );
	PARENT_CLASS(  CBehaviorGraphVariableNode );
	PROPERTY_EDIT( m_engineValueType, TXT("Engine value type to get") );
	PROPERTY_EDIT( m_manualControl, TXT("Is active") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphEngineVectorValueNode : public  CBehaviorGraphVectorVariableNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphEngineVectorValueNode, CBehaviorGraphVectorVariableNode, "Vector", "Engine vector value" );		

protected:
	Bool							m_manualControl;
	EBehaviorEngineVectorValueType	m_engineValueType;
	CAnimatedComponent*				m_animatedComponent;

protected:
	CBehaviorVectorVariable*		m_cachedVectorVariable;

public:
	CBehaviorGraphEngineVectorValueNode();

	Bool IsManuallyControlled() const { return m_manualControl; }

	void SetManualControl( Bool state ) { m_manualControl = state; }

public:
	virtual String GetCaption() const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

protected:
	Vector GetValueInternal( CBehaviorGraphInstance& instance ) const;
	Vector GetLookAtTarget( CBehaviorGraphInstance& instance ) const;
	Vector GetLookAtCompressedData( CBehaviorGraphInstance& instance ) const;
	Vector GetLookAtBodyPartWeights( CBehaviorGraphInstance& instance ) const;
	Vector GetEyesLookAtData( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphEngineVectorValueNode );
	PARENT_CLASS(  CBehaviorGraphVectorVariableNode );
	PROPERTY_EDIT( m_engineValueType, TXT("Engine vector value type to get") );
	PROPERTY_EDIT( m_manualControl, TXT("Is active") );
	PROPERTY( m_cachedVectorVariable );
END_CLASS_RTTI();
