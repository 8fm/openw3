
#include "build.h"

#include "comboPlayer.h"
#include "../../common/engine/behaviorGraphStack.h"
#include "../../common/engine/skeletalAnimationContainer.h"
#include "../../common/engine/visualDebug.h"

IMPLEMENT_ENGINE_CLASS( CComboPlayer );
IMPLEMENT_ENGINE_CLASS( SComboAttackCallbackInfo );
IMPLEMENT_RTTI_ENUM( EComboAttackType );

RED_DEFINE_STATIC_NAME( ComboSlot )
RED_DEFINE_STATIC_NAME( AllowInput )
RED_DEFINE_STATIC_NAME( ActionBlend )
RED_DEFINE_STATIC_NAME( AllowBlend )
RED_DEFINE_STATIC_NAME( AttackEndAUX )
RED_DEFINE_STATIC_NAME( ComboAllowBlend )

const String CComboPlayer::COMBO_STATE_NAME( TXT("TOMSIN") );

CComboPlayer::CComboPlayer()
	: m_definition( NULL )
	, m_entity( NULL )
	, m_globalAttackCounter( 0 )
	, m_stringAttackCounter( 0 )
    , m_defaultBlendDuration( 0.2f )
	, m_paused( false )
	, m_sliderPaused( false )
{
	m_slotName = CNAME( ComboSlot );
	m_eventAllowInput = CNAME( AllowInput );
	m_eventAllowIntenalBlend = CNAME( ActionBlend );
	m_eventAllowExternalBlend = CNAME( AllowBlend );
	m_eventAnimationFinished = CNAME( AttackEndAUX );

	m_externalBlendVarName = CNAME( ComboAllowBlend );

	ASSERT( !m_currState.IsSet() );
	ASSERT( !m_nextState.IsSet() );
}

Bool CComboPlayer::Build( const CComboDefinition* definition, CEntity* entity )
{
	ASSERT( !m_definition );
	ASSERT( !m_entity );
	ASSERT( !m_currState.IsSet() );
	ASSERT( !m_nextState.IsSet() );

	m_definition = definition;
	m_entity = entity;

	return true;
}

Bool CComboPlayer::Init()
{
	Bool ret = false;

	m_attackId = -1;

	if ( !m_entity )
	{
		ASSERT( m_entity );
		return false;
	}

	const CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root && root->GetBehaviorStack() )
	{
		ret = root->GetBehaviorStack()->GetSlot( m_slotName, m_slot, false );
	}

	if ( ret )
	{
		m_cachedInstanceName = m_slot.GetInstanceName();
	}

	CActor* actor = Cast< CActor >( m_entity );
	if ( actor && actor->GetVisualDebug() )
	{
		actor->GetVisualDebug()->AddObject( static_cast< IVisualDebugInterface* >( this ) );
	}

	return ret;
}

void CComboPlayer::Deinit()
{
	if ( m_currState.IsSet() )
	{
		FinishAttackAction();
	}

	if ( m_nextState.IsSet() )
	{
		m_nextState.Reset( m_defaultBlendDuration );
	}

	if ( m_slot.IsValid() )
	{
		// Do not stop the slot, otherwise we get a TPose
		//m_slot.Stop();
		m_slot.ResetMotion();
		m_slot.Clear();
	}

	m_attackId = -1;
	m_cachedInstanceName = CName::NONE;

	CActor* actor = Cast< CActor >( m_entity );
	if ( actor && actor->GetVisualDebug() )
	{
		actor->GetVisualDebug()->RemoveObject( static_cast< IVisualDebugInterface* >( this ) );
	}

	RemoveDebugBars();
}

Bool CComboPlayer::Update( Float dt )
{
	if ( !IsInstanceValid( m_cachedInstanceName ) )
	{
		// Upsss....
		return false;
	}

	if ( IsPaused() )
	{
		return true;
	}

	// apply time mutliplier to play animation with same speed as animations from behavior graph
	if ( CAnimatedComponent* ac = m_entity->GetRootAnimatedComponent() )
	{
		dt *= ac->GetTimeMultiplier();
	}
	
	Float timeRest = 0.f;

	if ( IsPlayingAttack() )
	{
		Bool blendAnimations = false;

		if ( IsBlendingAttacks() )
		{
			blendAnimations = true;
		}
		else
		{
			if ( WillBeInternalBlendTime( dt ) && HasNextAttackScheduled() )
			{
				ASSERT( !IsAllowExternalBlendVariableSet() );

				blendAnimations = true;
			}
			else if ( WillBeExternalBlendTime( dt ) )
			{
				if ( HasNextAttackScheduled() )
				{
					ASSERT( !IsAllowExternalBlendVariableSet() );

					blendAnimations = true;
				}
				else
				{
					SetAllowExternalBlendVariable( true );
				}
			}
		}

		Bool silderUpdated = false;

		if ( blendAnimations )
		{
			if ( !ProcessInternalBlend( dt, timeRest ) )
			{
				FinishInternalBlendAction( timeRest );
			}
			else
			{
				UpdateSlider( m_nextState );
				silderUpdated = true;
			}
		}
		else if ( !UpdateAttackAnimation( dt, timeRest ) )
		{
			FinishAttackAction();

			if ( HasNextAttackScheduled() )
			{
				PlayScheduledAttack( timeRest );
			}
		}

		if ( !silderUpdated )
		{
			UpdateSlider( m_currState );
		}	
	}
	else
	{
		ASSERT( !HasNextAttackScheduled() );

		m_globalAttackCounter = 0;
		m_stringAttackCounter = 0;

		ProcessAttackFinishedEvent();
	}

	UpdateDebugBars();

	return IsPlayingAttack();
}

Bool CComboPlayer::PlayAttack( const CName& aspectName )
{
	ASSERT( m_definition );
	ASSERT( m_entity );

	if ( !IsInstanceValid( m_cachedInstanceName ) )
	{
		// Upsss....
		return false;
	}

	if ( IsPlayingAttack() && !IsInputTime() )
	{
		return false;
	}

	if ( HasNextAttackScheduled() && IsInternalBlendTime() )
	{
		return false;
	}

	if ( m_nextState.IsSet() )
	{
		ASSERT( m_nextState.m_animationState.m_prevTime < 0.0001f );
	}

	Bool ret = true;

	if ( !IsBehaviorComboStateActive() )
	{
		ret = SendComboEventToBehaviorGraph();
	}

	ret = SetAllowExternalBlendVariable( false );

	ret = ScheduleAttack( aspectName );

	if ( !IsPlayingAttack() && HasNextAttackScheduled() )
	{
		ret = PlayScheduledAttack( 0.f );
	}

	return ret;
}

Bool CComboPlayer::PlayHit()
{
	ASSERT( m_definition );
	ASSERT( m_entity );

	if ( !IsInstanceValid( m_cachedInstanceName ) )
	{
		// Upsss....
		return false;
	}

	if ( !IsPlayingAttack() )
	{
		return false;
	}

	if ( ! m_currState.IsSet() )
	{
		return false;
	}
	
	const CComboAspect* aspect = m_definition->m_aspects[ m_currState.m_aspectIndex ];
	CName hitAnimation = aspect->GetHitAnimation( m_currState.m_animationState.m_animation );
	if ( hitAnimation.Empty() )
	{
		return false;
	}

	const CSkeletalAnimationSetEntry* animationEntry = FindAnimation( hitAnimation );
	if ( ! animationEntry )
	{
		return false;
	}

	m_currState.m_animationDuration = animationEntry->GetDuration();
	m_currState.m_animationState.m_currTime = 0.0f;
	m_currState.m_animationState.m_prevTime = 0.0f;
	m_currState.m_animationState.m_animation = hitAnimation;
	FillStateEvents( m_currState, animationEntry );

	return true;
}

void CComboPlayer::Pause()
{
	ASSERT( !IsPaused() );

	m_paused = true;

	if ( m_slot.IsValid() )
	{
		m_slot.SetMotion( AnimQsTransform::IDENTITY );
	}

	RemoveDebugBars();
}

void CComboPlayer::Unpause()
{
	ASSERT( IsPaused() );

	if ( m_slot.IsValid() )
	{
		m_slot.ResetMotion();
	}

	m_paused = false;
}

Bool CComboPlayer::IsPaused() const
{
	return m_paused;
}

void CComboPlayer::PauseSlider()
{
	ASSERT( !IsSliderPaused() );

	m_sliderPaused = true;

	if ( m_slot.IsValid() )
	{
		m_slot.ResetMotion();
	}

	if ( m_currState.IsSet() )
	{
		m_currState.m_isSliderInit = false;
	}
	if ( m_nextState.IsSet() )
	{
		ASSERT( !m_nextState.m_isSliderInit );
		m_nextState.m_isSliderInit = false;
	}
}

void CComboPlayer::UnpauseSlider()
{
	ASSERT( IsSliderPaused() );

	m_sliderPaused = false;
}

Bool CComboPlayer::IsSliderPaused() const
{
	return m_sliderPaused;
}

void CComboPlayer::Render( CRenderFrame* frame, const Matrix& matrix )
{
	if ( m_currState.IsSet() && !IsPaused() && !IsSliderPaused() )
	{
		m_currState.m_slider.GenerateDebugFragments( frame );
	}
}

Bool CComboPlayer::IsInstanceValid( const CName& name ) const
{
	ASSERT( name != CName::NONE );

	CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root && root->GetBehaviorStack() )
	{
		return root->GetBehaviorStack()->HasActiveInstance( name );
	}

	// No good...
	return false;
}

Bool CComboPlayer::ScheduleAttack( const CName& aspectName )
{
	ASSERT( !IsAllowExternalBlendVariableSet() );

	SComboPlayerState nextState;
	FindNextAttack( m_currState, nextState, aspectName );

	if ( nextState.IsSet() )
	{
		m_nextState = nextState;
        m_nextState.m_blendDuration = m_defaultBlendDuration;

		return true;
	}

	return false;
}

Bool CComboPlayer::PlayScheduledAttack( Float timeToSync )
{
	SetAllowExternalBlendVariable( false );

	ASSERT( m_nextState.IsSet() );
	ASSERT( !m_currState.IsSet() );

	m_currState = m_nextState;
	m_nextState.Reset( m_defaultBlendDuration );

	m_currState.m_animationState.m_prevTime = 0.f;
	m_currState.m_animationState.m_currTime = timeToSync;

	return true;
}

Bool CComboPlayer::IsPlayingAttack() const
{
	return m_currState.IsSet();
}

Bool CComboPlayer::IsPlayingDirAttack() const
{
	return IsPlayingAttack() && m_currState.m_animationIndex == DIR_ANIMATION;
}

Bool CComboPlayer::HasNextAttackScheduled() const
{
	return m_nextState.IsSet();
}

Bool CComboPlayer::IsBlendingAttacks() const
{
	return IsPlayingAttack() && HasNextAttackScheduled() && m_currState.m_blendingTimer > 0.f;
}

Bool CComboPlayer::UpdateAttackAnimation( Float dt, Float& timeRest )
{
	if ( !m_slot.IsValid() )
	{
		return false;
	}

	ASSERT( m_currState.IsSet() );

	m_currState.m_animationState.m_prevTime = m_currState.m_animationState.m_currTime;
	m_currState.m_animationState.m_currTime += dt;

	m_slot.PlayAnimation( m_currState.m_animationState );

	const Float timeDiff = m_currState.m_animationState.m_currTime - m_currState.m_animationDuration;
	if ( timeDiff < 0.f )
	{
		timeRest = 0.f;

		return true;
	}
	else
	{
		timeRest = timeDiff;

		return false;
	}
}

void CComboPlayer::UpdateSlider( SComboPlayerState& state )
{
	Bool slideAdded = false;

	if ( !IsSliderPaused() && state.IsSet() && ( state.m_useRotation || state.m_useTranslation ) )
	{
		if ( !IsBehaviorComboStateActive() )
		{
			state.m_isSliderInit = false;
		}

		if ( !state.m_isSliderInit )
		{
			if ( m_entity->GetRootAnimatedComponent()->GetAnimationContainer() )
			{
				const CSkeletalAnimationSetEntry* animEntry = m_entity->GetRootAnimatedComponent()->GetAnimationContainer()->FindAnimation( state.m_animationState.m_animation );

				SAnimSliderSettings settings;
				settings.m_rotationPolicy = state.m_useDeltaRotatePolicy ? SAnimSliderSettings::RP_Delta : SAnimSliderSettings::RP_Speed;
				settings.m_useRotationScaling = state.m_useRotationScaling;

				state.m_isSliderOk = state.m_slider.Init( animEntry, settings );

				state.m_slider.SetCurrentPosition( m_entity->GetLocalToWorld() );

				if( state.m_useTranslation )
				{
					state.m_slider.SetTargetPosition( state.m_outSlideToPosition );
				}
				if ( state.m_useRotation )
				{
					state.m_slider.SetTargetRotation( state.m_rotateToEnemyAngle );	
				}
			}

			state.m_isSliderInit = true;
		}

		if ( state.m_isSliderOk )
		{
			Vector motionTransDeltaWS;
			Float motionRotDeltaWS;

			static Bool useWS = true;

			if ( useWS )
			{
				state.m_slider.UpdateAtWS( m_entity->GetLocalToWorld(), state.m_animationState.m_prevTime, state.m_animationState.m_currTime,  motionTransDeltaWS,  motionRotDeltaWS, state.m_useTranslation? &state.m_outSlideToPosition : nullptr );
				//state.m_slider2.UpdateAt( m_entity->GetLocalToWorld(), state.m_animationState.m_currTime,  motionTransDeltaWS,  motionRotDeltaWS );

				if ( m_slot.IsValid() )
				{
					AnimQsTransform extraMotion( AnimQsTransform::IDENTITY );

					Matrix worldToLocal;
					m_entity->GetWorldToLocal( worldToLocal );

#ifdef USE_HAVOK_ANIMATION
					extraMotion.m_translation.setXYZ( TO_CONST_HK_VECTOR_REF( worldToLocal.TransformVector( motionTransDeltaWS ) ) );
					extraMotion.m_rotation.setAxisAngle( hkVector4( 0.f, 0.f, 1.f ), DEG2RAD( motionRotDeltaWS ) );

					ASSERT( motionTransDeltaWS.Mag3() < 25.f );
					ASSERT( extraMotion.m_translation.length3() < 25.f );
#else
					Vector transVec = worldToLocal.TransformVector( motionTransDeltaWS );
					extraMotion.Translation.Set( transVec.X, transVec.Y, transVec.Z, 0.0f );
					extraMotion.Rotation.SetAxisAngle( RedVector4( 0.0f, 0.0f, 1.0f ), DEG2RAD( motionRotDeltaWS ) );

					ASSERT( motionTransDeltaWS.Mag3() < 25.f );
					ASSERT( extraMotion.Translation.Length3() < 25.f );
#endif
					m_slot.SetMotion( extraMotion );

					slideAdded = true;
				}
			}
			else
			{
				motionRotDeltaWS = 0.f;
				motionTransDeltaWS = Vector::ZERO_3D_POINT;
				state.m_slider.UpdateAtMS( m_entity->GetLocalToWorld(), state.m_animationState.m_prevTime, state.m_animationState.m_currTime,  motionTransDeltaWS,  motionRotDeltaWS );
				//ASSERT( 0 );

				if ( m_slot.IsValid() )
				{
					AnimQsTransform extraMotion( AnimQsTransform::IDENTITY );

#ifdef USE_HAVOK_ANIMATION
					extraMotion.m_translation.setXYZ( TO_CONST_HK_VECTOR_REF( motionTransDeltaWS ) );
					extraMotion.m_rotation.setAxisAngle( hkVector4( 0.f, 0.f, 1.f ), DEG2RAD( motionRotDeltaWS ) );
#else
					extraMotion.Translation.Set( motionTransDeltaWS.X, motionTransDeltaWS.Y, motionTransDeltaWS.Z, 0.0f );
					extraMotion.Rotation.SetAxisAngle( RedVector4( 0.0f, 0.0f, 1.0f ), DEG2RAD( motionRotDeltaWS ) );
#endif
					m_slot.SetMotion( extraMotion );

					slideAdded = true;
				}
			}
		}
	}

	if ( !slideAdded && m_slot.IsValid() )
	{
		m_slot.ResetMotion();
	}
}

void CComboPlayer::FinishAttackAction()
{
	m_currState.Reset( m_defaultBlendDuration );

	SetAllowExternalBlendVariable( true );

	if ( m_slot.IsValid() )
	{
		// Do not stop the slot, otherwise we get a TPose
		//m_slot.Stop();
		m_slot.ResetMotion();
	}

	ProcessAttackFinishedEvent();
}

void CComboPlayer::ProcessAttackFinishedEvent()
{
	m_entity->GetRootAnimatedComponent()->GetBehaviorStack()->GenerateBehaviorEvent( m_eventAnimationFinished );
}

Bool CComboPlayer::ProcessInternalBlend( Float dt, Float& timeRest )
{
	if ( !m_slot.IsValid() )
	{
		return false;
	}

	ASSERT( m_currState.IsSet() );
	ASSERT( m_nextState.IsSet() );
	ASSERT( m_currState.m_blendingTimer >= 0.f );
	ASSERT( m_currState.m_blendingTimer <= m_currState.m_animationDuration );

	const Float blendTime = m_currState.m_blendingTimer + dt;
	const Float blendDuration = m_currState.m_blendDuration;

	m_currState.m_blendingTimer += dt;

	m_currState.m_animationState.m_prevTime = m_currState.m_animationState.m_currTime;
	m_currState.m_animationState.m_currTime += dt;

	m_nextState.m_animationState.m_prevTime = m_nextState.m_animationState.m_currTime;
	m_nextState.m_animationState.m_currTime += dt;

	if ( blendDuration < 0.0001f )
	{
        m_slot.PlayAnimation( m_nextState.m_animationState );

		return false;
	}
	else if ( blendTime < blendDuration )
	{
		const Float p = blendTime / blendDuration;

		ASSERT( p <= 1.f && p >= 0.f );

		m_slot.PlayAnimations( m_currState.m_animationState, m_nextState.m_animationState, p );

		return true;
	}
	else
	{
        m_slot.PlayAnimation( m_nextState.m_animationState );

		return false;
	}
}

void CComboPlayer::FinishInternalBlendAction( Float timeRest )
{
	ASSERT( m_currState.IsSet() );
	ASSERT( m_nextState.IsSet() );

	m_currState = m_nextState;
	m_nextState.Reset( m_defaultBlendDuration );
}

RED_DEFINE_STATIC_NAME( OnComboAttackCallback );

Bool CComboPlayer::FindNextAttack_ContinueString( const SComboPlayerState& curr, SComboPlayerState& state )
{
	state.m_aspectIndex = curr.m_aspectIndex;
	state.m_animationIndex = curr.m_animationIndex;
	state.m_animationState.m_animation = curr.m_animationState.m_animation;
	state.m_animationsUsed = curr.m_animationsUsed;

	const CComboAspect* aspect = m_definition->m_aspects[ state.m_aspectIndex ];

	if ( state.m_animationIndex != DIR_ANIMATION )
	{
		ASSERT( state.m_animationIndex >= 0 );
	}

	Bool ret = false;

	if ( m_entity->GetCurrentState() )
	{
		SComboAttackCallbackInfo callbackInfo;
		callbackInfo.m_inAspectName = aspect->GetAspectName();
		callbackInfo.m_inGlobalAttackCounter = m_globalAttackCounter++;
		callbackInfo.m_inStringAttackCounter = m_stringAttackCounter++;
		callbackInfo.m_prevDirAttack = state.m_animationIndex == DIR_ANIMATION;
		callbackInfo.m_inAttackId = state.m_id = ++m_attackId;

		if ( CallFunctionRef( m_entity->GetCurrentState(), CNAME( OnComboAttackCallback ), callbackInfo ) )
		{
			const CComboString* str = aspect->GetRandomComboString( callbackInfo.m_outLeftString );
			callbackInfo.CheckData();

			if ( callbackInfo.m_outAttackType == ComboAT_Normal )
			{
				if ( state.m_animationIndex == DIR_ANIMATION )
                {
					// PTom: What is that?!
                    //temporary thing what has to be removed/replaced asap. Submitted on Design request as a temp hack.
                    if(m_currState.m_animationState.m_currTime > m_currState.m_internalBlendEnd && m_currState.m_animationState.m_currTime < m_currState.m_externalBlendEnd )
                    {
                        return false;
                    }

					state.m_animationIndex = 0;
					state.m_animationState.m_animation = str->GetAttack( state, callbackInfo.m_outDistance ).m_animationName;

					ret = true;
				}
				else
				{
					state.m_animationIndex += 1;
					// try to get linked animation first, if it fails, get normal attack
					CName linkedAnimation = aspect->GetLinkedAnimation( state.m_animationState.m_animation );
					if ( ! linkedAnimation.Empty() )
					{
						state.m_animationState.m_animation = linkedAnimation;
					}
					else
					{
						state.m_animationState.m_animation = str->GetAttack( state, callbackInfo.m_outDistance ).m_animationName;
					}

					ret = true;
				}
			}
			else if ( callbackInfo.m_outAttackType == ComboAT_Directional )
			{
				state.m_animationIndex = DIR_ANIMATION;
				state.m_animationState.m_animation = str->GetDirAttack( state, callbackInfo.m_outDirection, callbackInfo.m_outDistance ).m_animationName;
				m_stringAttackCounter = 1;

				ret = true;
			}
			else if ( callbackInfo.m_outAttackType == ComboAT_Restart )
			{
				state.m_animationIndex = 0;
				state.m_animationState.m_animation = str->GetAttack( state, callbackInfo.m_outDistance ).m_animationName;

				ret = true;
			}
			else if ( callbackInfo.m_outAttackType == ComboAT_Stop )
			{
				//...
			}
			else
			{
				ASSERT( 0 );
			}

			state.m_useRotation = callbackInfo.m_outShouldRotate;
			if ( callbackInfo.m_outShouldRotate )
			{
				state.m_rotateToEnemyAngle = callbackInfo.m_outRotateToEnemyAngle;
			}

			state.m_useTranslation = callbackInfo.m_outShouldTranslate;
			if ( callbackInfo.m_outShouldTranslate )
			{
				state.m_outSlideToPosition = callbackInfo.m_outSlideToPosition;
			}
		}
	}

	return ret;
}

Bool CComboPlayer::FindNextAttack_RandomAndStartNewString( SComboPlayerState& state )
{
	const CComboAspect* aspect = m_definition->m_aspects[ state.m_aspectIndex ];

	const Int32 max = aspect->m_strings.SizeInt();
	if ( max == 0 )
	{
		state.Reset( m_defaultBlendDuration );
		return false;
	}

	Bool ret = false;

	if ( m_entity->GetCurrentState() )
	{
		m_stringAttackCounter = 0;
		 
		SComboAttackCallbackInfo callbackInfo;
		callbackInfo.m_inAspectName = aspect->GetAspectName();
		callbackInfo.m_inGlobalAttackCounter = m_globalAttackCounter++;
		callbackInfo.m_inStringAttackCounter = m_stringAttackCounter++;
		callbackInfo.m_prevDirAttack = false;
		callbackInfo.m_inAttackId = state.m_id = ++m_attackId;

		if ( CallFunctionRef( m_entity->GetCurrentState(), CNAME( OnComboAttackCallback ), callbackInfo ) )
		{
			callbackInfo.CheckData();

			// Find new proper string
			const CComboString* str = aspect->GetRandomComboString( callbackInfo.m_outLeftString );
			if ( !str || ( str->m_attacks.Size() == 0 && str->m_distAttacks[0].Size() == 0 && str->m_distAttacks[1].Size() == 0 && str->m_distAttacks[2].Size() == 0 ) )
			{
				state.Reset( m_defaultBlendDuration );
				return false;
			}

			ASSERT( str );
			ASSERT( str->IsLeftSide() == callbackInfo.m_outLeftString );

			if ( callbackInfo.m_outAttackType == ComboAT_Normal )
			{
				state.m_animationIndex = 0;
				state.m_animationState.m_animation = str->GetAttack( state, callbackInfo.m_outDistance ).m_animationName;

				ret = true;
			}
			else if ( callbackInfo.m_outAttackType == ComboAT_Directional )
			{
				state.m_animationIndex = DIR_ANIMATION;
				const SComboAnimationData& anim = str->GetDirAttack( state, callbackInfo.m_outDirection, callbackInfo.m_outDistance );
				state.m_animationState.m_animation = anim.m_animationName;

				ret = true;
			}
			else if ( callbackInfo.m_outAttackType == ComboAT_Stop )
			{
				//...
			}
			else
			{
				ASSERT( 0 );
			}

			state.m_useRotation = callbackInfo.m_outShouldRotate;
			if ( callbackInfo.m_outShouldRotate )
			{
                state.m_rotateToEnemyAngle = callbackInfo.m_outRotateToEnemyAngle;
            }

			state.m_useTranslation = callbackInfo.m_outShouldTranslate;
            if ( callbackInfo.m_outShouldTranslate )
            {
                state.m_outSlideToPosition = callbackInfo.m_outSlideToPosition;
            }
		}
	}

	if ( !ret )
	{
		state.Reset( m_defaultBlendDuration );
	}

	return ret;
}

void CComboPlayer::FindNextAttack_StartNewAspect( SComboPlayerState& state, const CName& aspectName )
{
	const Int32 aspectIndex = FindAspectIndex( aspectName );
	if ( aspectIndex == -1 )
	{
		state.Reset( m_defaultBlendDuration );
		return;
	}

	const CComboAspect* aspect = m_definition->m_aspects[ aspectIndex ];
	if ( aspect->m_strings.Empty() )
	{
		state.Reset( m_defaultBlendDuration );
		return;
	}

	state.m_aspectIndex = aspectIndex;
	
	FindNextAttack_RandomAndStartNewString( state );
}

void CComboPlayer::FillStateEvents( SComboPlayerState& state, const CSkeletalAnimationSetEntry* animationEntry ) const
{
	TDynArray< CExtAnimEvent* > events;
	animationEntry->GetAllEvents( events );

	Bool inputEventAFound = false;

	ASSERT( state.m_inputStartB < 0.f );
	ASSERT( state.m_inputEndB < 0.f );

	const Uint32 evtSize = events.Size();
	for ( Uint32 i=0; i<evtSize; ++i )
	{
		CExtAnimEvent* evt = events[ i ];

		if ( evt->GetEventName() == m_eventAllowInput && IsType< CExtAnimDurationEvent >( evt ) )
		{
			CExtAnimDurationEvent* evtDuration = static_cast< CExtAnimDurationEvent* >( evt );

			if ( !inputEventAFound )
			{
				state.m_inputStartA = evtDuration->GetStartTime();
				state.m_inputEndA = evtDuration->GetEndTimeWithoutClamp();

				inputEventAFound = true;
			}
			else
			{
				state.m_inputStartB = evtDuration->GetStartTime();
				state.m_inputEndB = evtDuration->GetEndTimeWithoutClamp();
			}
		}
		else if ( evt->GetEventName() == m_eventAllowIntenalBlend && IsType< CExtAnimDurationEvent >( evt ) )
		{
			CExtAnimDurationEvent* evtDuration = static_cast< CExtAnimDurationEvent* >( evt );

			state.m_internalBlendStart = evtDuration->GetStartTime();
			state.m_internalBlendEnd = evtDuration->GetEndTimeWithoutClamp();
		}
		else if ( evt->GetEventName() == m_eventAllowExternalBlend && IsType< CExtAnimDurationEvent >( evt ) )
		{
			CExtAnimDurationEvent* evtDuration = static_cast< CExtAnimDurationEvent* >( evt );

			state.m_externalBlendStart = evtDuration->GetStartTime();
			state.m_externalBlendEnd = evtDuration->GetEndTimeWithoutClamp();
		}
	}

    state.m_externalBlendEnd = animationEntry->GetDuration();
    //state.m_inputEnd = state.m_internalBlendEnd; //tmp hack - see changelist description #227172
}

void CComboPlayer::FindNextAttack( const SComboPlayerState& curr, SComboPlayerState& next, const CName& aspectName )
{
	ASSERT( m_definition );

	const Bool isInExtarnalBlendOnly = !IsInternalBlendTime() && IsExteralBlendTime();

	if ( IsPlayingAttack() && !isInExtarnalBlendOnly )
	{
		const CName& currentAspectName = m_definition->m_aspects[ curr.m_aspectIndex ]->GetAspectName();
		if ( currentAspectName == aspectName )
		{
			if ( !FindNextAttack_ContinueString( curr, next ) )
			{
				FindNextAttack_RandomAndStartNewString( next );
			}
		}
        // if commented out for now. The function wasn't used anyway.
		else //if ( !TryFindCorrelatedAttackForString( curr, next, aspectName ) )
		{
			FindNextAttack_StartNewAspect( next, aspectName );
		}
	}
	else
	{
		FindNextAttack_StartNewAspect( next, aspectName );
	}
	
	if ( !next.IsSet() )
	{
		return;
	}

	const CSkeletalAnimationSetEntry* animationEntry = FindAnimation( next.m_animationState.m_animation );
	if ( !animationEntry )
	{
		next.Reset( m_defaultBlendDuration );
	}
	else
	{
		ASSERT( next.m_animationState.m_currTime == 0.f );
		ASSERT( next.m_animationState.m_prevTime == 0.f );

		next.m_animationDuration = animationEntry->GetDuration();

		FillStateEvents( next, animationEntry );
	}
}

const CSkeletalAnimationSetEntry* CComboPlayer::FindAnimation( const CName& animationName ) const
{
	const CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root && root->GetAnimationContainer() )
	{
		return root->GetAnimationContainer()->FindAnimation( animationName );
	}
	return NULL;
}

Int32 CComboPlayer::FindAspectIndex( const CName& aspectName ) const
{
	ASSERT( m_definition );

	const Uint32 size = m_definition->m_aspects.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CComboAspect* aspect = m_definition->m_aspects[ i ];
		if ( aspect->GetAspectName() == aspectName )
		{
			return i;
		}
	}
	return -1;
}

const CComboAspect* CComboPlayer::FindAspect( const CName& aspectName ) const
{
	const Int32 index = FindAspectIndex( aspectName );
	return index != -1 ? m_definition->m_aspects[ index ] : NULL;
}

Bool CComboPlayer::SetAllowExternalBlendVariable( Bool state )
{
	CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root && root->GetBehaviorStack() )
	{
		return root->GetBehaviorStack()->SetBehaviorVariable( m_externalBlendVarName, state ? 1.f : 0.f );
	}
	return false;
}

Bool CComboPlayer::IsAllowExternalBlendVariableSet() const
{
	CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root && root->GetBehaviorStack() )
	{
		return root->GetBehaviorStack()->GetBehaviorFloatVariable( m_externalBlendVarName ) > 0.5f;
	}
	return false;
}

Bool CComboPlayer::IsBehaviorComboStateActive() const
{
	CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( m_slot.IsValid() && root && root->GetBehaviorStack() )
	{
		const String stateName = root->GetBehaviorStack()->GetStateInDefaultStateMachine( m_slot.GetInstanceName() );
		return stateName == COMBO_STATE_NAME;
	}

	return false;
}

Bool CComboPlayer::SendComboEventToBehaviorGraph()
{
	CAnimatedComponent* root = m_entity->GetRootAnimatedComponent();
	if ( root && root->GetBehaviorStack() )
	{
		return root->GetBehaviorStack()->GenerateBehaviorForceEvent( CNAME( ComboSlot ) );
	}
	return false;
}

Bool CComboPlayer::IsInputTime( const SComboPlayerState& state ) const
{
	const Bool isInTimeA = state.m_inputStartA <= state.m_animationState.m_currTime &&
		state.m_inputEndA > state.m_animationState.m_currTime;

	const Bool isInTimeB = state.m_inputStartB > 0.f && 
		state.m_inputStartB <= state.m_animationState.m_currTime &&
		state.m_inputEndB > state.m_animationState.m_currTime;

	return isInTimeA || isInTimeB;
}

Bool CComboPlayer::IsInternalBlendTime( const SComboPlayerState& state ) const
{
	return state.m_internalBlendStart <= state.m_animationState.m_currTime &&
		state.m_internalBlendEnd > state.m_animationState.m_currTime;
}

Bool CComboPlayer::IsExteralBlendTime( const SComboPlayerState& state ) const
{
	return state.m_externalBlendStart <= state.m_animationState.m_currTime &&
		state.m_externalBlendEnd > state.m_animationState.m_currTime;
}

Bool CComboPlayer::IsInputTime() const
{
	return IsInputTime( m_currState );
}

Bool CComboPlayer::IsInternalBlendTime() const
{
	return IsInternalBlendTime( m_currState );
}

Bool CComboPlayer::IsExteralBlendTime() const
{
	return IsExteralBlendTime( m_currState );
}

Bool CComboPlayer::WillBeInternalBlendTime( Float dt ) const
{
	const Float time = m_currState.m_animationState.m_currTime + dt;
	return m_currState.m_internalBlendStart <= time && m_currState.m_internalBlendEnd > time;
}

Bool CComboPlayer::WillBeExternalBlendTime( Float dt ) const
{
	const Float time = m_currState.m_animationState.m_currTime + dt;
	return m_currState.m_externalBlendStart <= time && m_currState.m_externalBlendEnd > time;
}

RED_DEFINE_STATIC_NAME( ComboBar1 );
RED_DEFINE_STATIC_NAME( ComboBar2 );
RED_DEFINE_STATIC_NAME( ComboBar3 );
RED_DEFINE_STATIC_NAME( ComboBar4 );
RED_DEFINE_STATIC_NAME( ComboBar5 );
RED_DEFINE_STATIC_NAME( ComboBar6 );
RED_DEFINE_STATIC_NAME( ComboBar7 );
RED_DEFINE_STATIC_NAME( ComboBar8 );

void CComboPlayer::UpdateDebugBar( const SComboPlayerState& state, Int32 x, Int32 y, Int32 h, Int32 h2, Int32 w, const CName& n1, const CName& n2, const CName& n3, const CName& n4 ) const
{
	Float p1 = 0.f;
	Float p2 = 1.f;
	Float ptr = 0.f;

	// well, it crashes...
	if ( !GCommonGame || !GCommonGame->GetPlayer() || !GCommonGame->GetPlayer()->GetVisualDebug() )
		return;

	const Float time = state.IsSet() ? state.m_animationState.m_currTime / state.m_animationDuration : 0.f;
	ptr = time;

	if ( state.IsSet() )
	{
		String name = state.m_animationState.m_animation.AsString();

		if ( state.m_blendingTimer > 0.f )
		{
			name += String::Printf( TXT(" - [%1.2f]"), state.m_blendingTimer / state.m_blendDuration * 100.f );
		}

		GCommonGame->GetPlayer()->GetVisualDebug()->AddRangeBarWithPointer( n1, x, y, w, h2, p1, p2, ptr, Color( 0, 80, 0 ), name );
	}
	else
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddBar( n1, x, y, w, h2, 0.f, Color( 0, 80, 0 ), state.m_animationState.m_animation.AsString().AsChar(), -1.f, true );
	}


	y += h2 + 2;
	p1 = state.m_inputStartA / state.m_animationDuration;
	p2 = state.m_inputEndA / state.m_animationDuration;
	Float p1B = state.m_inputStartB / state.m_animationDuration;
	Float p2B = state.m_inputEndB / state.m_animationDuration;
	ptr = time;

	if ( state.IsSet() )
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddRangeBarWithPointer( n2, x, y, w, h, p1, p2, p1B, p2B, ptr, Color( 0, 0, 255 ) );
	}
	else
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddBar( n2, x, y, w, h, 0.f, Color( 0, 0, 255 ), String::EMPTY, -1.f, true );
	}

	y += h + 2;
	p1 = state.m_internalBlendStart / state.m_animationDuration;
	p2 = state.m_internalBlendEnd / state.m_animationDuration;
	ptr = time;

	if ( state.IsSet() )
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddRangeBarWithPointer( n3, x, y, w, h, p1, p2, ptr, Color( 128, 0, 255 ) );
	}
	else
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddBar( n3, x, y, w, h, 0.f, Color( 128, 0, 255 ), String::EMPTY, -1.f, true );
	}


	y += h + 2;
	p1 = state.m_externalBlendStart / state.m_animationDuration;
	p2 = state.m_externalBlendEnd / state.m_animationDuration;
	ptr = time;

	if ( state.IsSet() )
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddRangeBarWithPointer( n4, x, y, w, h, p1, p2, ptr, Color( 128, 255, 0 ) );
	}
	else
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->AddBar( n4, x, y, w, h, 0.f, Color( 128, 255, 0 ), String::EMPTY, -1.f, true );
	}
}

void CComboPlayer::UpdateDebugBars() const
{
	// well, it crashes...
	if ( !GCommonGame || !GCommonGame->GetPlayer() || !GCommonGame->GetPlayer()->GetVisualDebug() )
		return;

	if ( m_entity == GCommonGame->GetPlayer() )
	{
		Int32 x = 100;
		Int32 y = 20;
		const Int32 h = 10;
		const Int32 h2 = 15;
		const Int32 w = 300;

		UpdateDebugBar( m_currState, x, y, h, h2, w, CNAME( ComboBar1 ), CNAME( ComboBar2 ), CNAME( ComboBar3 ), CNAME( ComboBar4 ) );

		x = x + w + 50;
		y = 20;

		UpdateDebugBar( m_nextState, x, y, h, h2, w, CNAME( ComboBar5 ), CNAME( ComboBar6 ), CNAME( ComboBar7 ), CNAME( ComboBar8 ) );
	}
}

void CComboPlayer::RemoveDebugBars()
{
	// well, it crashes...
	if ( !GCommonGame || !GCommonGame->GetPlayer() || !GCommonGame->GetPlayer()->GetVisualDebug() )
		return;

	if ( m_entity == GCommonGame->GetPlayer() )
	{
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar1 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar2 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar3 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar4 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar5 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar6 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar7 ) );
		GCommonGame->GetPlayer()->GetVisualDebug()->RemoveBar( CNAME( ComboBar8 ) );
	}
}

Bool CComboPlayer::ShouldRecalcTargetPosition( const Vector& prevTargetPos, const Vector& newTargetPos ) const
{
	// TODO
	// After checking if everything is ok in worst case scenario we can write here real test
	return true;
}

//////////////////////////////////////////////////////////////////////////

void CComboPlayer::funcPlayAttack( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, attackType, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( PlayAttack( attackType ) );
}

void CComboPlayer::funcPlayHit( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( PlayHit() );
}

void CComboPlayer::funcBuild( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CComboDefinition >, def, NULL );
	GET_PARAMETER( THandle< CEntity >, entity, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( Build( def.Get(), entity.Get() ) );
}

void CComboPlayer::funcInit( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( Init() );
}

void CComboPlayer::funcDeinit( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Deinit();
}

void CComboPlayer::funcUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, dt, 0.0f );
	FINISH_PARAMETERS;

	RETURN_BOOL( Update( dt ) );
}

void CComboPlayer::funcPause( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Pause();
}

void CComboPlayer::funcUnpause( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Unpause();
}

void CComboPlayer::funcIsPaused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsPaused() );
}

void CComboPlayer::funcPauseSlider( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	PauseSlider();
}

void CComboPlayer::funcUnpauseSlider( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	UnpauseSlider();
}

void CComboPlayer::funcIsSliderPaused( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsSliderPaused() );
}

void CComboPlayer::funcSetDurationBlend( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Float, blend, 0.0f );
    FINISH_PARAMETERS;

    m_defaultBlendDuration = blend;
}

void CComboPlayer::funcUpdateTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, attackId, -1 );
    GET_PARAMETER( Vector, pos, Vector::ZEROS );
	GET_PARAMETER( Float, rot, 0.f );
	GET_PARAMETER_OPT( Bool, deltaPolicy, false );
	GET_PARAMETER_OPT( Bool, useScaling, true );
    FINISH_PARAMETERS;

	ASSERT( attackId != -1 );

	ASSERT( !Red::Math::NumericalUtils::IsNan( pos.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( pos.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( pos.Z ) );

	ASSERT( pos.X == pos.X );
	ASSERT( pos.Y == pos.Y );
	ASSERT( pos.Y == pos.Y );

	if ( attackId != -1 )
	{
		if ( m_currState.m_id == attackId )
		{
			ASSERT( m_currState.IsSet() );

			if ( ShouldRecalcTargetPosition( m_currState.m_outSlideToPosition, pos ) )
			{
				m_currState.m_isSliderInit = false;
				m_currState.m_outSlideToPosition = pos;
				m_currState.m_rotateToEnemyAngle = rot;
				m_currState.m_useDeltaRotatePolicy = deltaPolicy;
				m_currState.m_useRotationScaling = useScaling;
			}
		}
		else if ( m_nextState.m_id == attackId )
		{
			ASSERT( m_nextState.IsSet() );

			if ( ShouldRecalcTargetPosition( m_nextState.m_outSlideToPosition, pos ) )
			{
				m_nextState.m_isSliderInit = false;
				m_nextState.m_outSlideToPosition = pos;
				m_nextState.m_rotateToEnemyAngle = rot;
				m_nextState.m_useDeltaRotatePolicy = deltaPolicy;
				m_nextState.m_useRotationScaling = useScaling;
			}
		}
	}
}

void CComboPlayer::funcStopAttack( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    FinishAttackAction();
}
