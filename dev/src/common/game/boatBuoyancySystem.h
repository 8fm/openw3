#pragma once

//////////////////////////////////////////////////////////////////////////

#include "gameTypeRegistry.h"
#include "boatConfig.h"

//////////////////////////////////////////////////////////////////////////

#ifdef USE_PHYSX
namespace physx{
    class PxRigidDynamic;
    class PxD6Joint;
}
#endif

class CBoatComponent;

//////////////////////////////////////////////////////////////////////////

// Super super shitty pid controller
class PID
{
public:
	Float	m_kp;
	Float	m_ki;
	Float	m_kd;
	Float	m_maxValue;

private:
	Float	m_value;
	Float	m_integral;
	Float	m_prevErr;

public:
	PID() : m_kp( 0.1f ), m_ki( 0.005f ), m_kd( 0.01f ), m_maxValue( FLT_MAX ) { Reset( 0.f ); }

	void Reset( Float value )
	{
		m_value = value;
		m_integral = 0.f;
		m_prevErr = 0.f;
	}

	Float Update( Float setValue, Float dt );
};

//////////////////////////////////////////////////////////////////////////

//#define FORCE_KINEMATIC_STATE

//////////////////////////////////////////////////////////////////////////

class CBoatBuoyancySystem
{
//friends:
    friend class CBoatComponent;

public:
    CBoatBuoyancySystem(void);
    ~CBoatBuoyancySystem(void);
    
    Bool Initialize( CBoatComponent* boatComponent, THandle<CRigidMeshComponent>* hBoatBodyRigidMesh );
    void Deinitialize();
    void BuoyancyUpdate( Vector& inOutForce, Vector& inOutTorque, Float timeDelta, CPhysicsWrapperInterface* wrapper );
    void DebugDraw( CRenderFrame* frame, Uint32& inOutTestOffset );
    void DisableJointTiltLimit();

    void SetLocalTiltAngle( const Vector2& anglePerc ){ m_tiltAnglePerc = anglePerc; }

    void TriggerDrowning( const Vector& globalHitPoint );
	RED_INLINE Bool IsDrowning() const { return m_isDrowning; }

    void LockXYMovement( Bool lock );
    Bool IsXYLocked() const { return m_isXYLocked; };
    
	void ResetState();

private:
    Vector  GetTiltAngles() const;
    void    UpdateConstrainerPose( CPhysicsWrapperInterface* wrapper );

private:
    Bool                            m_firstTick;
    Bool                            m_isInitialized;
    Bool                            m_isXYLocked;
    CBoatComponent*                 m_boatComponent;
    THandle<CRigidMeshComponent>*   m_boatBodyRigidMesh;

    SWaterContactPoint m_buoyancyPoints[MAX_WATER_CONTACT_POINTS];
    
    Bool							m_isDrowning;
    Float							m_drowningTimeSoFar;
    Float                           m_increasedDampingTimeout;
    
    Vector2                         m_tiltAnglePerc;
    
    Uint32                          m_noBuoyantsOnTerrain;

	PID								m_pid;

#ifdef USE_PHYSX
    // Constrainer
    physx::PxRigidDynamic*          m_dynamicConstrainer;
    physx::PxD6Joint*               m_sixDofJoint;
#endif
    
#ifndef FINAL
    Matrix DBG_massGlobalPose;

    Float  DBG_angleX;
    Float  DBG_angleY;

    Float  DBG_desiredAngleX;
    Float  DBG_desiredAngleY;

    Float  DBG_avgHeight;

    Vector DBG_buoyancyTorqueX;
    Vector DBG_buoyancyTorqueY;
    Vector DBG_buoyancyForce;

    Float  DBG_pointFTerrainHeight;
    Float  DBG_pointFWaterHeight;

    Float  DBG_pointBTerrainHeight;
    Float  DBG_pointBWaterHeight;

    Float  DBG_pointLTerrainHeight;
    Float  DBG_pointLWaterHeight;

    Float  DBG_pointRTerrainHeight;
    Float  DBG_pointRWaterHeight;

    Float  DBG_powTorqueX;
    Float  DBG_torqueX;

    Float  DBG_powForceZ;
    Float  DBG_forceZ;

    Vector DBG_constrainerGlobalPos;
#endif
};

//////////////////////////////////////////////////////////////////////////
