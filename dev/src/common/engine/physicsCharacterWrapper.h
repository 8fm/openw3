#pragma once

//////////////////////////////////////////////////////////////////////////
// headers
#include "..\physics\physicsWrapper.h"
#include "..\physics\physXEngine.h"
#include "..\physics\physicsWorldPhysXImpl.h"
#include "interactionPriority.h"
#include "springDampers.h"
#include "showFlags.h"

//////////////////////////////////////////////////////////////////////////
// defines
#define CC_MAX_COLLISION_OBSTACLES_DATA		6
#define CC_MAX_COLLISION_CHARACTERS_DATA	4
#define CC_MIN_SLIDING_SQR					0.0001f
#define CC_MAX_RAGDOLL_PUSH_MUL				10.0f
#define CC_SPHERICAL_COLLIDERS_MAX			6

//#define USE_PLATFORM_ROTATION

//////////////////////////////////////////////////////////////////////////
// forwards
class CVirtualCharacterController;


//////////////////////////////////////////////////////////////////////////
// macros
#define SETBIT( x, y )		( x |= (1 << ( y )) )
#define GETBIT( x, y )		( x & (1 << ( y )) )


//////////////////////////////////////////////////////////////////////////
// structs

// Default values used by character controllers
struct SCCTDefaults
{
	static const Float			DEFAULT_HEIGHT;
	static const Float			DEFAULT_RADIUS;
	static const Float			DEFAULT_STEP_OFFSET;
	static const Float			DEFAULT_MAX_SLOPE;
	static const Float			DEFAULT_CONTACT_OFFSET;
	static const InteractionPriorityType DEFAULT_INTERACTION_PRIORITY;
	static const CName&			DEFAULT_COLLISION_TYPE;
	static const Float			MAX_SEP;
	static const Float			SEP_VEC_FACTOR;
	static const Float          TIME_OFF_PLATFORM_THRESH;
    static const Float          PLATTFORM_DETACH_THRESHOLD;
};

// struct that defines a character controller (for the constructor)
struct SPhysicsCharacterWrapperInit
{
	Vector										m_initialPosition;
	const class CCharacterControllerParam*		m_params;
	class IPhysicalCollisionTriggerCallback*	m_onTouchCallback;

	Bool										m_needsHitCallback		: 1;
	Bool										m_needsBehaviorCallback	: 1;
	Bool										m_combatMode			: 1;
	Bool										m_canBePlayerControlled	: 1;
	Bool										m_vehicle				: 1;
	SPhysicsCharacterWrapperInit();
};

// debug capsule - sweeps
struct DebugCapsule
{
	DebugCapsule() {};
	DebugCapsule( const Vector& pos, const Float r, const Float h ) { m_pos = pos; m_height = h; m_radius = r; }
	Vector m_pos;
	Float m_height;
	Float m_radius;
};

// collision data
struct SCollisionData
{
	DECLARE_RTTI_STRUCT( SCollisionData );

	THandle<CEntity>		m_entity;
	Vector					m_point;
	Vector					m_normal;

	static const SCollisionData EMPTY;

	RED_INLINE SCollisionData() : m_point( Vector::ZEROS ), m_normal( Vector::ZEROS ) {};
};

BEGIN_CLASS_RTTI( SCollisionData );
	PROPERTY( m_entity );
	PROPERTY( m_point );
	PROPERTY( m_normal );
END_CLASS_RTTI();

// spherical collider
struct SphericalCollider
{
	SphericalCollider()
	{
		Reset();
	}
	void Reset()
	{
		m_enabled = false;
		m_lifeTime = 0.0f;
		m_pos = Vector::ZEROS;
	}

	Vector m_pos;
	Float m_lifeTime;
	Bool m_enabled;
};


//////////////////////////////////////////////////////////////////////////
// enums
enum ECharacterPhysicsState : CEnum::TValueType
{
	CPS_Simulated,
	CPS_Animated,

	CPS_Falling,
	CPS_Swimming,
	CPS_Ragdoll,
	
	CPS_Count,
};

BEGIN_ENUM_RTTI( ECharacterPhysicsState );
	ENUM_OPTION( CPS_Simulated );
	ENUM_OPTION( CPS_Animated );

	ENUM_OPTION( CPS_Falling );
	ENUM_OPTION( CPS_Swimming );
	ENUM_OPTION( CPS_Ragdoll );
	
	ENUM_OPTION( CPS_Count );
END_ENUM_RTTI();

enum ESlidingStates
{
	SS_None,
	SS_Front,
	SS_Back,
	SS_Left,
	SS_Right,
};

enum ECollisionSides
{
	CS_FRONT,
	CS_RIGHT,
	CS_BACK,
	CS_LEFT,
	CS_FRONT_LEFT,
	CS_FRONT_RIGHT,
	CS_BACK_RIGHT,
	CS_BACK_LEFT,
	CS_CENTER,
	CS_MAX = 32,
};

BEGIN_ENUM_RTTI( ECollisionSides );
	ENUM_OPTION( CS_FRONT );
	ENUM_OPTION( CS_RIGHT );
	ENUM_OPTION( CS_BACK );
	ENUM_OPTION( CS_LEFT );
	ENUM_OPTION( CS_FRONT_LEFT );
	ENUM_OPTION( CS_FRONT_RIGHT );
	ENUM_OPTION( CS_BACK_RIGHT );
	ENUM_OPTION( CS_BACK_LEFT );
	ENUM_OPTION( CS_CENTER );
END_ENUM_RTTI();


//////////////////////////////////////////////////////////////////////////
// vector damper
class VectorDamper
{
public:
	VectorDamper();
	void Init( const Vector& start, Float factor );
	void SetDampingFactor( const Float factor ) { m_factor = factor; }
	void Update( const Vector& dest, float dt );
	RED_INLINE void Set( const Vector& current ) { m_current = m_prev = current; }
	RED_INLINE const Vector& Get() const { return m_current; }
	RED_INLINE const Float GetLenSqr() const { return m_current.SquareMag3(); }

protected:
	Vector m_current;
	Vector m_prev;
	Float m_factor;
};

#ifdef USE_PHYSX
namespace physx
{
	class PxCharacterController;
	class PxBoxController;
};

// character controller for all the actors in game
class CPhysicsCharacterWrapper
	: public CPhysicsWrapperInterface,
	  public physx::PxUserControllerHitReport,
	  public physx::PxQueryFilterCallback,
	  public physx::PxControllerBehaviorCallback,
	  public physx::PxControllerFilterCallback
{
	friend class CCharacterControllersManager;
	friend class CPhysicsWorld;
	friend class CPhysicsWorldPhysXImpl;
	friend class CDebugServerCommandGetCharacterController;
	friend class TWrappersPool< CPhysicsCharacterWrapper, SWrapperContext >;

protected:
	// internal structs/data
	class CPhysicsWorldPhysXImpl*			m_world;
	physx::PxCapsuleController*				m_characterController;
	physx::PxCapsuleControllerDesc			m_desc;
	SPhysicalFilterData						m_filterData;

	Red::TAtomicSharedPtr< CCharacterControllersManager > m_characterControllersManager;
	// move vectors
	Vector									m_internalVelocity;
	Vector									m_moveExternalDisp;
	Vector									m_moveInputDisp;
	Vector									m_lastMoveVec;
	Vector									m_currMovementVector;
	Vector									m_footPosition;

	// terrain
	Vector									m_orient;
	Float									m_terrainLimitMin;		// [in] terrain influence limit min
	Float									m_terrainLimitMax;		// [in] terrain influence limit max
	Float									m_terrainLimitMul;		// [in] terrain influence multiplier
	Float									m_speedMul;				// [out] anim speed multiplier
	Float									m_slideCoef;			// [out] from 0 to 1, not sliding to sliding on max slope
	Float									m_slidingLimitMin;		// sliding limit min
	Float									m_slidingLimitMax;		// sliding limit max
	Float									m_slidingSpeed;			// sliding speed
	ESlidingStates							m_slidingState;			// sliding state
	Vector									m_slidingDir;			// sliding direction vector
	VectorDamper							m_slidingDirDamped;		// sliding direction damped
	Vector									m_terrainNormal;		// terrain normal
	VectorDamper							m_terrainNormalDamped;	// terrain normal damped
	Uint32									m_normalShapeHits;		// count of terrain normal tests

	// falling
	Float									m_fallingTime;
	Vector									m_fallingStartPos;

	// event listeners
	TDynArray< THandle< IScriptable > >		m_eventListeners;

	// state
	ECharacterPhysicsState					m_state;
	SPhysicalMaterial*						m_currentStandPhysicalMaterial;
	Uint32									m_collisions;	// set of bits - ECollisionSides
	Float									m_ragdollPushingMultiplier;

	// swimming/diving
	Uint32									m_lastWaterLevelUpdateTick;
	Float									m_waterLevel;
	Float									m_waterDepth;
	Float									m_emergeSpeed;
	Float									m_submergeSpeed;

	// CC creation parameters
	Float									m_startVirtualRadius;
	Float									m_currentVirtualRadius;
	Float									m_virtualRadiusTimer;
	Float									m_stepOffset;

	// flags
	Bool									m_enabled						: 1;
	Bool									m_teleport						: 1;
	Bool									m_correctPosition				: 1;
	Bool									m_teleportCorrectZ				: 1;
	Bool									m_gravity						: 1;
	Bool									m_needsBehaviorCallback			: 1;
	Bool									m_isInGame						: 1;
	Bool									m_ocean							: 1;
	Bool									m_isPlayerControlled			: 1;
	Bool									m_isVehicle						: 1;
	Bool									m_canPush						: 1;
	Bool									m_canBePlayerControlled			: 1;
	Bool									m_isStandingOnDynamic			: 1;
	Bool									m_shapeHit						: 1;	// shape hit while moving
	Bool									m_isDiving						: 1;
	Bool									m_slidingEnabled				: 1;
	Bool									m_collisionDown					: 1;
	Bool									m_collisionSides				: 1;
	Bool									m_collisionUp					: 1;
	Bool									m_isUpdatingVirtualRadius		: 1;
	Bool									m_additionalVertIteration		: 1;
	Bool									m_switchFromRagdollToSwimming	: 1;
	Bool									m_ragdollIsAway					: 1;
	Bool									m_ragdollInAir					: 1;
	Bool									m_staticCollisionsEnabled		: 1;
	Bool									m_dynamicCollisionsEnabled		: 1;
	Bool									m_collisionPrediction			: 1;

	// Platform movement variables
	THandle<IScriptable>                     m_hPlatform;
	Vector                                  m_platformAttachPointLocal;
    Vector                                  m_platformCurrentDisplacement;

#ifdef USE_PLATFORM_ROTATION
	Vector                                  m_platformAttachDir;
	Float                                   m_platformRotInfluenceRad;	// Amount of radians character should be rotated per step due to platform rotation
#endif

	// collision data
	SCollisionData							m_collisionObstaclesData[ CC_MAX_COLLISION_OBSTACLES_DATA ];
	SCollisionData							m_collisionCharactersData[ CC_MAX_COLLISION_CHARACTERS_DATA ];
	Uint32									m_collisionObstaclesDataCount;
	Uint32									m_collisionCharactersDataCount;

	// collision prediction
	Float									m_collisionPredictionMovementAdd;
	Float									m_collisionPredictionMovementMul;
	CName									m_collisionPredictionEventName;

	IPhysicalCollisionTriggerCallback*		m_onTouchCallback;

#ifndef NO_EDITOR
	Vector                                  dbg_platformAttachGlobal;

	TDynArray<Vector>						m_normalRays;
	TDynArray<DebugCapsule>					m_sweeps;
	Vector									dbg_correctionPos;
#endif

#ifndef RED_FINAL_BUILD
	Double									m_cacheCreationTime;
#endif

	Float									m_pushingTime;

	// ragdoll shared states
	Red::Threads::CAtomic< Bool >			m_onNoLongerInRagdoll;

	// construction
	CPhysicsCharacterWrapper				( SPhysicsCharacterWrapperInit& characterInfo );
	virtual									~CPhysicsCharacterWrapper();
	Bool									MakeReadyToDestroy( TDynArray< void* >* toRemove );

	void									PreSimulation( SWrapperContext* simulationContext, Float timeDelta, Uint64 tickMarker, TDynArray< void* >* toAdd, TDynArray< void* >* toRemove, TDynArray< void* >* toRelease, Bool allowAdd );
	virtual Uint32							GetActorsCount() const;

public:
	virtual void							Release( Uint32 actorIndex = 0 );
	virtual Bool							IsReady() const;
	virtual CPhysicsWorld*					GetPhysicsWorld();

	// internal
	RED_INLINE const SPhysicalFilterData*	GetFilterData() { return &m_filterData; }

	// debug
	virtual void							OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	// movement
	RED_INLINE void							ApplyVelocity( const Vector& vel ) { m_internalVelocity += vel; }
	void									Move( const Vector& moveDelta );
	RED_INLINE const Vector&				GetInternalVelocity() { return m_internalVelocity; }
	RED_INLINE const Vector&				GetExternalDisp() { return m_moveExternalDisp; }
	RED_INLINE const Vector&				GetInputDisp() { return m_moveInputDisp; }
	RED_INLINE Vector&						GetCurrentMovementVectorRef() { return m_currMovementVector; }
	RED_INLINE const Vector&				GetLastMoveVector() { return m_lastMoveVec; }
    
	// states
	void									Enable( const Bool enable );
	Bool									IsEnabled() { return m_enabled; }
	void									SetAnimated( const Bool enable );
	Bool									IsAnimated() { return m_state == CPS_Animated; }
	void									SetGravity( const Bool enable ) { m_gravity = enable; m_internalVelocity.SetZeros(); }
	Bool									IsGravity() { return m_gravity; }
	void									SetNeedsBehaviorCallback(Bool enable) { m_needsBehaviorCallback = enable;  }
	Bool									IsBehaviorCallbackNeeded() { return m_needsBehaviorCallback; }
	void									CheckCollisionPrediction();
	void									EnableCollisionPrediction( const Bool enable ) { m_collisionPrediction = enable; }
	const Bool								IsCollisionPredictionEnabled() { return m_collisionPrediction; }
	void									SetIfCanPush( Bool decision ) { m_canPush = decision; }
	const Bool								CanPush() { return m_canPush; }
	void									EnableAdditionalVerticalSlidingIteration( const Bool enable ) { m_additionalVertIteration = enable; }
	Bool									IsAdditionalVerticalSlidingIterationEnabled() { return m_additionalVertIteration; }

	// ragdoll
	void									SetRagdollPushMultiplier( const Float mul ) { m_ragdollPushingMultiplier = Clamp( mul, 0.0f, CC_MAX_RAGDOLL_PUSH_MUL ); };
	Float									GetRagdollPushMultiplier() const { return m_ragdollPushingMultiplier; };

	// swimming/diving
	void									SetRagdollToSwimming( const Bool enable );
	void									SetSwimming( const Bool enable );
	Bool									IsSwimming() const { return m_state == CPS_Swimming; }
	Float									GetWaterLevel();
	void									SetEmergeSpeed( const float speed ) { m_emergeSpeed = speed; };
	Float									GetEmergeSpeed() const { return m_emergeSpeed; }
	void									SetSubmergeSpeed( const float speed ) { m_submergeSpeed = speed; };
	Float									GetSubmergeSpeed() const { return m_submergeSpeed; }
	void									CheckSwimming();
	void									SetDiving( const Bool diving );
	const Bool								IsDiving() const { return m_isDiving; }
	const Bool								IsNearWater() { return m_ocean; }
	const Bool								IsFalling() const { return m_fallingTime>0; };

	// terrain
	void									AddTerrainNormal( const Vector& normal );
	const Vector&							GetTerrainNormal( const Bool damped ) const { return damped ? m_terrainNormalDamped.Get() : m_terrainNormal; }
	void									ProbeTerrainWide( Vector& normalAverage, Vector& normalGlobal, Vector& direction, Float separationH, Float separationF, Float separationB );

	// collision data
	void									AddCollisionObstacle( class CEntity* entity, const Vector& point, const Vector& normal );
	void									ResetCollisionObstaclesData();
	const Uint32							GetCollisionObstaclesDataCount();
	const SCollisionData&					GetCollisionObstaclesData( const Uint32 index );
	void									AddCollisionCharacter( class CEntity* entity, const Vector& point, const Vector& normal );
	void									ResetCollisionCharactersData();
	const Uint32							GetCollisionCharactersDataCount();
	const SCollisionData&					GetCollisionCharactersData( const Uint32 index );
	const Bool								GetGroundGridCollisionOn( ECollisionSides side) { if( GETBIT(m_collisions, side) ) return true; return false; };

	// common accessors
	RED_INLINE ECharacterPhysicsState	GetState() const		{ return m_state; }
	RED_INLINE Bool						DownCollision() const	{ return m_collisionDown; }
	RED_INLINE Bool						UpCollision() const		{ return m_collisionUp;	}
	RED_INLINE Bool						SideCollision() const	{ return m_collisionSides; }
	RED_INLINE SPhysicalMaterial*		GetCurrentStandPhysicalMaterial() const { return m_currentStandPhysicalMaterial; }
	static SPhysicalMaterial*			GetCurrentStandPhysicalMaterial( const Vector& position, float height, const CPhysicalCollision& collision, CPhysicsWorld* world );
	RED_INLINE Bool						IsTeleport() const		{ return m_teleport; }
	RED_INLINE Bool						IsPlayerControlled() const { return m_isPlayerControlled; }
	RED_INLINE Bool						IsStandingOnDynamic() const { return m_isStandingOnDynamic;	}
	RED_INLINE Float					GetSpeedMul() { return m_speedMul; }
	RED_INLINE void						SetShapeHit() { m_shapeHit = true; }
	const Bool							IsShapeHit() { return m_shapeHit; }

	// collisions
	RED_INLINE void						EnableStaticCollisions( Bool enable ) { m_staticCollisionsEnabled = enable; }
	RED_INLINE Bool						IsStaticCollisionsEnabled()	{ return m_staticCollisionsEnabled; }
	RED_INLINE void						EnableDynamicCollisions( Bool enable ) { m_dynamicCollisionsEnabled = enable; }
	RED_INLINE Bool						IsDynamicCollisionsEnabled() { return m_dynamicCollisionsEnabled; }
	const Float							GetPushingTime() { return m_pushingTime; }

	// sliding
	RED_INLINE Bool						IsSliding() { return m_slidingDirDamped.GetLenSqr() > CC_MIN_SLIDING_SQR; }
	RED_INLINE ESlidingStates			GetSlidingState() { return m_slidingState; }
	RED_INLINE Float					GetSlideCoef() const { return m_slideCoef; }
	RED_INLINE Vector					GetSlidingDir() const { return m_slidingDir; }
	RED_INLINE void						SetSlidingSpeed( const Float speed ) { m_slidingSpeed = speed; }
	RED_INLINE void						SetSliding( const Bool enable ) { m_slidingEnabled = enable; }
	const Bool							IsSlidingEnabled() { return m_slidingEnabled; }
	RED_INLINE void						SetSlidingLimits( const Float min, const Float max ) { m_slidingLimitMin = min; m_slidingLimitMax = max; }

	// terrain
	RED_INLINE void						SetTerrainLimits( const Float min, const Float max ) { m_terrainLimitMin = min; m_terrainLimitMax = max; }
	RED_INLINE void						SetTerrainInfluenceMultiplier( const Float mul ) { m_terrainLimitMul = mul; }

	// position
	void								Teleport( const Vector& target, const Bool correctZ = false );
	void								CorrectPosition();
	void								ForceMoveToPosition( const Vector& position, Bool resetZAxis = true );
	void								ForceSetCenterPosition( const Vector& position );
	void								ForceSetPosition( const Vector& position );
	void								ForceSetRawPosition( const Vector& position );
	virtual Vector						GetPosition( Uint32 actorIndex = 0 ) const override { RED_UNUSED( actorIndex ); return m_footPosition; };
	Vector								GetCenterPosition() const;
	Bool								DoTraceZTest( const Vector& pointWS, Vector& outPosition ) const;

	// platform
	const Bool							IsOnPlatform();
	const Vector&						GetPlatformLocalPos() { return m_platformAttachPointLocal; }
	const Float							GetPlatformRotation() const;

	// height
	RED_INLINE Float						GetCurrentHeight() const { return m_currentHeight;																	}
	void									SetHeight( const Float height );
	void									ResetHeight();

	// radiuses
	RED_INLINE Float						GetCurrentCharacterRadius() const { return Red::Math::NumericalUtils::Max( m_currentVirtualRadius, m_physicalRadius ); }
	RED_INLINE Float						GetVirtualRadius() const { return  m_virtualRadius;																	}
	RED_INLINE Float						GetCurrentVirtualRadius() const { return m_currentVirtualRadius;													}
	RED_INLINE Float						GetPhysicalRadius() const { return m_physicalRadius;																}
	void									SetVirtualRadius( const Float vRadius, Bool immediately );
	void									UpdateVirtualRadius( const Float dt );
	void									ResetVirtualRadius();
	const Bool								IsUpdatingVirtualRadius() { return m_isUpdatingVirtualRadius; }
	void									UpdateVirtualControllers( const Float dt );

	// event listeners
	void									RegisterEventListener( const THandle<IScriptable>& listener );
	void									UnregisterEventListener( const THandle<IScriptable>& listener );

	// character-character interactions
	InteractionPriorityType					SetInteractionPriority( InteractionPriorityType interactionPriority );
	CPhysicsCharacterWrapper*				SetUnpushableTarget( CPhysicsCharacterWrapper* targetCharacter );
	InteractionPriorityType					GetInteractionPriority() const { return m_interactionPriority; }
	CPhysicsCharacterWrapper*				GetUnpushableTarget() const { return m_unpushableTarget; }

	// utils
	void									InvalidatePhysicsCache();
	virtual const char*						GetName( Int32 actorIndex = -1 );

	// PxUserControllerHitReport - hit reports
	virtual void							onShapeHit( const physx::PxControllerShapeHit& hit );
	virtual void							onControllerHit( const physx::PxControllersHit& hit );
	virtual void							onObstacleHit( const physx::PxControllerObstacleHit& hit );

	// PxQueryFilterCallback - collisions filtering
	virtual physx::PxQueryHitType::Enum		preFilter(const physx::PxFilterData& filterData, const physx::PxShape* shape, const physx::PxRigidActor* actor, physx::PxHitFlags& queryFlags);
	virtual physx::PxQueryHitType::Enum		postFilter( const physx::PxFilterData& filterData, const physx::PxSceneQueryHit& hit );

	// PxControllerBehaviorCallback - behavior callback
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags( const physx::PxShape& shape, const physx::PxActor& actor );
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags( const physx::PxController& controller );
	virtual physx::PxControllerBehaviorFlags getBehaviorFlags( const physx::PxObstacle& obstacle );

	// PxControllerFilterCallback - cc vs cc filter
	virtual bool							filter( const physx::PxController& a, const physx::PxController& b );

protected:
	// construction
	void									InitDescription( physx::PxCapsuleControllerDesc& desc, const SPhysicsCharacterWrapperInit& characterInfo );

	// state
	void									FetchInternalState();
	void									ProbeTerrain();

	// update
	void									SimulatedUpdate( const Float dt );
	void									FallingUpdate( const Float dt );
	void									SwimmingUpdate( const Float dt );
	void									CustomUpdate( const Float dt );
	void									AnimatedUpdate( const Float dt );
	void									RagdollUpdate( const Float dt );
	void									PlatformUpdate( const Float dt );

	// movement
	void									ApplyInternalMovement( const Float dt );
	void									ApplyExternalMovement( const Float dt );
	void									ApplyGravity( const Float dt );
	void									ApplySliding( Vector& disp, const Float dt );
	void									ApplySwimming( Vector& disp, const Float dt );
	void									ApplyGravity( Vector& _orgin, const Float dt );
	void									ApplyTerrainInfluence( Vector& disp );

private:
	// ragdoll related stuff 
	void									SwitchToRagdollState( const Bool ragdollInAir );
	const Bool								IsRagdollOnGround();
	void									PullRagdollBackIfTooFarAway();
	void									CheckOnSwitchingFromRagdoll();
	Bool									GetRagdollData( Vector& position, Vector& delta );
    void                                    ResetPlatform();
	
public:
	// Movement update
    void									Update1_ComputeMovementVector( const Float timeDelta );
    void                                    Update2_ApplyMovement( const Float timeDelta );

	// move check
	Bool									CanMove( const Vector& position );
	Bool									ShouldMove();

#ifndef RED_FINAL_BUILD
	void									CheckNormal( SPhysicsContactInfo &outInfo );
#endif

	void									GrabPlatform( SPhysicsContactInfo& hit );
	bool									AddTerrainNormalIfNeeded(  const Vector& from, const Vector& to, CPhysicsEngine::CollisionMask include, CPhysicsEngine::CollisionMask exclude, SPhysicsContactInfo& outContactInfo,  Vector& resultingNormal );
	Bool									DoCapsuleSweepTest( const Vector& from, const Vector& disp, const Float h, const Float r, CName& eventName, Bool virtualController, SPhysicsContactInfo& result, const Uint64 includeGroups );

	// misc
	void									NotifyListeners( const CName& event, CEntity* entity = nullptr );
	void									NotifyListeners( const CName& event, const Float value );
	Bool									OnTeleport( const Float dt );
	void									OnLanded( Float velocity );

	// corrections
	void									PlaceAt( Vector const & newLoc );
	Bool									CorrectInitialPlacement();
	void									DoCorrectionBeforeMove( physx::PxVec3& disp );
	void									DoCorrectionAfterMove( const physx::PxVec3& disp );
	void									CorrectGravity( const Float dt );

	//////////////////////////////////////////////////////////////////////////
	
protected:
	typedef TDynArray<CVirtualCharacterController> VCCTArray;

	// virtual controllers
	VCCTArray							m_virtualControllers;
	Float								m_virtualPitch;
	Float								m_lastResDist;

	// interactions
    InteractionPriorityType				m_interactionPriority;
	mutable CPhysicsCharacterWrapper*	m_unpushableTarget;

	// params
	Float								m_height;
	Float								m_currentHeight;

	// radiuses
	Float								m_physicalRadius;
	Float								m_virtualRadius;
	Float								m_baseVirtualRadius;

	// spherical colliders
	SphericalCollider					m_sphereColliders[CC_SPHERICAL_COLLIDERS_MAX];
	Uint32								m_sphereCollidersCounter;
	
#ifndef NO_EDITOR
	StringAnsi							m_debugName;
#endif

 	// virtual controllers
	void									CacheVirtualControllersBones();
	void									VirtualControllersCollisionResponse( const Float dt );
public:
	void									OnParentSkeletonChanged();

	VCCTArray&								GetVirtualControllers( void ) { return m_virtualControllers; }
	void									SetVirtualControllersPitch( const Float pitch ) { m_virtualPitch = pitch; }
	const Float								GetVirtualControllerPitch() { return m_virtualPitch; }
};

#endif
