/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageCollisionHandler.h"
#include "renderer.h"
#include "../physics/physicsSettings.h"
#include "../physics/physicsWorld.h"
#include "../physics/physicsWrapper.h"
#include "physicsTileWrapper.h"
#include "baseTree.h"
#include "clipMap.h"
#include "terrainTile.h"
#include "physicsDataProviders.h"

CFoliageCollisionHandler::CFoliageCollisionHandler()
	:	m_world( nullptr ),
		m_renderer( nullptr )
{}

CFoliageCollisionHandler::~CFoliageCollisionHandler()
{}

void CFoliageCollisionHandler::AddAllCollision( const CSRTBaseTree* baseTree, const FoliageInstanceContainer& instances, const Box& box )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow AddAllCollision") );

	if( !m_renderer ) return;
	if( !baseTree ) return;
	if( !m_world ) return;
	if( !SPhysicsSettings::m_dontCreateTrees && !instances.Empty() )
	{
		TDynArray< Sphere > collisionShapes;
		RenderObjectHandle renderObject = baseTree->AcquireRenderObject();
		if( !renderObject ) return;
		m_renderer->GetSpeedTreeResourceCollision( renderObject.Get(), collisionShapes );
		if( !collisionShapes.Empty() )
		{
			CPhysicsWorld * physicsWorld = nullptr;
			m_world->GetPhysicsWorld( physicsWorld );
			CClipMap * clipMap = m_world->GetTerrain();

			if( clipMap )
			{	
				if(instances.Size() > 3)
				{
					CPhysicsTileWrapper * possibleTiles[4];
					const Vector & Min = box.Min;
					const Vector & Max = box.Max;
					Vector corners[4] =
					{
						Vector( Min.X, Min.Y, Min.Z ),
						Vector( Max.X, Min.Y, Min.Z ),
						Vector( Min.X, Max.Y, Min.Z ),
						Vector( Max.X, Max.Y, Min.Z )
					};

					for(int i=0; i < 4; i++)
						possibleTiles[i] = GetTerrainTileFromPosition(physicsWorld, corners[i], clipMap);

					for( auto iter = instances.Begin(), end = instances.End(); iter != end; ++iter )
					{

						const SFoliageInstance & instance = *iter;
						const Vector & position = instance.GetPosition();
						CPhysicsTileWrapper * tile = GetTerrainTileFromPositionOptimized(position, possibleTiles);

						if( tile )
						{
							tile->AddFoliageBody( CPhysicsWrapperParentResourceProvider( baseTree ), &instance, collisionShapes );
						}
					}
				}
				else
				{
					for( auto iter = instances.Begin(), end = instances.End(); iter != end; ++iter )
					{
						const SFoliageInstance & instance = *iter;
						const Vector & position = instance.GetPosition();
						CPhysicsTileWrapper * tile = GetTerrainTileFromPosition(physicsWorld, position, clipMap);
						if( tile )
						{
							tile->AddFoliageBody( CPhysicsWrapperParentResourceProvider( baseTree ), &instance, collisionShapes );
						}
					}
				}

				
			}
		}
	}
}

void CFoliageCollisionHandler::AddCollisionsForInstance(const CSRTBaseTree* baseTree, const SFoliageInstance & newInstance)
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("AddCollisionsForInstance") );

	if( !m_renderer ) return;
	if( !baseTree ) return;
	if( !m_world ) return;

	if( !SPhysicsSettings::m_dontCreateTrees  )
	{
		TDynArray< Sphere > collisionShapes;
		RenderObjectHandle renderObject = baseTree->AcquireRenderObject();
		if( !renderObject ) return;
		m_renderer->GetSpeedTreeResourceCollision( renderObject.Get(), collisionShapes );

		if( !collisionShapes.Empty() )
		{
			CPhysicsWorld * physicsWorld = nullptr;
			m_world->GetPhysicsWorld( physicsWorld );
			CClipMap * clipMap = m_world->GetTerrain();

			if( clipMap )
			{
				const Vector & position = newInstance.GetPosition();
				CPhysicsTileWrapper * tile = GetTerrainTileFromPosition(physicsWorld, position, clipMap);
				if( tile )
				{
					tile->AddFoliageBody( CPhysicsWrapperParentResourceProvider( baseTree ), &newInstance, collisionShapes );
				}
			}
		}
	}
}


CPhysicsTileWrapper * CFoliageCollisionHandler::GetTerrainTileFromPosition(CPhysicsWorld * physicsWorld, const Vector & position, CClipMap * clipMap)
{
	CPhysicsTileWrapper * tile = physicsWorld->GetTerrainTileWrapper( position );
	if( !tile )
	{
		if( !physicsWorld->IsPositionInside( position ) ) return nullptr;
		Int32 x = 0;
		Int32 y = 0;
		CTerrainTile* terrainTile = clipMap->GetTileFromPosition( position, x, y );
		if( terrainTile && terrainTile->IsCollisionEnabled() )
		{
			tile = CTerrainTile::CreateCollisionPlaceholder( m_world, clipMap->GetBoxForTile( x, y, 0.0f ), x, y, terrainTile->GetResolution() );
		}
	}

	return tile;
}


CPhysicsTileWrapper * CFoliageCollisionHandler::GetTerrainTileFromPositionOptimized(const Vector & position, CPhysicsTileWrapper * tilesArray[4])
{
	for(int i=0; i < 4; i++)
	{
		if(tilesArray[i] && tilesArray[i]->GetBounds2D().Contains(position) )
		{
			return tilesArray[i];
		}
	}

	return nullptr;
}


void CFoliageCollisionHandler::RemoveAllCollision( const CSRTBaseTree* baseTree, const Box & box )
{
	PC_CHECKER_SCOPE( 0.001f, TXT("FOLIAGE"), TXT("Slow RemoveAllCollision") );

	if( m_world )
	{
		CPhysicsWorld * physicsWorld = nullptr;
		if( m_world->GetPhysicsWorld( physicsWorld ) )
		{
			const Box2 box2D( Vector2( box.Min.X, box.Min.Y ), Vector2( box.Max.X, box.Max.Y ) );
			CPhysicsTileWrapper* tile = physicsWorld->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetWrapperFirst();
			while( tile )
			{
				Box2 bounds = tile->GetBounds2D();
				if( bounds.Intersects( box2D ) )
				{
					tile->DisposeArea( baseTree, box2D );
				}
				tile = physicsWorld->GetWrappersPool< CPhysicsTileWrapper, SWrapperContext >()->GetWrapperNext( tile );
			}		
		}
	}
}

void CFoliageCollisionHandler::SetInternalWorld( CWorld * world )
{
	m_world = world;
}

void CFoliageCollisionHandler::SetInternalRender( IRender * renderer )
{
	m_renderer = renderer;
}



Red::TUniquePtr< CFoliageCollisionHandler > CreateFoliageCollisionHandler( CWorld * world, IRender * renderer )
{
	Red::TUniquePtr< CFoliageCollisionHandler > collisionHandler( new CFoliageCollisionHandler );
	collisionHandler->SetInternalWorld( world );
	collisionHandler->SetInternalRender( renderer );
	return collisionHandler;
}
