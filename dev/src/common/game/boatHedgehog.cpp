#include "build.h"
#include "boatHedgehog.h"
#include "boatConfig.h"
#include "boatComponent.h"
#include "gameWorld.h"
#include "..\physics\physicsWorld.h"
#include "..\physics\physicsWorldUtils.h"
#include "..\engine\renderFrame.h"
#include "..\engine\globalWaterUpdateParams.h"
#include "../physics/physicsWrapper.h"
#include "..\engine\rigidMeshComponent.h"
#include "..\engine\mesh.h"
#include "..\core\gameSave.h"
#include "boatDestructionComponent.h"

//////////////////////////////////////////////////////////////////////////

const static Float sBBOX_SCALE = 0.9f;

//////////////////////////////////////////////////////////////////////////

CBoatHedgehog::CBoatHedgehog(void)
    : m_isInitialized( false )
    , m_boatComponent( nullptr )
    , m_hBoatBodyRigidMesh( nullptr )
    , m_hBoatDestruction( nullptr )
    , m_basicWaterLevel( WATER_DEFAULT_NON_INIT_LEVEL - 666.0f )
    , m_terrainRescueMode(false)
    , m_lastWaterPose(Matrix::IDENTITY)
{
    m_raycastLengthDamper.Setup( BoatConfig::cvBoatHedgeRaycastMinLength.Get(), BoatConfig::cvBoatHedgeRaycastMinLength.Get(), 0.25f, EET_Linear, EET_InOut);
    m_repelDamper.Setup( Vector::ZEROS, Vector::ZEROS, BoatConfig::cvBoatHedgeNormalNForceDamperSpeed.Get(), EET_Linear, EET_InOut);
    m_inputDamper.Setup( 1.0f, 1.0f, BoatConfig::cvBoatHedgeInputDamperSpeed.Get(), EET_Linear, EET_InOut);

    //m_normalDamper.Setup( Vector::ZEROS, Vector::ZEROS, BoatConfig::cvBoatHedgeRepelNormalDamperSpeed.Get(), EET_Linear, EET_InOut);
}

//////////////////////////////////////////////////////////////////////////

Bool CBoatHedgehog::Initialize( CBoatComponent* boatComponent, THandle<CRigidMeshComponent>* hBoatBodyRigidMesh, THandle<CBoatDestructionComponent>* hBoatDestruction, const Box& bbox )
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

    // Create raycasts
    const Float frac = 360.0f / (Float)RAYCASTS_NUM;
    for (int i = 0; i < RAYCASTS_NUM; ++i)
    {
        const Float rad = DEG2RAD(i * frac);
        const Vector direction( cosf(rad), sinf(rad), 0 );

        SProbingRaycast raycast;
        raycast.rayDirection = direction;

        m_raycastsList.PushBack(raycast);
    }
    
    m_collisionMask  =  GPhysicEngine->GetCollisionGroupMask( CName( TXT("Boat") )     );
    m_collisionMask |=  GPhysicEngine->GetCollisionTypeBit(   CName( TXT("Terrain") )  ); // add terrin collisions
    m_collisionMask &= ~GPhysicEngine->GetCollisionTypeBit(   CName( TXT("Boat") )     );
    m_collisionMask &= ~GPhysicEngine->GetCollisionTypeBit(   CName( TXT("Ragdoll") )  );
    m_collisionMask &= ~GPhysicEngine->GetCollisionTypeBit(   CName( TXT("BoatSide") ) );
    m_collisionMask &= ~GPhysicEngine->GetCollisionTypeBit(   CName( TXT("Weapon") )   );

    m_hBoatBodyRigidMesh    = hBoatBodyRigidMesh;
    m_hBoatDestruction      = hBoatDestruction;
    m_boatComponent         = boatComponent;
    m_basicWaterLevel       = world->GetWaterLevel( boatBodyWrapper->GetPosition(), 2 );
    m_isInitialized         = true;


    // Matrix should always be Z aligned (no pith and roll) and centered on basic water level
    m_lastWaterPose = boatBodyWrapper->GetPose();
    m_lastWaterPose.V[0].Normalize2();
    m_lastWaterPose.V[0].Z = 0.0f;
    m_lastWaterPose.V[1].Normalize2();
    m_lastWaterPose.V[1].Z = 0.0f;
    m_lastWaterPose.V[2] = Vector::EZ;
    m_lastWaterPose.V[3].Z = m_basicWaterLevel;

    // Position test points in the same places as buoyant points
    const Vector bbsize = bbox.CalcExtents() * sBBOX_SCALE;        
    m_localCorners[0].Set( 0, bbsize.Y );   // (0,1)
    m_localCorners[1].Set( bbsize.X, 0 );   // (1,0)
    m_localCorners[2].Set( 0, -bbsize.Y / (sBBOX_SCALE - 0.1f) );   // Undo scaling    (-1,0)
    m_localCorners[3].Set( -bbsize.X, 0 );  // (-1,0)

    return true;
}

//////////////////////////////////////////////////////////////////////////

void CBoatHedgehog::Deinitialize()
{
    m_isInitialized = false;
    m_hBoatBodyRigidMesh = nullptr;
    m_boatComponent = nullptr;
}

//////////////////////////////////////////////////////////////////////////

void CBoatHedgehog::UpdateHedgehog( Float deltaTime, Uint32 noBuoyantsOnTerrain )
{
    if( !m_isInitialized )
    {
        return;
    }
    
    if( m_hBoatBodyRigidMesh == nullptr || (*m_hBoatBodyRigidMesh).Get() == nullptr || m_hBoatDestruction == nullptr || (*m_hBoatDestruction).Get() == nullptr)
    {
        return;
    }

    CPhysicsWrapperInterface* wrapper = (*m_hBoatBodyRigidMesh)->GetPhysicsRigidBodyWrapper();
    if( wrapper == nullptr )
    {
        return;
    }

    CMesh* mesh = (*m_hBoatBodyRigidMesh)->GetMeshNow();
    if( mesh == nullptr )
    {
        return;
    }
    
    CWorld* world = m_boatComponent->GetWorld();
    if( world == nullptr )
    {
        return;
    }

    CPhysicsWorld* physWorld = nullptr;
    if( !world->GetPhysicsWorld(physWorld))
    {
        return;
    }
        
    m_currentPose = wrapper->GetPose();
    Vector discretGlobalPosition = wrapper->GetPosition();
    discretGlobalPosition.Z = m_basicWaterLevel;

    Matrix boatRotation = m_currentPose;
    boatRotation.SetTranslation( Vector::ZEROS );

    Float inputTargetValue = m_inputDamper.GetDestValue();



    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Terrain rescue mode
    if( !m_terrainRescueMode && noBuoyantsOnTerrain != 0 )
    {
        m_terrainRescueMode = true;
    }

    if( m_terrainRescueMode )
    {
        PC_SCOPE( BoatHedgehogIsNotOnWater );
        
        // Move pose away from shore
        const Vector repelDirection = m_repelDamper.GetDestValue().Normalized2();
        
        // Compute target pose
        static Float sMOVEMENT_SPEED_SCALLER = 0.666f;
        static Float sROTATION_SPEED_SCALLER = 66.6f;

        Matrix targetPose = m_lastWaterPose;
        targetPose.SetTranslation( Vector::ZEROS );

        // Get rotation direction
        Float dotDiff = Vector::Dot2( repelDirection, m_lastWaterPose.GetAxisY() );
        const Float rotationDir = (Float)Sgn( Vector::Cross2( repelDirection, m_lastWaterPose.GetAxisY() ) ) * (Float)Sgn(dotDiff);

        // Backward hit makes this a little bit confusing
        if( dotDiff > 0 )
        {
            dotDiff = 1.0f - dotDiff;
        }
        else
        {
            dotDiff = 1.0f + dotDiff;
        }

        // It below threshold do rotation
        static Float sDOT_DIFF_THRESHOLD = 0.85f;
        if( fabsf(dotDiff) < sDOT_DIFF_THRESHOLD )
        {
            // Rotate it over time
            const EulerAngles eulerAngles( 0, 0, rotationDir * deltaTime * sROTATION_SPEED_SCALLER );
            const Matrix rotationMatrix = eulerAngles.ToMatrix();
            targetPose = targetPose * rotationMatrix;
        }

        // Move it over time
        targetPose.SetTranslation( m_lastWaterPose.GetTranslation() - repelDirection * deltaTime * sMOVEMENT_SPEED_SCALLER );
        
        static Float sMAX_SQ_DISTANCE = 25.0f;
        const Float rescueBoxToBoatSqDist = targetPose.GetTranslation().DistanceSquaredTo2D( discretGlobalPosition );

        Bool canMoveToTarget = true;
        static Float sWATER_LEVEL_TERRAINH_DIFF = - 0.2f;

        Vector previousCorner = targetPose.TransformPoint( m_localCorners[0] );
        if( rescueBoxToBoatSqDist < sMAX_SQ_DISTANCE )
        {
            for( Uint32 i=0; i<4; ++i)
            {
                const Vector2 globalCorner = targetPose.TransformPoint( m_localCorners[i] );            
                const Vector probeStart = globalCorner + Vector( 0, 0, m_currentPose.GetTranslation().Z + 5.0f );
                const Vector probeEnd = globalCorner + Vector( 0, 0, m_basicWaterLevel + sWATER_LEVEL_TERRAINH_DIFF );

                SPhysicsContactInfo hit;

                if( TRV_Hit == physWorld->RayCastWithSingleResult( probeStart, probeEnd, m_collisionMask, 0, hit ) )
                {
                    canMoveToTarget = false;
                    break;
                }

                // Do raycast between previous and current corner
                if( i != 0 )
                {
                    SPhysicsContactInfo hitInfo;
                    if( TRV_Hit == physWorld->RayCastWithSingleResult( previousCorner, globalCorner, m_collisionMask, 0, hitInfo ) )
                    {
                        canMoveToTarget = false;
                        break;
                    }

                    previousCorner = globalCorner;
                }
                
                // If we re on last corner
                // also do raycast between last and first to complete the circle
                if( i == 3 )
                {
                    SPhysicsContactInfo hitInfo;
                    if( TRV_Hit == physWorld->RayCastWithSingleResult( globalCorner, targetPose.TransformPoint( m_localCorners[0] ), m_collisionMask, 0, hitInfo ) )
                    {
                        canMoveToTarget = false;
                        break;
                    }

                    previousCorner = globalCorner;
                }
            }   // For all corners
        } // If is not far enough

        // Swap matrices
        if( canMoveToTarget )
        {
            m_lastWaterPose = targetPose;
        }
        

        // Move back to previous on water position
        Matrix lastWaterPose = m_lastWaterPose;
        lastWaterPose.V[3].Z = m_basicWaterLevel;

        const Vector positionDiff = lastWaterPose.GetTranslation() - discretGlobalPosition;
        const Float rotationDot = Vector::Dot2( lastWaterPose.GetAxisY(), m_currentPose.GetAxisY() );
        const Float sqMag = positionDiff.SquareMag2();

        static Float sDOT_THRESHOLD = 0.85f;
        static Float sDIST_SQ_THRESHOLD = 0.09f;

        if( sqMag <= sDIST_SQ_THRESHOLD && rotationDot > sDOT_THRESHOLD )   // And continue pushing off the shore
        {
            m_terrainRescueMode = false;
        }
        else
        {
            const Vector rotationVec = Vector::Cross( lastWaterPose.GetAxisY().AsVector2(), m_currentPose.GetAxisY().AsVector2() ) * (Float)Sgn(rotationDot);
        
            Int32 gear = m_boatComponent->GetCurrentGear();
            Float currentSpeedRatio = 1.0f;
            
            if( gear != 0 )
            {
                if( gear < 2 )
                {
                    gear = 2;
                }

                currentSpeedRatio = m_boatComponent->m_sailSpeeds[gear-1].maxSpeed;
                currentSpeedRatio /= m_boatComponent->m_sailSpeeds[MAX_GEARS-1].maxSpeed;
            }

            const Float moveMultiplier = BoatConfig::cvBoatSailingForceScaller.Get() * BoatConfig::cvBoatHedgeRepelForceMulti.Get() * currentSpeedRatio;
            const Float rotationMultiplier = -BoatConfig::cvBoatHedgeRepelTorqueMulti.Get() * currentSpeedRatio;

            wrapper->ApplyImpulse( positionDiff.Normalized3() * deltaTime * moveMultiplier, wrapper->GetCenterOfMassPosition() );
            wrapper->ApplyTorqueImpulse(rotationVec * deltaTime * rotationMultiplier);
        
            // Zero input damper
            if( m_inputDamper.GetDestValue() != 0.0f )
            {
                m_inputDamper.SetStartValue( inputTargetValue );
                m_inputDamper.SetDestValue( -0.1f );
                m_inputDamper.SetInterpolationTime( 1.0f );
            }

            return;
        }
    }





    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Hedgehog mode

    Vector linearVelocity;
    Vector angularVelocity;
    wrapper->GetVelocity(linearVelocity,angularVelocity);

    // Compute speed dependent raycast length    
    Float raycastMaxLength = 
        BoatConfig::cvBoatHedgeRaycastMinLength.Get() + 
        Clamp<Float>( m_boatComponent->GetMaxVelocityAtCurrentGear() / BoatConfig::cvBoatSailingGearThreeMaxSpeed.Get(), 0.0f, 1.0f ) * 
        ( BoatConfig::cvBoatHedgeRaycastMaxLength.Get() - BoatConfig::cvBoatHedgeRaycastMinLength.Get() );

    if( m_raycastLengthDamper.GetDestValue() != raycastMaxLength )
    {
        m_raycastLengthDamper.SetStartValue( m_raycastLengthDamper.GetInterpolatedValue() );
        m_raycastLengthDamper.SetDestValue( raycastMaxLength );
        m_raycastLengthDamper.ResetInterpolationTime();
    }

    raycastMaxLength = m_raycastLengthDamper.Update(deltaTime);

    const Vector bboxExtents = mesh->GetBoundingBox().CalcExtents();
    Float shortest = raycastMaxLength + 1.0f;
    Uint32 numHits = 0;

    {   // PC_SCOPE( BoatHedgehogRaycasts );
        PC_SCOPE( BoatHedgehogRaycasts );

        // Gather raycast hits
        for(Uint32 i=0; i<RAYCASTS_NUM; ++i)
        {
            SProbingRaycast& probingRay = m_raycastsList[i];
        
            Vector boatBorder = boatRotation.Inverted().TransformPoint( probingRay.rayDirection + Vector( 0, BoatConfig::cvBoatHedgeRayOffsetY.Get(), 0 ) );
            boatBorder.X *= bboxExtents.X * BoatConfig::cvBoatHedgeRayScaleX.Get();
            boatBorder.Y *= bboxExtents.Y * BoatConfig::cvBoatHedgeRayScaleY.Get();
            boatBorder = boatRotation.TransformPoint( boatBorder );

#ifndef FINAL
            probingRay.DBG_localRayStartPos = boatBorder;
#endif

            SPhysicsContactInfo hitInfo;

            if( TRV_Hit == physWorld->RayCastWithSingleResult( boatBorder+discretGlobalPosition, boatBorder+discretGlobalPosition+probingRay.rayDirection*raycastMaxLength, m_collisionMask, 0, hitInfo ) )
            {
//                 // Get what is colliding here
//                 CPhysicsEngine::CollisionMask mask = hitInfo.m_userDataA->GetCollisionTypesBits( hitInfo.m_rigidBodyIndexA.m_actorIndex, hitInfo.m_rigidBodyIndexA.m_shapeIndex );
// 
//                 if( false )
//                 {
//                     m_collisionMask &= ~GPhysicEngine->GetCollisionTypeBit(collisionGroup);
//                 }

                //probingRay.globalHitNormal = hitInfo.m_normal;
                probingRay.globalHitPos = hitInfo.m_position;

                Vector dir = hitInfo.m_position - (boatBorder + discretGlobalPosition);
                probingRay.hitDistance = dir.Mag3();
                if (probingRay.hitDistance < shortest)
                {
                    shortest = probingRay.hitDistance;
                }

                ++numHits;
            }
            else
            {
                probingRay.globalHitPos.Set3(0, 0, 0);
                //probingRay.globalHitNormal.Set3(0, 0, 0);
                probingRay.hitDistance = -1.0f;
            }
        }
    }   // PC_SCOPE( BoatHedgehogRaycasts );

    Bool canGatherPose = true;

    // Process hits
    if (numHits > 0)
    {
        if( shortest - RAY_DISTANCE_EPSILON < 0.0f )
        {
            shortest = RAY_DISTANCE_EPSILON;
        }

        Vector totalRepelVec( 0, 0, 0 );
        Vector destructionHitPosition( 0, 0, 0 );
        //Vector totalShoreNormal(0, 0, 0);

        for(Uint32 i=0; i<RAYCASTS_NUM; ++i)
        {
            SProbingRaycast& raycastInfo = m_raycastsList[i];

            if( raycastInfo.hitDistance <= 0.0f )
            {
                continue;
            }

            if( raycastInfo.hitDistance - RAY_DISTANCE_EPSILON < 0.0f )
            {
                raycastInfo.hitDistance = RAY_DISTANCE_EPSILON;
            }

            const Float fraction = shortest / raycastInfo.hitDistance;

            //totalShoreNormal += raycastInfo.globalHitNormal;
            totalRepelVec += raycastInfo.rayDirection * fraction;

            // Save shortest ray hit pos for destruction system
            if( fraction + RAY_DISTANCE_EPSILON > 1.0f )
            {
                destructionHitPosition = raycastInfo.globalHitPos;
            }
        }

        //totalShoreNormal /= (Float)numHits;
        //totalShoreNormal.Normalize3();

        totalRepelVec /= (Float)numHits;
        totalRepelVec.Z = 0;
        
        // Set new dampers target
        if( !Vector::Equal2(totalRepelVec, m_repelDamper.GetDestValue() ) )
        {
            m_repelDamper.SetStartValue( m_repelDamper.GetInterpolatedValue() );
            m_repelDamper.SetDestValue( totalRepelVec );
            m_repelDamper.ResetInterpolationTime();
        }

//         if( !Vector::Equal3(totalShoreNormal, m_normalDamper.GetDestValue() ) )
//         {
//             m_normalDamper.SetStartValue( m_normalDamper.GetInterpolatedValue() );
//             m_normalDamper.SetDestValue( totalShoreNormal );
//             m_normalDamper.ResetInterpolationTime();
//         }

        // Damp
        m_smoothRepelVec = m_repelDamper.Update(deltaTime);
        //m_smoothShoreNormal = m_normalDamper.Update(deltaTime);

        // get rotation direction
        Vector rotationVec = Vector::Cross( m_smoothRepelVec, m_currentPose.GetAxisY() ).Normalized3();
        Float rotationDot = Vector::Dot3(m_smoothRepelVec, m_currentPose.GetAxisY());
        const Float velocityDot = Vector::Dot2( m_smoothRepelVec.Normalized2(), linearVelocity.Normalized2() );

        // Ignore hits that are on boat side opposite to movement vector
        Bool ignoreBackwardHit = false;
        const Vector localVelocity = boatRotation.Inverted().TransformPoint( linearVelocity );
        if ((rotationDot < 0.0f && localVelocity.Y > 0.0f) || (rotationDot > 0.0f && localVelocity.Y < 0.0f))
        {
            ignoreBackwardHit = true;
        }

        // Square dote change
        rotationDot = -powf(rotationDot - 1.0f, 2) + 1;

        // Rotate only around global Z
        rotationVec.X = 0.0f;
        rotationVec.Y = 0.0f;
        rotationVec.Z *= rotationDot; // Use squared dot

        // Change amount of input influence
        static Float sINPUT_CUTOFF_ANGLE = 0.5f;
        if( !ignoreBackwardHit && Abs(velocityDot) > sINPUT_CUTOFF_ANGLE )
        {
            inputTargetValue = 1.0f - ( ( Abs(velocityDot) - sINPUT_CUTOFF_ANGLE ) / ( 1.0f - sINPUT_CUTOFF_ANGLE ) );
        }
        else
        {
            inputTargetValue = 1.0f;
        }
        
        //const Float shoreAngle = RAD2DEG( acosf( Vector::Dot3(m_smoothShoreNormal.Normalized3(), Vector::EZ) ) );
        const Float rayCutoffLengthRatio = Clamp<Float>( BoatConfig::cvBoatHedgeRaycastCutoffLength.Get() / raycastMaxLength, 0.0f, 1.0f );
        Float rayLengthRatio = Clamp<Float>( 1 - shortest / raycastMaxLength, 0.0f, 1.0f);

        if (rayLengthRatio + rayCutoffLengthRatio >= 1.0f) // hard
        {
            canGatherPose = false; // do not gather boat pose when hard repel is triggered
            rayLengthRatio = powf( rayLengthRatio + rayCutoffLengthRatio, BoatConfig::cvBoatHedgeHardRepelPower.Get() );

        }
        else // smooth
        {
            rayLengthRatio = powf( rayLengthRatio + rayCutoffLengthRatio, BoatConfig::cvBoatHedgeSmoothRepelPower.Get() );
        }

        //////////////////////////////////////////////////////////////////////////
        // Trigger destruction
        static Float sDESTRUCTION_RAY_LENGTH_RATIO = 0.5f;
        static Float sDESTRUCTION_RAY_VELOCITY_ANGLE = 0.5f;        
        if( rayLengthRatio > sDESTRUCTION_RAY_LENGTH_RATIO && velocityDot > sDESTRUCTION_RAY_VELOCITY_ANGLE )
        {
            // Notify destruction about collision with terrain or other obstacle
            (*m_hBoatDestruction)->GlobalSpaceExternalCollision( destructionHitPosition, m_smoothRepelVec, EBCS_Hedgehog );
        }

        static Float sCLAMP_RATIO = 1.0f;
        rayLengthRatio = Clamp<Float>(rayLengthRatio, -sCLAMP_RATIO, sCLAMP_RATIO);

        const Int32 gear = m_boatComponent->GetCurrentGear();
        Float currentSpeedRatio = 1.0f;
        if( gear < 0 )
        {
            currentSpeedRatio = m_boatComponent->m_sailSpeeds[0].maxSpeed;
        }
        else if( gear > 0 )
        {
            currentSpeedRatio = m_boatComponent->m_sailSpeeds[gear-1].maxSpeed;
        }
        currentSpeedRatio /= m_boatComponent->m_sailSpeeds[MAX_GEARS-1].maxSpeed;
        
        const Float moveMultiplier = rayLengthRatio * BoatConfig::cvBoatSailingForceScaller.Get() * BoatConfig::cvBoatHedgeRepelForceMulti.Get() * currentSpeedRatio;
        const Float rotationMultiplier = rayLengthRatio * BoatConfig::cvBoatHedgeRepelTorqueMulti.Get() * currentSpeedRatio;

        wrapper->ApplyImpulse(-m_smoothRepelVec * deltaTime * moveMultiplier, wrapper->GetCenterOfMassPosition() );
        wrapper->ApplyTorqueImpulse(rotationVec * deltaTime * rotationMultiplier);
    }
    else // no hits at all
    {
        inputTargetValue = 1.0f;

        if( !Vector::Equal2(Vector::ZEROS, m_repelDamper.GetDestValue() ) )
        {
            m_repelDamper.SetStartValue( m_repelDamper.GetInterpolatedValue() );
            m_repelDamper.SetDestValue( Vector::ZEROS );
            m_repelDamper.ResetInterpolationTime();
        }

//         if( !Vector::Equal2(Vector::ZEROS, m_normalDamper.GetDestValue() ) )
//         {
//             m_normalDamper.SetStartValue( m_normalDamper.GetInterpolatedValue() );
//             m_normalDamper.SetDestValue( Vector::ZEROS );
//             m_normalDamper.ResetInterpolationTime();
//         }

        // damp to target
        m_smoothRepelVec = m_repelDamper.Update(deltaTime);
        //m_smoothShoreNormal = m_normalDamper.Update(deltaTime);
    }

    // Gather pose
    if( noBuoyantsOnTerrain == 0 && canGatherPose )
    {
        m_lastWaterPose = wrapper->GetPose();

        // Matrix should always be Z aligned (no pith and roll) and centered on basic water level
        m_lastWaterPose.V[0].Normalize2();
        m_lastWaterPose.V[0].Z = 0.0f;
        m_lastWaterPose.V[1].Normalize2();
        m_lastWaterPose.V[1].Z = 0.0f;
        m_lastWaterPose.V[2] = Vector::EZ;
        m_lastWaterPose.V[3].Z = m_basicWaterLevel;
    }

    // Update input damper
    if( inputTargetValue != m_inputDamper.GetDestValue() )
    {
        if( inputTargetValue <= m_inputDamper.GetDestValue() )
        {
            m_inputDamper.SetDampingTime( BoatConfig::cvBoatHedgeInputDamperSpeed.Get() );
        }
        else
        {
            static Float sDAMPING_TIME = 1.0f;
            m_inputDamper.SetDampingTime( sDAMPING_TIME );
        }

        m_inputDamper.SetStartValue( m_inputDamper.GetInterpolatedValue() );
        m_inputDamper.SetDestValue( inputTargetValue );
        m_inputDamper.ResetInterpolationTime();
    }

    m_inputDamper.Update(deltaTime);
}

//////////////////////////////////////////////////////////////////////////

void CBoatHedgehog::RestoreSerializedState( SBoatSerialization* serializationStruct )
{
    if( serializationStruct == nullptr )
    {
        ASSERT(serializationStruct)
        return;
    }

    m_lastWaterPose = serializationStruct->hedgeLastWaterPose;
    m_terrainRescueMode = serializationStruct->hedgeTerrainRescueMode;

    const Uint32 numRays = Min( m_raycastsList.Size(), serializationStruct->hedgeRaycastsDistances.Size() );
    for( Uint32 i=0; i<numRays; ++i )
    {
        m_raycastsList[i].hitDistance = serializationStruct->hedgeRaycastsDistances[i];
    }

    m_repelDamper = serializationStruct->hedgeRepelDamper;
    m_inputDamper = serializationStruct->hedgeInputDamper;
    m_raycastLengthDamper = serializationStruct->hedgeRaycastLengthDamper;
}

//////////////////////////////////////////////////////////////////////////

void CBoatHedgehog::OnDebugPageClosed()
{
    //m_normalDamper.SetDampingTime( BoatConfig::cvBoatHedgeRepelNormalDamperSpeed.Get() );
    m_repelDamper.SetDampingTime( BoatConfig::cvBoatHedgeNormalNForceDamperSpeed.Get() );
    m_inputDamper.SetDampingTime( BoatConfig::cvBoatHedgeInputDamperSpeed.Get() );
}

//////////////////////////////////////////////////////////////////////////

void CBoatHedgehog::DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset )
{
    const Vector globalPos = m_currentPose.GetTranslation() + Vector(0,0,0.3f);

    ++inOutTestOffset;
    ++inOutTestOffset;
    frame->AddDebugScreenText( 10, 120
        ,   String::Printf( TXT("Input multiplier: %.4f"), m_inputDamper.GetInterpolatedValue() )
        ,   inOutTestOffset
        ,   true
        );

    // Draw last saved water position
    for( Uint32 i=0; i<4; ++i)
    {
        const Vector globalCorner = m_lastWaterPose.TransformPoint( m_localCorners[i] );
        frame->AddDebugLine( globalCorner, globalCorner + Vector(0,0,2), Color::RED );
    }

#ifndef FINAL
    Bool any = false;
    for(Uint32 i=0; i<RAYCASTS_NUM; ++i)
    {
        SProbingRaycast& raycastInfo = m_raycastsList[i];
        Vector start = raycastInfo.DBG_localRayStartPos + globalPos;
        if( raycastInfo.hitDistance < 0 )
        {
            // Mark ray start position
            frame->AddDebugLine( start, start + Vector::EZ, Color::BLUE);
            // Cutoff length
            frame->AddDebugLine( start, start + raycastInfo.rayDirection * BoatConfig::cvBoatHedgeRaycastCutoffLength.Get(), Color::BLUE);
            // Cutoff end to raycast length
            start = start + raycastInfo.rayDirection * BoatConfig::cvBoatHedgeRaycastCutoffLength.Get();
            const Float raycastLength = m_raycastLengthDamper.GetInterpolatedValue();
            frame->AddDebugLine( start, start+raycastInfo.rayDirection * ( raycastLength - BoatConfig::cvBoatHedgeRaycastCutoffLength.Get() ), Color::GREEN);
        }
        else
        {
            any = true;
            const Vector dir = raycastInfo.globalHitPos - start;
            frame->AddDebugLine( start, start+dir, Color::RED );
            //frame->AddDebugLine( start+dir, start+dir+raycastInfo.globalHitNormal, Color::YELLOW );
        }
    }

    // Draw global repell force and normal
    if( any )
    {
        const Vector start = globalPos + Vector(0, 0, 2.0f);

        Vector temp = m_smoothRepelVec;
        Float mag = temp.Normalize3();
        frame->AddDebug3DArrow( start, temp, mag, 0.015f, 0.045f, 0.3f, Color::GREEN, Color::GREEN );

//         temp = m_smoothShoreNormal;
//         mag = temp.Normalize3();
//         frame->AddDebug3DArrow( start, temp, mag, 0.015f, 0.045f, 0.3f, Color::YELLOW, Color::YELLOW );
    }
#endif
}

//////////////////////////////////////////////////////////////////////////