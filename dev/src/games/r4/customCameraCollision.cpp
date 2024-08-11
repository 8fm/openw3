#include "build.h"
#include "customCameraCollision.h"
#include "customCamera.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/game/itemIterator.h"
#include "../../common/engine/curve.h"
#include "../../common/physics/physicsWorld.h"
#include "../../common/physics/physicsWorldUtils.h"
#include "../../common/physics/physicsWrapper.h"
#include "../../common/engine/renderProxyIterator.h"
#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/characterControllerParam.h"

#include "../../common/engine/visualDebug.h"

#include "../../common/physics/physicsSettings.h"

RED_DEFINE_STATIC_NAME( GetCameraPadding );

IMPLEMENT_ENGINE_CLASS( ICustomCameraCollisionController );

Bool ICustomCameraCollisionController::SweepCheck( Float radius, const Vector& start, const Vector& end )
{
	CPhysicsWorld* physics = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physics ) )
	{
		return 0;
	}

	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Camera ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionType );

	SPhysicsContactInfo contact;
	return physics->SweepTestWithSingleResult( start, end, radius, include, 0, contact ) == TRV_Hit;
}

Bool ICustomCameraCollisionController::Sweep( Float radius, const Vector& start, const Vector& end, Float& t, Vector& hitPosition )
{
	CPhysicsWorld* physics = nullptr;
	if ( ! GGame->GetActiveWorld()->GetPhysicsWorld( physics ) )
	{
		return 0;
	}

	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Camera ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionType );

	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionTypeOccludable = GPhysicEngine->GetCollisionTypeBit( CNAME( CameraOccludable ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask exclude = GPhysicEngine->GetCollisionGroupMask( collisionTypeOccludable );

	Bool result = false;
	SPhysicsContactInfo contact;
	if( physics->SweepTestWithSingleResult( start, end, radius, include, exclude, contact, true ) == TRV_Hit )
	{
		ASSERT( contact.m_distance >= 0.f );

		t = contact.m_distance;
		hitPosition = contact.m_position;

		result = true;
	}

	

	return result;
}

Bool ICustomCameraCollisionController::CheckSphereOverlap( const Vector& position, Float radius, TDynArray< CollosionPoint >* contacts )
{
	CPhysicsWorld* physics = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physics ) )
	{
		return false;
	}

	SPhysicsOverlapInfo hits[16];
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Camera ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionType );
	Uint32 numHits = 0;
	const ETraceReturnValue retVal = physics->SphereOverlapWithMultipleResults( position, radius, include, 0, hits, numHits, 16 );

	Bool col = false;

	// Process hits
	for ( Uint32 i = 0; i < numHits; ++i )
	{
		SPhysicsOverlapInfo& contact = hits[ i ];

		// Check penetration
		const Float colDist = contact.m_penetration;
		if ( colDist < 0.f )
		{
			col = true;

			if ( contacts )
			{
				contacts->PushBack( CollosionPoint( contact.m_position, contact.m_penetration ) );
			}
			else
			{
				break;
			}
		}
	}

	return col;
}

Bool ICustomCameraCollisionController::CheckPlayerOverlap( const Vector& position, Float radius )
{
	CPhysicsWorld* physics = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physics ) )
	{
		return false;
	}

	const CPlayer* player = GCommonGame->GetPlayer();
	if( player )
	{
		CVisualDebug* vDebug = player->GetVisualDebug();
		if( vDebug )
		{
			vDebug->AddSphere( CNAME( Ragdoll ), radius, position, true, Color::BLUE );
		}
	}

	static const Uint8 c_numContactsMax = 8;
	SPhysicsOverlapInfo overlapInfoList[ c_numContactsMax ];
	Uint32 numContacts = 0;
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) );
	const ETraceReturnValue result = physics->SphereOverlapWithMultipleResults( position, radius, include, 0, overlapInfoList, numContacts, c_numContactsMax );
	if( result == TRV_Hit )
	{
		const CObject* playerObject = GGame->GetPlayerEntity();
		for( Uint32 i = 0; i < numContacts; ++i )
		{
			const SPhysicsOverlapInfo& info = overlapInfoList[ i ];
			CPhysicsWrapperInterface* physicsData = info.m_userData;
			CObject* object = nullptr;
 			physicsData->GetParent( object, info.m_actorNshapeIndex.m_actorIndex );
			if( object )
			{
				if( object->GetParent() == playerObject )
				{
					return true;
				}
			}
		}
	}

	return false;
}


void ICustomCameraCollisionController::FadeOutCameraOccludables( Float radius, const Vector& start, const Vector& end )
{
	PC_SCOPE_PIX( FadeCameraOccludables );
	//Fade objects.
	CPhysicsWorld* physics = nullptr;
	if ( !GGame->GetActiveWorld()->GetPhysicsWorld( physics ) )
	{
		return;
	}

	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionTypeOccludable = GPhysicEngine->GetCollisionTypeBit( CNAME( CameraOccludable ) );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionTypeOccludable );
	//Might be moved to task if would take too much time
	
#ifdef USE_PHYSX
	FadeOutCameraOccludablesOp fadeOutOp;
	Bool hit = CCustomCameraSweepHelper::SphereSweep( start, end, radius, include, 0, physics, &fadeOutOp ); 
#endif
}

void ICustomCameraCollisionController::FadeOutPlayerWeapons( SCameraMovementData& data )
{
	ICamera::Data cameraData;
	data.m_camera.Get()->GetData( cameraData );

	CPhysicsWorld* physics;
	GGame->GetActiveWorld()->GetPhysicsWorld( physics );
	if( physics )
	{
		// Create a collision volume to encapsulate the near plane
		const Float c_nearPlane = cameraData.m_nearPlane / 10.0f;
		Vector halfExtents( Vector::ZEROS );
		{
			static const Float c_boundsPadding = 0.05f;
			halfExtents.X = ( MTan( DEG2RAD( cameraData.m_fov ) / 2 ) * c_nearPlane ) + c_boundsPadding;
			halfExtents.Y = halfExtents.X * 0.1f; // Make the volume thin, it just needs to cover the near plane
			halfExtents.Z = halfExtents.X * 0.3f; // Pretty safe assumption that the viewport x will be bigger than the z
		}

		const Vector cameraLookAt		= cameraData.m_rotation.TransformVector( Vector::EY );
		const Vector nearPlanePosition	= cameraData.m_position + ( cameraLookAt * c_nearPlane );
		const Matrix cameraOrientation	= cameraData.m_rotation.ToMatrix();

		static const CPhysicsEngine::CollisionMask includeMask = GPhysicEngine->GetCollisionGroupMask( CNAME( Ragdoll ) );

#ifdef USE_PHYSX
		FadeOutPlayerWeaponsOp fadeOutOp;
		CCustomCameraSweepHelper::BoxOverlap( nearPlanePosition, halfExtents, cameraOrientation, includeMask, 0, physics, &fadeOutOp );
#endif // USE_PHYSX
	}
}

void ICustomCameraCollisionController::FadeOutPlayer( SCameraMovementData& data, const Vector& cameraPosition, const Vector& cameraLookAtPosition, const Float sweepRadius, const Float collisionVolumePadding )
{
	ICamera::Data cameraData;
	data.m_camera.Get()->GetData( cameraData );
	Float nearPlane = cameraData.m_nearPlane;

	if( CheckPlayerOverlap( cameraPosition, nearPlane + collisionVolumePadding ) )
	{
		const Float dist2D = ( cameraPosition - cameraLookAtPosition ).Mag2();
		const Float dist2DToCapsule = Max( dist2D - sweepRadius - 0.15f, 0.02f );
		data.m_camera.Get()->SetNearPlane( dist2DToCapsule );

		for ( EntityWithItemsComponentIterator< CMeshTypeComponent > it( GCommonGame->GetPlayer() ); it; ++it )
		{
			CMeshTypeComponent* c = *it;

			if ( c->GetRenderProxy() )
			{
				( new CRenderCommand_SetTemporaryFade( c->GetRenderProxy() ) )->Commit();
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CCustomCameraOutdoorCollisionController );

#define DECREASE_DIST_FACTOR 0.2f
#define INCREASE_DIST_FACTOR 1.f

// using default near plane dist should be good enough
#define NEAR_PLANE_DIST 0.25f

CCustomCameraOutdoorCollisionController::CCustomCameraOutdoorCollisionController()
	: m_distanceDmpFactor( DECREASE_DIST_FACTOR )
	, m_distanceDamp( m_distanceDmpFactor )
	, m_smallSphereDistDamp( m_distanceDmpFactor )
	, m_sweepRadius( 0.34f )
	, m_state( ECCS_Initializing )
	, m_reached( false )
	, m_usingSmallSphere( false )
	, m_isInMotion( false )
{
}

CCustomCameraOutdoorCollisionController::~CCustomCameraOutdoorCollisionController()
{
}

void CCustomCameraOutdoorCollisionController::FadeOutCameraOccludablesOp::operator()( CPhysicsWrapperInterface* physicsInterface, const SActorShapeIndex* actorIndex ) const
{
	CMeshTypeComponent * cmtCmp(nullptr);

	IRenderProxy * renderProxy;
	physicsInterface->GetParent(cmtCmp, actorIndex->m_actorIndex);

	if( !cmtCmp )
	{
		CComponent * cmp(nullptr);
		physicsInterface->GetParent(cmp);
		if( cmp )
		{
			const TList<IAttachment *> & children = cmp->GetChildAttachments();
			for(auto it = children.Begin(), end = children.End(); it != end; ++it)
			{
				CNode * node = (*it)->GetChild();
				CMeshTypeComponent * cmtComponent = Cast< CMeshTypeComponent>(node);	

				if( cmtComponent && ((renderProxy = cmtComponent->GetRenderProxy()) != nullptr) )
				{
					( new CRenderCommand_SetTemporaryFade( renderProxy) )->Commit();
				}
			}
		}	
	}
	else if( ((renderProxy = cmtCmp->GetRenderProxy()) != nullptr) )
	{
		( new CRenderCommand_SetTemporaryFade( renderProxy ) )->Commit();
	}
}

void CCustomCameraOutdoorCollisionController::FadeOutPlayerWeaponsOp::operator()( CPhysicsWrapperInterface* physicsInterface, const SActorShapeIndex* actorIndex ) const
{
	CMeshTypeComponent* meshTypeComponent( nullptr );
	physicsInterface->GetParent( meshTypeComponent, actorIndex->m_actorIndex );
	if( meshTypeComponent )
	{
		if( IRenderProxy* renderProxy = meshTypeComponent->GetRenderProxy() )
		{
			if( const CItemEntity* itemEntity = Cast< CItemEntity >( meshTypeComponent->GetParent() ) )
			{
				if( itemEntity->GetParentEntity() == GGame->GetPlayerEntity() )
				{
					( new CRenderCommand_SetTemporaryFade( renderProxy ) )->Commit();
				}
			}
		}
	}
}

void CCustomCameraOutdoorCollisionController::Activate( const SCameraMovementData& data )
{
	m_state = ECCS_Initializing;
}

void CCustomCameraOutdoorCollisionController::Update( SCameraMovementData& data, Float timeDelta )
{
	PC_SCOPE_PIX( CameraCollision );

	const EulerAngles predictedAngle = data.m_pivotRotationValue + data.m_pivotRotationVelocity * 0.15f;
	const Vector pivotWithOffset = data.m_pivotPositionValue + data.m_cameraOffset;
	Vector desiredPosition = pivotWithOffset + data.m_pivotRotationValue.ToMatrix().GetAxisY() * -(data.m_pivotDistanceValue);
	Vector predictedPosition = pivotWithOffset + predictedAngle.ToMatrix().GetAxisY() * -(data.m_pivotDistanceValue);
	const Vector playerPositionNoOffset = GGame->GetPlayerEntity() ? GGame->GetPlayerEntity()->GetWorldPositionRef() : Vector::ZERO_3D_POINT;

	m_isInMotion = !data.m_pivotRotationVelocity.AlmostEquals( EulerAngles::ZEROS ) || data.m_pivotPositionVelocity.SquareMag3() > 0.001f;

	STATIC_GAME_ONLY const Float stepOffset = GGame->GetPlayerEntity() ?
		GGame->GetPlayerEntity()->GetEntityTemplate()->FindGameplayParamT< CCharacterControllerParam >( true )->m_stepOffset + 0.01f :
		/*failsafe*/0.36f;

	const Vector playerPositionLower = playerPositionNoOffset + Vector( 0.f, 0.f, m_sweepRadius + stepOffset );
	const Vector sweepStartPosition = playerPositionNoOffset + m_originOffset;

	static const Vector lookAtOffest = Vector( 0.f, 0.f, 0.3f );
	const Vector lookAtPosition = sweepStartPosition + lookAtOffest;

	Vector direction = desiredPosition - lookAtPosition;
	const Float distance = direction.Normalize3() - NEAR_PLANE_DIST;

	predictedPosition -= (predictedPosition - lookAtPosition).Normalized3() * NEAR_PLANE_DIST;

	Vector hitPosition;

	switch( m_state )
	{
	case ECCS_Initializing:
		{
			m_finalPosition = desiredPosition + direction * NEAR_PLANE_DIST;

			m_distanceDamp.Force( distance );

			m_state = ECCS_Unobstructed;
			// Continue to the unobstructed state
		}

	case ECCS_Unobstructed:
		{
			Float ratio = 1.f;

			if( /*CheckSphereOverlap( predictedPosition, m_sweapRadius, NULL ) &&*/ SweepCheck( m_sweepRadius, playerPositionLower, predictedPosition ) && Sweep( m_sweepRadius, sweepStartPosition, predictedPosition, ratio, hitPosition ) )
			{
				const CPlayer* player = GCommonGame->GetPlayer();
				if( player )
				{
					CVisualDebug* vDebug = player->GetVisualDebug();
					if( vDebug )
					{
						vDebug->AddSphere( CNAME( Ragdoll ), 0.1f, Vector::Interpolate( sweepStartPosition, predictedPosition, ratio ), true, Color::GREEN );
					}
				}

				UpdateCollision( sweepStartPosition, lookAtPosition, desiredPosition, ratio, timeDelta );

				m_state = ECCS_SlidingOnCollision;
				m_reached = false;
			}
			else if( !SweepCheck( m_sweepRadius, playerPositionLower, desiredPosition ) || VerifyPredictedPositionAndUpdateIfNeeded( sweepStartPosition, lookAtPosition, desiredPosition ) )
			{
				/*const Float smallRadius = m_sweapRadius * 0.9f;
				const Float checkDist = smallRadius * smallRadius;
				if( m_finalPosition.DistanceSquaredTo( desiredPosition ) > checkDist
					&& Trace( m_sweapRadius, m_finalPosition, predictedPosition, ratio )
					&& Trace( m_sweapRadius, playerPositionLower, predictedPosition, ratio )
					&& Trace( m_sweapRadius, playerPosition, predictedPosition, ratio ) )
				{
					UpdateCollision( playerPosition, desiredPosition, ratio, timeDelta );

					m_state = ECCS_SlidingOnCollision;
					m_reached = false;
				}
				else
				{*/
					if( !m_reached )
					{
						if( distance < m_distanceDamp.GetValue() )
						{
							// Camera is getting closer on gameplay assume no more collision
							m_distanceDamp.Force( distance );
							m_reached = true;
						}
						else
						{
							if( m_isInMotion )
							{
								m_distanceDmpFactor = INCREASE_DIST_FACTOR;
								m_distanceDamp.Update( distance, timeDelta );
							}

							if( MAbs( m_distanceDamp.GetValue() - distance ) < 0.01f )
							{
								m_reached = true;
							}
						}
					}
					else
					{
						m_distanceDamp.Force( distance );
					}

					m_finalPosition = lookAtPosition + direction * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);
				//}
			}
			break;
		}

	case ECCS_SlidingOnCollision:
		{
			Float ratio = 1.f;

			if( distance < m_distanceDamp.GetValue() )
			{
				m_usingSmallSphere = false;

				// Camera is getting closer on gameplay assume no more collision
				m_distanceDamp.Force( distance );
				m_reached = true;
				m_finalPosition = lookAtPosition + direction * (distance + NEAR_PLANE_DIST);

				m_state = ECCS_Unobstructed;
			}
			else if( false )//!SweepCheck( m_sweepRadius, playerPositionLower, desiredPosition ) )
			{
				// We can again see the player so teleport the camera to a valid position behind the colliding object and keep on increasing distance

				Float newDistance = m_distanceDamp.GetValue();

				if( Sweep( m_sweepRadius, desiredPosition, sweepStartPosition, ratio, hitPosition ) )
				{
					m_sweepPosition = Vector::Interpolate( desiredPosition, lookAtPosition, ratio );

					Vector direction = m_sweepPosition - lookAtPosition;
					newDistance = direction.Normalize3();

					m_distanceDamp.Force( newDistance );
				}
				else
				{
					if( m_isInMotion )
					{
						m_distanceDmpFactor = INCREASE_DIST_FACTOR;
						m_distanceDamp.Update( distance, timeDelta );
					}
					newDistance = m_distanceDamp.GetValue();
				}

				m_finalPosition = lookAtPosition + direction * (newDistance + NEAR_PLANE_DIST);

				m_state = ECCS_Unobstructed;
			}
			else if( Sweep( m_sweepRadius, sweepStartPosition, predictedPosition, ratio, hitPosition ) )
			{
				const CPlayer* player = GCommonGame->GetPlayer();
				if( player )
				{
					CVisualDebug* vDebug = player->GetVisualDebug();
					if( vDebug )
					{
						vDebug->AddSphere( CNAME( Ragdoll ), 0.1f, Vector::Interpolate( sweepStartPosition, predictedPosition, ratio ), true, Color::GREEN );
					}
				}
				
				UpdateCollision( sweepStartPosition, lookAtPosition, desiredPosition, ratio, timeDelta );
			}
			else if( VerifyPredictedPositionAndUpdateIfNeeded( sweepStartPosition, lookAtPosition, desiredPosition ) )
			{
				if( m_usingSmallSphere )
				{
					Float temp = m_smallSphereDistDamp.GetValue() - NEAR_PLANE_DIST;
					Float temp2 = m_smallSphereDistDamp.GetVelocity();
					m_distanceDamp.Update( temp, temp2, temp, 0.f );
					m_usingSmallSphere = false;
				}

				if( m_isInMotion )
				{
					m_distanceDmpFactor = INCREASE_DIST_FACTOR;
					m_distanceDamp.Update( distance, timeDelta );
				}

				m_finalPosition = lookAtPosition + direction * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);

				m_state = ECCS_Unobstructed;
			}
			break;
		}
	}

	m_finalRotation = data.m_pivotRotationValue;

	// Handle near plane cut offs
	const CPlayer* player = GCommonGame->GetPlayer();
	if( m_state == ECCS_SlidingOnCollision && (player && !player->IsInNonGameplayScene()) )
	{
		const Vector hitDirection = (hitPosition - m_sweepPosition).Normalized3();
		const Float angle = MAcos( Vector::Dot3( direction, hitDirection ) );

		if( MAbs( angle ) > 1.22f )
		{
			const Float idealNearPlane = m_sweepRadius / MTan( DEG2RAD(data.m_camera.Get()->GetFoV()) );
			data.m_camera.Get()->SetNearPlane( idealNearPlane );
		}
		else
		{
			data.m_camera.Get()->SetNearPlane( 0.f );
		}

		if( m_distanceDamp.GetValue() < 0.5f )
		{
			const Float sweepRadius = Max( m_distanceDamp.GetValue() * 0.8f, 0.05f );

			const Float idealNearPlane = sweepRadius / MTan( DEG2RAD(data.m_camera.Get()->GetFoV()) );

			Float ratio;
			if( Sweep( sweepRadius, sweepStartPosition, desiredPosition, ratio, hitPosition ) )
			{
				const Vector sweepPosition = Vector::Interpolate( lookAtPosition, desiredPosition, ratio );

				Vector direction = sweepPosition - lookAtPosition;
				const Float distance = direction.Normalize3();
				const Float finalDistance = distance + idealNearPlane;

				if( finalDistance > m_smallSphereDistDamp.GetValue() )
				{
					if( m_usingSmallSphere )
					{
						m_distanceDmpFactor = INCREASE_DIST_FACTOR;
						m_smallSphereDistDamp.Update( (finalDistance > 0.5f + NEAR_PLANE_DIST ? 0.5f + NEAR_PLANE_DIST : finalDistance), timeDelta );
					}
					else
					{
						m_smallSphereDistDamp.Force( finalDistance > 0.5f + NEAR_PLANE_DIST ? 0.5f + NEAR_PLANE_DIST : finalDistance );
					}
				}
				else
				{
					m_smallSphereDistDamp.Force( finalDistance );
				}
#ifndef HANDS_ON_DEMO_HACK
				data.m_camera.Get()->SetNearPlane( idealNearPlane );
#endif
			}
			else
			{
				if( m_usingSmallSphere )
				{
					m_distanceDmpFactor = INCREASE_DIST_FACTOR;
					m_smallSphereDistDamp.Update( 0.5f + NEAR_PLANE_DIST, timeDelta );
				}
				else
				{
					m_smallSphereDistDamp.Force( 0.5f + NEAR_PLANE_DIST );
				}

				data.m_camera.Get()->SetNearPlane( idealNearPlane );

#ifdef HANDS_ON_DEMO_HACK
				m_finalPosition = lookAtPosition + (direction * m_smallSphereDistDamp.GetValue());
			}

			// Hack - we no longer move the nearPlane and m_finalPosition when very close to Geralt with camera
			// so we check if we overlap and occlude if needed.
			const Float realNearPlane = GGame->GetCachedFrameInfo().m_worldRenderSettings.m_cameraNearPlane + 0.1f;
			if ( CheckPlayerOverlap( m_finalPosition, idealNearPlane + 0.1f) || CheckPlayerOverlap( m_finalPosition, realNearPlane ))
			{
				for ( EntityWithItemsComponentIterator< CMeshTypeComponent > it( GCommonGame->GetPlayer() ); it; ++it )
				{
					CMeshTypeComponent* c = *it;

					if ( c->GetRenderProxy() )
					{
						( new CRenderCommand_SetTemporaryFade( c->GetRenderProxy() ) )->Commit();
					}
				}
			}

#else
			}

			m_finalPosition = lookAtPosition + direction * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);
#endif // HANDS_ON_DEMO_HACK

			m_usingSmallSphere = true;
		}
		else
		{
			const Float realNearPlane = GGame->GetCachedFrameInfo().m_worldRenderSettings.m_cameraNearPlane + 0.1f;
			if ( CheckPlayerOverlap( m_finalPosition, realNearPlane ) )
			{
				const Float dist2D = (m_finalPosition - lookAtPosition).Mag2();

				// Don't ask...
				data.m_camera.Get()->SetNearPlane( Max( dist2D - m_sweepRadius - 0.1f, 0.02f ) );
			}

			if( m_usingSmallSphere )
			{
				Float temp = m_smallSphereDistDamp.GetValue() - NEAR_PLANE_DIST;
				Float temp2 = m_smallSphereDistDamp.GetVelocity();
				m_distanceDamp.Update( temp, temp2, temp, 0.f );
			}

			m_usingSmallSphere = false;
		}
	}
	else
	{
		const Float distToPlayer = Max( m_distanceDamp.GetValue() + NEAR_PLANE_DIST - m_sweepRadius, 0.02f );
		if( distToPlayer < NEAR_PLANE_DIST )
		{
			data.m_camera.Get()->SetNearPlane( distToPlayer );
		}
		else
		{
			data.m_camera.Get()->SetNearPlane( 0.f );
		}
	}
	
	const Vector dir = m_finalPosition - sweepStartPosition;
	FadeOutCameraOccludables(m_sweepRadius * SPhysicsSettings::m_cameraOccludablesRadiusRatio, sweepStartPosition + (dir * SPhysicsSettings::m_cameraOccludablesStartRatio), m_finalPosition);

	Float cameraPadding = 0.02f;
	CallFunctionRet( GGame->GetPlayerEntity(), CNAME( GetCameraPadding ), cameraPadding );

	FadeOutPlayer( data, m_finalPosition, lookAtPosition, m_sweepRadius, cameraPadding );
	FadeOutPlayerWeapons( data );
}

void CCustomCameraOutdoorCollisionController::Reset()
{
	m_state	= ECCS_Initializing;
}

void CCustomCameraOutdoorCollisionController::UpdateCollision( const Vector& sweepStartPosition, const Vector& lookAtPosition, const Vector& desiredPosition, Float ratio, Float timeDelta )
{
	m_sweepPosition = Vector::Interpolate( lookAtPosition, desiredPosition, ratio );

	const Float fullDist = (desiredPosition - lookAtPosition).Mag3();

	Vector direction = m_sweepPosition - lookAtPosition;
	const Float newDistance = direction.Normalize3();

	if( newDistance < m_distanceDamp.GetValue() )
	{
		m_distanceDmpFactor = DECREASE_DIST_FACTOR;
		m_distanceDamp.Update( newDistance, timeDelta );

		m_finalPosition = lookAtPosition + direction * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);

		Float distRatio = m_distanceDamp.GetValue() / fullDist;
		Float overlapRadius = distRatio * 0.3f + 0.09f;

		if( CheckSphereOverlap( m_finalPosition, overlapRadius, NULL ) )
		{
			Uint32 counter = 0;
			do
			{
				m_distanceDamp.Update( newDistance, timeDelta );
				m_finalPosition = lookAtPosition + direction * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);

				distRatio = m_distanceDamp.GetValue() / fullDist;
				overlapRadius = distRatio * 0.3f + 0.09f;
			} while( ++counter <= 2 && CheckSphereOverlap( m_finalPosition, overlapRadius, NULL ) );

			// still overlapping? just force it...
			if( counter > 2 && CheckSphereOverlap( m_finalPosition, overlapRadius, NULL ) )
			{
				Float newRatio = 1.f;
				Vector hitPosition;
				Sweep( m_sweepRadius, sweepStartPosition, desiredPosition, newRatio, hitPosition );

				m_sweepPosition = Vector::Interpolate( lookAtPosition, desiredPosition, newRatio );

				Vector newDirection = m_sweepPosition - lookAtPosition;
				const Float newerDistance = newDirection.Normalize3();

				if( newerDistance > m_distanceDamp.GetValue() )
				{
					if( m_isInMotion )
					{
						m_distanceDmpFactor = INCREASE_DIST_FACTOR;
						m_distanceDamp.Update( newerDistance, timeDelta );
					}
				}
				else
				{
					m_distanceDamp.Force( newerDistance );
				}

				m_finalPosition = lookAtPosition + newDirection * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);
			}
		}
	}
	else
	{
		Float newRatio = 1.f;
		Vector hitPosition;
		Sweep( m_sweepRadius, sweepStartPosition, desiredPosition, newRatio, hitPosition );

		m_sweepPosition = Vector::Interpolate( lookAtPosition, desiredPosition, newRatio );

		Vector newDirection = m_sweepPosition - lookAtPosition;
		const Float newerDistance = newDirection.Normalize3();

		if( newerDistance > m_distanceDamp.GetValue() )
		{
			if( m_isInMotion )
			{
				m_distanceDmpFactor = INCREASE_DIST_FACTOR;
				m_distanceDamp.Update( newerDistance, timeDelta );
			}
		}
		else
		{
			m_distanceDamp.Force( newerDistance );
		}

		m_finalPosition = lookAtPosition + newDirection * (m_distanceDamp.GetValue() + NEAR_PLANE_DIST);
	}
}

Bool CCustomCameraOutdoorCollisionController::VerifyPredictedPositionAndUpdateIfNeeded( const Vector& sweepStartPosition, const Vector& lookAtPosition, const Vector& desiredPosition )
{
	Float ratio;
	Vector hitPosition;

	const Float usedDistance = m_usingSmallSphere ? m_smallSphereDistDamp.GetValue() - NEAR_PLANE_DIST : m_distanceDamp.GetValue() ;

	const Vector sweepEndPosition = sweepStartPosition + (desiredPosition - sweepStartPosition).Normalized3() * usedDistance;
	if( Sweep( m_sweepRadius, sweepStartPosition, sweepEndPosition, ratio, hitPosition ) )
	{
		const Vector tempPosition = lookAtPosition + (desiredPosition - lookAtPosition).Normalize3() * usedDistance;
		m_sweepPosition = Vector::Interpolate( lookAtPosition, tempPosition, ratio );

		Vector newDirection = m_sweepPosition - lookAtPosition;
		const Float newDistance = newDirection.Normalize3();

		if( newDistance < usedDistance )
		{
			m_distanceDamp.Force( newDistance );

			m_finalPosition = lookAtPosition + newDirection * (newDistance + NEAR_PLANE_DIST);

			return false;
		}
	}

	return true;
}

void CCustomCameraOutdoorCollisionController::GenerateDebugFragments( CRenderFrame* frame )
{
	frame->AddDebugSphere( m_sweepPosition, m_sweepRadius, Matrix::IDENTITY, Color::RED );
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////



IMPLEMENT_ENGINE_CLASS( CCustomCameraAutoAvoidanceCollisionController );

CCustomCameraAutoAvoidanceCollisionController::CCustomCameraAutoAvoidanceCollisionController()
	: m_angleDmpFactor( 0.7f )
	, m_angleDamp( m_angleDmpFactor )
	, m_angleStep( 20.f )
	, m_sweapRadius( 0.35f )
	, m_smallOverlapRadius( 0.4f )
	, m_bigOverlapRadius( 0.7f )
	, m_collisionOffset( 0.1f )
	, m_calculatedRot( NULL )
	, m_calculatedRight( false )
	, m_criticalCollisionSquaredDistance( 9.f )
	, m_state( ECCS_Initializing )
	, m_catchUpCurve( NULL )
	, m_timer( 0.f )
{
}

CCustomCameraAutoAvoidanceCollisionController::~CCustomCameraAutoAvoidanceCollisionController()
{
}

void CCustomCameraAutoAvoidanceCollisionController::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( !m_catchUpCurve )
	{
		m_catchUpCurve = CreateObject< CCurve >( this );
		m_catchUpCurve->GetCurveData().AddPoint( 0.f, 0.f );
		m_catchUpCurve->GetCurveData().AddPoint( 1.f, 1.f );
	}
}

void CCustomCameraAutoAvoidanceCollisionController::Activate( const SCameraMovementData& data )
{
	m_state = ECCS_Initializing;
}

Float m_acceleration = 500.f;
EulerAngles m_finalVelocity = EulerAngles::ZEROS;
#define PENETRATION_EPSILON 0.05f

void CCustomCameraAutoAvoidanceCollisionController::Force( SCameraMovementData& data )
{
	m_finalRotation = data.m_pivotRotationValue;
	m_finalVelocity = data.m_pivotRotationVelocity;
}

void CCustomCameraAutoAvoidanceCollisionController::Update( SCameraMovementData& data, Float timeDelta )
{
	//data.m_pivotDistanceValue = 4.25f;
	const Vector playerPosition = (GGame->GetPlayerEntity() ? GGame->GetPlayerEntity()->GetWorldPositionRef() : Vector::ZERO_3D_POINT) + m_originOffset;
	const Vector desiredPosition = data.m_pivotPositionValue + data.m_pivotRotationValue.ToMatrix().GetAxisY() * -data.m_pivotDistanceValue;

	Float leftPenetration = 0.f;
	Float rightPenetration = 0.f;
	Float bottomPenetration = 0.f;
	if( CollectCollisionInfo( playerPosition, desiredPosition, leftPenetration, rightPenetration, bottomPenetration ) )
	{
		//if( MAbs( penetration ) > 0.3f )
		{
			/*Float softPenetration = 0.f;
			if( CollectOneSidedSoftSphereInfo( playerPosition, desiredPosition, penetration < 0.f, softPenetration ) )
			{

			}*/

			// Horizontal correction
			Float multiplier = 1.f;
			if( leftPenetration > 0.f && rightPenetration > 0.f && MAbs(leftPenetration - rightPenetration) < PENETRATION_EPSILON )
			{
				m_finalVelocity.Yaw = 0.f;
				multiplier = 0.f;
			}
			else
			{
				if( ( rightPenetration > 0.5f && m_finalVelocity.Yaw > 0.f ) || ( leftPenetration > 0.5f && m_finalVelocity.Yaw < 0.f ) )
				{
					m_finalVelocity.Yaw = 0.f;
				}

				multiplier = leftPenetration > rightPenetration ? -leftPenetration : rightPenetration;
			}

			m_finalVelocity.Yaw -= m_acceleration * timeDelta * multiplier;
			m_finalRotation.Yaw += m_finalVelocity.Yaw * timeDelta;

			// Vertical correction
			multiplier = bottomPenetration;
			m_finalVelocity.Pitch -= m_acceleration * timeDelta * multiplier;
			m_finalRotation.Pitch += m_finalVelocity.Pitch * timeDelta;

			data.m_pivotRotationVelocity = m_finalVelocity;
			data.m_pivotRotationValue = m_finalRotation;
		}
		/*else if( data.m_pivotRotationController.Get()->GetRotationDelta().Yaw * penetration < 0.f )
		{
			m_finalRotation = data.m_pivotRotationValue;
			m_finalVelocity = data.m_pivotRotationVelocity;
		}*/
		/*else
		{
			const Float deceleration = m_acceleration * timeDelta * 0.5f;
			m_finalVelocity.Pitch = m_finalVelocity.Pitch > 0.f ? Max( m_finalVelocity.Pitch - deceleration, 0.f ) : Min( m_finalVelocity.Pitch + deceleration, 0.f );
			m_finalVelocity.Yaw = m_finalVelocity.Yaw > 0.f ? Max( m_finalVelocity.Yaw - deceleration, 0.f ) : Min( m_finalVelocity.Yaw + deceleration, 0.f );
			m_finalRotation += m_finalVelocity * timeDelta;
			data.m_pivotRotationValue = m_finalRotation;
			data.m_pivotRotationVelocity = m_finalVelocity;
		}*/

		m_finalPosition = data.m_pivotPositionValue + m_finalRotation.ToMatrix().GetAxisY() * -data.m_pivotDistanceValue;
	}
	else
	{
		m_finalPosition = desiredPosition;
		m_finalRotation = data.m_pivotRotationValue;
		m_finalVelocity = data.m_pivotRotationVelocity;
		m_angleDamp.Force( m_finalRotation );
		/*
		Float mult = 1.f;
		if( CollectOneSidedSoftSphereInfo( playerPosition, desiredPosition, m_finalVelocity.Yaw < 0.f, leftPenetration ) )
		{
			const Float factor = 1.f - leftPenetration;
			mult = 1.f / ( factor * factor * factor );
		}

		Float deceleration = MAbs(m_finalVelocity.Yaw) * timeDelta * 4.f * mult;
		m_finalVelocity.Yaw = m_finalVelocity.Yaw > 0.f ? Max( m_finalVelocity.Yaw - deceleration, 0.f ) : Min( m_finalVelocity.Yaw + deceleration, 0.f );

		deceleration = m_finalVelocity.Pitch * timeDelta * 4.f;
		m_finalVelocity.Pitch = m_finalVelocity.Pitch > 0.f ? Max( m_finalVelocity.Pitch - deceleration, 0.f ) : Min( m_finalVelocity.Pitch + deceleration, 0.f );
		m_finalRotation += m_finalVelocity * timeDelta;
		data.m_pivotRotationValue = m_finalRotation;
		data.m_pivotRotationVelocity = m_finalVelocity;*/

		m_finalPosition = data.m_pivotPositionValue + m_finalRotation.ToMatrix().GetAxisY() * -data.m_pivotDistanceValue;
	}
}

#define RADIUS_GROWTH 0.12f

Bool CCustomCameraAutoAvoidanceCollisionController::CollectCollisionInfo( const Vector& start, const Vector& end, Float& outLeftPenetration, Float& outRightPenetration, Float& outBottomPenetration )
{
	Vector direction = end - start;
	const Float distance = direction.Mag3();
	direction.Div3( distance );

	const Vector cross = Vector::Cross( direction, Vector::EZ );

	TDynArray<TEST_SPHERE> spheres;

	if( distance < 1.4f )
	{
		const Vector offset = cross * m_smallOverlapRadius;
		const Vector position = start + direction;
		spheres.PushBack( TEST_SPHERE( position + offset, 0.f, m_smallOverlapRadius ) );
		spheres.PushBack( TEST_SPHERE( position - offset, 0.f, m_smallOverlapRadius ) );
	}
	else
	{
		//front spheres
		{
			const Vector offset = cross * m_smallOverlapRadius;
			const Vector position = start + direction;
			spheres.PushBack( TEST_SPHERE( position + offset, 0.f, m_smallOverlapRadius ) );
			spheres.PushBack( TEST_SPHERE( position - offset, 0.f, m_smallOverlapRadius ) );
		}

		//rear spheres
		{
			const Float radius = (distance - 1.4f) * RADIUS_GROWTH + m_smallOverlapRadius;
			const Vector offset = cross * radius;
			const Vector position = end;
			spheres.PushBack( TEST_SPHERE( position + offset, 0.f, radius ) );
			spheres.PushBack( TEST_SPHERE( position - offset, 0.f, radius ) );
		}

		//fill the remaining field with spheres
		const Float rearRadius = spheres.Back().m_radius;
		const Float remaining = distance - rearRadius - ( 1.f + m_smallOverlapRadius ); //distance between front and rear sphere edges

		//check if the spheres don't overlap already
		if( remaining > 0.f )
		{
			const Float count = MCeil( remaining / ( m_smallOverlapRadius + rearRadius ) );
			const Float distDelta = remaining / count;
			for( Float i = 0.f; i < count; ++i )
			{
				const Float finalDist = distDelta * 0.5f + distDelta * i;
				const Float radius = finalDist * RADIUS_GROWTH + m_smallOverlapRadius;
				const Vector offset = cross * radius;
				const Vector position = start + direction * ( 1.f + m_smallOverlapRadius + finalDist );
				spheres.PushBack( TEST_SPHERE( position + offset, 0.f, radius ) );
				spheres.PushBack( TEST_SPHERE( position - offset, 0.f, radius ) );
			}
		}
	}

	Float maxLeftPen = 0.f;
	Float maxRightPen = 0.f;
	Float maxBottomPen = 0.f;

	m_dbgSpheres.ClearFast();

	const Uint32 sphearsCount = spheres.Size();
	for( Uint32 i = 0; i < sphearsCount; i += 2 )
	{
		Float penetration = 0.f;
		Vector normal;
		if( CheckSpherePenetration( spheres[i].m_position, normal, penetration ) )
		{
			if( normal.Dot3( -Vector::EZ ) > 0.5f )
			{
				if( penetration < maxBottomPen ) maxBottomPen = penetration;
			}

			if( normal.Dot3( cross ) > 0.5f )
			{
				if( penetration < maxLeftPen ) maxLeftPen = penetration;
			}
		}

		spheres[i].m_penetration = penetration;
		m_dbgSpheres.PushBack( spheres[i] );

		if( CheckSpherePenetration( spheres[i + 1].m_position, normal, penetration ) )
		{
			if( normal.Dot3( -Vector::EZ ) > 0.5f )
			{
				if( penetration < maxBottomPen ) maxBottomPen = penetration;
			}

			if( normal.Dot3( cross ) < -0.5f )
			{
				if( penetration < maxRightPen ) maxRightPen = penetration;
			}
		}

		spheres[i + 1].m_penetration = penetration;
		m_dbgSpheres.PushBack( spheres[i + 1] );
	}

	outLeftPenetration = -maxLeftPen;
	outRightPenetration = -maxRightPen;
	outBottomPenetration = -maxBottomPen;

	return maxRightPen || maxLeftPen;
}

Bool CCustomCameraAutoAvoidanceCollisionController::CollectOneSidedSoftSphereInfo( const Vector& start, const Vector& end, Bool leftSide, Float& outPenetration )
{
	Vector direction = end - start;
	const Float distance = direction.Mag3();
	direction.Div3( distance );

	const Vector cross = Vector::Cross( direction, Vector::EZ );

	TDynArray<TEST_SPHERE> spheres;

	if( distance < 1.4f )
	{
		const Vector offset = cross * m_bigOverlapRadius * (leftSide ? 1.f : -1.f);
		const Vector position = start + direction * (1.f + m_bigOverlapRadius - m_smallOverlapRadius);
		spheres.PushBack( TEST_SPHERE( position + offset, 0.f, m_bigOverlapRadius ) );
	}
	else
	{
		//front sphere
		{
			const Vector offset = cross * m_bigOverlapRadius * (leftSide ? 1.f : -1.f);
			const Vector position = start + direction * (1.f + m_bigOverlapRadius - m_smallOverlapRadius);
			spheres.PushBack( TEST_SPHERE( position + offset, 0.f, m_bigOverlapRadius ) );
		}

		//rear sphere
		{
			const Float radius = (distance - 1.4f) * RADIUS_GROWTH + m_bigOverlapRadius;
			const Vector offset = cross * radius * (leftSide ? 1.f : -1.f);
			const Vector position = end;
			spheres.PushBack( TEST_SPHERE( position + offset, 0.f, radius ) );
		}

		//fill the remaining field with spheres
		const Float rearRadius = spheres.Back().m_radius;
		const Float remaining = distance - rearRadius - ( 1.f + m_bigOverlapRadius ); //distance between front and rear sphere edges

		//check if the spheres don't overlap already
		if( remaining > 0.f )
		{
			const Float count = MCeil( remaining / ( m_bigOverlapRadius + rearRadius ) );
			const Float distDelta = remaining / count;
			for( Float i = 0.f; i < count; ++i )
			{
				const Float finalDist = distDelta * 0.5f + distDelta * i;
				const Float radius = finalDist * RADIUS_GROWTH + m_bigOverlapRadius;
				const Vector offset = cross * radius * (leftSide ? 1.f : -1.f);
				const Vector position = start + direction * ( 1.f + m_bigOverlapRadius + finalDist );
				spheres.PushBack( TEST_SPHERE( position + offset, 0.f, radius ) );
			}
		}
	}

	Float maxPen = 0.f;
	//m_dbgSpheres.ClearFast();

	const Uint32 sphearsCount = spheres.Size();
	for( Uint32 i = 0; i < sphearsCount; ++i )
	{
		Float penetration = 0.f;
		Vector temp;
		CheckSpherePenetration( spheres[i].m_position, temp, penetration );
		if( penetration > maxPen ) maxPen = penetration;

		spheres[i].m_penetration = penetration;
		//m_dbgSpheres.PushBack( spheres[i] );
	}

	outPenetration = maxPen;

	return maxPen > 0;
}

Bool CCustomCameraAutoAvoidanceCollisionController::CheckSpherePenetration( const Vector& position, Vector& normal, Float& penetration )
{
	TDynArray< CollosionPoint > points;

	if( CheckSphereOverlap( position, m_bigOverlapRadius, &points ) )
	{
		Float maxPen = 0.f;

		const TDynArray< CollosionPoint >::const_iterator end = points.End();
		for( TDynArray< CollosionPoint >::const_iterator it = points.Begin(); it != end; ++it )
		{
			if( it->m_penetration < maxPen )
			{
				maxPen = it->m_penetration;
				normal = it->m_point - position;
			}
		}

		penetration = maxPen;
		normal.Normalize3();

		return true;
	}

	return false;
}

void CCustomCameraAutoAvoidanceCollisionController::GenerateDebugFragments( CRenderFrame* frame )
{
	TDynArray<TEST_SPHERE>::const_iterator end = m_dbgSpheres.End();
	for( TDynArray<TEST_SPHERE>::const_iterator it = m_dbgSpheres.Begin(); it != end; ++it )
	{
		frame->AddDebugSphere( it->m_position, it->m_radius, Matrix::IDENTITY, Color( (Uint8)(255.f * it->m_penetration), 0, 0, 255 ) );
	}
}
