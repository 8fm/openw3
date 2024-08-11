#include "build.h"
#include "boatBuoyancySystem.h"
#include "BoatComponent.h"

#include "../../common/game/commonGame.h"
#include "../../common/game/gameWorld.h"
#include "../../common/engine/component.h"
#include "../../common/physics/physicsWrapper.h"
#include "../engine/mesh.h"
#include "../physics/physXEngine.h"
#include "../physics/physicsWorldPhysXImpl.h"
#include "../engine/clipMap.h"
#include "../engine/rigidMeshComponent.h"
#include "../engine/renderFrame.h"
#include "../engine/world.h"

//////////////////////////////////////////////////////////////////////////

#ifdef USE_PHYSX
using namespace physx;
#endif

//////////////////////////////////////////////////////////////////////////

CBoatBuoyancySystem::CBoatBuoyancySystem(void)
    : m_isDrowning( false )
#ifdef USE_PHYSX
    , m_dynamicConstrainer( nullptr )
    , m_sixDofJoint( nullptr )
#endif
    , m_drowningTimeSoFar( 0.0f )
    , m_tiltAnglePerc( Vector2(0,0) )
    , m_boatComponent( nullptr )
    , m_boatBodyRigidMesh( nullptr )
    , m_isInitialized( false )
    , m_isXYLocked( false )
    , m_firstTick( true )
    , m_increasedDampingTimeout(0.0f)
{
}

//////////////////////////////////////////////////////////////////////////

CBoatBuoyancySystem::~CBoatBuoyancySystem(void)
{
    Deinitialize();
}

//////////////////////////////////////////////////////////////////////////

Bool CBoatBuoyancySystem::Initialize( CBoatComponent* boatComponent, THandle<CRigidMeshComponent>* hBoatBodyRigidMesh )
{
    if( m_isInitialized )
        return true;

    ASSERT( hBoatBodyRigidMesh );
    ASSERT( boatComponent );
    if( hBoatBodyRigidMesh == nullptr || (*hBoatBodyRigidMesh).Get() == nullptr || boatComponent == nullptr )
        return false;

    CWorld* world = boatComponent->GetWorld();
    if( world == nullptr )
        return false;

    CPhysicsWrapperInterface* boatBodyWrapper = (*hBoatBodyRigidMesh)->GetPhysicsRigidBodyWrapper();
    ASSERT( boatBodyWrapper );
    if( boatBodyWrapper == nullptr )
        return false;
    
    m_boatBodyRigidMesh = hBoatBodyRigidMesh;
    m_boatComponent     = boatComponent;
    
#ifdef FORCE_KINEMATIC_STATE
    // This is very wrong what is done here, but we have no other option since the engine design is so sick
    // All boats wrappers (encapsulated in CRigitMeshComponents) should be dynamic rigid bodies attached to boat body using fixed joint.
    // Hard attachment does not let you do such thing
    // So...
    // Force rigid components kinematic state
    for( ComponentIterator<CRigidMeshComponent> it(m_boatComponent->GetEntity()); it; ++it )
    {
        CRigidMeshComponent* rc = *it;
        if( rc == nullptr )
            continue;

        CPhysicsWrapperInterface* wrapper = rc->GetPhysicsRigidBodyWrapper();
        if( wrapper == nullptr )
            continue;

        //         LOG_ENGINE( TXT("_4fiter_ name: %s isKinematic: %s noBuoyancy: %s noGravity: %s")
        //             , rc->GetName().AsChar()
        //             , wrapper->IsKinematic() ? TXT("true"):TXT("false")
        //             , wrapper->GetFlag(PRBW_DisableBuoyancy) ? TXT("true"):TXT("false")
        //             , wrapper->GetFlag(PRBW_DisableGravity) ? TXT("true"):TXT("false")
        //             );

        // In case somebody forgot to do this in boat entity, switch all components to kinematic state
        if( !wrapper->IsKinematic() )
            boatBodyWrapper->SwitchToKinematic( true );
    }
#endif

#ifdef USE_PHYSX

    //////////////////////////////////////////////////////////////////////////
    // Set up 6DOF joint to constrain tilt
    PxRigidDynamic* boatActor = nullptr;
    {
        PxActor* actor = ( PxActor* ) boatBodyWrapper->GetActor();
        if( actor->isRigidDynamic() )
            boatActor = (PxRigidDynamic*)actor;

        if( boatActor == nullptr )
            return false;
    }

    // Disable gravity for boat
    boatActor->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, true );

    // Change mass position from debug pages    
    Vector localMassPosition = TO_VECTOR( boatActor->getCMassLocalPose().p );
    if( localMassPosition.Y != BoatConfig::cvBoatMassCenter.Get() )
    {
        localMassPosition.Y = BoatConfig::cvBoatMassCenter.Get();
        boatBodyWrapper->SetMassPosition( localMassPosition );
    }

    // Change mass from debug pages
    const Float massLocal = boatBodyWrapper->GetMass();
    if( massLocal != BoatConfig::cvBoatMass.Get() )
        boatBodyWrapper->SetMass( BoatConfig::cvBoatMass.Get() );

    // Set damping
    if( boatBodyWrapper->GetDampingLinear() != BoatConfig::cvBoatLinearDamping.Get() )
        boatBodyWrapper->SetDampingLinear( BoatConfig::cvBoatLinearDamping.Get() );

    if( boatBodyWrapper->GetDampingAngular() != BoatConfig::cvBoatAngularDamping.Get() )
        boatBodyWrapper->SetDampingAngular( BoatConfig::cvBoatAngularDamping.Get() );

    // Check physx
    PxPhysics* physics = GPhysXEngine->GetPxPhysics();
    CPhysicsWorldPhysXImpl* physXWorld = nullptr;
	world->GetPhysicsWorld( physXWorld );
    PxScene* commonScene = physXWorld->GetPxScene();
    if( physics == nullptr || commonScene == nullptr )
        return false;

    // Get default material
    PxMaterial* material = nullptr;
    physics->getMaterials( &material, 1 );

    if( material == nullptr )
    {
        return false;
    }

    PxTransform kinematicPose = boatActor->getGlobalPose();
    kinematicPose.p -= PxVec3( 0, 0, KINEMATIC_CONSTRAINER_RADIUS );

#ifndef FINAL
    DBG_constrainerGlobalPos = TO_VECTOR( kinematicPose.p );
#endif

    // Create kinematic actor
    m_dynamicConstrainer = PxCreateDynamic( *physics, kinematicPose, PxSphereGeometry( KINEMATIC_CONSTRAINER_RADIUS ), *material, 0.01f );
    if( m_dynamicConstrainer == nullptr )
    {
        return false;
    }

    // Disable gravity for dynamic constrainer
    boatActor->setActorFlag( PxActorFlag::eDISABLE_GRAVITY, true );
    m_dynamicConstrainer->setRigidBodyFlag( PxRigidBodyFlag::eKINEMATIC, true );

    // Add to common scene
    commonScene->addActor( *m_dynamicConstrainer );

    // Remove constrainer's shape from queries/collisions
    {
        PxShape* constrainerShape = nullptr;
        m_dynamicConstrainer->getShapes( &constrainerShape, 1 );

        if( constrainerShape == nullptr )
        {
            Deinitialize();
            return false;
        }

        // Disable collision for this shape
        constrainerShape->setFlag( PxShapeFlag::eSIMULATION_SHAPE, false );
        // Disable raycast, overlaps, etc for this shape
        constrainerShape->setFlag( PxShapeFlag::eSCENE_QUERY_SHAPE, false );
    }

    // Create constraining joint
    {
        // Create transforms
        PxTransform constrainerTransform = PxTransform::createIdentity();
        constrainerTransform.p += PxVec3( 0, 0, KINEMATIC_CONSTRAINER_RADIUS );

        m_sixDofJoint = PxD6JointCreate( *physics, boatActor, PxTransform::createIdentity(), m_dynamicConstrainer, constrainerTransform );

        if( m_sixDofJoint == nullptr )
        {
            Deinitialize();
            return false;
        }
    }

    // Set infinite mass to constrain angular movement
    m_sixDofJoint->setInvMassScale1( 0 );

    // Set up constraining 6DOF joint
    // Free movement
    m_sixDofJoint->setMotion( PxD6Axis::eX, PxD6Motion::eFREE );
    m_sixDofJoint->setMotion( PxD6Axis::eY, PxD6Motion::eFREE );
    m_sixDofJoint->setMotion( PxD6Axis::eZ, PxD6Motion::eFREE );

    // Free Z rotation
    m_sixDofJoint->setMotion( PxD6Axis::eSWING2, PxD6Motion::eFREE );

    // Limited X, Y rotations
    m_sixDofJoint->setMotion( PxD6Axis::eSWING1, PxD6Motion::eLIMITED );
    m_sixDofJoint->setMotion( PxD6Axis::eTWIST, PxD6Motion::eLIMITED );

    // Limit rotation axes
    m_sixDofJoint->setTwistLimit( PxJointAngularLimitPair( -DEG2RAD( BoatConfig::cvBoatTiltXLimit.Get() ), DEG2RAD( BoatConfig::cvBoatTiltXLimit.Get() ) ) );
    m_sixDofJoint->setSwingLimit( PxJointLimitCone(         DEG2RAD( BoatConfig::cvBoatTiltYLimit.Get() ), DEG2RAD( BoatConfig::cvBoatTiltYLimit.Get() ) ) );
#endif

    //////////////////////////////////////////////////////////////////////////
    // Set up buoyancy points, a little bit on the inside of boats body convex mesh
    const Float scale = 0.9f;
    const Vector bbsize = (*m_boatBodyRigidMesh)->GetMeshNow()->GetBoundingBox().CalcExtents() * scale;

    SWaterContactPoint pointF;
    SWaterContactPoint pointB;
    SWaterContactPoint pointR;
    SWaterContactPoint pointL;

    pointF.m_localOffset.Set3( 0, bbsize.Y, 0 );
    pointB.m_localOffset.Set3( 0, -bbsize.Y / (scale - 0.1f), 0 );   // Undo scaling
    pointR.m_localOffset.Set3( bbsize.X, 0, 0 );
    pointL.m_localOffset.Set3( -bbsize.X, 0, 0 );

    m_buoyancyPoints[0] = pointF;
    m_buoyancyPoints[1] = pointB;
    m_buoyancyPoints[2] = pointR;
    m_buoyancyPoints[3] = pointL;

    // Initialized
    m_isInitialized = true;
    return true;
}

//////////////////////////////////////////////////////////////////////////

void CBoatBuoyancySystem::Deinitialize( void )
{   
    if( !m_isInitialized )
        return;

#ifdef USE_PHYSX
    if( m_sixDofJoint != nullptr )
    {
        m_sixDofJoint->release();
        m_sixDofJoint = nullptr;
    }

    if( m_dynamicConstrainer != nullptr)
    {
        m_dynamicConstrainer->release();
        m_dynamicConstrainer = nullptr;
    }    
#endif

    m_isInitialized = false;
}

//////////////////////////////////////////////////////////////////////////

Float PID::Update( Float setValue, Float dt )
{
	const Float error = setValue - m_value;

	if ( m_ki > 0.f && MAbs( error ) > 0.0001f )
	{
		m_integral = m_integral + error*dt;
	}

	const Float derivative = ( error - m_prevErr ) / dt;
	const Float output = m_kp*error + m_ki*m_integral + m_kd*derivative;

	const Float outputClamped = Clamp( output, -m_maxValue, m_maxValue );

	m_prevErr = error;

	return outputClamped;
}

//////////////////////////////////////////////////////////////////////////

void CBoatBuoyancySystem::ResetState()
{
	m_pid.Reset( 0.f );
}

void CBoatBuoyancySystem::BuoyancyUpdate( Vector& inOutForce, Vector& inOutTorque, Float timeDelta, CPhysicsWrapperInterface* wrapper )
{
    if( !m_isInitialized )
        return;

    CWorld* world = m_boatComponent->GetWorld();
    CClipMap* clipMap = world->GetTerrain();

    if( world == nullptr || clipMap == nullptr || m_boatBodyRigidMesh == nullptr || (*m_boatBodyRigidMesh).Get() == nullptr || wrapper == nullptr )
    {
        return;
    }

    // Keyframed is set when it cutscene mode
    if( !world->IsWaterShaderEnabled() || wrapper->IsKinematic() )
        return;
    
    // Compute buoyancy forces
    const Matrix massGlobalPose = wrapper->GetPose();
    const Float bbExtentsZ = (*m_boatBodyRigidMesh)->GetMeshNow()->GetBoundingBox().CalcExtents().Z;

    Float averageHeightZ = 0.0f;

    // Update drowning time
    if( m_isDrowning )
    {
        if( IsXYLocked() )
        {
            LockXYMovement( false );
        }

        m_drowningTimeSoFar += timeDelta;
    }

    m_noBuoyantsOnTerrain = 0;

    for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i)
    {
        SWaterContactPoint& point = m_buoyancyPoints[i];
        const Float spreadScaler = i < 2 ? BoatConfig::cvBoatBuoyancyPointsSpreadY.Get() : BoatConfig::cvBoatBuoyancyPointsSpreadX.Get();

        // Get global pose
        const Vector globalPos = massGlobalPose.TransformPoint( point.m_localOffset * spreadScaler );

        // Get water level
        const Float waterLevel = world->GetWaterLevel( globalPos, (Uint32)BoatConfig::cvBoatBuoyancyWaterProbingPrecision.Get() );
        Float terrainHeight = -10.0f;

        // Get terrain height
        if( clipMap->GetControlmapValueForWorldPosition( globalPos ) > 0 )  // Check for holes in clip map (eg caves)
        {
            clipMap->GetHeightForWorldPosition( globalPos, terrainHeight );
        }

        // Save this step global position and water level, used in scripts and for angles computation;
        point.m_globalPositionWaterOnW = globalPos;
        point.m_globalPositionWaterOnW.W = waterLevel;

        // Set drowning time and offset
        // Stop drowning if buoyancy point is below terrain level
        if( m_isDrowning && point.m_drowningTimeout <= m_drowningTimeSoFar/* &&*/)
        {
            const Float time = m_drowningTimeSoFar - point.m_drowningTimeout;
            point.m_drowningOffsetZ = -BoatConfig::cvBoatDrowningAcceleration.Get() * time * time * 0.5f;

            point.m_floatingHeight = waterLevel + BoatConfig::cvBoatFloatingHeight.Get() + point.m_drowningOffsetZ; // Add floating height and drowning offset
            if(  point.m_floatingHeight < terrainHeight  )
            {
                point.m_floatingHeight = terrainHeight;
            }

            point.m_isInAir = false;
        }
        else
        {
            point.m_floatingHeight = waterLevel + BoatConfig::cvBoatFloatingHeight.Get();

            // Check if point is in air
            point.m_isInAir = ( point.m_globalPositionWaterOnW.Z - waterLevel ) > ( bbExtentsZ * BoatConfig::cvBoatBuoyancyFlyThresholdScaller.Get() ) ? true : false;

            // If water level at any buoyancy point is lower than terrain height
            if( waterLevel < terrainHeight )
            {
                ++m_noBuoyantsOnTerrain;
            }
        }
        
        // Sum avg floating height
        averageHeightZ += point.m_floatingHeight;

#ifndef FINAL
        switch(i)
        {
        case 0:
            DBG_pointFTerrainHeight = terrainHeight;
            DBG_pointFWaterHeight   = waterLevel;
            break;
        case 1:
            DBG_pointBTerrainHeight = terrainHeight;
            DBG_pointBWaterHeight   = waterLevel;
            break;
        case 2:
            DBG_pointRTerrainHeight = terrainHeight;
            DBG_pointRWaterHeight   = waterLevel;
            break;
        case 3:
            DBG_pointLTerrainHeight = terrainHeight;
            DBG_pointLWaterHeight   = waterLevel;
            break;
        }
#endif
    }
    
    //////////////////////////////////////////////////////////////////////////
    Vector globalA( m_buoyancyPoints[0].m_globalPositionWaterOnW );
    globalA.Z = m_buoyancyPoints[0].m_floatingHeight;

    Vector globalB( m_buoyancyPoints[1].m_globalPositionWaterOnW );
    globalB.Z = m_buoyancyPoints[1].m_floatingHeight;

    const Vector tiltX = globalA - globalB;

    // Fix desired angle
    const Float xTiltLimitRad = DEG2RAD(BoatConfig::cvBoatTiltXLimit.Get());
    Float xAngleDesired = asinf( tiltX.Z / tiltX.Mag3() + ( xTiltLimitRad * m_tiltAnglePerc.X ) );
    if( fabsf( xAngleDesired ) > xTiltLimitRad )
        xAngleDesired = Clamp<Float>( xAngleDesired, -xTiltLimitRad, xTiltLimitRad );

    //////////////////////////////////////////////////////////////////////////
    globalA = m_buoyancyPoints[2].m_globalPositionWaterOnW;
    globalA.Z = m_buoyancyPoints[2].m_floatingHeight;

    globalB = m_buoyancyPoints[3].m_globalPositionWaterOnW;
    globalB.Z = m_buoyancyPoints[3].m_floatingHeight;

    const Vector tiltY = globalA - globalB;

    // Add tilt from turning
    const Float yTiltLimitRad = DEG2RAD(BoatConfig::cvBoatTiltYLimit.Get());
    Float yAngleDesired = asinf( tiltY.Z / tiltY.Mag3() ) + ( yTiltLimitRad * m_tiltAnglePerc.Y );

    // Fix desired angle
    if( fabsf( yAngleDesired ) > yTiltLimitRad )
        yAngleDesired = Clamp<Float>( yAngleDesired, -yTiltLimitRad, yTiltLimitRad );

    // Get current tilts
    const Float xAngleCurr = asinf( massGlobalPose.V[1].Z );
    const Float yAngleCurr = asinf( massGlobalPose.V[0].Z );

    //////////////////////////////////////////////////////////////////////////
    // Compute torque
    const Vector inertiaTensor = wrapper->GetLocalInertiaTensor();

    // Over X axis
    Float difference = xAngleDesired - xAngleCurr;

    //////////////////////////////////////////////////////////////////////////
    // Scale buoyancy X torque     
    if( difference > 0 )
    {

        const Float pow = powf(BoatConfig::cvBoatTorqueUpBase.Get(), difference * BoatConfig::cvBoatTorqueUpExpo.Get()) - 1.0f;

        if( IsFinite(pow) )
        {
#ifndef FINAL
            DBG_powTorqueX = pow;
            DBG_torqueX = difference;
#endif
            difference = pow;
        }
    }
    else if( difference < 0)
    {
        Float pow = 0;

        if( m_buoyancyPoints[0].m_isInAir )
            pow = powf(BoatConfig::cvBoatTorqueDownInAirBase.Get(), -difference* BoatConfig::cvBoatTorqueDownInAirExpo.Get() ) - 1.0f;
        else
            pow = powf(BoatConfig::cvBoatTorqueDownNormBase.Get(), -difference * BoatConfig::cvBoatTorqueDownNormExpo.Get() ) - 1.0f;

        if( IsFinite(pow) )
        {
#ifndef FINAL
            DBG_powTorqueX = -pow;
            DBG_torqueX = difference;
#endif
            difference = -pow;
        }
    }

    Vector torqueToApply = massGlobalPose.GetAxisX().Normalized2() * difference * inertiaTensor.X;

#ifndef FINAL
    DBG_buoyancyTorqueX = massGlobalPose.GetAxisX().Normalized2() * difference;
    DBG_buoyancyTorqueX.Z = 0.0f;
#endif

    // Over Y axis
    difference = yAngleDesired - yAngleCurr;
    torqueToApply -= massGlobalPose.GetAxisY().Normalized2() * difference * inertiaTensor.Y;
    torqueToApply *= BoatConfig::cvBoatTorqueScaller.Get();
    torqueToApply.Z = 0.0f;

#ifndef FINAL
    DBG_buoyancyTorqueY = massGlobalPose.GetAxisY().Normalized2() * difference;
    DBG_buoyancyTorqueY.Z = 0.0f;
#endif

    //////////////////////////////////////////////////////////////////////////
    // Apply buoyancy forces
    averageHeightZ *= 0.25f; // Get average height
    difference = averageHeightZ - massGlobalPose.GetTranslation().Z;    

    //////////////////////////////////////////////////////////////////////////
    // Increase boat damping to prevent any oscilations
    static Bool wasSet = false;
    static Float scale = 2.0f;
    if( Abs(difference) / BoatConfig::cvBoatBuoyancyZDiffLimit.Get() > scale && !wasSet )
    {
        wrapper->SetDampingLinear( wrapper->GetDampingLinear() * scale*2.0f );
        m_increasedDampingTimeout = scale*2.0f;
        wasSet = true;
    }
    if( wasSet && m_increasedDampingTimeout < 0.0f )
    {
        wrapper->SetDampingLinear( wrapper->GetDampingLinear() / (scale*2.0f));
        m_increasedDampingTimeout = 0.0f;
        wasSet = false;
    }

    if( m_increasedDampingTimeout > 0.0f )
    {
        m_increasedDampingTimeout -= timeDelta;
    }
    //////////////////////////////////////////////////////////////////////////

    // Limit distance to prevent force from escalating infinitively
    difference = Clamp<Float>( difference, -BoatConfig::cvBoatBuoyancyZDiffLimit.Get(), BoatConfig::cvBoatBuoyancyZDiffLimit.Get() );

	Bool resetPID( false );
	if ( resetPID )
	{
		m_pid.Reset( 0.f );
	}

	m_pid.m_kp = BoatConfig::cvBoatPID_P.Get();
	m_pid.m_ki = BoatConfig::cvBoatPID_I.Get();
	m_pid.m_kd = BoatConfig::cvBoatPID_D.Get();

	const Float corr = m_pid.Update( difference, timeDelta );

	Float forceToAdd( 0.f );
    if( difference > 0 )
    {
        const Float pow = powf(BoatConfig::cvBoatForceDownBase.Get() * difference, BoatConfig::cvBoatForceDownExpo.Get());

        if( IsFinite(pow) )
        {
#ifndef FINAL
            DBG_powForceZ = pow;
            DBG_forceZ = difference;
#endif
            forceToAdd = pow;
        }
    }
    else if( difference < 0 )
    {
        const Float pow = powf(BoatConfig::cvBoatForceUpBase.Get() * -(difference), BoatConfig::cvBoatForceUpExpo.Get() );

        if( IsFinite(pow) )
        {
#ifndef FINAL
            DBG_powForceZ = -pow;
            DBG_forceZ = difference;
#endif
            forceToAdd = -pow;
        }
    }

	const Float forceToAddScaled = forceToAdd * BoatConfig::cvBoatForceScaller.Get();
	const Float froceClamp = BoatConfig::cvBoatForceMax.Get();
	const Float forceToAddFinal = Clamp( corr, -froceClamp, froceClamp );

    const Vector buoyancyForce( 0.f, 0.f, forceToAddFinal );

    inOutForce += buoyancyForce;
    inOutTorque += torqueToApply;

    //////////////////////////////////////////////////////////////////////////
    // Move kinematic constrainer so that it would follow the boat
    // Update constrainer pose in first tick so that it's position match the boat entity's pose
    if( !m_isXYLocked || m_firstTick )
    {
        UpdateConstrainerPose(wrapper);
    }

    // Do teleport in first tick to avoid boat bouncing
    if( m_firstTick )
    {
        inOutForce -= buoyancyForce;
        inOutTorque -= torqueToApply;

        // Align around Z axis
        Matrix trans = massGlobalPose;
        trans.V[0].Normalize2();
        trans.V[0].Z = 0.0f;
        trans.V[1].Normalize2();
        trans.V[1].Z = 0.0f;
        trans.V[2] = Vector::EZ;

        trans.V[3].Z = averageHeightZ;
        
        // Suspend boat component tick
        m_boatComponent->StartTeleport();
        m_firstTick = false;

        // Do teleport
        m_boatComponent->GetEntity()->Teleport( trans.GetTranslation(), trans.ToEulerAnglesFull() );
    }

#ifndef FINAL
    DBG_massGlobalPose  = massGlobalPose;
    DBG_angleX          = xAngleCurr;
    DBG_angleY          = yAngleCurr;
    DBG_desiredAngleX   = xAngleDesired;
    DBG_desiredAngleY   = yAngleDesired;
    DBG_avgHeight       = averageHeightZ;
    DBG_buoyancyForce   = buoyancyForce;
#endif
    // DEBUG
    //////////////////////////////////////////////////////////////////////////
}

//////////////////////////////////////////////////////////////////////////

void CBoatBuoyancySystem::UpdateConstrainerPose( CPhysicsWrapperInterface* wrapper )
{
    if( wrapper == nullptr )
    {
        return;
    }

#ifdef USE_PHYSX
    // Remove rotation around X and Y axis
    Matrix boatPose = wrapper->GetPose();
    boatPose.V[2].Set3( 0.0f, 0.0f, 1.0f );                                      // Z
    boatPose.V[1] = Vector::Cross( boatPose.V[2], boatPose.V[0] ).Normalized3(); // Y = ( Z.cross(X) ).Normalized();
    boatPose.V[0] = Vector::Cross( boatPose.V[1], boatPose.V[2] );               // X = Y.cross(Z)

    // Set pose to 
    const Vector globalPosition = boatPose.GetTranslation() - Vector( 0, 0, KINEMATIC_CONSTRAINER_RADIUS );
    PxTransform constrainerPose;
    constrainerPose.p = TO_PX_VECTOR( globalPosition );
    constrainerPose.q = TO_PX_QUAT( boatPose.ToQuat() );

    m_dynamicConstrainer->setKinematicTarget( constrainerPose );

    #ifndef FINAL
    DBG_constrainerGlobalPos = globalPosition;
    #endif

#endif
}

//////////////////////////////////////////////////////////////////////////

void CBoatBuoyancySystem::DisableJointTiltLimit()
{
    // Unlock joint limits to avoid unnecessary oscillation
#ifdef USE_PHYSX
    if( m_sixDofJoint != nullptr )
    {
        m_sixDofJoint->setMotion( PxD6Axis::eX,      PxD6Motion::eFREE );
        m_sixDofJoint->setMotion( PxD6Axis::eY,      PxD6Motion::eFREE );
    }
#endif
}

//////////////////////////////////////////////////////////////////////////

#ifndef FINAL
void CBoatBuoyancySystem::DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset )
{
    if( !m_isInitialized )
        return;

    CWorld* world = m_boatComponent->GetWorld();
    if( world == nullptr )
        return;
    
    //////////////////////////////////////////////////////////////////////////
    // Draw texts
    ++inOutTestOffset;
    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Increased damping timeout: %f"), m_increasedDampingTimeout )
        ,   inOutTestOffset 
        ,   true
        );
    
    frame->AddDebugSphere( DBG_constrainerGlobalPos, KINEMATIC_CONSTRAINER_RADIUS, Matrix::IDENTITY, Color::DARK_GREEN );

    const Vector offset( 0, 0, 1 );

    //////////////////////////////////////////////////////////////////////////
    // Draw tilts and offsets
    const Float hLength = m_buoyancyPoints[0].m_localOffset.Y;
    const Float hWidth = m_buoyancyPoints[2].m_localOffset.X;

    Vector pos = DBG_massGlobalPose.GetTranslation() + offset - Vector( 0, 0, BoatConfig::cvBoatFloatingHeight.Get() );

    // Draw applied torque
    Float scale = 0.0f;
    const Float scaller = 10.0f;

    //////////////////////////////////////////////////////////////////////////
    scale = DBG_buoyancyTorqueX.Normalize2();
    scale = DBG_torqueX * scaller;
    //scale = log10f( 1.0f + scale );
    //if ( fabsf( scale ) > FLT_EPSILON )
    frame->AddDebug3DArrow( pos, DBG_buoyancyTorqueX, scale, 0.03f, 0.04f, 0.15f, Color::DARK_CYAN, Color::DARK_CYAN );
    scale = DBG_powTorqueX * scaller;
    frame->AddDebug3DArrow( pos, DBG_buoyancyTorqueX, scale, 0.025f, 0.035f, 0.12f, Color::LIGHT_CYAN, Color::LIGHT_CYAN );

    //////////////////////////////////////////////////////////////////////////
    scale = DBG_buoyancyTorqueY.Normalize2();
    //scale = log10f( 1.0f + scale );
    //if ( fabsf( scale ) > FLT_EPSILON )
    frame->AddDebug3DArrow( pos, DBG_buoyancyTorqueY, scale, 0.025f, 0.035f, 0.12f, Color::DARK_MAGENTA, Color::DARK_MAGENTA );

    //////////////////////////////////////////////////////////////////////////
    // Draw applied force
    scale = DBG_buoyancyForce.Normalize3();
    scale = DBG_forceZ * scaller;
    //scale /= BoatConfig::cvBoatForceScaller.Get();
    //scale = log10f( 1.0f + scale );
    //if ( fabsf( scale ) > FLT_EPSILON )
    frame->AddDebug3DArrow( pos, DBG_buoyancyForce, scale, 0.03f, 0.04f, 0.15f, Color::RED, Color::RED );
    scale = DBG_powForceZ * scaller;
    frame->AddDebug3DArrow( pos, DBG_buoyancyForce, scale, 0.025f, 0.035f, 0.12f, Color::LIGHT_RED, Color::LIGHT_RED );

    //////////////////////////////////////////////////////////////////////////
    // Current X tilt
    Vector tilt = DBG_massGlobalPose.GetAxisY();
    tilt.Z = hLength * sinf( DBG_angleX );
    tilt.Normalize2();
    tilt.X *= hLength;
    tilt.Y *= hLength;

    frame->AddDebugText( pos, String::Printf(TXT("Offset diff %.2f"), DBG_avgHeight - pos.Z + offset.Z ), 0, 0 );
    frame->AddDebugText( pos + tilt, String::Printf(TXT("Angle X diff %.2f"), RAD2DEG(DBG_angleX - DBG_desiredAngleX) ), 0, 0 );
    frame->AddDebugLine( pos, pos+tilt, Color::WHITE );
    frame->AddDebugLine( pos, pos-tilt, Color::WHITE );

    // Current Y tilt
    tilt = DBG_massGlobalPose.GetAxisX();
    tilt.Z = hWidth * sinf( DBG_angleY );
    tilt.Normalize2();
    tilt.X *= hWidth;
    tilt.Y *= hWidth;

    frame->AddDebugText( pos + tilt, String::Printf(TXT("Angle Y diff %.2f"), RAD2DEG(DBG_angleY - DBG_desiredAngleY) ), 0, 0 );
    frame->AddDebugLine( pos, pos+tilt, Color::WHITE );
    frame->AddDebugLine( pos, pos-tilt, Color::WHITE );

    //////////////////////////////////////////////////////////////////////////
    // Desired  X tilt
    pos.Z = DBG_avgHeight + offset.Z;
    tilt = -DBG_massGlobalPose.GetAxisY();
    tilt.Z = hLength * -sinf( DBG_desiredAngleX );
    tilt.Normalize2();
    tilt.X *= hLength;
    tilt.Y *= hLength;

    frame->AddDebugLine( pos, pos+tilt, Color::GREEN );
    frame->AddDebugLine( pos, pos-tilt, Color::GREEN );

    // Desired Y tilt
    tilt = -DBG_massGlobalPose.GetAxisX();
    tilt.Z = hWidth * -sinf( DBG_desiredAngleY );
    tilt.Normalize2();
    tilt.X *= hWidth;
    tilt.Y *= hWidth;

    frame->AddDebugLine( pos, pos+tilt, Color::GREEN );
    frame->AddDebugLine( pos, pos-tilt, Color::GREEN );

    //////////////////////////////////////////////////////////////////////////
    // Draw buoyancy points terrain and water heights
    //Front
    Vector globalPoint = DBG_massGlobalPose.TransformPoint( m_buoyancyPoints[0].m_localOffset * BoatConfig::cvBoatBuoyancyPointsSpreadY.Get() );
    frame->AddDebugSphere( globalPoint, 0.15f, Matrix::IDENTITY, Color::RED );
    globalPoint.Z = DBG_pointFWaterHeight;
    frame->AddDebugSphere( globalPoint, 0.2f, Matrix::IDENTITY, Color::BLUE );
    frame->AddDebugText( globalPoint, String::Printf( TXT("T: %.2f"), DBG_pointFTerrainHeight ), 0, 0 );
    frame->AddDebugText( globalPoint, String::Printf( TXT("W: %.2f"), DBG_pointFWaterHeight), 0, 1 );

    globalPoint = DBG_massGlobalPose.TransformPoint( m_buoyancyPoints[1].m_localOffset * BoatConfig::cvBoatBuoyancyPointsSpreadY.Get() );
    frame->AddDebugSphere( globalPoint, 0.15f, Matrix::IDENTITY, Color::RED );
    globalPoint.Z = DBG_pointBWaterHeight;
    frame->AddDebugSphere( globalPoint, 0.2f, Matrix::IDENTITY, Color::BLUE );
    frame->AddDebugText( globalPoint, String::Printf( TXT("T: %.2f"), DBG_pointBTerrainHeight ), 0, 0 );
    frame->AddDebugText( globalPoint, String::Printf( TXT("W: %.2f"), DBG_pointBWaterHeight), 0, 1 );

    globalPoint = DBG_massGlobalPose.TransformPoint( m_buoyancyPoints[2].m_localOffset * BoatConfig::cvBoatBuoyancyPointsSpreadX.Get() );
    frame->AddDebugSphere( globalPoint, 0.15f, Matrix::IDENTITY, Color::RED );
    globalPoint.Z = DBG_pointLWaterHeight;
    frame->AddDebugSphere( globalPoint, 0.2f, Matrix::IDENTITY, Color::BLUE );
    frame->AddDebugText( globalPoint, String::Printf( TXT("T: %.2f"), DBG_pointRTerrainHeight ), 0, 0 );
    frame->AddDebugText( globalPoint, String::Printf( TXT("W: %.2f"), DBG_pointLWaterHeight), 0, 1 );

    globalPoint = DBG_massGlobalPose.TransformPoint( m_buoyancyPoints[3].m_localOffset * BoatConfig::cvBoatBuoyancyPointsSpreadX.Get() );
    frame->AddDebugSphere( globalPoint, 0.15f, Matrix::IDENTITY, Color::RED );
    globalPoint.Z = DBG_pointLWaterHeight;
    frame->AddDebugSphere( globalPoint, 0.2f, Matrix::IDENTITY, Color::BLUE );
    frame->AddDebugText( globalPoint, String::Printf( TXT("T: %.2f"), DBG_pointLTerrainHeight ), 0, 0 );
    frame->AddDebugText( globalPoint, String::Printf( TXT("W: %.2f"), DBG_pointLWaterHeight), 0, 1 );
}
#endif

//////////////////////////////////////////////////////////////////////////

void CBoatBuoyancySystem::TriggerDrowning( const Vector& globalHitPoint )
{
    if( !m_isInitialized )
    {
        return;
    }

    if( m_boatBodyRigidMesh == nullptr || (*m_boatBodyRigidMesh).Get() == nullptr )
    {
        return;
    }

    CPhysicsWrapperInterface* boatBodyWrapper = (*m_boatBodyRigidMesh)->GetPhysicsRigidBodyWrapper();    
    if( boatBodyWrapper == nullptr )
    {
        return;
    }
    
    if( !BoatConfig::cvBoatDrowningEnabled.Get() || m_isDrowning )
    {
        return;
    }

    const Matrix invPose = boatBodyWrapper->GetPose().Inverted();
    const Vector localHitPoint = invPose.TransformPoint( globalHitPoint );

    m_drowningTimeSoFar = 0.0f;
    m_isDrowning = true;

    // Compute distance to hit point
    Float greatestDistance = NumericLimits< Float >::Min();
    Float smalestDistance = NumericLimits< Float >::Max();
    for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i)
    {
        SWaterContactPoint& point = m_buoyancyPoints[i];
        point.m_drowningOffsetZ = m_buoyancyPoints[i].m_localOffset.DistanceTo2D( localHitPoint );

        // Find greatest distance
        if( point.m_drowningOffsetZ > greatestDistance )
        {
            greatestDistance = point.m_drowningOffsetZ;
        }

        if( point.m_drowningOffsetZ < smalestDistance )
        {
            smalestDistance = point.m_drowningOffsetZ;
        }
    }

    // Set drowning timeout according to distance from hit point
    const Float devider = greatestDistance - smalestDistance;

    TStaticArray<SWaterContactPoint, MAX_WATER_CONTACT_POINTS> sortArr;

    for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i)
    {
        SWaterContactPoint& point = m_buoyancyPoints[i];
        point.m_drowningTimeout = ( ( point.m_drowningOffsetZ - smalestDistance ) / devider ) * BoatConfig::cvBoatDrowningPropagationTime.Get();
        point.m_drowningOffsetZ = 0.0f;
    }

#ifdef USE_PHYSX
    // Unlock joint limits to avoid unnecesary oscillation
    if( m_sixDofJoint != nullptr )
    {
        m_sixDofJoint->setMotion( PxD6Axis::eX,      PxD6Motion::eFREE );
        m_sixDofJoint->setMotion( PxD6Axis::eY,      PxD6Motion::eFREE );
    }
#endif
}

//////////////////////////////////////////////////////////////////////////

void CBoatBuoyancySystem::LockXYMovement( Bool lock )
{
#ifdef USE_PHYSX
    if( m_sixDofJoint == nullptr )
    {
        return;
    }

    if( lock )
    {
        static Float LINEAR_STIFFNESS   = 300.0f;
        static Float LINEAR_DAMPING     = 0.2f;
        static Float LINEAR_MAX_EXTENT  = 0.001f;

        static Float ANGULAR_STIFFNESS  = 300.0f;
        static Float ANGULAR_DAMPING    = 0.2f;
        static Float ANGULAR_MAX_ANGLE  = 0.001f;

        // Limit X, Y movement and Z rotation
        m_sixDofJoint->setMotion( PxD6Axis::eX,      PxD6Motion::eLIMITED );
        m_sixDofJoint->setMotion( PxD6Axis::eY,      PxD6Motion::eLIMITED );
        m_sixDofJoint->setMotion( PxD6Axis::eSWING2, PxD6Motion::eLIMITED );

        // Add spring linear limit
        const PxSpring linearSpring( LINEAR_STIFFNESS, LINEAR_DAMPING );
        const PxJointLinearLimit linearLimit( LINEAR_MAX_EXTENT, linearSpring );
        m_sixDofJoint->setLinearLimit( linearLimit );

        // Add spring angular limit
        const PxSpring angularSpring( ANGULAR_STIFFNESS, ANGULAR_DAMPING );
        const PxJointLimitCone coneLimit( 3.14f, ANGULAR_MAX_ANGLE, linearSpring );
        m_sixDofJoint->setSwingLimit( coneLimit );

        m_isXYLocked = true;
    }
    else
    {
        // Free X, Y movement and Z rotation
        m_sixDofJoint->setMotion( PxD6Axis::eX,      PxD6Motion::eFREE );
        m_sixDofJoint->setMotion( PxD6Axis::eY,      PxD6Motion::eFREE );
        m_sixDofJoint->setMotion( PxD6Axis::eSWING2, PxD6Motion::eFREE );

        m_isXYLocked = false;
    }

#endif
}

//////////////////////////////////////////////////////////////////////////
