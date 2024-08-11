#pragma once
#include "boatConfig.h"

//////////////////////////////////////////////////////////////////////////

class CBoatComponent;

//////////////////////////////////////////////////////////////////////////

class CBoatPathfindingSystem
{
//firends:
    friend class CBoatComponent;

public:
    CBoatPathfindingSystem(void);
    ~CBoatPathfindingSystem(void);

    Bool Initialize( CBoatComponent* boatComponent, THandle<CRigidMeshComponent>* hBoatBody );
    void Deinitialize();
    void DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset );

    void PathFindingUpdate( CPhysicsWrapperInterface* wrapper, Float& inOutTargetAnglePercent, Int32& inOutTargetGear );

    void PathFindingMoveToLocation( const Vector& destinationGlobal, const Vector& destinationLookAtNormal, Int32& inOutTargetGear );
    void PathFindingFollowCurve( SMultiCurve* path, Int32& inOutTargetGear );
    void PathFindingStop();

    void RestoreSerializedState( SBoatSerialization* serializationStruct );

private:
    Int32 PathFindingInit();
    Int32 PathFindingSetGears( Float curveZ );

private:
    CBoatComponent*         m_boatComponent;
    THandle<CRigidMeshComponent>*    m_hBoatBody;
    Bool                    m_isInitialized;

    Bool                    m_isPathFinding;
    Bool                    m_externalCurve;

    SMultiCurve*            m_curve;
    SMultiCurve             m_localCurve;
    Float                   m_curveTimeStep;
    Float                   m_timeOnCurve;
    Matrix                  m_curveLocalToWorld;
    Vector                  m_nextCurveNodePos;
    Vector2                 m_nextCurveNodeTangent;
    Float                   m_boatLength;
    Bool                    m_useOutOfFrustumTeleport;
    
#ifndef FINAL
    Vector2                 DBG_boatHeading;
    Vector2                 DBG_boatToNode;
    TDynArray<Vector>       DBG_pathFollow;
    Vector                  DBG_currFollowedNode;
    Vector                  DBG_currFollowedNodeTangent;
#endif
};

//////////////////////////////////////////////////////////////////////////
