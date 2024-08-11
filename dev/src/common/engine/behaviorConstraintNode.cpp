/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintNode.h"
#include "behaviorConstraintObject.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"
#include "behaviorGraphSocket.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/curve.h"
#include "../engine/animatedComponent.h"
#include "../engine/renderFrame.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "behaviorGraphContext.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNode );
IMPLEMENT_RTTI_ENUM( EBehaviorConstraintDampType );

//////////////////////////////////////////////////////////////////////////

const Float CBehaviorGraphConstraintNode::ACTIVATION_THRESHOLD = 0.01f;

CBehaviorGraphConstraintNode::CBehaviorGraphConstraintNode()
	: m_dampCurve( NULL)
	, m_dampTimeAxisScale( 1.0f )
	, m_followCurve( NULL)
	, m_followTimeAxisScale( 1.0f )
	, m_targetObject( NULL )
	, m_useDampCurve( false )
	, m_dampType( BCDT_Duration )
{

}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintNode::OnSpawned(const GraphBlockSpawnInfo& info)
{
	TBaseClass::OnSpawned( info );

	m_dampCurve = CreateObject< CCurve >( this );
	m_followCurve = CreateObject< CCurve >( this );
}

#endif

void CBehaviorGraphConstraintNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_state;
	compiler << i_progress;
	compiler << i_dampCurveTimer;
	compiler << i_dampTimeAxisScale;
	compiler << i_followCurveTimer;
	compiler << i_followTimeAxisScale;
	compiler << i_controlValue;
	compiler << i_cacheControlValue;
	compiler << i_isLimited;
	compiler << i_endTransform;
	compiler << i_currTransform;
	compiler << i_startTransform;
	compiler << i_posShift;
	compiler << i_distToTarget;
	compiler << i_dampSpeed;
	compiler << i_followSpeed;

	if ( m_targetObject )
	{
		m_targetObject->OnBuildDataLayout( compiler );
	}
}

void CBehaviorGraphConstraintNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_state ] =				CS_Deactivated;
	instance[ i_dampTimeAxisScale ] =	m_dampTimeAxisScale;
	instance[ i_followTimeAxisScale ] = m_followTimeAxisScale;
	instance[ i_controlValue ] =		0.f;
	instance[ i_cacheControlValue ] =	0.f;
	instance[ i_isLimited ] =			false;
	instance[ i_dampSpeed ] =			0.f;
	instance[ i_followSpeed ] =			0.f;

	ResetCurveTimersAndProgress( instance );

	instance[ i_endTransform ].Identity();
	instance[ i_currTransform ].Identity();
	instance[ i_startTransform ].Identity();
	instance[ i_posShift ].Identity();

	if ( m_targetObject )
	{
		m_targetObject->OnInitInstance( instance );
	}
}

void CBehaviorGraphConstraintNode::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_state );
	INST_PROP( i_progress );
	INST_PROP( i_dampCurveTimer );
	INST_PROP( i_dampTimeAxisScale );
	INST_PROP( i_dampSpeed );
	INST_PROP( i_followCurveTimer );
	INST_PROP( i_followTimeAxisScale );
	INST_PROP( i_followSpeed );
	INST_PROP( i_controlValue );
	INST_PROP( i_cacheControlValue );
	INST_PROP( i_isLimited );
	INST_PROP( i_distToTarget );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphConstraintNode::OnPropertyPostChange( IProperty *property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( targetObject ) )
	{
		OnRebuildSockets();
	}
}

void CBehaviorGraphConstraintNode::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );

	CreateSocket( CBehaviorGraphAnimationInputSocketSpawnInfo( CNAME( Input ) ) );		
	CreateSocket( CBehaviorGraphAnimationOutputSocketSpawnInfo( CNAME( Output ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Weight ) ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( Duration ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( FollowDuration ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( SpeedDamp ), false ) );
	CreateSocket( CBehaviorGraphVariableInputSocketSpawnInfo( CNAME( SpeedFollow ), false ) );

	if ( m_targetObject ) 
	{
		m_targetObject->CreateSocketForOwner( this );
	}
}

#endif

void CBehaviorGraphConstraintNode::ActivateConstraint( CBehaviorGraphInstance& instance ) const
{
	// Set new state
	SetState( instance, CS_PrepareToActivating );

	ActivateInputs( instance );

	OnConstraintActivated( instance );
}

void CBehaviorGraphConstraintNode::DeactivateConstraint( CBehaviorGraphInstance& instance ) const
{
	// Set new state
	SetState( instance, CS_PrepareToDeactivating );

	DeactivateInputs( instance );

	OnConstraintDeactivated( instance );
}

void CBehaviorGraphConstraintNode::ActivateInputs( CBehaviorGraphInstance& instance ) const
{
	ASSERT( ShouldUseInputs( instance ) );

	if ( m_cachedDurationValueNode )
	{
		m_cachedDurationValueNode->Activate( instance );	
	}
	if ( m_cachedDurationFollowValueNode )
	{
		m_cachedDurationFollowValueNode->Activate( instance );
	}
	if ( m_cachedSpeedDampValueNode )
	{
		m_cachedSpeedDampValueNode->Activate( instance );
	}
	if ( m_cachedSpeedFollowValueNode )
	{
		m_cachedSpeedFollowValueNode->Activate( instance );
	}
	if ( m_targetObject ) 
	{
		m_targetObject->OnActivated( instance );
	}
}

void CBehaviorGraphConstraintNode::DeactivateInputs( CBehaviorGraphInstance& instance ) const
{
	ASSERT( !ShouldUseInputs( instance ) );

	if ( m_cachedDurationValueNode )
	{
		m_cachedDurationValueNode->Deactivate( instance );	
	}
	if ( m_cachedDurationFollowValueNode )
	{
		m_cachedDurationFollowValueNode->Deactivate( instance );
	}
	if ( m_cachedSpeedDampValueNode )
	{
		m_cachedSpeedDampValueNode->Deactivate( instance );
	}
	if ( m_cachedSpeedFollowValueNode )
	{
		m_cachedSpeedFollowValueNode->Deactivate( instance );
	}
	if ( m_targetObject ) 
	{
		m_targetObject->OnDeactivated( instance );
	}
}

void CBehaviorGraphConstraintNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	// Update input
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Update( context, instance, timeDelta );
	}

	if ( !m_targetObject )
	{
		// No target object, nothing to do...
		return;
	}

	Float newControlValue = 0.0f;

	// Update node's control value
	if ( m_cachedControlValueNode )
	{
		ASSERT( m_cachedControlValueNode->IsActive( instance ) );
		m_cachedControlValueNode->Update( context, instance, timeDelta );
		newControlValue = m_cachedControlValueNode->GetValue( instance );
	}

	// Toggle constraint activation
	if ( newControlValue > ACTIVATION_THRESHOLD && instance[ i_controlValue ] < ACTIVATION_THRESHOLD )
	{
		ActivateConstraint( instance );
	}
	else if ( newControlValue < ACTIVATION_THRESHOLD && instance[ i_controlValue ] > ACTIVATION_THRESHOLD )
	{
		DeactivateConstraint( instance );
	}

	if ( ShouldUseInputs( instance ) )
	{
		// Update duration value - for damp timer
		if ( m_cachedDurationValueNode )
		{
			ASSERT( m_cachedDurationValueNode->IsActive( instance ) );
			m_cachedDurationValueNode->Update( context, instance, timeDelta );
			instance[ i_dampTimeAxisScale ] = m_cachedDurationValueNode->GetValue( instance );
		}

		// Update follow duration value - for follow timer
		if ( m_cachedDurationFollowValueNode )
		{
			ASSERT( m_cachedDurationFollowValueNode->IsActive( instance ) );
			m_cachedDurationFollowValueNode->Update( context, instance, timeDelta );
			instance[ i_followTimeAxisScale ] = m_cachedDurationFollowValueNode->GetValue( instance );
		}

		// Update speeds
		if ( m_cachedSpeedDampValueNode )
		{
			ASSERT( m_cachedSpeedDampValueNode->IsActive( instance ) );
			m_cachedSpeedDampValueNode->Update( context, instance, timeDelta );
			instance[ i_dampSpeed ] = m_cachedSpeedDampValueNode->GetValue( instance );
		}
		if ( m_cachedSpeedFollowValueNode )
		{
			ASSERT( m_cachedSpeedFollowValueNode->IsActive( instance ) );
			m_cachedSpeedFollowValueNode->Update( context, instance, timeDelta );
			instance[ i_followSpeed ] = m_cachedSpeedFollowValueNode->GetValue( instance );
		}
	}

	// Remember new value
	instance[ i_controlValue ] = newControlValue;

	// Constraint state machine
	switch ( instance[ i_state ] )
	{
	case CS_PrepareToActivating:
		UpdateActionPrepareToActivating( instance, timeDelta );
		break;
	case CS_Activating:
		UpdateActionActivating( instance, timeDelta );
		break;
	case CS_Activated:
		UpdateActionActivated( instance, timeDelta );
		break;
	case CS_PrepareToDeactivating:
		UpdateActionPrepareToDeactivating( instance, timeDelta );
		break;
	case CS_Deactivating:
		UpdateActionDeactivating( instance, timeDelta );
		break;
	case CS_Deactivated:
		UpdateActionDeactivated( instance, timeDelta );
		return;
	}

	if ( ShouldUseInputs( instance ) )
	{
		// Update target
		UpdateTarget( context, instance, timeDelta );
	}
}

Bool CBehaviorGraphConstraintNode::IsConstraintActive( CBehaviorGraphInstance& instance ) const
{
	EConstraintState state = (EConstraintState)instance[ i_state ];

	return ( state == CS_Activating || state == CS_Activated || state == CS_Deactivating ) ? true : false;
}
#ifdef USE_HAVOK_ANIMATION
hkQsTransform CBehaviorGraphConstraintNode::GetCurrentConstraintTransform( CBehaviorGraphInstance& instance ) const	
{
	EngineQsTransform mat = GetTargetCurr( instance );
	hkQsTransform transform;

	EngineToHkQsTransform( mat, transform );

	return transform;
}
#else
RedQsTransform CBehaviorGraphConstraintNode::GetCurrentConstraintTransform( CBehaviorGraphInstance& instance ) const
{
	EngineQsTransform mat = GetTargetCurr( instance );
	RedQsTransform transform;
	transform = reinterpret_cast< const RedQsTransform& >( mat );

	return transform;
}
#endif
void CBehaviorGraphConstraintNode::UpdateActionPrepareToActivating( CBehaviorGraphInstance& instance, Float /*dt*/ ) const
{
	// Reset internal timer
	ResetCurveTimersAndProgress( instance );

	// Cache control value
	instance[ i_cacheControlValue ] = instance[ i_controlValue ];
}

void CBehaviorGraphConstraintNode::UpdateActionActivating( CBehaviorGraphInstance& instance, Float dt ) const
{
	Float progress = UpdateDampValue( instance, dt );

	SetProgress( instance, progress );

	// Cache control value
	instance[ i_cacheControlValue ] = instance[ i_controlValue ];
}

void CBehaviorGraphConstraintNode::UpdateActionActivated( CBehaviorGraphInstance& instance, Float dt ) const
{
	Float progress = UpdateFollowValue( instance, dt );

	SetProgress( instance, progress );

	// Cache control value
	instance[ i_cacheControlValue ] = instance[ i_controlValue ];
}

void CBehaviorGraphConstraintNode::UpdateActionPrepareToDeactivating( CBehaviorGraphInstance& instance, Float /*dt*/ ) const
{
	// Reset timer
	ResetCurveTimersAndProgress( instance );
}

void CBehaviorGraphConstraintNode::UpdateActionDeactivating( CBehaviorGraphInstance& instance, Float dt ) const
{
	Float progress = UpdateDampValue( instance, dt );

	SetProgress( instance, progress );
}

void CBehaviorGraphConstraintNode::UpdateActionDeactivated( CBehaviorGraphInstance& /*instance*/, Float /*dt*/ ) const
{
	// Empty
}

void CBehaviorGraphConstraintNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	CHECK_SAMPLE_ID;

	// Sample input
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Sample( context, instance, output );
	}

	// Sample target object
	if ( m_targetObject )
	{
		if ( ShouldUseInputs( instance ) )
		{
			m_targetObject->Sample( output, instance );
		}
	}
	else
	{
		// No target object, nothing to do...
		return;
	}

	// Constraint state machine
	switch ( instance[ i_state ] )
	{
	case CS_PrepareToActivating:
		SampleActionPrepareToActivating( instance, output );
		break;
	case CS_Activating:
		SampleActionActivating( instance, output );
		break;
	case CS_Activated:
		SampleActionActivated( instance, output );
		break;
	case CS_PrepareToDeactivating:
		SampleActionPrepareToDeactivating( instance, output );
		break;
	case CS_Deactivating:
		SampleActionDeactivating( instance, output );
		break;
	case CS_Deactivated:
		SampleActionDeactivated( instance, output );
		break;
	}
}

void CBehaviorGraphConstraintNode::SampleActionPrepareToActivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// Set start transform
	EngineQsTransform startTransform;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform targetFromOutput = CalcTargetFromOutput( instance, output );

	HkToEngineQsTransform( targetFromOutput, startTransform );
#else
	RedQsTransform targetFromOutput = CalcTargetFromOutput( instance, output );
	
	startTransform = reinterpret_cast< const EngineQsTransform& >( targetFromOutput );
#endif
	SetTargetStart( instance, startTransform );

	// Change state
	SetState( instance, CS_Activating );

	// Call ActionActivating - no frame skip
	SampleActionActivating( instance, output );
}

void CBehaviorGraphConstraintNode::SampleActionActivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &/*output*/ ) const
{
	// Update destination, End = Object
	SetTargetEnd( instance, m_targetObject->GetTransform( instance ) );

	const Float progress = instance[ i_progress ];

	if ( progress == 0.0f )
	{
		// Curr = Start
		SetTargetCurr( instance, GetTargetStart( instance ) );
	}
	else if ( progress < 1.0f )
	{
		EngineQsTransform start = GetTargetStart( instance );
		EngineQsTransform end = GetTargetEnd( instance );
		EngineQsTransform curr = GetTargetCurr( instance );

		// Calc position shift
		CalcPosShift( start, end, instance[ i_posShift ] );

		// Calc current transform - apply shift with progress level to start transform
		ApplyShift( start, instance[ i_posShift ], progress, m_dampCurve, curr );

		SetTargetCurr( instance, curr );
	}
	else
	{
		// Curr = End
		SetTargetCurr( instance, GetTargetEnd( instance ) );

		// Set follow timer
		SetFollowTimer( instance );

		// Change state
		SetState( instance, CS_Activated );
	}
}

void CBehaviorGraphConstraintNode::SampleActionActivated( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &/*output*/ ) const
{
	// Check if destination was changed
	EngineQsTransform newDest = m_targetObject->GetTransform( instance );
	EngineQsTransform end = GetTargetEnd( instance );

	static const Float THRESHOLD = 0.0001f;
	if ( !Vector::Near3( end.GetPosition(), newDest.GetPosition(), THRESHOLD ) )
	{
		EngineQsTransform start = GetTargetStart( instance );

		// Destination was changed
		end = newDest;

		// New start = current
		start = GetTargetCurr( instance );

		// Reset timers
		ResetFollowTimer( instance );

		// Reset progress
		SetProgress( instance, 0.f );

		// Calc position shift
		CalcPosShift( start, end, instance[ i_posShift ] );

		SetTargetStart( instance, start );
		SetTargetEnd( instance, end );
	}

	const Float progress = instance[ i_progress ];

	// Update current transform
	if ( progress == 0.0f )
	{
		// Curr = Start
		SetTargetCurr( instance, GetTargetStart( instance ) );
	}
	if ( m_useFollowCurve )
	{
		// Calc current transform - apply shift with progress level to start transform
		EngineQsTransform newCurr;
		ApplyShift( GetTargetStart( instance ), instance[ i_posShift ], progress, m_followCurve, newCurr );
		SetTargetCurr( instance, newCurr );
	}
	else
	{
		// Curr = End
		SetTargetCurr( instance, GetTargetEnd( instance ) );
	}
}

void CBehaviorGraphConstraintNode::SampleActionPrepareToDeactivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// Set start transform, Start = Curr
	SetTargetStart( instance, GetTargetCurr( instance ) );

	// Change state
	SetState( instance, CS_Deactivating );

	// Call SampleActionDeactivating - no frame skip
	SampleActionDeactivating( instance, output );
}

void CBehaviorGraphConstraintNode::SampleActionDeactivating( CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	// Update destination, End = Object
	EngineQsTransform newEnd;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform targetFromOutput = CalcTargetFromOutput( instance, output );
	HkToEngineQsTransform( targetFromOutput, newEnd );
#else
	RedQsTransform targetFromOutput = CalcTargetFromOutput( instance, output );
	newEnd = reinterpret_cast< const EngineQsTransform& >( targetFromOutput );
#endif
	SetTargetEnd( instance, newEnd );

	const Float progess = instance[ i_progress ];

	if ( progess < 1.0f )
	{
		EngineQsTransform start = GetTargetStart( instance );
		EngineQsTransform newCurr;

		// Calc position shift
		CalcPosShift( start, newEnd, instance[ i_posShift ] );

		// Calc current transform - apply shift with progress level to start transform
		ApplyShift( start, instance[ i_posShift ], progess, m_dampCurve, newCurr );

		SetTargetCurr( instance, newCurr );
	}
	else
	{
		// Curr = End
		SetTargetCurr( instance, newEnd );

		// Change state
		SetState( instance, CS_Deactivated );
	}
}

void CBehaviorGraphConstraintNode::SampleActionDeactivated( CBehaviorGraphInstance& /*instance*/, SBehaviorGraphOutput &/*output*/ ) const
{
	// Empty
}

void CBehaviorGraphConstraintNode::CalcPosShift( const EngineQsTransform& start, const EngineQsTransform& end, EngineQsTransform& shiftInOut ) const
{
	shiftInOut.Identity();
	shiftInOut.SetPosition( end.GetPosition() - start.GetPosition() );
}

void CBehaviorGraphConstraintNode::ApplyShift( const EngineQsTransform& start, const EngineQsTransform& shift, Float progress, const CCurve* curve, EngineQsTransform& transformInOut ) const
{
	ASSERT( progress >= 0.f && progress <= 1.f );

	// Reset
	transformInOut = start;
#ifdef USE_HAVOK_ANIMATION
	hkQsTransform transformInOutHk;
	EngineToHkQsTransform( transformInOut, transformInOutHk );
		
	hkQsTransform hkShift;
	EngineToHkQsTransform( shift, hkShift );

	// Get weight from progress
	Float weight = curve->GetFloatValue( progress );

	Int32 weightLooped = (Int32)weight;
	weight -= weightLooped;

	if ( weightLooped > 1 )
	{
		for ( Int32 i=0; i<weightLooped; ++i )
		{
			transformInOutHk.setMul( transformInOutHk, hkShift );
		}
	}
	else if ( weightLooped > 0 )
	{
		transformInOutHk.setMul( transformInOutHk, hkShift );
	}

	if ( weight < 1.f )
	{
		hkShift.setInterpolate4( hkQsTransform::getIdentity(), hkShift, weight );
		transformInOutHk.setMul( transformInOutHk, hkShift );
	}

	HkToEngineQsTransform( transformInOutHk, transformInOut );
#else

	RedQsTransform transformInOutRed;
	transformInOutRed = reinterpret_cast< const RedQsTransform& >( transformInOut );
	
	RedQsTransform redShift;
	redShift = reinterpret_cast< const RedQsTransform& >( shift );

	// Get weight from progress
	Float weight = curve->GetFloatValue( progress );

	Int32 weightLooped = (Int32)weight;
	weight -= weightLooped;

	if ( weightLooped > 1 )
	{
		for ( Int32 i=0; i<weightLooped; ++i )
		{
			transformInOutRed.SetMul( transformInOutRed, redShift );
		}
	}
	else if ( weightLooped > 0 )
	{
		transformInOutRed.SetMul( transformInOutRed, redShift );
	}

	if ( weight < 1.f )
	{
		redShift.Lerp( RedQsTransform::IDENTITY, redShift, weight );
		transformInOutRed.SetMul( transformInOutRed, redShift );
	}
	transformInOut = reinterpret_cast< const EngineQsTransform& >( transformInOutRed );
#endif


	// Rot
	//hkQsTransform tempRot;
	//tempRot.setInterpolate4(hkQsTransform::getIdentity(), m_rotShift, m_dampCurveValue);
	//m_currTarget.setRotation(tempRot.getRotation());
}

void CBehaviorGraphConstraintNode::ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const
{
	TBaseClass::ProcessActivationAlpha( instance, alpha );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->ProcessActivationAlpha( instance, alpha );
	}

	if ( ShouldUseInputs( instance ) )
	{
		if ( m_cachedDurationValueNode )
		{
			m_cachedDurationValueNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_cachedDurationFollowValueNode )
		{
			m_cachedDurationFollowValueNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_cachedSpeedDampValueNode )
		{
			m_cachedSpeedDampValueNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_cachedSpeedFollowValueNode )
		{
			m_cachedSpeedFollowValueNode->ProcessActivationAlpha( instance, alpha );
		}

		if ( m_targetObject )
		{
			m_targetObject->ProcessActivationAlpha( instance, alpha );
		}
	}
}

void CBehaviorGraphConstraintNode::OnReset( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnReset( instance );
}

void CBehaviorGraphConstraintNode::OnActivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnActivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Activate( instance );
	}

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Activate( instance );
	}
}

void CBehaviorGraphConstraintNode::OnDeactivated( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnDeactivated( instance );

	if ( m_cachedInputNode )
	{
		m_cachedInputNode->Deactivate( instance );
	}

	if ( m_cachedControlValueNode )
	{
		m_cachedControlValueNode->Deactivate( instance );
	}

	if ( instance[ i_state ] != CS_Deactivated && instance[ i_state ] != CS_Deactivating )
	{
		DeactivateConstraint( instance );
	}
}

void CBehaviorGraphConstraintNode::GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->GetSyncInfo( instance, info );
	}
}

void CBehaviorGraphConstraintNode::SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const
{
	if ( m_cachedInputNode )
	{
		m_cachedInputNode->SynchronizeTo( instance, info );
	}
}

Bool CBehaviorGraphConstraintNode::ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const
{
	if ( m_cachedInputNode )
	{
		return m_cachedInputNode->ProcessEvent( instance, event );
	}

	return false;
}

void CBehaviorGraphConstraintNode::ResetCurveTimersAndProgress( CBehaviorGraphInstance& instance ) const
{
	ResetFollowTimer( instance );
	ResetDampTimer( instance );

	SetProgress( instance, 0.f );

	SetDistanceToTarget( instance, 0.f );
}

void CBehaviorGraphConstraintNode::ResetFollowTimer( CBehaviorGraphInstance& instance ) const
{
	instance[ i_followCurveTimer ] = 0.0f;
}

void CBehaviorGraphConstraintNode::SetFollowTimer( CBehaviorGraphInstance& instance ) const
{
	if ( m_dampType == BCDT_Duration )
	{
		instance[ i_followCurveTimer ] = m_followTimeAxisScale;
	}
	else
	{
		instance[ i_followCurveTimer ] = 1.0f;
	}
}

void CBehaviorGraphConstraintNode::ResetDampTimer( CBehaviorGraphInstance& instance ) const
{
	instance[ i_dampCurveTimer ] = 0.0f;
}

void CBehaviorGraphConstraintNode::SetDampTimer( CBehaviorGraphInstance& instance ) const
{
	if ( m_dampType == BCDT_Duration )
	{
		instance[ i_dampCurveTimer ] = m_dampTimeAxisScale;
	}
	else
	{
		instance[ i_dampCurveTimer ] = 1.0f;
	}
}

void CBehaviorGraphConstraintNode::SetLimitFlag( CBehaviorGraphInstance& instance, Bool flag ) const
{
	instance[ i_isLimited ] = flag;
}

Bool CBehaviorGraphConstraintNode::GetLimitFlag( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_isLimited ];
}

Uint32 CBehaviorGraphConstraintNode::GetState( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_state ];
}

Bool CBehaviorGraphConstraintNode::ShouldUseInputs( CBehaviorGraphInstance& instance ) const
{
	return GetState( instance ) <= CS_Activated;
}

void CBehaviorGraphConstraintNode::SetTargetStart( CBehaviorGraphInstance& instance, const EngineQsTransform& trans ) const
{
	instance[ i_startTransform ] = trans;
}

void CBehaviorGraphConstraintNode::SetTargetEnd( CBehaviorGraphInstance& instance, const EngineQsTransform& trans ) const
{
	instance[ i_endTransform ] = trans;

	if ( m_dampType == BCDT_Speed )
	{
		Vector vec1 = trans.GetPosition().Normalized3();
		Vector vec2 = GetTargetStart( instance ).GetPosition().Normalized3();

		Float angle = MAcos_safe( vec1.Dot3( vec2 ) );
		SetDistanceToTarget( instance, angle );
	}
}

void CBehaviorGraphConstraintNode::SetTargetCurr( CBehaviorGraphInstance& instance, const EngineQsTransform& trans ) const
{
	instance[ i_currTransform ] = trans;
}

EngineQsTransform CBehaviorGraphConstraintNode::GetTargetStart( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_startTransform ];
}

EngineQsTransform CBehaviorGraphConstraintNode::GetTargetEnd( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_endTransform ];
}

EngineQsTransform CBehaviorGraphConstraintNode::GetTargetCurr( const CBehaviorGraphInstance& instance ) const
{
	return instance[ i_currTransform ];
}

void CBehaviorGraphConstraintNode::SetProgress( CBehaviorGraphInstance& instance, Float progress ) const
{
	instance[ i_progress ] = progress;
}

void CBehaviorGraphConstraintNode::SetDistanceToTarget( CBehaviorGraphInstance& instance, Float dist ) const
{
	instance[ i_distToTarget ] = dist;
}

Float CBehaviorGraphConstraintNode::GetDistanceToTarget( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_distToTarget ];
}

Float CBehaviorGraphConstraintNode::GetDampSpeed( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_dampSpeed ] > 0.f ? instance[ i_dampSpeed ] : m_dampTimeSpeed;
}

Float CBehaviorGraphConstraintNode::GetFollowSpeed( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_followSpeed ] > 0.f ? instance[ i_followSpeed ] : m_followTimeSpeed;
}

Float CBehaviorGraphConstraintNode::UpdateDampValue( CBehaviorGraphInstance& instance, Float dt ) const
{
	if ( m_useDampCurve )
	{
		Float& timer = instance[ i_dampCurveTimer ];

		if ( m_dampType == BCDT_Duration )
		{
			timer += dt;
			return ::Clamp< Float >( timer / m_dampTimeAxisScale, 0.0f, 1.0f );
		}
		else
		{
			Float dist = GetDistanceToTarget( instance );
			if ( dist > 0.f )
			{
				timer = ::Clamp< Float >( timer + ( GetDampSpeed( instance ) * dt ) / dist , 0.0f, 1.0f );
			}
			return timer;
		}
	}
	else
	{
		return 1.0f;
	}
}

Float CBehaviorGraphConstraintNode::UpdateFollowValue( CBehaviorGraphInstance& instance, Float dt ) const
{
	if ( m_useFollowCurve )
	{
		Float& timer = instance[ i_followCurveTimer ];

		if ( m_dampType == BCDT_Duration )
		{
			timer += dt;
			return ::Clamp< Float >( timer / m_followTimeAxisScale, 0.0f, 1.0f );
		}
		else
		{
			Float dist = GetDistanceToTarget( instance );
			if ( dist > 0.f )
			{
				timer = ::Clamp< Float >( timer + ( GetFollowSpeed( instance ) * dt ) / dist , 0.0f, 1.0f );
			}
			return timer;
		}
	}
	else
	{
		return 1.0f;
	}
}

void CBehaviorGraphConstraintNode::UpdateTarget( SBehaviorUpdateContext& context, CBehaviorGraphInstance& instance, Float timeDelta ) const
{
	if ( m_targetObject ) 
	{
		m_targetObject->OnUpdate( context, instance, timeDelta );
	}
}
#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphConstraintNode::TransformToTrajectorySpace( SBehaviorGraphOutput &/*output*/, hkQsTransform& /*transform*/ ) const
{

}
#else
void CBehaviorGraphConstraintNode::TransformToTrajectorySpace( SBehaviorGraphOutput &/*output*/, RedQsTransform& /*transform*/ ) const
{

}
#endif
Float CBehaviorGraphConstraintNode::GetControlValue( CBehaviorGraphInstance& instance ) const 
{ 
	return instance[ i_controlValue ]; 
}

Float CBehaviorGraphConstraintNode::GetProgress( CBehaviorGraphInstance& instance ) const
{
	return instance[ i_progress ];
}

Float CBehaviorGraphConstraintNode::GetSingedProgress( CBehaviorGraphInstance& instance ) const
{
	return GetState( instance ) <= CS_Activated ? instance[ i_progress ] : 1.f - instance[ i_progress ];
}

Bool CBehaviorGraphConstraintNode::IsTargetDamping( CBehaviorGraphInstance& instance ) const
{
	return !IsTagetFollowing( instance );
}

Bool CBehaviorGraphConstraintNode::IsTagetFollowing( CBehaviorGraphInstance& instance ) const
{
	return GetState( instance ) == CS_Activated;
}
#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphConstraintNode::SyncPoseFromOutput( CBehaviorGraphInstance& instance, hkaPose& pose, SBehaviorGraphOutput &output ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();

	if ( ac->GetSkeleton() )
	{
		hkArray<hkQsTransform>& poseLocalTrans = pose.accessUnsyncedPoseLocalSpace();

		Int32 numBones = Min( ac->GetSkeleton()->GetBonesNum(), poseLocalTrans.getSize() );

		for ( Int32 i=0; i<numBones; i++ )
		{
			poseLocalTrans[i] = output.m_outputPose[i];
		}

		pose.syncModelSpace();
	}
}
#else
RED_MESSAGE( "CBehaviorGraphConstraintNode::SyncPoseFromOutput uses hkaPose, which is depricated" )
#endif

void CBehaviorGraphConstraintNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	TBaseClass::OnGenerateFragments( instance, frame );

	if( !m_generateEditorFragments || instance[ i_state ] == CS_Deactivated ) return;

	// Mark target position
	EngineQsTransform tempDestTarget = GetTargetEnd( instance );
	 EngineQsTransform tempCurrTarget = GetTargetCurr( instance );

	Color color = Color::RED;

	if ( instance[ i_state ] == CS_Activated )
	{
		if ( instance[ i_isLimited ] ) color = Color::LIGHT_RED;
		else color = Color::YELLOW;
	}
	else if ( instance[ i_state ] == CS_Activating )
	{
		color = Color::LIGHT_MAGENTA;
	}
	else if ( instance[ i_state ] == CS_Deactivating )
	{
		color = Color::LIGHT_BLUE;
	}

	const Matrix& localToWorld = instance.GetAnimatedComponent()->GetLocalToWorld();

	frame->AddDebugSphere( localToWorld.TransformPoint( tempDestTarget.GetPosition() ), 0.1f, Matrix::IDENTITY, color, false);
	frame->AddDebugSphere( localToWorld.TransformPoint( tempCurrTarget.GetPosition() ), 0.1f, Matrix::IDENTITY, Color::GREEN, false);
}
#ifdef USE_HAVOK_ANIMATION
void CBehaviorGraphConstraintNode::LogPositon( const String& label, hkQsTransform& transform, Bool details ) const
{
	Vector vector = TO_CONST_VECTOR_REF( transform.getTranslation() );
#else
void CBehaviorGraphConstraintNode::LogPositon( const String& label, RedQsTransform& transform, Bool details ) const
{
	Vector vector = reinterpret_cast< const Vector& >( transform.GetTranslation() );
#endif
	if ( details )
	{
		BEH_LOG(TXT("%s %f %f %f"), label.AsChar(), vector.X, vector.Y, vector.Z);
	}
	else
	{
		BEH_LOG(TXT("%s %.2f %.2f %.2f"), label.AsChar(), vector.X, vector.Y, vector.Z);
	}
}

void CBehaviorGraphConstraintNode::LogPositon( const String& label, Vector& vector, Bool details ) const
{
	if (details)
	{
		BEH_LOG(TXT("%s %f %f %f"), label.AsChar(), vector.X, vector.Y, vector.Z);
	}
	else
	{
		BEH_LOG(TXT("%s %.2f %.2f %.2f"), label.AsChar(), vector.X, vector.Y, vector.Z);
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CBehaviorGraphConstraintNode::GetCaption() const
{
	return String( TXT("Constraint") );
}

Color CBehaviorGraphConstraintNode::GetTitleColor() const
{
	return TBaseClass::GetTitleColor();
}

Color CBehaviorGraphConstraintNode::GetColorFromProgress( Float p ) const
{
	// Color activation
	Color cA = TBaseClass::GetTitleColor();

	// Color deactivation
	Color cD = Color( 128, 150, 64 );

	Uint8 r = (Uint8)((1.f - p)*cA.R + p*cD.R);
	Uint8 g = (Uint8)((1.f - p)*cA.G + p*cD.G);
	Uint8 b = (Uint8)((1.f - p)*cA.B + p*cD.B);

	return Color( r, g, b);
}

#endif

void CBehaviorGraphConstraintNode::CacheConnections()
{
	// Pass to base class
	TBaseClass::CacheConnections();

	// Cache constraint settings connection
	if ( m_targetObject )
	{
		m_targetObject->CacheConnections( this );
	}

	// Cache input
	m_cachedInputNode = CacheBlock( TXT("Input") );
	m_cachedControlValueNode = CacheValueBlock( TXT("Weight") );
	m_cachedDurationValueNode = CacheValueBlock( TXT("Duration") );
	m_cachedSpeedDampValueNode = CacheValueBlock( TXT("SpeedDamp") );
	m_cachedSpeedFollowValueNode = CacheValueBlock( TXT("SpeedFollow") );
}

namespace
{
	String GetConstraintStateName( CBehaviorGraphConstraintNode::EConstraintState state )
	{
		switch ( state )
		{
		case CBehaviorGraphConstraintNode::CS_PrepareToActivating:
			return TXT("CS_PrepareToActivating");
		case CBehaviorGraphConstraintNode::CS_Activating:
			return TXT("CS_Activating");
		case CBehaviorGraphConstraintNode::CS_Activated:
			return TXT("CS_Activated");
		case CBehaviorGraphConstraintNode::CS_PrepareToDeactivating:
			return TXT("CS_PrepareToDeactivating");
		case CBehaviorGraphConstraintNode::CS_Deactivating:
			return TXT("CS_Deactivating");
		case CBehaviorGraphConstraintNode::CS_Deactivated:
			return TXT("CS_Deactivated");
		}

		return String::EMPTY;
	}
}

void CBehaviorGraphConstraintNode::SetState( CBehaviorGraphInstance& instance, EConstraintState state ) const
{
	//-----
	// Log
	//String actor;
	//if ( GetGraph()->GetAnimatedComponent() && GetGraph()->GetAnimatedComponent()->GetEntity() )
	//{
	//	actor = GetGraph()->GetAnimatedComponent()->GetEntity()->GetName();
	//}

	//String prevState = GetConstraintStateName( m_state );
	//String newState = GetConstraintStateName( state );
	//LOG_ENGINE( TXT("Constraint %s change state from %s to %s - actor %s"), GetCaption().AsChar(), prevState.AsChar(), newState.AsChar(), actor.AsChar() );
	//-----

	instance[ i_state ] = state;
}