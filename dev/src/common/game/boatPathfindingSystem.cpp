#include "build.h"
#include "boatPathfindingSystem.h"

#include "boatComponent.h"

#include "gameWorld.h"
#include "..\engine\mesh.h"
#include "../physics/physicsWrapper.h"
#include "..\core\mathUtils.h"
#include "..\engine\rigidMeshComponent.h"
#include "..\core\gameSave.h"
#include "..\engine\renderFrame.h"
#include "..\engine\game.h"
#include "..\engine\tagManager.h"
#include "..\engine\pathComponent.h"

//////////////////////////////////////////////////////////////////////////

CBoatPathfindingSystem::CBoatPathfindingSystem(void)
    : m_isInitialized( false )
    , m_boatComponent( nullptr )
    , m_hBoatBody( nullptr )
    , m_externalCurve( false )
{
}

//////////////////////////////////////////////////////////////////////////

CBoatPathfindingSystem::~CBoatPathfindingSystem(void)
{
    Deinitialize();
}

//////////////////////////////////////////////////////////////////////////

Bool CBoatPathfindingSystem::Initialize( CBoatComponent* boatComponent, THandle<CRigidMeshComponent>* hBoatBody )
{
    if( m_isInitialized )
        return true;

    if( boatComponent == nullptr || hBoatBody == nullptr || (*hBoatBody).Get() == nullptr )
        return false;

    m_boatComponent = boatComponent;
    m_hBoatBody = hBoatBody;

    m_isInitialized = true;
    return true;
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::Deinitialize()
{
    m_curve = nullptr;
    m_localCurve.Reset();
    m_externalCurve = false;

    m_isInitialized = false;
    m_boatComponent = nullptr;
    m_hBoatBody = nullptr;
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset )
{
#ifndef FINAL
    if( !m_isInitialized || !m_isPathFinding )
        return;
    
    frame->AddDebugSphere( DBG_currFollowedNode, 0.3f, Matrix::IDENTITY, Color::YELLOW );
    frame->AddDebugLine( DBG_currFollowedNode, DBG_currFollowedNode + DBG_currFollowedNodeTangent * 3.0f, Color::GREEN );

    Float scale = DBG_boatToNode.Mag();
    Vector norm = DBG_boatToNode / scale;
    frame->AddDebug3DArrow( DBG_currFollowedNode - DBG_boatToNode, norm, scale, 0.015f, 0.045f, 0.3f, Color::DARK_MAGENTA, Color::DARK_MAGENTA );

    scale = DBG_boatHeading.Mag();
    norm = DBG_boatHeading / scale;
    frame->AddDebug3DArrow( DBG_currFollowedNode - DBG_boatToNode, norm, scale, 0.015f, 0.045f, 0.3f, Color::LIGHT_BLUE, Color::LIGHT_BLUE );

    if( !DBG_pathFollow.Empty() )
    {
        for( Uint32 i=1; i<DBG_pathFollow.Size(); ++i )
        {
            frame->AddDebugLine( DBG_pathFollow[i-1], DBG_pathFollow[i], Color::YELLOW );
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////

Int32 CBoatPathfindingSystem::PathFindingSetGears( Float curveZ )
{
    if( m_boatComponent == nullptr )
    {
        return 1;
    }

    Int32 gear = ( Int32 )MCeil( fabsf( curveZ - m_boatComponent->GetBasicWaterLevel() ) );

    if(gear >= (Int32)MAX_GEARS)
    {
        gear = (Int32)MAX_GEARS;
    }

    if( gear <= 0 )
    {
        gear = 1;
    }

    return gear;    
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::PathFindingMoveToLocation( const Vector& destinationGlobal, const Vector& destinationLookAtNormal, Int32& inOutTargetGear )
{
    if( m_isPathFinding || m_boatComponent == nullptr || m_hBoatBody == nullptr || (*m_hBoatBody).Get() == nullptr )
    {
        return;
    }
    
    // Get boat length
    CMesh* boatMesh = (*m_hBoatBody)->GetMeshNow();
    Box boatBBox = boatMesh->GetBoundingBox();
    m_boatLength = boatBBox.CalcSize().Y;

    const Vector startPosition = (*m_hBoatBody)->GetWorldPosition();
    const Vector tangent1 = (*m_hBoatBody)->GetLocalToWorld().GetAxisY() * m_boatLength * 0.25f;
    const Vector tangent2 = destinationLookAtNormal.Normalized3() * m_boatLength * 0.25f;

    // Set up curve
    m_localCurve.Reset();
    m_localCurve.SetCurveType( ECurveType_Vector, nullptr, false );
    m_localCurve.SetPositionInterpolationMode( ECurveInterpolationMode_Manual );
    m_localCurve.SetTransformationRelativeMode( ECurveRelativeMode_None );
    m_localCurve.SetShowFlags( SHOW_Paths );
    m_localCurve.SetLooping( false );
    m_localCurve.SetColor( Color::YELLOW );

    m_localCurve.AddControlPoint( 0.0f, startPosition );
    m_localCurve.AddControlPoint( 0.1f, MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( startPosition, tangent1, destinationGlobal, tangent2, 0.1f ) );
    m_localCurve.AddControlPoint( 0.2f, MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( startPosition, tangent1, destinationGlobal, tangent2, 0.2f ) );
    m_localCurve.AddControlPoint( 0.3f, MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( startPosition, tangent1, destinationGlobal, tangent2, 0.3f ) );

    m_localCurve.AddControlPoint( 0.7f, MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( startPosition, tangent2, destinationGlobal, tangent2, 0.7f ) );
    m_localCurve.AddControlPoint( 0.8f, MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( startPosition, tangent2, destinationGlobal, tangent2, 0.8f ) );
    m_localCurve.AddControlPoint( 0.9f, MathUtils::InterpolationUtils::HermiteInterpolateWithTangents( startPosition, tangent2, destinationGlobal, tangent2, 0.8f ) );
    m_localCurve.AddControlPoint( 1.0f, destinationGlobal );

    m_localCurve.EnableAutomaticTimeByDistanceRecalculation( true );

    m_curve = &m_localCurve;
    m_curveLocalToWorld = Matrix::IDENTITY;

    m_externalCurve = false;

    inOutTargetGear = PathFindingInit();
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::PathFindingFollowCurve( SMultiCurve* path, Int32& inOutTargetGear )
{
    if( m_isPathFinding || path == nullptr )
    {
        return;
    }

    m_curve = path;
    m_externalCurve = true;

    if( path->GetParent() != nullptr )
    {
        path->GetParent()->GetLocalToWorld( m_curveLocalToWorld );
    }
    else
    {
        m_curveLocalToWorld = Matrix::IDENTITY;
    }

    inOutTargetGear = PathFindingInit();
}

//////////////////////////////////////////////////////////////////////////

Int32 CBoatPathfindingSystem::PathFindingInit()
{
    if( m_curve == nullptr || m_boatComponent == nullptr || m_hBoatBody == nullptr || (*m_hBoatBody).Get() == nullptr )
    {
        return 0;
    }

    // Start path finding
    m_isPathFinding = true;

    // Get boat length
    CMesh* boatMesh = (*m_hBoatBody)->GetMeshNow();
    Box boatBBox = boatMesh->GetBoundingBox();
    m_boatLength = boatBBox.CalcSize().Y;

    // Get curve sampling time step
    Float timeStep = 0.05f;
    Vector start, end;
    m_curve->GetPosition( 0.0f, start );
    m_curve->GetPosition( 1.0f, end );
    Float mag = ( end - start ).Mag2();
    mag = m_boatLength / mag;

    if( mag < timeStep )
    {
        timeStep = mag;
    }

    timeStep *= 0.25f;

    // Get curve length
    Float curveLength = 0.0f;
    Float curveTime = 0.0f;
    Vector p1;
    m_curve->GetPosition( curveTime, p1 );
    p1 = m_curveLocalToWorld.TransformPoint( p1 );
#ifndef FINAL
    DBG_pathFollow.PushBack( p1 );
#endif
    while( curveTime < 1.0f )
    {
        EngineTransform et;
        curveTime += timeStep;
        Vector p2;
        m_curve->GetPosition( curveTime, p2 );
        p2 = m_curveLocalToWorld.TransformPoint( p2 );
        curveLength += p1.DistanceTo2D( p2 );
        p1 = p2;

#ifndef FINAL
        DBG_pathFollow.PushBack( p2 );
#endif
    }

    // Compute time step for path finding
    m_curveTimeStep = m_boatLength / curveLength;
    m_curveTimeStep *= m_curve->GetTotalTime() * 0.25f;
    m_timeOnCurve = m_curveTimeStep;

    // Get first target node
    m_curve->GetPosition( m_timeOnCurve, m_nextCurveNodePos );
    m_nextCurveNodePos = m_curveLocalToWorld.TransformPoint( m_nextCurveNodePos );

    // And Tangent
    m_curve->CalculateTangentFromCurveDirection( m_timeOnCurve, start );
    p1 = m_curveLocalToWorld.GetTranslation();
    m_curveLocalToWorld.SetTranslation( Vector::ZEROS );
    m_nextCurveNodeTangent = m_curveLocalToWorld.TransformPoint( start.Normalized2() );
    m_curveLocalToWorld.SetTranslation( p1 );


#ifndef FINAL
    DBG_currFollowedNode = m_nextCurveNodePos;
    DBG_currFollowedNodeTangent = m_nextCurveNodeTangent;
#endif

    // Set gear
    return PathFindingSetGears( m_nextCurveNodePos.Z );
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::PathFindingStop()
{
    m_isPathFinding = false;

    m_curve = nullptr;
    m_localCurve.Reset();
    m_externalCurve = false;

#ifndef FINAL
    DBG_pathFollow.Clear();
#endif

    if( m_boatComponent != nullptr )
    {
	    m_boatComponent->OnReachedEndOfPath();
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::RestoreSerializedState( SBoatSerialization* serializationStruct )
{
    if( serializationStruct == nullptr || m_boatComponent == nullptr )
    {
        ASSERT(serializationStruct);
        return;
    }

    if( serializationStruct->pathfindingCurvesEntityTag == CName::NONE/*serializationStruct->pathfindingComponentTagList.Empty()*/ )
    {
        Int32 dummy = 0;
        PathFindingMoveToLocation( serializationStruct->pathfindingCurveEndPos, serializationStruct->pathfindingCurveEndTangent, dummy );
    }
    else
    {
        //const Uint32 tagsSize = serializationStruct->pathfindingComponentTagList.Size();
        SMultiCurve* curve = nullptr;
        
        // Get entity tag
        CWorld* world = m_boatComponent->GetWorld();
        if( world == nullptr )
        {
            return;
        }
        
        CNode* entityNode = world->GetTagManager()->GetTaggedNode( serializationStruct->pathfindingCurvesEntityTag );
        if( entityNode == nullptr )
            return;

        if( !entityNode->IsA<CEntity>() )
            return;

        // Get path component
        CEntity* curveEntity = (CEntity*)entityNode;
        CPathComponent* pathComponent = curveEntity->FindComponent<CPathComponent>();
        if( pathComponent == nullptr )
            return;

        curve = &pathComponent->GetCurve();
        
        if( curve == nullptr )
            return;

        // Speed damper is saved directly no need for gear setting here
        Int32 dummy = 0;
        PathFindingFollowCurve( curve, dummy );
    }

    m_timeOnCurve              = serializationStruct->pathfindingTimeOnCurve;
    m_curveLocalToWorld        = serializationStruct->pathfindingCurveLocalToWorld;
    m_nextCurveNodePos         = serializationStruct->pathfindingNextCurveNodePos;
    m_nextCurveNodeTangent     = serializationStruct->pathfindingNextCurveNodeTangent;
    m_useOutOfFrustumTeleport  = serializationStruct->pathfindingUseOutOfFrustumTeleport;
}

//////////////////////////////////////////////////////////////////////////

void CBoatPathfindingSystem::PathFindingUpdate( CPhysicsWrapperInterface* wrapper, Float& inOutTargetAnglePercent, Int32& inOutTargetGear )
{
    if( !m_isPathFinding || m_boatComponent == nullptr || m_hBoatBody == nullptr || (*m_hBoatBody).Get() == nullptr || m_curve == nullptr )
    {
        return;
    }

    Vector2 boatHeading = wrapper->GetPose().GetAxisY();
    boatHeading.Normalize();

    const Vector2 boatPosition = wrapper->GetPosition() + ( boatHeading * m_boatLength );
    
    if( m_useOutOfFrustumTeleport)
    {
        // If boat is to far away from player, and player is not looking in boats direction, do teleport
        CEntity* player = GGame->GetPlayerEntity();
        if( player != nullptr )
        {
            const Vector playerPosition = player->GetWorldPosition();
            const Float distanceSq = wrapper->GetPosition().DistanceSquaredTo2D( playerPosition );
            const Float sqTeleportDist = BoatConfig::cvBoatPathFindingOutFrustumMaxDistance.Get() * BoatConfig::cvBoatPathFindingOutFrustumMaxDistance.Get();
            if( distanceSq > sqTeleportDist )
            {
                CMesh* boatMesh = (*m_hBoatBody)->GetMeshNow();
                CCameraDirector* cameraDirector = nullptr;
                CWorld* world = m_boatComponent->GetWorld();

                if( world != nullptr )
                {
                    cameraDirector = world->GetCameraDirector();
                }

                if( cameraDirector != nullptr && boatMesh != nullptr )
                {
                    CFrustum frustum;
                    cameraDirector->CalcCameraFrustum( frustum );

                    Matrix ltw;
                    m_boatComponent->GetLocalToWorld( ltw );
                    const Box localBBox = boatMesh->GetBoundingBox();

                    Box globalBBox = localBBox;
                    globalBBox.Min = ltw.TransformPoint( localBBox.Min );
                    globalBBox.Max = ltw.TransformPoint( localBBox.Max );

                    // Boat is out of current camera frustum, we can teleport it now
                    if( frustum.TestBox( globalBBox ) == 0 )
                    {
                        Float curveTime = 0;
                        while( ( curveTime += m_curveTimeStep ) < m_curve->GetTotalTime() )
                        {
                            Vector positionOnCurve;
                            m_curve->GetPosition( curveTime, positionOnCurve );
                            positionOnCurve = m_curveLocalToWorld.TransformPoint( positionOnCurve );

                            // Teleport if point on path was found
                            if( positionOnCurve.DistanceSquaredTo2D( playerPosition ) < sqTeleportDist * 0.44f ) // times ( 2/3 )^2
                            {
                                // Set proper position on water surface
                                CWorld* world = m_boatComponent->GetWorld();
                                if( world != nullptr && world->IsWaterShaderEnabled() )
                                {
                                    positionOnCurve.Z = world->GetWaterLevel( positionOnCurve, 1 );
                                }

                                Vector tangent;
                                m_curve->CalculateTangentFromCurveDirection( curveTime, tangent );

                                // Transform tangent to global space
                                Vector temp = m_curveLocalToWorld.GetTranslation();
                                m_curveLocalToWorld.SetTranslation( Vector::ZEROS );
                                tangent = m_curveLocalToWorld.TransformPoint( tangent );
                                m_curveLocalToWorld.SetTranslation( temp );

                                // Rotate boat as tangent directs but preserve its tilt
                                ltw.V[1] = tangent.Normalized2();  // Y
                                ltw.V[1].Z = 0.0f;
                                ltw.V[0] = Vector::Cross( ltw.V[1], ltw.V[2] ); // X = Y.cross(Z)
                                ltw.V[1] = Vector::Cross( ltw.V[2], ltw.V[0] ); // Y = Z.cross(X)
                                ltw.V[3] = positionOnCurve;

                                globalBBox.Min = ltw.TransformPoint( localBBox.Min );
                                globalBBox.Max = ltw.TransformPoint( localBBox.Max );

                                if( frustum.TestBox( globalBBox ) == 0 )
                                {
                                    // Teleport
                                    m_timeOnCurve = curveTime;

                                    if( m_boatComponent != nullptr )
                                    {
                                        m_boatComponent->GetEntity()->Teleport( ltw.GetTranslation(), ltw.ToEulerAngles() );
                                    }

                                    break;
                                }
                            } // Found point is closer to the player
                        }
                    }// if is out of frustum
                }// is camera director not null
            }// is distance to big
        }// is player not null
    }// Use teleportation

    // Get next target node if wee passed current one
    Vector2 boatPosToNodePos;
    boatPosToNodePos = m_nextCurveNodePos - boatPosition;

#ifndef FINAL
    DBG_boatHeading = boatHeading;
    DBG_boatToNode = boatPosToNodePos;
#endif

    // Compare vector directions
    while( boatPosToNodePos.Dot( m_nextCurveNodeTangent ) < 0.0f )
    {
        m_timeOnCurve += m_curveTimeStep;

        // Following finished
        if( m_timeOnCurve >= m_curve->GetTotalTime() )
        {
            PathFindingStop();
            inOutTargetGear = 0;
            return;
        }

        // Get next position
        m_curve->GetPosition( m_timeOnCurve, m_nextCurveNodePos );
        m_nextCurveNodePos = m_curveLocalToWorld.TransformPoint( m_nextCurveNodePos );

        // Get next tangent
        Vector temp;
        m_curve->CalculateTangentFromCurveDirection( m_timeOnCurve, temp );
        m_nextCurveNodeTangent = temp.Normalized2();

        // Transform to global
        temp = m_curveLocalToWorld.GetTranslation();
        m_curveLocalToWorld.SetTranslation( Vector::ZEROS );
        m_nextCurveNodeTangent = m_curveLocalToWorld.TransformPoint( m_nextCurveNodeTangent );
        m_curveLocalToWorld.SetTranslation( temp );

        // Set for new node pos
        boatPosToNodePos = m_nextCurveNodePos - boatPosition;

#ifndef FINAL
        DBG_currFollowedNode = m_nextCurveNodePos;
        DBG_currFollowedNodeTangent = m_nextCurveNodeTangent;
#endif
    }

    // Set gear every frame in case it was reset durnig teleport routine
    inOutTargetGear = PathFindingSetGears( m_nextCurveNodePos.Z );

    // Get angle between heading vec and "boatPos to nodePos" vec and compute percent from range <0,90> deg
    inOutTargetAnglePercent = Clamp<Float>( 1.0f - boatHeading.Dot( boatPosToNodePos.Normalized() ), 0.0f, 1.0f );

    // Get turning direction
    const Vector turningCompare = Vector::Cross( Vector::EZ, boatHeading );
    if( turningCompare.Dot2( boatPosToNodePos ) > 0.0f )
    {
        inOutTargetAnglePercent *= -1.0f;
    }
}

//////////////////////////////////////////////////////////////////////////
