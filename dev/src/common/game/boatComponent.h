#pragma once

//////////////////////////////////////////////////////////////////////////

#include "vehicle.h"
#include "..\engine\floatDamper.h"
#include "boatConfig.h"

//////////////////////////////////////////////////////////////////////////

class CBoatBuoyancySystem;
class CBoatInputSystem;
class CBoatPathfindingSystem;
class CBoatHedgehog;

//////////////////////////////////////////////////////////////////////////

// #ifndef FINAL
//     #define DEBUG_WATER_TRACE
// #endif

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS
#ifndef FINAL
    #define USE_BOAT_DEBUG_PAGES
#endif
#endif
#endif

#ifdef DEBUG_WATER_TRACE
    static const Uint32 TRACE_SIZE_X = 12;
    static const Uint32 TRACE_SIZE_Y = 12; 
#endif

//////////////////////////////////////////////////////////////////////////

class CBoatComponent	: public CVehicleComponent
						, public ILODable
{
//friends:
    friend class CBoatBuoyancySystem;
    friend class CBoatInputSystem;
    friend class CBoatPathfindingSystem;
    friend class CNotStreamedBoatSpawnEventHandler;
    friend class CBoatHedgehog;

private:
	struct SBoatCameraSetup
	{
		Float m_fov;
		Float m_distance;
		Float m_pitch;
		Float m_offsetUp;
		Float m_sailOffset;

		SBoatCameraSetup();
	};

    DECLARE_ENGINE_CLASS( CBoatComponent, CVehicleComponent, 0 );
public:
    CBoatComponent();
    virtual ~CBoatComponent();

    // CComponent ovverides
    virtual void OnAttached( CWorld* world );
    virtual void OnDetached( CWorld* world );
    virtual void OnParentAttachmentAdded( IAttachment* attachment );
    virtual void OnParentAttachmentBroken( IAttachment* attachment );
    virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

    // Save load
    void OnSaveGameplayState( IGameSaver* saver );
	void OnSaveGameplayState_FallbackHack( IGameSaver* saver );
    void OnLoadGameplayState( IGameLoader* loader );
	void OnLoadGameplayState_FallbackHack( IGameLoader* loader );

    // Reset state
    void ResetState();

    // Path finding system
    void PathFindingMoveToLocation( const Vector& destinationGlobal, const Vector& lookAtNormal );
    void PathFindingFollowCurve( SMultiCurve* path );
    void PathFindingStop();
	void OnReachedEndOfPath();

    void SetStateDrowning();

    // Get basic water level
    Float GetBasicWaterLevel() const;

    // Gears linear velocity
    Int32 GetCurrentGear() const;
    const SBoatGear& GetGear( Int32 gear ) const;
    Int32 GetGearFromXYVelocity() const;
    Float GetLinearVelocityXY() const;
    Float GetMaxVelocityAtCurrentGear() const;

    void LockXYMovement( Bool lockMovement );

    void BoatBodyStruck( const Vector2& globalForce, const Vector& globalHit, EBoatCollisionSource collisionSource );
    
    void NotifyTeleported(){ m_waitForTeleport = false; }
    void StartTeleport(){ m_waitForTeleport = true; }

    Bool IsSpanedFromQuestBlock() const { return m_isSpawnedFromQuestBlock; };
    
    void OnDebugPageClosed();

public:
	virtual void OnDriverMount() override;

	inline Bool IsDriverDismounted() { return m_driverDismounted; }
    Bool IsWaitingForTeleport() const;

private:
    // Sunsystems initialization
    Bool InitializeSubsystems( const Box& bbox );
    void DeinitializeSubsystems();
    void RestoreSerializedState( CPhysicsWrapperInterface* wrapper );
    
    // Updates
    virtual void OnTickPrePhysics( Float deltaTime );
    void UpdateSailAngleAndWindTilt( Float deltaTime, CPhysicsWrapperInterface* wrapper, const Matrix& globalRotation );

protected:
	// ILODable
	void UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager ) override;
	ILODable::LOD ComputeLOD( CLODableManager* manager ) const override;

private:
	void GetTargetCameraSetup( SBoatCameraSetup& outTarget, Bool passenger );
	void UpdateCurrentCameraSetup( const SBoatCameraSetup& target, float dt, Bool passenger );
private:
    // Script functions
    void funcMountStarted( CScriptStackFrame& stack, void* result );
    void funcDismountFinished( CScriptStackFrame& stack, void* result );

    void funcGetCurrentGear( CScriptStackFrame& stack, void* result );
	void funcGetCurrentSpeed( CScriptStackFrame& stack, void* result );

    void funcGetBuoyancyPointStatus_Front( CScriptStackFrame& stack, void* result );
    void funcGetBuoyancyPointStatus_Back( CScriptStackFrame& stack, void* result );
    void funcGetBuoyancyPointStatus_Right( CScriptStackFrame& stack, void* result );
    void funcGetBuoyancyPointStatus_Left( CScriptStackFrame& stack, void* result );

    void funcGetMaxSpeed( CScriptStackFrame& stack, void* result );
    void funcGetLinearVelocityXY( CScriptStackFrame& stack, void* result );
    void funcGetBoatBodyMass( CScriptStackFrame& stack, void* result );
    void funcStopAndDismountBoat( CScriptStackFrame& stack, void* result  );
    void funcUseOutOfFrustumTeleportation( CScriptStackFrame& stack, void* result  );

    void funcTriggerCutsceneStart( CScriptStackFrame& stack, void* result );
    void funcTriggerCutsceneEnd( CScriptStackFrame& stack, void* result );

    void funcTriggerDrowning( CScriptStackFrame& stack, void* result );
	void funcIsDrowning( CScriptStackFrame& stack, void* result );

    void funcMountedAfterSaveRestore( CScriptStackFrame& stack, void* result );

	void funcGameCameraTick( CScriptStackFrame& stack, void* result );

    void funcSetInputValues( CScriptStackFrame& stack, void* result );
private:
    TDynArray< CName >              m_collisionNames;
    THandle<CRigidMeshComponent>    m_hBoatBodyRigidMesh;
    THandle<CBoatDestructionComponent> m_hDestructionComponent;

    // Subsystems
    Bool                            m_subSystemsInitialized;
    CBoatBuoyancySystem*            m_buoyancySystem;
    CBoatHedgehog*                  m_hedgehogSystem;
    CBoatInputSystem*               m_inputSystem;
    CBoatPathfindingSystem*         m_pathfindingSystem;
        
    // Flags
    Bool 							m_isInCutsceneMode;

    Bool                            m_isDismountRequested;
    Bool                            m_mountAnimationFinished;
    
    // Steering
    TDynArray<SBoatGear>            m_sailSpeeds;

    // Drowning
    Float                           m_drowningShutdownTimeout;

    Bool                            m_shouldSaveSet;
    Bool                            m_waitForTeleport;
    
    // Mast wind and sailing
    CFloatDamper                    m_turningYTiltDamper;
    Float                           m_sailDir;

    SBoatSerialization*             m_serializationStruct;

    static SBoatGear                m_sGearZero;
	Bool							m_driverDismounted;

#ifndef FINAL
    Float                           DBG_linearSpeed2D;
    Float                           DBG_localLinearSpeed2D;
    Float                           DBG_angularSpeedZ;
    Vector2                         DBG_gradient;
    Matrix                          DBG_centerOfMassPose;
    Vector                          DBG_movementForce;

    Matrix                          DBG_sweepRotation;    
    Vector                          DBG_sweepBoxExtent;

    Float                           DBG_tiltDamperTarget;
    Float                           DBG_tiltYDampedValue;
#endif

    Uint32 m_yTestOffset;

#ifdef DEBUG_WATER_TRACE
    Vector m_debugWaterTrace[TRACE_SIZE_X*TRACE_SIZE_Y];
#endif

private:

	SBoatCameraSetup				m_currentCameraSetup;
    Bool                            m_isSpawnedFromQuestBlock;
    Bool                            m_wasWrapperUnstreamLocked;
};
 
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

BEGIN_CLASS_RTTI( CBoatComponent );
    PARENT_CLASS( CVehicleComponent );

    PROPERTY_SAVED( m_sailDir )
    PROPERTY_SAVED( m_mountAnimationFinished );
    PROPERTY_CUSTOM_EDIT( m_collisionNames, TXT( "Defines which collision groups will collide with boat" ), TXT("CollisionGroupSelector") );
    
    NATIVE_FUNCTION( "GetBuoyancyPointStatus_Front", funcGetBuoyancyPointStatus_Front );
    NATIVE_FUNCTION( "GetBuoyancyPointStatus_Back",  funcGetBuoyancyPointStatus_Back );
    NATIVE_FUNCTION( "GetBuoyancyPointStatus_Right", funcGetBuoyancyPointStatus_Right );
    NATIVE_FUNCTION( "GetBuoyancyPointStatus_Left",  funcGetBuoyancyPointStatus_Left );
    NATIVE_FUNCTION( "GetLinearVelocityXY",          funcGetLinearVelocityXY );
    NATIVE_FUNCTION( "GetBoatBodyMass",              funcGetBoatBodyMass );
    NATIVE_FUNCTION( "StopAndDismountBoat",          funcStopAndDismountBoat );
    NATIVE_FUNCTION( "UseOutOfFrustumTeleportation", funcUseOutOfFrustumTeleportation );
    NATIVE_FUNCTION( "TriggerCutsceneStart",         funcTriggerCutsceneStart );
    NATIVE_FUNCTION( "TriggerCutsceneEnd",           funcTriggerCutsceneEnd );
    NATIVE_FUNCTION( "TriggerDrowning",              funcTriggerDrowning );
	NATIVE_FUNCTION( "IsDrowning",					 funcIsDrowning );
    NATIVE_FUNCTION( "MountStarted",                 funcMountStarted );
    NATIVE_FUNCTION( "DismountFinished",             funcDismountFinished );
    NATIVE_FUNCTION( "GetCurrentGear",               funcGetCurrentGear );
	NATIVE_FUNCTION( "GetCurrentSpeed",              funcGetCurrentSpeed );
	NATIVE_FUNCTION( "GameCameraTick",               funcGameCameraTick );
    NATIVE_FUNCTION( "GetMaxSpeed",                  funcGetMaxSpeed );
    NATIVE_FUNCTION( "SetInputValues",               funcSetInputValues );
    
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
