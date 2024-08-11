#include "build.h"

#include "bulletProjectile.h"

IMPLEMENT_ENGINE_CLASS( SBulletImpactInfo );
IMPLEMENT_ENGINE_CLASS( CBulletProjectile );

RED_DEFINE_STATIC_NAME( impact_default );

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
#ifndef RED_FINAL_BUILD
Int32 CBulletProjectile::sm_debugBulletIndex = 0;
#endif

//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
void CBulletProjectile::OnTick( Float timeDelta )
{
    /*
	// spawn impacts first
	if ( !m_collisionReports.Empty() )
	{
		// find closest collision
		if( m_collisionReports.Size() > 1 )
		{
			SProjectileCollisionReport closestCollision = m_collisionReports[ 0 ];
			Float bestDistance = FLT_MAX;
			for( Uint32 i=0; i<m_collisionReports.Size(); ++i )
			{
				Float dist = ( m_startPosition - m_collisionReports[ i ].m_position ).SquareMag3();
				if( dist < bestDistance )
				{
					closestCollision = m_collisionReports[ i ];
					bestDistance = dist;
				}
			}

			m_collisionReports.ClearFast();
			m_collisionReports.PushBack( closestCollision );
		}

		for ( Uint32 i = 0; i < m_collisionReports.Size(); ++i )
		{
			CComponent* component = m_collisionReports[ i ].m_component.Get();
			if( !component )
				continue;

			// stop projectile
			StopProjectile();

			// workaround for bug when transforming pure Z vector into euler angles
			Vector newDir = m_collisionReports[ i ].m_normal;
			if( Abs( newDir.X ) < 0.0001f && Abs( newDir.Y ) < 0.0001f )
			{
				newDir.X = 0.001f;
			}

			Teleport( m_collisionReports[ i ].m_position, newDir.ToEulerAngles() );
			ForceUpdateTransform();

			// create impact fx name from the physics material hit
			CName physicsMaterialName = ( m_collisionReports[ i ].m_physicsMaterial ? m_collisionReports[ i ].m_physicsMaterial->m_name : CNAME( default ) );
			String fxNameString = String::Printf( TXT("impact_%s"), physicsMaterialName.AsString().AsChar() );
			CName fxName( fxNameString );

			// check if there's an impact effect to be played
			if( HasEffect( fxName ) )
			{
				PlayEffect( fxName );
			}
			else if( HasEffect( CNAME( impact_default ) ) )
			{
				PlayEffect( CNAME( impact_default ) );
			}

			// notify scripts
			CallEvent( CNAME( OnImpact ), 
					   m_collisionReports[ i ].m_position, 
					   m_collisionReports[ i ].m_normal, 
					   physicsMaterialName );
			
#ifndef RED_FINAL_BUILD
#if 0
			// bullet impact position
			GGame->GetVisualDebug()->AddSphere( CName( String::Printf( TXT( "DebugBulletSphere_%i" ), sm_debugBulletIndex++ ) ), 
												0.05f, 
												GetWorldPosition(), 
												true, 
												Color::RED, 	
												10.0f );

			// bullet impact normal
			GGame->GetVisualDebug()->AddLine( CName( String::Printf( TXT( "DebugBulletLine_%i" ), sm_debugBulletIndex ) ), 
											  GetWorldPosition(), 
											  GetWorldPosition() + GetWorldForward() * 0.3f, 
											  true, 
											  Color::YELLOW, 	
											  10.0f );
#endif
#endif
		}
	}
    */
	TBaseClass::OnTick( timeDelta );
}
/*
void CBulletProjectile::CollisionCheck( const Vector& startPoint, const Vector& endPoint, Float timeDelta )
{
	// The reason this method is overriden is that collision hits from the engine is not sorted in depth
	// which is a problem for very fast projectiles

	CPhysicsWorld* phyWorld = GetLayer()->GetWorld()->GetPhysicsWorld();
	ASSERT( phyWorld );
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask collisionType = GPhysicEngine->GetCollisionTypeBit( CNAME( Projectile ) );																																    
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionGroupMask( collisionType );																																    

	CEntity *pCaster = m_caster.Get();

	// Optimize amount of steps
	const Vector dirVec = endPoint - startPoint; // Distance from start to end points

	Float step = dirVec.Mag3();
	m_deltaStep += step;
	// Divide into steps
	if( m_deltaStep >= PROJECTILE_OVERLAP_ACCURACY )
	{
		step = m_deltaStep;
		m_deltaStep = 0.0f;        
	}
	else
	{
		return;
	}

	SPhysicsContactInfo infos[ MAX_AREA_TEST_RESULTS ];
	Uint32 hits = phyWorld->RayCastWithMultipleResults( m_lastStartStep, endPoint, include, 0, infos, MAX_AREA_TEST_RESULTS );
	m_lastStartStep = endPoint;

	if( hits > 0 )
	{
		// sort hits in distance from origin
		THashMap< CComponent*, SPhysicsContactInfo* > sortedHits;
		for( Uint32 i=0; i<hits; ++i )
		{
			if( !infos[i].m_userDataA )
				continue;

			CComponent* hitComponent = infos[ i ].m_userDataA->GetParent( infos[ i ].m_rigidBodyIndexA.m_actorIndex );

			if( !sortedHits.KeyExist( hitComponent ) )
			{
				// first time this actor was hit
				sortedHits.Insert( hitComponent, &infos[ i ] );
			}
			else
			{
				// get hit already in map
				SPhysicsContactInfo* hitInfo = NULL;
				if( sortedHits.Find( hitComponent, hitInfo ) )
				{
					// compare distance from origin
					if( ( infos[ i ].m_position - m_startPosition ).SquareMag3() <
						( hitInfo->m_position - m_startPosition ).SquareMag3() )
					{
						// ...and set it to the closest hit
						sortedHits.Set( hitComponent, &infos[ i ] );
					}
				}
			}
		}

		// create the collision reports
		THashMap< CComponent*, SPhysicsContactInfo* >::iterator it = sortedHits.Begin();
		for( ; it != sortedHits.End(); ++it )
		{
			SProjectileCollisionReport col;
			col.m_component = (*it).m_first;
			col.m_normal = (*it).m_second->m_normal;
			col.m_position = (*it).m_second->m_position;
			col.m_physicsMaterial = (*it).m_second->m_userDataA->GetMaterial( (*it).m_second->m_rigidBodyIndexA.m_actorIndex );
			m_collisionReports.PushBack( col );
		}
	}
}
*/