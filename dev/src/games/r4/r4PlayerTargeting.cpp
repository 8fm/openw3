/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

//////////////////////////////////////////////////////////////////////////
// Most of methods in this file are "copies" of scripted methods from r4player.ws.
// Until the scripted version will be fully moved to code, one needs to keep them compatible.

#include "build.h"
#include "r4PlayerTargeting.h"
#include "../../common/engine/pathLibWorld.h"
#include "../../common/engine/staticMeshComponent.h"
#include "../../common/physics/physicsWrapper.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/game/gameWorld.h"
#include "r4Player.h"
#include "r4Enums.h"
#include "w3GenericVehicle.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( STargetingInfo );
IMPLEMENT_ENGINE_CLASS( SR4PlayerTargetingConsts );
IMPLEMENT_ENGINE_CLASS( SR4PlayerTargetingPrecalcs );
IMPLEMENT_ENGINE_CLASS( SR4PlayerTargetingIn );
IMPLEMENT_ENGINE_CLASS( SR4PlayerTargetingOut );
IMPLEMENT_ENGINE_CLASS( CR4PlayerTargeting );

const Float CR4PlayerTargeting::HARD_LOCK_DISTANCE = 50.0f;
const Float CR4PlayerTargeting::SOFT_LOCK_DISTANCE_VISIBILITY_DURATION = 1.0f;
const Float CR4PlayerTargeting::IS_THREAT_EXPANDED_DISTANCE = 40.0f;

// #define PROFILE_TARGETING

//////////////////////////////////////////////////////////////////////////

STargetingInfo::STargetingInfo()
	: m_canBeTargetedCheck( false )
	, m_coneCheck( false )
	, m_coneHalfAngleCos( 0.0f )
	, m_coneDist( 0.0f )
	, m_coneHeadingVector( Vector::ZEROS )
	, m_distCheck( false )
	, m_invisibleCheck( false )
	, m_navMeshCheck( false )
	, m_inFrameCheck( false )
	, m_frameScaleX( 1.0f )
	, m_frameScaleY( 1.0f )
	, m_knockDownCheck( false )
	, m_knockDownCheckDist( 0.0f )
	, m_rsHeadingCheck( false )
	, m_rsHeadingLimitCos( 0.0f )
{
}

//////////////////////////////////////////////////////////////////////////

SR4PlayerTargetingConsts::SR4PlayerTargetingConsts()
	: m_softLockDistance( 12.0f )
	, m_softLockFrameSize( 1.25f )
{
}

//////////////////////////////////////////////////////////////////////////

SR4PlayerTargetingPrecalcs::SR4PlayerTargetingPrecalcs()
	: m_cameraDirector( nullptr )
{
}

Bool SR4PlayerTargetingPrecalcs::ObtainCameraDirector()
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_ObtainCameraDirector );
#endif
	m_cameraDirector = nullptr;
	if ( GCommonGame != nullptr && GCommonGame->GetActiveWorld() != nullptr )
	{
		m_cameraDirector = GCommonGame->GetActiveWorld()->GetCameraDirector();
	}
	return m_cameraDirector != nullptr;
}

void SR4PlayerTargetingPrecalcs::Calculate( CR4Player* player )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_CalculatePrecalcs );
#endif

	m_playerPosition = player->GetWorldPositionRef();
	m_playerHeading = player->GetWorldRotation().Yaw;
	m_playerHeadingVector = player->GetLocalToWorld().GetAxisY();
	m_playerHeadingVector.Z = 0.0f;
	m_playerHeadingVector.Normalize2();
	m_playerIsInCombat = player->IsInCombat();

	m_playerRadius = 0.5f;
	if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( player->GetMovingAgentComponent() ) )
	{
		m_playerRadius = mpac->GetCurrentRadius();
	}

	if ( ObtainCameraDirector() )
	{
		m_cameraPosition = m_cameraDirector->GetCameraPosition();
		m_cameraDirection = m_cameraDirector->GetCameraForward();
		m_cameraHeading = m_cameraDirector->GetCameraRotation().Yaw;
		m_cameraHeadingVector = m_cameraDirection;
		m_cameraHeadingVector.Z = 0.0f;
		m_cameraHeadingVector.Normalize2();
	}
	// if no camera found (almost impossible, but... ) let's get some safe values
	else
	{
		m_cameraPosition = m_playerPosition;
		m_cameraDirection = player->GetWorldForward();
		m_cameraHeading = m_playerHeading;
		m_cameraHeadingVector = m_playerHeadingVector;
	}

}

//////////////////////////////////////////////////////////////////////////

SR4PlayerTargetingIn::SR4PlayerTargetingIn()
	: m_canFindTarget( false )
	, m_playerHasBlockingBuffs( false )
	, m_isActorLockedToTarget( false )
	, m_isCameraLockedToTarget( false )
	, m_actionCheck( false )
	, m_actionInput( false )
	, m_isInCombatAction( false )
	, m_isLAxisReleased( false )
	, m_isLAxisReleasedAfterCounter( false )
	, m_isLAxisReleasedAfterCounterNoCA( false )
	, m_lastAxisInputIsMovement( false )
	, m_isAiming( false )
	, m_isSwimming( false )
	, m_isDiving( false )
	, m_isThreatened( false )
	, m_isCombatMusicEnabled( false )
	, m_isPcModeEnabled( false )
	, m_shouldUsePcModeTargeting( false )
	, m_isInParryOrCounter( false )
	, m_bufferActionType( EBAT_EMPTY )
	, m_orientationTarget( OT_None )
	, m_coneDist( 0.0f )
	, m_findMoveTargetDist( 0.0f )
	, m_cachedRawPlayerHeading( 0.0f )
	, m_combatActionHeading( 0.0f )
	, m_rawPlayerHeadingVector( Vector::ZEROS )
	, m_lookAtDirection( Vector::ZEROS )
{
}

//////////////////////////////////////////////////////////////////////////

SR4PlayerTargetingOut::SR4PlayerTargetingOut()
{
	Reset();
}

void SR4PlayerTargetingOut::Reset()
{
	m_target						= nullptr;
	m_result						= false;
	m_confirmNewTarget				= false;
	m_forceDisableUpdatePosition	= false;
}

//////////////////////////////////////////////////////////////////////////

CR4PlayerTargeting::CR4PlayerTargeting()
	: m_player( nullptr )
{
}

CR4PlayerTargeting::~CR4PlayerTargeting()
{
	m_visibleActors.ClearFast();
}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::BeginFindTarget( const SR4PlayerTargetingIn& inValues )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_BeginFindTarget );
#endif

	m_outValues.Reset();

	UpdateVisibleActors();

	m_player = Cast< CR4Player>( GCommonGame->GetPlayer() );
	RED_ASSERT( m_player != nullptr, TXT( "CR4PlayerTargeting::BeginFindTarget: cannot find CR4Player" ) );
	if ( m_player == nullptr )
	{
		return;
	}

	m_precalcs.Calculate( m_player );
	m_inValues = inValues;
}

void CR4PlayerTargeting::EndFindTarget( SR4PlayerTargetingOut& outValues )
{
	outValues = m_outValues;
}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::FindTarget()
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_FindTarget );
#endif

	RED_ASSERT( m_player != nullptr, TXT( "CR4PlayerTargeting.FindPlayer: player is null, you probably should call BeginFindTarget first") );
	if ( m_player == nullptr )
	{
		return;
	}

	CActor* currentTarget = m_player->GetScriptTarget();
	if ( currentTarget != nullptr )
	{
		// if we are locked to alive target
		if ( m_inValues.m_isHardLockedToTarget && currentTarget->IsAlive() && !IsKnockedUnconscious( currentTarget ) )
		{
			const Float hardLockDistSqr = HARD_LOCK_DISTANCE * HARD_LOCK_DISTANCE;
			// if too far from hard lock, let's release it
			if ( m_precalcs.m_playerPosition.DistanceSquaredTo( currentTarget->GetWorldPositionRef() ) > hardLockDistSqr )
			{
				HardLockToTarget( false );
				m_inValues.m_isHardLockedToTarget = false;
			}
			// otherwise return hard locked target
			else
			{
				m_outValues.m_target = currentTarget;
				m_outValues.m_result = true;
				return;
			}
		}
	}

	CActor* newTarget = nullptr;
	if ( m_inValues.m_canFindTarget && !m_inValues.m_isActorLockedToTarget )
	{
		TDynArray< CActor* > targets;
		Vector selectionHeadingVector = Vector::ZEROS;
		if ( !m_inValues.m_playerHasBlockingBuffs )
		{
			FindTargetsInCone( targets, selectionHeadingVector );
		}

		STargetingInfo targetingInfo;
		targetingInfo.m_source				= m_player;
		targetingInfo.m_canBeTargetedCheck	= true;
		targetingInfo.m_coneCheck			= false;
		targetingInfo.m_coneHalfAngleCos	= 1.0f;
		targetingInfo.m_coneDist	 		= m_inValues.m_coneDist;
		targetingInfo.m_distCheck			= true;
		targetingInfo.m_invisibleCheck		= true;
		targetingInfo.m_navMeshCheck		= false;
		if ( m_inValues.m_shouldUsePcModeTargeting )
		{
			targetingInfo.m_inFrameCheck 	= false;
		}
		else
		{
			targetingInfo.m_inFrameCheck 	= true;
		}
		targetingInfo.m_frameScaleX 		= 1.0f;
		targetingInfo.m_frameScaleY 		= 1.f;
		targetingInfo.m_knockDownCheck 		= false;
		targetingInfo.m_knockDownCheckDist 	= 1.5f;
		targetingInfo.m_rsHeadingCheck 		= false;
		targetingInfo.m_rsHeadingLimitCos	= 1.0f;

		if ( currentTarget != nullptr )
		{
			targetingInfo.m_targetEntity = currentTarget;
			if ( !IsEntityTargetable( targetingInfo ) )
			{
				currentTarget = nullptr;
			}
			if ( currentTarget != nullptr && !CanBeTargetedIfSwimming( currentTarget ) )
			{
				currentTarget = nullptr;
			}
		}

		Bool isMoveTargetTargetable = false;
		CActor* moveTarget = m_inValues.m_moveTarget.Get();
		if ( moveTarget != nullptr )
		{
			if ( CanBeTargetedIfSwimming( moveTarget ) )
			{
				targetingInfo.m_targetEntity = moveTarget;
				targetingInfo.m_coneDist = m_inValues.m_findMoveTargetDist;
				targetingInfo.m_inFrameCheck = false;
				if ( IsEntityTargetable( targetingInfo ) )
				{
					isMoveTargetTargetable = true;
				}
			}
		}

		// checking "standard" cone dist again
		targetingInfo.m_coneDist = m_inValues.m_coneDist;
		
		if ( !m_inValues.m_playerHasBlockingBuffs )
		{
			RemoveNonTargetable( targets, targetingInfo, selectionHeadingVector );
		}

		CActor* selectedTarget = nullptr;
		if ( m_inValues.m_isAiming )
		{
			newTarget = m_inValues.m_aimingTarget.Get();
			if ( newTarget == nullptr )
			{
				STargetSelectionWeights selectionWeights;
				selectionWeights.m_angleWeight = 1.0f;
				selectionWeights.m_distanceWeight = 0.0f;
				selectionWeights.m_distanceRingWeight = 0.0f;

				selectedTarget = SelectTarget( targets, false, m_precalcs.m_cameraPosition, m_precalcs.m_cameraHeadingVector, selectionWeights );
				newTarget = selectedTarget;
			}
		}
		else if ( m_inValues.m_isSwimming )
		{
			STargetSelectionWeights selectionWeights;
			selectionWeights.m_angleWeight = 0.9f;
			selectionWeights.m_distanceWeight = 0.1f;
			selectionWeights.m_distanceRingWeight = 0.f;

			newTarget = SelectTarget( targets, true, m_precalcs.m_cameraPosition, m_precalcs.m_cameraHeadingVector, selectionWeights );
		}
		else if ( m_inValues.m_isThreatened )
		{
			// Change locked enemy when the current one becomes invisible
			if ( m_inValues.m_isCameraLockedToTarget )
			{
				if ( currentTarget != nullptr && !GetGameplayVisibility( currentTarget ) )
				{
					ForceSelectLockTarget();
				}
			}

			CActor* displayTarget = m_inValues.m_displayTarget.Get();
			CActor* selectedTarget = SelectTarget( targets, true, m_precalcs.m_playerPosition, selectionHeadingVector, m_inValues.m_defaultSelectionWeights );

			if ( selectedTarget == nullptr )
			{
				m_outValues.m_forceDisableUpdatePosition = true;
			}

			Bool targetChangeFromActionInput = m_inValues.m_actionInput && !m_inValues.m_isLAxisReleasedAfterCounter;
			if ( selectedTarget != nullptr &&
				 ( !IsThreat( currentTarget ) || m_inValues.m_shouldUsePcModeTargeting || ( !m_inValues.m_isInCombatAction && !m_inValues.m_isLAxisReleasedAfterCounterNoCA ) || targetChangeFromActionInput ) )
			{
				newTarget = selectedTarget;
			}
			else if ( displayTarget != nullptr &&
				      ( ( m_inValues.m_isLAxisReleased && !m_inValues.m_shouldUsePcModeTargeting ) || m_inValues.m_isInCombatAction ) &&
				      ( displayTarget->IsAlive() || m_inValues.m_finishableEnemies.Exist( m_inValues.m_displayTarget ) ) &&
					  ( m_player->IsEnemyVisible( displayTarget ) || m_inValues.m_finishableEnemies.Exist( m_inValues.m_displayTarget ) ) &&
					  GetGameplayVisibility( displayTarget ) &&
					  CanBeTargetedIfSwimming( displayTarget ) && 
					  IsThreat( displayTarget ) &&
					  WasVisibleInScaledFrame( displayTarget, 1.f, 1.f ) )
			{
				newTarget = displayTarget;
			}
			// target closest enemy immediately when transitioning from running/sprint to walk/idle,
			// but when you are already in walk/idle player should hit air after he kills his target, he should only target closest enemy when that enemy is within his field of vision
			else if ( moveTarget != nullptr &&
					  isMoveTargetTargetable &&
					  ( !m_inValues.m_isInCombatAction || m_inValues.m_isInParryOrCounter || IsDodgingOrRolling( m_player ) ) )
			{
				newTarget = moveTarget;
			}
			else
			{
				newTarget = nullptr;
			}
		}
		else
		{
			Bool retainCurrentTarget = false;
			selectedTarget = nullptr;
			if ( m_inValues.m_isLAxisReleasedAfterCounterNoCA )
			{
				if ( m_inValues.m_lastAxisInputIsMovement && !m_inValues.m_isSwimming )
				{
					STargetSelectionWeights selectionWeights;
					selectionWeights.m_angleWeight = 0.375f;
					selectionWeights.m_distanceWeight = 0.275f;
					selectionWeights.m_distanceRingWeight = 0.35f;
					selectedTarget = SelectTarget( targets, false, m_precalcs.m_playerPosition, m_precalcs.m_playerHeadingVector, selectionWeights );

					if ( currentTarget != selectedTarget )
					{
						targetingInfo.m_targetEntity = currentTarget;
						if ( IsEntityTargetable( targetingInfo ) && currentTarget->IsAlive() )
						{
							retainCurrentTarget = true;
						}
					}
				}
				else
				{
					STargetSelectionWeights selectionWeights;
					selectionWeights.m_angleWeight = 0.75f;
					selectionWeights.m_distanceWeight = 0.125f;
					selectionWeights.m_distanceRingWeight = 0.125f;
					selectedTarget = SelectTarget( targets, false, m_precalcs.m_cameraPosition, m_precalcs.m_cameraHeadingVector, selectionWeights );
				}
			}
			else
			{
				STargetSelectionWeights selectionWeights;
				selectionWeights.m_angleWeight = 0.6f;
				selectionWeights.m_distanceWeight = 0.4f;
				selectionWeights.m_distanceRingWeight = 0.0f;
				selectedTarget = SelectTarget( targets, true, m_precalcs.m_playerPosition, m_inValues.m_rawPlayerHeadingVector, selectionWeights );
			}

			if ( retainCurrentTarget )
			{
				newTarget = currentTarget;
			}
			else if ( m_inValues.m_isInCombatAction && m_player->GetBehaviorFloatVariable( CNAME( isPerformingSpecialAttack ) ) == 1.0f )
			{
				newTarget = moveTarget;
			}
			else if ( selectedTarget != nullptr )
			{
				newTarget = selectedTarget;
			}
			else
			{
				newTarget = nullptr;
			}
		}
		m_outValues.m_confirmNewTarget = true;
	}
	else
	{
		newTarget = nullptr;	
	}

	m_outValues.m_result = true;
	m_outValues.m_target = newTarget;
}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::UpdateVisibleActors()
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_UpdateVisibleActor );
#endif

	TDynArray< CActor* > actorsToRemove;
	EngineTime now = GGame->GetEngineTime();
	for ( auto& it : m_visibleActors )
	{
		if ( ( now - it.m_second ) > SOFT_LOCK_DISTANCE_VISIBILITY_DURATION )
		{
			actorsToRemove.PushBack( it.m_first );
		}
	}
	for ( CActor* actor : actorsToRemove )
	{
		m_visibleActors.Erase( actor );
	}
}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::FindTargetsInCone( TDynArray< CActor* > & targets, Vector& outHeadingVector )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_FindTargetsInCone );
#endif

	targets.ClearFast();
	m_player->GetVisibleEnemies( targets );

	// add finishable enemies to targets list
	for ( auto& actorHandle : m_inValues.m_finishableEnemies )
	{
		if ( CActor* actor = actorHandle.Get() )
		{
			targets.PushBackUnique( actor );
		}
	}

	Bool onlyThreatTargetsFound = false;
	FilterActors( targets, onlyThreatTargetsFound );

	if ( m_inValues.m_isCombatMusicEnabled && targets.Size() > 0 && !onlyThreatTargetsFound && !IsThreat( targets[ 0 ] ) )
	{
		targets.ClearFast();
	}

	Float coneHeading = 0.0f;
	Float coneHalfAngleDot = 0.0f;
	const Float COS_HALF_120 = 0.5f;			// = Cos( Deg2Rad( 120.f * 0.5f ) );
	const Float COS_HALF_160 = 0.17364817766f;	// = Cos( Deg2Rad( 160.f * 0.5f ) );
	if ( m_inValues.m_orientationTarget == OT_Camera || m_inValues.m_orientationTarget == OT_CameraOffset )
	{
		coneHeading = m_precalcs.m_cameraHeading;
		coneHalfAngleDot = COS_HALF_120;
	}
	else
	{
		if ( m_inValues.m_isSwimming )
		{
			coneHeading = m_precalcs.m_cameraHeading;
			coneHalfAngleDot = COS_HALF_160;
		}
		else if ( m_inValues.m_isLAxisReleased )
		{
			if ( m_inValues.m_isInCombatAction )
			{
				coneHeading = m_inValues.m_combatActionHeading;
			}
			else
			{
				if ( m_inValues.m_shouldUsePcModeTargeting )
				{
					coneHeading = m_precalcs.m_cameraHeading;
				}
				else
				{
					coneHeading = m_inValues.m_cachedRawPlayerHeading;
				}
			}
			if ( m_precalcs.m_playerIsInCombat )
			{
				if ( m_inValues.m_shouldUsePcModeTargeting )
				{
					coneHalfAngleDot = -1.0f; // full angle
				}
				else
				{
					coneHalfAngleDot = COS_HALF_160;
				}
			}
			else
			{
				coneHalfAngleDot = -1.0f; // full angle
			}
		}
		else
		{
			if ( m_inValues.m_isInCombatAction )
			{
				coneHeading = m_inValues.m_combatActionHeading;
			}
			else
			{
				if ( m_inValues.m_shouldUsePcModeTargeting )
				{
					coneHeading = m_precalcs.m_cameraHeading;
				}
				else
				{
					coneHeading = m_inValues.m_cachedRawPlayerHeading;
				}
			}
			if ( m_inValues.m_shouldUsePcModeTargeting )
			{
				coneHalfAngleDot = -1.0f; // full angle
			}
			else
			{
				coneHalfAngleDot = COS_HALF_160;
			}
		}

		Vector coneHeadingVector = Vector::ZEROS;
		coneHeadingVector.X = -MSin( DEG2RAD( coneHeading ) );
		coneHeadingVector.Y = MCos( DEG2RAD( coneHeading ) );

		for ( Int32 i = targets.SizeInt() - 1; i >= 0; i-- )
		{
			const Vector playerToTargetNormalized = ( targets[ i ]->GetWorldPositionRef() - m_precalcs.m_playerPosition ).Normalized2();
			if ( Vector::Dot2( coneHeadingVector, playerToTargetNormalized ) < coneHalfAngleDot )
			{
				targets.RemoveAtFast( i );
			}
		}

		outHeadingVector = coneHeadingVector;
	}

}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::RemoveNonTargetable( TDynArray< CActor* > & targets, STargetingInfo& info, const Vector& selectionHeadingVector )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_RemoveNonTargetable );
#endif

	const Float playerToCamPlaneDist = m_precalcs.m_cameraDirector != nullptr ? Vector::Dot2( m_precalcs.m_cameraDirection, m_precalcs.m_playerPosition - m_precalcs.m_cameraPosition ) : 0.0f;

	if ( targets.Size() == 0 )
	{
		return;
	}

	const Bool nonCombatCheck = m_inValues.m_isLAxisReleased && !m_precalcs.m_playerIsInCombat;
	const Float COS_HALF_80 = 0.76604444311f;	// = Cos( Deg2Rad( 80.f * 0.5f ) );
	const Float COS_HALF_60 = 0.86602540378f;	// = Cos( Deg2Rad( 60.f * 0.5f ) );

	// prepare targeting info
	if ( nonCombatCheck )
	{
		info.m_coneHeadingVector = m_precalcs.m_playerHeadingVector;
		if ( m_inValues.m_lastAxisInputIsMovement )
		{
			info.m_coneHeadingVector	= selectionHeadingVector;
			info.m_invisibleCheck		= false;
			info.m_coneCheck 			= true;
			info.m_coneHalfAngleCos		= COS_HALF_80;
		}
		else
		{
			info.m_invisibleCheck	= false;
			info.m_frameScaleX 		= 0.9f;
			info.m_frameScaleY 		= 0.9f;
		}
	}
	else
	{
		info.m_coneHeadingVector = Vector::EY;
		if ( m_precalcs.m_playerIsInCombat )
		{
			info.m_inFrameCheck = false;
		}
		else
		{
			if ( !m_inValues.m_isLAxisReleased )
			{
				info.m_coneCheck = true;
				if ( m_inValues.m_isSwimming )
				{
					info.m_coneHalfAngleCos	= -1.0f; // full angle
				}
				else
				{
					info.m_coneHalfAngleCos	= COS_HALF_60;
				}
				info.m_coneHeadingVector = m_inValues.m_rawPlayerHeadingVector;
			}
		}
	}

	// filtering targets using targeting info
	for ( Int32 i = targets.SizeInt() - 1; i >= 0; i-- )
	{
		info.m_targetEntity = targets[ i ];

		if ( !CanBeTargetedIfSwimming( targets[ i ] ) )
		{
			targets.RemoveAtFast( i );
		}
		else if ( !IsEntityTargetable( info ) )
		{
			targets.RemoveAtFast( i );
		}
		else
		{
			if ( nonCombatCheck && !m_inValues.m_lastAxisInputIsMovement )
			{
				if ( m_precalcs.m_cameraDirector != nullptr )
				{
					const Float targetToCamPlaneDist = Vector::Dot2( m_precalcs.m_cameraDirection, targets[ i ]->GetWorldPosition() - m_precalcs.m_cameraPosition );
					if ( targetToCamPlaneDist < playerToCamPlaneDist )
					{
						targets.RemoveAtFast( i );
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

CActor* CR4PlayerTargeting::SelectTarget( TDynArray< CActor* > & targets, Bool useVisibilityCheck, const Vector& sourcePosition, const Vector& headingVector, STargetSelectionWeights& selectionWeights )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_SelectTarget );
#endif

	if ( targets.Size() == 0 )
	{
		return nullptr;
	}

	if ( useVisibilityCheck )
	{
		CActor* currentTarget = m_player->GetTarget();
		EngineTime now = GGame->GetEngineTime();

		for ( Int32 i = targets.SizeInt() - 1; i >= 0; i-- )
		{
			CActor* target = targets[ i ];
			if ( target != currentTarget && !m_inValues.m_isPcModeEnabled && !WasVisibleInScaledFrame( target, m_consts.m_softLockFrameSize, m_consts.m_softLockFrameSize ) )
			{
				Bool remove = true;
				TVisibleActors::iterator it = m_visibleActors.Find( target );
				if ( it != m_visibleActors.End() )
				{
					Float distanceToPlayer = m_precalcs.m_playerPosition.DistanceTo2D( target->GetWorldPositionRef() ) - m_precalcs.m_playerRadius;
					if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( targets[ i ]->GetMovingAgentComponent() ) )
					{
						distanceToPlayer -= mpac->GetRadius();
					}
					// if withing soft lock distance and soft lock visibility duration -> don't remove yet
					if ( distanceToPlayer < m_consts.m_softLockDistance && ( now - it->m_second ) < SOFT_LOCK_DISTANCE_VISIBILITY_DURATION )					
					{
						remove = false;
					}
				}
				if ( remove )
				{
					targets.RemoveAtFast( i );
				}
			}
			else
			{
				m_visibleActors[ targets[ i ] ] = now;
			}
		}
	}

	CActor* selectedTarget = nullptr;
	Float maxPriority = -1.0f;
	STargetSelectionData selectionData;
	selectionData.m_sourcePosition = sourcePosition;
	selectionData.m_headingVector = headingVector;
	selectionData.m_softLockDistance = m_consts.m_softLockDistance;
	for ( Int32 i = targets.SizeInt() - 1; i >= 0; i-- )
	{
		Float priority = CTargetingUtils::CalcSelectionPriority( targets[ i ], selectionWeights, selectionData );
		if ( priority > maxPriority )
		{
			maxPriority = priority;
			selectedTarget = targets[ i ];
		}
	}

	return selectedTarget;
}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::FilterActors( TDynArray< CActor* > & targets, Bool& onlyThreatTargetsFound )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_FilterActors );
#endif

	Bool foundThreat = false;
	Bool foundNonThreat = false;
	Uint32 threatsCount = 0;
	
	const Uint32 size = targets.Size();
	for ( Uint32 i = 0; i < size; i++ )
	{
		if ( IsThreat( targets[ i ] ) )
		{
			foundThreat = true;
			if ( i != threatsCount )
			{
				::Swap( targets[ i ], targets[ threatsCount ] );
			}
			threatsCount++;
		}
		else
		{
			foundNonThreat = true;
		}
	}
	if ( foundThreat )
	{
		onlyThreatTargetsFound = true;
		if ( foundNonThreat )
		{
			targets.Resize( threatsCount );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CR4PlayerTargeting::IsEntityTargetable( STargetingInfo& info )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_IsEntityTargetable );
#endif

	if ( m_inValues.m_playerHasBlockingBuffs )
	{
		return false;
	}

	CActor* sourceActor = info.m_source.Get();
	CEntity* targetEntity = info.m_targetEntity.Get();
	if ( sourceActor == nullptr || targetEntity == nullptr )
	{
		return false;
	}

	CActor* targetActor = Cast< CActor >( targetEntity );
	const Vector sourcePosition = sourceActor->GetWorldPositionRef();
	const Vector targetPosition = targetEntity->GetWorldPositionRef();

	if ( targetActor != nullptr )
	{
		// do not target mounted horses
		W3HorseComponent* horseComponent = targetActor->FindComponent< W3HorseComponent >();
		if ( horseComponent != nullptr && !horseComponent->IsDismounted() )
		{
			return false;
		}
	}

	Float sourceToTargetDist = 0.0f;
	if ( info.m_distCheck || info.m_knockDownCheck )
	{
		sourceToTargetDist = sourcePosition.DistanceTo2D( targetPosition ) - m_precalcs.m_playerRadius;
		if ( targetActor != nullptr )
		{
			if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( targetActor->GetMovingAgentComponent() ) )
			{
				sourceToTargetDist -= mpac->GetCurrentRadius();
			}
		}
	}

	// check distance
	if ( info.m_distCheck )
	{
		if ( sourceToTargetDist >= info.m_coneDist )
		{
			return false;
		}
	}

	// prepare sourceToTarget vector if needed
	const Vector sourceToTarget = ( info.m_coneCheck || info.m_rsHeadingCheck ) ? ( targetPosition - sourcePosition ).Normalized2() : Vector::ZEROS;

	// cone check
	if ( info.m_coneCheck )
	{
		if ( Vector::Dot2( sourceToTarget, info.m_coneHeadingVector ) < info.m_coneHalfAngleCos )
		{
			return false;
		}
	}

	// heading cone check
	if ( info.m_rsHeadingCheck )
	{
		if ( Vector::Dot2( sourceToTarget, m_inValues.m_lookAtDirection ) < info.m_rsHeadingLimitCos )
		{
			return false;
		}
	}

	// "can be targeted" check
	if ( info.m_canBeTargetedCheck && !CanBeTargeted( targetActor ) )
	{
		return false;
	}

	// visibility check
	if ( info.m_invisibleCheck && !GetGameplayVisibility( targetActor ) )
	{
		return false;
	}

	// "in frame" check
	if ( info.m_inFrameCheck && !WasVisibleInScaledFrame( targetEntity, info.m_frameScaleX, info.m_frameScaleY ) )
	{
		return false;
	}

	// navmesh check
	if ( info.m_navMeshCheck && !m_inValues.m_isSwimming )
	{
		if ( GGame->GetActiveWorld() == nullptr )
		{
			return false;
		}
		CPathLibWorld* pathLib = GGame->GetActiveWorld()->GetPathLibWorld();
		if ( pathLib == nullptr || !pathLib->TestLine( sourcePosition, targetPosition, m_precalcs.m_playerRadius, PathLib::CT_DEFAULT ) )
		{
			return false;
		}
	}

	// knockdown check
	if ( info.m_knockDownCheck )
	{
		// if actor is not alive
		if ( targetActor != nullptr && !targetActor->IsAlive() )
		{
			// and contains enabled "Finish" interaction
			CInteractionComponent* ic = Cast< CInteractionComponent >( targetActor->FindComponent( CNAME( Finish ) ) );
			if ( ic != nullptr && ic->IsEnabled() )
			{
				// and is contained in finishable enemies list
				if ( m_inValues.m_finishableEnemies.Exist( targetActor ) )
				{
					// and is too far to "finish" -> we cannot target it
					if ( sourceToTargetDist >= info.m_knockDownCheckDist )
					{
						return false;
					}
				}
			}
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool CR4PlayerTargeting::CanBeTargetedIfSwimming( CActor* actor )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_CanBeTargetedIfSwimming );
#endif

	if ( actor == nullptr )
	{
		return false;
	}

	Float subDepth = 0.0f;
	if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() ) )
	{
		subDepth = mpac->GetSubmergeDepth();
	}

	if ( m_inValues.m_isDiving )
	{
		return ( subDepth < -1.0f );
	}
	else
	{
		return ( subDepth >= -1.0f );
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CR4PlayerTargeting::IsThreat( CActor* actor )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_IsThreat );
#endif

	if ( actor == nullptr )
	{
		return false;
	}

	if ( m_inValues.m_finishableEnemies.Exist( actor ) )
	{
		return true;
	}

	if ( !actor->IsAlive() || IsKnockedUnconscious( actor ) )
	{
		return false;
	}

	if ( actor->FindComponent< W3HorseComponent >() )
	{
		return false;
	}

	if ( m_inValues.m_hostileEnemies.Exist( actor ) )
	{
		return true;
	}

	if ( m_player->GetAttitude( actor ) == AIA_Hostile )
	{
		Float targetCapsuleHeight = 1.0f;
		Float distance = m_player->GetWorldPositionRef().DistanceTo2D( actor->GetWorldPositionRef() ) - m_precalcs.m_playerRadius;
		if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() ) )
		{
			distance -= mpac->GetCurrentRadius();
			targetCapsuleHeight = mpac->GetHeight();
		}
		const Float shortDistance = m_inValues.m_findMoveTargetDist + 5.0f;
		if ( distance < shortDistance )
		{
			return true;
		}
		if ( actor->IsInCombat() || m_inValues.m_isHardLockedToTarget )
		{
			if ( targetCapsuleHeight > 2.0f || IsFlying( actor ) )
			{
				if ( distance < IS_THREAT_EXPANDED_DISTANCE )
				{
					return true;
				}
			}
		}
	}

	if ( actor->GetAttitudeGroup() == CNAME( npc_charmed ) )
	{
		if ( CAttitudeManager* am = GCommonGame->GetSystem< CAttitudeManager >() )
		{
			EAIAttitude attitude = AIA_Neutral;
			Bool isCustom = false;
			if ( am->GetAttitude( m_player->GetBaseAttitudeGroup(), actor->GetBaseAttitudeGroup(), attitude, isCustom ) && attitude == AIA_Hostile )
			{
				return true;
			}
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CR4PlayerTargeting::WasVisibleInScaledFrame( CEntity* entity, Float frameSizeX, Float frameSizeY )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_WasVisibleInScaleFrame );
#endif

	if ( entity == nullptr )
	{
		return false;
	}
	if ( frameSizeX <= 0.0f && frameSizeY <= 0.0f )
	{
		return false;
	}
	if ( m_precalcs.m_cameraDirector == nullptr )
	{
		if ( !m_precalcs.ObtainCameraDirector() )
		{
			return false;
		}
	}

	Vector position = entity->GetWorldPositionRef();
	Bool positionFound = false;

	if ( CActor* actor = Cast< CActor >( entity ) )
	{
		Int32 boneIndex = -1;
		if ( CAnimatedComponent* anim = actor->GetRootAnimatedComponent() )
		{
			boneIndex = anim->FindBoneByName( CNAME( pelvis ) );
			if ( boneIndex < 0 )
			{
				boneIndex = anim->FindBoneByName( CNAME( k_pelvis_g ) );	// well...
			}
			if ( boneIndex >= 0 )
			{
				position = anim->GetBoneMatrixWorldSpace( static_cast< Uint32 >( boneIndex ) ).GetTranslationRef();
			}
		}
		if ( boneIndex < 0 )
		{
			position = entity->GetWorldPositionRef();
			Float capsuleHeight = 0.0f;
			if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() ) )
			{
				capsuleHeight = mpac->GetHeight();
			}
			if ( capsuleHeight <= 0.0f )
			{
				capsuleHeight = 2.0f;
			}
			position.Z += capsuleHeight * 0.5f;
		}
		positionFound = true;
	}
	else if ( CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( entity ) )
	{
		if ( !gameplayEntity->GetAimVector().AsVector3().IsAlmostZero() )
		{
			position = gameplayEntity->GetLocalToWorld().TransformPoint( gameplayEntity->GetAimVector() );
			positionFound = true;
		}
	}

	// if still not found proper position for test
	if ( !positionFound )
	{
		position = entity->GetWorldPositionRef();
		Box box;
		if ( GetObjectBoundingVolume( entity, box ) )
		{
			position.Z += ( box.CalcSize().Z * 0.66f );
		}
	}

	Float x = 0.0f;
	Float y = 0.0f;
	Bool inFront = m_precalcs.m_cameraDirector->WorldVectorToViewRatio( position, x, y );
	if ( !inFront )
	{
		return false;
	}

	x = MAbs( x );
	y = MAbs( y );

	Bool ok = true;
	ok = ok && ( frameSizeX <= 0 || x < frameSizeX );
	ok = ok && ( frameSizeY <= 0 || y < frameSizeY );

	return ok;
}

//////////////////////////////////////////////////////////////////////////

Bool CR4PlayerTargeting::GetObjectBoundingVolume( CEntity* entity, Box& box )
{
#ifdef PROFILE_TARGETING
	PC_SCOPE_PIX( Targeting_GetPhysicalObjectBoundingVolume );
#endif

	if ( entity == nullptr )
	{
		return false;
	}

	if ( CDrawableComponent* dc = entity->FindComponent< CDrawableComponent >() )
	{
		// Bacause calculating world bound for CDestructionSystemComponent may be expensive (many actors/shapes),
		// let lets try to use bounding box directly from drawable component first
		if ( dc->IsA< CDestructionSystemComponent >() )
		{
			box = dc->GetBoundingBox();
			if ( !box.IsEmpty() )
			{
				return true;
			}
		}
		// if this is not enough, calculate physical bounding box
		CPhysicsWrapperInterface* wrapper = dc->GetPhysicsRigidBodyWrapper();
		if ( wrapper == nullptr )
		{
			return false;
		}
		if ( CStaticMeshComponent* mesh = Cast< CStaticMeshComponent >( dc ) )
		{
			Int32 index = mesh->GetPhysicsBodyIndex();
			if ( index < 0 )
			{
				return false;
			}
			box = wrapper->GetWorldBounds( index );
		}
		else
		{
			box = wrapper->GetWorldBounds();
		}
		return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CR4PlayerTargeting::GetGameplayVisibility( CActor* actor ) const
{
	if ( actor == nullptr )
	{
		return false;
	}
	Bool ret = false;
	CallFunctionRet( actor, CNAME( GetGameplayVisibility ), ret );
	return ret;
}

Bool CR4PlayerTargeting::IsKnockedUnconscious( CActor* actor ) const
{
	if ( actor == nullptr )
	{
		return false;
	}
	Bool ret = false;
	CallFunctionRet( actor, CNAME( IsKnockedUnconscious ), ret );
	return ret;
}

Bool CR4PlayerTargeting::CanBeTargeted( CActor* actor ) const
{
	if ( actor == nullptr )
	{
		return false;
	}
	Bool ret = false;
	CallFunctionRet( actor, CNAME( CanBeTargeted ), ret );
	return ret;
}

Bool CR4PlayerTargeting::IsFlying( CActor* actor ) const
{
	if ( actor == nullptr )
	{
		return false;
	}

	const Uint32 stance = static_cast< Uint32 >( actor->GetBehaviorFloatVariable( CNAME( npcStance ) ) );
	return stance == static_cast< Uint32 >( NS_Fly );
}

Bool CR4PlayerTargeting::IsDodgingOrRolling( CActor* actor ) const
{
	if ( actor == nullptr )
	{
		return false;
	}

	Uint32 actionType = static_cast< Uint32 >( actor->GetBehaviorFloatVariable( CNAME( combatActionType ) ) );
	return actionType == static_cast< Uint32 >( CAT_Dodge ) || actionType == static_cast< Uint32 >( CAT_Roll );
}

void CR4PlayerTargeting::HardLockToTarget( Bool lock )
{
	if ( m_player == nullptr )
	{
		return;
	}
	CallFunction( m_player, CNAME( HardLockToTarget ), lock );
}

void CR4PlayerTargeting::ForceSelectLockTarget()
{
	if ( m_player == nullptr )
	{
		return;
	}
	CallFunction( m_player, CNAME( ForceSelectLockTarget ) );
}

//////////////////////////////////////////////////////////////////////////

void CR4PlayerTargeting::funcSetConsts( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SR4PlayerTargetingConsts, consts, SR4PlayerTargetingConsts() );
	FINISH_PARAMETERS;

	m_consts = consts;
}

void CR4PlayerTargeting::funcBeginFindTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SR4PlayerTargetingIn, inValues, SR4PlayerTargetingIn() );
	FINISH_PARAMETERS;

	BeginFindTarget( inValues );
}

void CR4PlayerTargeting::funcEndFindTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( SR4PlayerTargetingOut, outValues, SR4PlayerTargetingOut() );
	FINISH_PARAMETERS;

	EndFindTarget( outValues );
}

void CR4PlayerTargeting::funcFindTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	FindTarget();
}

void CR4PlayerTargeting::funcWasVisibleInScaledFrame( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, hEntity, THandle< CEntity >() );
	GET_PARAMETER( Float, frameSizeX, 0.0f );
	GET_PARAMETER( Float, frameSizeY, 0.0f );
	FINISH_PARAMETERS;

	Bool res = false;
	CEntity* entity = hEntity.Get();
	if ( entity != nullptr )
	{
		res = WasVisibleInScaledFrame( entity, frameSizeX, frameSizeY );
	}

	RETURN_BOOL( res );
}