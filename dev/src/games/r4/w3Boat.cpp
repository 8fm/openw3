#include "build.h"
#include "w3Boat.h"
#include "commonMapManager.h"
#include "..\..\common\game\boatBodyComponent.h"
#include "..\..\common\physics\physicsWrapper.h"
#include "..\..\common\engine\tickManager.h"
#include "..\..\common\game\boatComponent.h"
#include "..\..\common\game\gameWorld.h"
#include "..\..\common\engine\clipMap.h"
#include "..\..\common\engine\idTagManager.h"
#include "..\..\common\physics\physicsWorld.h"
#include "..\..\common\physics\physicsWorldUtils.h"
#include "..\..\common\engine\mesh.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( W3Boat );

//////////////////////////////////////////////////////////////////////////

THashMap<const CEntity*, W3Boat::STeleportLocation> W3Boat::s_teleportLocations;
TDynArray<W3Boat::SDBGTeleportDestination> W3Boat::s_dbgPlacements;

//////////////////////////////////////////////////////////////////////////

W3Boat::W3Boat()
    : m_numTeleportTicks( -1 )
    , m_teleportPose( Matrix::IDENTITY )
    , m_prevTransform( Matrix::IDENTITY )
    , m_firstUpdate( true )
	, m_teleportedFromOtherHUB( false )
	, m_hasDrowned( false )
{
}

//////////////////////////////////////////////////////////////////////////

W3Boat::~W3Boat()
{
    W3Boat::s_teleportLocations.Erase( this );
}

//////////////////////////////////////////////////////////////////////////

void W3Boat::OnTick( Float deltaTime )
{
	PC_SCOPE( W3Boat );

    TBaseClass::OnTick( deltaTime );

    if( m_numTeleportTicks < 0 )
    {
        return;
    }

    ++m_numTeleportTicks;

    // Get wrapper from CBoatBodyComponent
    CPhysicsWrapperInterface* wrapper = nullptr;
    CBoatBodyComponent* boatBody = nullptr;
    if( m_numTeleportTicks > 1 )
    {
        boatBody = FindComponent<CBoatBodyComponent>();
        if( boatBody == nullptr )
        {
            --m_numTeleportTicks;
            return;
        }

        wrapper = boatBody->GetPhysicsRigidBodyWrapper();
        if( wrapper == nullptr )
        {
            --m_numTeleportTicks;
            return;
        }
    }
    else
    {
        return;
    }


    if( m_numTeleportTicks == 2 )
    {
        // Velocities to zero
        wrapper->SetVelocityAngular( Vector::ZEROS );
        wrapper->SetVelocityLinear( Vector::ZEROS );

        // Set pose to wrapper
        wrapper->SetPose( m_teleportPose );

        // Reset boat component state so that input is reset
        CBoatComponent* boatComp = FindComponent<CBoatComponent>();
        if( boatComp != nullptr )
        {
            boatComp->ResetState();
        }
    }
    else if( m_numTeleportTicks == 3 )
    {
        // When teleporting far away from camera, physx removes wrapper from scene and entity pose does not get updated
        // Force entity pose manually
        TBaseClass::Teleport( m_teleportPose.GetTranslation(), m_teleportPose.ToEulerAngles() );

        // Reset ticks num, switch wrapper back to dynamic state
        m_teleportPose.SetIdentity();

        wrapper->SwitchToKinematic( false );
    }
    else if( m_numTeleportTicks == 4 )
    {
        // Notify boat component that teleport is finished
        CBoatComponent* boatComponent = FindComponent<CBoatComponent>();
        if( boatComponent != nullptr )
        {
            boatComponent->NotifyTeleported();            
        }
        
        //==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK
        // Boat's kinematic components' positions update is delayed 3 (or even more) ticks in comparison to boat body
        // In case when we teleport/spawn/restore save boat body might be moved far away from all other components and camera frustum.
        // Due to that those components are put to sleep by physx and are removed from scene tick by the engine
        // Code below forces wrapper to update their positions if they re to far away from their graphics representations
        for( ComponentIterator<CRigidMeshComponent> it(this); it; ++it )
        {
            CRigidMeshComponent* comp = *it;
            CPhysicsWrapperInterface* miscComponentWrapper = comp->GetPhysicsRigidBodyWrapper();

            // Omit main boat wrapper, it's already moved
            if( miscComponentWrapper == nullptr || miscComponentWrapper == wrapper )
            {
                continue;
            }

            const Matrix& cMat = comp->GetLocalToWorld();
            const Vector& wPos = miscComponentWrapper->GetPosition();

            if( cMat.GetTranslation().DistanceSquaredTo(wPos) > 1.0f )
            {
                miscComponentWrapper->SetPose( cMat );
            }        
        }
        //==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK==HACK
    }
    else if( m_numTeleportTicks >= 6 )
    {
        W3Boat::s_teleportLocations.Erase( this );

        // Reset and remove from tick
        m_numTeleportTicks = -1;
        if( GetLayer() != nullptr && GetLayer()->GetWorld() != nullptr )
        {
            GetLayer()->GetWorld()->GetTickManager()->RemoveEntity( this );
        }

        // Set position lock back
        CBoatComponent* boatComponent = FindComponent<CBoatComponent>();
        if( boatComponent != nullptr )
        {
            if( boatComponent->GetUser().Get() == nullptr )
            {
                boatComponent->LockXYMovement( true );
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////

Vector W3Boat::AlignPositionToWaterLevel( const Vector& posToFix )
{
    Vector newPosition = posToFix;

    CGameWorld* world = GCommonGame->GetActiveWorld();
    if( world == nullptr )
    {
        return newPosition;
    }

    CClipMap* clipMap = world->GetTerrain();
    if( clipMap == nullptr )
    {
        return newPosition;
    }
    
    //////////////////////////////////////////////////////////////////////////
    // Fix Z position
    const Float waterLevel = world->GetWaterLevel( posToFix, 0 );

    // Get terrain height
    Float terrainHeight = 0.0f;
    if( clipMap->GetControlmapValueForWorldPosition( posToFix ) > 0 )  // Check for holes in clip map (eg caves)
    {
        clipMap->GetHeightForWorldPosition( posToFix, terrainHeight );
    }

    // Fix position
    newPosition.Z = Max( posToFix.Z, waterLevel );
    newPosition.Z = Max( posToFix.Z, terrainHeight );
    newPosition.Z += BoatConfig::cvBoatFloatingHeight.Get();

    return newPosition;
}

//////////////////////////////////////////////////////////////////////////

Bool W3Boat::IsLocationOverlapingOnPhysx( const CEntity* callerPtr, const Vector& targetPosition, const Matrix& targetRotation, const Box& targetBBox, TDynArray<CEntity*>& outOverlapingEntities )
{
    CPhysicsWorld* physicsWorld	= nullptr;
    CWorld* world = GGame->GetActiveWorld();
    if( world != nullptr )
    {
        world->GetPhysicsWorld( physicsWorld );
    }
    else
    {
        return false;
    }

    if( physicsWorld == nullptr )
    {
        return false;
    }

    const Uint32 maxContacts = 10;
    Uint32 numContactsFound = 0;
    SPhysicsOverlapInfo infos[maxContacts];

    const CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionTypeBit( CName(TXT("Boat")) );

    Bool isOverlaping = false;
    if( TRV_Hit == physicsWorld->BoxOverlapWithMultipleResults( targetPosition, targetBBox.CalcExtents(), targetRotation, includeMask, 0, infos, numContactsFound, maxContacts ) )
    {
        for( Uint32 i=0; i<numContactsFound; ++i )
        {
            // Get wrapper
            CPhysicsWrapperInterface* collider = static_cast< CPhysicsWrapperInterface* >(infos[i].m_userData);
            if( collider == nullptr )
            {
                continue;
            }

            // Get rigid component that owns the wrapper
            CComponent* rigid = nullptr;
            collider->GetParent<CComponent>(rigid);

            if( rigid == nullptr )
            {
                continue;
            }

            // Check if we are not self colliding
            CEntity* boatEntity = rigid->GetEntity();
            /*CBoatComponent* boatComp = boatEntity->FindComponent<CBoatComponent>();*/
            
            if( boatEntity != nullptr && boatEntity != callerPtr/* && !boatComp->IsWaitingForTeleport() */)
            {
                isOverlaping = true;
                outOverlapingEntities.PushBack( boatEntity );
            }
        }
    }

    return isOverlaping;
}

//////////////////////////////////////////////////////////////////////////

Bool W3Boat::IsLocationOverlapingOnTeleportLocations( const CEntity* callerPtr, const Vector& targetPosition, const Matrix& targetRotation, const Box& targetBBox )
{
    if( W3Boat::s_teleportLocations.Empty() )
    {
        return false;
    }

    // Test if any of already set teleport locations do not overlap
    Matrix wtl = targetRotation;
    wtl.SetTranslation(targetPosition);
    wtl.Invert();

    for( auto it = W3Boat::s_teleportLocations.Begin(); it != W3Boat::s_teleportLocations.End(); ++it )
    {
        if( (*it).m_first == callerPtr )
        {
            continue;
        }

        const W3Boat::STeleportLocation& loc = (*it).m_second;
        const Matrix reference = loc.pose * wtl;
        const Box compareBox = W3Boat::ConvertToGlobal( loc.bbox, reference );

        if( W3Boat::IsOverlaping2D( targetBBox, compareBox ) )
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////

Bool W3Boat::IsOverlaping2D( const Box& a, const Box& b )
{
    if( a.Contains2D( b.CalcCenter() ) ) return true;

    Vector diff = a.CalcSize();

    if( a.Contains2D( b.Min ) ) return true;
    if( a.Contains2D( b.Max ) ) return true;

    Vector perp = Vector::Cross( Vector::EZ, Vector(diff.X, diff.Y, 0) );

    if( a.Contains2D( b.Min + (diff + perp) * 0.5f ) ) return true;
    if( a.Contains2D( b.Min + (diff - perp) * 0.5f ) ) return true;

    diff = b.CalcSize();

    if( b.Contains2D( a.Min ) ) return true;
    if( b.Contains2D( a.Max ) ) return true;

    perp = Vector::Cross( Vector::EZ, Vector(diff.X, diff.Y, 0) );

    if( b.Contains2D( a.Min + (diff + perp) * 0.5f ) ) return true;
    if( b.Contains2D( a.Min + (diff - perp) * 0.5f ) ) return true;

    return false;
}

//////////////////////////////////////////////////////////////////////////

Box W3Boat::ConvertToGlobal( const Box& localBBox, const Matrix& pose )
{
    Box ret = localBBox;

    ret.Min = pose.TransformPoint( ret.Min );
    ret.Max = pose.TransformPoint( ret.Max );

    return ret;
}

//////////////////////////////////////////////////////////////////////////

Bool W3Boat::Teleport( const Vector& position, const EulerAngles& rotation )
{
    CBoatBodyComponent* boatBody = FindComponent<CBoatBodyComponent>();
    if( boatBody == nullptr )
    {
        return false;
    }

    CPhysicsWrapperInterface* wrapper = boatBody->GetPhysicsRigidBodyWrapper();
    if( wrapper == nullptr )
    {
        return false;
    }
    
    CMesh* boatMesh =  boatBody->GetMeshNow();
    if(boatMesh == nullptr )
    {
        return false;
    }
    
    Vector fixedPosition = position;
    Matrix fixedRotation = rotation.ToMatrix();
    const Box boundingBox = boatMesh->GetBoundingBox();
    
    CBoatComponent* boatComp = FindComponent<CBoatComponent>();
    if( boatComp != nullptr && !boatComp->IsWaitingForTeleport() )
    {
        const Float bboxRadius = boundingBox.CalcSize().Mag2();

        // Hack, remove all boats spawned in the same place (very similar position and rotation)
		{
            TDynArray<CEntity*> collidingBoats;
        
            if( W3Boat::IsLocationOverlapingOnPhysx( this, fixedPosition, fixedRotation, boundingBox, collidingBoats ) )
            {
                const Float positionThreshold = 0.5f;
                const Float rotationThresholdDeg = 5.0f;
                const Uint32 amount = collidingBoats.Size();

                for( Uint32 i=0; i<amount; ++i )
                {
                    CEntity* ent = collidingBoats[i];
                    
                    if( ( ent == nullptr ) || ( ent == this ) )
                    {
                        continue;
                    }

					{ // hack to repair hack repairing hack
						// it is possible that the player is already attached to the boat pointed by ent
						auto childAttachments = ent->GetChildAttachments();
						Bool shouldContinue = false;
						for( auto it = childAttachments.Begin(); it != childAttachments.End(); ++it )
						{
							if( ( *it )->GetChild() )
							{ // somebody is mounted to this boat. Possibly Geralt ?
								shouldContinue = true;
								break;
							}
						}

						if( shouldContinue )
						{
							continue;
						}
					} // end of hack ( inner hack, outer is sitll in progress )

                    // Compare transform similarity
                    Float diff = fixedPosition.DistanceTo2D( ent->GetWorldPosition() );
                    if( diff < positionThreshold )
                    {
                        ent->Destroy();
                        continue;
                    }

                    diff = RAD2DEG( acosf( fixedRotation.GetAxisY().Dot2( ent->GetLocalToWorld().GetAxisY() ) ) );
                    if( diff < rotationThresholdDeg )
                    {
                        ent->Destroy();
                    }
                }

                collidingBoats.Clear();
            }
        }
        // Hack

        if( W3Boat::IsLocationOverlapingOnTeleportLocations( this, fixedPosition, fixedRotation, boundingBox ) )
        {
            Bool isOverlaping = false;

            for( Uint32 i=0; i<360; i+= 10 )
            {
                Vector newPosition( cosf(DEG2RAD((Float)i)) * bboxRadius, sinf(DEG2RAD((Float)i)) * bboxRadius, 0.0f );
                newPosition += fixedPosition;

                TDynArray<CEntity*> collidingBoats;

                isOverlaping =
                    W3Boat::IsLocationOverlapingOnTeleportLocations( this, newPosition, fixedRotation, boundingBox ) &&
                    W3Boat::IsLocationOverlapingOnPhysx( this, newPosition, fixedRotation, boundingBox, collidingBoats );
                
                if( !isOverlaping )
                {
                    fixedPosition = newPosition;
                    break;
                }
            }

            // If we still do not have proper location, terminate teleport routine
            if( isOverlaping )
            {
                return false;
            }
        }
    }

    // Fix position to match water/terrain level
    fixedPosition = AlignPositionToWaterLevel( fixedPosition );
    
    if( m_numTeleportTicks < 0 )
    {
        // Unlock xy movement
        if( boatComp != nullptr )
        {
            boatComp->LockXYMovement( false );
        }

        wrapper->SwitchToKinematic( true );
        m_numTeleportTicks = 0;
        
        if( GetLayer() != nullptr && GetLayer()->GetWorld() != nullptr )
        {
            GetLayer()->GetWorld()->GetTickManager()->AddEntity( this );
        }
    }

    m_teleportPose = fixedRotation;
    
    // Align to eZ
    m_teleportPose.V[0].Z = 0.0f;
    m_teleportPose.V[1].Z = 0.0f;
    m_teleportPose.V[2] = Vector::EZ;

    m_teleportPose.V[0] = m_teleportPose.V[0].Normalized2();
    m_teleportPose.V[1] = m_teleportPose.V[1].Normalized2();
    m_teleportPose.V[0].W = 0.0f;
    m_teleportPose.V[1].W = 0.0f;

    // Set fixed position
    m_teleportPose.SetTranslation( fixedPosition );

    // Save this location for simultaneous teleports
    STeleportLocation location;
    location.pose = m_teleportPose;
    location.bbox = boundingBox;
    W3Boat::s_teleportLocations.Insert( this, location );

    SDBGTeleportDestination place;
    place.bbox = boundingBox;
    place.pose = m_teleportPose;
    place.tickCreated = GEngine->GetRawEngineTime();
    W3Boat::s_dbgPlacements.PushBack(place);

    return true;
}

//////////////////////////////////////////////////////////////////////////

Bool W3Boat::Teleport( CNode* node, Bool applyRotation )
{
    ASSERT( node );
    if( node == nullptr )
    {
        return false;
    }

    const Vector& position = node->GetPosition();
    const EulerAngles& rotation = applyRotation ? node->GetRotation() : GetRotation();

    return Teleport( position, rotation );
}

//////////////////////////////////////////////////////////////////////////

void W3Boat::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BoatSailing );

	if ( GGame->IsActive() )
	{
		CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
		if ( commonMapManager )
		{
			commonMapManager->OnAttachedBoat( this );
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void W3Boat::OnDetached( CWorld* world )
{
	if ( GGame->IsActive() )
	{
		CCommonMapManager* commonMapManager = GCommonGame->GetSystem< CCommonMapManager >();
		if ( commonMapManager )
		{
			commonMapManager->OnDetachedBoat( this );
		}
	}

    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BoatSailing );

	TBaseClass::OnDetached( world );
}

//////////////////////////////////////////////////////////////////////////

RED_DEFINE_STATIC_NAME( OnDismountImediete );

void W3Boat::OnDestroyed( CLayer* layer )
{
    TBaseClass::OnDestroyed( layer );

    // Make sure both driver and passenger are unmounted
    CBoatComponent* boatComponent = FindComponent<CBoatComponent>();
    if( boatComponent == nullptr )
        return;

    boatComponent->CallEvent( CNAME(OnDismountImediete) );
}

//////////////////////////////////////////////////////////////////////////

void W3Boat::OnStreamIn()
{
	TBaseClass::OnStreamIn();
	CallEvent( CNAME( OnStreamIn ) );

	if( m_teleportedFromOtherHUB && !( GetIdTag().IsValid() ) )
	{
		ConvertToManagedEntity( GGame->GetIdTagManager()->Allocate() );
		SetShouldSave( true );
	}
}

//////////////////////////////////////////////////////////////////////////

void W3Boat::OnStreamOut()
{
	TBaseClass::OnStreamOut();
	CallEvent( CNAME( OnStreamOut ) );

	if( m_teleportedFromOtherHUB )
	{
		ForgetTheState();
	}
}

//////////////////////////////////////////////////////////////////////////

void W3Boat::SetRawPlacementNoScale( const Matrix& newPlacement )
{
	// Ignore and reset damping if the boat is in the process of teleporting.
	if( m_numTeleportTicks >= 0 )
	{
		m_transform.Init( newPlacement );

		m_prevTransform = newPlacement;

		m_damperX.Setup( m_prevTransform.V[0], m_prevTransform.V[0], BoatConfig::cvPhysxThreadDamperTime.Get() );
		m_damperY.Setup( m_prevTransform.V[1], m_prevTransform.V[1], BoatConfig::cvPhysxThreadDamperTime.Get() );
		m_damperZ.Setup( m_prevTransform.V[2], m_prevTransform.V[2], BoatConfig::cvPhysxThreadDamperTime.Get() );
		m_damperT.Setup( m_prevTransform.V[3], m_prevTransform.V[3], BoatConfig::cvPhysxThreadDamperTime.Get() );

		return;
	}
	else
	{
    if( m_firstUpdate )
    {
        m_prevTransform = newPlacement;
        
        m_damperX.Setup( m_prevTransform.V[0], m_prevTransform.V[0], BoatConfig::cvPhysxThreadDamperTime.Get() );
        m_damperY.Setup( m_prevTransform.V[1], m_prevTransform.V[1], BoatConfig::cvPhysxThreadDamperTime.Get() );
        m_damperZ.Setup( m_prevTransform.V[2], m_prevTransform.V[2], BoatConfig::cvPhysxThreadDamperTime.Get() );
        m_damperT.Setup( m_prevTransform.V[3], m_prevTransform.V[3], BoatConfig::cvPhysxThreadDamperTime.Get() );

        m_firstUpdate = false;
    }

    if( m_damperX.GetDampingTime() != BoatConfig::cvPhysxThreadDamperTime.Get() )
    {
        m_damperX.SetDampingTime( BoatConfig::cvPhysxThreadDamperTime.Get() );
        m_damperY.SetDampingTime( BoatConfig::cvPhysxThreadDamperTime.Get() );
        m_damperZ.SetDampingTime( BoatConfig::cvPhysxThreadDamperTime.Get() );
        m_damperT.SetDampingTime( BoatConfig::cvPhysxThreadDamperTime.Get() );
    }

    const Float deltaTime = GEngine->GetLastTimeDelta();

    // X
    if( !Vector::Equal3( m_prevTransform.V[0], newPlacement.V[0] ) )
    {
        m_damperX.SetStartValue( m_damperX.GetInterpolatedValue() );
        m_damperX.SetDestValue( newPlacement.V[0] );
        m_damperX.ResetInterpolationTime();
    }
    //Y
    if( !Vector::Equal3( m_prevTransform.V[1], newPlacement.V[1] ) )
    {
        m_damperY.SetStartValue( m_damperY.GetInterpolatedValue() );
        m_damperY.SetDestValue( newPlacement.V[1] );
        m_damperY.ResetInterpolationTime();
    }
    //Z
    if( !Vector::Equal3( m_prevTransform.V[2], newPlacement.V[2] ) )
    {
        m_damperZ.SetStartValue( m_damperZ.GetInterpolatedValue() );
        m_damperZ.SetDestValue( newPlacement.V[2] );
        m_damperZ.ResetInterpolationTime();
    }
    //T
    if( !Vector::Equal3( m_prevTransform.V[3], newPlacement.V[3] ) )
    {
        m_damperT.SetStartValue( m_damperT.GetInterpolatedValue() );
        m_damperT.SetDestValue( newPlacement.V[3] );
        m_damperT.ResetInterpolationTime();
    }
    
    m_prevTransform = newPlacement;

    const Matrix damped( m_damperX.Update(deltaTime), m_damperY.Update(deltaTime), m_damperZ.Update(deltaTime), m_damperT.Update(deltaTime) );

		// Update pose with damped values
		m_transform.Init( damped );
	}
	m_transform.RemoveScale();

	// Schedule transform update
	ScheduleUpdateTransformNode();
}

void W3Boat::funcHasDrowned( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_BOOL( m_hasDrowned );
}

void W3Boat::funcSetHasDrowned( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, val, false );
	FINISH_PARAMETERS;

	m_hasDrowned = val;
}

void W3Boat::funcSetTeleportedFromOtherHUB( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, val, false );
	FINISH_PARAMETERS;
	m_teleportedFromOtherHUB = val;
}

////////////////////////////////////////////////////////////////////////

void W3Boat::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
    TBaseClass::OnGenerateEditorFragments( frame, flag );

    if( flag != SHOW_BoatSailing || W3Boat::s_teleportLocations.Empty())
    {
        return;
    }

    const Float currEngineTime = GEngine->GetRawEngineTime();

    for( auto it = W3Boat::s_dbgPlacements.Begin(); it != W3Boat::s_dbgPlacements.End(); ++it )
    {
        W3Boat::SDBGTeleportDestination& location = (*it);

        frame->AddDebugBox( location.bbox, location.pose, Color::RED, true, true );

        static Float timeout = 100.0f;

        if( currEngineTime - location.tickCreated > timeout )
        {
            W3Boat::s_dbgPlacements.Erase(it);
        }
        
    }
}

//////////////////////////////////////////////////////////////////////////

//void W3Boat::OnAttached( CWorld* world )
//{
//    TBaseClass::OnAttached( world );
//
//    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
//}
//
////////////////////////////////////////////////////////////////////////////
//
//void W3Boat::OnDetached( CWorld* world )
//{
//    TBaseClass::OnDetached( world );
//
//    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );
//}
//
////////////////////////////////////////////////////////////////////////////
