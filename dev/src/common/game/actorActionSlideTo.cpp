/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "actorActionSlideTo.h"

#include "actionAreaComponent.h"
#include "moveAnimationLocomotionSegment.h"
#include "movementTargeter.h"
#include "moveSteeringLocomotionSegment.h"

ActorActionSlideTo::ActorActionSlideTo( CActor* actor, EActorActionType type )
	: ActorAction( actor, type )
	, m_status( ActorActionResult_Succeeded )
{
}

ActorActionSlideTo::~ActorActionSlideTo()
{
}

void ActorActionSlideTo::SetMoveSpeed( CMovingAgentComponent& mac, EMoveType moveType, Float absSpeed ) const
{
	mac.SetMoveType( moveType, absSpeed );
}

Float ActorActionSlideTo::GetSlideDuration( Float dist ) const
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return 0.0f;
	}

	Float speed = mac->GetAbsoluteMoveSpeed();
	if ( speed > 0 )
	{
		return dist / speed;
	}
	else
	{
		return FLT_MAX;
	}
}

Bool ActorActionSlideTo::StartSlide( const Vector& target, Float duration )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	Float dist = m_actor->GetWorldPosition().DistanceTo( target );
	if ( dist < 1e-1 )
	{
		return true;
	}

	// take control over the locomotion
	mac->AttachLocomotionController( *this );

	// Only slide motion
	mac->SetUseExtractedMotion( false );

	if ( CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;
		locomotion().PushSegment( new CMoveLSSlide( target, duration ) );
		return true;
	}
	else
	{
		return false;
	}
}

Bool ActorActionSlideTo::StartSlide( const Vector& target, Float heading, Float duration, ESlideRotation rotation )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	// take control over the locomotion
	mac->AttachLocomotionController( *this );

	// Only slide motion
	mac->SetUseExtractedMotion( false );

	if ( CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;
		locomotion().PushSegment( new CMoveLSSlide( target, duration, &heading, rotation ) );
		return true;
	}
	else
	{
		return false;
	}
}

Bool ActorActionSlideTo::StartCustomMove( IMovementTargeter* targeter )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	// take control over the locomotion
	mac->AttachLocomotionController( *this );

	if ( targeter && CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;

		locomotion().PushSteeringSegment().AddTargeter( targeter );
		return true;
	}
	else
	{
		return false;
	}
}

void ActorActionSlideTo::ResetAgentMovementData()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}
	mac->SetMoveTarget( Vector::ZERO_3D_POINT );
	mac->SetMoveHeading( CEntity::HEADING_ANY );
	mac->ForceSetRelativeMoveSpeed( 0.f );
	mac->SetMoveRotation( 0.f );
	mac->SetMoveRotationSpeed( 0.f );

	mac->SetUseExtractedMotion( true );
};

void ActorActionSlideTo::Stop()
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}
	m_status = ActorActionResult_Failed;
	mac->DetachLocomotionController( *this );
	ResetAgentMovementData();
}

Bool ActorActionSlideTo::Update( Float timeDelta )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac  || m_status == ActorActionResult_InProgress )
	{
		return true;
	}

	ResetAgentMovementData();
	m_actor->ActionEnded( ActorAction_Sliding, m_status );

	if ( CanUseLocomotion() )
	{
		mac->DetachLocomotionController( *this );
	}
	return false;
}

void ActorActionSlideTo::OnSegmentFinished( EMoveStatus status )
{
	if ( status == MS_Completed )
	{
		m_status = ActorActionResult_Succeeded;
	}
	else if ( status == MS_Failed )
	{
		m_status = ActorActionResult_Failed;
	}
}

void ActorActionSlideTo::OnControllerAttached()
{
	// nothing to do here
}

void ActorActionSlideTo::OnControllerDetached()
{
	if ( m_status == ActorActionResult_InProgress )
	{
		m_status = ActorActionResult_Failed;
	}
}

///////////////////////////////////////////////////////////////////////////////
Bool CActor::ActionSlideTo( const Vector& target, Float duration )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionSlideTo.StartSlide( target, duration ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionSlideTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Sliding;

	OnActionStarted( ActorAction_Sliding );

	return true;
}

Bool CActor::ActionSlideTo( const Vector& target, Float heading, Float duration, ESlideRotation rotation )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionSlideTo.StartSlide( target, heading, duration, rotation ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionSlideTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Sliding;

	OnActionStarted( ActorAction_Sliding );

	return true;
}


Bool CActor::ActionMoveCustom( IMovementTargeter* targeter )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move away action
	if ( !targeter || !m_actionSlideTo.StartCustomMove( targeter ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionSlideTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Sliding;

	OnActionStarted( ActorAction_Sliding );

	return true;
}


///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
extern Bool GLatentFunctionStart;

void CActor::funcActionSlideTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, duration, 1.0f );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionSlideTo( target, duration );
		if ( !actionResult )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}


void CActor::funcActionSlideToWithHeading( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( Float, duration, 1.0f );
	GET_PARAMETER_OPT( ESlideRotation, rotation, SR_Nearest );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		Bool actionResult = ActionSlideTo( target, heading, duration, rotation );
		if ( !actionResult )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}

void CActor::funcActionMoveCustom( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CMoveTRGScript >, targeter, NULL );
	FINISH_PARAMETERS;

	// Only in threaded code
	ASSERT( stack.m_thread );

	// Start test
	ACTION_START_TEST;

	// Starting !
	if ( GLatentFunctionStart )
	{
		// Start action
		CMoveTRGScript* pTargeter = targeter.Get();
		Bool actionResult = ActionMoveCustom( pTargeter );
		if ( !actionResult )
		{
			// Already failed
			RETURN_BOOL( false );
			return;
		}

		// Bump the latent stuff
		m_latentActionIndex = stack.m_thread->GenerateLatentIndex();

		// Yield the thread to pause execution
		stack.m_thread->ForceYield();
		return;
	}

	// Action still in progress
	if ( m_latentActionIndex == stack.m_thread->GetLatentIndex() )
	{
		if ( m_latentActionResult == ActorActionResult_InProgress )
		{
			// Yield the thread to pause execution
			stack.m_thread->ForceYield();
			return;
		}
	}

	// Get the state
	const Bool canceled = ( m_latentActionIndex != stack.m_thread->GetLatentIndex() );
	const Bool succeeded = !canceled && ( m_latentActionResult == ActorActionResult_Succeeded );

	// Reset
	if ( ! canceled )
	{
		m_latentActionResult = ActorActionResult_Failed;
		m_latentActionType = ActorAction_None;
		m_latentActionIndex = 0;
	}

	// Return state
	RETURN_BOOL( succeeded );
}

void CActor::funcActionSlideToAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, duration, 1.0f );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionSlideTo( target, duration );
	RETURN_BOOL( actionResult );
}


void CActor::funcActionSlideToWithHeadingAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( Float, duration, 1.0f );
	GET_PARAMETER_OPT( ESlideRotation, rotation, SR_Nearest );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	Bool actionResult = ActionSlideTo( target, heading, duration, rotation );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionMoveCustomAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CMoveTRGScript >, targeter, NULL );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	// Start action
	CMoveTRGScript* pTargeter = targeter.Get();
	Bool actionResult = ActionMoveCustom( pTargeter );
	RETURN_BOOL( actionResult );
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SAnimatedSlideSettings );

ActorActionAnimatedSlideTo::ActorActionAnimatedSlideTo( CActor* actor )
	: ActorAction( actor, ActorAction_Animation )
	, m_status( ActorActionResult_Succeeded )
	, m_proxy( NULL )
{
}

Bool ActorActionAnimatedSlideTo::StartSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target, THandle< CActionMoveAnimationProxy >& proxy )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	mac->AttachLocomotionController( *this );

	if ( CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;
		proxy = CreateObject< CActionMoveAnimationProxy >( m_actor );
		m_proxy = proxy;

		locomotion().PushSegment( new CMoveLSAnimationSlide( settings, target, m_proxy ) );

		return true;
	}
	else
	{
		return false;
	}
}

Bool ActorActionAnimatedSlideTo::StartSlide( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return false;
	}

	ASSERT( !m_proxy.Get() );
	m_proxy = NULL;

	mac->AttachLocomotionController( *this );

	if ( CanUseLocomotion() )
	{
		m_status = ActorActionResult_InProgress;

		locomotion().PushSegment( new CMoveLSAnimationSlide( settings, target ) );

		return true;
	}
	else
	{
		return false;
	}
}

void ActorActionAnimatedSlideTo::OnGCSerialize( IFile& file )
{
	CActionMoveAnimationProxy* p = m_proxy.Get();
	if ( p )
	{
		file << p;
	}
}

void ActorActionAnimatedSlideTo::ResetAgentMovementData()
{
	m_proxy = NULL;

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}

	mac->SetMoveTarget( Vector::ZERO_3D_POINT );
	mac->SetMoveHeading( CEntity::HEADING_ANY );
	mac->ForceSetRelativeMoveSpeed( 0.f );
	mac->SetMoveRotation( 0.f );
	mac->SetMoveRotationSpeed( 0.f );
};

void ActorActionAnimatedSlideTo::Stop()
{
	m_proxy = NULL;
	m_status = ActorActionResult_Failed;

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}

	mac->DetachLocomotionController( *this );

	ResetAgentMovementData();
}

Bool ActorActionAnimatedSlideTo::Update( Float timeDelta )
{
	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac  || m_status == ActorActionResult_InProgress )
	{
		return true;
	}

	ResetAgentMovementData();

	m_actor->ActionEnded( ActorAction_Sliding, m_status );

	if ( CanUseLocomotion() )
	{
		mac->DetachLocomotionController( *this );
	}
	return false;
}

void ActorActionAnimatedSlideTo::OnSegmentFinished( EMoveStatus status )
{
	if ( status == MS_Completed )
	{
		m_status = ActorActionResult_Succeeded;
	}
	else if ( status == MS_Failed )
	{
		m_status = ActorActionResult_Failed;
	}
}

void ActorActionAnimatedSlideTo::OnControllerAttached()
{
	// nothing to do here
}

void ActorActionAnimatedSlideTo::OnControllerDetached()
{
	if ( m_status == ActorActionResult_InProgress )
	{
		m_status = ActorActionResult_Failed;
	}
}

///////////////////////////////////////////////////////////////////////////////

Bool CActor::ActionAnimatedSlideTo( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target, THandle< CActionMoveAnimationProxy >& proxy )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionAnimatedSlideTo.StartSlide( settings, target, proxy ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionAnimatedSlideTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Sliding;

	OnActionStarted( ActorAction_Sliding );

	return true;
}

Bool CActor::ActionAnimatedSlideTo( const SAnimatedSlideSettings& settings, const SAnimSliderTarget& target )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionAnimatedSlideTo.StartSlide( settings, target ) )
	{
		return false;
	}

	// Start action
	m_action = &m_actionAnimatedSlideTo;
	m_latentActionResult = ActorActionResult_InProgress;
	m_latentActionType   = ActorAction_Sliding;

	OnActionStarted( ActorAction_Sliding );

	return true;
}

namespace
{
	struct ProcessActionAnimatedSlideToData : public Red::System::NonCopyable
	{
		CActor*				actor;
		CName				animationName;
		SAnimSliderTarget	target;
		SAnimatedSlideSettings settings;
		THandle< CActionMoveAnimationProxy > proxy;

		CScriptStackFrame&	stack;
		Bool				ret;
		Bool				yield;

		EActorActionType&	latentActionType;
		EActorActionResult& latentActionResult;
		Uint32&				latentActionIndex;

		ProcessActionAnimatedSlideToData( CScriptStackFrame& s, EActorActionType& t, EActorActionResult& r, Uint32& i)
			: stack( s ), latentActionType( t ), latentActionResult( r ), latentActionIndex( i ), ret( true ), yield( false ), proxy( NULL )
		{

		}
	};
	void ProcessActionAnimatedSlideTo( ProcessActionAnimatedSlideToData& data, const ActorActionAnimatedSlideTo& action )
	{
		// Only in threaded code
		ASSERT( data.stack.m_thread );

		// Starting !
		if ( GLatentFunctionStart )
		{
			// Start action
			Bool actionResult = data.actor->ActionAnimatedSlideTo( data.settings, data.target, data.proxy );
			if ( !actionResult )
			{
				// Already failed
				data.ret = false;
				return;
			}

			// Bump the latent stuff
			data.latentActionIndex = data.stack.m_thread->GenerateLatentIndex();

			// Yield the thread to pause execution	
			data.stack.m_thread->ForceYield();
			data.yield = true;
			return;
		}

		// Action still in progress
		if ( data.latentActionIndex == data.stack.m_thread->GetLatentIndex() )
		{
			if ( data.latentActionResult == ActorActionResult_InProgress )
			{
				// Yield the thread to pause execution
				data.stack.m_thread->ForceYield();
				data.yield = true;
				return;
			}
		}

		// Get the state
		const Bool canceled = ( data.latentActionIndex != data.stack.m_thread->GetLatentIndex() );
		const Bool succeeded = !canceled && ( data.latentActionResult == ActorActionResult_Succeeded );

		// Reset
		if ( !canceled )
		{
			data.latentActionResult = ActorActionResult_Failed;
			data.latentActionType = ActorAction_None;
			data.latentActionIndex = 0;
		}

		data.ret = succeeded;
	}
}

void CActor::funcActionAnimatedSlideToStatic( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( Bool, translation, true );
	GET_PARAMETER( Bool, rotation, true );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	ProcessActionAnimatedSlideToData data( stack, m_latentActionType, m_latentActionResult, m_latentActionIndex );

	if ( translation && rotation )
	{
		data.target.Set( target, heading );
	}
	else if ( rotation )
	{
		data.target.Set( heading );
	}
	else
	{
		data.target.Set( target );
	}

	data.actor = this;
	data.settings = settings;
	data.proxy = NULL;

	ProcessActionAnimatedSlideTo( data, m_actionAnimatedSlideTo );

	if ( data.yield )
	{
		return;
	}

	RETURN_BOOL( data.ret );
}

void CActor::funcActionAnimatedSlideTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	GET_PARAMETER( THandle< CNode >, target, NULL );
	GET_PARAMETER( Bool, translation, true );
	GET_PARAMETER( Bool, rotation, true );
	FINISH_PARAMETERS;

	ASSERT( translation || rotation );

	ACTION_START_TEST;

	ProcessActionAnimatedSlideToData data( stack, m_latentActionType, m_latentActionResult, m_latentActionIndex );

	data.target.Set( target.Get(), translation, rotation );
	data.actor = this;
	data.settings = settings;
	data.proxy = NULL;

	ProcessActionAnimatedSlideTo( data, m_actionAnimatedSlideTo );

	if ( data.yield )
	{
		return;
	}

	RETURN_BOOL( data.ret );
}

void CActor::funcActionAnimatedSlideToStaticAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( Bool, translation, true );
	GET_PARAMETER( Bool, rotation, true );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	SAnimSliderTarget sliderTarget;

	if ( translation && rotation )
	{
		sliderTarget.Set( target, heading );
	}
	else if ( rotation )
	{
		sliderTarget.Set( heading );
	}
	else
	{
		sliderTarget.Set( target );
	}

	Bool actionResult = ActionAnimatedSlideTo( settings, sliderTarget );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionAnimatedSlideToStaticAsync_P( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	GET_PARAMETER( Vector, target, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.0f );
	GET_PARAMETER( Bool, translation, true );
	GET_PARAMETER( Bool, rotation, true );
	GET_PARAMETER_REF( THandle< CActionMoveAnimationProxy >, proxyH, THandle< CActionMoveAnimationProxy >() );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	SAnimSliderTarget sliderTarget;

	if ( translation && rotation )
	{
		sliderTarget.Set( target, heading );
	}
	else if ( rotation )
	{
		sliderTarget.Set( heading );
	}
	else
	{
		sliderTarget.Set( target );
	}

	Bool actionResult = ActionAnimatedSlideTo( settings, sliderTarget, proxyH );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionAnimatedSlideToAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	GET_PARAMETER( THandle< CNode >, target, NULL );
	GET_PARAMETER( Bool, translation, true );
	GET_PARAMETER( Bool, rotation, true );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	SAnimSliderTarget sliderTarget;
	sliderTarget.Set( target.Get(), translation, rotation );

	Bool actionResult = ActionAnimatedSlideTo( settings, sliderTarget );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionAnimatedSlideToAsync_P( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimatedSlideSettings, settings, SAnimatedSlideSettings() );
	GET_PARAMETER( THandle< CNode >, target, NULL );
	GET_PARAMETER( Bool, translation, true );
	GET_PARAMETER( Bool, rotation, true );
	GET_PARAMETER_OPT( THandle< CActionMoveAnimationProxy >, proxyH, THandle< CActionMoveAnimationProxy >() );
	FINISH_PARAMETERS;

	// Start test
	ACTION_START_TEST;

	SAnimSliderTarget sliderTarget;
	sliderTarget.Set( target.Get(), translation, rotation );

	Bool actionResult = ActionAnimatedSlideTo( settings, sliderTarget, proxyH );
	RETURN_BOOL( actionResult );
}

//////////////////////////////////////////////////////////////////////////
