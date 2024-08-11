/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////
// CNames for serialization
// RED_DECLARE_NAME( ProjectileTrajectory );
// 
// RED_DECLARE_NAME( projectileCaster );
// RED_DECLARE_NAME( projectileProjectileName );
// RED_DECLARE_NAME( projectileStarted );
// RED_DECLARE_NAME( projectileProjectileTarget );
// RED_DECLARE_NAME( projectileProjectileVelocity );
// RED_DECLARE_NAME( projectileMaxRange );
// RED_DECLARE_NAME( projectileDistanceTraveled );
// RED_DECLARE_NAME( projectileRadius );
// RED_DECLARE_NAME( projectileCollisionGroups );
// RED_DECLARE_NAME( projectileStartPosition );
// RED_DECLARE_NAME( projectileCurveProgress );
// RED_DECLARE_NAME( projectileInitialTargetDistance );
// RED_DECLARE_NAME( projectileVelocityVec );
// RED_DECLARE_NAME( projectileSineAmplitude );
// RED_DECLARE_NAME( projectileBounceOfCount );
// RED_DECLARE_NAME( projectileBounceOfVelocityPreserve );
// RED_DECLARE_NAME( projectileCakeAngle );
// RED_DECLARE_NAME( projectileDeltaStep );
// RED_DECLARE_NAME( projectileLastStartStep );
// 
// RED_DECLARE_NAME( projectileTargetType );
// 
// RED_DECLARE_NAME( projectileAnimatedTimeMultiplier );

//////////////////////////////////////////////////////////////////////////

const static Uint32 PROJECTILE_MAX_AREA_TEST_RESULTS    = 20;
const static Float  PROJECTILE_OVERLAP_ACCURACY         = 0.2f;
const static Float  PROJECTILE_WATER_TEST_ACCURACY      = 0.5f;
const static Float  PROJECTILE_WATER_MIN_TEST_HEIGHT    = 2000.0f;  // really don't know how high water pools will be in final game

//////////////////////////////////////////////////////////////////////////

class IProjectileTarget;

//////////////////////////////////////////////////////////////////////////

class CProjectileTrajectory : public CGameplayEntity
{
    DECLARE_ENGINE_CLASS( CProjectileTrajectory, CGameplayEntity, 0 )

	struct SSphereOverlapTestData
	{
		SSphereOverlapTestData( Vector worldPosition, Float radius, CPhysicsEngine::CollisionMask collisionMask )
			: m_worldPosition( worldPosition )
			, m_radius( radius )
			, m_collisionMask( collisionMask )
		{}

		Vector m_worldPosition;
		Float m_radius;
		CPhysicsEngine::CollisionMask m_collisionMask;
	};

public:
    CProjectileTrajectory();
    virtual ~CProjectileTrajectory(){}

    void            StartProjectile( IProjectileTarget* target, Float angle, Float velocity, Float projectileRange );
    void            Initialize( CEntity* caster );

    virtual void    OnFinalize();
    virtual void    OnAttached( CWorld* world );
    virtual void    OnDetached( CWorld* world );
    virtual void    OnTick( Float timeDelta );
    virtual void    OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

    virtual void    OnPropertyExternalChanged( const CName& propertyName );

//     virtual void    OnSaveGameplayState( IGameSaver* saver );
//     virtual void    OnLoadGameplayState( IGameLoader* loader );

    // Getters
    RED_INLINE CEntity&   GetCaster()                         { ASSERT( m_caster.Get() ); return *m_caster.Get(); }
    RED_INLINE void       SetRealCaster( CEntity* entity )    { m_realCaster = entity; }

protected:

    Bool    IsBehindWall( const Vector& startPoision, CComponent* component, CPhysicsEngine::CollisionMask collisionMask );  // Checks if hit object is behind other objects
    void    StopProjectile();
    Vector  CalculateNewPosition( Float deltaTime );
    //void    WaterSlowdown( Float deltaTime, Float slowdownMultiplier );

	// raycast query using both: physical representation and virtual controllers, returns true if there was a hit
    Bool    CheckForCollisionsRaycast( const Vector& endPoint, const Vector& dirVec, Float timeDelta, Bool wasWaterHit = false );

	// raycast query using physical representation, returns true if there was a hit
	Bool	CheckForCollisionsRaycast_PhysX( const Vector& startPoint, const Vector& endPoint, CPhysicsEngine::CollisionMask collisionMask, Bool wasWaterHit = false );

	// raycast query using virtual controllers, returns true if there was a hit
	Bool	CheckForCollisionsRaycast_Gameplay( const Vector& startPoint, const Vector& endPoint );

	// box overlap test using physical representation, true if there was a hit
    Bool    CheckForCollisionsOverlap( const Vector& endPoint, const Vector& dirVec, Float moveStep, Bool wasWaterHit = false );

	// sphere overlap test using both: physical representation and virtual controllers, returns true if there was a hit
	Bool	SphereOverlapTest( const Vector& position, Float radius, CPhysicsEngine::CollisionMask collisionMask );

	// sphere overlap test using physical representation, returns true if there was a hit
	Bool	SphereOverlapTest_PhysX( const Vector& position, Float radius, CPhysicsEngine::CollisionMask collisionMask );

	// sphere overlap test using virtual controllers, returns true if there was a hit
	Bool	SphereOverlapTest_Gameplay( const Vector& position, Float radius );

private:
    // Events
    void funcInit( CScriptStackFrame& stack, void* result );
    void funcShootProjectileAtPosition( CScriptStackFrame& stack, void* result );
    void funcShootProjectileAtNode( CScriptStackFrame& stack, void* result );
	void funcShootProjectileAtBone( CScriptStackFrame& stack, void* result );
    void funcStopProjectile( CScriptStackFrame& stack, void* result );
    void funcShootCakeProjectileAtPosition( CScriptStackFrame& stack, void* result );   // Starts area projectile, tests for overlap in cake like shape
    void funcBounceOff( CScriptStackFrame& stack, void* result );                       // Does bounce off after projectile collision
    void funcIsBehindWall( CScriptStackFrame& stack, void* result );                    // Checks if hit object is behind other objects
    void funcSphereOverlapTest( CScriptStackFrame& stack, void* result );               // Simple sphere overlap tests for a given radius
    void funcIsStopped( CScriptStackFrame& stack, void* result );

protected:
    THandle< CEntity >						m_caster;
    THandle< CEntity >						m_realCaster;

	TDynArray< SSphereOverlapTestData >		m_sphereOverlapTestData;

    TSortedArray< CComponent* >             m_uniqueCollisionComponents;
    CName									m_projectileName;

    Bool									m_started;

    IProjectileTarget*                      m_projectileTarget;
    Float                                   m_projectileVelocity;
    Float									m_maxRange;
    Float                                   m_distanceTraveled;
    Float									m_radius;

    CPhysicsEngine::CollisionMask           m_collisionGroups;
    Vector                                  m_startPosition;
    Vector                                  m_prevTrajectoryPositon;
    Float                                   m_curveProgress;
    Float                                   m_initialTargetDistance;
    Vector                                  m_velocityVec;

    Float                                   m_sineAmplitude;

    Float                                   m_bounceOfVelocityPreserve;

    // Checking collisions in cake like shape
    Float                                   m_cakeAngle;
    Float                                   m_deltaStep;
    Vector                                  m_lastStartStep;

    // Animated properties
    Vector                                  m_animatedOffset;
    Float                                   m_animatedTimeMultiplier;

    // Accuracy properties
    Float                                   m_overlapAccuracy;

    // Water testing
    Bool                                    m_doWaterLevelTest;
    Float                                   m_waterTestDelta;
    Float                                   m_waterTestAccuracy;
    Float                                   m_waterMinTestHeight;

    // Debug cake shape
#ifndef NO_EDITOR
private:
    struct SDebugTimeoutPoint
    {
        Vector hitPos;
        Float radius;
        String name;
        Float totalDrawTime;

        SDebugTimeoutPoint( const Vector& pos )
            : totalDrawTime(0.0f)
            , hitPos(pos)
            , radius(0.0f)
            , name(TXT(""))
        {}
    };

    //Uint32              m_dbgWaterTestsNo;
    Vector              m_dbgBoxHalfExtent;
    Vector              m_dbgBoxPosition;
    Vector              m_dbgDirVec;
    //TDynArray<Vector>   m_dbgWallTest;

    TDynArray<Vector>   m_dbgTrajcetory;
    Float               m_dbgTimePass;

    TDynArray<Vector>   m_dbgGlobalHitPos;
    
    TDynArray<SDebugTimeoutPoint> m_dbgOverlapSpheres;
    TDynArray<SDebugTimeoutPoint> m_dbgOverlapHits;
#endif
};

//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CProjectileTrajectory );
    PARENT_CLASS( CGameplayEntity );

    PROPERTY_EDIT( m_projectileName, TXT( "Projectile name" ) );
    PROPERTY_EDIT( m_animatedOffset, TXT("Offset from trajectory") );
    PROPERTY_EDIT( m_animatedTimeMultiplier, TXT("Time step multiplier") );
    PROPERTY_EDIT( m_bounceOfVelocityPreserve, TXT("Percent of velocity preserved after each bounce.") );

    PROPERTY_EDIT( m_overlapAccuracy, TXT("How accurate overlap test is ( steps length in meters ).") );

    PROPERTY_EDIT( m_doWaterLevelTest, TXT("Perform water level tests") );
    PROPERTY_EDIT( m_waterTestAccuracy, TXT("How accurate water level test is ( steps length in meters ).") );

    PROPERTY( m_caster );
    PROPERTY( m_realCaster );
    PROPERTY_EDIT_RANGE( m_radius, TXT("Projectile radius. If radius is set to 0, it will operate on raycasts, if greater it will do the sweep tests."), 0.f, 1000.f );

    NATIVE_FUNCTION( "Init", funcInit );
    NATIVE_FUNCTION( "ShootProjectileAtPosition", funcShootProjectileAtPosition );
    NATIVE_FUNCTION( "ShootProjectileAtNode", funcShootProjectileAtNode );
	NATIVE_FUNCTION( "ShootProjectileAtBone", funcShootProjectileAtBone );
    NATIVE_FUNCTION( "StopProjectile", funcStopProjectile );
    NATIVE_FUNCTION( "ShootCakeProjectileAtPosition", funcShootCakeProjectileAtPosition );
    NATIVE_FUNCTION( "BounceOff", funcBounceOff );
	NATIVE_FUNCTION( "IsBehindWall", funcIsBehindWall );
	NATIVE_FUNCTION( "SphereOverlapTest", funcSphereOverlapTest );
    NATIVE_FUNCTION( "IsStopped", funcIsStopped );

END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
