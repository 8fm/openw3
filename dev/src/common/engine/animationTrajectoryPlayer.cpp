
#include "build.h"
#include "animationTrajectoryPlayer.h"
#include "animationTrajectory.h"
#include "animationGameParams.h"
#include "animationTrajectoryPlayer_State.h"
#include "animationTrajectoryPlayer_Trajectory.h"
#include "animationTrajectoryPlayer_Blend2.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../core/scriptStackFrame.h"
#include "entity.h"
#include "actorInterface.h"


IMPLEMENT_RTTI_ENUM( EAnimationTrajectorySelectorType );
IMPLEMENT_ENGINE_CLASS( SAnimationTrajectoryPlayerToken );
IMPLEMENT_ENGINE_CLASS( SAnimationTrajectoryPlayerInput );

AnimationTrajectoryPlayer::AnimationTrajectoryPlayer()
	: m_component( NULL )
{
	m_slotName = CNAME( GAMEPLAY_SLOT );
}

AnimationTrajectoryPlayer::~AnimationTrajectoryPlayer()
{
	Deinit();
}

Bool AnimationTrajectoryPlayer::Init( const CEntity* entity, const CName& slotName )
{
	ASSERT( !m_currAnimation );
	ASSERT( !m_nextAnimation );
	ASSERT( !m_component.Get() );

	CAnimatedComponent* ac = entity->GetRootAnimatedComponent();

	if ( slotName != CName::NONE )
	{
		m_slotName = slotName;
	}

	if ( ac && ac->GetAnimationContainer() && SetupSlot( ac ) )
	{
		m_component = ac;
		return true;
	}

	

	return false;
}

void AnimationTrajectoryPlayer::Deinit()
{
	if ( m_component.Get() )
	{
		CloseSlot();
	}

	ClearState( m_currAnimation );
	ClearState( m_nextAnimation );

	m_component = NULL;
}

void AnimationTrajectoryPlayer::Tick( Float dt )
{
	if ( !m_currAnimation && m_nextAnimation )
	{
		CopyState( m_nextAnimation, m_currAnimation );

		ClearState( m_nextAnimation );

		ASSERT( !m_nextAnimation );
	}

	CAnimatedComponent* ac = m_component.Get();
	if ( m_currAnimation && ac )
	{
		const Bool finish = m_currAnimation->Update( dt );

		const Bool isAnimationPlaying = m_currAnimation->PlayAnimationOnSlot( m_slot );

		if ( !isAnimationPlaying || finish )
		{
			CloseSlot();

			ClearState( m_currAnimation );
		}
	}
}

Float AnimationTrajectoryPlayer::GetTime() const
{
	return m_currAnimation ? m_currAnimation->GetTime() : 1.f;
}

void AnimationTrajectoryPlayer::ClearState( AnimationTrajectoryPlayer_State*& state )
{
	delete state;
	state = NULL;
}

void AnimationTrajectoryPlayer::CopyState( AnimationTrajectoryPlayer_State*& src, AnimationTrajectoryPlayer_State*& dest )
{
	ASSERT( !dest );
	dest = src;
	src = NULL;
}

/*void AnimationTrajectoryPlayer::SetupControlRig( const AnimationTrajectoryPlayer::InternalAnimState& state )
{
	if ( state.m_animation )
	{
		static Float weight = 1.f;
		static Float weapon = 1.f;

		//m_crController.SetEffectorActiveHandL( weight );
		m_crController.SetEffectorActive_HandR( weight );

		//m_crController.SetWeaponOffsetHandL( weapon );
		m_crController.SetWeaponOffset_HandR( weapon );

		if ( state.m_trajectoryR )
		{
			//const Vector pointLS_L = state.m_trajectoryL->GetPointLS( state.m_animationState.m_currTime );
			const Vector pointLS_R = state.m_trajectoryR->GetPointLS( state.m_animationState.m_currTime );

			//const Vector pointL = m_component->GetLocalToWorld().TransformPoint( pointLS_L );
			const Vector pointR = m_component->GetLocalToWorld().TransformPoint( pointLS_R );

			//m_crController.SetEffectorHandL( pointL );
			m_crController.SetEffector_HandR( pointR );
		}
	}
	else
	{
		m_crController.SetEffectorActive_HandL( 0.f );
		m_crController.SetEffectorActive_HandR( 0.f );

		m_crController.SetWeaponOffset_HandL( 0.f );
		m_crController.SetWeaponOffset_HandR( 0.f );

		m_crController.SetEffector_HandL( Vector::ZERO_3D_POINT );
		m_crController.SetEffector_HandR( Vector::ZERO_3D_POINT );
	}
}*/

Bool AnimationTrajectoryPlayer::IsPlayingAnimation() const
{
	return m_currAnimation || m_nextAnimation;
}

Bool AnimationTrajectoryPlayer::IsBeforeSyncTime() const
{
	return ( m_currAnimation && m_currAnimation->IsBeforeSyncTime() ) || m_nextAnimation;
}

Bool AnimationTrajectoryPlayer::SelectAnimation( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const
{
	if ( !IsValid() )
	{
		return false;
	}

	token.m_isValid = false;

	token.m_proxySyncType = input.m_proxySyncType;
	token.m_proxy = input.m_proxy;
	token.m_tagId = input.m_tagId;
	token.m_selectorType = input.m_selectorType;

	switch ( input.m_selectorType )
	{
	case ATST_IK:
		SelectAnimation_IK( input, token );
		break;

	case ATST_Blend2:
		SelectAnimation_Blend2( input, token );
		break;

	case ATST_Blend3:
		SelectAnimation_Blend3( input, token );
		break;

	case ATST_Blend2Direction:
		SelectAnimation_Blend2Direction( input, token );
		break;

	default:
		ASSERT( 0 );
	}

	return token.m_isValid;
}

void AnimationTrajectoryPlayer::SelectAnimation_IK( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const
{
	ComponentAnimationIterator it( m_component.Get() );

	AnimationSelector_Trajectory selector;
	selector.Init( it );

	AnimationSelector_Trajectory::InputData si;
	si.m_pointWS = input.m_pointWS;
	si.m_tagId = input.m_tagId;

	const CSkeletalAnimationSetEntry* animation = selector.DoSelect( si, input.m_localToWorld );
	if ( !animation )
	{
		return;
	}

	Vector syncPointMS;
	const CSkeletalAnimationAttackTrajectoryParam* trajParam = animation->FindParam< CSkeletalAnimationAttackTrajectoryParam >();
	if ( !trajParam->GetSyncPointRightMS( syncPointMS ) )
	{
		ASSERT( 0 );
		return;
	}

	token.m_isValid = true;
	token.m_animationA = animation;
	token.m_trajectoryParamA = trajParam;
	token.m_pointWS = input.m_pointWS;
	token.m_syncPointMS = syncPointMS;
	token.m_duration = animation->GetDuration();
	token.m_syncTime = trajParam->GetDataR().m_syncFrame * 1.f/30.f;
}

void AnimationTrajectoryPlayer::SelectAnimation_Blend2( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const
{
	ComponentAnimationIterator it( m_component.Get() );

	AnimationSelector_Blend2 selector;
	selector.Init( it );

	AnimationSelector_Blend2::InputData si;
	si.m_pointWS = input.m_pointWS;
	si.m_tagId = input.m_tagId;

	AnimationSelector_Blend2::OutputData so;

	const Bool ret = selector.DoSelect( si, input.m_localToWorld, so );
	if ( !ret )
	{
		return;
	}

	Vector syncPointMS;
	const CSkeletalAnimationAttackTrajectoryParam* trajParamA = so.m_animationA->FindParam< CSkeletalAnimationAttackTrajectoryParam >();
	const CSkeletalAnimationAttackTrajectoryParam* trajParamB = so.m_animationB->FindParam< CSkeletalAnimationAttackTrajectoryParam >();
	
	if ( !trajParamA->GetSyncPointRightMS( syncPointMS ) )
	{
		ASSERT( 0 );
		return;
	}

	token.m_isValid = true;
	token.m_animationA = so.m_animationA;
	token.m_animationB = so.m_animationB;
	token.m_weightA = so.m_weight;
	token.m_trajectoryParamA = trajParamA;
	token.m_trajectoryParamB = trajParamB;
	token.m_pointWS = input.m_pointWS;
	token.m_syncPointMS = syncPointMS;
	token.m_duration = so.m_animationA->GetDuration();
	token.m_syncTime = trajParamA->GetDataR().m_syncFrame * 1.f/30.f;
}

void AnimationTrajectoryPlayer::SelectAnimation_Blend3( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const
{
	// TODO
	ASSERT( 0 );
}

void AnimationTrajectoryPlayer::SelectAnimation_Blend2Direction( const SAnimationTrajectoryPlayerInput& input, SAnimationTrajectoryPlayerToken& token ) const
{
	ComponentAnimationIterator it( m_component.Get() );

	AnimationSelector_Blend2Direction selector;
	selector.Init( it );

	AnimationSelector_Blend2Direction::InputData si;
	si.m_pointWS = input.m_pointWS;
	si.m_directionWS = input.m_directionWS;
	si.m_tagId = input.m_tagId;

	AnimationSelector_Blend2Direction::OutputData so;

	const Bool ret = selector.DoSelect( si, input.m_localToWorld, so );
	if ( !ret )
	{
		return;
	}

	Vector syncPointMS;
	const CSkeletalAnimationAttackTrajectoryParam* trajParamA = so.m_animationA->FindParam< CSkeletalAnimationAttackTrajectoryParam >();
	const CSkeletalAnimationAttackTrajectoryParam* trajParamB = so.m_animationB->FindParam< CSkeletalAnimationAttackTrajectoryParam >();

	if ( !trajParamA->GetSyncPointRightMS( syncPointMS ) )
	{
		ASSERT( 0 );
		return;
	}

	token.m_isValid = true;
	token.m_animationA = so.m_animationA;
	token.m_animationB = so.m_animationB;
	token.m_weightA = so.m_weight;
	token.m_trajectoryParamA = trajParamA;
	token.m_trajectoryParamB = trajParamB;
	token.m_pointWS = input.m_pointWS;
	token.m_syncPointMS = syncPointMS;
	token.m_duration = so.m_animationA->GetDuration();
	token.m_syncTime = trajParamA->GetDataR().m_syncFrame * 1.f/30.f;
}

Bool AnimationTrajectoryPlayer::PlayAnimation( const SAnimationTrajectoryPlayerToken& token )
{
	if ( !token.m_isValid || m_nextAnimation )
	{
		return false;
	}

	ASSERT( !m_nextAnimation );

	switch ( token.m_selectorType )
	{
	case ATST_IK:
		m_nextAnimation = new AnimationTrajectoryPlayer_Trajectory( token, m_component.Get() );
		break;

	case ATST_Blend2:
	case ATST_Blend2Direction:
		m_nextAnimation = new AnimationTrajectoryPlayer_Blend2( token, m_component.Get() );
		break;

	case ATST_Blend3:
		// TODO
		break;

	default:
		ASSERT( 0 );
	}
	
	ASSERT( m_nextAnimation );

	return m_nextAnimation != NULL;
}

Bool AnimationTrajectoryPlayer::Stop()
{
	Bool ret = IsPlayingAnimation();

	ClearState( m_currAnimation );
	ClearState( m_nextAnimation );

	return ret;
}

void AnimationTrajectoryPlayer::UpdatePoint( const Vector& pointWS )
{
	const CAnimatedComponent* ac = m_component.Get();
	if ( ac )
	{
		UpdatePoint( ac->GetLocalToWorld(), pointWS );
	}
}

void AnimationTrajectoryPlayer::UpdatePoint( const Matrix& l2w, const Vector& pointWS )
{
	if ( m_currAnimation )
	{
		const Bool ret = m_currAnimation->UpdatePoint( l2w, pointWS );
		if ( !ret )
		{
			// TODO - recreate state with new params
			ASSERT( 0 );
		}
	}
}

void AnimationTrajectoryPlayer::GenerateFragments( CRenderFrame* frame )
{
	if ( m_currAnimation )
	{
		m_currAnimation->GenerateFragments( frame );
	}
}

Bool AnimationTrajectoryPlayer::SetupSlot( CAnimatedComponent* ac )
{
	return ac && ac->GetBehaviorStack() ? ac->GetBehaviorStack()->GetSlot( m_slotName, m_slot ) : false;
}

void AnimationTrajectoryPlayer::CloseSlot()
{
	if ( m_slot.IsValid() )
	{
		m_slot.Stop();
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( AnimationTrajectoryPlayerScriptWrapper );

AnimationTrajectoryPlayerScriptWrapper::AnimationTrajectoryPlayerScriptWrapper()
	: m_actor( NULL )
{

}

void AnimationTrajectoryPlayerScriptWrapper::Render( CRenderFrame* frame, const Matrix& matrix )
{
	ASSERT( m_actor );

	static Bool RENDER = true;
	if ( RENDER )
	{
		m_player.GenerateFragments( frame );
	}
}

void AnimationTrajectoryPlayerScriptWrapper::funcInit( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entity, NULL );
	GET_PARAMETER_OPT( CName, slotName, CName::NONE );
	FINISH_PARAMETERS;

	CEntity* e = entity.Get();
	if ( e )
	{
		m_player.Init( e, slotName );
	}

	m_actor = e->QueryActorInterface();

	if ( m_actor && m_actor->GetVisualDebug() )
	{
		m_actor->GetVisualDebug()->AddObject( static_cast< IVisualDebugInterface* >( this ) );
	}
}

void AnimationTrajectoryPlayerScriptWrapper::funcDeinit( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( m_actor && m_actor->GetVisualDebug() )
	{
		m_actor->GetVisualDebug()->RemoveObject( static_cast< IVisualDebugInterface* >( this ) );
	}

	m_player.Deinit();
	m_actor = NULL;
}

void AnimationTrajectoryPlayerScriptWrapper::funcSelectAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationTrajectoryPlayerInput, input, SAnimationTrajectoryPlayerInput() );
	FINISH_PARAMETERS;

	SAnimationTrajectoryPlayerToken token;
	m_player.SelectAnimation( input, token );

	RETURN_STRUCT( SAnimationTrajectoryPlayerToken, token );
}

void AnimationTrajectoryPlayerScriptWrapper::funcPlayAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( SAnimationTrajectoryPlayerToken, token, SAnimationTrajectoryPlayerToken() );
	FINISH_PARAMETERS;

	Bool ret = m_player.PlayAnimation( token );

	RETURN_BOOL( ret );
}

void AnimationTrajectoryPlayerScriptWrapper::funcTick( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, dt, 0.f );
	FINISH_PARAMETERS;

	m_player.Tick( dt );
}

void AnimationTrajectoryPlayerScriptWrapper::funcIsPlayingAnimation( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_player.IsPlayingAnimation() );
}

void AnimationTrajectoryPlayerScriptWrapper::funcIsBeforeSyncTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_player.IsBeforeSyncTime() );
}

void AnimationTrajectoryPlayerScriptWrapper::funcUpdateCurrentPoint( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pointWS, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	m_player.UpdatePoint( pointWS );
}

void AnimationTrajectoryPlayerScriptWrapper::funcUpdateCurrentPointM( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Matrix, mat, Matrix::IDENTITY );
	GET_PARAMETER( Vector, pointWS, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	m_player.UpdatePoint( mat, pointWS );
}

void AnimationTrajectoryPlayerScriptWrapper::funcGetTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	const Float time = m_player.GetTime();

	RETURN_FLOAT( time );
}

void AnimationTrajectoryPlayerScriptWrapper::funcWaitForSyncTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// Timeout
	const Float timeout = 100.f;
	const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
	if ( waitedTime > timeout )
	{
		m_player.Stop();

		RETURN_BOOL( false );
		return;
	}

	if ( m_player.IsBeforeSyncTime() )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( true );
}

void AnimationTrajectoryPlayerScriptWrapper::funcWaitForFinish( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// Timeout
	const Float timeout = 100.f;
	const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
	if ( waitedTime > timeout )
	{
		m_player.Stop();

		RETURN_BOOL( false );
		return;
	}

	if ( m_player.IsPlayingAnimation() )
	{
		stack.m_thread->ForceYield();
		return;
	}

	RETURN_BOOL( true );
}
