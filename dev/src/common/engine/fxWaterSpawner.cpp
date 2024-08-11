/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "fxSpawner.h"
#include "fxWaterSpawner.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "game.h"
#include "../physics/physicsEngine.h"
#include "../physics/physicalCollision.h"
#include "world.h"
#include "entity.h"

IMPLEMENT_ENGINE_CLASS( CFXWaterSpawner );

CFXWaterSpawner::CFXWaterSpawner()
{
}

Uint32 CFXWaterSpawner::AmountOfPC( CEntity* parentEntity, TDynArray<Uint32> &indices )
{ 
	indices.PushBack(0);
	return 1;
}

Bool CFXWaterSpawner::Calculate( CEntity* parentEntity, Vector &position, EulerAngles &rotation, Uint32 pcNumber )
{
	// No entity - no fun
	if( parentEntity == NULL )
	{
		return false;
	}

	rotation = EulerAngles::ZEROS;

	// Make cast to find water surface. If found return true, otherwise false
	return TraceWater( parentEntity->GetWorldPositionRef(), position );
}

Bool CFXWaterSpawner::TraceWater( const Vector& position, Vector& target )
{	
	CWorld* world = GGame->GetActiveWorld();

	if( !world )
	{
		return false;
	}

	CPhysicsWorld* physWorld = nullptr;
	if( !world->GetPhysicsWorld( physWorld ) )
	{
		return false;
	}

	SPhysicsContactInfo outInfo;
	const CPhysicsEngine::CollisionMask includeMask = CPhysicalCollision::COLLIDES_ALL;
	CPhysicsEngine::CollisionMask excludeMask = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( ClothCollider ) ) | /* GPhysicEngine->GetCollisionTypeBit( CNAME( Dangles ) ) |  GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) ) | */ GPhysicEngine->GetCollisionTypeBit( CNAME( Projectile ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Weapon ) );
	//CPhysicsEngine::CollisionMask excludeMaskButWather = GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Debris ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( ClothCollider ) ) | /* GPhysicEngine->GetCollisionTypeBit( CNAME( Dangles ) ) | */ GPhysicEngine->GetCollisionTypeBit( CNAME( Projectile ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Ragdoll ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Weapon ) );

	Vector3 upPos = position;
	upPos.Z += 1.5f;

	Vector3 downPos = position;
	downPos.Z -= 0.5f;

	// we will bump water approximation for better water detection based on cam position.
	const Vector& camPos = world->GetCameraPosition();
	const Float distSqr = camPos.DistanceSquaredTo( position );

	// always use approx = 1 but sometimes (like 20m) bump it for better visuals
	Uint32 waterLod = 1;
	if( distSqr < 400.f ) //20 meters
	{
		// bump water detection
		waterLod = 0;
	}

	const Float waterLevel = world->GetWaterLevel( position, waterLod );
	if( physWorld->RayCastWithSingleResult( upPos, downPos,  includeMask, excludeMask, outInfo ) != TRV_Hit )
	{
		if( upPos.Z > waterLevel && downPos.Z < waterLevel)
		{
			target = position;
			target.Z = waterLevel;
			return true;
		}
	}
	else
	{
		if( upPos.Z > waterLevel && downPos.Z < waterLevel && outInfo.m_position.Z < waterLevel )
		{
			target = position;
			target.Z = waterLevel;
			return true;
		}
		Vector3 hitPos = outInfo.m_position;
		if( physWorld->RayCastWithSingleResult( upPos, downPos,  GPhysicEngine->GetCollisionTypeBit( CNAME( Water ) ) , GPhysicEngine->GetCollisionTypeBit( CNAME( Character ) ), outInfo ) == TRV_Hit )
		{
			if( outInfo.m_position.Z >= hitPos.Z )
			{
				target = outInfo.m_position;
				return true;
			}
		}
	}
	// We didn't hit water
	return false;
}
