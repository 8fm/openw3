/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "behaviorGraphGameplayTransitions.h"
#include "../engine/behaviorGraphInstance.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/behaviorProfiler.h"
#include "movementAdjustor.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CChangeMovementDirectionTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CChangeFacingDirectionTransitionCondition );
IMPLEMENT_ENGINE_CLASS( CIsMovingForwardTransitionCondition );
IMPLEMENT_RTTI_ENUM( EChangeFacingDirectionSide );
IMPLEMENT_ENGINE_CLASS( CBehaviorGraphStateTransitionFinalStepNode );

///////////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( FinalStepTransition );
RED_DEFINE_STATIC_NAME( MovementAdjustmentLocation );
RED_DEFINE_STATIC_NAME( MovementAdjustmentActive )
RED_DEFINE_STATIC_NAME( MovementDirectionFromDirectionalMovement  )
RED_DEFINE_STATIC_NAME( requestedMovementDirection )
RED_DEFINE_STATIC_NAME( requestedFacingDirection )

///////////////////////////////////////////////////////////////////////////////

CChangeMovementDirectionTransitionCondition::CChangeMovementDirectionTransitionCondition()
	: m_requestedMovementDirectionWSVariableName( CNAME( requestedMovementDirection ) )
	, m_currentMovementDirectionMSInternalVariableName( CNAME( MovementDirectionFromDirectionalMovement ) )
	, m_angleDiffThreshold( 120.0f )
	, m_startCheckingAfterTime( 0.0f )
{
}

void CChangeMovementDirectionTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeActive;
	compiler << i_requiresChange;
	compiler << i_requiresUpdate;
}

void CChangeMovementDirectionTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_requiresUpdate ] = true;
}

Bool CChangeMovementDirectionTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_requiresUpdate ] )
	{
		UpdateRequiresChange( instance );
	}
	return instance[ i_requiresChange ];
}

void CChangeMovementDirectionTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	BEH_PROFILER_LEVEL_3( TransReset_ChangeMovement );
	TBaseClass::Reset( instance );
	instance[ i_requiresUpdate ] = true;
}

void CChangeMovementDirectionTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnStartBlockActivated( instance );

	instance[ i_timeActive ] = 0.0f;
	instance[ i_requiresUpdate ] = true;
}

void CChangeMovementDirectionTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnStartBlockUpdate( context, instance, timeDelta );

	Float& timeActive = instance[ i_timeActive ];
	
	timeActive += timeDelta;

	instance[ i_requiresUpdate ] = true;
}

void CChangeMovementDirectionTransitionCondition::UpdateRequiresChange( CBehaviorGraphInstance& instance ) const
{
	instance[ i_requiresUpdate ] = false;

	Float& timeActive = instance[ i_timeActive ];
	Bool& requiresChange = instance[ i_requiresChange ];
	requiresChange = false;

	if ( timeActive >= m_startCheckingAfterTime )
	{
		const Float requestedMovementDirectionWSYaw = instance.GetFloatValue( m_requestedMovementDirectionWSVariableName );
		const Float currentMovementDirectionWSYaw =  instance.GetInternalFloatValue( m_currentMovementDirectionMSInternalVariableName ) + instance.GetAnimatedComponent()->GetWorldYaw();
		requiresChange = Abs( EulerAngles::NormalizeAngle180( currentMovementDirectionWSYaw - requestedMovementDirectionWSYaw ) ) > m_angleDiffThreshold;
	}
}

void CChangeMovementDirectionTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	if (transition->IsA<CChangeMovementDirectionTransitionCondition>())
	{
		const CChangeMovementDirectionTransitionCondition* templateCondition = static_cast<const CChangeMovementDirectionTransitionCondition*>(transition);
		m_requestedMovementDirectionWSVariableName			= templateCondition->m_requestedMovementDirectionWSVariableName;
		m_currentMovementDirectionMSInternalVariableName	= templateCondition->m_currentMovementDirectionMSInternalVariableName;
		m_angleDiffThreshold								= templateCondition->m_angleDiffThreshold;
		m_startCheckingAfterTime							= templateCondition->m_startCheckingAfterTime;
	}
}

void CChangeMovementDirectionTransitionCondition::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	TBaseClass::GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
	var.PushBack( m_requestedMovementDirectionWSVariableName );
	intVar.PushBack( m_currentMovementDirectionMSInternalVariableName );
}

///////////////////////////////////////////////////////////////////////////////

CChangeFacingDirectionTransitionCondition::CChangeFacingDirectionTransitionCondition()
	: m_side( CFDS_Left )
	, m_requestedFacingDirectionWSVariableName( CNAME( requestedFacingDirection ) )
	, m_angleDiffMin( 70.0f )
	, m_angleDiffMax( 180.0f )
	, m_startCheckingAfterTime( 0.0f )
{
}

void CChangeFacingDirectionTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeActive;
	compiler << i_requiresChange;
	compiler << i_requiresUpdate;
}

void CChangeFacingDirectionTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	String caption = m_dontChange? TXT("Don't change facing dir") : TXT("Change facing dir");
	caption += String::Printf( TXT(" by %.0f'-%.0f'"), m_angleDiffMin, m_angleDiffMax );
	if ( m_side == CFDS_Left )
	{
		caption += String::Printf( TXT(" left") );
	}
	if ( m_side == CFDS_Right )
	{
		caption += String::Printf( TXT(" right") );
	}
	if ( m_side == CFDS_Any )
	{
		caption += String::Printf( TXT(" both") );
	}
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + caption );
}

void CChangeFacingDirectionTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_requiresUpdate ] = true;
}

Bool CChangeFacingDirectionTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_requiresUpdate ] )
	{
		UpdateRequiresChange( instance );
	}
	return instance[ i_requiresChange ];
}

void CChangeFacingDirectionTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	BEH_PROFILER_LEVEL_3( TransReset_ChangeFacing );
	TBaseClass::Reset( instance );
	instance[ i_requiresUpdate ] = true;
}

void CChangeFacingDirectionTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnStartBlockActivated( instance );

	instance[ i_timeActive ] = 0.0f;
	instance[ i_requiresUpdate ] = true;
}

void CChangeFacingDirectionTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnStartBlockUpdate( context, instance, timeDelta );

	Float& timeActive = instance[ i_timeActive ];

	timeActive += timeDelta;

	instance[ i_requiresUpdate ] = true;
}

void CChangeFacingDirectionTransitionCondition::UpdateRequiresChange( CBehaviorGraphInstance& instance ) const
{
	instance[ i_requiresUpdate ] = false;

	Float& timeActive = instance[ i_timeActive ];
	Bool& requiresChange = instance[ i_requiresChange ];
	requiresChange = false;

	if ( timeActive >= m_startCheckingAfterTime )
	{
		const Float requestedFacingDirectionWSYaw = instance.GetFloatValue( m_requestedFacingDirectionWSVariableName );
		const Float entityWSYaw = instance.GetAnimatedComponent()->GetWorldYaw();
		Float difference = EulerAngles::NormalizeAngle180( requestedFacingDirectionWSYaw - entityWSYaw );
		if ( m_side == CFDS_Right ) { difference = -difference; }
		if ( m_side == CFDS_None || m_side == CFDS_Any ) { difference = Abs( difference ); }
		requiresChange = difference >= 0.0f && difference >= m_angleDiffMin && ( difference < m_angleDiffMax || m_angleDiffMax <= 0.0f );
		if ( m_dontChange )
		{
			requiresChange = ! requiresChange;
		}
	}
}

void CChangeFacingDirectionTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	if (transition->IsA<CChangeFacingDirectionTransitionCondition>())
	{
		const CChangeFacingDirectionTransitionCondition* templateCondition = static_cast<const CChangeFacingDirectionTransitionCondition*>(transition);
		m_requestedFacingDirectionWSVariableName	= templateCondition->m_requestedFacingDirectionWSVariableName;
		m_angleDiffMin								= templateCondition->m_angleDiffMin;
		m_angleDiffMax								= templateCondition->m_angleDiffMax;
		m_startCheckingAfterTime					= templateCondition->m_startCheckingAfterTime;
	}
}

void CChangeFacingDirectionTransitionCondition::GetUsedVariablesAndEvents( TDynArray<CName>& var, TDynArray<CName>& vecVar, TDynArray<CName>& events, TDynArray<CName>& intVar, TDynArray<CName>& intVecVar ) const
{
	TBaseClass::GetUsedVariablesAndEvents( var, vecVar, events, intVar, intVecVar );
	var.PushBack( m_requestedFacingDirectionWSVariableName );
}

///////////////////////////////////////////////////////////////////////////////

CIsMovingForwardTransitionCondition::CIsMovingForwardTransitionCondition()
	: m_maxOffAngle( 30.0f )
	, m_notMovingForward( false )
{
}

void CIsMovingForwardTransitionCondition::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_movingForward;
	compiler << i_requiresUpdate;
}

void CIsMovingForwardTransitionCondition::GetCaption( TDynArray< String >& captions, Bool getCaptionTests, CBehaviorGraphInstance* instance ) const
{
	String caption;
	caption = m_notMovingForward? TXT("Not moving forward") : TXT("Moving forward");
	caption += String::Printf( TXT("  %.0f'"), m_maxOffAngle );
	captions.PushBack( ( getCaptionTests ? GetCaptionTest( instance ) : TXT("") ) + caption );
}

void CIsMovingForwardTransitionCondition::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_movingForward ] = false;
	instance[ i_requiresUpdate ] = true;
}

Bool CIsMovingForwardTransitionCondition::Check( CBehaviorGraphInstance& instance ) const
{
	if ( instance[ i_requiresUpdate ] )
	{
		UpdateMovingForward( instance );
	}
	return instance[ i_movingForward ];
}

void CIsMovingForwardTransitionCondition::Reset( CBehaviorGraphInstance& instance ) const
{
	BEH_PROFILER_LEVEL_3( TransReset_IsMovingForward );
	TBaseClass::Reset( instance );
	instance[ i_requiresUpdate ] = true;
}

void CIsMovingForwardTransitionCondition::OnStartBlockActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnStartBlockActivated( instance );

	instance[ i_requiresUpdate ] = true;
}

void CIsMovingForwardTransitionCondition::OnStartBlockUpdate( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	TBaseClass::OnStartBlockUpdate( context, instance, timeDelta );

	instance[ i_requiresUpdate ] = true;
}

void CIsMovingForwardTransitionCondition::UpdateMovingForward( CBehaviorGraphInstance& instance ) const
{
	instance[ i_requiresUpdate ] = false;

	Bool& movingForward = instance[ i_movingForward ];
	movingForward = false;

	if ( CMovingAgentComponent const * mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponent() ) )
	{
		const Float movingInDirWSYaw = EulerAngles::YawFromXY( mac->GetVelocity().X, mac->GetVelocity().Y );
		const Float entityWSYaw = mac->GetWorldYaw();
		Float difference = Abs( EulerAngles::NormalizeAngle180( movingInDirWSYaw - entityWSYaw ) );
		movingForward = difference <= m_maxOffAngle;
		if ( m_notMovingForward )
		{
			movingForward = ! movingForward;
		}
	}
}

void CIsMovingForwardTransitionCondition::CopyDataFrom(const IBehaviorStateTransitionCondition* transition)
{
	if (transition->IsA<CIsMovingForwardTransitionCondition>())
	{
		const CIsMovingForwardTransitionCondition* templateCondition = static_cast<const CIsMovingForwardTransitionCondition*>(transition);
		m_maxOffAngle = templateCondition->m_maxOffAngle;
	}
}

///////////////////////////////////////////////////////////////////////////////

CBehaviorGraphStateTransitionFinalStepNode::CBehaviorGraphStateTransitionFinalStepNode()
	: m_locationAdjustmentVar( CNAME( MovementAdjustmentLocation ) )
	, m_adjustmentActiveVar( CNAME( MovementAdjustmentActive ) )
{
}

String CBehaviorGraphStateTransitionFinalStepNode::GetCaption() const
{
	return String(TXT("Final step transition"));
}

void CBehaviorGraphStateTransitionFinalStepNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
	{
		if ( const Float* actVarPtr = instance.GetFloatValuePtr( m_adjustmentActiveVar ) )
		{
			if ( *actVarPtr > 0.0f )
			{
				if ( const Vector* varPtr = instance.GetVectorValuePtr( m_locationAdjustmentVar ) )
				{
					CMovementAdjustor* ma = mac->GetMovementAdjustor();
					SMovementAdjustmentRequest* request = ma->CreateNewRequest( CNAME( FinalStepTransition ) );
					// as simple as that
					request->SlideTo( *varPtr );
					request->AdjustmentDuration( m_transitionTime );
				}
			}
		}
	}
}

void CBehaviorGraphStateTransitionFinalStepNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( CMovingAgentComponent* mac = Cast< CMovingAgentComponent >( instance.GetAnimatedComponentUnsafe() ) )
	{
		CMovementAdjustor* ma = mac->GetMovementAdjustor();
		ma->CancelByName( CNAME( FinalStepTransition ) );
	}
}

void CBehaviorGraphStateTransitionFinalStepNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	output.m_deltaReferenceFrameLocal = AnimQsTransform::IDENTITY;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
