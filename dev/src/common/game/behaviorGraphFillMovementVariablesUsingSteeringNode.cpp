/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphFillMovementVariablesUsingSteeringNode.h"
#include "../engine/behaviorGraphBlendNode.h"
#include "../engine/behaviorGraphInstance.h"
#include "../engine/cacheBehaviorGraphOutput.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../core/instanceDataLayoutCompiler.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphFillMovementVariablesUsingSteeringNode );


RED_DEFINE_STATIC_NAME( requestedMovementDirection );
RED_DEFINE_STATIC_NAME( requestedFacingDirection );

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphFillMovementVariablesUsingSteeringNode::CBehaviorGraphFillMovementVariablesUsingSteeringNode()
	: m_fillRequestedMovementDirectionWSVariable( true )
	, m_requestedMovementDirectionWSVariableName( CNAME( requestedMovementDirection ) )
	, m_fillRequestedFacingDirectionWSVariable( true )
	, m_requestedFacingDirectionWSVariableName( CNAME( requestedFacingDirection ) )
{
}

void CBehaviorGraphFillMovementVariablesUsingSteeringNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );

	FillVariables( instance );
}

void CBehaviorGraphFillMovementVariablesUsingSteeringNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	FillVariables( instance );
}

void CBehaviorGraphFillMovementVariablesUsingSteeringNode::FillVariables( CBehaviorGraphInstance& instance ) const
{
	if ( CMovingAgentComponent const * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponent() ) )
	{
		if ( m_fillRequestedMovementDirectionWSVariable && instance.HasFloatValue( m_requestedMovementDirectionWSVariableName ) )
		{
			Vector2 headingVec = mac->GetSteeringVelocity();
			Float headingYaw = EulerAngles::YawFromXY( headingVec.X, headingVec.Y );
			instance.SetFloatValue( m_requestedMovementDirectionWSVariableName, EulerAngles::NormalizeAngle( headingYaw ) );
		}
		if ( m_fillRequestedFacingDirectionWSVariable && instance.HasFloatValue( m_requestedFacingDirectionWSVariableName ) )
		{
			Float currentYaw = mac->GetWorldYaw();
			Float facingYaw = currentYaw - mac->GetMoveRawDesiredRotationWorldSpace(); // although it is named world space, it ain't world space, but relative value and it is reversed (left -180, right 180)
			instance.SetFloatValue( m_requestedFacingDirectionWSVariableName, EulerAngles::NormalizeAngle( facingYaw ) );
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
