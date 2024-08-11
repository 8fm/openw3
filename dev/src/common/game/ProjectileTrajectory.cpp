/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "ProjectileTrajectory.h"
#include "ProjectileTargets.h"
#include "actorsManager.h"
#include "movingPhysicalAgentComponent.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "gameplayStorageAcceptors.h"
#include "../engine/physicsCharacterVirtualController.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsWrapper.h"
#include "../engine/gameTimeManager.h"
#include "../engine/tickManager.h"
#include "../engine/renderFrame.h"
#include "../engine/clipMap.h"

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CProjectileTrajectory );

// RED_DEFINE_NAME( ProjectileTrajectory );
// 
// RED_DEFINE_NAME( projectileCaster );
// RED_DEFINE_NAME( projectileProjectileName );
// RED_DEFINE_NAME( projectileStarted );
// RED_DEFINE_NAME( projectileProjectileTarget );
// RED_DEFINE_NAME( projectileProjectileVelocity );
// RED_DEFINE_NAME( projectileMaxRange );
// RED_DEFINE_NAME( projectileDistanceTraveled );
// RED_DEFINE_NAME( projectileRadius );
// RED_DEFINE_NAME( projectileCollisionGroups );
// RED_DEFINE_NAME( projectileStartPosition );
// RED_DEFINE_NAME( projectileCurveProgress );
// RED_DEFINE_NAME( projectileInitialTargetDistance );
// RED_DEFINE_NAME( projectileVelocityVec );
// RED_DEFINE_NAME( projectileSineAmplitude );
// RED_DEFINE_NAME( projectileBounceOfCount );
// RED_DEFINE_NAME( projectileBounceOfVelocityPreserve );
// RED_DEFINE_NAME( projectileCakeAngle );
// RED_DEFINE_NAME( projectileDeltaStep );
// RED_DEFINE_NAME( projectileLastStartStep );

RED_DEFINE_NAME( projectileTargetType ); 
RED_DEFINE_NAME( projectileAnimatedTimeMultiplier );

#ifndef NO_EDITOR
    const static Float DBG_TrajectoryFragmentsSave = 0.1f;
    const static Float SHOW_TIMEOUT = 1.0f;
#endif
const static Float GPRaycastProjectileRadius = 0.05f;

//////////////////////////////////////////////////////////////////////////

CProjectileTrajectory::CProjectileTrajectory()
    :   m_caster( NULL )
    ,   m_radius( 0.0f )
    ,   m_started( false )
    ,   m_projectileTarget( NULL )
    ,   m_projectileVelocity( 0.0f )
    ,   m_maxRange( 0.0f )
    ,   m_distanceTraveled( 0.0f )

    ,   m_curveProgress( 0.0f )
    ,   m_initialTargetDistance( 0.0f )
    ,   m_startPosition( Vector::ZEROS )
    ,   m_velocityVec( Vector::ZEROS )
    ,   m_sineAmplitude( 1.0f )

    ,   m_animatedOffset( Vector::ZEROS )
    ,   m_animatedTimeMultiplier( 1.0f )

    ,   m_bounceOfVelocityPreserve( 0.5f )

    ,   m_cakeAngle( 0.0f )
    ,   m_deltaStep( 0.0f )
    ,   m_lastStartStep( Vector::ZEROS )
    ,   m_prevTrajectoryPositon( Vector::ZEROS )
    ,   m_waterTestDelta( PROJECTILE_WATER_TEST_ACCURACY )
    ,   m_doWaterLevelTest( true )

    ,	m_overlapAccuracy( PROJECTILE_OVERLAP_ACCURACY )
    ,	m_waterTestAccuracy( PROJECTILE_WATER_TEST_ACCURACY )
    ,	m_waterMinTestHeight( PROJECTILE_WATER_MIN_TEST_HEIGHT )
#ifndef NO_EDITOR
    ,   m_dbgBoxHalfExtent( Vector::ZEROS )
    ,   m_dbgBoxPosition( Vector::ZEROS )
    //,   m_dbgWaterTestsNo( 0 )
    ,   m_dbgTimePass( 0.0f )
#endif
	, m_collisionGroups( GPhysicEngine ? GPhysicEngine->GetCollisionGroupMask( CNAME( Projectile ) ) : 0 )
{
	SetDynamicFlag( EDF_AlwaysTick );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::OnFinalize()
{
    if( m_projectileTarget != NULL )
    {
        delete m_projectileTarget;
    }

    TBaseClass::OnFinalize();
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::OnAttached( CWorld* world )
{
    TBaseClass::OnAttached( world );

    // Add entity to tick manager
    world->GetTickManager()->AddEntity( this );

    world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Projectile );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::OnDetached( CWorld* world )
{
    TBaseClass::OnDetached( world );

    world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Projectile );

    // Remove entity from tick manager
    world->GetTickManager()->RemoveEntity( this );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::OnPropertyExternalChanged( const CName& propertyName )
{
    if( propertyName == CNAME( projectileAnimatedTimeMultiplier ) && m_propertyAnimationSet != NULL )
    {
        const Float deltaTime = GGame->GetTimeManager()->GetLastTickTime();
        const Uint32 count = m_propertyAnimationSet->GetAnimationInstanceCount();
        for( Uint32 i = 0; i < count; ++i )
        {
			SPropertyAnimationInstance* animationInstance = m_propertyAnimationSet->GetAnimationInstanceByIndex( i );
			const Float timeAdvance = deltaTime / animationInstance->m_lengthScale;
            animationInstance->m_timer += timeAdvance * m_animatedTimeMultiplier - timeAdvance;
		}
    }
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::Initialize( CEntity* caster )
{
    m_caster = caster;
    CallEvent( CNAME( OnProjectileInit ) );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::OnTick( Float timeDelta )
{
	PC_SCOPE( Projectile_OnTick );

	TBaseClass::OnTick( timeDelta );

	if( m_sphereOverlapTestData.Size() )
	{
		for( auto iter = m_sphereOverlapTestData.Begin(); iter != m_sphereOverlapTestData.End(); ++iter )
		{
			const SSphereOverlapTestData& data = *iter;
			SphereOverlapTest( data.m_worldPosition, data.m_radius, data.m_collisionMask );
		}
		m_sphereOverlapTestData.ClearFast();
	}

    if( !m_started )
        return;

    // Scale time passing using animated property
    const Float deltaTime = timeDelta * m_animatedTimeMultiplier;

    //////////////////////////////////////////////////////////////////////////
    // Calculate new trajectory position
    Vector newTrajectoryPosition = CalculateNewPosition( deltaTime );

    // Calculate distance traveled over trajectory
    Vector dirVec = newTrajectoryPosition - m_prevTrajectoryPositon;
#ifndef NO_EDITOR
    m_dbgDirVec = dirVec;
#endif
    Float moveStep = dirVec.Normalize3();
    m_distanceTraveled += moveStep;


    m_prevTrajectoryPositon = newTrajectoryPosition;

    // Calculate animated offset in projectile's local space using trajectory's matrix
    Matrix tmat;
    tmat.BuildFromDirectionVector( dirVec );
    newTrajectoryPosition += tmat.TransformPoint( m_animatedOffset );

    // Get offset dir and movement step
    dirVec = newTrajectoryPosition - GetWorldPosition();
    moveStep = dirVec.Normalize3();

    //////////////////////////////////////////////////////////////////////////
    // Test water level + optimization

	Bool waterHit = false;
	Vector waterHitPos = Vector::ZEROS;

    if( m_doWaterLevelTest )
    {
        if( m_waterMinTestHeight > newTrajectoryPosition.Z )    // Star testing below certain height
        {
            m_waterTestDelta += moveStep;
            if( m_waterTestDelta > m_waterTestAccuracy )        // Test only after projectile travels more than m_waterTestAccuracy
            {
                m_waterTestDelta = 0.0f;
                const Float waterLevel = GGame->GetActiveWorld()->GetWaterLevel( Vector( newTrajectoryPosition.X, newTrajectoryPosition.Y, FLT_MIN ), 1 );

#ifndef NO_EDITOR
                //++m_dbgWaterTestsNo;
#endif
				// Send event every time we're under water
                if( waterLevel >= newTrajectoryPosition.Z )
				{
					waterHit = true;
					waterHitPos = newTrajectoryPosition;
				}
				// We re not under water, optimize by decreasing minimal test height
                else
				{
					// Take average value of new projectile pos.Z and found waterLevel
                    m_waterMinTestHeight = ( newTrajectoryPosition.Z + waterLevel ) * 0.5f;
				}
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
    // Check collisions for animated projectile

	Bool hit = false;

    if( m_radius == 0.0f )
    {
        hit = CheckForCollisionsRaycast( newTrajectoryPosition, dirVec, timeDelta, waterHit ) || hit;
    }
    else    // Optimize amount of steps
    {
        m_deltaStep += moveStep;

        // Divide into steps
        if( m_deltaStep >= PROJECTILE_OVERLAP_ACCURACY )
        {
            moveStep = m_deltaStep;
            m_deltaStep = 0.0f;
            hit = CheckForCollisionsOverlap( newTrajectoryPosition, dirVec, moveStep, waterHit ) || hit;
        }
    }

	// if we hit the water, but didn't report it to the scripts inside CheckForCollisionsXXX, we do it now
	if ( waterHit && !hit )
	{
		TDynArray< CName > hitCollisionGroups( 1 );
		hitCollisionGroups[ 0 ] = CNAME( Water );
		CallEvent( CNAME( OnProjectileCollision ), waterHitPos, Vector::EZ, THandle< CComponent >(), hitCollisionGroups, 0, 0 );
	}

	if ( ! m_started )
	{
		// we were stopped during "OnProjectileCollision" handling
		return;
	}

	// Send range reached event only once
    if( m_maxRange > 0 && m_distanceTraveled != m_maxRange && m_distanceTraveled > m_maxRange )
    {
        m_distanceTraveled = m_maxRange;
        CallEvent( CNAME( OnRangeReached ) );
    }

#ifndef NO_EDITOR
    m_dbgTimePass += deltaTime;
    if( m_dbgTimePass >= DBG_TrajectoryFragmentsSave )
    {
        m_dbgTimePass = 0.0f;
        m_dbgTrajcetory.PushBack( newTrajectoryPosition );
    }
#endif

    // Get transformation matrix from direction vector
    tmat.BuildFromDirectionVector( dirVec );

	// Advance movement if not attached - because when attached, transform is relative and should be kept like this
	Teleport( newTrajectoryPosition, tmat.ToEulerAngles() );
}

//////////////////////////////////////////////////////////////////////////
//
// get actor on ray
static Int32 GamePlayRayCastOnActors( const Vector& from, const Vector& to, CActor** outBuffer )
{
	CActorsManager* actorsManager = GCommonGame->GetActorsManager();
	return actorsManager ? actorsManager->CollectActorsAtLine( from, to, GPRaycastProjectileRadius, outBuffer, PROJECTILE_MAX_AREA_TEST_RESULTS ) : 0;	
}
//////////////////////////////////////////////////////////////////////////
// actors overlap test

namespace
{
	template < typename CylinderAcceptor >
	struct CylinderAcceptorWrapper : Red::System::NonCopyable
	{
		enum { SORT_OUTPUT = false };

		CylinderAcceptorWrapper( CActor** outputArray, Int32 maxElems, CylinderAcceptor& cylinderAcceptor )
			: m_outputArray( outputArray )
			, m_maxElems( maxElems )
			, m_nextElem( 0 )
			, m_cylinderAcceptor( cylinderAcceptor )
			{}

		Bool operator()( const CActorsManagerMemberData& member )
		{
			if ( m_nextElem >= m_maxElems )
			{
				return false;
			}

			CActor* actor = member.Get();
			if ( actor != nullptr )
			{
				Float radius = 0.5f;
				Float height = 2.0f;
				if ( CMovingAgentComponent* mac = actor->GetMovingAgentComponent() )
				{
					radius = mac->GetRadius();
					if ( CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( mac ) )
					{
						height = mpac->GetHeight();
					}
				}

				AACylinder cylinder( actor->GetWorldPositionRef(), radius, height );
				if ( m_cylinderAcceptor( cylinder ) )
				{
					m_outputArray[ m_nextElem++ ] = actor;
					return ( m_nextElem < m_maxElems );
				}
			}

			return true;
		}

		CActor**			m_outputArray;
		Int32				m_nextElem;
		Int32				m_maxElems;
		CylinderAcceptor&	m_cylinderAcceptor;		
	};
}

template < typename CylinderAcceptor >
static Int32 GamePlayOverlapTestOnActors( const Vector& position, Float radius, CActor** outBuffer, CylinderAcceptor acceptor )
{
	CActorsManager* actorsManager = GCommonGame->GetActorsManager();
	if ( actorsManager != nullptr )
	{
		CylinderAcceptorWrapper< CylinderAcceptor > functor( outBuffer, PROJECTILE_MAX_AREA_TEST_RESULTS, acceptor );
		actorsManager->CollectActorsOverlapTest( position, radius, functor );
		return functor.m_nextElem;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
//
// line projectile

Bool CProjectileTrajectory::CheckForCollisionsRaycast( const Vector& endPoint, const Vector& dirVec, Float timeDelta, Bool wasWaterHit /* = false */ )
{
	PC_SCOPE( Projectile_CheckForCollisionsRaycast );

	static const CPhysicsEngine::CollisionMask charMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) );

	Bool wasHit = false;

	// physical raycast for all collision groups which are not 'Character'
	if ( m_collisionGroups & ( ~charMask ) )
	{
		// disable character checking - will be tested below
		CPhysicsEngine::CollisionMask physicsCollisionMask = m_collisionGroups & ( ~charMask );
		// Due to inaccuracies, short back to back raycasts could fail to register hits near the endPoint.
		// We add an epsilon to the endPoint in order to avoid this phenomenon.
		Vector epsilon = dirVec * timeDelta * m_projectileVelocity * 0.5f;
		wasHit = CheckForCollisionsRaycast_PhysX( m_lastStartStep, endPoint + epsilon, physicsCollisionMask );
	}

	// gameplay characters raycast
	if ( !wasHit && ( m_collisionGroups & charMask ) )
	{
		wasHit = CheckForCollisionsRaycast_Gameplay( m_lastStartStep, endPoint );
	}

	m_lastStartStep = endPoint;

	return wasHit;
}

Bool CProjectileTrajectory::CheckForCollisionsRaycast_PhysX( const Vector& startPoint, const Vector& endPoint, CPhysicsEngine::CollisionMask collisionMask, Bool wasWaterHit /* = false */ )
{
	SPhysicsContactInfo infos[ PROJECTILE_MAX_AREA_TEST_RESULTS ];
	Uint32 hits = 0;
	Bool wasHit = false;
    ETraceReturnValue retVal = TRV_NoHit;

	{
		PC_SCOPE( Projectile_PhysX_Raycast );

		CPhysicsWorld* phyWorld = nullptr;
		GetLayer()->GetWorld()->GetPhysicsWorld( phyWorld );
		if ( phyWorld != nullptr )
		{
			retVal = phyWorld->RayCastWithMultipleResults( startPoint, endPoint, collisionMask, 0, infos, hits, PROJECTILE_MAX_AREA_TEST_RESULTS );
		}
	}

	if ( hits > 0 )
	{
		PC_SCOPE( Projectile_PhysX_Raycast_Handle );
        
		const Vector dirVec = ( endPoint - startPoint ).Normalized3();

		Float smallestHitDist = 1.0f;
		for ( Uint32 i = 0; i < hits; ++i )
		{
			if( infos[i].m_userDataA == nullptr )
			{
				continue;
			}

			Bool callEvents = false;
			wasHit = true;

			CComponent* actorComponent = nullptr;
            infos[i].m_userDataA->GetParent( actorComponent, infos[i].m_rigidBodyIndexA.m_actorIndex );
       
			if ( actorComponent == nullptr )
			{
				// If we hit terrain infos[i].m_userDataA is TerrainWrapper but no CComponent is return upon calling GetParent(index)
				callEvents = true;
			}
			else
			{
				CEntity* componentEntity = actorComponent->GetEntity();
			    TDynArray<CComponent*>::iterator foundIt = m_uniqueCollisionComponents.Find( actorComponent );
				if( componentEntity != nullptr && componentEntity != m_caster.Get() && foundIt == m_uniqueCollisionComponents.End() )
				{
					// Collision with other objects
					// Test for unique actors... (SIC!!)
					// Physics returns all shapes that raycast hits
					// Since we want only actors affected by raycast and actor can have multiple shapes...
					// Filter actors we already have hit
					m_uniqueCollisionComponents.Insert( actorComponent );
					callEvents = true;
				}
			}

			if ( callEvents )
			{                
				// Get smallest hit distance and move projectile to that position
				// In scripts projectile is usually stopped on impact, we want entity to stop directly at collision position
				if( smallestHitDist > infos[i].m_distance )
				{
					smallestHitDist = infos[i].m_distance;

					// If any hit was detected, move projectile to detected hit position
					// In scripts projectile movement is usually disabled after hit is detected and position where projectile stops is not the same as hit position (for eg arrows)
					Matrix transM;
					transM.BuildFromDirectionVector( dirVec );

					Teleport( infos[i].m_position, transM.ToEulerAngles() );
				}

                // Get collision groups
				CPhysicsEngine::CollisionMask mask;
                if ( infos[i].m_rigidBodyIndexA.m_actorIndex != 0 || infos[i].m_rigidBodyIndexA.m_shapeIndex != 0 )
				{
					mask = infos[i].m_userDataA->GetCollisionTypesBits( infos[i].m_rigidBodyIndexA.m_actorIndex, infos[i].m_rigidBodyIndexA.m_shapeIndex );
				}
				else
				{
					mask = infos[i].m_userDataA->GetCollisionTypesBits();
				}

                // Translate bits to CNames
				TDynArray< CName > hitCollisionsGroups;
				GPhysicEngine->GetGroupsNamesByCollisionMask( mask, hitCollisionsGroups );

				if ( wasWaterHit )
				{
					hitCollisionsGroups.PushBack( CNAME( Water ) );
				}

				{
					PC_SCOPE( Projectile_PhysX_Raycast_OnProjectileCollision );
					THandle< CComponent > componentHandle( actorComponent );
					CallEvent( CNAME( OnProjectileCollision ), infos[i].m_position, infos[i].m_normal, componentHandle, hitCollisionsGroups, (Int32)infos[i].m_rigidBodyIndexA.m_actorIndex, (Int32)infos[i].m_rigidBodyIndexA.m_shapeIndex );
				}

#ifndef NO_EDITOR
				if ( !hitCollisionsGroups.Exist( CNAME( Terrain ) ) )
				{
					m_dbgGlobalHitPos.PushBack( infos[i].m_position );
				}
#endif
			}
		}   // For all hits
	}

	return wasHit;
}

Bool CProjectileTrajectory::CheckForCollisionsRaycast_Gameplay( const Vector& startPoint, const Vector& endPoint )
{
	CActor* foundActorsAround[ PROJECTILE_MAX_AREA_TEST_RESULTS ];
	Uint32 hits = 0;
	Int32 resultsCount = 0;
	Bool wasHit = false;

	{
		PC_SCOPE( Projectile_GP_Raycast );

		// get actors on ray
		resultsCount = GamePlayRayCastOnActors( startPoint, endPoint, foundActorsAround );
	}

	{
		PC_SCOPE( Projectile_GP_Raycast_Handle );
		const Vector dirVec = ( endPoint - startPoint ).Normalized3();

		for ( Int32 i = 0; i < resultsCount; ++i )
		{
			CActor* actor = foundActorsAround[ i ];

            // If pointer invalid or we have self collision
			if ( !actor || actor == m_caster.Get() )
				continue;

			CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
			if ( mpac && mpac->GetPhysicalCharacter() )
			{
				// init
				AACylinder cylinder;
				hits = 0;
				Vector hitPos( FLT_MAX, FLT_MAX, FLT_MAX );

#ifdef USE_PHYSX
				// check its virtual controllers
				if ( mpac->GetPhysicalCharacter()->GetCharacterController() )
				{
					typedef TDynArray< CVirtualCharacterController > VCCArray;
					const VCCArray& virtualControllers = mpac->GetPhysicalCharacter()->GetCharacterController()->GetVirtualControllers();
					VCCArray::const_iterator itEnd = virtualControllers.End();
					for ( VCCArray::const_iterator it = virtualControllers.Begin(); it != itEnd; ++it )
					{
						typedef TDynArray< CVirtualCharacterController > VCCArray;
#ifdef USE_PHYSX
						const VCCArray& virtualControllers = mpac->GetPhysicalCharacter()->GetCharacterController()->GetVirtualControllers();
						VCCArray::const_iterator itEnd = virtualControllers.End();
						for ( VCCArray::const_iterator it = virtualControllers.Begin(); it != itEnd; ++it )
						{
							cylinder.m_positionAndRadius = it->GetGlobalPosition();
							cylinder.m_positionAndRadius.W = it->GetCurrentRadius();
							cylinder.m_height = it->GetCurrentHeight();

							Vector locaHitPos;
							if ( cylinder.IntersectRay( m_lastStartStep, endPoint-m_lastStartStep, locaHitPos ) )
							{
								if ( m_lastStartStep.DistanceSquaredTo( locaHitPos ) < m_lastStartStep.DistanceSquaredTo( hitPos ) )
								{
									hitPos = locaHitPos;
								}

#ifndef NO_EDITOR
								m_dbgGlobalHitPos.PushBack( locaHitPos );
#endif
								wasHit = true;
								++hits;

								// next v-controller
								continue;
							}
						}
#endif
					}
				}
#endif // USE_PHYSX

				// than check physical controller
				const Vector& worldPosition = actor->GetWorldPositionRef();
				if ( mpac->GetPhysicalCharacter()->GetCollisionControllerExtents( cylinder, worldPosition ) )
				{
					Vector localHitPos;
					if ( cylinder.IntersectRay( m_lastStartStep, endPoint-m_lastStartStep, localHitPos ) )
					{
						if ( m_lastStartStep.DistanceSquaredTo( localHitPos ) < m_lastStartStep.DistanceSquaredTo( hitPos ) )
						{
							hitPos = localHitPos;
						}

#ifndef NO_EDITOR
						m_dbgGlobalHitPos.PushBack( hitPos );
#endif
						wasHit = true;
						++hits;
					}
				}

				// if sth was hit
				if ( hits )
				{
					PC_SCOPE( Projectile_GP_Raycast_OnProjectileCollision );

					// teleport projectile to collision pos
					Matrix transM;
					transM.BuildFromDirectionVector( dirVec );
					Teleport( hitPos, transM.ToEulerAngles() );

					// call scripts handler
					THandle< CComponent > componentHandle( mpac );
					TDynArray< CName > hitCollisionsGroups;
					hitCollisionsGroups.PushBack( CNAME( Character ) );
					CallEvent( CNAME( OnProjectileCollision ), hitPos, -dirVec, componentHandle, hitCollisionsGroups, 0, 0 );
				}	
			}
		}
	}

	return wasHit;
}

//////////////////////////////////////////////////////////////////////////

Bool CProjectileTrajectory::CheckForCollisionsOverlap( const Vector& endPoint, const Vector& dirVec, Float moveStep, Bool wasWaterHit /* = false */ )
{
	PC_SCOPE( Projectile_CheckForCollisionsOverlap );

	Bool wasHit = false;
    CPhysicsWorld* phyWorld = nullptr;
	GetLayer()->GetWorld()->GetPhysicsWorld( phyWorld );

    SPhysicsOverlapInfo infos[ PROJECTILE_MAX_AREA_TEST_RESULTS ];
    Uint32 hits = 0;
    const Vector boxPos = ( m_lastStartStep + endPoint ) * 0.5f;  // Position in middle between start and end points
#ifndef NO_EDITOR
    m_dbgBoxPosition = boxPos;
#endif

    Matrix orient;
    orient.BuildFromDirectionVector( dirVec );

    // Do cake overlap
    // The following formula approximates cake slice with "pyramid of blocks"
    // Overlapping boxes of growing volume
    if( m_cakeAngle > 0.0f)
    {
        // Compute volume of overlap area in this step
        const Float firstEdgeTravel = m_radius / ( tanf( DEG2RAD( m_cakeAngle * 0.5f ) ) * 4.0f );
        const Float travelSoFar = (m_startPosition - m_lastStartStep).Mag3() + firstEdgeTravel;
        const Float edge = 2.0f * travelSoFar * tanf( DEG2RAD( m_cakeAngle * 0.5f ) );
        const Vector halfExtent = Vector( edge, moveStep, m_radius ) * 0.5f;

#ifndef NO_EDITOR
        m_dbgBoxHalfExtent = halfExtent;
#endif

        const ETraceReturnValue retVal = phyWorld->BoxOverlapWithMultipleResults( boxPos, halfExtent, orient, m_collisionGroups, 0, infos, hits, PROJECTILE_MAX_AREA_TEST_RESULTS );
    }
    // Do box overlap through
    else
    {
        Float hExtent = 1.25f * m_radius;                  // compare areas a^2 = sqrt( 2*PI*radius^2 ) ==> a~= 2.5*radius
        const Vector halfExtent = Vector( hExtent, moveStep * 0.5f, hExtent );
#ifndef NO_EDITOR
        m_dbgBoxHalfExtent = halfExtent;
#endif
         const ETraceReturnValue retVal = phyWorld->BoxOverlapWithMultipleResults( boxPos, halfExtent, orient, m_collisionGroups, 0, infos, hits, PROJECTILE_MAX_AREA_TEST_RESULTS );
    }

    m_lastStartStep = endPoint;

    // Test for unique actors
    if( hits > 0 )
    {
// #ifndef NO_EDITOR
//         m_dbgWallTest.Clear();
// #endif

        Float smallestHitDist = 1.0f;

        for( Uint32 i=0; i<hits; ++i )
        {
            if( infos[i].m_userData == nullptr )
            {
                continue;
            }

			Bool wasHit = true;
            CComponent* actorComponent = nullptr;
			infos[i].m_userData->GetParent( actorComponent, infos[i].m_actorNshapeIndex.m_actorIndex );
            TDynArray<CComponent*>::iterator foundIt = m_uniqueCollisionComponents.Find( actorComponent );
            Bool callEvents = false;

            if( actorComponent == NULL )
            {
                // If we hit terrain infos[i].m_userDataA is TerrainWrapper but no CComponent is return upon calling GetParent(index)
                callEvents = true;
            }
            else
            {
                CEntity* componentEntity = actorComponent->GetEntity();
                if( componentEntity != NULL && componentEntity != m_caster.Get() && foundIt == m_uniqueCollisionComponents.End() )
                {
                    // Collision with other objects
                    // Test for unique actors... (SIC!!)
                    // Physics returns all shapes that raycast hits
                    // Since we want only actors affected by raycast and actor can have multiple shapes...
                    // Filter actors we already have hit
                    m_uniqueCollisionComponents.Insert( actorComponent );
                    callEvents = true;
                }
            }

            if( callEvents )
            {
                // Get smallest hit distance and move projectile to that position
                // In scripts projectile is usually stopped on impact, we want entity to stop directly at collision position
                if( smallestHitDist > infos[i].m_penetration )
                {
                    smallestHitDist = infos[i].m_penetration;

                    // If any hit was detected, move projectile to detected hit position
                    // In scripts projectile movement is usually disabled after hit is detected and position where projectile stops is not the same as hit position (for eg arrows)
                    Matrix transM;
                    transM.BuildFromDirectionVector( dirVec );

                    Teleport( infos[i].m_position, transM.ToEulerAngles() );
                }

				CPhysicsEngine::CollisionMask mask;
				if ( infos[i].m_actorNshapeIndex.m_actorIndex != 0 || infos[i].m_actorNshapeIndex.m_shapeIndex != 0 )
				{
					mask = infos[i].m_userData->GetCollisionTypesBits( infos[i].m_actorNshapeIndex.m_actorIndex, infos[i].m_actorNshapeIndex.m_shapeIndex );
				}
				else
				{
					mask = infos[i].m_userData->GetCollisionTypesBits();
				}
                TDynArray< CName > hitCollisionsGroups;
				GPhysicEngine->GetGroupsNamesByCollisionMask( mask, hitCollisionsGroups );
				if ( wasWaterHit )
				{
					hitCollisionsGroups.PushBack( CNAME( Water ) );
				}
                THandle< CComponent > componentHandle( actorComponent );
                CallEvent( CNAME( OnProjectileCollision ), boxPos, -dirVec, componentHandle, hitCollisionsGroups, (Int32)infos[i].m_actorNshapeIndex.m_actorIndex, (Int32)infos[i].m_actorNshapeIndex.m_shapeIndex );
                
#ifndef NO_EDITOR
                if ( !hitCollisionsGroups.Exist( CNAME( Terrain ) ) )
                {
                    m_dbgGlobalHitPos.PushBack( infos[i].m_position );
                }
#endif
            }
        } // for all results
    } // if any hits found

	return wasHit;
}

//////////////////////////////////////////////////////////////////////////

// void CProjectileTrajectory::OnSaveGameplayState( IGameSaver* saver )
// {
//     TBaseClass::OnSaveGameplayState( saver );
// 
//     // store state of every animated property
//     CGameSaverBlock block( saver, CNAME(ProjectileTrajectory) );
// 
//     saver->WriteValue( CNAME( projectileCaster ), m_caster );
//     saver->WriteValue( CNAME( projectileProjectileName ), m_projectileName);
//     saver->WriteValue( CNAME( projectileStarted ), m_started );
//     saver->WriteValue( CNAME( projectileProjectileVelocity ), m_projectileVelocity );
//     saver->WriteValue( CNAME( projectileMaxRange ), m_maxRange );
//     saver->WriteValue( CNAME( projectileDistanceTraveled ), m_distanceTraveled );
//     saver->WriteValue( CNAME( projectileRadius ), m_radius );
//     saver->WriteValue( CNAME( projectileCollisionGroups ), m_collisionGroups );
//     saver->WriteValue( CNAME( projectileStartPosition ), m_startPosition );
//     saver->WriteValue( CNAME( projectileCurveProgress ), m_curveProgress );
//     saver->WriteValue( CNAME( projectileInitialTargetDistance ), m_initialTargetDistance );
//     saver->WriteValue( CNAME( projectileVelocityVec ), m_velocityVec );
//     saver->WriteValue( CNAME( projectileSineAmplitude ), m_sineAmplitude );
//     saver->WriteValue( CNAME( projectileBounceOfVelocityPreserve ), m_bounceOfVelocityPreserve );
//     saver->WriteValue( CNAME( projectileCakeAngle ), m_cakeAngle );
//     saver->WriteValue( CNAME( projectileDeltaStep ), m_deltaStep );
//     saver->WriteValue( CNAME( projectileLastStartStep ), m_lastStartStep );
// 
//     EProjectileTargetType type = PTT_Null;
//     if( m_projectileTarget != nullptr )
//         type = m_projectileTarget->GetType();
// 
//     saver->WriteValue( CNAME( projectileTargetType ), (Uint32)type );
// 
//     if( m_projectileTarget != nullptr )
//         m_projectileTarget->OnSaveGameplayState( saver );
// }

//////////////////////////////////////////////////////////////////////////

// void CProjectileTrajectory::OnLoadGameplayState( IGameLoader* loader )
// {
//     TBaseClass::OnLoadGameplayState( loader );
// 
//     // get no of props
//     CGameSaverBlock block( loader, CNAME(ProjectileTrajectory) );
// 
//     m_caster                    = loader->ReadValue< THandle<CEntity> >( CNAME(projectileCaster) );
//     m_projectileName            = loader->ReadValue< CName >( CNAME(projectileProjectileName), CName::NONE );
//     m_started                   = loader->ReadValue< Bool >( CNAME(projectileStarted) );
//     m_projectileVelocity        = loader->ReadValue< Float >( CNAME(projectileProjectileVelocity) );
//     m_maxRange                  = loader->ReadValue< Float >( CNAME(projectileMaxRange) );
//     m_distanceTraveled          = loader->ReadValue< Float >( CNAME(projectileDistanceTraveled) );
//     m_radius                    = loader->ReadValue< Float >( CNAME(projectileRadius) );
//     m_collisionGroups           = loader->ReadValue< Uint64 >( CNAME(projectileCollisionGroups) );
//     m_startPosition             = loader->ReadValue< Vector >( CNAME(projectileStartPosition) );
//     m_curveProgress             = loader->ReadValue< Float >( CNAME(projectileCurveProgress) );
//     m_initialTargetDistance     = loader->ReadValue< Float >( CNAME(projectileInitialTargetDistance) );
//     m_velocityVec               = loader->ReadValue< Vector >( CNAME(projectileVelocityVec) );
//     m_sineAmplitude             = loader->ReadValue< Float >( CNAME(projectileSineAmplitude) );
//     m_bounceOfVelocityPreserve  = loader->ReadValue< Float >( CNAME(projectileBounceOfVelocityPreserve) );
//     m_cakeAngle                 = loader->ReadValue< Float >( CNAME(projectileCakeAngle) );
//     m_deltaStep                 = loader->ReadValue< Float >( CNAME(projectileDeltaStep) );
//     m_lastStartStep             = loader->ReadValue< Vector >( CNAME(projectileLastStartStep) );
// 
//     EProjectileTargetType type  = (EProjectileTargetType)loader->ReadValue< Uint32 >( CNAME(projectileTargetType) );
// 
//     if( m_projectileTarget != nullptr )
//     {
//         delete m_projectileTarget;
//         m_projectileTarget = nullptr;
//     }
// 
//     switch( type )
//     {
//     case PTT_Fixed:
//         m_projectileTarget = new CFixedTarget( Vector::ZEROS );
//         break;
//     case PTT_Static:
//         m_projectileTarget = new CStaticTarget( nullptr, 0.0f );
//         break;
//     case PTT_Node:
//         m_projectileTarget = new CNodeTarget( nullptr );
//         break;
//     case PTT_Bone:
//         m_projectileTarget = new CBoneTarget( nullptr, 0 );
//         break;
//     }
// 
//     if( m_projectileTarget != nullptr )
//         m_projectileTarget->OnLoadGameplayState( loader );
// }

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
    TBaseClass::OnGenerateEditorFragments( frame, flags );

#ifndef NO_EDITOR

//     const Vector worldPos = GetWorldPosition();
//     frame->AddDebugSphere( worldPos, 0.025f, Matrix::IDENTITY, Color::RED );
//     frame->AddDebugText( worldPos, String::Printf( TXT("x: %.1f y: %.1f z: %.1f"), worldPos.X, worldPos.Y, worldPos.Z ), 0, 0, true );

    if ( m_started )
    {
        // Draw projectile trajectory
        if( !m_dbgTrajcetory.Empty() )
        {
            Vector previous = m_startPosition;
            for( Uint32 i = 0; i<m_dbgTrajcetory.Size(); ++i)
            {
                frame->AddDebugLine( previous, m_dbgTrajcetory[i], Color::BLUE );
                previous = m_dbgTrajcetory[i];
            }
        }

        // Draw all hits
        if( !m_dbgGlobalHitPos.Empty() )
            for(  Uint32 i = 0; i<m_dbgGlobalHitPos.Size(); ++i )
                frame->AddDebug3DArrow( m_dbgGlobalHitPos[i], Vector(0,0,1.0f), 2.0f, 0.02f, 0.03f, 0.2f, Color::RED, Color::RED );

        // Amount of water tests done
        //frame->AddDebugText( m_lastStartStep,  String::Printf( TXT("Water level test: %d"), m_dbgWaterTestsNo), 0, 0 );

//         // Point objects that are behind the wall
//         if( !m_dbgWallTest.Empty() )
//         {
//             const Uint32 count = m_dbgWallTest.Size();
//             for( Uint32 i=0; i<count; ++i )
//             {
//                 Vector dir = m_dbgWallTest[i] - m_startPosition;
//                 Float scale = dir.Normalize3();
// 
//                 frame->AddDebug3DArrow( m_startPosition, dir, scale, 0.02f, 0.03f, 0.2f, Color::RED, Color::RED );
//             }
//         }

        // Draw sweep test per step
        if( m_dbgBoxHalfExtent != Vector::ZEROS && m_radius > 0.0f )
        {
            Matrix orient;
            orient.BuildFromDirectionVector( m_dbgDirVec.Normalized3() );
            orient.SetTranslation(m_dbgBoxPosition);

            Box box;
            box.Min = Vector::ZEROS;
            box.Max = Vector::ZEROS;
            box.Extrude( m_dbgBoxHalfExtent );
            frame->AddDebugBox( box, orient, Color::WHITE, true );

            // Draw all trajectory
            if( m_cakeAngle > 0.0f ) // Cake
            {
                // TODO
            }
            else // Overlap
                frame->AddDebugWireframeTube( m_startPosition, m_projectileTarget->GetWorldPosition(), m_radius, m_radius, Matrix::IDENTITY, Color::WHITE, Color::WHITE );
        }
        // Draw raycast per test
        else
        {
            Float scale = m_dbgDirVec.Normalize3();

            // Draw current step
            frame->AddDebug3DArrow( m_lastStartStep, m_dbgDirVec, scale, 0.03f, 0.04f, 0.1f, Color::GREEN, Color::GREEN );

            // Draw whole trajectory
            m_dbgDirVec = m_projectileTarget->GetWorldPosition() - m_startPosition;
            scale = m_dbgDirVec.Normalize3();
            frame->AddDebug3DArrow( m_startPosition, m_dbgDirVec, scale, 0.02f, 0.03f, 0.1f, Color::WHITE, Color::WHITE );
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Collision hits
    const Float deltaTime = GGame->GetTimeManager()->GetLastTickTime();
    
    // Spheres
    for( Uint32 i=0; i<m_dbgOverlapSpheres.Size(); ++i )
    {
        SDebugTimeoutPoint& sphere = m_dbgOverlapSpheres[i];
        sphere.totalDrawTime += deltaTime;
        if( sphere.totalDrawTime >= SHOW_TIMEOUT )
        {
            m_dbgOverlapSpheres.Erase( m_dbgOverlapSpheres.Begin() + i );

            if( i > 0)
                --i;
        }
        else
            frame->AddDebugSphere( sphere.hitPos, sphere.radius, Matrix::IDENTITY, Color::RED );
    }

    // Hits
    for( Uint32 i=0; i<m_dbgOverlapHits.Size(); ++i )
    {
        SDebugTimeoutPoint& hit = m_dbgOverlapHits[i];
        hit.totalDrawTime += deltaTime;
        if( hit.totalDrawTime-1.0f >= SHOW_TIMEOUT )
        {
            m_dbgOverlapHits.Erase( m_dbgOverlapHits.Begin() + i );

            if( i > 0)
                --i;
        }
        else
        {
            frame->AddDebugText( hit.hitPos, hit.name.AsChar(), 0, 0, true );
            frame->AddDebugSphere( hit.hitPos, 0.3f, Matrix::IDENTITY, Color::GREEN );
        }
    }
    //////////////////////////////////////////////////////////////////////////

#endif // NO_EDITOR
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::StartProjectile( IProjectileTarget* target, Float angle, Float velocity, Float projectileRange )
{
    if( target == NULL)
    {
        return;
    }

    if( m_projectileTarget != NULL )
    {
        delete m_projectileTarget;
    }

    m_uniqueCollisionComponents.Clear();

#ifndef NO_EDITOR
    m_dbgTrajcetory.Clear();
#endif

	ForceUpdateTransformNodeAndCommitChanges();

    m_distanceTraveled      = 0.0f;
    m_projectileTarget      = target;
    m_startPosition         = GetWorldPosition();
    m_projectileVelocity    = velocity;
    m_maxRange              = projectileRange;
    m_started               = true;
    m_lastStartStep         = m_startPosition;
    m_prevTrajectoryPositon = m_startPosition;
    m_initialTargetDistance = m_startPosition.DistanceTo( m_projectileTarget->GetWorldPosition() ); // Get length factor in 2D
    m_curveProgress         = 1.0f;
    m_sineAmplitude         = 0.0f;
    m_velocityVec           = ( m_projectileTarget->GetWorldPosition() - m_startPosition ).Normalized3();

    if( angle != 0.0f )
    {
        // clamp to avoid infinity in tan
        angle = ::Min( angle, 89.5f );
        angle = ::Max( angle, -89.5f );
        if( angle < 0 )
        {
            angle = 1.0f - angle / -89.5f;
            angle = ::Max( angle, 0.001f );
        }
        m_curveProgress = 0.0f;

        // Get sin amplitude and curve bend factor
        m_sineAmplitude = MTan( DEG2RAD( angle ) ) * m_initialTargetDistance * 0.5f / ( M_PI / 2.0f );
    }

    m_velocityVec *= velocity;

#ifndef NO_EDITOR
    const Float hExtent = 1.25f * m_radius;                  // compare areas a^2 = sqrt( 2*PI*radius^2 ) ==> a~= 2.5*radius
    m_dbgBoxHalfExtent.Set3( hExtent, 0.1f, hExtent );
#endif

    // Play animated curves
	const Float curveAnimationLength = m_initialTargetDistance / m_projectileVelocity;
	PlayPropertyAnimation( CName::NONE, 1, curveAnimationLength );
}

//////////////////////////////////////////////////////////////////////////

Vector CProjectileTrajectory::CalculateNewPosition( Float deltaTime  )
{
    // Calculate linear
    if( m_curveProgress == 1.0f )
    {
        // If we were dealing with motion under gravity
        if( m_sineAmplitude > 0.0f )
        {
            m_velocityVec.Z -= 9.81f * deltaTime;
            if( m_velocityVec.Z >= 200.0f )
                m_velocityVec.Z = 200.0f;
        }

        return m_prevTrajectoryPositon + m_velocityVec * deltaTime;
    }

    // Calculate Z offset
    m_curveProgress += ( m_projectileVelocity * deltaTime) / m_initialTargetDistance;
    if(m_curveProgress > 1.0f )
    {
        m_curveProgress = 1.0f;
    }

    // Linear movement interpolation
    Vector newPosition = m_startPosition + ( m_projectileTarget->GetWorldPosition() - m_startPosition ) * m_curveProgress;

    const Float sine = sinf( m_curveProgress * M_PI ) * m_sineAmplitude;
    newPosition.Z += sine;

    // Calculate velocity vector for linear movement
    if( m_curveProgress == 1.0f )
    {
        m_velocityVec = ( newPosition - m_prevTrajectoryPositon ).Normalized3() * m_projectileVelocity;
    }

    return newPosition;
}

//////////////////////////////////////////////////////////////////////////

Bool CProjectileTrajectory::IsBehindWall( const Vector& startPoision, CComponent* component, CPhysicsEngine::CollisionMask include )
{
    // If component is null, it can mean that we hit the terrain and then we receive no CComponent from overlap query
    // See CProjectileTrajectory::CheckForCollisions for more info
    if( component == NULL )
        return false;

    CPhysicsWorld* phyWorld = nullptr;
	GetLayer()->GetWorld()->GetPhysicsWorld( phyWorld );

    // For characters GetWorldPosition returns feet position. Raycast can hit the ground
    Vector pos = component->GetWorldPosition();
    // So modify position Z component so it is the same as current overlap step endPoint
    pos.Z = m_lastStartStep.Z;

    SPhysicsContactInfo info;
	Bool isBehindWall =  phyWorld->RayCastWithSingleResult( startPoision, pos, include, 0, info ) == TRV_Hit;
    // We can hit the same object we want to test
    if( isBehindWall )
    {
        CComponent* foundComponent = nullptr;
		info.m_userDataA->GetParent( foundComponent, info.m_rigidBodyIndexA.m_actorIndex );
        isBehindWall = component != foundComponent;  // No hit if its the same object
    }

// #ifndef NO_EDITOR
//     if( isBehindWall )
//         m_dbgWallTest.PushBack( pos );
// #endif

    return isBehindWall;
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::StopProjectile( )
{
    m_collisionGroups = 0;
    m_started = false;
    m_cakeAngle = 0.0f;
    m_deltaStep = 0.0f;
    m_uniqueCollisionComponents.Clear();

    // Force set transform
    const Matrix& globalPose = GetLocalToWorld();
    Teleport( globalPose.GetTranslation(), globalPose.ToEulerAngles() );

#ifndef NO_EDITOR
    m_dbgBoxHalfExtent = Vector::ZEROS;
    m_dbgBoxPosition = Vector::ZEROS;
#endif
}

//////////////////////////////////////////////////////////////////////////

Bool CProjectileTrajectory::SphereOverlapTest( const Vector& position, Float radius, CPhysicsEngine::CollisionMask collisionMask )
{
	PC_SCOPE( Projectile_SphereOverlapTest );

	if ( radius == 0.0f || collisionMask == 0 )
	{
		return false;
	}

#ifndef NO_EDITOR
	SDebugTimeoutPoint spn(position);
	spn.radius = radius;
	m_dbgOverlapSpheres.PushBack( spn );
#endif

	static const CPhysicsEngine::CollisionMask charMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) );
	Bool wasHit = false;

	// overlap test for all collision groups which are not 'Character'
	if ( collisionMask & ( ~charMask ) )
	{
		// disable character checking - will be tested below
		CPhysicsEngine::CollisionMask physicsCollisionMask = collisionMask & ( ~charMask );
		wasHit = SphereOverlapTest_PhysX( position, radius, physicsCollisionMask );
	}

	// gameplay characters test
	if ( collisionMask & charMask )
	{
		wasHit = SphereOverlapTest_Gameplay( position, radius ) || wasHit;
	}

	CallEvent( CNAME( OnRangeReached ) );

	return wasHit;
}

Bool CProjectileTrajectory::SphereOverlapTest_PhysX( const Vector& position, Float radius, CPhysicsEngine::CollisionMask collisionMask )
{
	SPhysicsOverlapInfo infos[ PROJECTILE_MAX_AREA_TEST_RESULTS ];
	Uint32 hits = 0;
	Bool wasHit = false;

	{
		PC_SCOPE( Projectile_PhysX_SphereOverlapTest );

		CPhysicsWorld* phyWorld = nullptr;
		GetLayer()->GetWorld()->GetPhysicsWorld( phyWorld );
		if ( phyWorld != nullptr )
		{
			const ETraceReturnValue& retVal = phyWorld->SphereOverlapWithMultipleResults( position, radius, collisionMask, 0, infos, hits, PROJECTILE_MAX_AREA_TEST_RESULTS );
		}
	}

	if ( hits > 0 )
	{
		PC_SCOPE( Projectile_PhysX_SphereOverlapTest_Handle );
		TSortedSet< CComponent* > uniqueCollisions;
		for ( Uint32 i = 0; i < hits; ++i )
		{
			if ( infos[i].m_userData == nullptr )
			{
				continue;
			}

			CComponent* actorComponent = nullptr;
			infos[i].m_userData->GetParent( actorComponent, infos[i].m_actorNshapeIndex.m_actorIndex );
			Bool callEvents = false;

			if ( actorComponent == nullptr )
			{
				// If we hit terrain infos[i].m_userDataA is TerrainWrapper but no CComponent is return upon calling GetParent(index)
				callEvents = true;
			}
			else
			{
				CEntity* componentEntity = actorComponent->GetEntity();
				if( componentEntity != nullptr && componentEntity != m_caster.Get() && !uniqueCollisions.Exist( actorComponent ) )
				{
					// Collision with other objects
					// Test for unique actors... (SIC!!)
					// Physics returns all shapes that raycast hits
					// Since we want only actors affected by raycast and actor can have multiple shapes...
					// Filter actors we already have hit
					uniqueCollisions.Insert( actorComponent );
					callEvents = true;
				}
			}

			if ( callEvents )
			{
				wasHit = true;
				CPhysicsEngine::CollisionMask mask;
				if ( infos[i].m_actorNshapeIndex.m_actorIndex != 0 || infos[i].m_actorNshapeIndex.m_shapeIndex != 0 )
				{
					mask = infos[i].m_userData->GetCollisionTypesBits( infos[i].m_actorNshapeIndex.m_actorIndex, infos[i].m_actorNshapeIndex.m_shapeIndex );
				}
				else
				{
					mask = infos[i].m_userData->GetCollisionTypesBits();
				}
				TDynArray< CName > hitCollisionsGroups;
				GPhysicEngine->GetGroupsNamesByCollisionMask( mask, hitCollisionsGroups );
				THandle< CComponent > componentHandle( actorComponent );
				Vector dir = position - infos[i].m_position;
				CallEvent( CNAME( OnProjectileCollision ), position, dir, componentHandle, hitCollisionsGroups, (Int32)infos[i].m_actorNshapeIndex.m_actorIndex, (Int32)infos[i].m_actorNshapeIndex.m_shapeIndex );

#ifndef NO_EDITOR
				SDebugTimeoutPoint spn(infos[i].m_position);
				String s;
				for ( Uint32 j = 0; j < hitCollisionsGroups.Size(); ++j )
				{
					s = s + hitCollisionsGroups[ 0 ].AsString();
					if ( j < hitCollisionsGroups.Size() - 1 )
					{
						s = s + TXT(", ");
					}
				}
				spn.name = s;
				m_dbgOverlapHits.PushBack( spn );
#endif
			}
		}
	}

	return wasHit;
}

Bool CProjectileTrajectory::SphereOverlapTest_Gameplay( const Vector& position, Float radius )
{
	CActor* foundActorsAround[ PROJECTILE_MAX_AREA_TEST_RESULTS ];
	Int32 resultsCount = 0;
	Uint32 hits = 0;
	GameplayStorageAcceptors::SphereAcceptor sphereAcceptor( position, radius );

	{
		PC_SCOPE( Projectile_GP_SphereOverlapTest );

		// get actors on ray
		resultsCount = GamePlayOverlapTestOnActors( position, radius, foundActorsAround, sphereAcceptor );
	}

	if ( resultsCount > 0 )
	{
		PC_SCOPE( Projectile_GP_SphereOverlapTest_Handle );

		for ( Int32 i = 0; i < resultsCount; ++i )
		{
			CActor* actor = foundActorsAround[ i ];
			if ( !actor )
				continue;

			CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
			if ( mpac && mpac->GetPhysicalCharacter() )
			{
				Bool wasHit = false;
				AACylinder cylinder;

#ifdef USE_PHYSX
				// check its virtual controllers
				if ( mpac->GetPhysicalCharacter()->GetCharacterController() )
				{
					typedef TDynArray< CVirtualCharacterController > VCCArray;
					const VCCArray& virtualControllers = mpac->GetPhysicalCharacter()->GetCharacterController()->GetVirtualControllers();
					VCCArray::const_iterator itEnd = virtualControllers.End();
					for ( VCCArray::const_iterator it = virtualControllers.Begin(); it != itEnd; ++it )
					{
						if ( it->IsEnabled() )
						{
							cylinder.m_positionAndRadius = it->GetGlobalPosition();
							cylinder.m_positionAndRadius.W = it->GetCurrentRadius();
							cylinder.m_height = it->GetCurrentHeight();

							if ( sphereAcceptor( cylinder ) )
							{
								wasHit = true;
								break;
							}
						}
					}
				}
#endif // USE_PHYSX

				// if wasn't hit yet, check physical controller
				const Vector& worldPosition = actor->GetWorldPositionRef();
				if ( !wasHit && mpac->GetPhysicalCharacter()->GetCollisionControllerExtents( cylinder, worldPosition ) )
				{
					if ( sphereAcceptor( cylinder ) )
					{
						wasHit = true;
					}
				}

				// if sth was hit
				if ( wasHit )
				{
					PC_SCOPE( Projectile_GP_SphereOverlapTest_OnProjectileCollision );

					// call scripts handler
					THandle< CComponent > componentHandle( mpac );
					TDynArray< CName > hitCollisionsGroups;
					hitCollisionsGroups.PushBack( CNAME( Character ) );
					const Vector hitDirection = worldPosition - position;
					CallEvent( CNAME( OnProjectileCollision ), worldPosition, hitDirection, componentHandle, hitCollisionsGroups, 0, 0 );

					hits++;
				}	
			}
		}
	}

	return hits > 0 ;
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcInit( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( THandle< CEntity >, caster, NULL );
    FINISH_PARAMETERS;

    CEntity *pCaster = caster.Get();
    Initialize( pCaster );
};

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcShootProjectileAtPosition( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Float, angle, 0.0f );
    GET_PARAMETER( Float, vel, 1.0f );
    GET_PARAMETER( Vector, targetPos, Vector::ZEROS );
    GET_PARAMETER_OPT( Float, range, -1.0f );
    GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
    FINISH_PARAMETERS;

    // Get collisionGroups
    if ( collisionGroupsNames.Empty() )
        m_collisionGroups = GPhysicEngine->GetCollisionGroupMask( CNAME( Projectile ) );
    else
		m_collisionGroups = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );

    CFixedTarget* target = new CFixedTarget( targetPos );

    StartProjectile( target, angle, vel, range );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcShootProjectileAtNode( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Float, angle, 0.0f );
    GET_PARAMETER( Float, vel, 1.0f );
    GET_PARAMETER( THandle< CNode >, targetNode, NULL );
    GET_PARAMETER_OPT( Float, range, -1.0f );
    GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
    FINISH_PARAMETERS;

    if ( collisionGroupsNames.Empty() )
        m_collisionGroups = GPhysicEngine->GetCollisionGroupMask( CNAME( Projectile ) );
    else
		m_collisionGroups = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );

    // calculate initial position of the target
    IProjectileTarget* target = NULL;

    CNode* node = targetNode.Get();
    if ( node )
    {
        Int32 boneIndex = -1;
        CEntity* entity = Cast< CEntity >( node );
        const ISkeletonDataProvider* provider = ( entity && entity->GetRootAnimatedComponent() ) ? entity->GetRootAnimatedComponent()->QuerySkeletonDataProvider() : NULL;
        if ( provider )
        {
            boneIndex = provider->FindBoneByName( TXT("torso") );
        }

        if ( provider && boneIndex >= 0 )
        {
            THandle< CEntity > entityHandle( entity );
            target = new CBoneTarget( entityHandle, boneIndex );
        }
        else
        {
            target = new CNodeTarget( node );
        }
    }
    else
    {
        target = new CStaticTarget( this, range );
    }

    StartProjectile( target, angle, vel, range );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcStopProjectile( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    StopProjectile();
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcShootCakeProjectileAtPosition( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Float, cakeAngle, 0.0f );
    GET_PARAMETER( Float, cakeHeight, 0.0f );

    GET_PARAMETER( Float, shootAngle, 0.0f );
    GET_PARAMETER( Float, velocity, 1.0f );
    GET_PARAMETER( Vector, target, Vector::ZEROS );
    GET_PARAMETER( Float, range, 0.0f );
    GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
    FINISH_PARAMETERS;

    if ( collisionGroupsNames.Empty() )
        m_collisionGroups = GPhysicEngine->GetCollisionGroupMask( CNAME( Projectile ) );
    else
		m_collisionGroups = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );

    m_radius = cakeHeight;
    m_cakeAngle = cakeAngle;
    IProjectileTarget* targeter = new CFixedTarget( target );

    StartProjectile( targeter, shootAngle, velocity, range );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcBounceOff( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( Vector, collisionNormal, Vector::ZEROS );
    GET_PARAMETER( Vector, collisionPosition, Vector::ZEROS );
    FINISH_PARAMETERS;
    
    Vector heading		= GetWorldRotation().TransformVector( Vector::EY );
    Vector newHeading	= heading - collisionNormal * ( 2 * Vector::Dot3( collisionNormal, heading ) );

    // Heading vector projected on XY plane
    Vector newHeadingXY = newHeading;
    newHeadingXY.Z = 0.0f;
    newHeadingXY.Normalize2();

    // Calculate new target position
    Vector offset = collisionPosition + newHeadingXY * 3.0;

    // New angle from reflection vector;
    Float angle = RAD2DEG( MAcos_safe( Vector::Dot2( newHeading, newHeadingXY ) ) );

    CFixedTarget* target = new CFixedTarget( offset );
    StartProjectile( target, angle, m_projectileVelocity * m_bounceOfVelocityPreserve, m_maxRange * m_bounceOfVelocityPreserve );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcIsBehindWall( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( THandle< CComponent >, component, NULL );
    GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
    FINISH_PARAMETERS;

    CPhysicsEngine::CollisionMask collisionMask = 0;

    if ( collisionGroupsNames.Empty() )
    {
        collisionMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
    }
    else
    {
		collisionMask = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );
    }

    RETURN_BOOL( IsBehindWall( m_startPosition, component.Get(), collisionMask ) );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcSphereOverlapTest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, radius, 0 );
	GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
	FINISH_PARAMETERS;

	CPhysicsEngine::CollisionMask collisionMask = 0;

	if ( collisionGroupsNames.Empty() )
		collisionMask = GPhysicEngine->GetCollisionGroupMask( CNAME( Projectile ) );
	else
		collisionMask = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );

	m_sphereOverlapTestData.PushBack( SSphereOverlapTestData( GetWorldPosition(), radius, collisionMask ) );
}

//////////////////////////////////////////////////////////////////////////

void CProjectileTrajectory::funcIsStopped( CScriptStackFrame& stack, void* result )
{
    FINISH_PARAMETERS;
    RETURN_BOOL( !m_started );
}

//////////////////////////////////////////////////////////////
void CProjectileTrajectory::funcShootProjectileAtBone(CScriptStackFrame& stack, void* result)
{
	GET_PARAMETER( Float, angle, 0.0f );
	GET_PARAMETER( Float, vel, 1.0f );
	GET_PARAMETER( THandle< CEntity >, target, nullptr );
	GET_PARAMETER( CName, targetBone, CName::NONE );
	GET_PARAMETER_OPT( Float, range, -1.0f );
	GET_PARAMETER_OPT( TDynArray<CName>, collisionGroupsNames, TDynArray<CName>() );
	FINISH_PARAMETERS;

	if ( collisionGroupsNames.Empty() )
		m_collisionGroups = GPhysicEngine->GetCollisionGroupMask( CNAME( Projectile ) );
	else
		m_collisionGroups = GPhysicEngine->GetCollisionTypeBit( collisionGroupsNames );

	IProjectileTarget* proj_target = nullptr;

	Int32 boneIndex = -1;
	const ISkeletonDataProvider* provider = ( target && target->GetRootAnimatedComponent() ) ? target->GetRootAnimatedComponent()->QuerySkeletonDataProvider() : nullptr;

	RED_WARNING( provider != nullptr, "funcShootProjectileAtBone: skeleton provider does not exist for given entity" );

	if ( provider )
	{
		boneIndex = provider->FindBoneByName( targetBone );

		RED_WARNING( boneIndex >= 0, "funcShootProjectileAtBone: skeleton does not have >%s< bone!", targetBone.AsChar());

		if ( boneIndex >= 0 )
		{
			THandle< CEntity > entityHandle( target );
			proj_target = new CBoneTarget( entityHandle, boneIndex );

			// Fire a projectile!
			StartProjectile( proj_target, angle, vel, range );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
