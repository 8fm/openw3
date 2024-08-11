/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "movingAgentComponent.h"

#include "../engine/behaviorGraphOutput.h"
#include "../engine/visualDebug.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../engine/behaviorGraphUtils.inl"
#include "../physics/physicsWorldUtils.h"
#include "../engine/pathlibWorld.h"
#include "../engine/skeleton.h"
#include "../engine/speedConfig.h"

#include "actionAreaComponent.h"
#include "actor.h"
#include "aiHistory.h"
#include "movingPhysicalAgentComponent.h"
#include "moveSteeringBehavior.h"
#include "moveLocomotion.h"
#include "movableRepresentationEntity.h"
#include "movableRepresentationsStack.h"
#include "movementTargeter.h"
#include "moveGlobalPathPlanner.h"
#include "movableRepresentationPathAgent.h"
#include "movementAdjustor.h"
#include "roadsManager.h"


IMPLEMENT_ENGINE_CLASS( CMovingAgentComponent );
IMPLEMENT_ENGINE_CLASS( SAnimationProxyData );
IMPLEMENT_RTTI_BITFIELD( EMovementFlags );

RED_DEFINE_STATIC_NAME( OnPushed )
RED_DEFINE_STATIC_NAME( OnMovementCollision )

/////////////////////////////////////////////////////////////////////

void SAnimationProxyData::Update( Float timeDelta )
{
	Float targetIKWeight = m_useIK && m_enabledIKDueToLOD && m_enabledIKDueToWork? 1.0f : 0.0f;
	m_useIKWeight = BlendOnOffWithSpeedBasedOnTime( m_useIKWeight, targetIKWeight, m_useIKBlendTime, timeDelta );
	m_appliedIKOffsetWS = Vector::ZEROS;
	m_numStoredTransforms = 0;
	m_numStoredAnimEvents = 0;
	if ( m_numStoredSynchronizations )
	{
		// advance by one frame to allow some systems use result from previous frame
		Bool anyStoreActive = false;
		for ( SAnimationProxyStoredSynchronizationData *storedSync = m_storedSynchronizations, *endSync = &m_storedSynchronizations[ m_numStoredSynchronizations ]; storedSync != endSync; ++ storedSync )
		{
			++ storedSync->m_framesNotActive;
			if ( storedSync->m_framesNotActive < 2 )
			{
				anyStoreActive = true;
			}
		}
		if ( ! anyStoreActive )
		{
			m_numStoredSynchronizations = 0;
		}
	}
}

void SAnimationProxyData::StoreTransform( CName const & storeName, AnimQsTransform const & transform )
{
	SAnimationProxyStoredTransformData * store = m_storedTransforms;
	Uint32 idx;
	for ( idx = 0; idx < m_numStoredTransforms; ++ idx, ++ store )
	{
		if ( store->m_storeName == storeName )
		{
			break;
		}
	}
	ASSERT( idx < NUM_STORED_TRANSFORMS, TXT("Exceeded number of stored transforms! Increase that const value") );
#ifndef RED_FINAL_BUILD
	if ( idx >= NUM_STORED_TRANSFORMS )
	{
		RED_WARNING( idx < NUM_STORED_TRANSFORMS, "Exceeded number of stored transforms! Increase that const value" );
		return;
	}
#endif
	store->m_transform = transform;
	if ( idx == m_numStoredTransforms )
	{
		++ m_numStoredTransforms;
		store->m_storeName = storeName;
	}
}

Bool SAnimationProxyData::GetTransform( CName const & storeName, AnimQsTransform & outTransform ) const
{
	const SAnimationProxyStoredTransformData * store = m_storedTransforms;
	Uint32 idx;
	for ( idx = 0; idx < m_numStoredTransforms; ++ idx, ++ store )
	{
		if ( store->m_storeName == storeName )
		{
			outTransform = store->m_transform;
			return true;
		}
	}
	return false;
}

void SAnimationProxyData::StoreSynchronization( CName const & storeName, CSyncInfo const & syncInfo )
{
	SAnimationProxyStoredSynchronizationData * store = m_storedSynchronizations;
	Uint32 idx;
	for ( idx = 0; idx < m_numStoredSynchronizations; ++ idx, ++ store )
	{
		if ( store->m_storeName == storeName || store->m_framesNotActive > 1 )
		{
			break;
		}
	}
	ASSERT( idx < NUM_STORED_SYNCHRONIZATIONS, TXT("Exceeded number of stored synchronizations! Increase that const value") );
#ifndef RED_FINAL_BUILD
	if ( idx >= NUM_STORED_SYNCHRONIZATIONS )
	{
		RED_WARNING( idx < NUM_STORED_SYNCHRONIZATIONS, "Exceeded number of stored synchronizations! Increase that const value" );
		return;
	}
#endif
	store->m_syncInfo = syncInfo;
	store->m_framesNotActive = 0;
	store->m_storeName = storeName;
	if ( idx == m_numStoredSynchronizations )
	{
		++ m_numStoredSynchronizations;
	}
}

Bool SAnimationProxyData::GetSynchronization( CName const & storeName, CSyncInfo & outSyncInfo ) const
{
	const SAnimationProxyStoredSynchronizationData * store = m_storedSynchronizations;
	Uint32 idx;
	for ( idx = 0; idx < m_numStoredSynchronizations; ++ idx, ++ store )
	{
		if ( store->m_storeName == storeName &&
			 store->m_framesNotActive <= 1 )
		{
			outSyncInfo = store->m_syncInfo;
			return true;
		}
	}
	return false;
}

void SAnimationProxyData::StoreAnimEvent( CAnimationEventFired const & animEvt )
{
	const CName storeName = animEvt.GetEventName();
	CAnimationEventFired * store = m_storedAnimEvents;
	Uint32 idx;
	for ( idx = 0; idx < m_numStoredAnimEvents; ++ idx, ++ store )
	{
		if ( store->GetEventName() == storeName )
		{
			break;
		}
	}
	ASSERT( idx < NUM_STORED_ANIM_EVENTS, TXT("Exceeded number of stored anim events! Increase that const value") );
#ifndef RED_FINAL_BUILD
	if ( idx >= NUM_STORED_ANIM_EVENTS )
	{
		RED_WARNING( idx < NUM_STORED_ANIM_EVENTS, "Exceeded number of stored anim events! Increase that const value" );
		return;
	}
#endif
	*store = animEvt;
	if ( idx == m_numStoredAnimEvents )
	{
		++ m_numStoredAnimEvents;
	}
}

Bool SAnimationProxyData::GetAnimEvent( CName const & storeName, CAnimationEventFired & animEvt ) const
{
	const CAnimationEventFired * store = m_storedAnimEvents;
	Uint32 idx;
	for ( idx = 0; idx < m_numStoredAnimEvents; ++ idx, ++ store )
	{
		const CName animEvtName = store->GetEventName();

		if ( animEvtName == storeName )
		{
			animEvt = *store;
			return true;
		}
	}
	return false;
}

void SAnimationProxyData::SetUseEntityForPelvisOffset( CEntity * entity )
{
	if ( m_useEntityForPelvisOffset )
	{
		if ( CMovingAgentComponent * mac = Cast< CMovingAgentComponent > (m_useEntityForPelvisOffset->GetRootAnimatedComponent() ) )
		{
			mac->AccessAnimationProxy().DontProvidePelvisBoneData();
		}
	}
	m_useEntityForPelvisOffset = entity;
	if ( m_useEntityForPelvisOffset.Get() )
	{
		if ( CMovingAgentComponent * mac = Cast< CMovingAgentComponent > (m_useEntityForPelvisOffset->GetRootAnimatedComponent() ) )
		{
			mac->AccessAnimationProxy().ProvidePelvisBoneData();
		}
	}
}

void SAnimationProxyData::OnPreProcessPoseConstraints( CAnimatedComponent * ac, SBehaviorGraphOutput& output )
{
	if ( m_pelvisBoneRequestedCount )
	{
		ASSERT( m_pelvisBoneIdx != -1, TXT("Pelvis bone not provided for offset, but required") );
		m_preConstraintsPelvisBoneMS = output.GetBoneModelTransform( ac, m_pelvisBoneIdx );
		m_pelvisBoneDataValid = m_pelvisBoneIdx != -1;
	}
	else
	{
		m_pelvisBoneDataValid = false;
	}
}

void SAnimationProxyData::OnPostProcessPoseConstraints( CAnimatedComponent * ac, SBehaviorGraphOutput& output )
{
	if ( m_pelvisBoneIdx != -1 && m_pelvisBoneDataValid && m_pelvisBoneRequestedCount )
	{
		m_postConstraintsPelvisBoneMS = output.GetBoneModelTransform( ac, m_pelvisBoneIdx );
	}
}

void SAnimationProxyData::SetAdditionalOffsetMS( CEntity* attachingTo, Float time )
{
	if ( m_owner->GetEntity() && attachingTo )
	{
		SetAdditionalOffsetMSPointWS( attachingTo->GetLocalToWorld(), time );
	}
	else
	{
		m_additionalOffsetTimeLeft = 0.0f;
	}
}

void SAnimationProxyData::AddAdditionalOffsetMS( Vector const & offsetLocMS, EulerAngles const & offsetRotMS, Float time )
{
	if ( m_additionalOffsetTimeLeft <= 0.f )
	{
		SetAdditionalOffsetMS( offsetLocMS, offsetRotMS, time );
		return;
	}
	m_additionalOffsetLocationMS += offsetLocMS;
	m_additionalOffsetRotationMS += offsetRotMS;
	m_additionalOffsetTimeLeft = Max( m_additionalOffsetTimeLeft, time );
}

void SAnimationProxyData::SetAdditionalOffsetMSPointWS( const Matrix& pointWS, Float time )
{
	if ( m_owner->GetEntity() )
	{
		const Matrix& preAttachLocalToWorld = m_owner->GetEntity()->GetLocalToWorld();
		const Matrix& postAttachLocalToWorld = pointWS;
		Matrix additionalOffsetMS = Matrix::Mul( postAttachLocalToWorld.Inverted(), preAttachLocalToWorld );

		SetAdditionalOffsetMS( additionalOffsetMS.GetTranslation(), additionalOffsetMS.ToEulerAngles(), time );
	}
	else
	{
		m_additionalOffsetTimeLeft = 0.0f;
	}
}

/////////////////////////////////////////////////////////////////////

CMovingAgentComponent::CMovingAgentComponent()
	: m_pathAgent( nullptr )
	, m_steeringBehavior( nullptr )
	, m_locomotion( nullptr )
	, m_steeringListener( nullptr )
	, m_prevPosition( FLT_MIN, FLT_MIN, FLT_MIN )
	, m_lastNavigableAreaPosition( FLT_MIN, FLT_MIN, FLT_MIN )
	, m_moveType( MT_Run )
	, m_movementFlags( 0 )
	, m_staticTarget( nullptr )
	, m_maxSpeed( 2.0f )
	, m_allowExceedingMaxSpeedCoef( 1.00f ) // TODO increase it after all behavior graphs have updated thresholds
	, m_maxRotation( 360.0f )
	, m_rotationSet( MRT_None )
	, m_steeringRuntimeData( nullptr )
	, m_currentSteeringBehavior( nullptr )
	, m_currentSteeringRuntimeData( nullptr )
	, m_ragdollRadius( 2.0f )
	, m_meshRepresentation( nullptr )
	, m_representationStack( nullptr )
	, m_ragdollRepresentation( nullptr )
	, m_triggerAutoActivator( true ) // enable fallback for the old system
	, m_triggerEnableCCD( false )
	, m_triggerActivatorRadius( 0.3f )
	, m_triggerActivatorHeight( 1.8f )
	, m_triggerActivator( nullptr )
	, m_triggerChannels( TC_Default )
	, m_triggerMaxSingleFrameDistance( 10.0f )
	, m_velocity( 0, 0, 0, 1 )
	, m_velocityBasedOnRequestedMovement( 0, 0, 0, 1 )
	, m_deltaPositionFromBehaviorGraph( 0, 0, 0, 1 )
	, m_lastRelMovementSpeed( 0.0f )
	, m_directionChangeRate( 360.0f )
	, m_desiredDirection( 0.0f )
	, m_desiredAbsoluteSpeed( 0.0f )
	, m_gameplayRelativeMoveSpeed( 0.0f )
	, m_gameplayMoveDirection( 0.0f )
	, m_acceleration( FLT_MAX )
	, m_deceleration( FLT_MAX )
	, m_currentSpeedVal( 0.0f )
	, m_rotationTargetCooldown( 0.0f )
	, m_isDisabled( 0 )
	, m_isEntityRepresentationForced( 0 )
	, m_physicalRepresentationEnableRequest( 0 )
	, m_physicalRepresentationDisableRequest( 0 )
	, m_pullToNavigableSpace( false )
	, m_snapToNavigableSpace( true )
	, m_steeringControlledMovement( false )
	, m_physicalRepresentation( false )
	, m_characterCollisionsEnabled( true )
	, m_isCollidable( true )
	, m_moved( false )
	, m_slide( false )
	, m_isColliding( false )
	, m_teleported( true )
	, m_teleportCorrectZ( true )
	, m_enableAfterTeleport( true )
	, m_directionSet( false )
	, m_gameplayDirectionSet( false )
	, m_ragdollObstacle( false )
	, m_wasSeparatedBy( nullptr )
	, m_avoidanceRadius( 0.0f )
	, m_representationResetRequest( false )
{
	m_staticTarget = new CStaticMovementTargeter( *this );
	m_meshRepresentation = new CMREntity( *this );
	m_representationStack = new CMRStack( *this );
	m_movementAdjustor = CreateObject< CMovementAdjustor >( this );
	m_animationProxyData.SetOwner( this );
}

CMovingAgentComponent::~CMovingAgentComponent()
{
	if ( m_staticTarget )
	{
		m_staticTarget->Release();
		m_staticTarget = nullptr;
	}

	if ( m_locomotion )
	{
		delete m_locomotion;
		m_locomotion = nullptr;
	}

	delete m_ragdollRepresentation;
	m_ragdollRepresentation = nullptr;

	delete m_meshRepresentation;
	m_meshRepresentation = nullptr;

	delete m_representationStack;
	m_representationStack = nullptr;
}

void CMovingAgentComponent::OnDestroyed()
{
	CEntity* ent = GetEntity();
	if ( ent )
	{
		CActor* act = Cast< CActor > ( ent );
		if ( act )
		{
			act->OnMovingAgentComponentChanged();
		}
	}

	if ( m_movementAdjustor )
	{
		m_movementAdjustor->CancelAll();
		m_movementAdjustor->Discard();
		m_movementAdjustor = nullptr;
	}
	
	TBaseClass::OnDestroyed();
}

void CMovingAgentComponent::SnapToNavigableSpace( Bool b )
{
	if ( m_snapToNavigableSpace != b )
	{
		m_snapToNavigableSpace = b;
		if( b && m_pathAgent )
		{
			m_pathAgent->CheckIfIsOnNavdata();
		}
	}
}

void CMovingAgentComponent::PullToNavigableSpace()
{
	m_pullToNavigableSpace = true;

	m_pullToNavSpaceTimeLimit = GGame->GetEngineTime() + 5.f;
}

void CMovingAgentComponent::StopPullingToNavigableSpace()
{
	m_pullToNavigableSpace = false;
}

Bool CMovingAgentComponent::IsPulledToNavigableSpace()
{
	if ( m_pullToNavigableSpace )
	{
		if ( m_pullToNavSpaceTimeLimit > GGame->GetEngineTime() )
		{
			return true;
		}
		m_pullToNavigableSpace = false;
	}
	return false;
}


Float CMovingAgentComponent::GetSpeedForMoveType( EMoveType type, Float absSpeed ) const
{
	CSkeleton* skeleton = GetSkeleton();
	if ( !skeleton )
	{
		return 0.0f;
	}
	Float speed = 0.0f;
	switch ( type )
	{
	case MT_Walk:
		{
			if ( m_speedConfig )
			{
				speed = m_speedConfig->GetWalkSpeedAbs();
			}
			else
			{
				speed = skeleton->deprec_GetWalkSpeedAbs();
			}
			break;
		}
	case MT_Run:
		{
			if ( m_speedConfig )
			{
				speed = m_speedConfig->GetSlowRunSpeedAbs();
			}
			else
			{
				speed = skeleton->deprec_GetSlowRunSpeedAbs();
			}
			break;
		}
	case MT_FastRun:
		{
			if ( m_speedConfig )
			{
				speed = m_speedConfig->GetFastRunSpeedAbs();
			}
			else
			{
				speed = skeleton->deprec_GetFastRunSpeedAbs();
			}
			break;
		}
	case MT_Sprint:
		{
			if ( m_speedConfig )
			{
				speed = m_speedConfig->GetSprintSpeedAbs();
			}
			else
			{
				speed = skeleton->deprec_GetSprintSpeedAbs();
			}
			break;
		}
	case MT_AbsSpeed:
		{
			speed = absSpeed;
			break;
		}
	}

	return speed;
}

void CMovingAgentComponent::SetMoveType( EMoveType type, Float absSpeed ) 
{ 
	m_moveType = type; 
	m_maxSpeed = GetSpeedForMoveType( type, absSpeed );
}

void CMovingAgentComponent::EnableCombatMode( Bool combat )
{
	if ( combat )
    {
		m_movementFlags |= MM_CombatMode;
    }
	else
    {
		m_movementFlags &= ~MM_CombatMode;
    }
}

//! enable/disable virtual controller
void CMovingAgentComponent::EnableVirtualController( const CName& virtualControllerName, const Bool enabled )
{
	RED_UNUSED( enabled );
	RED_UNUSED( virtualControllerName );
}

//! set virtual radius
void CMovingAgentComponent::SetVirtualRadius( const CName& radiusName, Bool immediately, const CName& virtualControllerName )
{
	RED_UNUSED( radiusName );
	RED_UNUSED( immediately );
	RED_UNUSED( virtualControllerName );
}

//! reset virtual radius
void CMovingAgentComponent::ResetVirtualRadius( const CName& virtualControllerName )
{
	RED_UNUSED( virtualControllerName );
}

//! set height
void CMovingAgentComponent::SetHeight( const Float height )
{
	RED_UNUSED( height );
}

//! reset height
void CMovingAgentComponent::ResetHeight()
{
}

void CMovingAgentComponent::CreateMovingAgent()
{
	SetMoveType( m_moveType );
	SetMaxRotation( 180.0f );

	if ( m_locomotion )
	{
		delete m_locomotion;
		m_locomotion = nullptr;
	}

	if ( m_steeringBehavior )
	{
		m_locomotion = new CMoveLocomotion( *this );

		// memory leaks safeguard
		if ( m_steeringRuntimeData )
		{
			m_steeringBehavior->ReleaseRealtimeDataInstance( this, m_steeringRuntimeData );
			m_steeringRuntimeData = nullptr;
		}

		m_steeringRuntimeData = m_steeringBehavior->CreateRealtimeDataInstance( this );
	}
	
	ResetMaxAcceleration();
	m_currentSteeringBehavior = m_steeringBehavior.Get();
	m_currentSteeringRuntimeData = m_steeringRuntimeData;
}

void CMovingAgentComponent::OnAttached( CWorld* world )
{
	{
		PC_SCOPE_PIX( CMovingAgentComponent_OnAttached1 );
		CEntity* ent = GetEntity();
		if ( ent )
		{
			CActor* act = Cast< CActor > ( ent );
			if ( act )
			{
				act->OnMovingAgentComponentChanged();
				if ( act->IsA< CPlayer >() )
				{
					m_triggerChannels |= TC_Player;
				}
			}
		}

		const CCharacterControllerParam* params = ent->GetEntityTemplate() ? ent->GetEntityTemplate()->FindGameplayParamT<CCharacterControllerParam>( true ) : nullptr;
		if( !params )
		{
			params = CCharacterControllerParam::GetStaticClass()->GetDefaultObject< CCharacterControllerParam > ();
		}

		//compute the furthest boundary and use that as our radius
		Float maxSum = Max( params->m_physicalRadius, params->m_baseVirtualCharacterRadius );

		for ( const SControllerRadiusParams& vrParam : params->m_radiuses )
		{
			if ( vrParam.m_radius > maxSum )
			{
				maxSum = vrParam.m_radius;
			}
		}

		for ( const SVirtualControllerParams& vcParam : params->m_virtualControllers )
		{
			const Float sum = vcParam.m_localOffset.Mag2() + vcParam.m_radius;
			if ( sum > maxSum )
			{
				maxSum = sum;
			}
		}

		m_avoidanceRadius = Max( maxSum, params->m_customAvoidanceRadius );
		m_baseRadius = params->m_physicalRadius;
	}

	// Pass to base class
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CMovingAgentComponent_OnAttached2 );

	Bool isRunningGame = GGame->IsActive() && !world->GetPreviewWorldFlag();

	// Create automatic trigger activator
	if ( m_triggerAutoActivator && isRunningGame )
	{
		CTriggerActivatorInfo initInfo;
		initInfo.m_channels = m_triggerChannels;
#ifndef RED_TRIGGER_SYSTEM_NO_DEBUG_NAMES
		initInfo.m_debugName = GetFriendlyName();
#endif
		initInfo.m_component = this;
		initInfo.m_extents = Vector( m_triggerActivatorRadius, m_triggerActivatorRadius, m_triggerActivatorHeight/2.0f );
		initInfo.m_maxContinousDistance = m_triggerMaxSingleFrameDistance;
		initInfo.m_enableCCD = m_triggerEnableCCD;

		// offset the initial position by half of the height
		initInfo.m_localToWorld = GetLocalToWorld();
		initInfo.m_localToWorld.SetTranslation( initInfo.m_localToWorld.GetTranslation() + Vector( 0.0f, 0.0f, m_triggerActivatorHeight/2.0f ) );

		// release previous activator: this should not happen
		ASSERT( nullptr == m_triggerActivator );
		if ( nullptr != m_triggerActivator )
		{
			m_triggerActivator->Remove();
			m_triggerActivator->Release();
			m_triggerActivator = nullptr;
		}

		// create new activator
		m_triggerActivator = world->GetTriggerManager()->CreateActivator( initInfo );
	}

	AddRepresentation( m_meshRepresentation, true );
	IMovableRepresentation* mainRepresentation = m_meshRepresentation;

	// Create agent
	Bool createdMoveAgent = false;
	if ( isRunningGame )
	{
		CPathLibWorld* pathlib = world->GetPathLibWorld();
		CPhysicsWorld* physics = nullptr;
		if( pathlib && world->GetPhysicsWorld( physics ) )
		{
			m_pathAgent = new CPathAgent( pathlib, physics, *this, m_baseRadius );
			m_pathAgent->AddRef();
			AddRepresentation( m_pathAgent, false );
		}
 
		CreateMovingAgent();
		createdMoveAgent = true;
 
		m_prevPosition			= Vector( FLT_MIN, FLT_MIN, FLT_MIN );

		m_lastNavigableAreaPosition = Vector( FLT_MIN, FLT_MIN, FLT_MIN );
 
		// Attach to editor fragment list
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Steering );
		world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Wind );
	}
	else
	{
		m_isEntityRepresentationForced |= LS_Force;
	}

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Locomotion );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_MovableRep );

	// setup the representations stack
	m_teleported = true;
	m_moved = true;
	m_teleportCorrectZ = true;
	m_enableAfterTeleport = createdMoveAgent;
	m_representationStack->OnAttached( mainRepresentation, createdMoveAgent, m_teleportToPos, m_teleportToRot );

	// create custom move representations
	CreateCustomMoveRepresentations( world );

	m_ragdollObstacle = false;

	if ( m_physicalRepresentation )
	{
		SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_On, CMovingAgentComponent::LS_Initial );
	}

	m_representationResetRequest = true;
}

void CMovingAgentComponent::OnDetached( CWorld* world )
{
	// first we deactivate locomotion - which uses all the data below ( the representations stack,
	// and the representations themselves ) - let it clean up gracefully
	delete m_locomotion;
	m_locomotion = nullptr;

	// now we deactivate the representations stack...
	m_representationStack->Reset();

	// ... and remove the representations themselves
	delete m_ragdollRepresentation;
	m_ragdollRepresentation = nullptr;

	// Destroy 
	if ( m_pathAgent )
	{
		m_pathAgent->Release();
		m_pathAgent = nullptr;
	}

	if ( m_steeringRuntimeData )
	{
		m_steeringBehavior->ReleaseRealtimeDataInstance( this, m_steeringRuntimeData );
		m_steeringRuntimeData = nullptr;
	}

	// remove trigger activator
	if ( nullptr != m_triggerActivator )
	{
		m_triggerActivator->Remove();
		m_triggerActivator->Release();
		m_triggerActivator = nullptr;
	}

	// Detach from editor fragment list
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Locomotion );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_MovableRep );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Steering );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Wind );

	CEntity* ent = GetEntity();
	if ( ent )
	{
		CActor* act = Cast< CActor > ( ent );
		if ( act )
		{
			act->OnMovingAgentComponentChanged();
		}
	}

	// Pass to base class
	TBaseClass::OnDetached( world );
}

void CMovingAgentComponent::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( m_locomotion )
	{
		m_locomotion->OnSerialize( file );
	}
}

void CMovingAgentComponent::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT( "ragdollRadius" ) )
	{
		m_ragdollRadius = Clamp< Float >( m_ragdollRadius, 0, MAX_AGENT_RADIUS );
	}
	else if ( property->GetName() == TXT( "triggerChannels" ) )
	{
		////////////////////////////////////////////////
		// TC_Player collision flag support
		CEntity* entity = GetEntity();
		if ( entity && entity->IsA< CPlayer >() )
		{
			m_triggerChannels |= TC_Player;
		}
		////////////////////////////////////////////////
	}
	else
	{
		TBaseClass::OnPropertyPostChange( property );
	}
}

// ----------------------------------------------------------------------------
// Representations management
// ----------------------------------------------------------------------------

void CMovingAgentComponent::AddRepresentation( IMovableRepresentation* representation, Bool activeInBackground )
{
	m_representationStack->Add( representation, activeInBackground );
}

void CMovingAgentComponent::RemoveRepresentation( IMovableRepresentation* representation )
{
	m_representationStack->Remove( representation );
}

// ----------------------------------------------------------------------------
// Queries
// ----------------------------------------------------------------------------

void CMovingAgentComponent::OnDynamicCollision( CMovingAgentComponent* pushedAgent )
{
	if ( !pushedAgent )
	{
		return;
	}

	CMovingAgentComponent* pushingAgent = this;
	THandle< CMovingAgentComponent > hPushingAgent( pushingAgent );

	// determine the pushing order for this pair:
	// - the player should always be able to push other actors before the get a chance to push him
	// - if two NPCs collide with each other, determine who's pushing who by comparing their pointers 
	//   ( the simplest fixed order determination there can be )
	// - it's always the standing entity that gets pushed
	// their pointers values - as simple as that, it gives us a constant collision description - pointers
	// are guaranteed not to change their value during pushing
	Bool pushingOrderValid = pushingAgent->GetEntity() == GCommonGame->GetPlayer() || 
		( pushedAgent->GetEntity() != GCommonGame->GetPlayer() && 
		 ( pushedAgent->GetLastMovementSpeed() < 1e-1 || reinterpret_cast< uintptr_t >( pushingAgent ) < reinterpret_cast< uintptr_t >( pushedAgent ) ) );
	if ( pushingOrderValid )
	{
		pushedAgent->GetEntity()->CallEvent( CNAME( OnPushed ), hPushingAgent );
	}
}

void CMovingAgentComponent::ToggleMovementFlag( Uint8 flag, Bool enable )
{
	if ( enable )
	{
		m_movementFlags |= flag;
	}
	else
	{
		m_movementFlags &= ~flag;
	}
}

Bool CMovingAgentComponent::IsMovementFlagSet( Uint8 flag ) const
{
	return flag == ( m_movementFlags & flag );
}

Bool CMovingAgentComponent::CanUseAsyncUpdateMode() const
{
	return GGame->GetGameplayConfig().m_animationAsyncUpdate && ( IsInCinematic() || GetEntity()->IsA< CNewNPC >() );
}

void CMovingAgentComponent::InternalUpdateAndSampleMultiAsyncPart( Float timeDelta )
{
	m_animationProxyData.Update( timeDelta );

	TBaseClass::InternalUpdateAndSampleMultiAsyncPart( timeDelta );
}

void CMovingAgentComponent::InternalUpdateAndSampleMultiSyncPart( Float timeDelta )
{
	UpdateSynchronizedMovementBehavior( timeDelta );

	TBaseClass::InternalUpdateAndSampleMultiSyncPart( timeDelta );
}

void CMovingAgentComponent::OnTickPrePhysics( Float timeDelta )
{
	UpdateSynchronizedMovementBehavior( timeDelta );
	
	TBaseClass::OnTickPrePhysics( timeDelta );
}

void CMovingAgentComponent::OnTickPostUpdateTransform( Float timeDelta )
{
	PC_SCOPE_PIX( CMovingAgentComponent_OnTickPostUpdateTransform );

	TBaseClass::OnTickPostUpdateTransform( timeDelta );

	// This has to be here ATM, as during prePhysics tick update and sample of behavior graph takes place and this method may modify behavior variables
	UpdateParallelMovementBehavior( timeDelta );
}

void CMovingAgentComponent::UpdateSynchronizedMovementBehavior( Float timeDelta )
{
	PC_SCOPE( MAC_UpdateSyncMovement );

	// TODO: support m_pathAgent
	// Empty space test
	
}

void CMovingAgentComponent::UpdateParallelMovementBehavior( Float timeDelta )
{
	PC_SCOPE( MAC_UpdateParallelMovement );

	// reset movement sinks
	const Vector& currentPosition = GetWorldPosition();
	m_deltaPosition.SetZeros();
	m_deltaRotation = EulerAngles( 0, 0, 0 );

	if ( IsAttached() && GGame->IsActive() ) 
	{
		// update the static targeter
		if ( m_staticTarget )
		{
			if ( m_rotationTargetCooldown > 0.0f )
			{
				m_rotationTargetCooldown -= timeDelta;
			}
			else
			{
				m_rotationTargetCooldown = 0.0f;
				m_staticTarget->Update();
			}
		}

		// we need to call steering commands before the animated component gets
		// ticked, because it's in its tick is where the commands will be translated into
		// motion - that's the reason this call needs to be placed before 
		// the call to TBaseClass::OnTickPrePhysics
		if ( m_locomotion )
		{
			PC_SCOPE( MAC_LocomotionTick );
			m_locomotion->Tick( timeDelta );
		}


		// actuate the movement requests:
		{
			// update movement speed
			UpdateMoveSpeed( timeDelta );

			// update movement direction
			UpdateMoveDirection( timeDelta );

			// update movement rotation
			UpdateMoveRotation( timeDelta );
		}
	}

	// calculate 
	m_isColliding = false;
	m_collisionLine = Vector( 0, 0, 0 );
}

//////////////////////////////////////////////////////////////////////////

void CMovingAgentComponent::ProcessPoseConstraints( Float timeDelta, SBehaviorGraphOutput& output )
{
	m_animationProxyData.OnPreProcessPoseConstraints( this, output );
	TBaseClass::ProcessPoseConstraints( timeDelta, output );
}

void CMovingAgentComponent::PostProcessPoseConstraints( SBehaviorGraphOutput& output )
{
	m_animationProxyData.OnPostProcessPoseConstraints( this, output );
	TBaseClass::PostProcessPoseConstraints( output );
}

//////////////////////////////////////////////////////////////////////////

void CMovingAgentComponent::UpdateMovement1_PreSeperation( Float timeDelta )
{
    if ( !IsAttached() )
	{
        return;
	}

	PC_SCOPE( MAC_UpdateMovement1_PreSeperation );

    // Update triggers
    if ( nullptr != m_triggerActivator )
    {
		PC_SCOPE( MAC_UpdateMovement1_PreSeperation_UpdateTriggers );

		// use the position offset of half activator height
        Vector newPosition = GetWorldPositionRef();
        newPosition.Z += (m_triggerActivatorHeight / 2.0f);

        // convert the world space position to the trigger system coordinates
        m_triggerActivator->Move( IntegerVector4( newPosition ), m_teleported );
    }

    // probe the current height
    //PerformHeightTrace();

	ResetMoveRepresentation();

    // apply the movement
    if ( m_teleported )
    {
		PC_SCOPE( MAC_UpdateMovement1_PreSeperation_Teleported );
		if ( IsMotionEnabled() )
			m_representationStack->OnSetPlacement( timeDelta, m_teleportToPos, m_teleportToRot, m_teleportCorrectZ );
		else
			m_representationStack->OnFollowPlacement( timeDelta, m_teleportToPos, m_teleportToRot );
    }
    else
    {
		CalculateMovement( timeDelta );
		if ( IsMotionEnabled() )
		{			
			m_representationStack->OnMove( m_deltaPosition, m_deltaRotation );
		}		
    }

	{
		PC_SCOPE( MAC_UpdateMovement1_PreSeperation_UpdateRepresentationStack );
		m_representationStack->Update1_PreSeparation( timeDelta );
	}
}

//////////////////////////////////////////////////////////////////////////

void CMovingAgentComponent::FinalizeMovement2_PostSeparation( Float timeDelta )
{
	TBaseClass::FinalizeMovement2_PostSeparation( timeDelta );

	m_deltaPosition.SetZeros();
	m_deltaRotation = EulerAngles( 0, 0, 0 );
}

Bool CMovingAgentComponent::UpdateMovement2_PostSeperation( Float timeDelta )
{
    if ( !IsAttached() )
        return false;
    
	PC_SCOPE( MAC_UpdateMovement2_PostSeperation );
	m_representationStack->Update2_PostSeparation( timeDelta, !m_teleported );

	// get the actual delta position
	const Vector currentPosition = GetAgentPosition();
	const Vector actualDeltaPosition = (!m_teleported && m_prevPosition != Vector( FLT_MIN, FLT_MIN, FLT_MIN )) ? currentPosition - m_prevPosition : Vector::ZEROS;
	RED_WARNING( actualDeltaPosition.SquareMag3() < 100.0f * 100.0f, "Delta position is really big, there might be something very bad happening." );
	CalculateMovementStats( timeDelta, actualDeltaPosition, m_deltaPosition );

	if ( m_pathAgent )
	{
		if ( m_pathAgent->IsOnNavdata() )
		{
			m_lastNavigableAreaPosition = currentPosition;
		}
	}

	// smooth out Z correction mechanism
	//if ( m_smoothOutZCorrection && m_moved && !m_teleported )
	//{
	//	const Float MIN_Z_SMOOTH_DIST = 0.025f;
	//	const Float MAX_Z_SMOOTH_DIST = 1.0f;

	//	Float zDiff = m_prevPosition.Z - currentPosition.Z;
	//	Float zAbsDiff = Abs( zDiff );
	//	// kick off with Z correction smooth out
	//	if ( zAbsDiff > MIN_Z_SMOOTH_DIST && zAbsDiff < MAX_Z_SMOOTH_DIST )
	//	{
	//		Vector translation( 0.f, 0.f, zDiff );
	//		EulerAngles angle( 0.f, 0.f, 0.f );

	//		m_animationProxyData.AddAdditionalOffsetMS( translation, angle, 0.5f );
	//	}
	//}

	// cleanup
	m_teleported = false;
	m_teleportCorrectZ = false;
	
	// store the current position as the previous position
	m_prevPosition = currentPosition;

	return m_moved;
}

//////////////////////////////////////////////////////////////////////////

void CMovingAgentComponent::UpdateMovedFlag()
{
	m_moved = !Vector::Near3( m_deltaPosition, Vector::ZEROS )
		   || !m_deltaRotation.AlmostEquals( EulerAngles::ZEROS )
		   || m_teleported;
}

void CMovingAgentComponent::CalculateMovement( Float timeDelta )
{
	PC_SCOPE( MAC_CalculateMovement );

	// recalculate position
	if ( IsMovementFlagSet( MM_MoveSliding ) )
	{
		// we want to slide the agent in the specified direction
		const Float val = m_deltaPosition.Mag3();

		const Vector plannedHeading = EulerAngles::YawToVector( GetMoveDirectionWorldSpace() );
		m_deltaPosition = plannedHeading * val;
	}

	UpdateSlide( timeDelta );

	// it is called as last thing as it may replace deltas
	UpdateAdjustor( timeDelta );

	UpdateMovedFlag();
}

RED_INLINE static Vector CalcualteVelocityFromDeltaPosition( Float timeDelta, const Vector& deltaPosition )
{
	Vector velocity( 0, 0, 0 );
	if ( timeDelta > 0.0f )
	{
		// calculate the speed value
		velocity.X = Float( Double( deltaPosition.X ) / Double( timeDelta ) );		
		velocity.Y = Float( Double( deltaPosition.Y ) / Double( timeDelta ) );
		velocity.Z = Float( Double( deltaPosition.Z ) / Double( timeDelta ) );
		velocity.W = 1.0f;
	}

	ASSERT( !Red::Math::NumericalUtils::IsNan( velocity.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( velocity.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( velocity.Z ) );

	return velocity;
}

void CMovingAgentComponent::CalculateMovementStats( Float timeDelta, const Vector& actualDeltaPosition, const Vector& deltaPositionFromBehaviorGraph )
{
	PC_SCOPE( MAC_CalculateMovementStats );
	m_velocity = CalcualteVelocityFromDeltaPosition( timeDelta, actualDeltaPosition );
	m_deltaPositionFromBehaviorGraph = deltaPositionFromBehaviorGraph;
	m_velocityBasedOnRequestedMovement = CalcualteVelocityFromDeltaPosition( timeDelta, deltaPositionFromBehaviorGraph );
}

void CMovingAgentComponent::ResetMoveRepresentation()
{
	if ( m_representationResetRequest )
	{
		m_representationResetRequest = false;

		if ( IMovableRepresentation* rep = DetermineMoveRepresentation() )
		{
			m_representationStack->Activate( rep );
		}
	}
}

IMovableRepresentation* CMovingAgentComponent::DetermineMoveRepresentation() const
{
	if ( !m_isDisabled )
	{
		if ( !m_isEntityRepresentationForced )
		{
			return m_pathAgent;
		}
		else
		{
			return m_meshRepresentation;
		}
	}

	return nullptr;
}

void CMovingAgentComponent::SetTriggerActivatorRadius( const Float radius )
{
	if ( nullptr != m_triggerActivator )
	{
		if ( radius != m_triggerActivatorRadius && radius > 0.0f )
		{
			m_triggerActivatorRadius = radius;

			const Vector extents( m_triggerActivatorRadius, m_triggerActivatorRadius, m_triggerActivatorHeight / 2.0f );
			m_triggerActivator->SetExtents( extents );
		}
	}
}

void CMovingAgentComponent::SetTriggerActivatorHeight( const Float height )
{
	if ( nullptr != m_triggerActivator )
	{
		if ( height != m_triggerActivatorHeight && height > 0.0f )
		{
			m_triggerActivatorHeight = height;

			const Vector extents( m_triggerActivatorRadius, m_triggerActivatorRadius, m_triggerActivatorHeight / 2.0f );
			m_triggerActivator->SetExtents( extents );
		}
	}
}

void CMovingAgentComponent::AddTriggerActivatorChannel( const ETriggerChannel channel )
{
	if ( 0 == ( m_triggerChannels & channel ) )
	{
		m_triggerChannels |= (Uint32) channel;
		if ( m_triggerActivator != nullptr )
		{
			m_triggerActivator->SetMask( m_triggerChannels );
		}
	}
}

void CMovingAgentComponent::RemoveTriggerActivatorChannel( const ETriggerChannel channel )
{
	if ( 0 != ( m_triggerChannels & channel ) )
	{
		m_triggerChannels &= ~(Uint32) channel;
		if ( m_triggerActivator != nullptr )
		{
			m_triggerActivator->SetMask( m_triggerChannels );
		}
	}
}

Bool CMovingAgentComponent::IsInsideTrigger( const class ITriggerObject* trigger ) const
{
	if ( m_triggerActivator != nullptr )
	{
		return m_triggerActivator->IsInsideTrigger( trigger );
	}
	return false;
}

Bool CMovingAgentComponent::ProcessMovement( Float timeDelta )
{
	Vector deltaPos;
	EulerAngles deltaRot;
	Bool computedMovement = false;
	m_wasSeparatedBy = nullptr;

	if ( m_steeringControlledMovement || 
		 ( m_useExtractedMotion && m_animationSuppressionMask ) ) // if animation is suppressed, use steering movement instead of extracted motion
	{
		Float absSpeed = GetAbsoluteMoveSpeed();
		Float moveDir = GetMoveDirectionWorldSpace();
		deltaPos = EulerAngles::YawToVector( moveDir ) * (absSpeed*timeDelta);
		deltaRot = EulerAngles( 0.f, 0.f, m_moveRotation * timeDelta * 2.f );

		computedMovement = true;
	}
	else if ( m_useExtractedMotion )
	{
		if ( ExtractMotion( timeDelta, deltaPos, deltaRot ) )
		{
			computedMovement = true;
		}
	}
	if ( computedMovement )
	{
		AddDeltaMovement( deltaPos, deltaRot );
		return true;
	}

	m_deltaPosition.SetZeros();
	m_deltaRotation = EulerAngles::ZEROS;

	return true;
}

void CMovingAgentComponent::AddDeltaMovement( const Vector& deltaPosition, const EulerAngles& deltaRotation )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( deltaRotation.Yaw) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( deltaRotation.Pitch ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( deltaRotation.Roll ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( deltaPosition.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( deltaPosition.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( deltaPosition.Z ) );

	// if this was a slide, override the deltas
	m_deltaPosition += deltaPosition;
	m_deltaRotation += deltaRotation;

	UpdateMovedFlag();
}

void CMovingAgentComponent::CreateCustomMoveRepresentations( CWorld* world )
{

}
void CMovingAgentComponent::SetACMotionEnabled( Bool enabled )
{
	SetMotionEnabled( enabled );
}
Bool CMovingAgentComponent::IsACMotionEnabled() const
{
	return IsMotionEnabled();
}

void CMovingAgentComponent::SetMotionEnabled( Bool enabled, ELockSource lockSource)
{
	if ( IsMotionEnabled( lockSource ) == enabled )
	{
		return; // nothing to do
	}

	SetShouldSave( !enabled );

	if ( enabled )
	{
		auto lastVal = m_isDisabled;
		m_isDisabled &= ~lockSource;
		if ( m_isDisabled == 0 && lastVal != 0 )
		{
			if ( IsAttached() )
			{
				m_representationStack->OnActivate( GetWorldPositionRef(), GetWorldRotation() );
				m_representationResetRequest = true;
			}
		}
	}
	else
	{
		if ( m_isDisabled == 0 )
		{
			if ( IsAttached() )
			{
				m_representationStack->OnDeactivate();
			}
		}
		m_isDisabled |= lockSource;
	}
}

void CMovingAgentComponent::ForceEntityRepresentation( Bool force, ELockSource lockSource )
{
	Bool prevState = IsEntityRepresentationForced();

	if ( force )
	{
		m_isEntityRepresentationForced |= lockSource;
	}
	else
	{
		m_isEntityRepresentationForced &= ~lockSource;
	}

	if ( prevState != IsEntityRepresentationForced() )
	{
		m_representationResetRequest = true;
	}
}

void CMovingAgentComponent::SetPhysicalRepresentationRequest( ERequest enableRequest, ELockSource enableSource, ERequest disableRequest, ELockSource disableSource )
{
	Bool prevEnableState = ( m_physicalRepresentationEnableRequest & enableSource ) != 0;
	Bool enableState;
	switch ( enableRequest )
	{
	case Req_On: enableState = true; break;
	case Req_Off: enableState = false; break;
	case Req_Toggle: enableState = !prevEnableState; break;
	case Req_Ignore: enableState = prevEnableState; break;
	}

	Bool prevDisableState = ( m_physicalRepresentationDisableRequest & disableSource ) != 0;
	Bool disableState;
	switch ( disableRequest )
	{
	case Req_On: disableState = true; break;
	case Req_Off: disableState = false; break;
	case Req_Toggle: disableState = !prevDisableState; break;
	case Req_Ignore: disableState = prevDisableState; break;
	}

	if ( enableState != prevEnableState || disableState != prevDisableState )
	{
		Bool prevState = IsPhysicalRepresentationRequested();

		if ( enableState )
		{
			m_physicalRepresentationEnableRequest |= enableSource;
		}
		else
		{
			m_physicalRepresentationEnableRequest &= ~enableSource;
		}

		if ( disableState )
		{
			m_physicalRepresentationDisableRequest |= disableSource;
		}
		else
		{
			m_physicalRepresentationDisableRequest &= ~disableSource;
		}

		if ( prevState != IsPhysicalRepresentationRequested() )
		{
			m_representationResetRequest = true;
		}
	}
}

Bool CMovingAgentComponent::IsPhysicalRepresentationRequested() const 
{
	return m_physicalRepresentationEnableRequest > m_physicalRepresentationDisableRequest;
}

Bool CMovingAgentComponent::IsPhysicalRepresentationEnabled() const 
{
	ASSERT(m_representationStack && m_representationStack->GetActiveRepresentation());
	return m_representationStack->GetActiveRepresentation()->GetName() == CNAME( CMRPhysicalCharacter );
}

Bool CMovingAgentComponent::CanSlideAlong( CMovingAgentComponent& agent ) const
{
	THandle< CMovingAgentComponent > hPushingAgent( const_cast< CMovingAgentComponent* >( this ) );
	return ( CR_EventSucceeded == agent.GetEntity()->CallEvent( CNAME( OnMovementCollision ), hPushingAgent ) );
}

void CMovingAgentComponent::SetCustomSteeringBehavior( CMoveSteeringBehavior* steeringGraph, InstanceBuffer* runtimeData )
{
	if ( m_locomotion )
	{
		m_locomotion->OnSteeringBehaviorChanged( this, m_currentSteeringBehavior, m_currentSteeringRuntimeData, steeringGraph, runtimeData );
	}

	m_currentSteeringBehavior = steeringGraph;
	m_currentSteeringRuntimeData = runtimeData;
	ResetMaxAcceleration();
}

void CMovingAgentComponent::ClearCustomSteeringBehavior()
{
	if( m_locomotion )
	{
		m_locomotion->OnSteeringBehaviorChanged( this, m_currentSteeringBehavior, m_currentSteeringRuntimeData, m_steeringBehavior.Get(), m_steeringRuntimeData );
	}	
	m_currentSteeringBehavior = m_steeringBehavior.Get();
	m_currentSteeringRuntimeData = m_steeringRuntimeData;
	ResetMaxAcceleration();
}

Bool CMovingAgentComponent::GetRagdollPosition(Vector& position) const
{
#ifdef USE_HAVOK
	if (!m_ragdollInstance || m_ragdollInstance->GetNumRigidBodies()<=0)
	{
		return false;
	}

	MARK_FOR_READ( m_ragdollInstance->GetRigidBody( 0 ) );
	hkVector4 hkPosition = m_ragdollInstance->GetRigidBody( 0 )->getTransform().getTranslation();
	UNMARK_FOR_READ( m_ragdollInstance->GetRigidBody( 0 ) );
	position.Set4( hkPosition(0), hkPosition(1), hkPosition(2), 0.0f );
#endif

	return true;
}

Bool CMovingAgentComponent::IsRotationTargetEnabled() const 
{ 
	return m_staticTarget->IsRotationTargetSet(); 
}

void CMovingAgentComponent::UpdateGoal( SMoveLocomotionGoal& goal, Float timeDelta ) const
{
	if ( m_staticTarget )
	{
		m_staticTarget->UpdateChannels( *this, goal, timeDelta );
	}
}

void CMovingAgentComponent::CancelMove()
{
	if ( m_locomotion )
	{
		m_locomotion->CancelMove();
	}

	if ( m_movementAdjustor )
	{
		m_movementAdjustor->CancelAll();
	}

	// reset the movement sink
	Slide( Vector( 0.0f, 0.0f, 0.0f ), EulerAngles( 0.0f, 0.0f, 0.0f ) );
	ResetMoveRequests();
}

void CMovingAgentComponent::Teleport( const Vector& position, const EulerAngles* rotation )
{
	// Cancel rotation
	CancelMove();

	EulerAngles rot;
	if ( rotation )
	{
		rot = *rotation;
	}
	else
	{
		rot = GetEntity()->GetWorldRotation();
	}
	TeleportTo( position, rot );
}

Bool CMovingAgentComponent::CanCancelMovement() const
{
	if ( m_locomotion )
	{
		return m_locomotion->CanCancelMovement();
	}
	else
	{
		return true;
	}
}

void CMovingAgentComponent::ClearRotationTarget()
{
	if ( !m_locomotion )
	{
		return;
	}

	m_staticTarget->ClearRotation();
}

void CMovingAgentComponent::SetRotationTarget( const Vector& position, Bool clamping )
{
	if ( !m_locomotion )
	{
		return;
	}

	m_staticTarget->SetRotation( position, clamping );
}

void CMovingAgentComponent::SetRotationTarget( const THandle< CNode >& node, Bool clamping )
{
	if ( !m_locomotion )
	{
		return;
	}

	m_staticTarget->SetRotation( node, clamping );
}

void CMovingAgentComponent::RelaxRotationTarget( Float cooldown )
{
	m_rotationTargetCooldown = cooldown;
}

void CMovingAgentComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( flags == SHOW_Locomotion )
	{
		const Vector& currPos = GetEntity()->GetWorldPositionRef();
	
		if ( m_movementAdjustor )
		{
			SMovementAdjustorContext context( this, m_deltaPosition, m_deltaRotation );
			m_movementAdjustor->GenerateDebugFragments( frame, context );
		}

		if ( m_locomotion )
		{
			m_locomotion->GenerateDebugFragments( frame );
		}

		if ( m_staticTarget )
		{
			m_staticTarget->GenerateDebugFragments( *this, frame );
		}

		Vector velocity = GetVelocity();
		Float speedRatio = (m_maxSpeed > 0.f) ? m_desiredAbsoluteSpeed / m_maxSpeed : 10.f;
		Vector desMoveDir = EulerAngles::YawToVector( m_desiredDirection ) * speedRatio;
		frame->AddDebugLineWithArrow( currPos, currPos + velocity * 4.f, 1.0f, 0.2f, 0.2f, Color::GREEN, true );
		frame->AddDebugLineWithArrow( currPos, currPos + desMoveDir * 4.f, 1.0f, 0.2f, 0.2f, Color::BLUE, true );
		
		Float speed = GetRelativeMoveSpeed();
		if ( speed > 0.f )
		{
			String speedTxt = String::Printf( TXT("Speed: %0.2f"), speed );
			frame->AddDebugText( currPos, speedTxt, 0, -4, false, Color::LIGHT_BLUE );
		}

		if ( m_isColliding )
		{
			frame->AddDebugFatLine( currPos, currPos + m_collisionLine, Color::LIGHT_BLUE, 0.1f, true );
		}
	}

	if ( flags == SHOW_MovableRep )
	{
		if ( m_pathAgent )
		{
			m_pathAgent->GenerateDebugFragments( frame );
		}
	}

	if ( flags == SHOW_Steering )
	{
		if ( m_currentSteeringBehavior )
		{
			m_currentSteeringBehavior->GenerateDebugFragments( *this, frame );
		}
	}

	if ( flags == SHOW_Wind )
	{
		if ( GGame->GetActiveWorld() && frame->GetFrameInfo().m_renderingMode != RM_HitProxies )
		{
			Matrix m = Matrix::IDENTITY;
			m.SetTranslation( GetLocalToWorld().GetTranslationRef() ); 
			frame->AddDebugArrow( m, GGame->GetActiveWorld()->GetWindAtPoint( GetWorldPositionRef() ), 1.f, Color::RED, true );
		}
	}
}

void CMovingAgentComponent::AttachLocomotionController( CMoveLocomotion::IController& controller )
{
	if ( m_locomotion )
	{
		m_locomotion->AttachController( controller );
	}
}

void CMovingAgentComponent::DetachLocomotionController( CMoveLocomotion::IController& controller )
{
	if ( m_locomotion )
	{
		m_locomotion->DetachController( controller );
	}
}

/************************************************************************/
/* Controlled animated motion                                           */
/************************************************************************/

Bool CMovingAgentComponent::CanGoStraightToDestination( const Vector& pos ) const
{
	if ( IsMotionEnabled() )
	{
		if ( m_pathAgent )
		{
			return m_pathAgent->TestLine( pos.AsVector3() );
		}
	}
	return true;
}

Bool CMovingAgentComponent::IsEndOfLinePositionValid( const Vector& destination ) const
{
	PC_SCOPE( MAC_PathEngineIsEndOfLinePositionValid );

	if ( m_pathAgent )
	{
		return m_pathAgent->TestLocation( destination.AsVector3() );
	}
	return false;

	//if ( !m_pathEngineAgent )
	//	return false;

	//// No world, not valid
	//CPathEngineWorld* world = m_pathEngineAgent->GetWorld();
	//if ( !world )
	//	return false;	

	//cPosition pos = m_pathEngineAgent->GetPosition();
	//if( !pos.IsValid() )	
	//	return false;	

	//// Test
	//cPosition newPos = world->GetEndOfLinePosition( pos, CPathEngine::ToPEUnits( destination.X ), CPathEngine::ToPEUnits( destination.Y ) );
	//return newPos.IsValid();
}

Bool CMovingAgentComponent::CalculatePathLength( const Vector& startPos, const Vector& stopPos, Float maxDistance, Float& outLength ) const
{
	// TODO: Support m_pathAgent

	//if( m_pathEngineAgent )
	//{
	//	return m_pathEngineAgent->CalculatePathLength( startPos, stopPos, maxDistance, outLength );
	//}

	return false;
}

Bool CMovingAgentComponent::GetFirstCollision( const Vector& destination, Vector& lineDir ) const
{
	lineDir = m_collisionLine;
	return m_isColliding;
}

void CMovingAgentComponent::RequestAbsoluteMoveSpeed( Float val ) 
{
	m_desiredAbsoluteSpeed = Clamp< Float >( val, 0, m_maxSpeed * m_allowExceedingMaxSpeedCoef );
}

void CMovingAgentComponent::RequestRelativeMoveSpeed( Float val ) 
{
	RequestAbsoluteMoveSpeed( ConvertSpeedRelToAbs( val ) );
}

void CMovingAgentComponent::ForceSetRelativeMoveSpeed( Float relativeMoveSpeed )
{
	CAnimatedComponent::ForceSetRelativeMoveSpeed( relativeMoveSpeed );
	m_desiredAbsoluteSpeed = ConvertSpeedRelToAbs( relativeMoveSpeed );
	m_currentSpeedVal = m_desiredAbsoluteSpeed;
	m_lastRelMovementSpeed = relativeMoveSpeed;
}

void CMovingAgentComponent::UpdateMoveSpeed( Float timeDelta ) 
{
	Float speedDiff = m_desiredAbsoluteSpeed - m_currentSpeedVal;
	if ( speedDiff != 0.f )
	{
		Float acceleration = 0.0f;
		if ( speedDiff > 0.0f )
		{
			// acceleration
			acceleration = m_acceleration * timeDelta;	
			acceleration = ::Min( speedDiff, acceleration );
		}
		else
		{
			// deceleration
			acceleration = -m_deceleration * timeDelta;
			acceleration = ::Max( speedDiff, acceleration );
		}

		m_currentSpeedVal += acceleration;

		Float relLinearSpeed = ConvertSpeedAbsToRel( m_currentSpeedVal );

		m_lastRelMovementSpeed	= relLinearSpeed; 
	}
	
	// Adding speed fed from steering with speed fed from script ( most of the time input )
	m_relativeMoveSpeed		= m_lastRelMovementSpeed + m_gameplayRelativeMoveSpeed;
}

void CMovingAgentComponent::UpdateMoveDirection( Float timeDelta )
{
	if ( m_directionSet == false && m_gameplayDirectionSet == false )
	{
		// Do not set anything to preserve existing m_moveDirection 
		return;
	}

	Float currMoveDir		= TBaseClass::GetMoveDirectionWorldSpace();

	Float desiredDirection = 0.0f;
	Uint32 directionCount = 0;
	if ( m_gameplayDirectionSet )
	{
		desiredDirection += m_gameplayMoveDirection;
		directionCount++;
	}

	if ( m_directionSet )
	{
		desiredDirection += m_desiredDirection;
		directionCount++;
	}
	desiredDirection		/= directionCount;

	Float dirChange				= EulerAngles::AngleDistance( currMoveDir, desiredDirection );

	Float dirChangeSign			= dirChange >= 0 ? 1.0f : -1.0f;
	Float timeIndepDirChange	= ( dirChangeSign * m_directionChangeRate ) * timeDelta;

	Float newMoveDirection;
	if ( MAbs( dirChange ) < MAbs( timeIndepDirChange ) )
	{
		newMoveDirection		= desiredDirection;
		m_directionSet			= false;
		m_gameplayDirectionSet	= false;
	}
	else
	{
		newMoveDirection = EulerAngles::NormalizeAngle( currMoveDir + timeIndepDirChange );
	}

	// apply the direction change
	TBaseClass::SetMoveDirection( newMoveDirection );
}

Float CMovingAgentComponent::GetDesiredMoveDirectionWorldSpace() const
{
	return m_desiredDirection;
}

void CMovingAgentComponent::UpdateMoveRotation( Float timeDelta )
{
	Float rawRotation = m_desiredRotation;
	if ( m_rotationSet == MRT_None )
	{
		// reset
		m_desiredRotation = 0.0f;
		TBaseClass::SetMoveRotation( 0.0f );
		return;
	}

	Float newRotation = 0.0f;
	if ( m_rotationSet == MRT_Clamped )
	{
		newRotation = ClampRotation( timeDelta, m_desiredRotation );
		m_desiredRotation -= newRotation;
		if ( MAbs( m_desiredRotation ) < 1e-3 )
		{
			m_rotationSet = MRT_None;
		}
	}
	else
	{
		newRotation = m_desiredRotation;
		m_desiredRotation = 0.f;
		m_rotationSet = MRT_None;
	}

	// apply the rotation
	TBaseClass::SetMoveRotation( newRotation, rawRotation );
}


void CMovingAgentComponent::UpdateSlide( Float timeDelta )
{
	if ( !m_slide )
	{
		return;
	}

	PC_SCOPE( MAC_UpdateSlide );

	// calculate the speed of the slide
	Float maxDist = 0.0f;
	if ( m_slideSpeedDefined )
	{
		maxDist = m_slideSpeed * timeDelta;
	}
	else
	{
		maxDist = m_maxSpeed * timeDelta;
	}

	// apply the slide movement to the delta position and remove it from the remaining slide distance
	Float slideDist = m_slideDir.Mag3();
	Vector slideDir = m_slideDir.Normalized3();
	Float slideDistThisFrame = ::Min( slideDist, maxDist );
	m_deltaPosition += slideDir * slideDistThisFrame;

	Float remainingSlideDist = slideDist - slideDistThisFrame;
	if ( remainingSlideDist < 0.0f )
	{
		remainingSlideDist = 0.0f;
		m_slideDir = Vector( 0.0f, 0.0f, 0.0f );
	}
	else
	{
		m_slideDir = slideDir * remainingSlideDist;
	}

	// rotation
	Float rotationThisFrame = ClampRotation( timeDelta, m_slideRotation.Yaw );
	if ( MAbs( rotationThisFrame ) >= MAbs( m_slideRotation.Yaw ) )
	{
		m_slideRotation.Yaw = 0.0f;
	}
	else
	{
		m_slideRotation.Yaw -= rotationThisFrame;
	}
	m_deltaRotation.Yaw += rotationThisFrame;

	
	// cleanup
	if ( remainingSlideDist <= 0.0f && MAbs( m_slideRotation.Yaw ) <= 0.0f )
	{
		m_slide = false;
		m_slideDir = Vector( 0, 0, 0 );
		m_slideRotation = EulerAngles( 0, 0, 0 );
	}
}

void CMovingAgentComponent::UpdateAdjustor( float timeDelta )
{
	// add movement that is result from requests handled by adjustor - to this just before actual movement
	if ( m_movementAdjustor )
	{
		PC_SCOPE( MAC_UpdateMovementAdjustor );

		SMovementAdjustorContext context( this, m_deltaPosition, m_deltaRotation );
		m_movementAdjustor->Tick( context, timeDelta );
		m_deltaPosition = context.m_outputDeltaLocation;
		m_deltaRotation = context.m_outputDeltaRotation;
	}
}

void CMovingAgentComponent::Slide( const Vector& direction, const EulerAngles& rotation, Float* speed )
{
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Yaw) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Pitch ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( rotation.Roll ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( direction.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( direction.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( direction.Z ) );

	// a new slide requests needs to override the old one - we can't accumulate
	// the values due to the fact that the slide is executed in a latent way - it's 
	// spread across multiple frames, and a steering i.e. can and will 
	// set new slide values each frame (if a slide task is running there ), due to 
	// the steering nature ( the fact that IT IS EVALUATED EACH FRAME )
	m_slide = true;
	m_slideDir = direction;
	m_slideDir.W = 1.0f;
	m_slideRotation = rotation;

	m_slideSpeedDefined = speed != nullptr;
	m_slideSpeed = speed != nullptr ? *speed : 0.0f;

	// change movement direction as well
	Float desiredHeading = EulerAngles::NormalizeAngle( GetWorldYaw() + m_slideRotation.Yaw );
	RequestMoveDirection( desiredHeading );
}


Bool CMovingAgentComponent::TeleportTo( const Vector& newPosition, const EulerAngles& newRotation, const Bool correctZ )
{
	COMPONENT_EVENT( this, EAIE_Misc, EAIR_Success, TXT( "TeleportTo" ), String::Printf( TXT( "Teleporting to [%s] [%s] [%i]" ), ToString( newPosition ).AsChar(), ToString( newRotation ).AsChar(), !!correctZ ) );

	ASSERT( !Red::Math::NumericalUtils::IsNan( newRotation.Yaw) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( newRotation.Pitch ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( newRotation.Roll ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( newPosition.X ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( newPosition.Y ) );
	ASSERT( !Red::Math::NumericalUtils::IsNan( newPosition.Z ) );

	m_teleported = true;
	m_moved = true;
	m_enableAfterTeleport = false;
	m_teleportToPos = newPosition;
	m_teleportToRot = newRotation;
	m_teleportCorrectZ = correctZ;

	return true;
}

Float CMovingAgentComponent::ClampRotation( Float timeDelta, Float rotation ) const
{
	Float rotSpeed = timeDelta * m_maxRotation;
	Float valAbs = MAbs( rotation );
	if ( valAbs > rotSpeed )
	{
		rotation = ( rotation / valAbs ) * rotSpeed;
	}
	
	return rotation;
}

Vector2 CMovingAgentComponent::GetSteeringVelocity() const
{
	Vector2 v = ::EulerAngles::YawToVector2( m_moveDirection );
	v *= m_relativeMoveSpeed;
	return v;
}

Vector CMovingAgentComponent::GetAgentPosition() const
{
	return m_representationStack->GetStackPosition();
}

void CMovingAgentComponent::BlendPelvisWS( const Matrix& pointWS, Float blendTime )
{
	SAnimationProxyData& proxy = AccessAnimationProxy();
	proxy.SetRequestToApplyPelvisCorrectionWS( pointWS, blendTime );
}

void CMovingAgentComponent::SnapToMesh( Vector& position )
{
	// snap the height to the mesh we're walking on
	if ( m_placementTrace.DoTest( position ) && m_placementTrace.m_traceResult.m_isValid == true )
	{
		ASSERT( m_placementTrace.m_traceResult.m_height != INVALID_Z );
		position.Z = m_placementTrace.m_traceResult.m_height; 
	}
}

Float CMovingAgentComponent::CalcSlopeAngle() const
{
	const Vector& delta = GetWorldPositionRef() - m_prevPosition;
	const Float mag = delta.Mag3();

	if ( mag < 0.01f )
	{
		return 0.f;
	}
	
	const Float angle = RAD2DEG( MAsin( delta.Z / mag ) );
	return angle;
}

Float CMovingAgentComponent::GetRadius() const
{
	return m_baseRadius;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
const SPhysicalMaterial* CMovingAgentComponent::GetCurrentStandPhysicalMaterial() const
{
	if(m_forcedStandPhysicalMaterials.Size() > 0)
		return m_forcedStandPhysicalMaterials.Back();
	else
		return nullptr;
}

Vector CMovingAgentComponent::PredictWorldPosition( Float inTime ) const
{
	PC_SCOPE( MAC_PredictWorldPosition )

	if ( inTime <= 0.0f )
	{
		return GetWorldPositionRef();
	}

	const Vector& startingLoc = GetWorldPositionRef();
	const EulerAngles startingRot = GetWorldRotation();
	const Matrix startingRotMat = startingRot.ToMatrix();
	Vector endLoc = startingLoc;

	// predict only basing on normal anims
	const SBehaviorUsedAnimationData * usedAnim = m_recentlyUsedAnims.m_anims.GetUsedData();
	for ( Uint32 idx = 0; idx < m_recentlyUsedAnims.m_anims.GetNum(); ++ idx, ++ usedAnim )
	{
		if ( usedAnim->m_animation &&
			 usedAnim->m_animation->GetAnimation()->HasExtractedMotion() )
		{
			const Float startTime = usedAnim->m_currTime;
			Float endTime = startTime + inTime * usedAnim->m_playbackSpeed;
			const Float duration = usedAnim->m_animation->GetDuration();
			Int32 loops = 0;
			if ( usedAnim->m_looped )
			{
				while ( endTime >= duration )
				{
					endTime -= duration;
					++ loops;
				}
			}
			else
			{
				endTime = Min( endTime, duration );
			}
			const AnimQsTransform movement = usedAnim->m_animation->GetAnimation()->GetMovementBetweenTime( startTime, endTime, loops );
			const Matrix movementMatrix = AnimQsTransformToMatrix( movement );
			const Vector animDeltaLocation = startingRotMat.TransformVector( movementMatrix.GetTranslation() );
			endLoc += animDeltaLocation * usedAnim->m_weight;
		}
	}

	return endLoc;
}

Bool CMovingAgentComponent::StartRoadFollowing( Float speed, Float maxAngle, Float maxDistance, Vector& outDirection )
{
	return GCommonGame->GetSystem< CRoadsManager >()->FindClosestRoad( GetWorldPositionRef(), speed, maxAngle, maxDistance, outDirection );
}

void CMovingAgentComponent::ResetRoadFollowing()
{
	GCommonGame->GetSystem< CRoadsManager >()->Reset();
}

Bool CMovingAgentComponent::AdjustRequestedMovementDirectionNavMesh( Vector& directionWS, Float speed, Float maxAngle, Int32 maxIteration, Int32 maxIterationStartSide, Vector& preferedDirection, Bool acceptExploration )
{
	Bool	wayIsObstructed		= false;

	if ( !m_pathAgent || !m_pathAgent->IsOnNavdata() || maxIteration <= 0 ) // || !IsSnapedToNavigableSpace()
	{
		directionWS	= preferedDirection;
		return false;
	}

	Vector	testedDirection;

	// Prepare the sweep test
	Float	collisionRadius		= GetRadius();
	Float	sweepRadius			= 0.25f;
	Float	moveStartForward	= 0.0f;//collisionRadius * 0.5f;
	Vector	startWS				= GetWorldPosition() + directionWS * moveStartForward + Vector(0.0f, 0.0f, 0.2f + sweepRadius);
	Vector	endWS				= startWS + directionWS * ( collisionRadius * 2.0f + speed );
	Float	distance			= collisionRadius * 2.0f + speed;

	// For the path line we want to check from the feet position
	Vector	pointStartFlat		= startWS;
	Vector	pointEndFlat		= endWS;
	pointStartFlat.Z			= GetWorldPosition().Z;
	pointEndFlat.Z				= pointStartFlat.Z;

	// Is it free already?
	wayIsObstructed				= CheckLineCollisionNavmesh( pointStartFlat, pointEndFlat, collisionRadius, acceptExploration );
	if( !wayIsObstructed )
	{
		return false;
	}

	// TODO check for corners and portals if needed

	// Check directions with "the fan" method
	maxAngle			= DEG2RAD( maxAngle );
	Float	stepAngle	= maxAngle / ( float ) maxIteration;

	// Decide which side to check first
	if( preferedDirection	 == Vector::ZEROS ) // Both sides the same priority
	{
		wayIsObstructed		= CheckLineCollisionNavmeshInFan( pointStartFlat, directionWS, 0.0f, stepAngle, maxIteration, distance, collisionRadius, testedDirection, true, acceptExploration );
		if( ! wayIsObstructed )
		{
			directionWS		= testedDirection;
			return true;
		}
	}
	else
	{
		Vector	right		= Vector( directionWS.Y, -directionWS.X, directionWS.Z );
		Bool	leftSide	= Vector::Dot2( preferedDirection, right ) > 0.0f;
		Float	sideSign	= leftSide ? -1.0f : 1.0f;

		// DesiredSide first
		wayIsObstructed		= CheckLineCollisionNavmeshInFan( pointStartFlat, directionWS, 0.0f, sideSign * stepAngle, maxIterationStartSide, distance, collisionRadius, testedDirection, false, acceptExploration );
		if( ! wayIsObstructed )
		{
			directionWS		= testedDirection;
			return true;
		}

		// Secondary side
		wayIsObstructed		= CheckLineCollisionNavmeshInFan( pointStartFlat, directionWS, 0.0f, -sideSign * stepAngle, maxIterationStartSide, distance, collisionRadius, testedDirection, false, acceptExploration );
		if( ! wayIsObstructed )
		{
			directionWS		= testedDirection;
			return true;
		}

		// Remaining both sides
		wayIsObstructed		= CheckLineCollisionNavmeshInFan( pointStartFlat, directionWS, sideSign * stepAngle * maxIterationStartSide, sideSign * stepAngle, maxIteration - maxIterationStartSide, distance, collisionRadius, testedDirection, true, acceptExploration );
		if( ! wayIsObstructed )
		{
			directionWS		= testedDirection;
			return true;
		}
	}

	// We did not reach a safe place
	return false;
}

bool CMovingAgentComponent::AdjustRequestedMovementDirectionPhysics( Vector & directionWS, Bool & shouldStop, Float speed, Float angleToDeflect, Float freeSideDistanceRequired, Bool & cornerDetected, Bool & isPortal )
{
	PC_SCOPE( MAC_AdjustRequestedMovementDirectionPhysics );

	// Init return data
	Bool	corrected	= false;
	shouldStop			= false;
	cornerDetected		= false;
	isPortal			= false;

	// Early skip
	if ( speed < 0.0f || !GetWorld() )
	{
		return false;
	}

	// Prepare the sweep test
	Float collisionRadius	= GetRadius();
	Float sweepRadius		= 0.25f;
	Float moveStartForward	= -collisionRadius * 0.5f;
	Vector startWS			= GetWorldPosition() + directionWS * moveStartForward + Vector(0.0f, 0.0f, 0.2f + sweepRadius);
	Vector endWS			= GetWorldPosition() + directionWS * ( collisionRadius * 2.0f + speed ) + Vector(0.0f, 0.0f, 1.0f);

	// Do the Swipe test
	Vector collisionPosition;
	Vector collisionNormal;
	Bool obstructed;
	obstructed	= CheckLineCollisionsConvenient( startWS, endWS, collisionRadius, collisionPosition, collisionNormal );

	// Treat collision
	if ( obstructed )
	{
		if ( Vector::Dot2( collisionPosition - startWS, endWS - startWS ) > 0.0f // in front 
			&& collisionNormal.Z < 0.707f ) // not absolute value! we ignore elements that we could stand on 45'
		{
			Vector normal2D = collisionNormal.Normalized2();

			// Ignore inclination
			if ( collisionNormal.Z > 0.4f )
			{
				return false;
			}

			corrected	= true;

			// Find the side we need to check
			Vector direction		= directionWS;
			Float const dot			= Vector::Dot2( direction.Normalized3(), normal2D );
			Float const dotLimit	= sin( DEG2RAD( angleToDeflect ) );
			shouldStop				= Abs( dot ) > dotLimit;
			direction				= ( ( direction - normal2D * dot ).Normalized2() - normal2D * 0.1f ).Normalized2();

			 // Early exit
			/*if( shouldStop )
			{
				return false;
			}*/

			// Check if the way is clear
			if( !CheckIfAvoidDirectionIsFree( directionWS, direction, freeSideDistanceRequired, isPortal, collisionPosition.Z ) )
			{
				direction	= -direction;
				// If not, try the other side
				if( !CheckIfAvoidDirectionIsFree( directionWS, direction, freeSideDistanceRequired, isPortal, collisionPosition.Z ) )
				{
					// Failed again, chose the first side even if busy
					direction	= -direction;
				}
				cornerDetected	= true;
				shouldStop		= true;
			}

			// Materialize the direction
			directionWS	= direction;

			// We may need to stop but we keep the corrected direction
			//shouldStop &= contactInfo.m_distance < Max( 0.1f, 0.15f - moveStartForward );
		}
	}

#ifndef NO_EDITOR
	// Debug
	static const CName debugName( TXT("CorrectCheckCollision") );
	if( cornerDetected )
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddArrow( debugName, startWS, endWS, 1.f, 0.2f, 0.2f, Color::RED, true, true, 0.2f );
	}
	else
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddArrow( debugName, startWS, endWS, 1.f, 0.2f, 0.2f, Color::BLUE, true, true, 0.2f );
	}
	if( obstructed )
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugName, 0.2f, collisionPosition, true, Color::BLUE, 0.1f );
	}
#endif

	return	corrected;
}


bool CMovingAgentComponent::CheckIfAvoidDirectionIsFree( Vector& originalDirection, Vector& direction, Float freeSideDistanceRequired, Bool& isPortal, Float height )
{
	// Init out params
	isPortal	= false;


	// Prepare distances
	Float collisionRadius		= GetRadius();
	Float sweepRadius			= 0.12f;
	Float distanceFreeNeeded	= freeSideDistanceRequired;
	Float moveStartForward		= 0.0f; //collisionRadius * 0.5f;

	// Prepare the start and end positions
	Vector startWS				= GetWorldPosition();
	startWS.Z					= height + 0.2f + sweepRadius;
	Vector endWS				= startWS + direction * distanceFreeNeeded;


	// Do the Swipe test
	Vector collisionPosition;
	Vector collisionNormal;
	Bool wayIsObstructed		= CheckLineCollisionsConvenient( startWS, endWS, collisionRadius, collisionPosition, collisionNormal );


	// Debug
#ifndef NO_EDITOR
	static const CName debugName( TXT("CorrectCheckWall") );
	Cast<CActor>( GetEntity() )->GetVisualDebug()->AddArrow( debugName, startWS, endWS, 1.f, 0.2f, 0.2f, Color::YELLOW, true, true, 0.2f );
	if( wayIsObstructed )
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugName, 0.2f, collisionPosition, true, Color::YELLOW, 0.1f );
	}
#endif

	// Apparently obstructed
	if( wayIsObstructed )
	{
		// Check if there is space to walk in
		if(  CheckIfAvoidInDirectionHasPortal( originalDirection, direction, collisionPosition, collisionNormal ) )
		{
			isPortal	= true;
			return true;
		}

		// Really obstructed
		else
		{
			return false;
		}
	}

	// Free space
	else
	{
		return true;
	}
}

bool CMovingAgentComponent::CheckIfAvoidInDirectionHasPortal( Vector& originalDirection, Vector& direction, Vector& collisionPoint, Vector& collisionNormal )
{
	//Prepare distances and direction
	Float collisionRadius		= ::Max( GetRadius() - 0.2f, 0.1f );
	Float distanceFreeNeeded	= 1.0f;
	Vector directionToCheck;
	directionToCheck.X		= direction.Y;
	directionToCheck.Y		= -direction.X;
	directionToCheck.Z		= 0.0f;
	directionToCheck.Normalize3();
	if(  Vector::Dot2(directionToCheck, originalDirection ) < 0.0f )
	{
		directionToCheck	= -directionToCheck;
	}

	// Prepare the start and end positions
	Vector startWS			= collisionPoint + collisionNormal * ( collisionRadius + 0.2f );
	Vector endWS			= startWS + directionToCheck * distanceFreeNeeded;


	// Do the Swipe test
	Bool wayIsObstructed	= CheckLineCollisionsConvenient( startWS, endWS, collisionRadius, collisionPoint, collisionNormal );


	// Debug
#ifndef NO_EDITOR
	static const CName debugName( TXT("CorrectCheckPortal") );
	Cast<CActor>( GetEntity() )->GetVisualDebug()->AddArrow( debugName, startWS, endWS, 1.f, 0.2f, 0.2f, Color::GREEN, true, true, 0.2f );
	if( wayIsObstructed )
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugName, 0.2f, collisionPoint, true, Color::GREEN, 0.1f );
	}
#endif


	// For debug purposes we have this if
	if( wayIsObstructed )
	{
		return false;
	}

	return true;
}

Bool CMovingAgentComponent::CheckLineCollisionNavmesh( Vector& pointStart, Vector& pointEnd, Float collisionRadius, bool acceptExploration )
{
	PC_SCOPE( MAC_CheckLineCollisionNAvmesh );

	Bool	wayIsObstructed	= false;
	
	CPathLibWorld* pathlib = GetWorld()->GetPathLibWorld();
	if ( !pathlib )
	{
		return wayIsObstructed;
	}

	PathLib::AreaId areaId	= m_pathAgent->GetCachedAreaId();
	Vector3 newPointEnd;

	const PathLib::EClearLineTestResult testLineResult = pathlib->GetClearLineInDirection( areaId, pointStart, pointEnd, collisionRadius, newPointEnd, PathLib::CT_DEFAULT );
	if ( testLineResult == PathLib::CLEARLINE_SUCCESS )
	{
		wayIsObstructed		= false;
	}
	else if ( testLineResult == PathLib::CLEARLINE_INVALID_START_POINT )
	{
		wayIsObstructed		= true;
	}
	else
	{
		Vector3 testPos;
		wayIsObstructed		= pathlib->GetClearLineInDirection( areaId, newPointEnd, pointEnd, collisionRadius, testPos, PathLib::CT_DEFAULT ) != PathLib::CLEARLINE_SUCCESS;
	}
	
	Float	height			= pointEnd.Z;
	Bool	checkingFall	= false;
	Bool	fallValid		= false;
	Vector2	fallVector		= pointEnd;
	Vector	fallLocation	= pointEnd;
	Float	heightMin		= 0.0f;
	Float	heightMax		= 0.0f;

	// Try to get exploration and fall
	if( acceptExploration && wayIsObstructed )
	{
		// Fall
		checkingFall	= true;

		const static float HEIGHT_TO_FALL_AND_PREDICT_DOWN	= 6.0f;
		const static float HEIGHT_TO_FALL_AND_PREDICT_UP	= 0.3f;
		const static float DIST_TO_FALL_AND_PREDICT			= 0.5f;

		// prepare point to check fall
		fallVector		= pointEnd - pointStart;
		fallVector		*= 1.0f - ( DIST_TO_FALL_AND_PREDICT / fallVector.Mag() );
		fallVector		+= pointStart;

		// Prepare heights to fall
		heightMin		= pointEnd.Z - HEIGHT_TO_FALL_AND_PREDICT_DOWN;
		heightMax		= pointEnd.Z + HEIGHT_TO_FALL_AND_PREDICT_UP;
		PathLib::AreaId		areaId;

		// Is there navmesh after the fall?
		if ( pathlib->ComputeHeight( fallVector, heightMin, heightMax, height,  areaId ) )
		{
			// Agent fits?
			fallLocation.X	= fallVector.X;
			fallLocation.Y	= fallVector.Y;
			fallLocation.Z	= height;
			pointEnd.Z		= height;
			fallValid		= m_pathAgent->TestLine( fallLocation, pointEnd, collisionRadius ); //TestLocation
			if( fallValid )
			{
				wayIsObstructed	= false;
			}
		}
		// Exploration, todo
	}

	// Debug
#ifndef NO_EDITOR
	static const CName debugNameFail( TXT("CorrectCheckWallFail") );
	static const CName debugNameSuccess( TXT("CorrectCheckWallSuccess") );
	static const CName debugNameFall( TXT("CorrectionFall") );
	static const CName debugNameFallUp( TXT("CorrectionFallLocationUp") );
	static const CName debugNameFallDown( TXT("CorrectionFallLocationUp") );
	if( wayIsObstructed )
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddArrow( debugNameFail, pointStart, pointEnd, 1.f, 0.2f, 0.2f, Color::RED, true, true, 0.2f );
	}
	else
	{
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddArrow( debugNameSuccess, pointStart, pointEnd, 1.f, 0.2f, 0.2f, Color::GREEN, true, true, 0.2f );
	}
	if( checkingFall )
	{
		Vector checkLocation	= fallVector;
		checkLocation.Z			= heightMax;
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugNameFallUp, 0.3f, checkLocation, true, Color::DARK_BLUE );
		checkLocation.Z			= heightMin;
		Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugNameFallDown, 0.3f, checkLocation, true, Color::BLUE );

		if( fallValid )
		{
			Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugNameFall, 0.3f, fallLocation, true, Color::GREEN );
		}
		else 
		{
			Cast<CActor>( GetEntity() )->GetVisualDebug()->AddSphere( debugNameFall, 0.3f, fallLocation, true, Color::RED );
		}
	}
#endif

	return wayIsObstructed;
}

bool CMovingAgentComponent::CheckLineCollisionNavmeshWithYaw( Vector& pointStart, Vector& direction, Float angle, Float distance, Float collisionRadius, Vector& resultingDirection, bool acceptExploration )
{
	RedVector4	rotatedDirectionRed	= RedVector4( direction.X, direction.Y, direction.Z );

	AxisRotateVector( rotatedDirectionRed, RedVector4::EZ, angle );

	Vector	pointEnd	= pointStart + Vector( rotatedDirectionRed.X, rotatedDirectionRed.Y, rotatedDirectionRed.Z ) * ( distance - 1.5f );

	Bool	obstructed	= CheckLineCollisionNavmesh( pointStart, pointEnd, collisionRadius, acceptExploration );
	if( !obstructed )
	{
		resultingDirection	= Vector( rotatedDirectionRed.X, rotatedDirectionRed.Y, rotatedDirectionRed.Z );
	}
	// Debug
#ifndef NO_EDITOR
	RED_LOG(MovementCorrection, TXT("Checking Navmesh LineTest for correction with angle %f, dir: ( %f, %f, %f )"), angle, rotatedDirectionRed.X, rotatedDirectionRed.Y, rotatedDirectionRed.Z );
#endif

	return obstructed;
}

bool CMovingAgentComponent::CheckLineCollisionNavmeshInFan( Vector& pointStart, Vector& direction, Float angleStart, Float angleStep, int steps, Float distance, Float collisionRadius, Vector& resultingDirection, Bool simetric, bool acceptExploration )
{
	bool wayIsObstructed;

	for( int i = 0; i < steps; ++i )
	{
		Float		fanAngle			= angleStart + ( i + 1.0f ) * angleStep;
		Vector		testedDirection;

		// One side
		wayIsObstructed				= CheckLineCollisionNavmeshWithYaw( pointStart, direction, fanAngle, distance, collisionRadius, testedDirection, acceptExploration );
		if( !wayIsObstructed )
		{
			resultingDirection		= testedDirection;
			return	false;
		}

		// The other side
		if( simetric )
		{
			wayIsObstructed				= CheckLineCollisionNavmeshWithYaw( pointStart, direction, -fanAngle, distance, collisionRadius, testedDirection, acceptExploration );
			if( !wayIsObstructed )
			{
				resultingDirection		= testedDirection;
				return	false;
			}
		}
	}

	return	true;
}


bool CMovingAgentComponent::CheckLineCollisionsConvenient( const Vector& pointStart, const Vector& pointEnd, Float radius, Vector& collisionPoint, Vector& collisionNormal  )
{
	Bool wayIsObstructed	= false;
	

	SPhysicsContactInfo	contactInfo;
	CPhysicsWorld* physWorld = nullptr;

	if( GetWorld()->GetPhysicsWorld( physWorld ) )
	{
		const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
		const static CPhysicsEngine::CollisionMask excludeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
		wayIsObstructed	= physWorld->SweepTestWithSingleResult( pointStart, pointEnd, radius, includeMask, excludeMask, contactInfo ) == TRV_Hit;

		// Fill the data from the contact info
		if( wayIsObstructed )
		{
			collisionPoint	= contactInfo.m_position;
			collisionNormal	= contactInfo.m_normal;
		}
	}

	return wayIsObstructed;
}

void CMovingAgentComponent::OnCutsceneDebugCheck()
{
	TBaseClass::OnCutsceneDebugCheck();

	if ( CMovementAdjustor* a = GetMovementAdjustor() )
	{
		if ( a->HasAnyActiveRequest() )
		{
			HALT( "CMovementAdjustor has some active requests during cutscene" );
		}

		a->CancelAll();
	}
}

void CMovingAgentComponent::OnCutsceneStarted()
{
	TBaseClass::OnCutsceneStarted();

	if ( CMovementAdjustor* a = GetMovementAdjustor() )
	{
		if ( a->HasAnyActiveRequest() )
		{
			HALT( "CMovementAdjustor has some active requests during cutscene" );
		}

		a->CancelAll();
	}
}

void CMovingAgentComponent::OnCutsceneEnded()
{
	TBaseClass::OnCutsceneEnded();
}

//////////////////////////////////////////////////////////////////////////////////////////////////

void CMovingAgentComponent::funcSetMaxMoveRotationPerSec( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, rotSpeed, NumericLimits< Float >::Max() );
	FINISH_PARAMETERS;
	SetMaxRotation( rotSpeed );
}

void CMovingAgentComponent::funcSetMoveType( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EMoveType, moveType, MT_Run );
	FINISH_PARAMETERS;
	SetMoveType( moveType );
}

void CMovingAgentComponent::funcGetCurrentMoveSpeedAbs( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( m_maxSpeed );
}


void CMovingAgentComponent::funcAddDeltaMovement( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, translation, Vector::ZEROS);
	GET_PARAMETER( EulerAngles, rotation, EulerAngles::ZEROS);
	FINISH_PARAMETERS;

	AddDeltaMovement(translation, rotation);
}

void CMovingAgentComponent::funcTeleportBehindCamera( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, continueMovement, true );
	FINISH_PARAMETERS;

	if ( !continueMovement )
	{
		CancelMove();
	}

	const CCameraDirector* cd = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetCameraDirector() : nullptr;
	if ( !cd )
	{
		RETURN_BOOL( false );
		return;
	}

	const Vector targetPosition = cd->GetCameraPosition() - cd->GetCameraForward() * 5.f;

	if ( m_pathAgent )
	{
		// TODO: support m_pathAgent
		ASSERT( false, TXT("NOT YET IMPLEMENTED!!!") );
	}

	Bool res = TeleportTo( targetPosition, GetEntity()->GetWorldRotation() );
	RETURN_BOOL( res );
}

void CMovingAgentComponent::funcEnableCombatMode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, combat, true );
	FINISH_PARAMETERS;
	EnableCombatMode( combat );
}

void CMovingAgentComponent::funcEnableVirtualController( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( CName, virtualControllerName, CName::NONE );
	GET_PARAMETER( Bool, enabled, true );
	FINISH_PARAMETERS;
	EnableVirtualController( virtualControllerName, enabled );
}

void CMovingAgentComponent::funcSetVirtualRadius( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, radiusName, CName::NONE );
    GET_PARAMETER_OPT( CName, virtualControllerName, CName::NONE );
	FINISH_PARAMETERS;
	SetVirtualRadius( radiusName, false, virtualControllerName );
}

void CMovingAgentComponent::funcSetVirtualRadiusImmediately( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, radiusName, CName::NONE );
	FINISH_PARAMETERS;
	SetVirtualRadius( radiusName, true, CName::NONE );
}

void CMovingAgentComponent::funcResetVirtualRadius( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER_OPT( CName, virtualControllerName, CName::NONE );
	FINISH_PARAMETERS;
	ResetVirtualRadius( virtualControllerName );
}

void CMovingAgentComponent::funcSetHeight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, height, 0.0f );
	FINISH_PARAMETERS;
	SetHeight( height );
}

void CMovingAgentComponent::funcResetHeight( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ResetHeight();
}

void CMovingAgentComponent::funcGetSpeed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetAbsoluteMoveSpeed() );
}

void CMovingAgentComponent::funcGetRelativeMoveSpeed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetRelativeMoveSpeed() );
}

void CMovingAgentComponent::funcGetMoveTypeRelativeMoveSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EMoveType, moveType, MT_Walk );
	FINISH_PARAMETERS;


	if ( m_speedConfig )
	{
		RETURN_FLOAT( m_speedConfig->GetMoveTypeRelativeMoveSpeed( moveType ) );
	}
	else if ( m_skeleton )
	{
		RETURN_FLOAT( m_skeleton->deprec_GetMoveTypeRelativeMoveSpeed( moveType ) );
	}
}


void CMovingAgentComponent::funcForceSetRelativeMoveSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, relativeMoveSpeed, 0.0f );
	FINISH_PARAMETERS;
	
	ForceSetRelativeMoveSpeed( relativeMoveSpeed );
}

void CMovingAgentComponent::funcSetGameplayRelativeMoveSpeed( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, relativeMoveSpeed, 0.0f );
	FINISH_PARAMETERS;
	
	m_gameplayRelativeMoveSpeed = relativeMoveSpeed;
}

void CMovingAgentComponent::funcSetGameplayMoveDirection( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, actorMoveDirection, 0.0f );
	FINISH_PARAMETERS;
	
	m_gameplayMoveDirection = EulerAngles::NormalizeAngle( actorMoveDirection );
	m_gameplayDirectionSet	= true;
}

void CMovingAgentComponent::funcSetDirectionChangeRate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, maxDirectionChangeRate, 360.0f );
	FINISH_PARAMETERS;
	
	SetDirectionChangeRate( maxDirectionChangeRate );
}

void CMovingAgentComponent::funcGetMaxSpeed( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_FLOAT( GetMaxSpeed() );
}

void CMovingAgentComponent::funcGetVelocity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetVelocity() );
}

void CMovingAgentComponent::funcGetVelocityBasedOnRequestedMovement( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	Vector displacement	= GetVelocityBasedOnRequestedMovement();
	RETURN_STRUCT( Vector, displacement );
}

void CMovingAgentComponent::funcAdjustRequestedMovementDirectionPhysics( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, directionWS, Vector::ZEROS );
	GET_PARAMETER_REF( Bool, shouldStop, false );
	GET_PARAMETER( Float, speed, 0.0f );
	GET_PARAMETER( Float, angleToDeflect, 70.0f );
	GET_PARAMETER( Float, freeSideDistanceRequired, 2.0f );
	GET_PARAMETER_REF( Bool, cornerDetected, false );
	GET_PARAMETER_REF( Bool, isPortal, false );
	FINISH_PARAMETERS;
	
	Bool corrected;

	corrected	= AdjustRequestedMovementDirectionPhysics( directionWS, shouldStop, speed, angleToDeflect, freeSideDistanceRequired, cornerDetected, isPortal );

	RETURN_BOOL( corrected );
}

void CMovingAgentComponent::funcAdjustRequestedMovementDirectionNavMesh( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, directionWS, Vector::ZEROS );
	GET_PARAMETER( Float, speed, 0.0f );
	GET_PARAMETER_OPT( Float, maxAngle, 90.0f );
	GET_PARAMETER_OPT( Int32, maxIteration, 10 );
	GET_PARAMETER_OPT( Int32, maxIterationStartSide, 5 );
	GET_PARAMETER_OPT( Vector, preferedDirection, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, checkExploration, false );
	FINISH_PARAMETERS;

	Bool corrected;

	corrected	= AdjustRequestedMovementDirectionNavMesh( directionWS, speed, maxAngle, maxIteration, maxIterationStartSide, preferedDirection, checkExploration );

	RETURN_BOOL( corrected );
}

void CMovingAgentComponent::funcStartRoadFollowing( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, speed, 0.f );
	GET_PARAMETER( Float, maxAngle, 45.f );
	GET_PARAMETER( Float, maxDistance, 4.f );
	GET_PARAMETER_REF( Vector, direction, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_BOOL( StartRoadFollowing( speed, maxAngle, maxDistance, direction ) );
}

void CMovingAgentComponent::funcResetRoadFollowing( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	ResetRoadFollowing();
}

void CMovingAgentComponent::funcGetAgentPosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, GetAgentPosition() );
}

void CMovingAgentComponent::funcGetPathPointInDistance( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, pathDistance, 0.f );
	GET_PARAMETER_REF( Vector, position, m_representationStack->GetStackPosition() );
	FINISH_PARAMETERS;
	Bool foundLocation = false;
	if ( m_pathAgent->IsPathfollowing() )
	{
		foundLocation = m_pathAgent->GetPathPointInDistance( pathDistance, position.AsVector3(), true );
	}

	RETURN_BOOL( foundLocation );
}

void CMovingAgentComponent::funcSnapToNavigableSpace( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, snap, true );
	FINISH_PARAMETERS;
	SnapToNavigableSpace( snap );
	if ( snap )
	{
		PullToNavigableSpace();
	}
}

void CMovingAgentComponent::funcIsOnNavigableSpace( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_pathAgent && m_pathAgent->IsOnNavdata() );
}

void CMovingAgentComponent::funcIsEntityRepresentationForced( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( m_isEntityRepresentationForced );
}

void CMovingAgentComponent::funcGetLastNavigablePosition( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, m_lastNavigableAreaPosition );
}

void CMovingAgentComponent::funcCanGoStraightToDestination( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, destination, Vector( 0, 0, 0 ) );
	FINISH_PARAMETERS;

	Bool canGo = false;
	if ( m_pathAgent )
	{
		canGo = m_pathAgent->TestLine( destination.AsVector3() );
	}
	
	RETURN_BOOL( canGo );
}

void CMovingAgentComponent::funcIsPositionValid( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector( 0, 0, 0 ) );
	FINISH_PARAMETERS;

	Bool isValid = false;
	if ( m_pathAgent )
	{
		isValid = m_pathAgent->TestLocation( position.AsVector3() );
	}
	RETURN_BOOL( isValid );
}

void CMovingAgentComponent::funcGetEndOfLineNavMeshPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector( 0, 0, 0 ) );
	GET_PARAMETER_REF( Vector, outPos, Vector( 0, 0, 0 ) );
	FINISH_PARAMETERS;

	Bool isPositionFound = false;
	if ( m_pathAgent )
	{
		// TODO: Support m_pathAgent
		ASSERT( false, TXT("NOT YET IMPLEMENTED") );
	}
	
	RETURN_BOOL( isPositionFound );
}

void CMovingAgentComponent::funcIsEndOfLinePositionValid( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector( 0, 0, 0 ) );
	FINISH_PARAMETERS;

	Bool isValid = IsEndOfLinePositionValid( position );	
	RETURN_BOOL( isValid );
}

void CMovingAgentComponent::funcIsInSameRoom( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, destination, Vector( 0, 0, 0 ) );
	FINISH_PARAMETERS;

	Bool inSameRoom = true;
	ASSERT( false, TXT("NOT YET IMPLEMENTED") );

	RETURN_BOOL( inSameRoom );
}

void CMovingAgentComponent::funcGetMovementAdjustor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetMovementAdjustor() );
}

void CMovingAgentComponent::funcPredictWorldPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, inTime, 1.0f );
	FINISH_PARAMETERS;

	RETURN_STRUCT( Vector, PredictWorldPosition( inTime ) );
}

void CMovingAgentComponent::funcSetTriggerActivatorRadius( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, radius, 0.3f );
	FINISH_PARAMETERS;

	SetTriggerActivatorRadius(radius);
}

void CMovingAgentComponent::funcSetTriggerActivatorHeight( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, height, 1.0f );
	FINISH_PARAMETERS;

	SetTriggerActivatorHeight(height);
}

void CMovingAgentComponent::funcAddTriggerActivatorChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, TC_Default );
	FINISH_PARAMETERS;

	AddTriggerActivatorChannel(channel);
}

void CMovingAgentComponent::funcRemoveTriggerActivatorChannel( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( ETriggerChannel, channel, TC_Default );
	FINISH_PARAMETERS;

	RemoveTriggerActivatorChannel(channel);
}

void CMovingAgentComponent::funcSetEnabledFeetIK(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( Bool, enable, false );
	GET_PARAMETER_OPT( Float, blendTime, 0.2f );
	FINISH_PARAMETERS;

	SetEnabledFeetIK( enable, blendTime );
}

void CMovingAgentComponent::funcSetEnabledHandsIK(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	SetEnabledHandsIK( enable );
}

void CMovingAgentComponent::funcSetHandsIKOffsets(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( Float, left, 0.0f );
	GET_PARAMETER( Float, right, 0.0f );
	FINISH_PARAMETERS;

	SetHandsIKOffsets( left, right );
}

void CMovingAgentComponent::funcGetEnabledFeetIK(CScriptStackFrame & stack, void * result)
{
	FINISH_PARAMETERS;

	RETURN_BOOL( GetEnabledFeetIK() );
}

void CMovingAgentComponent::funcSetEnabledSlidingOnSlopeIK(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( Bool, enable, false );
	FINISH_PARAMETERS;

	SetEnabledSlidingOnSlopeIK( enable );
}

void CMovingAgentComponent::funcGetEnabledSlidingOnSlopeIK(CScriptStackFrame & stack, void * result)
{
	FINISH_PARAMETERS;

	RETURN_BOOL( GetEnabledSlidingOnSlopeIK() );
}

void CMovingAgentComponent::funcSetUseEntityForPelvisOffset(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( THandle< CEntity >, entity, nullptr );
	FINISH_PARAMETERS;

	SetUseEntityForPelvisOffset( entity.Get( ) );
}

void CMovingAgentComponent::funcGetUseEntityForPelvisOffset(CScriptStackFrame & stack, void * result)
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( GetUseEntityForPelvisOffset() );
}

void CMovingAgentComponent::funcSetAdditionalOffsetWhenAttachingToEntity(CScriptStackFrame & stack, void * result)
{
	GET_PARAMETER( THandle< CEntity >, entity, nullptr );
	GET_PARAMETER( Float, time, 0.0f );
	FINISH_PARAMETERS;

	SetAdditionalOffsetWhenAttachingToEntity( entity.Get( ), time );
}

void CMovingAgentComponent::funcSetAdditionalOffsetToConsumePointWS( CScriptStackFrame & stack, void * result )
{
	GET_PARAMETER( Matrix, posWS, Matrix::IDENTITY );
	GET_PARAMETER( Float, time, 0.f );
	FINISH_PARAMETERS;

	SetAdditionalOffsetToConsumePointWS( posWS, time );
}

void CMovingAgentComponent::funcSetAdditionalOffsetToConsumeMS( CScriptStackFrame & stack, void * result )
{
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	GET_PARAMETER( EulerAngles, rot, EulerAngles::ZEROS );
	GET_PARAMETER( Float, time, 0.f );
	FINISH_PARAMETERS;

	SetAdditionalOffsetToConsumeMS( pos, rot, time );
}

void CMovingAgentComponent::ForceStandPhysicalMaterial(const SPhysicalMaterial* forcedStandPhysicalMaterial )
{
	m_forcedStandPhysicalMaterials.PushBack(forcedStandPhysicalMaterial);
}

void CMovingAgentComponent::ReleaseStandPhysicalMaterial(const SPhysicalMaterial* forcedStandPhysicalMaterial)
{
	m_forcedStandPhysicalMaterials.RemoveFast(forcedStandPhysicalMaterial);
}

const EulerAngles& CMovingAgentComponent::GetTeleportedRotationOrWSRotation() const
{
	if ( m_teleported )
		return m_teleportToRot;
	
	return GetEntity()->GetRotation();
}
