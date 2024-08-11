/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/class.h"
#include "../../common/core/rttiSystem.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/apexResource.h"
#include "../../common/engine/apexDestructionResource.h"
#include "../../common/engine/collisionCache.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionCacheOptimizer.h"
#include "../../common/engine/apexClothResource.h"
#include "cookDataBase.h"
#include "cookDataBaseHelper.h"
#include "cacheBuilderCommandlet.h"
#include "baseCacheBuilder.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/physicsDestructionResource.h"

///------

class CPhysicsCacheBuilder : public IResourceBasedCacheBuilder
{
	DECLARE_RTTI_SIMPLE_CLASS( CPhysicsCacheBuilder );

public:
	CPhysicsCacheBuilder();
	~CPhysicsCacheBuilder();

	// interface
	virtual void GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const override;
	virtual Bool Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions ) override;
	virtual Bool ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource ) override;
	virtual Bool Save() override;

	// description
	virtual const Char* GetName() const override{ return TXT("physics"); }
	virtual const Char* GetDescription() const override { return TXT("Compile collision data for physics"); }

private:
	ICollisionCache*		m_physicsCache;
	String					m_physicsCacheTempPath;
	String					m_physicsCachePath;

	THandle< CResource > m_currentWorld;

	// actual per-resource processing
	Bool ProcessMesh( CMesh* meshResource );
	Bool ProcessPhysicsDestruction( CPhysicsDestructionResource* physDestResource );
	Bool ProcessTerrain( CTerrainTile* terrainResource );

	void LoadWorld(String worldPath);

	Bool ProcessApex( CApexResource* apexResource );
};

BEGIN_CLASS_RTTI( CPhysicsCacheBuilder )
	PARENT_CLASS( IResourceBasedCacheBuilder );
END_CLASS_RTTI()

///------

IMPLEMENT_ENGINE_CLASS( CPhysicsCacheBuilder );

CPhysicsCacheBuilder::CPhysicsCacheBuilder()
	: m_physicsCache( NULL ), m_currentWorld( nullptr )
{
}

CPhysicsCacheBuilder::~CPhysicsCacheBuilder()
{
	if ( m_physicsCache )
	{
		m_physicsCache->Flush();
		delete m_physicsCache;

		m_physicsCache = NULL;
	}

	m_currentWorld = nullptr;
}

void CPhysicsCacheBuilder::GetExtensions( TDynArray< String >& outExtensionList, const ECookingPlatform platform ) const
{
	outExtensionList.PushBack( ResourceExtension< CMesh >() );
	outExtensionList.PushBack( ResourceExtension< CApexClothResource >() );
	outExtensionList.PushBack( ResourceExtension< CApexDestructionResource >() );
	outExtensionList.PushBack( ResourceExtension< CPhysicsDestructionResource >() );
	outExtensionList.PushBack( TXT( "w2ter" ) );
}

Bool CPhysicsCacheBuilder::Initialize( const String& outputFilePath, const ECookingPlatform platform, const ICommandlet::CommandletOptions& additonalOptions )
{
	// initialize base builder
	if ( !IResourceBasedCacheBuilder::Initialize( outputFilePath, platform, additonalOptions ) )
		return false;

	// create/open existing shader cache
	m_physicsCache = ICollisionCache::CreateReadWriteCache( outputFilePath + TXT(".tmp") );
	if ( !m_physicsCache )
	{
		ERR_WCC( TXT("Unable to create collision cache at file '%ls'. Make sure file is avaiable for writing."), outputFilePath.AsChar() );
		return false;
	}

	// writer ready
	m_physicsCacheTempPath = outputFilePath + TXT(".tmp");
	m_physicsCachePath = outputFilePath;
	return true;
}

Bool CPhysicsCacheBuilder::ProcessResource( const CCookerDataBase& db, const CCookerDataBase::TCookerDataBaseID& dbId, CResource* resource )
{
	// do not store invalid collision in the cache (remember: we may be running this cook incrementally)
	m_physicsCache->InvalidateCollision( resource->GetDepotPath() );

	// compile the material graph
	Bool result = false;
	if ( resource->IsA< CPhysicsDestructionResource >() )
	{
		CPhysicsDestructionResource* physDest = static_cast< CPhysicsDestructionResource* >( resource );
		return ProcessPhysicsDestruction( physDest );
	}
	else if ( resource->IsA< CMesh >() )
	{
		CMesh* mesh = static_cast< CMesh* >( resource );
		return ProcessMesh( mesh );
	}
	else if ( resource->IsA< CTerrainTile >() )
	{
		CTerrainTile* terrain = static_cast< CTerrainTile* >( resource );
		return ProcessTerrain( terrain );
	}
	else if ( resource->IsA< CApexResource >() )
	{
		CApexResource* apex = static_cast< CApexResource* >( resource );
		return ProcessApex( apex );
	}	
	else
	{
		ERR_WCC( TXT("Unsupported object class for physics cooking: '%ls' for resource: '%s'"), 
			resource->GetClass()->GetName().AsChar(), resource->GetDepotPath() );

		return false;
	}
}

Bool CPhysicsCacheBuilder::Save()
{
	// flush the cache
	if ( m_physicsCache )
	{
		m_physicsCache->Flush();
		delete m_physicsCache;

		m_physicsCache = NULL;
	}

	// optimize the cache
	CCollisionCacheOptimizer optimier;
	const Bool ret = optimier.OptimizeFile( m_physicsCacheTempPath, m_physicsCachePath );

	// delete temporary case file
	GFileManager->DeleteFile( m_physicsCacheTempPath.AsChar() );
	return ret;
}

Bool CPhysicsCacheBuilder::ProcessMesh( CMesh* meshResource )
{
	// get the mesh collision data
	const CCollisionMesh* collisionMesh = meshResource->GetCollisionMesh();
	if ( !collisionMesh )
	{
		WARN_WCC( TXT("Mesh '%ls' does not contain collision"), 
			meshResource->GetDepotPath().AsChar() );

		return true;
	}

	// compile the collision
	CompiledCollisionPtr compiledMesh;
	m_physicsCache->Compile( compiledMesh, collisionMesh, meshResource->GetDepotPath(), meshResource->GetFileTime() );
	return ( compiledMesh.Get() != nullptr );
}

Bool CPhysicsCacheBuilder::ProcessPhysicsDestruction( CPhysicsDestructionResource* physDestResource )
{
	// get the mesh collision data
	const CCollisionMesh* collisionMesh = physDestResource->GetCollisionMesh();
	if ( !collisionMesh )
	{
		WARN_WCC( TXT("CPhysicsDestructionResource '%ls' does not contain collision"), 
			physDestResource->GetDepotPath().AsChar() );

		return true;
	}

	// compile the collision
	CompiledCollisionPtr compiledMesh;
	m_physicsCache->Compile( compiledMesh, collisionMesh, physDestResource->GetDepotPath(), physDestResource->GetFileTime() );
	return ( compiledMesh.Get() != nullptr );
}

Bool CheckForWorldReload(String path, CClipMap* clipMap)
{
	Int32 col = -1;
	Int32 row = -1;
	String name =  path;
	size_t pos = 0;
	name.FindSubstring( TXT( "tile_" ), pos );
	String temp = name.RightString( name.Size() - pos - 6 );
	temp.FindSubstring( TXT( "_x_" ), pos );

	FromString( temp.LeftString( pos ), col );

	temp = temp.RightString( temp.Size() - pos - 4 );

	temp.FindSubstring( TXT( "_res" ), pos );

	FromString( temp.LeftString( pos ), row );

	CTerrainTile* tile = clipMap->GetTile( row, col );
	if( !tile )
	{
		return true;
	}

	CTerrainTile* nextRowTile = NULL;
	if ( col < (Int32)clipMap->GetNumTilesPerEdge() - 1 )
	{
		nextRowTile = clipMap->GetTile( row, col + 1);
		if( !nextRowTile )
		{
			return true;
		}
	}

	CTerrainTile* nextColTile = NULL;
	if ( row < (Int32)clipMap->GetNumTilesPerEdge() - 1 )
	{
		nextColTile = clipMap->GetTile( row + 1, col );
		if( !nextColTile ) 
		{
			return true;
		}
	}

	CTerrainTile* nextRowColTile = NULL;
	if ( row < (Int32)clipMap->GetNumTilesPerEdge() - 1 && col < (Int32)clipMap->GetNumTilesPerEdge() - 1 )
	{
		nextRowColTile = clipMap->GetTile( row + 1, col + 1);
		if( !nextRowColTile )
		{
			return true;
		}
	}

	return false;
}

Bool CPhysicsCacheBuilder::ProcessTerrain( CTerrainTile* terrainResource )
{
	String path = terrainResource->GetFile()->GetDepotPath();

	size_t pos = 0;
	if( !path.FindSubstring( TXT( "dlc\\" ), pos ) || pos != 0 )
	{
		if( !path.FindSubstring( TXT( "engine\\reference_level\\" ), pos ) || pos != 0 )//hack for terrain when world isn't located in production directory "levels"
		{
			if( !path.FindSubstring( TXT( "levels" ), pos ) || pos > 0 )
			{
				return false;
			}
		}
	}


	path.FindSubstring( TXT( "terrain_tiles" ), pos );
	String worldPath = path.LeftString( pos );

	String worldDepotPath;
	if( m_currentWorld.Get() && m_currentWorld.Get()->GetFile() )
		worldDepotPath = m_currentWorld.Get()->GetFile()->GetDepotPath();

	if( !worldDepotPath.FindSubstring( worldPath, pos ) )
	{
		m_currentWorld = nullptr;
		SGarbageCollector::GetInstance().CollectNow();
	}

	terrainResource = Cast< CTerrainTile >( GDepot->LoadResource( path ).Get() );

	// If this tile has collision turned off, we have nothing to do.
	if ( !terrainResource->IsCollisionEnabled() )
	{
		return true;
	}

	if( !m_currentWorld.Get() )
	{
		LoadWorld(worldPath);
	}

	if( !m_currentWorld.Get() )
	{
		return false;
	}

	CClipMap* clipMap = Cast< CWorld >( m_currentWorld.Get() )->GetTerrain();

	if( !clipMap ) return false;

	if(CheckForWorldReload(path, clipMap))
	{
		m_currentWorld = nullptr;
		SGarbageCollector::GetInstance().CollectNow();

		LoadWorld(worldPath);

		if( !m_currentWorld.Get() )
		{
			return false;
		}
		clipMap = Cast< CWorld >( m_currentWorld.Get() )->GetTerrain();

		if( !clipMap )
		{
			return false;
		}
	}



	static Uint16 counter = 0;
	CompiledCollisionPtr compiledMesh;
	++counter;
	m_physicsCache->Compile( compiledMesh, terrainResource, terrainResource->GetDepotPath(), terrainResource->GetFileTime(), clipMap );

	if( counter > 100 )
	{
		counter = 0;
		m_physicsCache->Flush();
	}

	return ( compiledMesh.Get() != nullptr );
}

Bool CPhysicsCacheBuilder::ProcessApex( CApexResource* apexResource )
{	
	// compile the apex resource data
	CompiledCollisionPtr compiledMesh;
	m_physicsCache->Compile( compiledMesh, apexResource, apexResource->GetDepotPath(), apexResource->GetFileTime() );
	return ( compiledMesh.Get() != nullptr );

}

void CPhysicsCacheBuilder::LoadWorld(String worldPath)
{
	static TDynArray< String > worldPaths;
	size_t pos = 0;
	if( worldPaths.Empty() )
	{
		GDepot->FindResourcesByExtension( TXT( "w2w"), worldPaths );
	}

	for( auto i = worldPaths.Begin(); i != worldPaths.End(); ++i )
	{
		if( i->FindSubstring( worldPath, pos ) )
		{
			m_currentWorld = GDepot->LoadResource( *i );
			break;
		}
	}
}

