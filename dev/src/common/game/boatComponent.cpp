#include "build.h"
#include "boatComponent.h"

#include "boatBuoyancySystem.h"
#include "boatHedgehog.h"
#include "boatInputSystem.h"
#include "boatPathfindingSystem.h"
#include "boatBodyComponent.h"
#include "boatDestructionComponent.h"
#include "boatDestructionComponent.h"
#include "boatBodyComponent.h"
#include "debugWindowBoatSettings.h"
#include "gameWorld.h"

#include "..\core\gameSave.h"
#include "..\engine\mesh.h"
#include "..\physics\physicsWorld.h"
#include "..\engine\effectDummyComponent.h"
#include "..\physics\physicsWorldUtils.h"
#include "..\engine\tickManager.h"
#include "..\engine\rigidMeshComponent.h"
#include "..\engine\renderFrame.h"
#include "../physics/physicsWrapper.h"
#include "..\engine\globalEventsManager.h"
#include "..\engine\skeleton.h"
#include "..\engine\animatedComponent.h"
#include "..\engine\behaviorGraphContext.h"
#include "..\engine\renderVertices.h"
#include "..\engine\dynamicColliderComponent.h"
#include "..\engine\dynamicCollisionCollector.h"
#include "..\engine\dynamicLayer.h"
#include "..\engine\debugWindowsManager.h"
#include "..\physics\physicsSimpleBodyWrapper.h"
#include "..\physics\physicsIncludes.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CBoatComponent );
RED_DEFINE_STATIC_NAME( OnGatherBoatInput );

//////////////////////////////////////////////////////////////////////////

SBoatGear CBoatComponent::m_sGearZero( 0, BoatConfig::cvBoatSailingGearOneAcceleration.Get(), BoatConfig::cvBoatSailingGearOneDeceleration.Get() );

//////////////////////////////////////////////////////////////////////////

CBoatComponent::CBoatComponent()
    :   m_hBoatBodyRigidMesh()
    ,   m_hDestructionComponent()
    ,   m_subSystemsInitialized( false )
    ,   m_buoyancySystem( nullptr )
    ,   m_hedgehogSystem( nullptr )
    ,   m_inputSystem( nullptr )
    ,   m_pathfindingSystem( nullptr )
    ,   m_sailDir( 0.45f )
    ,   m_isDismountRequested( false )
    ,   m_mountAnimationFinished( false )
    ,   m_isInCutsceneMode( false )
    ,   m_serializationStruct( nullptr )
    ,   m_yTestOffset(0)
    ,   m_drowningShutdownTimeout( 0.0f )
    ,   m_shouldSaveSet(false)
    ,   m_isSpawnedFromQuestBlock(false)
    ,   m_wasWrapperUnstreamLocked(false)
    ,   m_waitForTeleport(false)
	,	m_driverDismounted( false )
{
    m_turningYTiltDamper.Setup( 0, 0, BoatConfig::cvBoatTiltingYSpeed.Get(), EET_Linear );

    m_currentLOD = ILODable::LOD_2; // to init by LOD0 init
}

//////////////////////////////////////////////////////////////////////////
CBoatComponent::~CBoatComponent()
{
    DeinitializeSubsystems();
}

//////////////////////////////////////////////////////////////////////////

Bool CBoatComponent::IsWaitingForTeleport() const
{
    if( m_buoyancySystem == nullptr )
    {
        return false;
    }

    return m_buoyancySystem->m_firstTick;
}

//////////////////////////////////////////////////////////////////////////

Bool CBoatComponent::InitializeSubsystems( const Box& bbox )
{
    if( m_subSystemsInitialized )
        return true;

    if( m_hBoatBodyRigidMesh.Get() == nullptr )
        return false;

    // Buoyancy
    if( m_buoyancySystem == nullptr )
        m_buoyancySystem = new CBoatBuoyancySystem();

    if( !m_buoyancySystem->Initialize( this, &m_hBoatBodyRigidMesh ) )
    {
        delete m_buoyancySystem;
        m_buoyancySystem = nullptr;
        return false;
    }
//     else
//         m_buoyancySystem->LockXYMovement( true );   // Lock at the beginning

    // Hedgehog
    if( m_hedgehogSystem == nullptr )
        m_hedgehogSystem = new CBoatHedgehog();

    if( !m_hedgehogSystem->Initialize( this, &m_hBoatBodyRigidMesh, &m_hDestructionComponent, bbox ) )
    {
        delete m_hedgehogSystem;
        m_hedgehogSystem = nullptr;
        return false;
    }

    // Input
    if( m_inputSystem == nullptr )
        m_inputSystem = new CBoatInputSystem();

    if( !m_inputSystem->Initialize( this ) )
    {
        delete m_inputSystem;
        m_inputSystem = nullptr;
        return false;
    }

    // Pathfinding
    if( m_pathfindingSystem == nullptr )
        m_pathfindingSystem = new CBoatPathfindingSystem();

    if( !m_pathfindingSystem->Initialize( this, &m_hBoatBodyRigidMesh ) )
    {
        delete m_pathfindingSystem;
        m_pathfindingSystem = nullptr;
        return false;
    }

    // Disable scene removal for wrapper
    if( m_isSpawnedFromQuestBlock && !m_wasWrapperUnstreamLocked )
    {
        // Remove wrapper unloading from scene if entity is not streamed    
        if( GetEntity()->CheckDynamicFlag( EDF_StreamingLocked ) )
            return true;

        CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();
        if( wrapper == nullptr )
            return false;

        CPhysicsWorld* physicalWorld = nullptr;
        if( GetWorld() == nullptr || !GetWorld()->GetPhysicsWorld( physicalWorld ) )
        {
            return false;
        }

        SWrapperContext* context = physicalWorld->GetWrappersPool< CPhysicsSimpleWrapper, SWrapperContext >()->GetContextAt( wrapper->GetPoolIndex() );
        if( context == nullptr )
            return false;

        context->m_desiredDistanceSquared = FLT_MAX;
        m_wasWrapperUnstreamLocked = true;
    }

    m_subSystemsInitialized = true;

#ifdef DEBUG_WATER_TRACE
    CWorld* world = GetWorld();
    if( world == nullptr )
        return true;

    Vector2 extent = bbox.CalcSize() * 2.0f;
    extent.X *=2.0f;

    const Float xFrag = extent.X / (Float)TRACE_SIZE_X;
    const Float yFrag = extent.Y / (Float)TRACE_SIZE_Y;

    for( Uint32 x=0; x<TRACE_SIZE_X; ++x )
    {
        for( Uint32 y=0; y<TRACE_SIZE_Y; ++y )
        {
            Vector& point = m_debugWaterTrace[ y + (TRACE_SIZE_X * x) ];

            point = extent * -0.5f;
            point.X += x * xFrag;
            point.Y += y * yFrag;
            point.Z = 0.0f;
        }
    }
#endif

#ifdef USE_BOAT_DEBUG_PAGES
    // Register this boat to debug pages
    DebugWindows::CDebugWindowBoatSettings* window = GDebugWin::GetInstance().GetDebugWindow<DebugWindows::CDebugWindowBoatSettings>( DebugWindows::DW_BoatSettings );
    if( window )
        window->RegisterBoat( this );
#endif

    m_subSystemsInitialized = true;
    return true;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::DeinitializeSubsystems()
{
    if( m_buoyancySystem != nullptr )
    {
        m_buoyancySystem->Deinitialize();
        delete m_buoyancySystem;
        m_buoyancySystem = nullptr;
    }

    if( m_inputSystem != nullptr )
    {
        m_inputSystem->Deinitialize();
        delete m_inputSystem;
        m_inputSystem = nullptr;
    }

    if( m_pathfindingSystem != nullptr )
    {
        m_pathfindingSystem->Deinitialize();
        delete m_pathfindingSystem;
        m_pathfindingSystem = nullptr;
    }

#ifdef USE_BOAT_DEBUG_PAGES
    // Deregister debug pages
    if( m_subSystemsInitialized )
    {
        DebugWindows::CDebugWindowBoatSettings* window = GDebugWin::GetInstance().GetDebugWindow<DebugWindows::CDebugWindowBoatSettings>( DebugWindows::DW_BoatSettings );
        if( window )
            window->DeRegisterBoat( this );
    }
#endif

    m_subSystemsInitialized = false;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnAttached( CWorld* world )
{
    TBaseClass::OnAttached( world );

    PC_SCOPE_PIX( CBoatComponent_OnAttached );

    // If is on dynamic layer we should save it
    CEntity* ent = GetEntity();
    if( ent != nullptr )
    {
        CLayer* layer = ent->GetLayer();
        if( layer != nullptr )
        {
            if( layer->IsA<CDynamicLayer>() )
                SetShouldSave( true );
        }
    }

    // Remove from super class
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );
    world->GetTickManager()->RemoveFromGroup( this, TICK_Main );

    // Set up gears
    m_sailSpeeds.Clear();

    SBoatGear gearOne;
    gearOne.maxSpeed         = BoatConfig::cvBoatSailingGearOneMaxSpeed.Get();
    gearOne.acceleration     = BoatConfig::cvBoatSailingGearOneAcceleration.Get();
    gearOne.deceleration     = BoatConfig::cvBoatSailingGearOneDeceleration.Get();
    m_sailSpeeds.PushBack( gearOne );

    SBoatGear gearTwo;
    gearTwo.maxSpeed         = BoatConfig::cvBoatSailingGearTwoMaxSpeed.Get();
    gearTwo.acceleration     = BoatConfig::cvBoatSailingGearTwoAcceleration.Get();
    gearTwo.deceleration     = BoatConfig::cvBoatSailingGearTwoDeceleration.Get();
    m_sailSpeeds.PushBack( gearTwo );

    SBoatGear gearThree;
    gearThree.maxSpeed       = BoatConfig::cvBoatSailingGearThreeMaxSpeed.Get();
    gearThree.acceleration   = BoatConfig::cvBoatSailingGearThreeAcceleration.Get();
    gearThree.deceleration   = BoatConfig::cvBoatSailingGearThreeDeceleration.Get();
    m_sailSpeeds.PushBack( gearThree );

    SBoatGear gearReverse;
    gearReverse.maxSpeed     = BoatConfig::cvBoatSailingGearReverseMaxSpeed.Get();
    gearReverse.acceleration = BoatConfig::cvBoatSailingGearReverseAcceleration.Get();
    gearReverse.deceleration = BoatConfig::cvBoatSailingGearReverseDeceleration.Get();
    m_sailSpeeds.PushBack( gearReverse );

    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatSailing );
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatHedgehog );
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatBuoyancy );
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatPathFollowing );
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatInput );
#ifdef DEBUG_WATER_TRACE
    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatWaterProbbing );
#endif

    CallEvent( CNAME( OnComponentAttached ) );

	m_currentLOD = ILODable::LOD_2; // to init by LOD0 init
    world->GetComponentLODManager().Register( this );

	m_driverDismounted = false;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnDetached( CWorld* world )
{
    TBaseClass::OnDetached( world );

    world->GetComponentLODManager().Unregister( this );

	world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
    world->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );

    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatSailing );
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatHedgehog );
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatBuoyancy );
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatPathFollowing );
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatInput );
#ifdef DEBUG_WATER_TRACE
    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatWaterProbbing );
#endif

    m_hBoatBodyRigidMesh = nullptr;

    DeinitializeSubsystems();
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
    TBaseClass::OnParentAttachmentAdded(attachment);

    CNode* parent = attachment->GetParent();

    if( parent->IsA<CRigidMeshComponent>() )
    {
        m_hBoatBodyRigidMesh = (CRigidMeshComponent*)parent;
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
    TBaseClass::OnParentAttachmentBroken(attachment);
    CNode* parent = attachment->GetParent();

    if( m_hBoatBodyRigidMesh.Get() != nullptr )
    {
        if( m_hBoatBodyRigidMesh.Get() == parent )
        {
            // Almost all subsystems use boat body mesh/wrapper, deinitialize them
            DeinitializeSubsystems();

            m_hBoatBodyRigidMesh = nullptr;
        }
    }
}

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME(boatStateSave);
RED_DEFINE_STATIC_NAME(boatGlobalPosition);
RED_DEFINE_STATIC_NAME(boatGlobalRotation);
RED_DEFINE_STATIC_NAME(boatLinearVelocity);
RED_DEFINE_STATIC_NAME(boatAngularVelocity);
RED_DEFINE_STATIC_NAME(boatDismountRequested);
RED_DEFINE_STATIC_NAME(boatDrowningShutdownTimeout);

RED_DEFINE_STATIC_NAME(inputSteerTargetAngle);
RED_DEFINE_STATIC_NAME(inputCurrentGear);

RED_DEFINE_STATIC_NAME(avoidanceSlowDownRatio);
RED_DEFINE_STATIC_NAME(avoidanceForceApplyTimeout);
RED_DEFINE_STATIC_NAME(avoidanceLocalForceApplyPos);
RED_DEFINE_STATIC_NAME(avoidanceForceValue);
RED_DEFINE_STATIC_NAME(avoidanceUpdateRotationDir);
RED_DEFINE_STATIC_NAME(avoidanceRotationDir);

RED_DEFINE_STATIC_NAME(buoyancyIsDrowning);
RED_DEFINE_STATIC_NAME(buoyancyDrowningTimeSoFar);
RED_DEFINE_STATIC_NAME(buoyancyTiltAnglePerc);
RED_DEFINE_STATIC_NAME(buoyancyNumBuoyants);

RED_DEFINE_STATIC_NAME(dockingIgnoreShoreAngle);

RED_DEFINE_STATIC_NAME(pathFindingSystem);
RED_DEFINE_STATIC_NAME(pathFindingEnabled);

RED_DEFINE_STATIC_NAME(pathFindingCurveLocal);
RED_DEFINE_STATIC_NAME(pathFindingEndPos);
RED_DEFINE_STATIC_NAME(pathFindingEndTangent);

RED_DEFINE_STATIC_NAME(pathFindingTimeOnCurve);
RED_DEFINE_STATIC_NAME(pathFindingCurveLocalToWorld);
RED_DEFINE_STATIC_NAME(pathFindingNextCurveNodePos);
RED_DEFINE_STATIC_NAME(pathFindingNextCurveNodeTangent);
RED_DEFINE_STATIC_NAME(pathFindingUseOutOfFrustumTeleport);
RED_DEFINE_STATIC_NAME(pathFindingCurvesEntityTag);

RED_DEFINE_STATIC_NAME(boatHedgehogSystem);
RED_DEFINE_STATIC_NAME(hedgeLastWaterPose);
RED_DEFINE_STATIC_NAME(hedgeTerrainRescueMode);
RED_DEFINE_STATIC_NAME(hedgeRaysNumber);
RED_DEFINE_STATIC_NAME(hedgeRayHitDistance);

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnSaveGameplayState_FallbackHack( IGameSaver* saver )
{
	Bool useFallback( true );
	saver->WriteValue( CNAME( Fallback ), useFallback );
	saver->WriteValue( CNAME( boatGlobalPosition ), GetEntity()->GetWorldPosition() );
    saver->WriteValue( CNAME( boatGlobalRotation ), GetEntity()->GetWorldRotation() );
}

void CBoatComponent::OnSaveGameplayState( IGameSaver* saver )
{
    TBaseClass::OnSaveGameplayState( saver );

    if (	false == m_subSystemsInitialized
		||	nullptr == m_hBoatBodyRigidMesh.Get()
		||	!IsValidObject( m_hBoatBodyRigidMesh ) )
	{
		OnSaveGameplayState_FallbackHack( saver );
        return;
	}

    CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();
    if ( nullptr == wrapper )
    {
		OnSaveGameplayState_FallbackHack( saver );
        return;
	}

	Bool useFallback( false );
	saver->WriteValue( CNAME( Fallback ), useFallback );

    Vector linear;
    Vector angular;
    wrapper->GetVelocity( linear, angular );

    const Matrix globalPose = wrapper->GetPose();

    {
		CGameSaverBlock block( saver, CNAME( boatStateSave ) );

        saver->WriteValue( CNAME(boatGlobalPosition),           globalPose.GetTranslationRef() );
        saver->WriteValue( CNAME(boatGlobalRotation),           globalPose.ToEulerAngles() );
        saver->WriteValue( CNAME(boatLinearVelocity),           linear );
        saver->WriteValue( CNAME(boatAngularVelocity),          angular );
        saver->WriteValue( CNAME(boatDismountRequested),        m_isDismountRequested );
        saver->WriteValue( CNAME(boatDrowningShutdownTimeout),  m_drowningShutdownTimeout );

        // Input
        ASSERT(m_inputSystem);
        m_inputSystem->m_speedDamper.OnSaveGameplayState(saver);
        saver->WriteValue( CNAME(inputSteerTargetAngle),            m_inputSystem->m_steerTargetAngleDeg );
        saver->WriteValue( CNAME(inputCurrentGear),                 m_inputSystem->m_currentGear );

        // Buoyancy
        ASSERT(m_buoyancySystem);
        saver->WriteValue( CNAME(buoyancyIsDrowning),           m_buoyancySystem->m_isDrowning );
        saver->WriteValue( CNAME(buoyancyDrowningTimeSoFar),    m_buoyancySystem->m_drowningTimeSoFar );
        saver->WriteValue( CNAME(buoyancyTiltAnglePerc),        m_buoyancySystem->m_tiltAnglePerc );
        saver->WriteValue( CNAME(buoyancyNumBuoyants),          MAX_WATER_CONTACT_POINTS );

        // Save buoyant points status
        for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i )
        {
            m_buoyancySystem->m_buoyancyPoints[i].SaveState(saver);
        }
        

        // Path finding
        ASSERT( m_pathfindingSystem );
        {
			CGameSaverBlock block( saver, CNAME( pathFindingSystem ) );

            if( m_pathfindingSystem->m_isPathFinding && m_pathfindingSystem->m_curve != nullptr )
            {
                saver->WriteValue( CNAME(pathFindingEnabled), true );

                // code generated curve used to move to position
                if( !m_pathfindingSystem->m_externalCurve )
                {
                    saver->WriteValue( CNAME(pathFindingCurveLocal), false );

                    Vector endPosition;
                    m_pathfindingSystem->m_curve->GetPosition( 1.0f, endPosition );
                    saver->WriteValue( CNAME(pathFindingEndPos), endPosition );

                    Vector tangent;
                    m_pathfindingSystem->m_curve->GetPosition( 0.95f, tangent );
                    tangent = ( endPosition - tangent ).Normalized3();

                    saver->WriteValue( CNAME(pathFindingEndTangent), tangent );
                }
                else // Curve on level
                {
                    saver->WriteValue( CNAME(pathFindingCurveLocal), true );

                    CNode* parentNode = m_pathfindingSystem->m_curve->GetParent();

                    Bool success = false;
                    if( parentNode != nullptr )
                    {
                        // Save curve entity
                        if( parentNode->IsA<CComponent>() )
                        {
                            CComponent* curveComp = (CComponent*)parentNode;
                            CEntity* curveEntity = curveComp->GetEntity();

                            if( curveEntity != nullptr )
                            {
                                // Get tag
                                const TagList& tagList = curveEntity->GetTags();

                                if( !tagList.Empty() )
                                {
                                    success = true;
                                    saver->WriteValue( CNAME(pathFindingCurvesEntityTag), tagList.GetTag(0) );
                                }
                            }
                        }
                    }

                    if( !success )
                        saver->WriteValue( CNAME(pathFindingCurvesEntityTag), CName::NONE );
                }

                saver->WriteValue( CNAME(pathFindingTimeOnCurve),              m_pathfindingSystem->m_timeOnCurve );
                saver->WriteValue( CNAME(pathFindingCurveLocalToWorld),        m_pathfindingSystem->m_curveLocalToWorld );
                saver->WriteValue( CNAME(pathFindingNextCurveNodePos),         m_pathfindingSystem->m_nextCurveNodePos );
                saver->WriteValue( CNAME(pathFindingNextCurveNodeTangent),     m_pathfindingSystem->m_nextCurveNodeTangent );
                saver->WriteValue( CNAME(pathFindingUseOutOfFrustumTeleport),  m_pathfindingSystem->m_useOutOfFrustumTeleport );
            }
            else
            {
                saver->WriteValue( CNAME(pathFindingEnabled),       false );
            }
        }

        // Save hedgehog state
        ASSERT(m_hedgehogSystem);
        {
			CGameSaverBlock block( saver, CNAME( boatHedgehogSystem ) );

            saver->WriteValue( CNAME(hedgeLastWaterPose), m_hedgehogSystem->m_lastWaterPose );
            saver->WriteValue( CNAME(hedgeTerrainRescueMode), m_hedgehogSystem->m_terrainRescueMode );

            const Uint32 rayNum = m_hedgehogSystem->m_raycastsList.Size();
            saver->WriteValue( CNAME(hedgeRaysNumber), rayNum );

            for( Uint32 i=0; i<rayNum; ++i )
            {
                saver->WriteValue( CNAME(hedgeRayHitDistance), m_hedgehogSystem->m_raycastsList[i].hitDistance );
            }

            m_hedgehogSystem->m_repelDamper.OnSaveGameplayState(saver);
            m_hedgehogSystem->m_inputDamper.OnSaveGameplayState(saver);
            m_hedgehogSystem->m_raycastLengthDamper.OnSaveGameplayState(saver);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CBoatComponent::OnLoadGameplayState_FallbackHack( IGameLoader* loader )
{
	Vector			boatGlobalPosition;
	EulerAngles		boatGlobalRotation;

	loader->ReadValue( CNAME(boatGlobalPosition), boatGlobalPosition );
    loader->ReadValue( CNAME(boatGlobalRotation), boatGlobalRotation );	

	ASSERT( boatGlobalPosition.IsOk() && false == Vector::Equal3( boatGlobalPosition, Vector::ZERO_3D_POINT ) );

	GetEntity()->SetPosition( boatGlobalPosition );
    GetEntity()->SetRotation( boatGlobalRotation );
}

void CBoatComponent::OnLoadGameplayState( IGameLoader* loader )
{
    TBaseClass::OnLoadGameplayState( loader );

    if ( nullptr != m_serializationStruct )
	{
        return;
	}

	Bool useFallback( false );
	if ( loader->GetSaveVersion() >= SAVE_VERSION_BOAT_SAVING_FALLBACK )
	{
		loader->ReadValue( CNAME( Fallback ), useFallback );
	}

	if ( useFallback )
	{
		OnLoadGameplayState_FallbackHack( loader );
		return;
	}

    m_serializationStruct = new SBoatSerialization();
    m_serializationStruct->saveVersion = loader->GetSaveVersion();

    {
		CGameSaverBlock block( loader, CNAME( boatStateSave ) );

        // Wrapper
        loader->ReadValue( CNAME(boatGlobalPosition),           m_serializationStruct->boatGlobalPosition );
        loader->ReadValue( CNAME(boatGlobalRotation),           m_serializationStruct->boatGlobalRotation );
        loader->ReadValue( CNAME(boatLinearVelocity),           m_serializationStruct->boatLinearVelocity );
        loader->ReadValue( CNAME(boatAngularVelocity),          m_serializationStruct->boatAngularVelocity );
        loader->ReadValue( CNAME(boatDismountRequested),        m_serializationStruct->boatIsDismountRequested );
        
        if( loader->GetSaveVersion() >= SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
        {
            loader->ReadValue( CNAME(boatDrowningShutdownTimeout),  m_serializationStruct->boatDrowningShutdownTimeout );
        }

        // Input
        m_serializationStruct->inputSpeedDamper.OnLoadGameplayState(loader);
        loader->ReadValue( CNAME(inputSteerTargetAngle),            m_serializationStruct->inputSteerTargetAngle );
        loader->ReadValue( CNAME(inputCurrentGear),                 m_serializationStruct->inputCurrentGear );

        if( loader->GetSaveVersion() < SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
        {
            Float dummy = 0;
            // Avoidance
            loader->ReadValue( CNAME(avoidanceSlowDownRatio),       dummy );
            loader->ReadValue( CNAME(avoidanceForceApplyTimeout),   dummy );
            loader->ReadValue( CNAME(avoidanceLocalForceApplyPos),  dummy );
            loader->ReadValue( CNAME(avoidanceForceValue),          dummy );
            loader->ReadValue( CNAME(avoidanceUpdateRotationDir),   dummy );
            loader->ReadValue( CNAME(avoidanceRotationDir),         dummy );
        }

        // Buoyancy
        loader->ReadValue( CNAME(buoyancyIsDrowning),           m_serializationStruct->buoyancyIsDrowning );
        loader->ReadValue( CNAME(buoyancyDrowningTimeSoFar),    m_serializationStruct->buoyancyDrowningTimeSoFar );
        loader->ReadValue( CNAME(buoyancyTiltAnglePerc),        m_serializationStruct->buoyancyTiltAnglePerc );

        if( loader->GetSaveVersion() >= SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
        {
            Uint32 numBuoyants = 0;
            loader->ReadValue( CNAME(buoyancyNumBuoyants),          numBuoyants );
        
            if( numBuoyants == MAX_WATER_CONTACT_POINTS )
            {
                for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i )
                {
                    m_serializationStruct->waterContactPoints[i].LoadState(loader);
                }
            }
            else
            {
                for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i )
                {
                    // mark that we had wrong save version
                    m_serializationStruct->waterContactPoints[i].m_localOffset.Set3( -1.0f, -1.0f, -1.0f );
                }

                ASSERT( false, TXT("Wrong number of buoyant points saved. Saved: %i, required %i"), numBuoyants, MAX_WATER_CONTACT_POINTS );
            }
        }

        if( loader->GetSaveVersion() < SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
        {
            CFloatDamper dummyDamp;
            dummyDamp.OnLoadGameplayState(loader);
            Float dummy = 0;
            loader->ReadValue( CNAME(dockingIgnoreShoreAngle), dummy );
        }

        // Path finding
        {
			CGameSaverBlock block( loader, CNAME( pathFindingSystem ) );

            loader->ReadValue( CNAME(pathFindingEnabled), m_serializationStruct->pathfindingIsEnabled );

            if( m_serializationStruct->pathfindingIsEnabled )
            {
                // Find curve type
                Bool enabled = false;
                loader->ReadValue( CNAME(pathFindingCurveLocal), enabled );

                // Locally created
                if( !enabled )
                {
                    loader->ReadValue( CNAME(pathFindingEndPos), m_serializationStruct->pathfindingCurveEndPos );
                    loader->ReadValue( CNAME(pathFindingEndTangent), m_serializationStruct->pathfindingCurveEndTangent );
                }
                // Loaded from world
                else
                {
                    loader->ReadValue( CNAME(pathFindingCurvesEntityTag), m_serializationStruct->pathfindingCurvesEntityTag );
                }


                loader->ReadValue( CNAME(pathFindingTimeOnCurve),              m_serializationStruct->pathfindingTimeOnCurve );
                loader->ReadValue( CNAME(pathFindingCurveLocalToWorld),        m_serializationStruct->pathfindingCurveLocalToWorld );
                loader->ReadValue( CNAME(pathFindingNextCurveNodePos),         m_serializationStruct->pathfindingNextCurveNodePos );
                loader->ReadValue( CNAME(pathFindingNextCurveNodeTangent),     m_serializationStruct->pathfindingNextCurveNodeTangent );
                loader->ReadValue( CNAME(pathFindingUseOutOfFrustumTeleport),  m_serializationStruct->pathfindingUseOutOfFrustumTeleport );
            }
        }

        if( loader->GetSaveVersion() >= SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
        {
            // Load hedgehog state
            {
				CGameSaverBlock block( loader, CNAME( boatHedgehogSystem ) );

                loader->ReadValue( CNAME(hedgeLastWaterPose), m_serializationStruct->hedgeLastWaterPose );
                loader->ReadValue( CNAME(hedgeTerrainRescueMode), m_serializationStruct->hedgeTerrainRescueMode );

                Uint32 numSavedRays = 0;
                loader->ReadValue( CNAME(hedgeRaysNumber), numSavedRays );

                if( numSavedRays == RAYCASTS_NUM )
                {
                    m_serializationStruct->hedgeRaycastsDistances.Reserve(numSavedRays);

                    for( Uint32 i=0; i<numSavedRays; ++i )
                    {
                        Float distance = 0.0f;
                        loader->ReadValue( CNAME(hedgeRayHitDistance), distance );
                        m_serializationStruct->hedgeRaycastsDistances.PushBack( distance );
                    }
                }
                else
                {
                    ASSERT( false, TXT("Wrong amount of ray properties saved. Saved: %i, Required: %i"), numSavedRays, RAYCASTS_NUM );
                    // Read to dummy
                    for( Uint32 i=0; i<numSavedRays; ++i )
                    {
                        Float dummy = 0.0f;
                        loader->ReadValue( CNAME(hedgeRayHitDistance), dummy );
                    }
                }

                m_serializationStruct->hedgeRepelDamper.OnLoadGameplayState(loader);
                m_serializationStruct->hedgeInputDamper.OnLoadGameplayState(loader);
                m_serializationStruct->hedgeRaycastLengthDamper.OnLoadGameplayState(loader);
            }
        }
    }

    GetEntity()->SetPosition( m_serializationStruct->boatGlobalPosition );
    GetEntity()->SetRotation(  m_serializationStruct->boatGlobalRotation );

    // If we saved this boat before, we should save it again if need arises
    SetShouldSave( true );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::ResetState()
{
    if( m_inputSystem != nullptr )
    {
        // Reset key state
        m_inputSystem->ResetState();
    }

	if ( m_buoyancySystem )
	{
		m_buoyancySystem->ResetState();
	}
}

//////////////////////////////////////////////////////////////////////////

Int32 CBoatComponent::GetGearFromXYVelocity() const
{
    if( !m_subSystemsInitialized || m_inputSystem == nullptr )
    {
        return 0;
    }

    const Float currVel = GetLinearVelocityXY();
    if( currVel - 0.3f <= 0 )
    {
        return 0;
    }

    for( Int32 i=0; i<MAX_GEARS; ++i)
    {
        if( m_sailSpeeds[i].maxSpeed + 0.5f >= currVel )
        {
            if( i == 0 )
                return m_inputSystem->m_currentGear > 0 ? 1 : -1;
            else
                return i+1;
        }
    }

    return MAX_GEARS;
}

//////////////////////////////////////////////////////////////////////////

Float CBoatComponent::GetMaxVelocityAtCurrentGear() const
{
    Int32 currentGear = GetCurrentGear();
    if( currentGear == 0 )
    {
        return 0.0f;
    }

    return GetGear(currentGear).maxSpeed;
}

//////////////////////////////////////////////////////////////////////////

Int32 CBoatComponent::GetCurrentGear() const
{
    if( !m_subSystemsInitialized || m_inputSystem == nullptr )
    {
        return 0;
    }

    return m_inputSystem->m_currentGear;
}

//////////////////////////////////////////////////////////////////////////

const SBoatGear& CBoatComponent::GetGear( Int32 gear ) const
{
    // Reverse
    if( gear < 0 )
    {
        return m_sailSpeeds.Back();
    }
    
    // Forward
    if( gear > 0 && gear <= MAX_GEARS )
    {
        return m_sailSpeeds[gear-1];
    }

    // Return zero gear (max speed = 0, acceleration and deceleration same as for first gear)
    return m_sGearZero;
}

//////////////////////////////////////////////////////////////////////////

Float CBoatComponent::GetLinearVelocityXY() const
{
    Float retVelocity = 0.0f;

    if( m_hBoatBodyRigidMesh.Get() != nullptr )
    {
        CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();
        if( wrapper != nullptr)
        {
            Vector angular;
            Vector linear;
            wrapper->GetVelocity( linear, angular );

            retVelocity = linear.Mag2();
        }
    }

    return retVelocity;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::RestoreSerializedState( CPhysicsWrapperInterface* wrapper )
{
    if( m_serializationStruct == nullptr )
    {
        return;
    }

    // Wrapper
    ASSERT(wrapper);
    EngineTransform engineTransform(  m_serializationStruct->boatGlobalPosition,  m_serializationStruct->boatGlobalRotation );
    Matrix localToWorld;
    engineTransform.CalcLocalToWorld( localToWorld );
    wrapper->SetPose( localToWorld );
    wrapper->SetVelocityLinear(   m_serializationStruct->boatLinearVelocity );
    wrapper->SetVelocityAngular(  m_serializationStruct->boatAngularVelocity );
    m_isDismountRequested       = m_serializationStruct->boatIsDismountRequested;
    
    if( m_serializationStruct->saveVersion >= SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
    {
        m_drowningShutdownTimeout   = m_serializationStruct->boatDrowningShutdownTimeout;
    }
    
    // Set drogning state if we were drowning durnig save game
    if( m_drowningShutdownTimeout > 0.0f)
    {
        SetStateDrowning();
    }

    // Input
    ASSERT(m_inputSystem);
    m_inputSystem->m_speedDamper            = m_serializationStruct->inputSpeedDamper;
    m_inputSystem->m_steerTargetAngleDeg    = m_serializationStruct->inputSteerTargetAngle;
    m_inputSystem->m_currentGear            = m_serializationStruct->inputCurrentGear;

    // Buoyancy
    ASSERT(m_buoyancySystem);
    m_buoyancySystem->m_isDrowning          = m_serializationStruct->buoyancyIsDrowning;
    m_buoyancySystem->m_drowningTimeSoFar   = m_serializationStruct->buoyancyDrowningTimeSoFar;
    m_buoyancySystem->m_tiltAnglePerc       = m_serializationStruct->buoyancyTiltAnglePerc;
    if( m_buoyancySystem->IsDrowning() )
    {
        m_buoyancySystem->DisableJointTiltLimit();
    }

    if( m_serializationStruct->saveVersion >= SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
    {
        // Assign new buoyant points values
        for( Uint32 i=0; i<MAX_WATER_CONTACT_POINTS; ++i )
        {
            m_buoyancySystem->m_buoyancyPoints[i] = m_serializationStruct->waterContactPoints[i];
        }
    }

    // Restore for path finding
    ASSERT(m_pathfindingSystem);
    if( m_serializationStruct->pathfindingIsEnabled )
    {
        m_pathfindingSystem->RestoreSerializedState( m_serializationStruct );
    }

    if( m_serializationStruct->saveVersion >= SAVE_VERSION_BOAT_SUBSYSTEMS_REMOVAL )
    {
        ASSERT(m_hedgehogSystem);
        m_hedgehogSystem->RestoreSerializedState( m_serializationStruct );
    }

    // Delete struct used for serialization
    delete m_serializationStruct;
    m_serializationStruct = nullptr;
}

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( OnDrowningFinished );

void CBoatComponent::OnTickPrePhysics( Float deltaTime )
{
    PC_SCOPE( BoatComponentUpdate );

    RED_ASSERT( ( m_currentLOD == ILODable::LOD_0 ), TXT( "This tick should only be performed in LOD 0" ) );

    // Don't call this. It calls event 'OnTick' in scripts, that is invoked at the end of this method
    //TBaseClass::OnTick( deltaTime );

    if( m_isInCutsceneMode || m_waitForTeleport )
        return;

    CEntity* ent = GetEntity();

    // Do not process if it is not in game
    if( ent == nullptr || !ent->IsInGame() )
    {
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Is boat body mesh present
    if( m_hBoatBodyRigidMesh.Get() == nullptr )
    {
        CBoatBodyComponent* body = ent->FindComponent<CBoatBodyComponent>();
        m_hBoatBodyRigidMesh = (CRigidMeshComponent*)body;
    }

    // Check is valid object
    if( m_hBoatBodyRigidMesh.Get() == nullptr || !IsValidObject( m_hBoatBodyRigidMesh ) )
    {
        m_hBoatBodyRigidMesh = nullptr;
        m_subSystemsInitialized = false;
        return;
    }

    //////////////////////////////////////////////////////////////////////////
    // Is boat body mesh present
    if( m_hDestructionComponent.Get() == nullptr )
    {
        CBoatDestructionComponent* body = ent->FindComponent<CBoatDestructionComponent>();
        m_hDestructionComponent = body;
    }

    // Check is valid object
    if( m_hDestructionComponent.Get() == nullptr || !IsValidObject( m_hDestructionComponent ) )
    {
        m_hDestructionComponent = nullptr;
        m_subSystemsInitialized = false;
        return;
    }    

    CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();
    ASSERT( wrapper );
    if( wrapper == nullptr )
    {
        return;
    }

    CMesh* boatMesh =  m_hBoatBodyRigidMesh->GetMeshNow();
    if(boatMesh == nullptr )
    {
        return;
    }

    const Box bbox = boatMesh->GetBoundingBox();

    // Initialize boats subsystems
    if( !InitializeSubsystems( bbox ) )
    {
        return;
    }

    // Restore boat state after load
    RestoreSerializedState( wrapper );

    //////////////////////////////////////////////////////////////////////////
    // Countdown till all systems turn off
    if( m_drowningShutdownTimeout > 0.0f )
    {
        m_drowningShutdownTimeout -= deltaTime;

        // Shutdown
        if( m_drowningShutdownTimeout < 0.0f )
        {
            m_drowningShutdownTimeout = 0.0f;

            // Deinitialize all subsystems
            DeinitializeSubsystems();

            // Switch to kinematic
            wrapper->SwitchToKinematic( true );

            // Remove from physics tick
			CWorld* world = GetWorld();
			world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
            world->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );

            // Call scripts
            CallEvent( CNAME(OnDrowningFinished) );
            return;
        }
    }
    //////////////////////////////////////////////////////////////////////////

    // Force set entity pose update from boat body wrapper
    Bool isVeryWrong = false;
    if( !wrapper->GetFlag( PRBW_UpdateEntityPose ) )
    {
        isVeryWrong = true;
        wrapper->SetFlag( PRBW_UpdateEntityPose, true );
    }

    if( !wrapper->GetFlag( PRBW_DisableBuoyancy ) )
    {
        isVeryWrong = true;
        wrapper->SetFlag( PRBW_DisableBuoyancy, true );
    }

    if( !wrapper->GetFlag( PRBW_DisableGravity ) )
    {
        isVeryWrong = true;
        wrapper->SetFlag( PRBW_DisableGravity, true );
    }

    // Be absolutely sure that engine does what we want it to do
    if( isVeryWrong )
    {
        // Fix it boat status
        wrapper->SetVelocityLinear( Vector::ZEROS );
        wrapper->SetVelocityAngular( Vector::ZEROS );

        OnDebugPageClosed();
    }


    // Physics pose transforms
    const Matrix globalPose = wrapper->GetPose();
    Matrix globalRotation = globalPose;
    globalRotation.SetTranslation( Vector::ZEROS );

    // Remove tilt from global rotation matrix
    Matrix sweepRotation = globalRotation;
    sweepRotation.V[2].Set3( 0.0f, 0.0f, 1.0f );                                                // Z
    sweepRotation.V[1] = Vector::Cross( sweepRotation.V[2], sweepRotation.V[0] ).Normalized3(); // Y = ( Z.cross(X) ).Normalized();
    sweepRotation.V[0] = Vector::Cross( sweepRotation.V[1], sweepRotation.V[2] );               // X = Y.cross(Z)

    // Bounding box and extents for sweep test
    const Vector sweepExtent = bbox.CalcExtents();

#ifndef FINAL
    DBG_centerOfMassPose = globalPose;
    DBG_centerOfMassPose.SetTranslation( wrapper->GetCenterOfMassPosition() );

    DBG_sweepRotation = sweepRotation;
#endif

#ifndef FINAL
    //////////////////////////////////////////////////////////////////////////
    // Get boat status
    Vector linearVelocity;
    Vector angularVelocity;
    wrapper->GetVelocity( linearVelocity, angularVelocity );

    DBG_linearSpeed2D = linearVelocity.Mag2();
    DBG_localLinearSpeed2D = globalRotation.Inverted().TransformVector(linearVelocity).Y;
    DBG_angularSpeedZ = angularVelocity.Z;
#endif

    //////////////////////////////////////////////////////////////////////////
    // buoyancy
    {
        PC_SCOPE( BoatBuoyancyUpdate );

        Vector buoyancyImpulse = Vector::ZEROS;
        Vector torqueImpulse = Vector::ZEROS;

        m_buoyancySystem->BuoyancyUpdate( buoyancyImpulse, torqueImpulse, deltaTime, wrapper );

        wrapper->ApplyImpulse( buoyancyImpulse * deltaTime, wrapper->GetPose().GetTranslation() );
        wrapper->ApplyTorqueImpulse( torqueImpulse * deltaTime );
    }
    
    //////////////////////////////////////////////////////////////////////////
    // Steering
    if(  m_user.Get() != nullptr )
    {
        // Remove lock if somehow it is still set
        if( m_buoyancySystem->IsXYLocked() )
        {
            m_buoyancySystem->LockXYMovement( false );
        }

        // Save boat only when user was mounted at least once
        // We should save boat any time anyone sits in it
        if( !m_shouldSaveSet )
        {
            m_shouldSaveSet = true;
            SetShouldSave( true );
        }

        // Process input
        if( !m_isDismountRequested && m_mountAnimationFinished && !m_buoyancySystem->IsDrowning() )
        {
            if( GGame->GetPlayerEntity() == m_user.Get() && BoatConfig::cvBoatSailingEnabled.Get() )
            {
                SInputAction accelerate;
                SInputAction decelerate;
                Vector stickTilt;

                // Cal input gathering from scripts
                if( BoatConfig::driverBoatLocalSpaceCamera.Get() )
                {
                    // Redirect input gathering to scripts to get current camera properties
                    CallEvent( CNAME(OnGatherBoatInput) );
                }
                else
                {
                    // Get input directly, no need to concern about the camera
                    m_inputSystem->GatherInputGlobalSpaceCamera();
                }
            }

            if( m_pathfindingSystem->m_isPathFinding )
            {
                Int32 targetGear = m_inputSystem->m_currentGear;
                Float targetAnglePerc = 0.0f;

                m_pathfindingSystem->PathFindingUpdate( wrapper, targetAnglePerc, targetGear );

                m_inputSystem->SteerSetGear( targetGear );

                // Get max steer angle at this speed
                const Float speedRatio = fabsf( m_inputSystem->GetTargetSpeed() / GetGear(MAX_GEARS).maxSpeed );
                const Float maxSteerAngle = BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() + ( speedRatio * ( BoatConfig::cvBoatTurningSecondSpeedSteerAngle.Get() - BoatConfig::cvBoatTurningFristSpeedSteerAngle.Get() ) );

                // Set new turning target angle
                if( fabsf( targetAnglePerc ) < 0.5f )
                {
                    targetAnglePerc *= 2.0f;
                    m_inputSystem->m_steerTargetAngleDeg = targetAnglePerc * maxSteerAngle;
                }
                else    // Set max turning speed when angle is > 45deg
                {
                    m_inputSystem->m_steerTargetAngleDeg = maxSteerAngle * (Float)Sgn(targetAnglePerc);
                }
            }
        }// Is sailor handle / not dismounting
    }

    //////////////////////////////////////////////////////////////////////////
    // Hedgehog
    if( (m_user.Get() != nullptr && m_mountAnimationFinished) || m_buoyancySystem->IsDrowning() )
    {
        PC_SCOPE( BoatHedgehogUpdate );
        m_hedgehogSystem->UpdateHedgehog( deltaTime, m_buoyancySystem->m_noBuoyantsOnTerrain );

        // Slow down input
        m_inputSystem->LimitSpeed( m_hedgehogSystem->m_inputDamper.GetInterpolatedValue() );
    }

    //////////////////////////////////////////////////////////////////////////
    // Input
    Vector2 propellForce = Vector::ZEROS;
    if( m_user.Get() != nullptr && m_mountAnimationFinished && !m_buoyancySystem->IsDrowning() )
    {
        PC_SCOPE( BoatInputUpdate );

        m_inputSystem->InputUpdate
            (
              propellForce
            , deltaTime
            , globalRotation
            , wrapper
            , m_pathfindingSystem->m_isPathFinding
            );
    }

    //////////////////////////////////////////////////////////////////////////
    // Update sail angle and wind force tilting
    if( m_user != nullptr && m_mountAnimationFinished )
    {
        UpdateSailAngleAndWindTilt( deltaTime, wrapper, globalRotation );
    }

    //////////////////////////////////////////////////////////////////////////
    // Apply forces from input
        
    // Split force along boats local axis
    Vector rotationForce = globalRotation.Inverted().TransformVector( propellForce );
    propellForce = globalRotation.TransformVector( Vector( 0.0f, rotationForce.Y, rotationForce.Z ) ); // Over Y axis
    rotationForce = globalRotation.TransformVector( Vector( rotationForce.X, 0.0f, rotationForce.Z ) ); // Over X axis

    // Apply propelling force
    wrapper->ApplyImpulse( propellForce * deltaTime, wrapper->GetCenterOfMassPosition() );

    // Apply turning force on front or back of the boat
    if( m_inputSystem->m_currentGear > 0 )
    {
        const Vector forwardTurningForceAppPos = wrapper->GetCenterOfMassPosition() - globalPose.GetAxisY() * BoatConfig::cvBoatTurningForcePosition.Get();
        wrapper->ApplyImpulse( -rotationForce * deltaTime, forwardTurningForceAppPos );
    }
    else
    {
        const Vector backwardTurningForceAppPos = wrapper->GetCenterOfMassPosition() + globalPose.GetAxisY() * BoatConfig::cvBoatTurningForcePosition.Get();
        wrapper->ApplyImpulse( rotationForce * deltaTime,  backwardTurningForceAppPos );
    }

#ifndef FINAL
    DBG_movementForce = propellForce;
#endif
    
    // Update floating component vars
    CallEvent( CNAME( OnTick ), deltaTime );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::UpdateSailAngleAndWindTilt( Float deltaTime, CPhysicsWrapperInterface* wrapper, const Matrix& globalRotation )
{    
    // Update tilting    
    Vector2 tiltAngle( 0.0f, m_inputSystem->m_stickInput.X );    

    // Cancel tilt if any of those is valid
    if( m_isDismountRequested )
    {
        tiltAngle.Set( 0, 0 );
    }

    Float maxTurningYTiltPerc = 0.0f;

    if( m_inputSystem->m_currentGear == 1 || m_inputSystem->m_currentGear == -1 )
    {
        maxTurningYTiltPerc = BoatConfig::cvBoatTiltingMinYTiltPercent.Get();
    }
    else if( m_inputSystem->m_currentGear == 0 )
    {
        tiltAngle.Set( 0,0 );
    }
    else
    {
        const Float fraction = (Float)m_inputSystem->m_currentGear / (Float)MAX_GEARS;
        maxTurningYTiltPerc = BoatConfig::cvBoatTiltingMinYTiltPercent.Get() + fraction * ( BoatConfig::cvBoatTiltingMaxYTiltPercent.Get() - BoatConfig::cvBoatTiltingMinYTiltPercent.Get() );
    }    

    const Float newYDamperTarget = tiltAngle.Y * maxTurningYTiltPerc;

#ifndef NO_EDITOR
    DBG_tiltDamperTarget = newYDamperTarget;
#endif

    // Y damper
    m_turningYTiltDamper.SetDampingTime( BoatConfig::cvBoatTiltingYSpeed.Get() );
    if( m_turningYTiltDamper.GetDestValue() != newYDamperTarget )
    {
        m_turningYTiltDamper.SetStartValue( m_turningYTiltDamper.GetInterpolatedValue() );
        m_turningYTiltDamper.SetDestValue( newYDamperTarget );
        m_turningYTiltDamper.ResetInterpolationTime();
    }

    tiltAngle.Y = m_turningYTiltDamper.Update( deltaTime );
    m_buoyancySystem->SetLocalTiltAngle( tiltAngle );

#ifndef FINAL
    DBG_tiltYDampedValue = tiltAngle.Y;
#endif
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
    if( m_isInCutsceneMode || !m_subSystemsInitialized )
        return;

    // Is movement locked
    if( flags == SHOW_BoatSailing )
    {
        String txt;
        if( m_buoyancySystem->m_isXYLocked )
            txt.Set( TXT("XY movement LOCKED") );
        else
            txt.Set( TXT("XY movement FREE") );

        frame->AddDebugText( DBG_centerOfMassPose.GetTranslation() + Vector(0,0,2), txt, 0, 0, true );
    }


    //////////////////////////////////////////////////////////////////////////
    if( m_user.Get() == nullptr )
        return;

    //////////////////////////////////////////////////////////////////////////
    // Boat status
    if( flags == SHOW_BoatSailing )
    {   
        m_yTestOffset = 0;

        // Phys linear speed / accel
        frame->AddDebugScreenText( 10, 120
            ,   String::Printf( TXT("Linear velocity 2D  : %.2f"), DBG_linearSpeed2D )
            ,   m_yTestOffset
            ,   true
            );
        
        ++m_yTestOffset;
        frame->AddDebugScreenText( 10, 120
            ,   String::Printf( TXT("Linear velocity Y axis  : %.2f"), DBG_localLinearSpeed2D )
            ,   m_yTestOffset
            ,   true
            );

        ++m_yTestOffset;
        frame->AddDebugScreenText( 10, 120
            ,   String::Printf( TXT("Angular velocity Z: %.2f"), DBG_angularSpeedZ )
            ,   m_yTestOffset
            ,   true
            );

        ++m_yTestOffset;
        frame->AddDebugScreenText( 10, 120
            ,   String::Printf( TXT("Sail mast angle: %f"), m_sailDir )
            ,   m_yTestOffset
            ,   true
            );

#ifndef FINAL
        ++m_yTestOffset;
        frame->AddDebugScreenText( 10, 120
            ,   String::Printf( TXT("Tilt Y damped val: %f"), DBG_tiltYDampedValue )
            ,   m_yTestOffset
            ,   true
            );
#endif

        const Vector globalPos = GetEntity()->GetWorldPosition();
        
#ifndef FINAL
        // Show force vector and position;
        DBG_sweepRotation.SetTranslation( globalPos );

        // Add force attach pos
        Vector forcePos = DBG_sweepRotation.GetTranslation() + DBG_sweepRotation.GetAxisY() * BoatConfig::cvBoatTurningForcePosition.Get();
        frame->AddDebugLine( forcePos, forcePos + Vector(0,0,3), Color::WHITE );

        // Add force arrow
        forcePos += Vector(0,0,3);

        if( m_inputSystem )
        {
            frame->AddDebug3DArrow( forcePos, Vector3(m_inputSystem->DBG_forceDirectionVector), 0.8f, 0.03f, 0.09f, 0.24f, Color::LIGHT_BLUE, Color::LIGHT_BLUE );

            EulerAngles rot( 0, 0, m_inputSystem->DBG_steerAngleLimitMin );
            Matrix rotation = rot.ToMatrix();
            rotation = rotation * DBG_sweepRotation;

            // Current angle limit min
            frame->AddDebug3DArrow( forcePos, rotation.GetAxisY(), 1.0f, 0.015f, 0.045f, 0.3f, Color::LIGHT_GREEN, Color::LIGHT_GREEN );

            rot.Yaw = m_inputSystem->DBG_steerAngleLimitMax;
            rotation = rot.ToMatrix();
            rotation = rotation * DBG_sweepRotation;

            // Current angle limit max
            frame->AddDebug3DArrow( forcePos, rotation.GetAxisY(), 1.0f, 0.015f, 0.045f, 0.3f, Color::LIGHT_GREEN, Color::LIGHT_GREEN );
        }

        Float scale = DBG_movementForce.Normalize3();
        scale = log10f( 1.0f + scale );
        frame->AddDebug3DArrow( forcePos, DBG_movementForce, scale, 0.015f, 0.045f, 0.12f, Color::RED, Color::RED );
#endif
    }

    if( flags == SHOW_BoatHedgehog )
    {
        m_hedgehogSystem->DebugDraw(frame,m_yTestOffset);
    }

    if( flags == SHOW_BoatBuoyancy )
        m_buoyancySystem->DebugDraw( frame, m_yTestOffset );

    if( flags == SHOW_BoatPathFollowing )
        m_pathfindingSystem->DebugDraw( frame, m_yTestOffset );

    if( flags == SHOW_BoatInput )
        m_inputSystem->DebugDraw( frame, m_yTestOffset );

#ifdef DEBUG_WATER_TRACE
    if( flags == SHOW_BoatWaterProbbing )
    {
        CWorld* world = GetWorld();
        if( world != nullptr )
        {
            // Remove XY tilts, only Z rotation
            Matrix ltw = DBG_centerOfMassPose;
            ltw.V[0] = Vector::Cross( ltw.V[1], Vector::EZ );
            ltw.V[1] = Vector::Cross( Vector::EZ, ltw.V[0] );
            ltw.V[2] = Vector::EZ;

            Box c = Box( Vector::ZEROS, 0.25f );

            const Uint32 count = TRACE_SIZE_Y * TRACE_SIZE_X;
            for( Uint32 i=0; i<count; ++i)
            {
                Vector current = m_debugWaterTrace[i];

                // to global
                current = ltw.TransformPoint( current );
                current.Z = world->GetWaterLevel( current, 0, nullptr );

                Matrix m = Matrix::IDENTITY;
                m.SetTranslation( current );

                frame->AddDebugSolidBox( c, m, Color::WHITE );
            }
        }
    }
#endif
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcGetBuoyancyPointStatus_Front( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    Vector ret(0,0,0,0);
    if( m_subSystemsInitialized )
        ret = m_buoyancySystem->m_buoyancyPoints[0].m_globalPositionWaterOnW;

    RETURN_STRUCT( Vector, ret );
}

void CBoatComponent::funcGetBuoyancyPointStatus_Back( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    Vector ret(0,0,0,0);
    if( m_subSystemsInitialized )
        ret = m_buoyancySystem->m_buoyancyPoints[1].m_globalPositionWaterOnW;

    RETURN_STRUCT( Vector, ret );
}

void CBoatComponent::funcGetBuoyancyPointStatus_Right( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    Vector ret(0,0,0,0);
    if( m_subSystemsInitialized )
        ret = m_buoyancySystem->m_buoyancyPoints[2].m_globalPositionWaterOnW;

    RETURN_STRUCT( Vector, ret );
}

void CBoatComponent::funcGetBuoyancyPointStatus_Left( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    Vector ret(0,0,0,0);
    if( m_subSystemsInitialized )
        ret = m_buoyancySystem->m_buoyancyPoints[3].m_globalPositionWaterOnW;

    RETURN_STRUCT( Vector, ret );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::LockXYMovement( Bool lockMovement )
{
    if( m_buoyancySystem == nullptr )
    {
        return;
    }

    m_buoyancySystem->LockXYMovement( lockMovement );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::BoatBodyStruck( const Vector2& globalForce, const Vector& globalHit, EBoatCollisionSource collisionSource )
{
    // Slow down boat if its body collision    
    m_inputSystem->BoatBodyCollision();

    if( m_hBoatBodyRigidMesh.Get() == nullptr )
    {
        return;
    }

    CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();

    static Float upForceScaller = 3.0f;

    const Float forceScaller = BoatConfig::cvBoatMass.Get() * BoatConfig::cvBoatDestructionHitRepelForceMultiplier.Get();
    const Int32 gear = GetGearFromXYVelocity();

    Vector totalForce = Vector::EZ * BoatConfig::cvBoatMass.Get() * upForceScaller * (Float)gear/(Float)MAX_GEARS;

    // Add repel impulse when it's physx collision
    if( collisionSource == EBCS_PhysxCollider )
    {
        totalForce += globalForce * forceScaller;
    }

    wrapper->ApplyImpulse( totalForce, globalHit );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnDebugPageClosed()
{
    if( m_hBoatBodyRigidMesh.Get() == nullptr )
    {
        return;
    }

    CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();
    if( wrapper == nullptr)
        return;

    // Set damping
    if( wrapper->GetDampingLinear() != BoatConfig::cvBoatLinearDamping.Get() )
        wrapper->SetDampingLinear( BoatConfig::cvBoatLinearDamping.Get() );

    if( wrapper->GetDampingAngular() != BoatConfig::cvBoatAngularDamping.Get() )
        wrapper->SetDampingAngular( BoatConfig::cvBoatAngularDamping.Get() );

    // Change gear values
    SBoatGear& gearOne = m_sailSpeeds[0];
    gearOne.maxSpeed         = BoatConfig::cvBoatSailingGearOneMaxSpeed.Get();
    gearOne.acceleration     = BoatConfig::cvBoatSailingGearOneAcceleration.Get();
    gearOne.deceleration     = BoatConfig::cvBoatSailingGearOneDeceleration.Get();

    SBoatGear& gearTwo = m_sailSpeeds[1];
    gearTwo.maxSpeed         = BoatConfig::cvBoatSailingGearTwoMaxSpeed.Get();
    gearTwo.acceleration     = BoatConfig::cvBoatSailingGearTwoAcceleration.Get();
    gearTwo.deceleration     = BoatConfig::cvBoatSailingGearTwoDeceleration.Get();

    SBoatGear& gearThree = m_sailSpeeds[2];
    gearThree.maxSpeed       = BoatConfig::cvBoatSailingGearThreeMaxSpeed.Get();
    gearThree.acceleration   = BoatConfig::cvBoatSailingGearThreeAcceleration.Get();
    gearThree.deceleration   = BoatConfig::cvBoatSailingGearThreeDeceleration.Get();

    SBoatGear& gearReverse = m_sailSpeeds[3];
    gearReverse.maxSpeed     = BoatConfig::cvBoatSailingGearReverseMaxSpeed.Get();
    gearReverse.acceleration = BoatConfig::cvBoatSailingGearReverseAcceleration.Get();
    gearReverse.deceleration = BoatConfig::cvBoatSailingGearReverseDeceleration.Get();

    // Change mass position from debug pages
    const Matrix wtl = wrapper->GetPose().Inverted();    
    Vector localMassPosition = wtl.TransformPoint( wrapper->GetCenterOfMassPosition() );
    if( fabsf( localMassPosition.Y - BoatConfig::cvBoatMassCenter.Get() ) > 0.001f )
    {
        localMassPosition.Y = BoatConfig::cvBoatMassCenter.Get();
        wrapper->SetMassPosition( localMassPosition );
    }

    // Change mass from debug pages
    if( wrapper->GetMass() != BoatConfig::cvBoatMass.Get() )
        wrapper->SetMass( BoatConfig::cvBoatMass.Get() );

    // Limit rotation axes
    if( m_buoyancySystem != nullptr )
    {
#ifdef USE_PHYSX
        const Float tiltYlimitRad = DEG2RAD( BoatConfig::cvBoatTiltYLimit.Get() );
        physx::PxJointLimitCone coneLimit = m_buoyancySystem->m_sixDofJoint->getSwingLimit();
        if( coneLimit.yAngle != tiltYlimitRad )
            m_buoyancySystem->m_sixDofJoint->setSwingLimit( physx::PxJointLimitCone( tiltYlimitRad, tiltYlimitRad ) );

        const Float tilXlimitRad = DEG2RAD( BoatConfig::cvBoatTiltXLimit.Get() );
        physx::PxJointAngularLimitPair pair = m_buoyancySystem->m_sixDofJoint->getTwistLimit();
        if( pair.lower != -tilXlimitRad )
            m_buoyancySystem->m_sixDofJoint->setTwistLimit( physx::PxJointAngularLimitPair( -tilXlimitRad, tilXlimitRad ) );
#endif
	}

    if( m_hedgehogSystem != nullptr )
    {
        m_hedgehogSystem->OnDebugPageClosed();
    }
}

void CBoatComponent::OnDriverMount()
{
	m_driverDismounted = false;
}

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME(OnDrowningDismount);

void CBoatComponent::SetStateDrowning()
{
    if( !m_subSystemsInitialized )
    {
        return;
    }

    // Turn off water shadder bending
    CWorld* world = GetWorld();
    if( world == nullptr )
    {
        ASSERT( world );
        return;
    }

    // Turn off destruction systems collision callback
    CBoatDestructionComponent* destruction = GetEntity()->FindComponent<CBoatDestructionComponent>();
    if( destruction == nullptr )
    {
        ASSERT( destruction );
        return;
    }
    destruction->DisableCollisionCallback();

    // Turn off water bending shadder
	if( GetEntity() )
	{
		CDynamicCollisionCollector* collector = world->GetDynamicCollisionsCollector();
		if( collector )
		{
			for ( ComponentIterator< CDynamicColliderComponent > it( GetEntity() ); it; ++it )
			{
				CDynamicColliderComponent* colider = ( *it );
				if( colider )
				{
					// force removal now
					collector->Remove( colider, true );
				}    
			}
		}
	}

    // Disable all collisions with boat body
    if( m_hBoatBodyRigidMesh.Get() != nullptr && m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper() != nullptr )
    {
        m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper()->SetFlag( PRBW_CollisionDisabled, true );
    }

    CallEvent( CNAME(OnDrowningDismount) );

	CPeristentEntity* entity = Cast< CPeristentEntity > ( GetEntity() );
	if ( entity )
	{
		entity->ForgetTheState();
	}
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcGetMaxSpeed( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    RETURN_FLOAT( GetGear(MAX_GEARS).maxSpeed );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcGetLinearVelocityXY( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    RETURN_FLOAT( GetLinearVelocityXY() );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcGetBoatBodyMass( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    Float retMass = 0.0f;

    if( m_hBoatBodyRigidMesh.Get() != nullptr )
    {
        CPhysicsWrapperInterface* wrapper = m_hBoatBodyRigidMesh->GetPhysicsRigidBodyWrapper();
        if( wrapper != nullptr)
        {
            retMass = wrapper->GetMass();
        }
    }

    RETURN_FLOAT( retMass );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcStopAndDismountBoat(  CScriptStackFrame& stack, void* result  )
{
    FINISH_PARAMETERS;

    m_isDismountRequested = true;
	m_driverDismounted = true;

	CallEvent( CNAME( OnBoatDismountRequest ) );

    if( m_subSystemsInitialized )
    {
        RED_ASSERT( m_buoyancySystem, TXT( "Should never be null is sub systems are initialized." ) );

        // If it is already drowning dismount it
        if( m_buoyancySystem->IsDrowning() )
        {
            CallEvent( CNAME( OnTriggerBoatDismountAnim ) );
        }

        m_buoyancySystem->SetLocalTiltAngle( Vector2( 0.0f, 0.0f ) );

        const Float deceleration = GetGear( MAX_GEARS ).deceleration;
        m_inputSystem->SetSpeedDamper( 0, deceleration );
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcUseOutOfFrustumTeleportation( CScriptStackFrame& stack, void* result  )
{
    GET_PARAMETER( Bool, enable, false );

    m_pathfindingSystem->m_useOutOfFrustumTeleport = enable;

    FINISH_PARAMETERS;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcTriggerDrowning( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Vector, hit, Vector::ZEROS );
    FINISH_PARAMETERS;

    // Trigger buoyancy drowning procedure
    m_buoyancySystem->TriggerDrowning( hit );

    // Turn off physics after some time passes
    m_drowningShutdownTimeout = BoatConfig::cvBoatDrowningSystemsShutdownTime.Get();

    SetStateDrowning();
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcIsDrowning( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    if( !m_subSystemsInitialized )
        return;

    RED_ASSERT( m_buoyancySystem );
    RETURN_BOOL( m_buoyancySystem->IsDrowning() );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcTriggerCutsceneStart( CScriptStackFrame& stack, void* result  )
{
    FINISH_PARAMETERS;

    m_isInCutsceneMode = true;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcTriggerCutsceneEnd( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    m_isInCutsceneMode = false;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::PathFindingMoveToLocation( const Vector& destinationGlobal, const Vector& lookAtNormal )
{
    if( !m_subSystemsInitialized )
        return;

    Int32 targetGear = m_inputSystem->m_currentGear;
    m_pathfindingSystem->PathFindingMoveToLocation( destinationGlobal, lookAtNormal, targetGear );

    m_inputSystem->SteerSetGear( targetGear );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::PathFindingFollowCurve( SMultiCurve* path )
{
    if( !m_subSystemsInitialized )
        return;

    Int32 targetGear = m_inputSystem->m_currentGear;
    m_pathfindingSystem->PathFindingFollowCurve( path, targetGear );

    m_inputSystem->SteerSetGear( targetGear );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::PathFindingStop()
{
    if( !m_subSystemsInitialized )
        return;

    m_pathfindingSystem->PathFindingStop();
    m_inputSystem->SteerSetGear( 0 );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::OnReachedEndOfPath()
{
    CActor *const sailor = m_user.Get();
    if ( sailor )
    {
        sailor->SignalGameplayEvent( CNAME( BoatReachedEndOfPath ) );
    }
}

////////////////////////////////////////////////////////////////////////////

void CBoatComponent::UpdateLOD( ILODable::LOD newLOD, CLODableManager* manager )
{
    ASSERT( m_currentLOD != newLOD );
    m_currentLOD = newLOD;

    CWorld* world = GetWorld();
    if( world == nullptr )
    {
        return;
    }

    if( m_currentLOD == ILODable::LOD_0 )
    {
		world->GetTickManager()->AddToGroup( this, TICK_Main );
        world->GetTickManager()->AddToGroup( this, TICK_PrePhysics );
    }
    else
    {
		world->GetTickManager()->RemoveFromGroup( this, TICK_Main );
        world->GetTickManager()->RemoveFromGroup( this, TICK_PrePhysics );
    }
}

ILODable::LOD CBoatComponent::ComputeLOD( CLODableManager* manager ) const
{
    if ( !GetEntity()->IsGameplayLODable() )
    {
        return ILODable::LOD_0;
    }

    if( m_isSpawnedFromQuestBlock )
    {
        // if this is a quest boat, don't use LODing on it
        return ILODable::LOD_0;
    }

    const Float distSqr = manager->GetPosition().DistanceSquaredTo2D( GetEntity()->GetWorldPosition() );
    if ( distSqr < manager->GetBudgetableDistanceSqr() )
    {
        return ILODable::LOD_0;
    }
    else if ( distSqr < manager->GetDisableDistanceSqr() )
    {
        return m_currentLOD;
    }

    return ILODable::LOD_1;
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcMountStarted( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    if( m_subSystemsInitialized && m_buoyancySystem != nullptr )
        m_buoyancySystem->LockXYMovement( false );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcDismountFinished( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    if( m_subSystemsInitialized && m_buoyancySystem != nullptr && !m_buoyancySystem->IsDrowning() )
    {
        m_buoyancySystem->LockXYMovement( true );
    }
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcGetCurrentGear( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    Int32 gear = 0;

    if( m_subSystemsInitialized && m_inputSystem != nullptr )
    {
        gear = m_inputSystem->SteerGetCurrentGear();
    }

    RETURN_INT( gear );
}

void CBoatComponent::funcGetCurrentSpeed( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;

    if ( m_inputSystem )
    {
        Vector speed = GetLocalToWorld().GetAxisY() * static_cast< Float >( Sgn( m_inputSystem->GetTargetSpeed() ) );
        RETURN_STRUCT( Vector, speed );
        return;
    }
    RETURN_STRUCT( Vector, Vector::ZEROS );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::GetTargetCameraSetup( SBoatCameraSetup& outTarget, Bool passenger )
{
    RED_ASSERT( m_inputSystem );
    RED_ASSERT( m_subSystemsInitialized );

	Int32 gear = m_inputSystem->SteerGetCurrentGear();

    if( passenger )
    {
		if( gear == -1 )
		{
			// reverse gear
			outTarget.m_fov         = BoatConfig::passengerBoatFovReverse.Get();
			outTarget.m_distance    = BoatConfig::passengerBoatDistanceReverse.Get();
			outTarget.m_pitch       = BoatConfig::passengerBoatPitchReverse.Get();
			outTarget.m_offsetUp    = BoatConfig::passengerBoatPivotOffsetUpReverse.Get();
			outTarget.m_sailOffset  = BoatConfig::passengerBoatCameraToSailOffsetReverse.Get();
			return;
		}

		if( gear == 1 )
		{
			// 1st gear
			outTarget.m_fov         = BoatConfig::passengerBoatFovGear1.Get();
			outTarget.m_distance    = BoatConfig::passengerBoatDistanceGear1.Get();
			outTarget.m_pitch       = BoatConfig::passengerBoatPitchGear1.Get();
			outTarget.m_offsetUp    = BoatConfig::passengerBoatPivotOffsetUpGear1.Get();
			outTarget.m_sailOffset  = BoatConfig::passengerBoatCameraToSailOffsetGear1.Get();
			return;
		}

		if( gear == 2 )
		{
			// 2nd gear
			outTarget.m_fov         = BoatConfig::passengerBoatFovGear2.Get();
			outTarget.m_distance    = BoatConfig::passengerBoatDistanceGear2.Get();
			outTarget.m_pitch       = BoatConfig::passengerBoatPitchGear2.Get();
			outTarget.m_offsetUp    = BoatConfig::passengerBoatPivotOffsetUpGear2.Get();
			outTarget.m_sailOffset  = BoatConfig::passengerBoatCameraToSailOffsetGear2.Get();
			return;
		}

		if( gear == 3 )
		{
			// 3rd gear
			outTarget.m_fov         = BoatConfig::passengerBoatFovGear3.Get();
			outTarget.m_distance    = BoatConfig::passengerBoatDistanceGear3.Get();
			outTarget.m_pitch       = BoatConfig::passengerBoatPitchGear3.Get();
			outTarget.m_offsetUp    = BoatConfig::passengerBoatPivotOffsetUpGear3.Get();
			outTarget.m_sailOffset  = BoatConfig::passengerBoatCameraToSailOffsetGear3.Get();
			return;
		}

		// standing
		outTarget.m_fov         = BoatConfig::passengerBoatFovStand.Get();
		outTarget.m_distance    = BoatConfig::passengerBoatDistanceStand.Get();
		outTarget.m_pitch       = BoatConfig::passengerBoatPitchStand.Get();
		outTarget.m_offsetUp    = 0.0f;
		outTarget.m_sailOffset  = BoatConfig::passengerBoatCameraToSailOffsetStand.Get();
    }
	else
	{
		if( gear == -1 )
		{
			// reverse gear
			outTarget.m_fov         = BoatConfig::driverBoatFovReverse.Get();
			outTarget.m_distance    = BoatConfig::driverBoatDistanceReverse.Get();
			outTarget.m_pitch       = BoatConfig::driverBoatPitchReverse.Get();
			outTarget.m_offsetUp    = BoatConfig::driverBoatPivotOffsetUpReverse.Get();
			outTarget.m_sailOffset  = BoatConfig::driverBoatCameraToSailOffsetReverse.Get();
			return;
		}

		if( gear == 1 )
		{
			// 1st gear
			outTarget.m_fov         = BoatConfig::driverBoatFovGear1.Get();
			outTarget.m_distance    = BoatConfig::driverBoatDistanceGear1.Get();
			outTarget.m_pitch       = BoatConfig::driverBoatPitchGear1.Get();
			outTarget.m_offsetUp    = BoatConfig::driverBoatPivotOffsetUpGear1.Get();
			outTarget.m_sailOffset  = BoatConfig::driverBoatCameraToSailOffsetGear1.Get();
			return;
		}

		if( gear == 2 )
		{
			// 2nd gear
			outTarget.m_fov         = BoatConfig::driverBoatFovGear2.Get();
			outTarget.m_distance    = BoatConfig::driverBoatDistanceGear2.Get();
			outTarget.m_pitch       = BoatConfig::driverBoatPitchGear2.Get();
			outTarget.m_offsetUp    = BoatConfig::driverBoatPivotOffsetUpGear2.Get();
			outTarget.m_sailOffset  = BoatConfig::driverBoatCameraToSailOffsetGear2.Get();
			return;
		}

		if( gear == 3 )
		{
			// 3rd gear
			outTarget.m_fov         = BoatConfig::driverBoatFovGear3.Get();
			outTarget.m_distance    = BoatConfig::driverBoatDistanceGear3.Get();
			outTarget.m_pitch       = BoatConfig::driverBoatPitchGear3.Get();
			outTarget.m_offsetUp    = BoatConfig::driverBoatPivotOffsetUpGear3.Get();
			outTarget.m_sailOffset  = BoatConfig::driverBoatCameraToSailOffsetGear3.Get();
			return;
		}

		// standing
		outTarget.m_fov         = BoatConfig::driverBoatFovStand.Get();
		outTarget.m_distance    = BoatConfig::driverBoatDistanceStand.Get();
		outTarget.m_pitch       = BoatConfig::driverBoatPitchStand.Get();
		outTarget.m_offsetUp    = 0.0f;
		outTarget.m_sailOffset  = BoatConfig::driverBoatCameraToSailOffsetStand.Get();
	}
}


void CBoatComponent::UpdateCurrentCameraSetup( const SBoatCameraSetup& target, Float dt, Bool passenger )
{
    if( m_currentCameraSetup.m_fov < 0.0f )
    {
        m_currentCameraSetup.m_fov = target.m_fov;
    }
    else
    {
        Float coef = passenger ? BoatConfig::passengerBoatFovAdjustCoef.Get() : BoatConfig::driverBoatFovAdjustCoef.Get();
        m_currentCameraSetup.m_fov = Lerp( dt * coef, m_currentCameraSetup.m_fov, target.m_fov );
    }

    if( m_currentCameraSetup.m_distance < 0.0f )
    {
        m_currentCameraSetup.m_distance = target.m_distance;
    }
    else
    {
		Float coef = passenger ? BoatConfig::passengerBoatDistanceAdjustCoef.Get() : BoatConfig::driverBoatDistanceAdjustCoef.Get();
        m_currentCameraSetup.m_distance = Lerp( dt * coef, m_currentCameraSetup.m_distance, target.m_distance );
    }

    {
		Float coef = passenger ? BoatConfig::passengerBoatPitchAdjustCoef.Get() : BoatConfig::driverBoatPitchAdjustCoef.Get();
        m_currentCameraSetup.m_pitch = Lerp( dt * coef, m_currentCameraSetup.m_pitch, target.m_pitch );
    }

    {
		Float coef = passenger ? BoatConfig::passengerBoatOffsetAdjustCoef.Get() : BoatConfig::driverBoatOffsetAdjustCoef.Get();
        m_currentCameraSetup.m_offsetUp = Lerp( dt * coef, m_currentCameraSetup.m_offsetUp, target.m_offsetUp );
    }

    {
		Float coef = passenger ? BoatConfig::passengerBoatCameraToSailOffsetAdjustCoef.Get() : BoatConfig::driverBoatCameraToSailOffsetAdjustCoef.Get();
        m_currentCameraSetup.m_sailOffset = Lerp( dt * coef, m_currentCameraSetup.m_sailOffset, target.m_sailOffset );
    }
}


void CBoatComponent::funcGameCameraTick( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER_REF( Vector, fovDistPitch, Vector::ZEROS );
    GET_PARAMETER_REF( Float, offsetZ, 0.0f );
    GET_PARAMETER_REF( Float, sailOffset, 0.0f );
    GET_PARAMETER( Float, dt, 0.0f );
    GET_PARAMETER( Bool, passenger, false );
    FINISH_PARAMETERS;

    if( dt <= 0.0f || !m_subSystemsInitialized || !m_inputSystem )
    {
        RETURN_BOOL( false );
        return;
    }

    SBoatCameraSetup targetCameraSetup;
    GetTargetCameraSetup( targetCameraSetup, passenger );
    UpdateCurrentCameraSetup( targetCameraSetup, dt, passenger );

    fovDistPitch.X = m_currentCameraSetup.m_fov;
    fovDistPitch.Y = m_currentCameraSetup.m_distance;
    fovDistPitch.Z = m_currentCameraSetup.m_pitch;
    offsetZ = m_currentCameraSetup.m_offsetUp;
    sailOffset = m_currentCameraSetup.m_sailOffset;

    RETURN_BOOL( true );
}

//////////////////////////////////////////////////////////////////////////

void CBoatComponent::funcSetInputValues( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER_REF( SInputAction, accelerate, SInputAction() );
    GET_PARAMETER_REF( SInputAction, decelerate, SInputAction() );
    GET_PARAMETER_REF( Vector, stickTilt, Vector::ZEROS );
    GET_PARAMETER( Float, localSpaceCameraTurnPercent, 0.0f );
    FINISH_PARAMETERS;

    if( m_inputSystem != nullptr )
    {
        m_inputSystem->GatherInputLocalSpaceCamera( accelerate, decelerate, stickTilt, localSpaceCameraTurnPercent );
    }
}

//////////////////////////////////////////////////////////////////////////

CBoatComponent::SBoatCameraSetup::SBoatCameraSetup()
    : m_fov( -1.0f )
    , m_distance( -1.0f )
    , m_pitch( 0.0f )
    , m_offsetUp(  0.0f )
{
}

//////////////////////////////////////////////////////////////////////////

Float CBoatComponent::GetBasicWaterLevel() const
{
    if( !m_subSystemsInitialized || m_hedgehogSystem == nullptr )
    {
        return 0.0f;
    }

    return m_hedgehogSystem->m_basicWaterLevel;
}

//////////////////////////////////////////////////////////////////////////
