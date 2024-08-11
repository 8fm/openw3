
#include "build.h"
#include "actorActionMatchTo.h"
#include "moveMatchToSegment.h"
#include "../../common/engine/behaviorGraphInstance.h"
#include "../../common/engine/behaviorGraphStack.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SActionMatchToTarget );

SActionMatchToTarget::SActionMatchToTarget()
	: m_isTypeStatic( true )
	, m_vec( Vector::ZERO_3D_POINT )
	, m_useRot( true )
	, m_useTrans( true )
{}

void SActionMatchToTarget::Set( const Vector& vec, Float yaw )
{
	m_isTypeStatic = true;
	m_useTrans = true;
	m_useRot = true;

	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Z ) );

	ASSERT( vec.X == vec.X );
	ASSERT( vec.Y == vec.Y );
	ASSERT( vec.Y == vec.Y );

	ASSERT( !Red::Math::NumericalUtils::IsNan( yaw ) );
	ASSERT( yaw == yaw );

	m_vec = vec;
	m_vec.W = DEG2RAD( yaw );

	ASSERT( m_vec.IsOk() );
}

void SActionMatchToTarget::Set( const Vector& vec, Float yaw, Bool trans, Bool rot )
{
	if ( trans && rot )
	{
		Set( vec, yaw );
	}
	else if ( trans )
	{
		Set( vec );
	}
	else if ( rot )
	{
		Set( yaw );
	}
}

void SActionMatchToTarget::Set( const Vector& vec )
{
	m_isTypeStatic = true;
	m_useTrans = true;
	m_useRot = false;

	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( vec.Z ) );

	ASSERT( vec.X == vec.X );
	ASSERT( vec.Y == vec.Y );
	ASSERT( vec.Y == vec.Y );

	m_vec = vec;
	m_vec.W = 0.f;

	ASSERT( m_vec.IsOk() );
}

void SActionMatchToTarget::Set( Float yaw )
{
	m_isTypeStatic = true;
	m_useTrans = false;
	m_useRot = true;

	ASSERT( !Red::Math::NumericalUtils::IsNan( yaw ) );
	ASSERT( yaw == yaw );

	m_vec = Vector::ZEROS;
	m_vec.W = DEG2RAD( yaw );

	ASSERT( m_vec.IsOk() );
}

void SActionMatchToTarget::Set( const CNode* node, Bool trans, Bool rot )
{
	m_isTypeStatic = false;
	m_useTrans = trans;
	m_useRot = rot;

	ASSERT( m_useRot || m_useTrans );

	m_node = node;
}

Bool SActionMatchToTarget::Get( const Matrix& defaultVal, Matrix& mat ) const
{
	if ( m_isTypeStatic )
	{
		if ( m_useRot )
		{
			mat.SetRotZ33( m_vec.W );
		}
		else
		{
			mat = defaultVal;
		}

		if ( m_useTrans )
		{
			mat.SetTranslation( m_vec.X, m_vec.Y, m_vec.Z );
		}
		else
		{
			mat.SetTranslation( defaultVal.GetTranslation() );
		}

		return true;
	}
	else
	{
		const CNode* node = m_node.Get();
		if ( node )
		{
			if ( m_useRot )
			{
				node->GetLocalToWorld( mat );

				if ( !m_useTrans )
				{
					mat.SetTranslation( defaultVal.GetTranslation() );
				}
			}
			else
			{
				mat = defaultVal;

				mat.SetTranslation( node->GetWorldPosition() );
			}

			return true;
		}
	}

	mat = Matrix::IDENTITY;
	return false;
}

Bool SActionMatchToTarget::IsRotationSet() const
{
	return m_useRot;
}

Bool SActionMatchToTarget::IsTranslationSet() const
{
	return m_useTrans;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SActionMatchToSettings );

ActorActionMatchTo::ActorActionMatchTo( CActor* actor )
	: ActorAction( actor, ActorAction_Animation )
	, m_status( ActorActionResult_Succeeded )
	, m_proxy( NULL )
{
}

Bool ActorActionMatchTo::Start( const SActionMatchToSettings& settings, const SActionMatchToTarget& target, THandle< CActionMoveAnimationProxy >& proxy )
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

		locomotion().PushSegment( new CMoveLSMatchTo( settings, target, m_proxy ) );

		return true;
	}
	else
	{
		return false;
	}
}

Bool ActorActionMatchTo::Start( const SActionMatchToSettings& settings, const SActionMatchToTarget& target )
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

		locomotion().PushSegment( new CMoveLSMatchTo( settings, target ) );

		return true;
	}
	else
	{
		return false;
	}
}

void ActorActionMatchTo::OnGCSerialize( IFile& file )
{
	CActionMoveAnimationProxy* p = m_proxy.Get();
	if ( p )
	{
		file << p;
	}
}

void ActorActionMatchTo::ResetAgentMovementData()
{
	m_proxy = NULL;

	CMovingAgentComponent* mac = m_actor->GetMovingAgentComponent();
	if ( !mac )
	{
		return;
	}

	mac->SetMoveTarget( Vector::ZERO_3D_POINT );
	mac->SetMoveHeading( CEntity::HEADING_ANY );
	mac->ForceSetRelativeMoveSpeed(  0.f );

	mac->SetMoveRotation( 0.f );
	mac->SetMoveRotationSpeed( 0.f );
};

void ActorActionMatchTo::Stop()
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

Bool ActorActionMatchTo::Update( Float timeDelta )
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

void ActorActionMatchTo::OnSegmentFinished( EMoveStatus status )
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

void ActorActionMatchTo::OnControllerAttached()
{
	// nothing to do here
}

void ActorActionMatchTo::OnControllerDetached()
{
	if ( m_status == ActorActionResult_InProgress )
	{
		m_status = ActorActionResult_Failed;
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CActor::ActionMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target, THandle< CActionMoveAnimationProxy >& proxy )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionMatchTo.Start( settings, target, proxy ) )
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

Bool CActor::ActionMatchTo( const SActionMatchToSettings& settings, const SActionMatchToTarget& target )
{
	// Check if current action may be canceled by other actions
	if ( m_action && ! m_action->IsCancelingAllowed() )
	{
		return false;
	}

	// Cancel actions
	ActionCancelAll();

	// Start move to action
	if ( !m_actionMatchTo.Start( settings, target ) )
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

//////////////////////////////////////////////////////////////////////////

extern Bool GLatentFunctionStart;

namespace
{
	struct ProcessActionMatchToData : public Red::System::NonCopyable
	{
		CActor*					actor;
		CName					animationName;
		SActionMatchToTarget	target;
		SActionMatchToSettings	settings;
		THandle< CActionMoveAnimationProxy > proxy;

		CScriptStackFrame&		stack;
		Bool					ret;
		Bool					yield;

		EActorActionType&		latentActionType;
		EActorActionResult&		latentActionResult;
		Uint32&					latentActionIndex;

		ProcessActionMatchToData( CScriptStackFrame& s, EActorActionType& t, EActorActionResult& r, Uint32& i)
			: stack( s ), latentActionType( t ), latentActionResult( r ), latentActionIndex( i ), ret( true ), yield( false ), proxy( NULL )
		{

		}
	};
	void ProcessActionMatchTo( ProcessActionMatchToData& data, const ActorActionMatchTo& action )
	{
		// Only in threaded code
		ASSERT( data.stack.m_thread );

		// Starting !
		if ( GLatentFunctionStart )
		{
			// Start action
			Bool actionResult = data.actor->ActionMatchTo( data.settings, data.target, data.proxy );
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

void CActor::funcActionMatchTo( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SActionMatchToSettings, settings, SActionMatchToSettings() );
	GET_PARAMETER( SActionMatchToTarget, target, SActionMatchToTarget() );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	/*ProcessActionMatchToData data( stack, m_latentActionType, m_latentActionResult, m_latentActionIndex );

	data.target.Set( target.Get(), translation, rotation );
	data.actor = this;
	data.settings = settings;
	data.proxy = NULL;

	ProcessActionMatchTo( data, m_actionMatchTo );

	if ( data.yield )
	{
		return;
	}

	RETURN_BOOL( data.ret );*/
	RETURN_BOOL( false );
}

void CActor::funcActionMatchToAsync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SActionMatchToSettings, settings, SActionMatchToSettings() );
	GET_PARAMETER( SActionMatchToTarget, target, SActionMatchToTarget() );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	Bool actionResult = ActionMatchTo( settings, target );
	RETURN_BOOL( actionResult );
}

void CActor::funcActionMatchToAsync_P( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SActionMatchToSettings, settings, SActionMatchToSettings() );
	GET_PARAMETER( SActionMatchToTarget, target, SActionMatchToTarget() );
	GET_PARAMETER_OPT( THandle< CActionMoveAnimationProxy >, proxyH, THandle< CActionMoveAnimationProxy >() );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	Bool actionResult = ActionMatchTo( settings, target, proxyH );
	RETURN_BOOL( actionResult );
}

//////////////////////////////////////////////////////////////////////////
