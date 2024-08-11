#pragma once
#include "..\engine\vectorDamper.h"
#include "..\engine\floatDamper.h"
#include "boatConfig.h"

//////////////////////////////////////////////////////////////////////////

class CBoatComponent;
const static Uint32 RAYCASTS_NUM           = 15;
const static Float RAY_DISTANCE_EPSILON    = 0.05f;

//////////////////////////////////////////////////////////////////////////

class CBoatHedgehog
{
//friends:
    friend class CBoatComponent;

public:
    CBoatHedgehog(void);

    Bool Initialize( CBoatComponent* boatComponent, THandle<CRigidMeshComponent>* hBoatBodyRigidMesh, THandle<CBoatDestructionComponent>* hBoatDestruction, const Box& bbox );
    void Deinitialize();

    void UpdateHedgehog( Float deltaTime, Uint32 noBuoyantsOnTerrain );

    void RestoreSerializedState( SBoatSerialization* serializationStruct );

    void OnDebugPageClosed();
    void DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset );

    CFloatDamper                    m_inputDamper;

private:
    Bool                            m_isInitialized;
    CBoatComponent*                 m_boatComponent;
    THandle<CRigidMeshComponent>*   m_hBoatBodyRigidMesh;
    THandle<CBoatDestructionComponent>* m_hBoatDestruction;

    Float                           m_basicWaterLevel;
    Vector                          m_smoothRepelVec;
    Vector                          m_smoothShoreNormal;
    Matrix                          m_currentPose;
    CPhysicsEngine::CollisionMask   m_collisionMask;

    CFloatDamper                    m_raycastLengthDamper;
    CVectorDamper                   m_repelDamper;
    //CVectorDamper                   m_normalDamper;
    
    struct SProbingRaycast
    {
        Float  hitDistance;
        Vector rayDirection;
        Vector globalHitPos;
        //Vector globalHitNormal;

#ifndef FINAL
        Vector DBG_localRayStartPos;
#endif
    };

    TStaticArray<SProbingRaycast, RAYCASTS_NUM> m_raycastsList;

    // Save last pose where boat was on water    
    Bool                            m_terrainRescueMode;
    Matrix                          m_lastWaterPose;
    Vector2                         m_localCorners[4];
};

//////////////////////////////////////////////////////////////////////////

