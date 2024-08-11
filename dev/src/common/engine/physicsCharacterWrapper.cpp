//////////////////////////////////////////////////////////////////////////
// headers
#include "build.h"
#include "physicsCharacterWrapper.h"
#include "physicsCharacterVirtualController.h"
#include "characterControllerParam.h"
#include "clipMap.h"
#include "characterControllerManager.h"

#ifdef USE_PHYSX
#include "../core/mathUtils.h"
#include "../physics/physXEngine.h"
#include "../physics/physicsWorld.h"

#include "../../../external/PhysX3/Source/PhysXCharacterKinematic/src/CctController.h"
#include "../../../external/PhysX3/Source/PhysXCharacterKinematic/src/CctCapsuleController.h"
#include "../../../external/PhysX3/Source/PhysXCharacterKinematic/src/CctInternalStructs.h"
#endif

#include "../core/dataError.h"
#include "../core/math.h"
#include "../engine/gameTimeManager.h"
#include "../engine/debugServerManager.h"

#include "../physics/physicsSettings.h"
#include "pathlibWorld.h"
#include "../physics/physicsWorldUtils.h"
#include "entity.h"
#include "animatedComponent.h"
#include "game.h"
#include "environmentManager.h"
#include "renderFrame.h"
#include "layer.h"
#include "world.h"
#include "../physics/physicsRagdollWrapper.h"
#include "sectorData.h"
#include "utils.h"
#include "../physics/PhysicsWrappersDefinition.h"
#include "baseEngine.h"
//////////////////////////////////////////////////////////////////////////
// namespaces
using namespace physx;

#ifdef USE_PHYSX
DECLARE_PHYSICS_WRAPPER(CPhysicsCharacterWrapper,EPW_Character,false,false)
#endif
//////////////////////////////////////////////////////////////////////////
// consts
const Float SCCTDefaults::DEFAULT_HEIGHT				( 1.8f );
const Float SCCTDefaults::DEFAULT_RADIUS				( 0.4f );
const Float SCCTDefaults::DEFAULT_STEP_OFFSET			( 0.35f );
const Float SCCTDefaults::DEFAULT_MAX_SLOPE				( 40.f );
const Float SCCTDefaults::DEFAULT_CONTACT_OFFSET		( 0.10f );
const CName& SCCTDefaults::DEFAULT_COLLISION_TYPE		( CNAME( Character ) );
const Float SCCTDefaults::MAX_SEP						( 2.f );
const Float SCCTDefaults::SEP_VEC_FACTOR				( 1.001f );
const Float SCCTDefaults::TIME_OFF_PLATFORM_THRESH      ( 0.2f );
const Float SCCTDefaults::PLATTFORM_DETACH_THRESHOLD    ( 0.008f );
const Float minMoveDelta								= 0.00001f;
const Float waterLevelThreshold							= 0.05f;
const Float additionalTerrainOverlapTestOffsetZ			= 0.25f;
const Float obstacleAddRadius							= 0.2f;
const Float orientOffsetMul								= 0.8f;
const Float controllerVsDynamicSeparationSpeed			= 0.3f;
const Float controllerVsDynamicOffsetZ					= -0.25f;
const Float controllerVsDynamicImpulseMul				= 0.1f;
const Float terrainNormalAddMul							= 1.0f;
const Float isCollsionOverlapOffsetZ					= -0.1f;
const Float isCollsionRadiusDiff						= -0.05f;
const Float virtualControllerSeparationDiffMax			= 0.05f;
const Float submergeMultiplierUnderCollider				= 0.1f;
const Float correctZDamping								= 0.1f;

// collision with spherically approximated scene
const Float collideSphereRadius							= 0.275f;
const Float collideSphereRadiusOffset					= -0.05f;
const Float collideSphereHeight							= 0.555f;
const Float collideSphereMinDist						= 0.05f;
const Float collideSphereLifeTime						= 0.5f;
const Float collideSphereSmooth							= 100.0f;
const Uint32 collideSphereIterations					= 10;
const Float collideSphereBackOffMul						= 0.25f;

const SCollisionData SCollisionData::EMPTY;
const InteractionPriorityType SCCTDefaults::DEFAULT_INTERACTION_PRIORITY = InteractionPriorityTypeNotSet;

//////////////////////////////////////////////////////////////////////////
// rtti
IMPLEMENT_ENGINE_CLASS( SCollisionData );

IMPLEMENT_RTTI_ENUM( ECharacterPhysicsState );
IMPLEMENT_RTTI_ENUM( ECollisionSides );


//////////////////////////////////////////////////////////////////////////
// cnames
RED_DEFINE_STATIC_NAME( OnInAirStarted );
RED_DEFINE_STATIC_NAME( OnHitGround );
RED_DEFINE_STATIC_NAME( OnHitCeiling );
RED_DEFINE_STATIC_NAME( ParticleCollider );
RED_DEFINE_STATIC_NAME( OnOceanTriggerEnter );
RED_DEFINE_STATIC_NAME( OnOceanTriggerLeave );
RED_DEFINE_STATIC_NAME( OnDivingEnd );
RED_DEFINE_STATIC_NAME( OnWaterBottomTouch );
RED_DEFINE_STATIC_NAME( ToSwim );
RED_DEFINE_STATIC_NAME( OnDamageFromFalling );
RED_DEFINE_STATIC_NAME( OnRagdollOnGround );
RED_DEFINE_STATIC_NAME( OnRagdollInAir );
RED_DEFINE_STATIC_NAME( OnNoLongerInRagdoll );
RED_DEFINE_STATIC_NAME( OnRagdollTouch );
RED_DEFINE_STATIC_NAME( OnRagdollStart );
RED_DEFINE_STATIC_NAME( OnRagdollUpdate );
RED_DEFINE_STATIC_NAME( OnRagdollIsAwayFromCapsule );
RED_DEFINE_STATIC_NAME( OnRagdollCloseToCapsule );
RED_DEFINE_STATIC_NAME( DeniedArea );
RED_DEFINE_STATIC_NAME( Dangles );
RED_DEFINE_STATIC_NAME( BoatDestruction );
RED_DEFINE_STATIC_NAME( BoatSide );
RED_DEFINE_STATIC_NAME( BoatCollider );
RED_DEFINE_STATIC_NAME( sliding );

//////////////////////////////////////////////////////////////////////////
//
// ctor
SPhysicsCharacterWrapperInit::SPhysicsCharacterWrapperInit()
	: m_params					( nullptr )
	, m_needsHitCallback		( false )
	, m_needsBehaviorCallback	( false )
	, m_combatMode				( false )
	, m_canBePlayerControlled	( false )
	, m_vehicle					( false )
{
}


#ifdef USE_PHYSX

using namespace physx;

//////////////////////////////////////////////////////////////////////////
//
// ctor
CPhysicsCharacterWrapper::CPhysicsCharacterWrapper( SPhysicsCharacterWrapperInit& init )
	: CPhysicsWrapperInterface			()
	, m_enabled							( false )
	, m_state							( CPS_Falling )
	, m_filterData						( init.m_params->m_collisionType )
	, m_footPosition					( init.m_initialPosition )
	, m_internalVelocity				( Vector::ZEROS )
	, m_moveExternalDisp				( Vector::ZEROS )
	, m_moveInputDisp					( Vector::ZEROS )
	, m_lastMoveVec						( Vector::ZEROS )
	, m_currentStandPhysicalMaterial	( nullptr )
	, m_collisions						( 0 )
	, m_teleport						( false )
	, m_teleportCorrectZ				( false )
	, m_gravity							( true )
	, m_ocean							( false )
	, m_isDiving						( false )
	, m_staticCollisionsEnabled			( true )
	, m_dynamicCollisionsEnabled		( true )
	, m_collisionDown					( false )
	, m_collisionUp						( false )
	, m_collisionSides					( false )
	, m_correctPosition					( false )
	, m_shapeHit						( false )
	, m_isUpdatingVirtualRadius			( false )
	, m_ragdollIsAway					( false )
	, m_switchFromRagdollToSwimming		( false )
	, m_ragdollInAir					( false )
	, m_ragdollPushingMultiplier		( 1.0f )
	, m_slidingState					( SS_None )
	, m_slideCoef						( 0.0f )
	, m_slidingLimitMin					( GGame->GetGameplayConfig().m_slidingLimitMin )
	, m_slidingLimitMax					( GGame->GetGameplayConfig().m_slidingLimitMax )
	, m_slidingSpeed					( 10.0f )
	, m_slidingEnabled					( true )
	, m_slidingDir						( Vector::ZEROS )
	, m_terrainLimitMin					( GGame->GetGameplayConfig().m_terrainInfluenceLimitMin )
	, m_terrainLimitMax					( GGame->GetGameplayConfig().m_terrainInfluenceLimitMax )
	, m_terrainLimitMul					( GGame->GetGameplayConfig().m_terrainInfluenceMul )
	, m_speedMul						( 1.0f )
	, m_normalShapeHits					( 0 )
	, m_terrainNormal					( Vector::EZ )
	, m_orient							( 0.0f, 1.0f, 0.0f, 0.0f )
	, m_waterLevel						( -10000.0f )
	, m_waterDepth						( 0.0f )
	, m_emergeSpeed						( GGame->GetGameplayConfig().m_emergeSpeed )
	, m_submergeSpeed					( GGame->GetGameplayConfig().m_submergeSpeed )
	, m_fallingTime						( 0.0f )
	, m_isVehicle						( init.m_vehicle )
	, m_canBePlayerControlled			( init.m_canBePlayerControlled )				// for future usage
	, m_collisionObstaclesDataCount		( 0 )
	, m_collisionCharactersDataCount	( 0 )
	, m_platformAttachPointLocal		( Vector::ZEROS )
    , m_platformCurrentDisplacement     ( Vector::ZEROS )
    , m_currMovementVector				( Vector::ZEROS )
#ifdef USE_PLATFORM_ROTATION
	, m_platformAttachDir				( Vector::ZEROS )
	, m_platformRotInfluenceRad			( 0.0f )
#endif
	, m_virtualPitch					( 0.0f )
	, m_collisionPrediction				( false )
	, m_collisionPredictionMovementAdd	( 0.0f )
	, m_collisionPredictionMovementMul	( 1.0f )
	, m_collisionPredictionEventName	( CName::NONE )
	, m_characterController				( nullptr )
	, m_lastResDist						( 1.0f )
	, m_pushingTime						( 0.0f )
	, m_fallingStartPos					( Vector::ZEROS )
	, m_canPush							( false )
	, m_needsBehaviorCallback			( true )
#ifndef NO_EDITOR
	, dbg_platformAttachGlobal			( Vector::ZEROS )
#endif
#ifdef USE_NEW_PRIORITY_SYSTEM
    , m_interactionPriority				( init.m_params->m_interactionPriorityEnum )
#else
    , m_interactionPriority				( init.m_params->m_interactionPriority )
#endif
	, m_unpushableTarget				( nullptr )
	, m_onNoLongerInRagdoll				( false )
	, m_sphereCollidersCounter			( 0 )
	, m_lastWaterLevelUpdateTick		( 0 )
{
	m_characterControllersManager.Reset( nullptr );

	//hack to removal
	CComponent* component;
	if( GetParentProvider()->GetParent( component ) )
	{
		m_isPlayerControlled = component->GetEntity()->IsPlayer();
		m_isInGame = component->GetEntity()->IsInGame();
	}
	//hack to removal

	GetParentProvider()->GetPhysicsWorld( m_world );

	// debug native properties
	if ( m_isPlayerControlled )
	{
		//DBGSRV_REG_NATIVE_PROP( "terrainZ" );
		//DBGSRV_REG_NATIVE_PROP( "terrainZRaw" );
		//DBGSRV_REG_NATIVE_PROP( "state" );
		//DBGSRV_REG_NATIVE_PROP( "normalShapeHits" );
		//DBGSRV_REG_NATIVE_PROP( "groundedTime" );
		//DBGSRV_REG_NATIVE_PROP( "collisionDown" );
		//DBGSRV_REG_NATIVE_PROP( "slidingEnabled" );
		//DBGSRV_REG_NATIVE_PROP( "slidingDirLen" );
		//DBGSRV_REG_NATIVE_PROP( "shouldMove" );
        //DBGSRV_REG_NATIVE_PROP( "separationZ" );
		//DBGSRV_REG_NATIVE_PROP( "platformDif" );
		//DBGSRV_REG_NATIVE_PROP( "platformPos" );
		//DBGSRV_REG_NATIVE_PROP( "currentMovementZ" );
        //DBGSRV_REG_NATIVE_PROP( "platformHit" );
		//DBGSRV_REG_NATIVE_PROP( "platformDisp" );
		//DBGSRV_REG_NATIVE_PROP( "CCfootPos" );
		//DBGSRV_REG_NATIVE_PROP( "havePlatform" );
		//DBGSRV_REG_NATIVE_PROP( "platformAttachGlobalZ" );
		//DBGSRV_REG_NATIVE_PROP( "dt" );
	}

	// collision type and group
	init.m_params->m_collisionType.RetrieveCollisionMasks( m_collisionType, m_collisionGroup );

    // TODO remove after all entities have switched to new interaction priority system
     if( init.m_params->m_interactionPriorityEnum == IP_NotSet )
	 {
         RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("Set proper 'InteractionPriorityEnum' for '%ls' ( levels 0-10 )! Old 'interactionPriority' is now: %f"), init.m_params->GetName().AsChar(), init.m_params->m_interactionPriority );
	 }

	 // init
	m_terrainNormalDamped.Init( m_terrainNormal, 0.1f );
	m_slidingDirDamped.Init( Vector::ZEROS, 0.2f );

	// init the description
	InitDescription( m_desc, init );
	ResetCollisionObstaclesData();
	ResetCollisionCharactersData();

	// init parameters
	m_stepOffset				= m_desc.stepOffset;
	m_simulationType			= SM_KINEMATIC;

	// init radiuses
	m_physicalRadius			= m_desc.radius;
	m_baseVirtualRadius			= ( init.m_params->m_baseVirtualCharacterRadius >= 0.f ) ? init.m_params->m_baseVirtualCharacterRadius : init.m_params->m_physicalRadius;
	m_virtualRadius				= m_baseVirtualRadius;
	m_currentVirtualRadius		= m_baseVirtualRadius;
	m_startVirtualRadius		= m_baseVirtualRadius;
	m_virtualRadiusTimer		= 0.0f;

	// init height
	m_height					= m_desc.height + 2.0f*m_desc.radius;
	m_currentHeight				= m_height;

	// init collision prediction
	m_collisionPrediction				= init.m_params->m_collisionPrediction;
	m_collisionPredictionMovementAdd	= init.m_params->m_collisionPredictionMovementAdd;
	m_collisionPredictionMovementMul	= init.m_params->m_collisionPredictionMovementMul;
	m_collisionPredictionEventName		= init.m_params->m_collisionPredictionEventName;

	SWrapperContext* position = m_world->GetWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
	if( !position )
	{
		int a = 0;
	}
	const Matrix& localToWorld = GetParentProvider()->GetLocalToWorld();
	position->m_x = localToWorld.GetTranslationRef().X;
	position->m_y = localToWorld.GetTranslationRef().Y;
	position->m_desiredDistanceSquared = 1000 * 1000;
	position->m_resultDistanceSquared = FLT_MAX;
	position->m_visibilityQueryId = 0;
}
//////////////////////////////////////////////////////////////////////////
//
// clean before destroy
Bool CPhysicsCharacterWrapper::MakeReadyToDestroy( TDynArray< void* >* toRemove )
{
	if ( m_characterController )
	{
		PxActor* actor = m_characterController->getActor();
		if( actor->getScene() )
		{
			toRemove->PushBack( actor );
			return false;
		}
	}
	return true;
}

void CPhysicsCharacterWrapper::PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd )
{
	PC_SCOPE_PHYSICS( CC_PreSimulation )

	if( m_characterController ) return;
	CComponent* component;
	if( !GetParent( component ) ) return;

	CWorld* world = component->GetWorld();
	m_characterControllersManager = world->GetCharacterControllerManager();
	CCharacterControllersManager* characterControllersManager = m_characterControllersManager.Get();
	if( !characterControllersManager ) return;
	m_characterController = ( PxCapsuleController* ) characterControllersManager->CreateController( &m_desc );
	
	ASSERT( m_characterController );

	PxRigidActor* actor = m_characterController->getActor();
	// setup the userdata
	actor->userData	= static_cast<CPhysicsWrapperInterface*>( this );

	PxShape* shape = nullptr;
	actor->getShapes( &shape, 1 );
	shape->userData = nullptr;

	// set the filter data
	shape->setSimulationFilterData( m_filterData.m_data );
	shape->setQueryFilterData( m_filterData.m_data );

#ifndef NO_EDITOR
	m_debugName = UNICODE_TO_ANSI( GetParentProvider()->GetFriendlyName().AsChar() );
	actor->setName( m_debugName.AsChar() );
#endif

	// initialize falling start position
	m_fallingStartPos = m_footPosition;

	UpdateTriggerShapes();

#ifndef RED_FINAL_BUILD
	m_cacheCreationTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
#endif

}
//////////////////////////////////////////////////////////////////////////
//
// return actor count
Uint32 CPhysicsCharacterWrapper::GetActorsCount() const
{
	return m_characterController ? 1 : 0;
}
//////////////////////////////////////////////////////////////////////////
//
//
void CPhysicsCharacterWrapper::Release( Uint32 actorIndex )
{
	RED_ASSERT( m_ref.GetValue() > 0 )
		if( !m_ref.Decrement() )
		{
			m_world->GetWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >()->PushWrapperToRemove( this );
		}
}
//////////////////////////////////////////////////////////////////////////
//
//
CPhysicsWorld* CPhysicsCharacterWrapper::GetPhysicsWorld()
{
	return m_world;
}
//////////////////////////////////////////////////////////////////////////
//
// is character ready
Bool CPhysicsCharacterWrapper::IsReady() const
{
#ifndef USE_PHYSX
	return false;
#else
	if( !m_characterController ) return false;
	return m_characterController->getScene() != 0;
#endif
}
//////////////////////////////////////////////////////////////////////////
//
// init controller
void CPhysicsCharacterWrapper::InitDescription( PxCapsuleControllerDesc& desc, const SPhysicsCharacterWrapperInit& init )
{
	if( !m_world ) return;

	CObject* object = nullptr;
	if( GetParentProvider()->GetParent( object ) )
	{
		DATA_HALT( DES_Major, CResourceObtainer::GetResource( object ), TXT("Physical Character Controller"), TXT("H < 2*R - capsule height is too low!") );
	}
	const Float radius = Clamp<Float>( init.m_params->m_physicalRadius, 0.1f, (init.m_params->m_height-desc.contactOffset)*0.5f );
	const Float height = Clamp<Float>( init.m_params->m_height - 2.0f*radius, 0.05f, 1000.f );
	desc.radius					= radius;
	desc.height					= height;

	PxExtendedVec3 pxPos		= TO_PX_EXT_VECTOR( init.m_initialPosition );
	pxPos.z						+= radius + 0.5f*height;

	// manual climbing mode for geralt
	desc.climbingMode			= PxCapsuleClimbingMode::eEASY;

	Vector gravity				= m_world->GetGravityVector();
	gravity.Normalize3();
	gravity.Negate();

	Float maxSlope( SCCTDefaults::DEFAULT_MAX_SLOPE );
	CComponent* component = nullptr;
	GetParentProvider()->GetParent( component );
	CWorld* world = component->GetEntity()->GetLayer()->GetWorld();
	CPathLibWorld* plw = world->GetPathLibWorld();
	if ( plw )
	{
		maxSlope = plw->GetGlobalSettings().GetMaxTerrainSlope();
	}

	// init desc
	desc.position				= pxPos;
	desc.stepOffset				= 0.0f;
	desc.slopeLimit				= 0.71f;
	desc.contactOffset			= SCCTDefaults::DEFAULT_CONTACT_OFFSET;
	desc.invisibleWallHeight	= 0.0f;
	desc.material				= GPhysXEngine->GetMaterial();
	desc.upDirection			= TO_PX_VECTOR( gravity );
	desc.userData				= static_cast< CPhysicsWrapperInterface* >( this );
	desc.nonWalkableMode		= PxControllerNonWalkableMode::ePREVENT_CLIMBING/*_AND_FORCE_SLIDING*/;
	desc.reportCallback			= init.m_needsHitCallback ? this : nullptr;
	desc.behaviorCallback		= init.m_needsBehaviorCallback ? this : nullptr;

	// create virtual controllers
	const Uint32 count = init.m_params->m_virtualControllers.Size();
	m_virtualControllers.Reserve( (size_t)count );
	for( Uint32 i = 0; i < count; ++i )
	{
		const SVirtualControllerParams& pa = init.m_params->m_virtualControllers[i];
		CVirtualCharacterController vc( pa.m_name, pa.m_boneName, pa.m_localOffset, pa.m_height, pa.m_radius, this );
		if ( pa.m_enabled ) vc.SetEnabled( true );
		if ( pa.m_collisions ) vc.EnableCollisions( pa.m_onCollisionEventName );
		if ( pa.m_collisionResponse ) vc.EnableCollisionResponse( true );
		if ( pa.m_localOffsetInModelSpace ) vc.SetLocalOffsetInModelSpace( true );
		if ( pa.m_collisionGrabber ) vc.SetCollisionGrabber( true, GPhysicEngine->GetCollisionTypeBit( pa.m_collisionGrabberGroupNames ) );
		if ( pa.m_additionalRaycastCheck.SquareMag3() > 0.0001f ) vc.EnableAdditionalRaycast( pa.m_additionalRaycastCheckEventName, pa.m_additionalRaycastCheck );
		m_virtualControllers.PushBack( vc );
	}
	CacheVirtualControllersBones();

	m_onTouchCallback = init.m_onTouchCallback;
	ASSERT( desc.isValid() );
}
//////////////////////////////////////////////////////////////////////////
//
// dtor
CPhysicsCharacterWrapper::~CPhysicsCharacterWrapper()
{
	if ( m_characterController )
	{
		m_characterController->getActor()->userData = nullptr;

		m_characterControllersManager.Get()->UnregisterController( this );
		m_characterController->release();
		m_characterControllersManager.Reset( nullptr );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// reset virtual radius
void CPhysicsCharacterWrapper::ResetVirtualRadius()
{
	m_virtualRadius				= m_baseVirtualRadius;
	m_currentVirtualRadius		= m_baseVirtualRadius;
	m_startVirtualRadius		= m_baseVirtualRadius;

	m_virtualRadiusTimer		= GGame->GetGameplayConfig().m_virtualRadiusTime;
}
//////////////////////////////////////////////////////////////////////////
//
// set virtual radius
void CPhysicsCharacterWrapper::SetVirtualRadius( const Float vRadius, Bool immediately )
{
	if ( immediately || GGame->GetGameplayConfig().m_virtualRadiusTime <= 0.0001f )
	{
		m_startVirtualRadius = vRadius;
		m_currentVirtualRadius = vRadius;
		m_virtualRadius = vRadius;
		m_virtualRadiusTimer = GGame->GetGameplayConfig().m_virtualRadiusTime;
		return;
	}
	
	m_startVirtualRadius = m_virtualRadius;
	m_currentVirtualRadius = m_virtualRadius;
	m_virtualRadius = vRadius;
	m_virtualRadiusTimer = 0.0f;
	m_isUpdatingVirtualRadius = true;
}
//////////////////////////////////////////////////////////////////////////
//
// update virtual radius
void CPhysicsCharacterWrapper::UpdateVirtualRadius( const Float dt )
{
	if ( m_isUpdatingVirtualRadius )
	{
		const Float virtualRadiusBlendTime = GGame->GetGameplayConfig().m_virtualRadiusTime;
		// progress timer
		m_virtualRadiusTimer += dt;
		if ( m_virtualRadiusTimer >= virtualRadiusBlendTime )
		{
			m_virtualRadiusTimer = virtualRadiusBlendTime;
			m_isUpdatingVirtualRadius = false;
		}

		// blend between m_startVirtualRadius and m_virtualRadius
		const Float alpha = m_virtualRadiusTimer/virtualRadiusBlendTime;
		m_currentVirtualRadius = m_startVirtualRadius + (m_virtualRadius-m_startVirtualRadius)*alpha;
	}

}
//////////////////////////////////////////////////////////////////////////
//
// set character height
void CPhysicsCharacterWrapper::SetHeight( const Float height )
{
	ASSERT( m_characterController );
	ASSERT( height>=m_physicalRadius*2.0f );

	// init
	const Float currentHeight = m_characterController->getHeight()+m_physicalRadius*2.0f;

	if ( m_characterController && height != currentHeight )
	{
		// init
		const Float newHeight = Clamp( height, 0.0f, 100.0f );

		// store new height
		m_currentHeight = newHeight;

		// check overlap for possible intersections when extending height
		if ( newHeight > currentHeight )
		{
			const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
				| GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) )
				| GPhysicEngine->GetCollisionTypeBit( CNAME( DeniedArea ) )
				| GPhysicEngine->GetCollisionTypeBit( CNAME( BoatSide ) )
				| GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );

			SPhysicsOverlapInfo res;
			ETraceReturnValue sweepResult = m_world->CapsuleOverlapWithAnyResult( GetCenterPosition()+Vector( 0, 0, (newHeight-currentHeight)*0.5f ), m_physicalRadius, newHeight, includeMask, 0, res );

			// waiting for main thread
			if ( sweepResult == TRV_ProcessedWhileFetch )
			{
				return;
			}

			// submerge when collider is above
			if ( sweepResult == TRV_Hit )
			{
				m_currMovementVector.Z -= (newHeight-currentHeight)*submergeMultiplierUnderCollider * m_submergeSpeed * GGame->GetTimeManager()->GetLastTickTime();
				return;
			}
		}

		// set new height
		m_characterController->setHeight( newHeight-m_physicalRadius*2.0f );

		// check new position and apply correction if needed
		// this correction is needed due to physX setHeight bug
		PxExtendedVec3 currentPos = m_characterController->getFootPosition();
		const Float prev = m_footPosition.Z;
		if ( fabsf( (Float)currentPos.z-prev ) > 0.001f )
		{
			currentPos.z += (newHeight-currentHeight)*0.5f;
			m_characterController->setFootPosition( currentPos );
			m_characterController->invalidateCache();
			m_footPosition = TO_VECTOR_FROM_EXT_VEC( m_characterController->getFootPosition() );
			//DBGSRV_SET_NATIVE_PROP_VALUE( "CCfootPos", m_footPosition );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// reset character height to original value
void CPhysicsCharacterWrapper::ResetHeight()
{
	SetHeight( m_height );
}
//////////////////////////////////////////////////////////////////////////
//
// teleport to new place
void CPhysicsCharacterWrapper::Teleport( const Vector& target, const Bool correctZ )
{
	m_moveExternalDisp = target;
	m_teleport = true;
	m_teleportCorrectZ = correctZ;
}
void CPhysicsCharacterWrapper::CorrectPosition()
{
	m_correctPosition = true;
}
//////////////////////////////////////////////////////////////////////////
//
// external move move
void CPhysicsCharacterWrapper::Move( const Vector& moveDelta )
{
	if ( m_teleport )
	{
		return;
	}

	m_moveExternalDisp += moveDelta;
}
//////////////////////////////////////////////////////////////////////////
//
// apply internal displacement
void CPhysicsCharacterWrapper::ApplyInternalMovement( const Float dt )
{
	m_internalVelocity.W = 0;
	m_moveInputDisp += m_internalVelocity * dt;
	if ( m_state == CPS_Falling )
	{
		m_internalVelocity -= m_internalVelocity.AsVector2() * Clamp( dt, 0.0f, 1.0f );
	}
	//LOG_ENGINE( TXT( "internal %0.3f;%0.3f;%0.3f" ), m_internalVelocity.X, m_internalVelocity.Y, m_internalVelocity.Z );	
}
//////////////////////////////////////////////////////////////////////////
//
// apply external displacement
void CPhysicsCharacterWrapper::ApplyExternalMovement( const Float dt )
{
	m_moveInputDisp += m_moveExternalDisp;
}
//////////////////////////////////////////////////////////////////////////
//
// apply gravity
void CPhysicsCharacterWrapper::ApplyGravity( const Float dt )
{
	if ( !m_collisionDown )
	{
		const Vector& gravity = m_world->GetGravityVector();
		m_internalVelocity += gravity * GGame->GetGameplayConfig().m_fallingMul * dt;
	}
}
//////////////////////////////////////////////////////////////////////////
//
// correct gravity before final movement
void CPhysicsCharacterWrapper::CorrectGravity( const Float dt )
{
	RED_UNUSED( dt );

	// change gravity vector
	if ( DownCollision() && (m_state == CPS_Simulated || m_hPlatform.Get()) )
	{
        //DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );
		//LOG_ENGINE( TXT( "platform move before %f - %i" ), m_currMovementVector.Z, !!m_hPlatform.Get() );
		if ( m_hPlatform.Get() )
		{
			if ( m_currMovementVector.Z > 0.00001f )
			{
				ForceSetPosition( m_footPosition + Vector( 0, 0, m_currMovementVector.Z ) );
				m_currMovementVector.Z = -0.01f;
			}
			else if ( m_currMovementVector.Z < -0.00001f )
			{
				ForceSetPosition( m_footPosition + Vector( 0, 0, m_currMovementVector.Z ) );
				m_currMovementVector.Z = -0.01f;
			}
		}
		else
			m_currMovementVector.Z -= m_currMovementVector.Mag2()*1.5f;
		//LOG_ENGINE( TXT( "platform move after %f" ), m_currMovementVector.Z );

        //DBGSRV_SET_NATIVE_PROP_VALUE( "moveZ", m_currMovementVector );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// grab platform
void CPhysicsCharacterWrapper::GrabPlatform( SPhysicsContactInfo& hit )
{
	CPhysicsWrapperInterface* wrapper = hit.m_userDataA;
	if ( wrapper == nullptr )
	{
        return;
    }

	SActorShapeIndex magicUnion = hit.m_rigidBodyIndexA;

	// Fetch CComponent from wrapper
	CComponent* platform = nullptr;
	wrapper->GetParent( platform, magicUnion.m_actorIndex );
	if ( platform == nullptr )
	{
        return;
    }

    const Bool hadPlatform = m_hPlatform.Get() != nullptr;

	m_hPlatform = platform;
	m_collisionDown = true;

	// Get platform matrix
    const EngineTransform& transform = platform->GetEntity()->GetTransform();
    Matrix globalMat;
    transform.CalcLocalToWorld( globalMat );

	// Get attach direction
#ifdef USE_PLATFORM_ROTATION
	m_platformAttachDir = globalMat.V[0].Normalized2();
#endif    

    const Float radiusOnNormal = m_physicalRadius * ( 1.0f - hit.m_normal.Z );

	// Correct Z positon
	ForceSetPosition( hit.m_position + Vector( 0, 0, radiusOnNormal ) );

	// Get attach point in platform's local space
	m_platformAttachPointLocal = globalMat.Inverted().TransformPoint( hit.m_position );
    
#ifndef NO_EDITOR
    dbg_platformAttachGlobal = hit.m_position;
#endif
	
	//DBGSRV_SET_NATIVE_PROP_VALUE( "collisionDown", true );
    //DBGSRV_SET_NATIVE_PROP_VALUE( "platformHit", hit.m_position.Z );
	//DBGSRV_SET_NATIVE_PROP_VALUE( "platformPos", globalMat.GetTranslation().Z );
    //DBGSRV_SET_NATIVE_PROP_VALUE( "platformAttachGlobalZ", hit.m_position.Z );
    //DBGSRV_SET_NATIVE_PROP_VALUE( "havePlatform", m_hPlatform.Get() ? 1.0f : 0.0f );
}

//////////////////////////////////////////////////////////////////////////

void CPhysicsCharacterWrapper::ResetPlatform()
{
    m_hPlatform = nullptr;
    m_platformAttachPointLocal = Vector::ZEROS;
    m_platformCurrentDisplacement = Vector::ZEROS;
}

//////////////////////////////////////////////////////////////////////////
//
// check is possible to move
Bool CPhysicsCharacterWrapper::CanMove( const Vector& position )
{
	const Bool physicsPresent = GetPhysicsWorld()->IsAvaible( position );
	return physicsPresent || !m_isInGame;
}
//////////////////////////////////////////////////////////////////////////
//
// should we apply movement?
Bool CPhysicsCharacterWrapper::ShouldMove()
{
	if (
		!(m_collisionDown && m_currMovementVector.Mag2() < minMoveDelta && m_currMovementVector.Z <= 0.0f) ||
		m_state == CPS_Animated ||
		m_hPlatform.Get() != nullptr ||
		m_isPlayerControlled ||
		m_isVehicle ||
		m_ocean )
	{
		//DBGSRV_SET_NATIVE_PROP_VALUE( "shouldMove", true );
		return true;
	}

	// reset movement
	m_moveExternalDisp.SetZeros();
	m_moveInputDisp.SetZeros();
	m_currMovementVector.SetZeros();

    //DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );
	//DBGSRV_SET_NATIVE_PROP_VALUE( "shouldMove", false );

	return false;
}

//////////////////////////////////////////////////////////////////////////
//
// place at new location
void CPhysicsCharacterWrapper::PlaceAt( Vector const & newLoc )
{
	ASSERT( m_characterController );
	if ( !m_characterController )
		return;

	// init
	Vector newLocation = newLoc;

	// if we need to correct Z pos
	if ( (m_teleportCorrectZ || m_correctPosition) && CanMove( newLoc ) )
	{
		Vector footPos = newLocation; footPos.Z += m_physicalRadius*2.0f;
		Vector destRay = newLocation; destRay.Z -= m_physicalRadius*1.0f;
		SPhysicsContactInfo outInfo;
		const static CPhysicsEngine::CollisionMask includeMask = CPhysicalCollision::COLLIDES_ALL;
		const static CPhysicsEngine::CollisionMask excludeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) ) 
																| GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) ) 
																| GPhysicEngine->GetCollisionTypeBit( CNAME( ClothCollider ) )
																| GPhysicEngine->GetCollisionTypeBit( CNAME( DeniedArea ) )
																| GPhysicEngine->GetCollisionTypeBit( CNAME( Dangles ) ) 
																| GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) ) 
																| GPhysicEngine->GetCollisionTypeBit( CNAME( Projectile ) ) 
																| GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) ) 
																| GPhysicEngine->GetCollisionTypeBit( CNAME( Weapon ) );
		const Bool precise = true;
		const Float testRadius = GetPhysicalRadius() * 0.75f;		// magical 0.75 multiplier...
		if ( m_world->SweepTestWithSingleResult( footPos, destRay, testRadius, includeMask, excludeMask, outInfo, precise ) == TRV_Hit)
		{
			ASSERT( outInfo.m_distance > 0.0f, TXT( "Sweep test returned distance 0! Collision occurred at start pos!" ) );
			if ( outInfo.m_distance > 0.0f )
			{
				newLocation.Z = outInfo.m_position.Z - testRadius*(1.0f-outInfo.m_normal.Z);
			}
		}
	}

	// set new pos
	ForceSetPosition( newLocation );
	m_footPosition = TO_VECTOR_FROM_EXT_VEC( m_characterController->getFootPosition() );
	m_fallingStartPos = m_footPosition;
}
//////////////////////////////////////////////////////////////////////////
//
// on teleport
Bool CPhysicsCharacterWrapper::OnTeleport( const Float dt )
{
	if ( m_teleport || m_correctPosition )
	{
		// move to new position
		PlaceAt( m_teleport ? m_moveExternalDisp : m_footPosition );
		 
		// this will update the character controller's state at new placement
		const Bool canMove = CanMove( m_moveExternalDisp );
		m_fallingStartPos = m_footPosition;
		m_moveExternalDisp.SetZeros();
		m_moveInputDisp.SetZeros();
		m_internalVelocity.SetZeros();
		if ( m_correctPosition && !m_teleport )
			m_currMovementVector.Set3( 0, 0, -dt );
		else
			m_currMovementVector.SetZeros();
        
        //DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );
		m_terrainNormal = Vector::EZ;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );

		m_terrainNormalDamped.Set( Vector::EZ );
		m_normalShapeHits = 1;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "normalShapeHits", m_normalShapeHits );
		m_shapeHit = true;
		m_collisions = 0;
		m_correctPosition = false;
		m_teleport = false;
		m_teleportCorrectZ = false;

		// we don't want to move characters in unstreamed areas
		if ( canMove )
		{
			ASSERT( TXT( "trying to teleport character controller to unstreamed area!!" ) );
			Update1_ComputeMovementVector( dt );
			Update2_ApplyMovement( dt );
		}
		FetchInternalState();

		return true;
	}

	return false;
}
//////////////////////////////////////////////////////////////////////////
//
// check swimming in global ocean
void CPhysicsCharacterWrapper::CheckSwimming()
{
	if ( GGame->GetActiveWorld()->IsWaterShaderEnabled() )
	{
		// init
		Bool leftWater = true;

		// get water level at pos
		//LOG_ENGINE( TXT("wnd scale %0.3f geralt Z %0.3f water level %0.3f - terrain %0.3f "), GGame->GetActiveWorld()->GetEnvironmentManager()->GetCurrentWindParameters().GetWindScale(), m_characterController->getPosition().z, GGame->GetActiveWorld()->GetWaterLevel( ccPos ), terrainH );
		Float waterLevel = GetWaterLevel();
		if ( waterLevel > -10000.0f+NumericLimits<Float>::Epsilon() && m_footPosition.Z < waterLevel+1.0f )
		{
			leftWater = false;

			// water get-in trigger
			if ( !m_ocean )
			{
				m_ocean = true;
				NotifyListeners( CNAME( OnOceanTriggerEnter ) );
			}
		}

		// water get out trigger - out of clipmap, out of water range or under terrain
		if ( m_ocean && leftWater && !m_isDiving )
		{
			m_ocean = false;
			NotifyListeners( CNAME( OnOceanTriggerLeave ) );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// set diving
void CPhysicsCharacterWrapper::SetDiving( const Bool diving )
{
	// call entity when we are currently in diving state
	if ( m_isDiving )
	{
		CComponent* component = nullptr;
		if( GetParent( component ) )
		{
			component->GetEntity()->CallEvent( CNAME( OnDivingEnd ) );
		}
	
	}
	// set state
	m_isDiving = diving;
}

// ============================================================================================================================
// HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK HACK
// Even if we set the PxCCTInteractionMode::eEXCLUDE flag, and return PxSceneQueryHitType::eNONE inside the
// CPhysicsCharacterCallback::preFilter(), it STILL DETECTS THE COLLISION BETWEEEN CHARACTER CONTROLLERS, and set the
// internal variable mOverlapRecover inside move().
//
// So, if you have let's say two character controllers, A and B, and you call A->move() and it collides with B, it sets the
// mOverlapRecover member of B! Then, when you call B->move() it takes the mOverlapRecover value into account to recover from
// that collision.
//
// It turned out that the only way to prevent PhysX from separating the character controllers is to reset it's internal member
// mOverlapRecover to zero berore each move() call. Since that member is internal and we have no direct access to it through
// the included interfaces (like PxCapsuleController), I needed to include some of the PhysX source files here. I hope any of
// you guys don't mind.
//
// I consider this a necessary hack, until nVidia fixes their bug.
// [Tomek Wójcik]
// ============================================================================================================================

//////////////////////////////////////////////////////////////////////////
//
// main update
void CPhysicsCharacterWrapper::Update1_ComputeMovementVector( const Float timeDelta )
{
	if ( !GGame || !GGame->GetActiveWorld() )
	{
		return;
	}

	// main thread events
	if ( m_onNoLongerInRagdoll.CompareExchange( false, true ) )
	{
		NotifyListeners( CNAME( OnNoLongerInRagdoll ) );
	}

	// when we need teleport to
	if ( OnTeleport( timeDelta ) )
	{
		return;
	}

	// we don't want to move characters in unstreamed areas
	if ( !CanMove( GetPosition() ) )
	{
		//if ( m_isPlayerControlled )
		WARN_ENGINE( TXT( "unstreamed %0.3f;%0.3f;%0.3f" ), m_footPosition.X, m_footPosition.Y, m_footPosition.Z );	

		m_moveExternalDisp.SetZeros();
		return;
	}

    PC_SCOPE_PHYSICS( CC_Update1_ComputeMovementVector );

    //DBGSRV_SET_NATIVE_PROP_VALUE( "dt", timeDelta );

	// init
	m_shapeHit = false;
	#ifndef NO_EDITOR
	m_normalRays.ClearFast();
	m_sweeps.ClearFast();
	#endif

	// compute movement
	if ( timeDelta > 0.0f )
	{
		// switch to ragdoll or from ragdoll
		CAnimatedComponent* ac = nullptr;
		if ( GetParent( ac ) && ac->IsRagdolled( true ) )
		{
			if ( m_state != CPS_Ragdoll )
			{
				m_ragdollIsAway = false;
				SwitchToRagdollState( !IsRagdollOnGround() );
			}
		}
		else if ( m_state == CPS_Ragdoll )
		{
			CheckOnSwitchingFromRagdoll();
			m_state = CPS_Simulated;
			//DBGSRV_SET_STRING_PROP_VALUE( "state", TXT( "CPS_Simulated" ) );
		}

		if ( m_state == CPS_Simulated || m_state == CPS_Swimming )
		{
			m_pushingTime += timeDelta;
			if( m_pushingTime > 1.0f )
			{
				m_pushingTime = 1.0f;
			}
		}
		else
		{
			m_pushingTime = 0.0f;
		}
		//DBGSRV_SET_NATIVE_PROP_VALUE( "groundedTime", m_groundedTime );

		switch ( m_state )
		{
			case CPS_Simulated:
				SimulatedUpdate( timeDelta );
			break;

			case CPS_Animated:
				AnimatedUpdate( timeDelta );
			break;

			case CPS_Falling:
				FallingUpdate( timeDelta );
			break;

			case CPS_Swimming:
				SwimmingUpdate( timeDelta );
			break;

			case CPS_Ragdoll:
				RagdollUpdate( timeDelta );
			break;
		}

		// check are we in ocean
		CheckSwimming();
	}

	// update virtual controllers global position
	UpdateVirtualControllers( timeDelta );

    // update virtual radius
    UpdateVirtualRadius( timeDelta );

    // get input displacement
    m_currMovementVector += m_moveInputDisp;

    //DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );

    // store orient
    if ( m_currMovementVector.SquareMag2() > minMoveDelta )
    {
        m_orient = m_currMovementVector;
        m_orient.Normalize2();
        m_orient.Z = 0.0f;
        m_orient.W = 0.0f;
    }

	// movement modifiers
	if ( m_isPlayerControlled || m_isVehicle )
	{
		ApplyTerrainInfluence( m_currMovementVector );
		ApplySliding( m_currMovementVector, timeDelta );
	}
	ApplySwimming( m_currMovementVector, timeDelta );

	// update platforms
	PlatformUpdate( timeDelta );
}
//////////////////////////////////////////////////////////////////////////
//
// prefilter - called on every collision during internal physX movement
PxQueryHitType::Enum CPhysicsCharacterWrapper::preFilter( const PxFilterData& filterData, const PxShape* shape, const PxRigidActor* actor, PxHitFlags& queryFlags ) 
{
	if ( shape )
	{
		PC_SCOPE_PHYSICS( CC_Move_preFilter )

		SPhysicalFilterData filter( shape->getSimulationFilterData() );
		if ( filter.GetFlags() & SPhysicalFilterData::EPFDF_CollisionDisabled )
		{
			return PxSceneQueryHitType::eNONE;
		}

		const CPhysicsEngine::CollisionMask shapeMask = filter.GetCollisionType();

		// init
		static const CPhysicsEngine::CollisionMask characterTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) );
		static const CPhysicsEngine::CollisionMask corpseTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Corpse ) );
		static const CPhysicsEngine::CollisionMask weaponTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Weapon ) );
		static const CPhysicsEngine::CollisionMask clothColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( ClothCollider ) );
		static const CPhysicsEngine::CollisionMask debrisColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
		static const CPhysicsEngine::CollisionMask destColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
		static const CPhysicsEngine::CollisionMask boatDestColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( BoatDestruction ) );
		static const CPhysicsEngine::CollisionMask boatTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
		static const CPhysicsEngine::CollisionMask boatSideTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( BoatSide ) );
		static const CPhysicsEngine::CollisionMask platformTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );
		static const CPhysicsEngine::CollisionMask doorTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) );
		static const CPhysicsEngine::CollisionMask rbColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
		static const CPhysicsEngine::CollisionMask softKinematicContactTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( SoftKinematicContact ) );
		static const CPhysicsEngine::CollisionMask deniedTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( DeniedArea ) );

		// check collision types

		// no collision with other characters - cc vs cc has custom solver
		// no collision with weapons and clothes
		if ( (characterTypeBit|weaponTypeBit|clothColliderTypeBit|softKinematicContactTypeBit) & shapeMask )
		{
			return PxSceneQueryHitType::eNONE;
		}

		// block on statics
		PxRigidActor* actor = shape->getActor()->isRigidActor();
		if ( actor->isRigidStatic() )
		{
			if ( m_staticCollisionsEnabled )
			{
				return PxSceneQueryHitType::eBLOCK;	
			}
			return PxSceneQueryHitType::eNONE;
		}

		// dynamics..
		PxRigidDynamic* dynamic = actor->isRigidDynamic();
		if ( dynamic )
		{
			if ( !m_dynamicCollisionsEnabled )
			{
				return PxSceneQueryHitType::eNONE;
			}

			// corpse collisions
			const Bool isCorpse( ( shapeMask & corpseTypeBit ) != 0 );
			if ( isCorpse )
			{
				if ( dynamic->userData )
				{
					CComponent* parentComponent = nullptr;
					if ( reinterpret_cast<CPhysicsWrapperInterface*>( dynamic->userData )->GetParent( parentComponent ) )
					{
						CEntity* entity = parentComponent->GetEntity();
						CPhysicsCharacterWrapper* character = static_cast<CPhysicsCharacterWrapper*>(this);
						CComponent* otherParentComponent = nullptr;
						character->GetParent( otherParentComponent );
						if ( entity != otherParentComponent->GetEntity() )
						{
							// we don't want to push disabled corpses
							CAnimatedComponent* ac = Cast<CAnimatedComponent>( parentComponent );
							if ( ac && ac->GetRagdollPhysicsWrapper() )
							{
								//const Float stopped = ac->GetRagdollPhysicsWrapper()->GetRagdollStoppedFactor();
								//if ( stopped < 1.0f-NumericLimits<Float>::Epsilon() )

								if ( !(dynamic->getActorFlags() & PxActorFlag::eDISABLE_GRAVITY) )
								{
									// notify gp to setup ragdoll pushing multiplier
									character->NotifyListeners( CNAME( OnRagdollTouch ), entity );

									PxVec3 force = TO_PX_VECTOR( character->m_lastMoveVec );
									force.z = 0.0f;

									const Float multipler = Clamp( character->GetRagdollPushMultiplier() * SPhysicsSettings::m_characterPushingMultipler * force.magnitude(), 0.0f, SPhysicsSettings::m_characterPushingMaxClamp );
									force.normalize();
									force *= multipler/* * (1.0f-stopped)*/;

									//LOG_ENGINE( TXT( "corpse push force %0.3f - %0.3f" ), character->GetRagdollPushMultiplier(), force.magnitude() );
									dynamic->addForce( force, PxForceMode::eIMPULSE, true ); 
								}
							}
						}
					}
				}

				return PxSceneQueryHitType::eNONE;
			}	

			// boat/platform/door/denied area - std approach
			if ( (platformTypeBit|boatTypeBit|boatSideTypeBit|doorTypeBit|boatDestColliderTypeBit|deniedTypeBit) & shapeMask )
			{
				return PxSceneQueryHitType::eBLOCK;
			}

			// if rigidbody+destruction and not debris
			if ( ( (rbColliderTypeBit|destColliderTypeBit) & shapeMask ) && ( ( debrisColliderTypeBit & shapeMask ) == 0 ) )
			{
				return PxSceneQueryHitType::eBLOCK;
			}

			// block on rest kinematics
			if ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
			{
				return PxSceneQueryHitType::eBLOCK;
			}

			// std dynamic
			return PxSceneQueryHitType::eNONE;
		}
	}

	// def result
	return PxSceneQueryHitType::eNONE;
}
//////////////////////////////////////////////////////////////////////////
//
// nothing on post
PxSceneQueryHitType::Enum CPhysicsCharacterWrapper::postFilter( const PxFilterData& filterData, const PxSceneQueryHit& hit )
{
	return PxSceneQueryHitType::eNONE;
}

//////////////////////////////////////////////////////////////////////////
//
// custom behavior on shape hit
PxControllerBehaviorFlags CPhysicsCharacterWrapper::getBehaviorFlags( const PxShape& shape, const PxActor& actor )
{
	
	if(!m_needsBehaviorCallback) return PxControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE;

    static const CPhysicsEngine::CollisionMask rbColliderTypeBit		= GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
    static const CPhysicsEngine::CollisionMask destColliderTypeBit		= GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
    static const CPhysicsEngine::CollisionMask debrisColliderTypeBit	= GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
	static const CPhysicsEngine::CollisionMask doorColliderTypeBit		= GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) );

	SPhysicalFilterData filter( shape.getSimulationFilterData() );
	const CPhysicsEngine::CollisionMask shapeMask = filter.GetCollisionType();

	if ( m_state != CPS_Animated )
	{
		if ( doorColliderTypeBit & shapeMask )
		{
			return PxControllerBehaviorFlag::eCCT_FORCE_UNWALKABLE;
		}
		if ( (rbColliderTypeBit|destColliderTypeBit) & shapeMask )
		{
			const PxRigidDynamic* dynamic = actor.isRigidDynamic();
			if ( dynamic )
			{
				PxBounds3 bounds = actor.getWorldBounds();
				float z = bounds.maximum.z;
				if ( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
				{
					float a = SPhysicsSettings::m_characterStepBigKinematicClamp;
					if( z > m_footPosition.Z + a )
					{
						return PxControllerBehaviorFlag::eCCT_PREVENT_CLIMB;
					}
				}
				else if ( ( debrisColliderTypeBit & shapeMask ) == 0 )
				{
					float b = SPhysicsSettings::m_characterStepBigDynamicsClamp;
					if ( z > m_footPosition.Z + b )
					{
						return PxControllerBehaviorFlag::eCCT_FORCE_UNWALKABLE;
					}
				}
			}
		}
	}

	return PxControllerBehaviorFlag::eCCT_USER_DEFINED_RIDE;
}
//////////////////////////////////////////////////////////////////////////
//
// todo
PxControllerBehaviorFlags CPhysicsCharacterWrapper::getBehaviorFlags( const PxController& controller )
{
	return PxControllerBehaviorFlag::eCCT_SLIDE;
}
//////////////////////////////////////////////////////////////////////////
//
// todo
PxControllerBehaviorFlags CPhysicsCharacterWrapper::getBehaviorFlags( const PxObstacle& obstacle )
{
	return PxControllerBehaviorFlag::eCCT_SLIDE;
}
//////////////////////////////////////////////////////////////////////////
//
// cc vs cc filtering is disabled as default
bool CPhysicsCharacterWrapper::filter( const PxController& a, const PxController& b )
{
	return false;
}
//////////////////////////////////////////////////////////////////////////
//
// apply terrain influence
void CPhysicsCharacterWrapper::ApplyTerrainInfluence( Vector& disp )
{
	// terrain curvature influence only on player movement
	if ( disp.SquareMag2() > minMoveDelta*minMoveDelta && m_state != CPS_Swimming )
	{
		if ( m_normalShapeHits && m_terrainNormalDamped.Get().Z < 0.999f )
		{
			// init
			Vector nz = m_terrainNormalDamped.Get();
			const Float nzZ = nz.Z;
			nz.Z = 0.0f;
			nz.Normalize2();
			Vector dispZ = disp;
			dispZ.Z = 0.0f;
			dispZ.Normalize2();

			// calc disp-terrain dot
			const Float dot = dispZ.Dot2( nz );

			// compute slope limit
			const Float terrainCoef = acosf( Clamp<Float>( nzZ, 0.0f, 1.0f ) )*2.0f/M_PI;
			//LOG_ENGINE( TXT( "dot %f/%f" ), dot, terrainCoef );

			// apply slope limit
			const Float tiMin = m_terrainLimitMin;
			Float alpha = 1.0f;
			if ( terrainCoef > tiMin )
			{
				Float slopeSub = 1.0f;
				const Float tiMax = m_terrainLimitMax;
				const Float terrainInfluence = Clamp( (terrainCoef - tiMin)/(tiMax - tiMin), 0.0f, 1.0f );
				slopeSub -= terrainInfluence;
				//LOG_ENGINE( TXT( "slope %f" ), slopeSub );

				if ( dot < 0.0f )
				{
					// apply slope func on displacement
					if ( slopeSub > 0.001f )
					{
						const Float slopeFunc = -(dot + 0.31f*sinf( dot*M_PI ));
						alpha = (1.0f + (slopeSub-1.0f)*slopeFunc);
						disp *= Clamp( 1.0f-(m_terrainLimitMul-m_terrainLimitMul*alpha), 0.0f, 1.0f );
					}
					else
					{
						alpha = Clamp( (dot+0.25f)/0.25f, 0.0f, 1.0f );
						disp *= Clamp( 1.0f-(m_terrainLimitMul-m_terrainLimitMul*alpha), 0.0f, 1.0f );
					}
				}
				else
				{
					alpha += (1.0f-slopeSub)*m_terrainLimitMul*dot;
					disp *= alpha;
				}

				//LOG_ENGINE( TXT( "alpha %f" ), alpha ); 
			}
			else
			{
				m_speedMul = 1.0f;
			}

			// compute speed mul
			if ( m_state == CPS_Simulated )
			{
				m_speedMul = alpha;
			}
			else
			{
				m_speedMul = 1.0f;
			}

			//LOG_ENGINE( TXT( "result speed %f" ), m_speedMul ); 
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// apply sliding
void CPhysicsCharacterWrapper::ApplySliding( Vector& disp, const Float dt )
{
	// sliding is disabled while swimming and on characters other than player
	if ( m_state == CPS_Swimming )
	{
		return;
	}

	PC_SCOPE_PHYSICS( CC_ApplySliding );

	// init
	m_slidingState = SS_None;
	ASSERT( m_slidingLimitMin < m_slidingLimitMax, TXT("Sliding limit min > max") );

	// check^apply sliding
	Vector nz = m_terrainNormalDamped.Get();
	nz.Normalize3();
	nz.W = 0.0f;

	// calc slide factor
	const Float terrainCoef = acosf( Clamp<Float>( nz.Z, 0.0f, 1.0f ) )*2.0f/M_PI;
	m_slideCoef = Clamp( (terrainCoef - m_slidingLimitMin)/(m_slidingLimitMax - m_slidingLimitMin), 0.0f, 1.0f );

	// we are on slope
	if ( nz.Z > 0.001f && nz.Z < 0.999f && terrainCoef > m_slidingLimitMin )
	{
		// compute perpendicular vector to terrain normal
		Vector perpTerrain = Vector::Cross( nz, Vector::EZ );
		perpTerrain.Normalize3();
		if ( perpTerrain.SquareMag3() > 0.001f )
		{
			// compute sliding vec
			Vector slidingVec = Vector::Cross( nz, perpTerrain );
			m_slidingDir = slidingVec * m_slideCoef * m_slidingSpeed * dt;
			m_slidingDirDamped.SetDampingFactor( GGame->GetGameplayConfig().m_slidingDamping );
			m_slidingDirDamped.Update( m_slidingDir, dt );
			//DBGSRV_SET_NATIVE_PROP_VALUE( "slidingDirLen", m_slidingDirDamped.Get().Mag3() );

			if ( m_slidingDirDamped.GetLenSqr() > CC_MIN_SLIDING_SQR )
			{
				// compute sliding direction
				{
					nz.Z = 0.0f;
					nz.Normalize2();

					// calc triple product
					Float dot = m_orient.Dot2( nz );
					Float dotZ = Vector::Cross2( m_orient, nz );

					// compute slide direction
					if ( dot > 0.71f )
					{
						m_slidingState = SS_Front;
					}
					else if ( dot < -0.71f )
					{
						m_slidingState = SS_Back;
					}
					else if ( dot > -0.71f && dot < 0.71f )
					{
						if ( dotZ < 0.0f )
						{
							m_slidingState = SS_Right;
						}
						else
						{
							m_slidingState = SS_Left;
						}
					}
				}

				// apply sliding
				//DBGSRV_SET_NATIVE_PROP_VALUE( "slidingEnabled", m_slidingEnabled );
				if ( m_slidingEnabled )  disp += m_slidingDirDamped.Get();
			}
		}
	}

	// fade out sliding
	else
	{
		m_slideCoef	= 0.0f;		
		m_slidingState = SS_None;
		m_slidingDir.SetZeros();
		m_slidingDirDamped.SetDampingFactor( GGame->GetGameplayConfig().m_slidingDamping );
		m_slidingDirDamped.Update( m_slidingDir, dt );
		if ( m_slidingEnabled && m_slidingDirDamped.GetLenSqr() > CC_MIN_SLIDING_SQR )  disp += m_slidingDirDamped.Get();
	}

	CComponent* component = nullptr;
	if( component )
	{
		component->GetEntity()->SetBehaviorVariable( CNAME( sliding ), m_slideCoef );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// apply swimming
void CPhysicsCharacterWrapper::ApplySwimming( Vector& disp, const Float dt )
{
	// compute buoyancy
	if ( m_state == CPS_Swimming )
	{
		// init
		const Float movSwimOff = GGame->GetGameplayConfig().m_movingSwimmingOffset;

		// calc dif between water surface and character foots
		const Float waterLevel = GGame->GetActiveWorld()->GetWaterLevel( m_footPosition + disp, 0 );
		const Float difZ = (waterLevel-waterLevelThreshold) - m_footPosition.Z - movSwimOff;

		// std buoyancy on surface
		if ( !m_isDiving )
		{
			Float dispSwim = 0.0f;
			if ( difZ > 0.0f )
			{
				dispSwim = m_emergeSpeed*difZ*dt;
				if ( dispSwim > difZ )
					dispSwim = difZ;
			}
			else
			{
				dispSwim = m_submergeSpeed*difZ*dt;
				if ( dispSwim < difZ )
					dispSwim = difZ;
			}

			//GAME_DBG_LOG( TXT("swimming"), TXT( "%f %f %f %f %f %f %f" ), (Double)GEngine->GetRawEngineTime(), (waterLevel-waterLevelThreshold), (Float)m_characterController->getFootPosition().z, disp.Z, difZ, m_internalVelocity.Mag3(), dispSwim );

			disp.Z += dispSwim;
		}

		// end of diving
		const Float expectedPos = m_footPosition.Z + disp.Z;
		//LOG_ENGINE( TXT( "water %0.3f, exp %0.3f, posZ %0.3f, dif %0.3f, dispZ %0.3f height %0.3f" ), m_waterLevel, expectedPos, (Float)m_characterController->getFootPosition().z, expectedPos - m_waterLevel, disp.Z, m_currentHeight );

		CComponent* component = nullptr;
		if( GetParentProvider()->GetParent( component ) )
		{
			// swim out
			if ( m_isDiving && expectedPos - waterLevel > -movSwimOff )
			{
				if ( disp.Z > 0.001f )
				{
					component->GetEntity()->CallEvent( CNAME( OnDivingEnd ) );
				}

				disp.Z = (waterLevel-movSwimOff) - expectedPos;
				//WARN_ENGINE( TXT( "diving disp div %0.3f" ), disp.Z );
			}

			// bottom touch event
			if ( m_isDiving && DownCollision() )
			{
				component->GetEntity()->CallEvent( CNAME( OnWaterBottomTouch ) );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// probe terrain around - first iteration
void CPhysicsCharacterWrapper::ProbeTerrain()
{
	PC_SCOPE_PHYSICS( CC_ProbeTerrain );

	// init
	m_collisions = 0;
	const Vector& perpOrient = Vector::Cross( m_orient, Vector::EZ );
	const Float radOff = m_physicalRadius;
	Vector footPos = m_footPosition;
	footPos.Z += 2.0f*radOff;
	Vector destRay = footPos;
	const Float probe = GGame->GetGameplayConfig().m_probeTerrainOffset;
	destRay.Z -= 2.0f*radOff + probe + (1.0f-m_terrainNormalDamped.Get().Z)*0.5f;
	SPhysicsContactInfo outInfo;
	const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) )
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) ) 
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) 
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

	// 0 - platforms get - player only
	if ( m_isPlayerControlled && m_state == CPS_Simulated )
	{
		if ( !m_hPlatform.Get() )
		{
			const static CPhysicsEngine::CollisionMask platformMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

			if ( m_world->RayCastWithSingleResult( footPos + Vector( 0, 0, m_height ), footPos + Vector( 0,0,-1.3f - m_height ), platformMask, 0, outInfo ) == TRV_Hit )
			{
				GrabPlatform( outInfo );
			}
//          else if(m_isPlayerControlled)
//          {
//              DBGSRV_SET_NATIVE_PROP_VALUE( "platformHit", 0.0f );
//          }
		}
		else
		{
			// preserve collision on platform
			m_collisionDown = true;
			//DBGSRV_SET_NATIVE_PROP_VALUE( "collisionDown", true );
		}
	}

	// 1 - terrain get
	if ( m_state != CPS_Animated )
	{
		const static CPhysicsEngine::CollisionMask terrainMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );

		destRay.Z -= additionalTerrainOverlapTestOffsetZ;
		if ( m_world->RayCastWithSingleResult( footPos, destRay, terrainMask, 0, outInfo ) == TRV_Hit )
		{
#ifndef RED_FINAL_BUILD
			if ( outInfo.m_userDataA )
			{
				CheckNormal(outInfo);
			}
#endif

			// store distortion
			if ( outInfo.m_normal.Z > 0.0f )
			{
				m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
			}
			m_normalShapeHits++;
			SETBIT( m_collisions, CS_CENTER );

			#ifndef NO_EDITOR
			m_normalRays.PushBack( outInfo.m_position );
			m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
			#endif
		}
		destRay.Z += additionalTerrainOverlapTestOffsetZ;
	}

	// 1 - ray on center
	if ( !GETBIT( m_collisions, CS_CENTER ) && (m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit ) )
	{
#ifndef RED_FINAL_BUILD
		if ( outInfo.m_userDataA )
		{
			CheckNormal(outInfo);
		}
#endif

		// store distortion
		if ( outInfo.m_normal.Z > 0.0f )
		{
			m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
		}
		m_normalShapeHits++;
		SETBIT( m_collisions, CS_CENTER );

		#ifndef NO_EDITOR
		m_normalRays.PushBack( outInfo.m_position );
		m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
		#endif
	}

	// for player and vehicle we need more accurate terrain normal
	if ( m_isPlayerControlled || m_isVehicle )
	{
		// check front
		footPos += m_orient*m_physicalRadius;
		destRay += m_orient*m_physicalRadius;
		if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
		{
#ifndef RED_FINAL_BUILD
			if ( outInfo.m_userDataA )
			{
				CheckNormal(outInfo);
			}
#endif

			// store distortion
			if ( outInfo.m_normal.Z > 0.0f )
			{
				m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
			}
			m_normalShapeHits++;
			SETBIT( m_collisions, CS_FRONT );

			#ifndef NO_EDITOR
			m_normalRays.PushBack( outInfo.m_position );
			m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
			#endif
		}

		// check back
		footPos -= m_orient*m_physicalRadius*2.0f;
		destRay -= m_orient*m_physicalRadius*2.0f;
		if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
		{
#ifndef RED_FINAL_BUILD
			if ( outInfo.m_userDataA )
			{
				CheckNormal(outInfo);
			}
#endif

			// store distortion
			if ( outInfo.m_normal.Z > 0.0f )
			{
				m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
			}
			m_normalShapeHits++;
			SETBIT( m_collisions, CS_BACK );

			#ifndef NO_EDITOR
			m_normalRays.PushBack( outInfo.m_position );
			m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
			#endif
		}

		// check right
		footPos += perpOrient*m_physicalRadius + m_orient*m_physicalRadius;
		destRay += perpOrient*m_physicalRadius + m_orient*m_physicalRadius;
		if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
		{
#ifndef RED_FINAL_BUILD
			if ( outInfo.m_userDataA )
			{
				CheckNormal(outInfo);
			}
#endif

			// store distortion
			if ( outInfo.m_normal.Z > 0.0f )
			{
				m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
			}
			m_normalShapeHits++;
			SETBIT( m_collisions, CS_RIGHT );

			#ifndef NO_EDITOR
			m_normalRays.PushBack( outInfo.m_position );
			m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
			#endif
		}

		// check left
		footPos -= perpOrient*m_physicalRadius*2.0f;
		destRay -= perpOrient*m_physicalRadius*2.0f;
		if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
		{
#ifndef RED_FINAL_BUILD
			if ( outInfo.m_userDataA )
			{
				CheckNormal(outInfo);
			}
#endif

			// store distortion
			if ( outInfo.m_normal.Z > 0.0f )
			{
				m_terrainNormal += outInfo.m_normal*0.25f;
			}
			m_normalShapeHits++;
			SETBIT( m_collisions, CS_LEFT );

			#ifndef NO_EDITOR
			m_normalRays.PushBack( outInfo.m_position );
			m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
			#endif
		}

		// test on diagonals
		{
			// front-left
			footPos += m_orient*m_physicalRadius*0.71f + perpOrient*m_physicalRadius*0.29f;
			destRay += m_orient*m_physicalRadius*0.71f + perpOrient*m_physicalRadius*0.29f;
			if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
			{
#ifndef RED_FINAL_BUILD
				if ( outInfo.m_userDataA )
				{
					CheckNormal(outInfo);
				}
#endif

				// store distortion
				if ( outInfo.m_normal.Z > 0.0f )
				{
					m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
				}
				m_normalShapeHits++;
				SETBIT( m_collisions, CS_FRONT_LEFT );

				#ifndef NO_EDITOR
				m_normalRays.PushBack( outInfo.m_position );
				m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
				#endif
			}

			// front-right
			footPos += perpOrient*m_physicalRadius*0.71f*2.0f;
			destRay += perpOrient*m_physicalRadius*0.71f*2.0f;
			if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
			{
#ifndef RED_FINAL_BUILD
				if ( outInfo.m_userDataA )
				{
					CheckNormal(outInfo);
				}
#endif

				// store distortion
				if ( outInfo.m_normal.Z > 0.0f )
				{
					m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
				}
				m_normalShapeHits++;
				SETBIT( m_collisions, CS_FRONT_RIGHT );

				#ifndef NO_EDITOR
				m_normalRays.PushBack( outInfo.m_position );
				m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
				#endif
			}

			// back-right
			footPos -= m_orient*m_physicalRadius*0.71f*2.0f;
			destRay -= m_orient*m_physicalRadius*0.71f*2.0f;
			if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
			{
#ifndef RED_FINAL_BUILD
				if ( outInfo.m_userDataA )
				{
					CheckNormal(outInfo);
				}
#endif

				// store distortion
				if ( outInfo.m_normal.Z > 0.0f )
				{
					m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
				}
				m_normalShapeHits++;
				SETBIT( m_collisions, CS_BACK_RIGHT );

				#ifndef NO_EDITOR
				m_normalRays.PushBack( outInfo.m_position );
				m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
				#endif
			}

			// back-left
			footPos -= perpOrient*m_physicalRadius*0.71f*2.0f;
			destRay -= perpOrient*m_physicalRadius*0.71f*2.0f;
			if ( m_world->RayCastWithSingleResult( footPos, destRay, includeMask, 0, outInfo ) == TRV_Hit )
			{
#ifndef RED_FINAL_BUILD
				if ( outInfo.m_userDataA )
				{
					CheckNormal(outInfo);
				}
#endif

				// store distortion
				if ( outInfo.m_normal.Z > 0.0f )
				{
					m_terrainNormal += outInfo.m_normal*terrainNormalAddMul;
				}
				m_normalShapeHits++;
				SETBIT( m_collisions, CS_BACK_LEFT );

				#ifndef NO_EDITOR
				m_normalRays.PushBack( outInfo.m_position );
				m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal*0.5f );
				#endif
			}
		}
	}
	m_terrainNormal.W = 0.0f;

	//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
	//DBGSRV_SET_NATIVE_PROP_VALUE( "normalShapeHits", m_normalShapeHits );
}

//////////////////////////////////////////////////////////////////////////

void CPhysicsCharacterWrapper::ProbeTerrainWide( Vector& normalAverage, Vector& normalGlobal, Vector& direction, Float separationH, Float separationF, Float separationB )
{
	if( !m_characterController ) return;

	// Ray cast data
	Vector lastHitPosition;
	Vector orient;
	Vector orientBack;
	Vector perpOrient;
	Vector terrainNormal;
	Vector basePos = m_footPosition;
	Vector baseRay = basePos;
	Vector curPos;
	Vector curRay;
	Vector terrainSlope1;
	Vector terrainSlope2;
	Vector offset;

	SPhysicsContactInfo	outInfo;
	SPhysicsOverlapInfo overlapInfo;
	const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) )
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) ) 
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) 
														   | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

	normalAverage	= Vector::ZEROS;
	normalGlobal	= Vector::ZEROS;

	// Use the narrow normal to modify the orient
	/*terrainNormal	= m_terrainNormal;
	if( terrainNormal.Z > 0.1f && terrainNormal.Z < 0.9f )
	{
		terrainNormal.Normalize3();
		orient		= Vector::Cross( terrainNormal, Vector::EZ );
		orient		= Vector::Cross( orient, terrainNormal );
		perpOrient	= Vector::Cross( orient, Vector::EZ );
	}
	else
	{
		orient		= Vector::EY;
		perpOrient	= Vector::EX;
	}*/
	orient		= direction;
	orientBack	= direction;
	perpOrient	= Vector::Cross( orient, Vector::EZ );

	orient		*= separationF;
	orientBack	*= separationB;
	perpOrient	*= separationH;

	basePos.Z	+= 2.0f * separationF;
	baseRay.Z	-= 3.0f * separationF;

	// check front left
	offset	=  orient  - perpOrient;
	curPos = basePos + offset;
	curRay = baseRay + offset;
	AddTerrainNormalIfNeeded( curPos, curRay, includeMask, 0, outInfo, normalAverage );
	lastHitPosition	= outInfo.m_position;

	// check front right
	offset	= orient + perpOrient;
	curPos = basePos + offset;
	curRay = baseRay + offset;
	AddTerrainNormalIfNeeded( curPos, curRay, includeMask, 0, outInfo, normalAverage );
	terrainSlope1	= lastHitPosition - outInfo.m_position;

	// check back left
	offset	= - orientBack - perpOrient;
	curPos = basePos + offset;
	curRay = baseRay + offset;
	AddTerrainNormalIfNeeded( curPos, curRay, includeMask, 0, outInfo, normalAverage );

	// check back right
	offset	= - orientBack + perpOrient;
	curPos = basePos + offset;
	curRay = baseRay + offset;
	AddTerrainNormalIfNeeded( curPos, curRay, includeMask, 0, outInfo, normalAverage );
	terrainSlope2	= lastHitPosition - outInfo.m_position;

	// Get the normal global
	normalGlobal	= Vector::Cross( terrainSlope2, terrainSlope1 );
	normalGlobal.Normalize3();

	// Polish the normal average
	normalAverage.W = 0.0f;
	normalAverage.Normalize3();
}

bool CPhysicsCharacterWrapper::AddTerrainNormalIfNeeded( const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outInfo,  Vector& resultingNormal )
{
	if ( m_world->RayCastWithSingleResult( from, to, include, exclude, outInfo ) == TRV_Hit )
	{
		CComponent* component = nullptr;
		outInfo.m_userDataA->GetParent( component, outInfo.m_rigidBodyIndexA.m_actorIndex );
		ASSERT( outInfo.m_normal.Z > 0.0f, TXT("Possible inverted normals on triangle mesh!!! [%s]"), component->GetParent()->GetFriendlyName().AsChar() );
		if ( outInfo.m_normal.Z > 0.0f )
		{
			resultingNormal += outInfo.m_normal;

			return true;
		}
	}

	// If no collision out position is set to the destination point
	outInfo.m_position	= to;

	return false;
}

//////////////////////////////////////////////////////////////////////////
//
// apply displacement vector on controller
void CPhysicsCharacterWrapper::Update2_ApplyMovement( const Float timeDelta )
{
	PC_SCOPE_PHYSICS( CC_Update2_ApplyMovement_All );

	// prediction
	if ( m_collisionPrediction )
	{
		CheckCollisionPrediction();
	}

	// virtual controller collision response
	VirtualControllersCollisionResponse( timeDelta );
	
    // final movement
	const Float minMovSqr = minMoveDelta*minMoveDelta;
	Bool movDone = false;
    if ( m_state == CPS_Swimming || m_currMovementVector.SquareMag3() > minMovSqr )
    {
        PC_SCOPE_PHYSICS( CC_Update2_ApplyMovement );
		
		// still standing on ground and don't move horizontally
		{
			// set height on main thread
			SetHeight( m_currentHeight );

			// correct gravity
			CorrectGravity( timeDelta );

			// reset platform data - need to check are we moving because of ShouldMove() function
            // This is done after platform update, m_currMovementVector already contains this frame platform displacement, remove it to make proper calculations
			if ( m_isPlayerControlled && ( ( m_currMovementVector - m_platformCurrentDisplacement ).SquareMag2() > SCCTDefaults::PLATTFORM_DETACH_THRESHOLD*SCCTDefaults::PLATTFORM_DETACH_THRESHOLD || m_state != CPS_Simulated) )
			{
                ResetPlatform();

                //DBGSRV_SET_NATIVE_PROP_VALUE( "havePlatform", false );
                //DBGSRV_SET_NATIVE_PROP_VALUE( "platformAttachGlobalZ", 0.0f );
            }

            //DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );

			PxVec3 thisIterDispl = TO_PX_VECTOR( m_currMovementVector );
			{
				physx::Cct::CapsuleController *cct = static_cast<physx::Cct::CapsuleController*>( m_characterController );
				cct->mOverlapRecover = PxVec3( 0.0f );

				static const CPhysicsEngine::CollisionMask rbColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
				static const CPhysicsEngine::CollisionMask destColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
				static const CPhysicsEngine::CollisionMask boatDestColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( BoatDestruction ) );
				static const CPhysicsEngine::CollisionMask ragdollTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) );

				SPhysicalFilterData data = *GetFilterData();
				( ( Uint64& )data.m_data.word2) |= rbColliderTypeBit|destColliderTypeBit|boatDestColliderTypeBit;
				( ( Uint64& )data.m_data.word2) &= ~ragdollTypeBit;

				// swap type and group for correct filtering
				Uint32 w2 = data.m_data.word2;
				data.m_data.word2 = data.m_data.word0;
				data.m_data.word0 = w2;

				{
					//if ( m_isPlayerControlled )
					//	LOG_ENGINE( TXT( "----------- move %0.3f;%0.3f;%0.3f - len %0.3f tz %f dt %f - %i" ), m_currMovementVector.X, m_currMovementVector.Y, m_currMovementVector.Z, m_currMovementVector.Mag3(), m_terrainNormal.Z, timeDelta, m_isPlayerControlled );
					PC_SCOPE_PHYSICS( CC_Update2_ApplyMovement_physX_move );

					// stairs handler - we are using ease constraint mode so need to handle stairs manually
					{
						Vector tn = m_terrainNormal;
						tn.Normalize2();
						Vector mov = m_currMovementVector;
						mov.Normalize2();

						//if ( m_isPlayerControlled )
							//LOG_ENGINE( TXT( "stairs dot %f/%f" ), m_terrainNormal.Dot2( m_currMovementVector ), tn.Dot2( mov ) );
						
						// stairs compensation
						if ( m_isPlayerControlled && m_terrainNormal.Z < 0.87f && m_terrainNormal.Z > 0.5f && tn.Dot2( mov ) < 0.0f )
						{
							//LOG_ENGINE( TXT( "stairs!!! %f %f dot %f" ), m_terrainNormalDamped.Get().Z, m_terrainNormal.Z, m_terrainNormal.Dot2( m_currMovementVector ) );
							thisIterDispl.x /= m_terrainNormal.Z*0.8f;
							thisIterDispl.y /= m_terrainNormal.Z*0.8f;

							m_currMovementVector.X /= m_terrainNormal.Z*0.8f;
							m_currMovementVector.Y /= m_terrainNormal.Z*0.8f;
						}
					}

					// reset normal data
					m_terrainNormal.Set3( 0, 0, 0.01f );
					m_normalShapeHits = 0;
					//DBGSRV_SET_NATIVE_PROP_VALUE( "normalShapeHits", m_normalShapeHits );

					// before correction
					DoCorrectionBeforeMove( thisIterDispl );

					// physical move
					Vector footPrev = m_footPosition;
					PxControllerCollisionFlags controllerCollisionFlags = m_characterController->move( thisIterDispl, minMoveDelta, timeDelta, PxControllerFilters( &data.m_data, this, this ) );
					m_footPosition = TO_VECTOR_FROM_EXT_VEC( m_characterController->getFootPosition() );                    
                    //DBGSRV_SET_NATIVE_PROP_VALUE( "CCfootPos", m_footPosition.Z );
					movDone = true;

					Bool colSides = controllerCollisionFlags & PxControllerCollisionFlag::eCOLLISION_SIDES;

					// stuck test - temp solution
					if ( m_isPlayerControlled && (m_footPosition-footPrev).Mag2() < 0.00001f && thisIterDispl.x*thisIterDispl.x > 0.001f 
						&& thisIterDispl.y*thisIterDispl.y > 0.001f && !m_collisionSides && m_collisionDown && !colSides)
					{
						thisIterDispl.z = 0.0f;
						PlaceAt( m_footPosition + TO_VECTOR( thisIterDispl ) );
					}

					// after correction
					DoCorrectionAfterMove( thisIterDispl );

					// update position in pool
					SWrapperContext* position = m_world->GetWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >()->GetContextAt( m_poolIndex );
					position->m_x = m_footPosition.X;
					position->m_y = m_footPosition.Y;

					m_currMovementVector = m_footPosition - footPrev;
				}
			}
		}
    }
	else
	{
		m_currMovementVector.SetZeros();
	}

    // store current displacement
    m_lastMoveVec = m_currMovementVector;

    // get current internal state of character
	if ( movDone )
	{
		FetchInternalState();
	}

    // are we in the air?
    if ( !DownCollision() )
    {
        m_fallingTime += timeDelta;
        if ( m_state == CPS_Simulated && m_fallingTime > GGame->GetGameplayConfig().m_fallingTime )
        {
            NotifyListeners( CNAME( OnInAirStarted ) );
			m_fallingStartPos = m_footPosition;
        }
    }
    else
    {
        m_internalVelocity.SetZeros();
    }

	// handle ragdoll
    if ( m_state == CPS_Ragdoll )
    {
        // update ragdoll state
        SwitchToRagdollState( !IsRagdollOnGround() );
    }
    else
    {
        // on down collision
        if ( DownCollision() )
        {
            if ( m_internalVelocity.Z < 0.0f )
            {
                if ( m_state != CPS_Simulated )
                {
                    OnLanded( m_internalVelocity.Z );
                }
                m_internalVelocity.Z = 0.0f;
            }
        }

        // on up collision - iteration 2
        if ( UpCollision() )
        {
            if ( m_internalVelocity.Z > 0.0f )
            {
                m_internalVelocity.Z = 0.0f;
            }
        }
    }

	// terrain normal
	//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZRaw", m_terrainNormal.Z );
	m_terrainNormal.Normalize3();
	//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
	if ( m_terrainNormal.SquareMag3() < 0.5f )
		m_terrainNormalDamped.Set( m_terrainNormal );
	m_terrainNormalDamped.Update( m_terrainNormal, timeDelta );

    // reset
    m_moveExternalDisp.SetZeros();
    m_moveInputDisp.SetZeros();
    m_currMovementVector.SetZeros();
	//DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );

	// at the end, pull ragdoll back, just in case we couldn't match up with it
	if ( m_state == CPS_Ragdoll )
	{
		PullRagdollBackIfTooFarAway();
	}

	// update virtual controllers - raycast collisions
	if( !m_virtualControllers.Empty() )
	{
		const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

		const Uint32 count = m_virtualControllers.Size();
		for( Uint32 i = 0; i < count; ++i )
		{
			if ( m_virtualControllers[i].IsEnabled() &&
				!m_virtualControllers[i].m_additionalRaycastCheckEventName.Empty() &&
				m_virtualControllers[i].m_additionalRaycastCheck.SquareMag3() > 0.0001f )
			{
				SPhysicsContactInfo outInfo;

				const Vector from = m_virtualControllers[i].GetCenterPosition();
				const Vector to = from+m_virtualControllers[i].m_additionalRaycastCheck;
				if ( m_world->RayCastWithSingleResult( from, to, includeMask, 0, outInfo ) == TRV_Hit )
				{
					CComponent* component = nullptr;
					if( GetParentProvider()->GetParent( component ) )
					{
						// notify entity
						component->GetEntity()->CallEvent( m_virtualControllers[i].m_additionalRaycastCheckEventName, outInfo.m_position, outInfo.m_normal );
					}

					// notify listeners
					const Uint32 listenersCount = m_eventListeners.Size();
					for( Uint32 j = 0; j < listenersCount; ++j )
					{
						m_eventListeners[j].Get()->CallEvent( m_virtualControllers[i].m_additionalRaycastCheckEventName, outInfo.m_position, outInfo.m_normal );
					}

					#ifndef NO_EDITOR
					m_normalRays.PushBack( outInfo.m_position );
					m_normalRays.PushBack( outInfo.m_position + outInfo.m_normal );
					#endif
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// correction before move
void CPhysicsCharacterWrapper::DoCorrectionBeforeMove( physx::PxVec3& disp )
{
	// jit protector - check direction of current and previous displacement
	if ( m_lastMoveVec.Mag2() > 0.001f && m_currMovementVector.Mag2() > 0.001f )
	{
		if ( m_currMovementVector.Dot2( m_lastMoveVec ) < 0.0f )
		{
			disp.x *= 0.0f;
			disp.y *= 0.0f;

			//if ( m_isPlayerControlled )
			//	ERR_ENGINE( TXT( "move stopped %f %f %f" ), m_lastMoveVec.Mag2(), m_currMovementVector.Mag2(), m_currMovementVector.Dot2( m_lastMoveVec ) );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// correction after move
void CPhysicsCharacterWrapper::DoCorrectionAfterMove( const physx::PxVec3& disp )
{
	// z correction - player only
	if ( m_isPlayerControlled && m_state == CPS_Simulated && !m_ocean )
	{
		static const CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) );

		SPhysicsContactInfo outInfo2;
		if ( m_world->CapsuleSweepTestWithSingleResult( m_footPosition+Vector(0,0,m_currentHeight*0.5f + m_physicalRadius*0.5f), m_footPosition, m_physicalRadius, m_currentHeight - m_physicalRadius*0.5f, includeMask, 0, outInfo2, true ) == TRV_Hit )
		{
			ASSERT( outInfo2.m_distance > 0.0f, TXT( "Sweep test returned distance 0! Collision occurred at start pos!" ) );
			if ( outInfo2.m_distance > 0.0f && outInfo2.m_normal.Z > 0.6f && m_collisionDown && disp.z <= 0.0f )
			{
				#ifndef NO_EDITOR
				dbg_correctionPos = outInfo2.m_position;
				#endif

				//if ( m_isPlayerControlled )
				//	ERR_ENGINE( TXT( "found f %f z %f n %f res %f coll %i disp %f dist %f state %i" ), m_footPosition.Z, outInfo2.m_position.Z, outInfo2.m_normal.Z, outInfo2.m_position.Z - m_physicalRadius*(1.0f-outInfo2.m_normal.Z), !!m_collisionDown, disp.z, outInfo2.m_distance, m_state );

				const Float newZ = outInfo2.m_position.Z - m_physicalRadius*(1.0f-outInfo2.m_normal.Z);
				m_footPosition.Z += (newZ-m_footPosition.Z)*correctZDamping;
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// moving on ground
void CPhysicsCharacterWrapper::SimulatedUpdate( const Float timeDelta )
{
	if ( m_gravity ) 
	{
		ApplyGravity( timeDelta );
	}

	ApplyInternalMovement( timeDelta );
	ApplyExternalMovement( timeDelta );
}
//////////////////////////////////////////////////////////////////////////
//
// falling movement
void CPhysicsCharacterWrapper::FallingUpdate( const Float timeDelta )
{
	ApplyGravity( timeDelta );
	ApplyInternalMovement( timeDelta );

	// landed
	if ( DownCollision() )
	{
		// on hit ground
		NotifyListeners( CNAME( OnHitGround ) );
		m_state = CPS_Simulated;
		//DBGSRV_SET_STRING_PROP_VALUE( "state", TXT( "CPS_Simulated" ) );
		m_internalVelocity.SetZeros();

		// calc falling dist
		const Float fallingDist = (m_footPosition - m_fallingStartPos).Mag3();
		const Float heightDiff = m_footPosition.Z - m_fallingStartPos.Z;

		CComponent* component = nullptr;
		if( GetParentProvider()->GetParent( component ) )
		{
			THandle< CComponent > activator = THandle< CComponent >( component );
			component->GetEntity()->CallEvent( CNAME( OnDamageFromFalling ), activator, fallingDist, heightDiff );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// swimming movement
void CPhysicsCharacterWrapper::SwimmingUpdate( const Float timeDelta )
{
	m_terrainNormal = Vector::EZ;
	//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
	m_terrainNormalDamped.Set( Vector::EZ );

	ApplyInternalMovement( timeDelta );
	ApplyExternalMovement( timeDelta );
	
	if ( m_internalVelocity.SquareMag3() > 0.001f )
	{
		m_internalVelocity -= m_internalVelocity * Clamp( timeDelta*m_submergeSpeed, 0.0f, 1.0f );
	}
	else
	{
		m_internalVelocity.SetZeros();
	}

	if( UpCollision() )
	{
		NotifyListeners( CNAME( OnHitCeiling ) );
	}

	if( m_moveInputDisp.Z < 0.f && DownCollision() )
	{
		NotifyListeners( CNAME( OnHitGround ) );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// externally animated movement
void CPhysicsCharacterWrapper::AnimatedUpdate( const Float timeDelta )
{
	m_terrainNormal = Vector::EZ;
	//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
	m_terrainNormalDamped.Set( Vector::EZ );

	ApplyExternalMovement( timeDelta );					// ???

	if ( UpCollision() )
	{
		NotifyListeners( CNAME( OnHitCeiling ) );
	}

	if ( m_moveInputDisp.Z < 0.0f && DownCollision() )
	{
		NotifyListeners( CNAME( OnHitGround ) );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// ragdoll update
void CPhysicsCharacterWrapper::RagdollUpdate( const Float timeDelta )
{
	// init
	m_terrainNormal = Vector::EZ;
	//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
	m_terrainNormalDamped.Set( Vector::EZ );
	m_internalVelocity = Vector::ZEROS;

	// move cc when we need that
	if ( m_state == CPS_Ragdoll && !m_ragdollInAir )
	{
		ApplyExternalMovement( timeDelta );
	}

	CAnimatedComponent* ac = nullptr;
	if( GetParent( ac ) && ac->GetRagdollPhysicsWrapper() )
	{
		Vector currentRagdollPosition;
		Vector deltaRagdollPosition;
		if ( ac->GetRagdollPhysicsWrapper()->GetCurrentPositionAndDeltaPosition( currentRagdollPosition, deltaRagdollPosition ) )
		{
			// try to go with capsule to ragdoll position
			Vector forwardEntityPosition = ac->GetEntity()->GetWorldPosition();
			m_moveInputDisp = currentRagdollPosition - forwardEntityPosition;
			// check predicted position of ragdoll (getting delta of last frame)
			const Float diffZ = m_moveInputDisp.Z + deltaRagdollPosition.Z + 0.5f * timeDelta; // by default try to move capsule down (that's why there's plus), so it tries to stay on the ground
			// don't move capsule vertically for no reason
			m_moveInputDisp.Z = 0.0f;
			// but try to keep capsule position close to ragdoll - just in case it would go away too far
			const Float maxHeight = m_currentHeight * 0.5f; // TODO this value should be fine, it should allow ragdoll go a little bit away from ground but should keep character location close to ground
			if ( diffZ < 0.0f )
			{
				m_moveInputDisp.Z = diffZ;
			}
			else if ( diffZ > maxHeight )
			{
				m_moveInputDisp.Z = diffZ - maxHeight;
			}

			// todo: additional multiplier from scripts
			//m_moveInputDisp *= m_ragdollPushingMultiplier;
		} 

		// notify ragdoll state
		NotifyListeners( CNAME( OnRagdollUpdate ), /*(depth <= 0.0f) ? 1.0f :*/ ac->GetRagdollPhysicsWrapper()->GetRagdollStoppedFactor() );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// platform update
void CPhysicsCharacterWrapper::PlatformUpdate( const Float dt )
{
    m_platformCurrentDisplacement.SetZeros();

	// Movement on platforms
	if ( CComponent* component = Cast< CComponent >( m_hPlatform.Get() ) )
	{
        const EngineTransform& transform = component->GetEntity()->GetTransform();
        Matrix globalMat;
        transform.CalcLocalToWorld( globalMat );
		//DBGSRV_SET_NATIVE_PROP_VALUE( "platformPos", globalMat.GetTranslation().Z );

		// Compare rotations of local vectors
		const Vector currentDir = globalMat.V[0].Normalized2();

#ifdef USE_PLATFORM_ROTATION
        // For DOT == 1 or -1 asin is QNAN!
        m_platformRotInfluenceRad = m_platformAttachDir.Dot2( currentDir );
        if( m_platformRotInfluenceRad + 0.001f > 1.0f || m_platformRotInfluenceRad - 0.001f < -1.0f )
        {
            m_platformRotInfluenceRad = 0.0f;
        }
        else
        {
		    m_platformRotInfluenceRad = -acosf( m_platformRotInfluenceRad );
        }

        // Set current attach dir
        m_platformAttachDir = currentDir;
#endif

		// Transform attach point to global space
		const Vector lastAttachGlobal = globalMat.TransformPoint( m_platformAttachPointLocal );
                
		// Compare current and previous position and move character with platform
		m_platformCurrentDisplacement = lastAttachGlobal - m_footPosition;

		// Limit max displacement
		const Float maxDisp = GGame->GetGameplayConfig().m_maxPlatformDisplacement;
		if ( m_platformCurrentDisplacement.SquareMag3() > maxDisp*maxDisp )
		{
			m_platformCurrentDisplacement.Normalize3();
			m_platformCurrentDisplacement *= GGame->GetGameplayConfig().m_maxPlatformDisplacement;
			//LOG_ENGINE( TXT( "platform max disp" ) );
		}

		m_currMovementVector += m_platformCurrentDisplacement;

        //DBGSRV_SET_NATIVE_PROP_VALUE( "platformDisp", m_platformCurrentDisplacement.Z );
        //DBGSRV_SET_NATIVE_PROP_VALUE( "platformAttachGlobalZ", lastAttachGlobal.Z );
        //DBGSRV_SET_NATIVE_PROP_VALUE( "currentMovementZ", m_currMovementVector.Z );
		//LOG_ENGINE( TXT( "platform %f %f %f - %f" ), m_platformCurrentDisplacement.X, m_platformCurrentDisplacement.Y, m_platformCurrentDisplacement.Z,  GGame->GetGameplayConfig().m_maxPlatformDisplacement );
        
#ifndef NO_EDITOR
        dbg_platformAttachGlobal = lastAttachGlobal;
#endif
	}
	// Reset rotation affection
	else
	{
#ifdef USE_PLATFORM_ROTATION
		m_platformRotInfluenceRad = 0.0f;
#endif
        //DBGSRV_SET_NATIVE_PROP_VALUE( "platformDisp", m_platformCurrentDisplacement.Z );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// switch to ragdoll
void CPhysicsCharacterWrapper::SwitchToRagdollState( const Bool ragdollInAir )
{
	if ( m_state == CPS_Ragdoll )
	{
		return;
	}

	// notify ragdoll started
	if ( m_state != CPS_Ragdoll )
	{
		NotifyListeners( CNAME( OnRagdollStart ) );
	}

	m_state = CPS_Ragdoll;
	//DBGSRV_SET_STRING_PROP_VALUE( "state", ToString( newRagdollState ) );

	NotifyListeners( ragdollInAir ? CNAME( OnRagdollInAir ) : CNAME( OnRagdollOnGround ) );
}
//////////////////////////////////////////////////////////////////////////
//
// check is ragdoll on ground
const Bool CPhysicsCharacterWrapper::IsRagdollOnGround()
{
	CAnimatedComponent* ac = nullptr;
	if( GetParent( ac ) && ac->GetRagdollPhysicsWrapper() )
	{
		Vector currentRagdollPosition;
		Vector deltaRagdollPosition;
		if ( ac->GetRagdollPhysicsWrapper()->GetCurrentPositionAndDeltaPosition( currentRagdollPosition, deltaRagdollPosition ) )
		{
			const Vector currentEntityPosition = ac->GetEntity()->GetPosition();
			// entity's location is at the bottom of capsule, check if ragdoll position is close to the bottom
			if ( currentRagdollPosition.Z < currentEntityPosition.Z + m_currentHeight * ( m_ragdollInAir ? 0.3f : 0.4f ) )
			{
				SPhysicsContactInfo outInfo;
				const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) )
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) ) 
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) 
					| GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

				// DownCollision() sometimes returns false, even if capsule seems to be touching ground - this happens on slopes
				// yes, we should check, why this is happening, why capsule is not pulled to ground, but deadlines whisper to me that maybe single raycast won't be such big problem (especially that it is last resort check)
				// dUd - todo - this need to be checked - ground touching was changed for e3 stage
				if ( DownCollision() || (m_world->RayCastWithSingleResult( currentRagdollPosition, currentRagdollPosition - Vector::EZ * m_physicalRadius * 1.5f, includeMask, 0, outInfo ) == TRV_Hit ) )
				{
					return true;
				}
			}
		}
	}
	return false;
}
//////////////////////////////////////////////////////////////////////////
//
//
void CPhysicsCharacterWrapper::PullRagdollBackIfTooFarAway()
{
	CAnimatedComponent* ac = nullptr;
	if ( GetParent( ac ) && ac->GetRagdollPhysicsWrapper() )
	{
		CPhysicsRagdollWrapper* wrapper = ac->GetRagdollPhysicsWrapper();

		Vector currentRagdollPosition;
		Vector deltaRagdollPosition;
		if ( wrapper->GetCurrentPositionAndDeltaPosition( currentRagdollPosition, deltaRagdollPosition ) )
		{
			// try to go with capsule to ragdoll position
			const Vector currentEntityPosition = ac->GetEntity()->GetWorldPosition();
			Vector ragdollDelta = currentRagdollPosition - currentEntityPosition;
			ragdollDelta.Z -= m_currentHeight * 0.5f;
			const Float maxDist = Max(m_currentHeight * 0.5f, m_physicalRadius) * 1.5f;
			const Float curDist = ragdollDelta.Mag3();
			if ( curDist > maxDist )
			{
				if ( ac->IsRagdollStickedToCapsule() )
				{
					const Uint32 count = wrapper->GetActorsCount();
					const Vector velocity( ( -ragdollDelta * ( Max( 0.3f, curDist - maxDist ) / curDist ) ) * 8.0f );
					for( Uint32 i = 0; i != count; ++i )
					{
						wrapper->SetVelocityLinear( velocity, i );
					}
				}
				else
				{
					if ( ! m_ragdollIsAway )
					{
						CComponent* component = nullptr;
						if( GetParentProvider()->GetParent( component ) )
						{
							component->GetEntity()->CallEvent( CNAME( OnRagdollIsAwayFromCapsule ), currentRagdollPosition, currentEntityPosition );
						}
						m_ragdollIsAway = true;
					}
				}
			}
			else
			{
				if ( m_ragdollIsAway )
				{
					CComponent* component = nullptr;
					if( GetParentProvider()->GetParent( component ) )
					{
						component->GetEntity()->CallEvent( CNAME( OnRagdollCloseToCapsule ), currentRagdollPosition, currentEntityPosition );
					}
					m_ragdollIsAway = false;
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// switch from ragdoll to normal state
void CPhysicsCharacterWrapper::CheckOnSwitchingFromRagdoll()
{
	if ( m_state == CPS_Ragdoll )
	{
		CAnimatedComponent* ac = nullptr;
		if ( GetParent( ac ) )
		{
			CPhysicsRagdollWrapper* ragdollWrapper = ac->GetRagdollPhysicsWrapper();
			if ( !ragdollWrapper || !ragdollWrapper->IsReady() )
			{
				return;
			}

			Vector currentRagdollPosition;
			Vector deltaRagdollPosition;
			if ( ragdollWrapper->GetCurrentPositionAndDeltaPosition( currentRagdollPosition, deltaRagdollPosition ) )
			{
				PlaceAt( currentRagdollPosition );
			}

			// store event
			m_onNoLongerInRagdoll.SetValue( true );
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// return ragdoll data
Bool CPhysicsCharacterWrapper::GetRagdollData( Vector& position, Vector& delta )
{
	CAnimatedComponent* ac = nullptr;
	if ( GetParent( ac ) && ac->GetRagdollPhysicsWrapper() )
	{
		CPhysicsRagdollWrapper* wrapper = ac->GetRagdollPhysicsWrapper();

		if ( wrapper && wrapper->IsReady() && wrapper->GetCurrentPositionAndDeltaPosition( position, delta ) )
		{
			return true;
		}
	}

	// default res
	return false;
}
//////////////////////////////////////////////////////////////////////////
//
// enable custom movement
void CPhysicsCharacterWrapper::Enable( Bool enable )
{
	if ( m_enabled == enable )
	{
		return;
	}
	
    ResetPlatform();

	m_enabled = enable;
}
//////////////////////////////////////////////////////////////////////////
//
// set as animated externally
void CPhysicsCharacterWrapper::SetAnimated( const Bool enable )
{
	CheckOnSwitchingFromRagdoll();
	if ( enable )
	{
		m_state = CPS_Animated;
		//DBGSRV_SET_STRING_PROP_VALUE( "state", TXT( "CPS_Animated" ) );
	}
	else
	{
		m_state = CPS_Simulated;
		//DBGSRV_SET_STRING_PROP_VALUE( "state", TXT( "CPS_Simulated" ) );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// capsule sweep test
Bool CPhysicsCharacterWrapper::DoCapsuleSweepTest( const Vector& from, const Vector& disp, const Float h, const Float r, CName& eventName, Bool virtualController, SPhysicsContactInfo& result, const Uint64 includeGroups )
{
	if ( !GetParentProvider()->HasParent() )
	{
		return false;
	}

	PC_SCOPE_PHYSICS( CC_VC_CollisionResponseCapsuleSweepTest );

	// init
	const Vector to = from + disp;
	Box actorVolume;
	result.m_actorVolume = &actorVolume;

	float radius = r * 2 < h ? r : ( h * 0.5f - 0.001f ); 
	if ( m_world->CapsuleSweepTestWithSingleResult( from, to, radius, h, includeGroups, 0, result, true ) == TRV_Hit )
	{
		// calc penetration
		const Float penetration = disp.Mag3() * result.m_distance;

		// calc object height and diff between
		const Float actorHeight = result.m_actorVolume->CalcSize().Z;
		const Float diffZ = result.m_actorVolume->Max.Z-GetPosition().Z;

		// notify entity
		if ( !eventName.Empty() )
		{
			{
				PC_SCOPE_PHYSICS( CC_VC_CollisionResponseCapsuleSweepTestEvent1 );
				CComponent* component = nullptr;
				if( GetParentProvider()->GetParent( component ) )
				{
					component->GetEntity()->CallEvent( eventName, result.m_position, result.m_normal, disp, penetration, actorHeight, diffZ, false );
				}
			}
		
			// notify listeners
			const Uint32 listenersCount = m_eventListeners.Size();
			for( Uint32 j = 0; j < listenersCount; ++j )
			{
				PC_SCOPE_PHYSICS( CC_VC_CollisionResponseCapsuleSweepTestEvent2 );
				m_eventListeners[j].Get()->CallEvent( eventName, result.m_position, result.m_normal, disp, penetration, actorHeight, diffZ, false );
			}
		}

		// debug view
		#ifndef NO_EDITOR
		//m_sweeps.PushBack( DebugCapsule( from+disp+Vector( 0, 0, -h*0.5f), r, h ) );
		//m_normalRays.PushBack( result.m_position );
		//m_normalRays.PushBack( result.m_position + result.m_normal );
		#endif

		// result
		return true;
	}

	// def result
	return false;
}
//////////////////////////////////////////////////////////////////////////

void CPhysicsCharacterWrapper::CacheVirtualControllersBones()
{
	// update virtual controllers
	if( !m_virtualControllers.Empty() )
	{
		PC_SCOPE_PHYSICS( CC_VC_CacheVirtualControllersBones );
		CAnimatedComponent* ac = nullptr;;
		if ( GetParent( ac ) )
		{
			VCCTArray::iterator itEnd = m_virtualControllers.End();
			for ( VCCTArray::iterator it = m_virtualControllers.Begin(); it != itEnd; ++it )
			{
				const CName& boneName = it->GetBoneName();
				it->SetBoneIndex( boneName != CName::NONE ? ac->FindBoneByName( boneName ) : -1 );
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// update v-controllerss
void CPhysicsCharacterWrapper::UpdateVirtualControllers( const Float dt )
{
	// update virtual controllers
	if( !m_virtualControllers.Empty() )
	{
		PC_SCOPE_PHYSICS( CC_VC_UpdateVirtualControllers );
		CAnimatedComponent* ac = nullptr;
		if ( GetParent( ac ) )
		{
			// because parent matrix ws is delayed one frame we need to apply translation directly from CC
			Matrix parentMatrixWS = ac->GetLocalToWorld();
			parentMatrixWS.SetTranslation( m_footPosition );

			VCCTArray::iterator itEnd = m_virtualControllers.End();
			for ( VCCTArray::iterator it = m_virtualControllers.Begin(); it != itEnd; ++it )
			{
				Int32 boneIndex = it->GetBoneIndex();
				if ( boneIndex != -1 )
				{
					it->UpdateGlobalPosition( ac->GetBoneMatrixWorldSpace( static_cast< Uint32 >( boneIndex ) ), parentMatrixWS );
				}
				else
				{
					it->UpdateGlobalPosition( parentMatrixWS, Matrix::IDENTITY );
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// compute separation
static Bool ComputeSeparation( const Vector& posA, const Vector& posB, const Float radiusA, const Float radiusB, const Float heightA, const Float heightB, Vector& separationSum, const Float dt, const Uint32 iteration )
{
	// compare space in Z axis
	if ( posB.Z < posA.Z + heightA && posB.Z + heightB > posA.Z )
	{
		const Float radiusSum = radiusA + radiusB;
		const Float radiusSumSqr = radiusSum*radiusSum;

		Vector distance = posA - posB;
		distance.Z = 0.0f;
		Float length = distance.SquareMag2();

		// square distance compare
		if ( radiusSumSqr > length )
		{
			// compute separation
			length = MSqrt( length );
			const Float dif = radiusSum - length;
			distance.Normalize2();
			distance *= dif;

			// sum separations
			separationSum += distance;
			return true;
		}
	}
	return false;
};
//////////////////////////////////////////////////////////////////////////
//
// response for collision of predicted movement
void CPhysicsCharacterWrapper::VirtualControllersCollisionResponse( const Float dt )
{
	// update virtual controllers
	if( !m_virtualControllers.Empty() )
	{
		PC_SCOPE_PHYSICS( CC_VC_CollisionResponseTest );

		// init
		SPhysicsContactInfo res[ CC_SPHERICAL_COLLIDERS_MAX ];

		// update spherical colliders
		for ( int no = 0; no < CC_SPHERICAL_COLLIDERS_MAX; ++no )
		{
			if ( m_sphereColliders[no].m_enabled )
			{
				// update life time
				if ( m_sphereColliders[no].m_lifeTime < collideSphereLifeTime )
					m_sphereColliders[no].m_lifeTime += dt;
				else
					m_sphereColliders[no].m_enabled = false;
			}
		}

		const Uint32 count = m_virtualControllers.Size();
		for( Uint32 i = 0; i < count; ++i )
		{
			if ( m_virtualControllers[i].IsEnabled() )
			{
				PC_SCOPE_PHYSICS( CC_VC_CollisionResponse );
				
				Vector from = GetCenterPosition();
				Vector vfrom = m_virtualControllers[i].GetCenterPosition();
				from.Z = vfrom.Z;
				from += (from-vfrom)*collideSphereBackOffMul;
				Vector currMove = vfrom-from + Vector( m_currMovementVector.X, m_currMovementVector.Y, 0 );
				{
					if ( m_virtualControllers[i].m_collisionGrabber )
					{
						CName none = CName::NONE;

						// todo: check box option
						//Vector3 halfExtends( m_virtualControllers[i].m_virtualRadius*0.71f, m_virtualControllers[i].m_virtualRadius*0.71f, m_virtualControllers[i].m_height*0.5f );
						//Int32 a = m_world->BoxSweepTestWithMultipleResults( from, from+currMove, halfExtends, ac->GetLocalToWorld(), m_virtualControllers[i].m_collisionGrabberGroupMask, 0, &res[0], 6, nullptr, true );

						Uint32 a = 0;
						m_world->CapsuleSweepTestWithMultipleResults( from, from+currMove, m_virtualControllers[i].m_virtualRadius, m_virtualControllers[i].m_height, m_virtualControllers[i].m_collisionGrabberGroupMask, 0, &res[0], a, 6, nullptr, true );
						if ( a > 0 )
						{
							// for all responses
							for ( Uint32 no = 0; no < a; ++no )
							{
								if  ( res[no].m_distance > 0.0f )
								{
									// compute collider position
									Vector normal = Vector( res[no].m_normal.X, res[no].m_normal.Y, 0 );
									normal.Normalize3();
									Vector newColliderPos = from + currMove*res[no].m_distance - normal*(m_virtualControllers[i].m_virtualRadius/* + SCCTDefaults::DEFAULT_CONTACT_OFFSET*/) - normal*(collideSphereRadius-collideSphereRadiusOffset) - Vector( 0, 0, collideSphereHeight*0.65f );

									//m_normalRays.PushBack( res[no].m_position );
									//m_normalRays.PushBack( res[no].m_position + normal );

									// check distance to other colliders
									Bool anyCollide = false;
									for ( Uint32 cols = 0; cols < CC_SPHERICAL_COLLIDERS_MAX; ++cols )
									{
										if ( m_sphereColliders[ cols ].m_enabled )
										{
											if ( newColliderPos.DistanceTo( m_sphereColliders[ cols ].m_pos ) < collideSphereMinDist )
											{
												anyCollide = true;
												break;
											}
										}
									}

									// setup new collider
									if ( !anyCollide )
									{
										SphericalCollider& col = m_sphereColliders[ m_sphereCollidersCounter++ ];
										m_sphereCollidersCounter %= CC_SPHERICAL_COLLIDERS_MAX;

										col.m_enabled = true;
										col.m_pos = newColliderPos;
										col.m_lifeTime = 0.0f;

										//WARN_ENGINE( TXT( "new collider %i/%i" ), m_sphereCollidersCounter, CC_SPHERICAL_COLLIDERS_MAX );
									}
								}
							}
						}
					}

					// collision response
					if ( m_virtualControllers[i].m_collisionResponse )
					{
						// compute separation
						Vector separation( Vector::ZEROS );
						Vector posA = m_footPosition;
						const Vector dispPerIteration = m_currMovementVector/(Float)collideSphereIterations;
						const Float characterH = GetCurrentHeight();
						const Float characterR = m_virtualControllers[i].m_physicalRadius;
						Bool itAnyEnabled;
						for ( int it = 0; it < collideSphereIterations; ++it )
						{
							itAnyEnabled = false;
							for ( int no = 0; no < CC_SPHERICAL_COLLIDERS_MAX; ++no )
							{
								if ( m_sphereColliders[no].m_enabled )
								{
									itAnyEnabled = true;

									// check direction
									if ( m_currMovementVector.Dot2( (m_sphereColliders[no].m_pos-posA) ) > 0.0f )
									{
										// compute separation with collider
										Vector localSep( Vector::ZEROS );
										if ( ComputeSeparation( vfrom + separation, m_sphereColliders[no].m_pos, characterR, collideSphereRadius, characterH, collideSphereHeight, localSep, dt, it ) )
										{
											// reset collider time
											m_sphereColliders[no].m_lifeTime = 0.0f;
											if ( localSep.Mag2() > 0.001f )
											{
												separation += localSep;
												//WARN_ENGINE( TXT( "> collide it %i with %i sep %f/%f (%f, %f, %f/%f, %f, %f)" ), it, no, localSep.Mag3(), separation.Mag3(), localSep.X, localSep.Y, localSep.Z, separation.X, separation.Y, separation.Z );
											}
										}
									}
								}
							}

							// break if not enabled
							if ( !itAnyEnabled )
							{
								break;
							}

							// move one iteration ahead
							posA += dispPerIteration;
							vfrom += dispPerIteration;
						}

                        //DBGSRV_SET_NATIVE_PROP_VALUE("separationZ", separation.Z);

						// set response
						m_currMovementVector += separation;
						m_currMovementVector.W = 1.0f;
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void CPhysicsCharacterWrapper::OnParentSkeletonChanged()
{
	CacheVirtualControllersBones();
}

//////////////////////////////////////////////////////////////////////////
//
// check for collisions - prediction
void CPhysicsCharacterWrapper::CheckCollisionPrediction()
{
	PC_SCOPE_PHYSICS( CC_CheckCollisionPrediction );

	if ( !m_collisionPredictionEventName.Empty() && m_currMovementVector.Mag3() > 0.001f )
	{
		// init
		static const CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Fence ) );
		const Vector currMovement = m_currMovementVector * (m_collisionPredictionMovementMul + m_collisionPredictionMovementAdd/m_currMovementVector.Mag3());
		SPhysicsContactInfo res;
		Vector from, to;

		// check main controller
		from = GetCenterPosition();
		DoCapsuleSweepTest( from, currMovement, GetCurrentHeight(), GetCurrentCharacterRadius(), m_collisionPredictionEventName, false, res, includeMask );

		// check virtual controllers
		if( !m_virtualControllers.Empty() )
		{
			const Uint32 count = m_virtualControllers.Size();
			for ( Uint32 i = 0; i < count; ++i )
			{
				if ( m_virtualControllers[i].IsEnabled() )
				{
					from = m_virtualControllers[i].GetCenterPosition();
					DoCapsuleSweepTest( from, currMovement, m_virtualControllers[i].m_height, m_virtualControllers[i].m_virtualRadius, m_collisionPredictionEventName, true, res, includeMask );
				}
			}
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// ragdoll will switch to swimming
void CPhysicsCharacterWrapper::SetRagdollToSwimming( const Bool enable )
{
	m_switchFromRagdollToSwimming = enable;
}
//////////////////////////////////////////////////////////////////////////
//
// set swimming mode
void CPhysicsCharacterWrapper::SetSwimming( const Bool enable )
{
	CheckOnSwitchingFromRagdoll();
	if ( enable )
	{
		m_state = CPS_Swimming;
		//DBGSRV_SET_STRING_PROP_VALUE( "state", TXT( "CPS_Swimming" ) );
		m_gravity = false;
		m_terrainNormal = Vector::EZ;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
		m_terrainNormalDamped.Set( Vector::EZ );
		m_normalShapeHits = 1;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "normalShapeHits", m_normalShapeHits );
		m_shapeHit = false;
		m_slidingState = SS_None;
		m_slidingDirDamped.Set( Vector::ZEROS );
	}
	// No sense in disabling custom state if not in it.
	else if( m_state == CPS_Swimming )
	{
		m_state = CPS_Simulated;
		//DBGSRV_SET_STRING_PROP_VALUE( "state", TXT( "CPS_Simulated" ) );
		m_gravity = true;
	}
}
//////////////////////////////////////////////////////////////////////////
//
// return center position
Vector CPhysicsCharacterWrapper::GetCenterPosition() const
{
	ASSERT( m_characterController );
	if ( !m_characterController )
		return Vector::ZEROS;
	return Vector( (Float)m_characterController->getPosition().x, (Float)m_characterController->getPosition().y, (Float)m_characterController->getPosition().z );
}
//////////////////////////////////////////////////////////////////////////
//
// add terrain normal
void CPhysicsCharacterWrapper::AddTerrainNormal( const Vector& normal )
{
	// store terrain normal
	if ( normal.Z > 0.0f )
	{
		++m_normalShapeHits;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "normalShapeHits", m_normalShapeHits );
		m_terrainNormal += normal*terrainNormalAddMul;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
		m_shapeHit = true;
	}
	m_terrainNormal.W = 0.0f;
}
//////////////////////////////////////////////////////////////////////////
//
// add collision obstacles data
void CPhysicsCharacterWrapper::AddCollisionObstacle( CEntity* entity, const Vector& point, const Vector& normal )
{
	if ( m_collisionObstaclesDataCount < CC_MAX_COLLISION_OBSTACLES_DATA )
	{
		m_collisionObstaclesData[ m_collisionObstaclesDataCount ].m_entity = entity;
		m_collisionObstaclesData[ m_collisionObstaclesDataCount ].m_point = point;
		m_collisionObstaclesData[ m_collisionObstaclesDataCount ].m_normal = normal;
		++m_collisionObstaclesDataCount;

		if ( normal.Z > 0.71f )
		{
			++m_normalShapeHits;
			//DBGSRV_SET_NATIVE_PROP_VALUE( "normalShapeHits", m_normalShapeHits );
			m_terrainNormal += normal*terrainNormalAddMul;
			//DBGSRV_SET_NATIVE_PROP_VALUE( "terrainZ", m_terrainNormal.Z );
			m_shapeHit = true;
		}
	}
}
//////////////////////////////////////////////////////////////////////////
//
// reset collision obstacles data
void CPhysicsCharacterWrapper::ResetCollisionObstaclesData()
{
	m_collisionObstaclesDataCount = 0;
	for ( int no = 0; no < CC_MAX_COLLISION_OBSTACLES_DATA; ++no )
	{
		m_collisionObstaclesData[no].m_entity = nullptr;
	}
}
//////////////////////////////////////////////////////////////////////////
//
// return of collision data count
const Uint32 CPhysicsCharacterWrapper::GetCollisionObstaclesDataCount()
{
	return m_collisionObstaclesDataCount;
}
//////////////////////////////////////////////////////////////////////////
//
// return collision data by index
const SCollisionData& CPhysicsCharacterWrapper::GetCollisionObstaclesData( const Uint32 index )
{
	if ( index < m_collisionObstaclesDataCount )
	{
		return m_collisionObstaclesData[ index ];
	}
	return SCollisionData::EMPTY;
}
//////////////////////////////////////////////////////////////////////////
//
// add collision characters data
void CPhysicsCharacterWrapper::AddCollisionCharacter( CEntity* entity, const Vector& point, const Vector& normal )
{
	if ( m_collisionCharactersDataCount < CC_MAX_COLLISION_CHARACTERS_DATA )
	{
		m_collisionCharactersData[ m_collisionCharactersDataCount ].m_entity = entity;
		m_collisionCharactersData[ m_collisionCharactersDataCount ].m_point = point;
		m_collisionCharactersData[ m_collisionCharactersDataCount ].m_normal = normal;
		++m_collisionCharactersDataCount;
	}
}
//////////////////////////////////////////////////////////////////////////
//
// reset collision data
void CPhysicsCharacterWrapper::ResetCollisionCharactersData()
{
	m_collisionCharactersDataCount = 0;
	for ( int no = 0; no < CC_MAX_COLLISION_CHARACTERS_DATA; ++no )
	{
		m_collisionCharactersData[no].m_entity = nullptr;
	}
}
//////////////////////////////////////////////////////////////////////////
//
// return of collision data count
const Uint32 CPhysicsCharacterWrapper::GetCollisionCharactersDataCount()
{
	return m_collisionCharactersDataCount;
}
//////////////////////////////////////////////////////////////////////////
//
// return collision data by index
const SCollisionData& CPhysicsCharacterWrapper::GetCollisionCharactersData( const Uint32 index )
{
	if ( index < m_collisionCharactersDataCount )
	{
		return m_collisionCharactersData[ index ];
	}
	return SCollisionData::EMPTY;
}
//////////////////////////////////////////////////////////////////////////
//
// move to position
void CPhysicsCharacterWrapper::ForceMoveToPosition( const Vector& position, Bool resetZAxis )
{
	PC_SCOPE_PHYSICS( CC_ForceMoveToPosition );

	if ( m_characterController )
	{
		Vector vec = position - GetPosition();
		if ( resetZAxis )
		{
			vec.Z = 0;
		
		}

		m_world->SceneUsageAddRef();

		if( !m_world->IsSceneWhileProcessing() )
		{
			physx::Cct::CapsuleController *cct = static_cast<physx::Cct::CapsuleController*>( m_characterController );
			cct->mOverlapRecover = PxVec3( 0.f );
			m_characterController->move( TO_PX_VECTOR( vec ), 0.0f, 0.01f, PxControllerFilters( &GetFilterData()->m_data, 0, 0 ) );
		}
		else
		{
			RED_LOG( CharacterController, TXT( "failed ForceMoveToPosition processed while scene fetch" ) );
		}

		m_world->SceneUsageRemoveRef();

		m_footPosition = TO_VECTOR_FROM_EXT_VEC( m_characterController->getFootPosition() );
	}
	else
	{
		Vector newPos = position;
		if ( resetZAxis )
		{
			newPos.Z = m_footPosition.Z;
		}
		m_footPosition = newPos - Vector( 0, 0.5f*m_currentHeight, 0 );
		m_desc.position = TO_PX_EXT_VECTOR( newPos );
	}

	//DBGSRV_SET_NATIVE_PROP_VALUE( "CCfootPos", m_footPosition );
}
//////////////////////////////////////////////////////////////////////////
//
// force set center position
// zero-move called to reset internal state 
void CPhysicsCharacterWrapper::ForceSetCenterPosition( const Vector& position )
{
	PC_SCOPE_PHYSICS( CC_ForceSetCenterPosition );

	if ( m_characterController )
	{
		m_characterController->setPosition( TO_PX_EXT_VECTOR( position ) );

		m_world->SceneUsageAddRef();

		if( !m_world->IsSceneWhileProcessing() )
		{
			physx::Cct::CapsuleController *cct = static_cast<physx::Cct::CapsuleController*>( m_characterController );
			cct->mOverlapRecover = PxVec3( 0.f );
			m_characterController->invalidateCache();
			m_characterController->move( PxVec3(PxZero), 0.0f, 0.01f, PxControllerFilters( &GetFilterData()->m_data, 0, 0 ) );
		}
		else
		{
			RED_LOG( CharacterController, TXT( "failed ForceSetCenterPosition processed while scene fetch" ) );
		}

		m_world->SceneUsageRemoveRef();

		m_footPosition = TO_VECTOR_FROM_EXT_VEC( m_characterController->getFootPosition() );
	}
	else
	{
		m_footPosition = position - Vector( 0, 0.5f*m_currentHeight, 0 );
		m_desc.position = TO_PX_EXT_VECTOR( position );
	}

    //DBGSRV_SET_NATIVE_PROP_VALUE( "CCfootPos", m_footPosition.Z );
}
//////////////////////////////////////////////////////////////////////////
//
// force set foot position
// zero-move called to reset internal state 
void CPhysicsCharacterWrapper::ForceSetPosition( const Vector& position )
{
	PC_SCOPE_PHYSICS( CC_ForceSetPosition );

	if ( m_characterController )
	{
		m_characterController->setFootPosition( TO_PX_EXT_VECTOR( position ) );

		m_world->SceneUsageAddRef();

		if( !m_world->IsSceneWhileProcessing() )
		{
			physx::Cct::CapsuleController *cct = static_cast<physx::Cct::CapsuleController*>( m_characterController );
			cct->mOverlapRecover = PxVec3( 0.f );
			m_characterController->invalidateCache();
			m_characterController->move( PxVec3(PxZero), 0.0f, 0.01f, PxControllerFilters( &GetFilterData()->m_data, 0, 0 ) );
		}
		else
		{
			RED_LOG( CharacterController, TXT( "failed ForceSetPosition processed while scene fetch" ) );
		}

		m_world->SceneUsageRemoveRef();

		m_footPosition = TO_VECTOR_FROM_EXT_VEC( m_characterController->getFootPosition() );
	}
	else
	{
		m_footPosition = position;
		m_desc.position = TO_PX_EXT_VECTOR( (position - Vector( 0, 0.5f*m_currentHeight, 0 )) );
	}

    //DBGSRV_SET_NATIVE_PROP_VALUE( "CCfootPos", m_footPosition.Z );
}
//////////////////////////////////////////////////////////////////////////
//
// set raw position of disabled physical controller
void CPhysicsCharacterWrapper::ForceSetRawPosition( const Vector& position )
{
	m_footPosition = position;
	m_desc.position = TO_PX_EXT_VECTOR( position );
	m_desc.position.z += m_desc.height * 0.5f;
}
//////////////////////////////////////////////////////////////////////////
//
//
SPhysicalMaterial* CPhysicsCharacterWrapper::GetCurrentStandPhysicalMaterial( const Vector& position, float height, const CPhysicalCollision& collision, CPhysicsWorld* world )
{
	static const CPhysicsEngine::CollisionMask rbColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( RigidBody ) );
	static const CPhysicsEngine::CollisionMask destColliderTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );

	SPhysicalFilterData data( collision );
	( ( Uint64& )data.m_data.word2) |= rbColliderTypeBit;
	( ( Uint64& )data.m_data.word2) |= destColliderTypeBit;

	SPhysicsContactInfo contact;
	if( world->RayCastWithSingleResult( position, position + ( world->GetGravityVector() * ( height+0.3f ) ), data.GetCollisionGroup(), 0, contact ) != TRV_Hit )
	{
		return nullptr;
	}

	if( !contact.m_userDataA )
	{
		return nullptr;
	}

	PxShape* shape = ( PxShape* ) contact.m_userDataA->GetShape( contact.m_rigidBodyIndexA.m_shapeIndex, contact.m_rigidBodyIndexA.m_actorIndex );
	if( !shape )
	{
		return nullptr;
	}

	PxMaterial* physxMaterial = nullptr;

	Uint32 faceIndex = 0;
	Uint32 materialsCount = shape->getNbMaterials();
	if ( materialsCount > 1 )
	{
		PxRaycastHit hit;

		if( PxShapeExt::raycast( *shape, *shape->getActor(), TO_PX_VECTOR( position ), PxVec3( 0.0f, 0.0f, -1.0f ), height+0.3f, PxSceneQueryFlag::eDISTANCE, 1, &hit, false ) )
		{
			faceIndex = hit.faceIndex;
			physxMaterial = shape->getMaterialFromInternalFaceIndex( faceIndex );
		}
	}
	else shape->getMaterials( &physxMaterial, 1 );

	if ( !physxMaterial )
	{
		return nullptr;
	}

	return static_cast<SPhysicalMaterial*>( physxMaterial->userData );
}
//////////////////////////////////////////////////////////////////////////
//
// register listener
void CPhysicsCharacterWrapper::RegisterEventListener( const THandle<IScriptable>& listener )
{
	m_eventListeners.PushBackUnique( listener );
}
//////////////////////////////////////////////////////////////////////////
//
// unregister listener
void CPhysicsCharacterWrapper::UnregisterEventListener( const THandle<IScriptable>& listener )
{
	m_eventListeners.RemoveFast( listener );
}
//////////////////////////////////////////////////////////////////////////
//
// notify listeners
void CPhysicsCharacterWrapper::NotifyListeners( const CName& event, CEntity* entity )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CC::NotifyListeners(entity) must be called fron main thread!" );
	if ( !::SIsMainThread() ) return;

	PC_SCOPE_PHYSICS( CC_NotifyListeners_Entity );

	CComponent* component = nullptr;
	if( GetParentProvider()->GetParent( component ) )
	{
		if ( entity ) component->GetEntity()->CallEvent( event, THandle<CEntity>( entity ) );
		else component->GetEntity()->CallEvent( event );
	}

	const Uint32 listenersCount = m_eventListeners.Size();
	for( Uint32 i = 0; i < listenersCount; ++i )
	{
		if ( entity ) m_eventListeners[i].Get()->CallEvent( event, THandle<CEntity>( entity ) );
		else m_eventListeners[i].Get()->CallEvent( event );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// notify listeners
void CPhysicsCharacterWrapper::NotifyListeners( const CName& event, const Float value )
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "CC::NotifyListeners(value) must be called fron main thread!" );
	if ( !::SIsMainThread() ) return;

	PC_SCOPE_PHYSICS( CC_NotifyListeners_Value );

	CComponent* component = nullptr;
	if( GetParentProvider()->GetParent( component ) )
	{
		component->GetEntity()->CallEvent( event, value );
	}

	const Uint32 listenersCount = m_eventListeners.Size();
	for( Uint32 i = 0; i < listenersCount; ++i )
	{
		m_eventListeners[i].Get()->CallEvent( event, value );
	}
}
//////////////////////////////////////////////////////////////////////////
//
// update and get internal controller state
void CPhysicsCharacterWrapper::FetchInternalState()
{
	ASSERT( m_characterController );
	if ( !m_characterController )
		return;

	PC_SCOPE_PHYSICS( CC_FetchInternalState );

	// get collision flags
	PxControllerState state;
	m_characterController->getState( state );
	m_collisionDown = false;
	m_collisionUp = false;
	m_collisionSides = false;

	// collision states
	if ( state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN )
		m_collisionDown = true;
	if ( state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_UP )
		m_collisionUp = true;
	if ( state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_SIDES || fabsf( m_terrainNormalDamped.Get().Z ) < 0.1f )
		m_collisionSides = true;

	// probe terrain normal
	if ( m_isPlayerControlled || m_isVehicle )
		ProbeTerrain();

	// set collision down when any shape on ground is hit or terrain normal Z is positive
	if ( !m_collisionDown )
	{
		if ( !m_isPlayerControlled )
			ProbeTerrain();

		if ( GETBIT( m_collisions, CS_CENTER ) )
			m_collisionDown = true;
		//DBGSRV_SET_NATIVE_PROP_VALUE( "collisionDown", m_collisionDown );
	}

	// additional overlap test
	if ( !m_collisionDown )
	{
		const static CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) )
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) ) 
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) )
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) ) 
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Door ) ) 
															   | GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

		// additional collision check
		// we do that because of problems with shapeHit results
		SPhysicsOverlapInfo res;
		if ( m_world->CapsuleOverlapWithAnyResult( GetCenterPosition()+Vector( 0, 0, isCollsionOverlapOffsetZ ), m_physicalRadius+isCollsionRadiusDiff, m_currentHeight, includeMask, 0, res ) == TRV_Hit )
		{
			m_collisionDown = true;
			//DBGSRV_SET_NATIVE_PROP_VALUE( "collisionDown", m_collisionDown );
		}
	}

	//DBGSRV_SET_NATIVE_PROP_VALUE( "collisionDown", m_collisionDown );

	// reset falling timer
	if ( m_collisionDown )
	{
		m_fallingTime = 0.0f;
	}

	m_currentStandPhysicalMaterial = nullptr;
	PxShape* shape = state.touchedShape;
	PxMaterial* physxMaterial = nullptr;
	if ( !shape )
	{
		return;
	}

	// check are we standing on dynamic
    PxRigidActor* actor = shape->getActor();
	m_isStandingOnDynamic = ( nullptr != actor->isRigidDynamic() );

	// get material
	Uint32 faceIndex = 0;
	Uint32 materialsCount = shape->getNbMaterials();
	if ( materialsCount > 1 )
	{
		PxRaycastHit hit;

		if( PxShapeExt::raycast( *shape, *shape->getActor(), m_characterController->getActor()->getGlobalPose().p, PxVec3( 0.0f, 0.0f, -1.0f ), m_currentHeight+0.3f, PxSceneQueryFlag::eDISTANCE, 1, &hit, false ) )
		{
			faceIndex = hit.faceIndex;
			physxMaterial = shape->getMaterialFromInternalFaceIndex( faceIndex );
		}
	}
	else shape->getMaterials( &physxMaterial, 1 );

	if ( !physxMaterial )
	{
		return;
	}

	m_currentStandPhysicalMaterial = static_cast<SPhysicalMaterial*>( physxMaterial->userData );

#ifndef NO_EDITOR
	// check material
	if( physxMaterial == GPhysXEngine->GetMaterial() )
	{
		PxRigidActor* actor = shape->getActor();
		if( actor->userData )
		{
			CPhysicsWrapperInterface* wrapper = ( CPhysicsWrapperInterface* ) actor->userData;
			if( wrapper )
			{
				CComponent* component = nullptr;
				if( wrapper->GetParent( component ) )
				{
					DATA_HALT( DES_Major, CResourceObtainer::GetResource( component ), TXT("Physical collision"), TXT("Physical geometry without material defined") );
				}
			}
		}
	}
#endif // NO_EDITOR
}
//////////////////////////////////////////////////////////////////////////
//
InteractionPriorityType CPhysicsCharacterWrapper::SetInteractionPriority( InteractionPriorityType interactionPriority )
{
    // grab old value
    const InteractionPriorityType returnValue = m_interactionPriority;

    // anything changed?
    if ( m_interactionPriority != interactionPriority )
        m_interactionPriority = interactionPriority;

    return returnValue;
}
//////////////////////////////////////////////////////////////////////////
//
CPhysicsCharacterWrapper* CPhysicsCharacterWrapper::SetUnpushableTarget( CPhysicsCharacterWrapper* targetCharacter )
{
	PC_SCOPE_PHYSICS( CC_SetUnpushableTarget );

	// can't set to self
	if ( targetCharacter == this )
	{
		ERR_ENGINE( TXT("CPhysicsCharacterWrapper::SetUnpushableTarget(): trying to set unpushable target to self (on: '%ls')"), GetParentProvider()->GetFriendlyName().AsChar() );
		return m_unpushableTarget;
	}

	CPhysicsCharacterWrapper* returnValue = m_unpushableTarget;

	// anything changed?
	if ( m_unpushableTarget != targetCharacter )
	{
		// we are changing unpushable target to something different
		// since it works both ways, we should null the previous target's target (if it was us)
		if ( m_unpushableTarget )
		{
			ASSERT( m_unpushableTarget->m_unpushableTarget == this );
			m_unpushableTarget->m_unpushableTarget = nullptr;
		}

		// it is safe to keep the m_unpushableTarget as a pointer, because when CPhysicsCharacterWrapper is being destroyed,
		// it notifies the CharacterControllerManager and CharacterControllerManager goes through all the wrappers and
		// resets the m_unpushableTarget field if it points the wrapper which is being destroyed.
		m_unpushableTarget = targetCharacter;

//		if ( targetCharacter )
//		{
//			// it works both ways
//			targetCharacter->SetUnpushableTarget( nullptr );  // it can have different unpushable target, so we need to recursively NULL it
//			targetCharacter->m_unpushableTarget = this;
//
//			// we need to check the initial collision and possibly move out
//			CCTManager::CCTSpatialInfoR info;
//			info.m_character = targetCharacter;
//			info.m_position = targetCharacter->GetPosition().AsVector2();
//			info.m_radius = targetCharacter->GetCurrentRadius();
//
//			if ( CollidesWith( info ) )
//			{
//				m_moveVec = ComputeSepVec( info );
//			}
//		}
	}

	//ASSERT( m_unpushableTarget == nullptr || m_unpushableTarget->m_unpushableTarget == this );
	return returnValue;
}
//////////////////////////////////////////////////////////////////////////
//

Bool CPhysicsCharacterWrapper::DoTraceZTest( const Vector& pointWS, Vector& outPosition ) const
{
	PC_SCOPE_PHYSICS( CC_DoTraceZTest );

	// looking for god place to spawn
	const Float radius = m_physicalRadius * 0.75f; // HACK? to avoid hitting ladders that have collisions set when looking for correct placement on ground
	const Float offsetUp = m_currentHeight - radius * 1.05f;

	Vector footPos( pointWS );
	footPos.Z += offsetUp-radius;
	Vector destRay = footPos;
	destRay.Z -= offsetUp+radius;
	SPhysicsContactInfo outInfo;
	const static CPhysicsEngine::CollisionMask includeMask = CPhysicalCollision::COLLIDES_ALL;
	const static CPhysicsEngine::CollisionMask excludeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( ClothCollider ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( DeniedArea ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( Dangles ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( Projectile ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) ) 
															| GPhysicEngine->GetCollisionTypeBit( CNAME( Weapon ) )
															| GPhysicEngine->GetCollisionTypeBit( CNAME( ParticleCollider ) )
                                                            | GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) )
															| GPhysicEngine->GetCollisionTypeBit( CNAME( BoatSide ) )
															| GPhysicEngine->GetCollisionTypeBit( CNAME( BoatCollider ) )
															| GPhysicEngine->GetCollisionTypeBit( CNAME( LockClimb ) )
															| GPhysicEngine->GetCollisionTypeBit( CNAME( UnlockClimb ) )
															| GPhysicEngine->GetCollisionTypeBit( CNAME( CameraOccludable ) );

	if ( m_world->SweepTestWithSingleResult( footPos, destRay, radius, includeMask, excludeMask, outInfo ) == TRV_Hit )
	{
		ASSERT( outInfo.m_distance > 0.0f, TXT( "Sweep test returned distance 0! Collision occurred at start pos!" ) );
		if ( outInfo.m_distance > 0.0f )
		{
			// use corrected location
			outPosition = footPos;
			outPosition.Z = outInfo.m_position.Z - radius*(1.0f-outInfo.m_normal.Z);

			// corrected
			return true;
		}
	}

	outPosition = pointWS;

	// def result - correction not needed
	return false;
}

const Bool CPhysicsCharacterWrapper::IsOnPlatform()
{
	return !!m_hPlatform.Get();
}

Bool CPhysicsCharacterWrapper::CorrectInitialPlacement()
{
	Vector outPosition;

	if ( DoTraceZTest( m_footPosition, outPosition ) )
	{
		ForceSetPosition( outPosition );
		
		return true;
	}
	else
	{
		return false;
	}
}
//////////////////////////////////////////////////////////////////////////
//
// invalidate internal cache
void CPhysicsCharacterWrapper::InvalidatePhysicsCache()
{
#ifndef RED_FINAL_BUILD
	if( !SPhysicsSettings::m_characterManualCacheInvalidation ) return;
#endif

	if ( m_characterController )
	{
		m_characterController->invalidateCache();
#ifndef RED_FINAL_BUILD
		Double time = Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_cacheCreationTime;
		m_cacheCreationTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
		RED_LOG_ERROR( RED_LOG_CHANNEL( CharacterController ), TXT("CACHE INVALIDATION which where %f seconds old on %s "), time, GetParentProvider()->GetFriendlyName().AsChar() );
#endif
	}
}
//////////////////////////////////////////////////////////////////////////
//
// render debug primitives
void CPhysicsCharacterWrapper::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	PC_SCOPE_PHYSICS( CC_OnGenerateEditorFragments );

#ifndef NO_EDITOR
    if( flags == SHOW_PhysXPlatforms )
    {
	    // Displacement
	    frame->AddDebug3DArrow( m_footPosition, m_platformCurrentDisplacement, 25.0f, 0.01f, 0.03f, 0.2f, Color::GREEN, Color::GREEN );
        
	    // Foot pos
	    frame->AddDebugSphere( m_footPosition, 0.1f, Matrix::IDENTITY, Color::RED );

	    // Attach point
	    frame->AddDebugSphere( dbg_platformAttachGlobal, 0.1f, Matrix::IDENTITY, Color::BLUE );
        
#ifdef USE_PLATFORM_ROTATION
	    // Rotation
	    frame->AddDebug3DArrow( dbg_platformAttachGlobal + Vector( 0, 0, 0.05f ), m_platformAttachDir, 1.5f, 0.01f, 0.03f, 0.2f, Color::RED, Color::RED );
#endif
    }

	if ( SHOW_PhysXTraceVisualization == flags )
	{
		// init
		const Vector& pos = GetPosition();
		FixedCapsule capsule;

		// character physical capsule
		{
			capsule.Set( pos, m_physicalRadius, m_currentHeight );
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, m_enabled ? Color::YELLOW : Color::GRAY );
		}

		// character virtual capsule
		if ( m_currentVirtualRadius != m_physicalRadius )
		{
			capsule.Set( pos, m_currentVirtualRadius, m_currentHeight );
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::RED );
		}

		// character virtual radius
		frame->AddDebugCircle( pos+Vector(0,0,m_currentHeight*0.5f), m_virtualRadius, Matrix::IDENTITY, Color::RED );

		// debug capsule sweeps
		const Uint32 sweepCount = m_sweeps.Size();
		for ( Uint32 no = 0; no < sweepCount; ++no )
		{
			capsule.Set( m_sweeps[no].m_pos, m_sweeps[no].m_radius, m_sweeps[no].m_height );
			frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::GREEN );
		}

		// show orient
		frame->AddDebugLine( pos, pos+m_orient, Color::RED, true );

		// show sliding
		frame->AddDebugLine( pos, pos+m_slidingDirDamped.Get()*10.0f, Color::LIGHT_CYAN, true );

		#ifndef NO_EDITOR
		if ( m_isPlayerControlled || m_isVehicle )
		{
			frame->AddDebugSphere( dbg_correctionPos, 0.1f, Matrix::IDENTITY, Color::RED, false );

			// spherical colliders
			for ( int no = 0; no < CC_SPHERICAL_COLLIDERS_MAX; ++no )
			{
				if ( m_sphereColliders[no].m_enabled )
				{
					capsule.Set( m_sphereColliders[no].m_pos-Vector(0,0,collideSphereHeight*0.5f), collideSphereRadius, collideSphereHeight );
					frame->AddDebugCapsule( capsule, Matrix::IDENTITY, Color::WHITE );
				}
			}
		}
		#endif

		// draw virtual controllers
		if( !m_virtualControllers.Empty() )
		{
			const Uint32 count = m_virtualControllers.Size();
			for( Uint32 i=0; i<count; ++i)
			{
				m_virtualControllers[i].OnGenerateEditorFragments( frame, flags );
			}
		}

		// player stats
		if ( m_isPlayerControlled )
		{
			// displacement
			Matrix mat = Matrix::IDENTITY;
			mat.SetTranslation( pos );
			if ( m_lastMoveVec.SquareMag3() > 0.0001f )
			{
				frame->AddDebugArrow( mat, m_lastMoveVec*5.0f, 1.0f, Color::WHITE, true );
			}

			// show normal
			frame->AddDebugLine( pos, pos+m_terrainNormal*3.0f, Color::BROWN, true );
			frame->AddDebugLine( pos, pos+m_terrainNormalDamped.Get()*3.0f, Color::WHITE, true );

			// normal rays
			const Uint32 vecCount = m_normalRays.Size();
			for ( Uint32 no = 0; no < vecCount; no+=2 )
			{
				frame->AddDebugLine( m_normalRays[no], m_normalRays[no+1], Color::GREEN, true );
			}

			// collisions
			for ( Uint32 no = 0; no < m_collisionCharactersDataCount; ++no )
			{
				frame->AddDebugLine( m_collisionCharactersData[no].m_point, m_collisionCharactersData[no].m_point+m_collisionCharactersData[no].m_normal, Color::LIGHT_BLUE, true );
			}
			for ( Uint32 no = 0; no < m_collisionObstaclesDataCount; ++no )
			{
				frame->AddDebugLine( m_collisionObstaclesData[no].m_point, m_collisionObstaclesData[no].m_point+m_collisionObstaclesData[no].m_normal, Color::DARK_BLUE, true );
			}

			// water level
			Vector waterPos = pos;
			waterPos.Z = GetWaterLevel();
			frame->AddDebugLine( waterPos, waterPos+Vector::EZ, Color::BLUE, true );
			frame->AddDebugLine( waterPos-Vector::EY, waterPos+Vector::EY, Color::BLUE, true );
			frame->AddDebugLine( waterPos-Vector::EX, waterPos+Vector::EX, Color::BLUE, true );

			// player states
			static Char* slidingName[5] = { TXT("None"), TXT("Front"), TXT("Back"), TXT("Left"), TXT("Right") };
			static Char* stateName[CPS_Count] = { TXT("Simulated"), TXT("Animated"), TXT("Falling"), TXT("Swimming"), TXT("Ragdoll") };
			//DBGSRV_SET_STRING_PROP_VALUE( "state", stateName[m_state] );

			const Float windScale = (GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetEnvironmentManager()) ? GGame->GetActiveWorld()->GetEnvironmentManager()->GetWindScale() : 0.0f;
			const Float depth = m_footPosition.Z - GetWaterLevel();
			Float terrainH = -10000.0f;
			CClipMap* clipMap = GGame->GetActiveWorld() ? GGame->GetActiveWorld()->GetTerrain() : nullptr;
			if ( clipMap )
			{
				clipMap->GetHeightForWorldPosition( pos, terrainH );
			}

			// controllers stats
			Uint32 controllers = 0;
			Uint32 controllersIntersections = 0;
			Uint32 controllersCollisions = 0;

			CComponent* component = nullptr;
			if( GetParent( component ) )
			{
				if( CWorld* world = component->GetWorld() )
				{

					controllers = world->GetCharacterControllerManager()->GetControllersCount();
					controllersIntersections = world->GetCharacterControllerManager()->GetIntersectionsCountFromLastFrame();
					controllersCollisions = world->GetCharacterControllerManager()->GetCollisionsCountFromLastFrame();
				}
			}
			
			// print on screen
			const Int32 posX = 840;
			Int32 posY = 150;
			frame->AddDebugScreenFormatedText( posX, posY, Color::WHITE, TXT("pos: %0.3f;%0.3f;%f (%ls)"), pos.X, pos.Y, pos.Z, m_currentStandPhysicalMaterial ? m_currentStandPhysicalMaterial->m_name.AsChar() : TXT("None") ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("state: %i - %ls - speed %f"), m_state, stateName[ m_state ], m_lastMoveVec.Mag2()/GGame->GetTimeManager()->GetLastTickTime() ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("sliding: %i coef %0.3f state %i-%ls vec len %0.3f "), IsSliding(), m_slideCoef, m_slidingState, slidingName[ m_slidingState ], m_slidingDirDamped.Get().Mag3() ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("gravity: %i intV %0.2f ext %0.2f inp %0.2f rdPushMul %0.2f"), !!m_gravity, m_internalVelocity.Mag3(), m_moveExternalDisp.Mag3(), m_moveInputDisp.Mag3(), m_ragdollPushingMultiplier ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("fallingT: %0.2f enabled %i"), m_fallingTime, m_enabled ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("radius: p %0.2f v %0.2f c %0.2f t %0.3f"), m_physicalRadius, m_virtualRadius, m_currentVirtualRadius, m_virtualRadiusTimer ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("height: p %0.2f c %0.2f"), m_height, m_currentHeight ); posY+=10;
			posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::LIGHT_BLUE, TXT("water level: %0.2f player depth %0.3f scale %0.3f"), GetWaterLevel(), depth, windScale ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::LIGHT_BLUE, TXT("inwater: %i diving: %i emerge %0.2f submerge %0.2f"), !!m_ocean, !!m_isDiving, m_emergeSpeed, m_submergeSpeed ); posY+=10;
			posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("terr: NZ: %0.3f Z: %0.3f speedMul: %0.3f "), m_terrainNormalDamped.Get().Z, terrainH, m_speedMul ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("terrain hits: %i shapeHit: %i sides: d%i/u%i/s%i "), m_normalShapeHits, m_shapeHit, !!m_collisionDown, !!m_collisionUp, !!m_collisionSides ); posY+=10;
			posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("%i %i %i "), !!GETBIT(m_collisions,CS_FRONT_LEFT), !!GETBIT(m_collisions,CS_FRONT), !!GETBIT(m_collisions,CS_FRONT_RIGHT) );
			frame->AddDebugScreenFormatedText( posX+50, posY, Color::YELLOW, TXT("collisions: chars %i objs %i"), m_collisionCharactersDataCount, m_collisionObstaclesDataCount );
			posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("%i %i %i "), !!GETBIT(m_collisions,CS_LEFT), !!GETBIT(m_collisions,CS_CENTER), !!GETBIT(m_collisions,CS_RIGHT) );
			frame->AddDebugScreenFormatedText( posX+50, posY, Color::YELLOW, TXT("controller: %i inters %i colls %i"), controllers, controllersIntersections, controllersCollisions);
			posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::YELLOW, TXT("%i %i %i "), !!GETBIT(m_collisions,CS_BACK_LEFT), !!GETBIT(m_collisions,CS_BACK), !!GETBIT(m_collisions,CS_BACK_RIGHT) ); posY+=10;
			posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::DARK_CYAN, TXT("platform %i local %ls"), !!m_hPlatform.Get(), ToString( m_platformAttachPointLocal ).AsChar() ); posY+=10;
			frame->AddDebugScreenFormatedText( posX, posY, Color::DARK_GREEN, TXT("canMov %i"), !!CanMove(m_footPosition) ); posY+=10;

			// jump start point
			frame->AddDebugAxis( m_fallingStartPos, Matrix::IDENTITY, 1.0f, Color::WHITE, true );
		}
	}
#endif // ndef NO_EDITOR
}
//////////////////////////////////////////////////////////////////////////
//
// return controller name - DanielB [TODO] iteration 2
const char*	CPhysicsCharacterWrapper::GetName( Int32 actorIndex )
{
	return "character\0";
}
//////////////////////////////////////////////////////////////////////////
//
// when landed after jump or falling state
void CPhysicsCharacterWrapper::OnLanded( Float velocity )
{
	// DanielB - killing is moved to scripts
	NotifyListeners( CNAME( OnHitGround ) );
}
//////////////////////////////////////////////////////////////////////////
//
// on shape hit - called when controller hit any shape while moving
void CPhysicsCharacterWrapper::onShapeHit( const PxControllerShapeHit& hit )
{
	if ( hit.shape == nullptr )
	{
		ASSERT( hit.shape );
		return;
	}

	PC_SCOPE_PHYSICS( CC_Move_OnShapeHit );

	// init
	const static CPhysicsEngine::CollisionMask debrisTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) );
	const static CPhysicsEngine::CollisionMask destTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Destructible ) );
	const static CPhysicsEngine::CollisionMask foliageTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Foliage ) );
	const static CPhysicsEngine::CollisionMask ragdollTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) );
	const static CPhysicsEngine::CollisionMask corpseTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Corpse ) );
	const static CPhysicsEngine::CollisionMask clothTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( ClothCollider ) );
	const static CPhysicsEngine::CollisionMask fenceTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Fence ) );
	const static CPhysicsEngine::CollisionMask platformTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( Platforms ) );

    static const CPhysicsEngine::CollisionMask boatTypeBit            = GPhysicEngine->GetCollisionTypeBit( CNAME( Boat ) );
	static const CPhysicsEngine::CollisionMask boatSideTypeBit        = GPhysicEngine->GetCollisionTypeBit( CNAME( BoatSide ) );
    static const CPhysicsEngine::CollisionMask boatDestructionTypeBit = GPhysicEngine->GetCollisionTypeBit( CNAME( BoatDestruction ) );

	if ( hit.controller )
	{
		SPhysicalFilterData filter( hit.shape->getSimulationFilterData() );
		CPhysicsEngine::CollisionMask shapeMask = filter.GetCollisionType();

		// init

		// filter out some kind of shapes before storing collision data
		if ( shapeMask & (ragdollTypeBit|corpseTypeBit|clothTypeBit|fenceTypeBit) )
		{
			AddTerrainNormal( Vector::EZ );
			return;
		}

		PxExtendedVec3 ccPos = m_characterController->getPosition();
		ccPos.z += GetCurrentCharacterRadius() - m_currentHeight*0.5f;
		const Float dist = sqrtf( (Float)hit.worldPos.distanceSquared( ccPos ) );

		// hit any shape (foliage is excluded in terrain normal calculations)
		if ( dist < GetCurrentCharacterRadius()+0.3f && !(shapeMask & (foliageTypeBit|debrisTypeBit)) && hit.shape->getActor()->isRigidDynamic() == nullptr )
		{
			// store terrain normal
			AddTerrainNormal( TO_VECTOR( hit.worldNormal ) );
		}

		// add collision data
		if ( hit.shape->getActor()->userData )
		{
			CPhysicsWrapperInterface* wrapper = reinterpret_cast< CPhysicsWrapperInterface* >( hit.shape->getActor()->userData );

			// dynamic or static object
			if ( wrapper && hit.shape->userData )
			{
				SActorShapeIndex magicUnion = ( SActorShapeIndex& )hit.shape->userData;
				CComponent* component = nullptr;
				wrapper->GetParent( component, magicUnion.m_actorIndex );
				
				// std entity
				if ( component )
				{
					AddCollisionObstacle( component->GetEntity(), TO_VECTOR_FROM_EXT_VEC( hit.worldPos ), TO_VECTOR_FROM_EXT_VEC( hit.worldNormal ) );
				}

				// foliage collision - important for jumping and other custom states
				else
				{
					AddCollisionObstacle( nullptr, TO_VECTOR_FROM_EXT_VEC( hit.worldPos ), TO_VECTOR_FROM_EXT_VEC( hit.worldNormal ) );
				}
			}

			// terrain
			else
			{
				AddCollisionObstacle( nullptr, TO_VECTOR_FROM_EXT_VEC( hit.worldPos ), TO_VECTOR_FROM_EXT_VEC( hit.worldNormal ) );
			}
		}

		// e3 - jumping on foliage - special case
		if ( shapeMask & foliageTypeBit )
		{
			m_shapeHit = true;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Get platform CC is standing on
	PxFilterData filter = hit.shape->getSimulationFilterData();
	SPhysicalFilterData& data = (SPhysicalFilterData&) filter;
	CPhysicsEngine::CollisionMask mask = data.GetCollisionType();

	// Compare hit shape mask
    if ( !( (boatTypeBit | boatSideTypeBit | boatDestructionTypeBit | platformTypeBit) & mask ) )   // If touched shape do not have "Platforms" group and is not part of the boat, apply force to push it aside
    {
		PxControllerState state;
		m_characterController->getState( state );

		PxActor* actor = hit.shape->getActor();
		PxRigidDynamic* dynamic = actor->isRigidDynamic();

	    if ( !dynamic )
		    return;

	    if( dynamic->getRigidBodyFlags() & PxRigidBodyFlag::eKINEMATIC )
		    return;

		if ( m_onTouchCallback )
		{
			SActorShapeIndex index = hit.shape ? ( SActorShapeIndex& ) hit.shape->userData : SActorShapeIndex();
			CPhysicsWrapperInterface* wrapper = ( CPhysicsWrapperInterface* ) actor->userData;
			CComponent* component = nullptr;
			if( wrapper->GetParent( component, index.m_actorIndex ) )
			{
				m_onTouchCallback->onCharacterTouch( component, index );
			}
		}
		
	    if ( hit.length < 0.001f )
		    return;

		if( SPhysicsSettings::m_onlyPlayerCanPush )
		{
			if( SPhysicsSettings::m_nonplayersCanPushOnlyNonsleepers )
			{
				if( dynamic->isSleeping() )
				{
					return;
				}
			}

			if( !m_isPlayerControlled && !m_canPush )
			{
				return;
			}
		}

	    PxVec3 force = TO_PX_VECTOR( m_orient );
	    force.z = 0.0f;
	    force.normalize();

	    float multiplier = SPhysicsSettings::m_characterPushingMultipler * hit.length;
	    if( multiplier > SPhysicsSettings::m_characterPushingMaxClamp )
	    {
		    multiplier = SPhysicsSettings::m_characterPushingMaxClamp;
	    }

	    force *= multiplier;

		dynamic->wakeUp();
		PxVec3 point( ( Float ) hit.worldPos.x, ( Float ) hit.worldPos.y, ( Float ) hit.worldPos.z );

		//LOG_ENGINE( TXT( "> impulse %0.3f len %0.3f impnew %0.3f dt %0.3f - 0x%08x/0x%08x (%i)" ), force.magnitude(), hit.length, force.magnitude()*GGame->GetTimeManager()->GetLastTickTime(), GGame->GetTimeManager()->GetLastTickTime(), hit.shape, actor, dynamic->getNbShapes() );

		force *= m_pushingTime;
		if( m_state == CPS_Simulated || m_state == CPS_Swimming )
		{
			PxRigidBodyExt::addForceAtPos( *dynamic, force, point, PxForceMode::eIMPULSE );
		}
    }
}

//////////////////////////////////////////////////////////////////////////

const Float CPhysicsCharacterWrapper::GetPlatformRotation() const 
{
#ifdef USE_PLATFORM_ROTATION
    return m_platformRotInfluenceRad;
#else
    ASSERT( true, TXT("Platform rotation calculation code is turned off!") );
    return 0.0f;
#endif
}

//////////////////////////////////////////////////////////////////////////
//
// DanielB [TODO] iteration 2
void CPhysicsCharacterWrapper::onControllerHit( const PxControllersHit& hit )
{
}
//////////////////////////////////////////////////////////////////////////
//
// DanielB [TODO] iteration 2
void CPhysicsCharacterWrapper::onObstacleHit( const PxControllerObstacleHit& hit )
{
}

Float CPhysicsCharacterWrapper::GetWaterLevel()
{
	Uint32 lastTick = Uint32( GEngine->GetCurrentEngineTick() );

	if( m_lastWaterLevelUpdateTick != lastTick )
	{
		const Vector ccPos( m_footPosition.X, m_footPosition.Y, -10000.0f );
		m_waterLevel = GGame->GetActiveWorld()->GetWaterLevel( ccPos, 0 );
	}

	return m_waterLevel;
}

#ifndef RED_FINAL_BUILD
void CPhysicsCharacterWrapper::CheckNormal(SPhysicsContactInfo &outInfo)
{
	if( outInfo.m_normal.Z <= 0.0f )
	{
		String debugName = String::EMPTY;
		const CComponent* component = nullptr;
		outInfo.m_userDataA->GetParent( component, outInfo.m_rigidBodyIndexA.m_actorIndex );
		if( !component )
		{
			const CResource* parentRes = nullptr;
			outInfo.m_userDataA->GetParent( parentRes, outInfo.m_rigidBodyIndexA.m_actorIndex );
			if( parentRes)
			{
				debugName = parentRes->GetFriendlyName();
			}
			else
			{
				debugName = TXT("Unknown");
			}
		}
		else
		{
			debugName =  component->GetParent()->GetFriendlyName();
		}
		ASSERT( outInfo.m_normal.Z > 0.0f, TXT("Possible inverted normals on triangle mesh!!! [%s]"), debugName.AsChar() );
	}	
}
#endif

//////////////////////////////////////////////////////////////////////////
//
// ctor
VectorDamper::VectorDamper()
{
	m_current = m_prev = Vector::ZEROS;
	m_factor = 0.5f;
}
//////////////////////////////////////////////////////////////////////////
//
// init params
void VectorDamper::Init( const Vector& start, Float factor )
{
	m_current = m_prev = start;
	m_factor = factor;
}
//////////////////////////////////////////////////////////////////////////
//
// do damping on current value
void VectorDamper::Update( const Vector& dest, Float dt )
{
	m_prev = m_current;
	Vector dif = dest-m_prev;
	if ( dif.SquareMag3() > 0.001f )
	{
		dif.W = 0;
		m_current = m_prev + ( dif * Clamp( m_factor * dt * 33.33f, 0.0f, 1.0f ) );
		m_current.W = 0;
		return;
	}

	m_current = dest;
}

//////////////////////////////////////////////////////////////////////////

#endif
