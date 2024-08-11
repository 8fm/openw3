#include "build.h"
#include "pathlibWorld.h"

#include "../core/versionControl.h"
#include "../core/directory.h"
#include "../core/configVar.h"
#include "../core/configVarLegacyWrapper.h"

#include "baseEngine.h"
#include "clipMap.h"
#include "deniedAreaComponent.h"
#include "game.h"
#include "gameResource.h"
#include "pathlibAgent.h"
#include "pathlibAreaDescription.h"
#include "pathlibBundler.h"
#include "pathlibComponent.h"
#include "pathlibConfiguration.h"
#include "pathlibConst.h"
#include "pathlibGenerationManager.h"
#include "pathlibNavgraph.h"
#include "pathlibNavmesh.h"
#include "pathlibNavmeshComponent.h"
#include "pathlibNavmeshArea.h"
#include "pathlibNodeFinder.h"
#include "pathlibObstacle.h"
#include "pathlibObstacleMapper.h"
#include "pathlibObstaclesMap.h"
#include "pathlibSearchEngine.h"
#include "pathlibSpatialQuery.h"
#include "pathlibStreamingManager.h"
#include "pathlibTaskManager.h"
#include "pathlibTerrain.h"
#include "pathlibHLGraph.h"
#include "pathlibVisualizer.h"
#include "terrainTile.h"
#include "renderFrame.h"
#include "viewport.h"
#include "worldIterators.h"

RED_DISABLE_WARNING_MSC( 4355 ) // STFU compiler! 'this' : used in base member initializer list

IMPLEMENT_ENGINE_CLASS( CPathLibWorld );

namespace
{
	static const Float SQRT2DIV2 = 0.7071067811865475f;
	static const Vector2 DIRS[8] =
	{
		Vector2( 1.0f, 0.f ),
		Vector2( 0.f, 1.f ),
		Vector2( -1.f, 0.f ),
		Vector2( 0.f, -1.f ),
		Vector2( SQRT2DIV2, SQRT2DIV2 ),
		Vector2( SQRT2DIV2, -SQRT2DIV2 ),
		Vector2( -SQRT2DIV2, SQRT2DIV2 ),
		Vector2( -SQRT2DIV2, -SQRT2DIV2 ),
	};

	static const Vector2 ROT[8] =
	{
		// SIN COS
		Vector2( 0.f, 1.f ),
		Vector2( SQRT2DIV2, SQRT2DIV2 ),
		Vector2( -SQRT2DIV2, SQRT2DIV2 ),
		Vector2( 0.f, -1.f ),
		Vector2( 1.f, 0.f ),
		Vector2( SQRT2DIV2, -SQRT2DIV2 ),
		Vector2( -SQRT2DIV2, -SQRT2DIV2 ),
		Vector2( 0.f, -1.f ),
	};

};

////////////////////////////////////////////////////////////////////////////
// CPathLibWorld
////////////////////////////////////////////////////////////////////////////
CPathLibWorld::CPathLibWorld()
	: m_terrainInfo( this )
	, m_instanceMap( *this )
	, m_cookingContext( nullptr )
	, m_cookedDir( nullptr )
	, m_localDir( nullptr )
	, m_sourceDir( nullptr )
	, m_visualizer( nullptr )
	, m_isGameRunning( false )
	, m_useLocalFolder( false )
	, m_isCookerMode( false )
	, m_isProcessingObstacles( false )
{
	m_mapper = new PathLib::CObstaclesMapper( *this );
	m_generationManager = new PathLib::CGenerationManager( *this );
	m_taskManager = new PathLib::CTaskManager( *this );
	m_streamingManager = new PathLib::CStreamingManager( *this );

	ReadConfiguration();
}
CPathLibWorld::~CPathLibWorld()
{
	delete m_mapper;
	delete m_generationManager;
	delete m_taskManager;
	delete m_streamingManager;

	ClearAreas();
}

void CPathLibWorld::SetReferencePosition( const Vector& position )
{
	m_streamingManager->SetReferencePosition( position );
}

Bool CPathLibWorld::HasJobOrTasks() const
{
	return m_streamingManager->HasJob() || m_taskManager->HasTasks();
}

void CPathLibWorld::Tick()
{
	m_streamingManager->Tick();
	m_taskManager->Update();

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	if ( !m_isGameRunning )
	{
		m_generationManager->Tick();
	}
	else
	{
		m_generationManager->GameTick();
	}
#endif
}

Bool CPathLibWorld::MoveAwayFromWall( PathLib::AreaId areaId, const Vector3& pos, Float personalSpace, Vector3& outPos, PathLib::CollisionFlags flags ) const
{
	Float minZ = pos.Z - 1.f;
	Float maxZ = pos.Z + 1.f;

	Vector3 testPos = pos;

	if ( !ComputeHeight( pos.AsVector2(), minZ, maxZ, testPos.Z, areaId ) )
	{
		return false;
	}

	PathLib::CAreaDescription* area = GetAreaDescription( areaId );

	PathLib::CCircleQueryData initialQuery( flags, pos, personalSpace );
	if ( area->VSpatialQuery( initialQuery ) )
	{
		outPos = testPos;
		return true;
	}

	if ( area->VTestLocation( pos, flags ) )
	{
		// algorithm "move away from wall"
		// up to 5 algorithm iterations
		for ( Uint32 i = 0; i < 5; ++i )
		{
			PathLib::CClosestObstacleCircleQueryData query( flags, testPos, personalSpace );
			area->VSpatialQuery( query );
			if ( !query.HasHit() )
			{
				break;
			}
			if ( query.m_closestDistSq == 0.f )
			{
				break;
			}
			Float obstacleDist = sqrt( query.m_closestDistSq );
			Vector2 diff = ( testPos.AsVector2() - query.m_pointOut.AsVector2() ).Normalized();
			diff *= personalSpace - obstacleDist + 0.05f;			// set length
			testPos.AsVector2() += diff;
			if ( !ComputeHeight( testPos.AsVector2(), minZ, maxZ, testPos.Z, areaId ) )
			{
				break;
			}

			PathLib::CAreaDescription* spotArea = areaId != area->GetId() ? GetAreaDescription( areaId ) : area;

			PathLib::CCircleQueryData safePosQuery( flags, testPos, personalSpace );
			if ( spotArea->VSpatialQuery( safePosQuery ) )
			{
				outPos = testPos;
				return true;
			}
		}
	}
	return false;
}

Bool CPathLibWorld::FindSafeSpot( PathLib::AreaId areaId, const Vector3& pos, Float radius, Float personalSpace, Vector3& outPos, Float* minZptr, Float* maxZptr, PathLib::CollisionFlags flags, const Vector2* optionalPreferedDir ) const
{
	Float minZ = minZptr ? *minZptr : pos.Z - 1.f;
	Float maxZ = maxZptr ? *maxZptr : pos.Z + 1.f;

	Vector3 testPos = pos;
	PathLib::CAreaDescription* area = nullptr;
	Bool isInitialPositionOk = false;

	if ( ComputeHeight( testPos.AsVector2(), minZ, maxZ, testPos.Z, areaId ) )
	{
		area = GetAreaDescription( areaId );

		PathLib::CCircleQueryData initialQuery( flags, testPos, personalSpace );
		if ( area->VSpatialQuery( initialQuery ) )
		{
			outPos = testPos;
			return true;
		}
		isInitialPositionOk = true;
	}
	else
	{
		area = GetTerrainAreaAtPosition( pos );
	}

	if ( !area )
	{
		// position is out of the world
		return false;
	}

	if ( isInitialPositionOk && area->VTestLocation( testPos, flags ) )
	{
		// algorithm "move away from wall"
		// up to 5 algorithm iterations
		for ( Uint32 i = 0; i < 5; ++i )
		{
			PathLib::CClosestObstacleCircleQueryData query( flags, testPos, personalSpace );
			area->VSpatialQuery( query );
			if ( !query.HasHit() )
			{
				break;
			}
			if ( query.m_closestDistSq == 0.f )
			{
				break;
			}
			Float obstacleDist = sqrt( query.m_closestDistSq );
			Vector2 diff = ( testPos.AsVector2() - query.m_pointOut.AsVector2() ).Normalized();
			diff *= personalSpace - obstacleDist + 0.05f;			// set length
			testPos.AsVector2() += diff;
			if ( !ComputeHeight( testPos.AsVector2(), minZ, maxZ, testPos.Z, areaId ) )
			{
				break;
			}
				
			PathLib::CAreaDescription* spotArea = areaId != area->GetId() ? GetAreaDescription( areaId ) : area;

			PathLib::CCircleQueryData safePosQuery( flags, testPos, personalSpace );
			if ( spotArea->VSpatialQuery( safePosQuery ) )
			{
				//// range test
				//if ( ( pos.AsVector2() - testPos.AsVector2() ).SquareMag() > radius*radius )
				//{
				//	return false;
				//}
				outPos = testPos;
				return true;
			}
		}
	}
	
	// fallback fully random mechanism
	Float testRadius = personalSpace * 2.f + 0.1f;
	Float radiusStep = Max( personalSpace, 0.5f );

	Float minZStep = radiusStep;
	if ( minZptr )
	{
		minZStep = 0;
	}

	Float maxZStep = radiusStep;
	if ( maxZptr )
	{
		maxZStep = 0;
	}

	while( testRadius <= radius )
	{
		for ( Uint32 i = 0; i < 8; ++i )
		{
			Vector3 testPos = pos;
			if ( optionalPreferedDir )
			{
				testPos.AsVector2() += MathUtils::GeometryUtils::Rotate2D( *optionalPreferedDir, ROT[ i ].X, ROT[ i ].Y );
			}
			else
			{
				testPos.AsVector2() += DIRS[i] * testRadius;
			}
			
			if ( area->IsTerrainArea() || !area->VComputeHeight( testPos.AsVector2(), minZ, maxZ, testPos.Z ) )
			{
				if ( !ComputeHeight( testPos.AsVector2(), minZ, maxZ, testPos.Z, areaId ) )
				{
					continue;
				}
			}
					
			PathLib::CAreaDescription* spotArea = areaId != area->GetId() ? GetAreaDescription( areaId ) : area;
			
			PathLib::CCircleQueryData safePosQuery( flags, testPos, personalSpace );
			if ( spotArea->VSpatialQuery( safePosQuery ) )
			{
				outPos = testPos;
				return true;
			}
		}
		testRadius += radiusStep;
		minZ -= minZStep;
		maxZ += maxZStep;
	}
	
	return false;
}

Bool CPathLibWorld::ComputeHeight( const Vector2& v, Float zMin, Float zMax, Float& zOut, PathLib::AreaId& outAreaId, Bool smooth ) const
{
	// instances
	{
		PathLib::CNavmeshAreaDescription* area = m_instanceMap.GetInstanceAt( v, zMin, zMax, zOut );
		if ( area )
		{
			outAreaId = area->GetId();
			return true;
		}
	}

	// terrain area
	PathLib::AreaId tileId = m_terrainInfo.GetTileIdAtPosition( v.AsVector2() );
	PathLib::CTerrainAreaDescription* terrainArea = GetTerrainAreaDescription( tileId );
	if ( terrainArea && terrainArea->IsReady() )
	{
		Float terrainZ;
		terrainArea->ComputeHeight( v, terrainZ, smooth );
		if ( terrainZ >= zMin && terrainZ <= zMax )
		{
			zOut = terrainZ;
			outAreaId = tileId;
			return true;
		}
	}
	return false;

}

// That should be just ComputeHeight( .. zMin, zMax .. ) code, but:
// - we already use ComputeHeight( .. zMin, zMax .. ) to hack ComputeHeight( pos ) where we want to guarantee output being in boundings (while we should return false in first place, but we didn't want to break things at that time).
// - at that hacked places we might be already dependent on the fact that we prefer navmeshes over terrain, even if they are below ground level.
// Insteady of unhacking above code we made new implementation.
// TODO: currently we are not supporting 'highest hit' on navmesh. It would be trivial thing to do, but would sacrifice performance a little, so until there are no problems issue is ignored.
Bool CPathLibWorld::ComputeHeightTop( const Vector2& v, Float zMin, Float zMax, Float& zOut, PathLib::AreaId& outAreaId ) const
{
	Bool wasHit = false;

	// terrain area
	PathLib::AreaId tileId = m_terrainInfo.GetTileIdAtPosition( v.AsVector2() );
	PathLib::CTerrainAreaDescription* terrainArea = GetTerrainAreaDescription( tileId );
	if ( terrainArea && terrainArea->IsReady() )
	{
		Float terrainZ;
		terrainArea->ComputeHeight( v, terrainZ, false );
		if ( terrainZ >= zMin && terrainZ <= zMax )
		{
			zOut = terrainZ;
			zMin = terrainZ;
			outAreaId = tileId;
			wasHit = true;
		}
	}


	// instances
	{
		PathLib::CNavmeshAreaDescription* area = m_instanceMap.GetInstanceAt( v, zMin, zMax, zOut );
		if ( area )
		{
			outAreaId = area->GetId();
			wasHit = true;
		}
	}

	return wasHit;

}

Bool CPathLibWorld::ComputeHeight( const Vector3& v, Float& zOut, Bool smooth ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v );
	if ( !area )
	{
		// position is out of the world
		return false;
	}

	return area->VComputeHeight( v, zOut, smooth );
}
Bool CPathLibWorld::ComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( posFrom );
	if ( !area )
	{
		// position is out of the world
		return false;
	}

	return area->VComputeHeightFrom( pos, posFrom, outHeight, smooth );
}
Bool CPathLibWorld::ComputeHeight( PathLib::AreaId& areaId, const Vector3& v, Float& zOut, Bool smooth ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v, areaId );
	if ( !area )
	{
		// position is out of the world
		return false;
	}

	return area->VComputeHeight( v, zOut, smooth );
}
Bool CPathLibWorld::ComputeHeightFrom( PathLib::AreaId& areaId, const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( posFrom, areaId );
	if ( !area )
	{
		// position is out of the world
		return false;
	}

	return area->VComputeHeightFrom( pos, posFrom, outHeight, smooth );
}
Bool CPathLibWorld::FindRandomPositionInRadius( PathLib::AreaId areaId, const Vector3& pos, Float searchRadius, Float personalSpace, Uint32 agentCategory, Vector3& posOut )
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( pos, areaId );
	if ( !area )
	{
		// position is out of the world
		return false;
	}

	if ( !area->IsReady() )
	{
		// find starting node
		PathLib::CNavGraph* navGraph = area->GetNavigationGraph( agentCategory );
		if ( navGraph )
		{
			Vector3 accessiblePos;
			if ( navGraph->FindClosestAccessibleNode( area, pos, personalSpace, searchRadius, accessiblePos ) )
			{
				posOut = accessiblePos;

				// TODO: move it randomly  around
				return true;
			}
		}
	}

	// couldn't find graph entry point
	// fallback to spacial search (might be usefull for small radiuses
	return FindSafeSpot( area->GetId(), pos, Min( searchRadius, 7.5f ), personalSpace, posOut );
}

void CPathLibWorld::Reload()
{
	Shutdown();
	Initialize();

	CWorld* world = GetWorld();

	WorldAttachedComponentsIterator it( world );
	while ( it )
	{
		PathLib::IComponent* pathlibComponent = (*it)->AsPathLibComponent();

		if ( pathlibComponent )
		{
			pathlibComponent->OnPathLibReload( world );
		}

		++it;
	}
}

void CPathLibWorld::Initialize()
{
	// folders initialization
	m_cookedDir = InternalGetDirectory( TXT("navi_cooked"), true );
	m_localDir = m_useLocalFolder ? InternalGetDirectory( TXT("navi_local"), true ) : nullptr;
	m_sourceDir = InternalGetDirectory( TXT("navi"), true );

	CWorld* world = GetWorld();
	const CClipMap* terrain = world->GetTerrain();
	if ( terrain )
	{
		m_terrainInfo.Initialize( terrain );
	}
	if ( world == GGame->GetActiveWorld() && GGame->IsActive() )
	{
		m_isGameRunning = true;
	}

	// initialize to zero all terrain tiles
	Uint32 tilesCount = m_terrainInfo.GetTilesCount();
	m_terrainAreas.Resize( tilesCount );
	for ( Uint32 i = 0; i < tilesCount; ++i )
	{
		m_terrainAreas[ i ] = NULL;
	}

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	m_generationManager->Initialize();
#endif
	
	// load all areas - new way
	Bool loadedCookedHeader = false;
	Red::TScopedPtr< PathLib::CPathLibConfiguration > configuration( PathLib::CPathLibConfiguration::Load( *this ) );
	if ( configuration && configuration->ValidateTerrainInfo( m_terrainInfo ) )
	{
		const auto& areas = configuration->GetAreas();
		for ( auto it = areas.Begin(), end = areas.End(); it != end; ++it )
		{
			PathLib::CAreaDescription* area = *it;
			area->SetPathLib( this );
			if ( area->IsTerrainArea() )
			{
				// terrain area registration
				Uint32 index = area->GetId() & PathLib::CAreaDescription::ID_MASK_INDEX;
				if ( index < tilesCount )
				{
					PATHLIB_ASSERT( !m_terrainAreas[ index ], TXT("There shouldn't be any area at this index (cause we do header checking)!") );
					m_terrainAreas[ index ] = area->AsTerrainArea();
				}
				else 
				{
					PATHLIB_ERROR( TXT("Too many terrain areas %d!"), index );
					delete area;
				}
			}
			else
			{
				PathLib::CNavmeshAreaDescription* naviArea = area->AsNavmeshArea();
				if ( !naviArea->GetOwnerGUID().IsZero() && m_instanceGuidMapping.Find( naviArea->GetOwnerGUID() ) != m_instanceGuidMapping.End() )
				{
					PATHLIB_ERROR( TXT("instance_%04x is duplicate. Please delete it!"), naviArea->GetId() );
					delete naviArea;
				}
				else
				{
					// instance area registration
					AddInstanceArea( naviArea );
				}
			}
		}

		m_worldLayers = Move( configuration->GetWorldLayers() );

		configuration->PostLoad();
		loadedCookedHeader = true;
	}
	else
	{
		// No configuration. Create some defaults for system to work.
		for ( PathLib::AreaId tileId = 0; tileId < tilesCount; ++tileId )
		{
			if ( !m_terrainAreas[ tileId ] )
			{
				PathLib::CTerrainAreaDescription* terrainArea = new PathLib::CTerrainAreaDescription( *this, tileId | PathLib::CAreaDescription::ID_MASK_TERRAIN );
				terrainArea->Initialize();
#ifndef NO_EDITOR_PATHLIB_SUPPORT
				terrainArea->MarkDirty( PathLib::CAreaDescription::DIRTY_GENERATE | PathLib::CAreaDescription::DIRTY_SURFACE );
#endif
				m_terrainAreas[ tileId ] = terrainArea;
			}
		}
	}

	// subsystems initialization
	m_worldLayers.Initialize( GetWorld() );
	m_taskManager->Initialize();
	m_instanceMap.Initialize();
	m_streamingManager->Initialize();
}

void CPathLibWorld::SetGameRunning( Bool isGameRunning )
{
	if ( isGameRunning != m_isGameRunning )
	{
		if ( !isGameRunning )
		{
			// destroy all temporary in-game objects (immediate obstacles, TODO: deactivate dynamic obstacles).
			m_mapper->RemoveImmediateObstacles();
		}
		
		m_isGameRunning = isGameRunning;
#ifndef NO_EDITOR_PATHLIB_SUPPORT
		m_generationManager->TurnOn( !isGameRunning, PathLib::CGenerationManager::DISABLE_GAME );
#endif
	}
}

void CPathLibWorld::Shutdown()
{
	m_streamingManager->Shutdown();

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	m_generationManager->Shutdown();
#endif

	m_taskManager->Shutdown();

	m_instanceMap.Shutdown();

	ClearAreas();
}

void CPathLibWorld::ClearAreas()
{
	for ( auto it = m_instanceAreas.Begin(), end = m_instanceAreas.End(); it != end; ++it )
	{
		delete it->m_area;
	}
	m_instanceAreas.ClearFast();
	for ( auto it = m_terrainAreas.Begin(), end = m_terrainAreas.End(); it != end; ++it )
	{
		delete (*it);
	}
	m_terrainAreas.ClearFast();
}

void CPathLibWorld::AddInstanceArea( PathLib::CNavmeshAreaDescription* area )
{
	PathLib::AreaId areaId = area->GetId();
	SInstanceAreaDesc areaDesc;
	areaDesc.m_id = areaId;
	areaDesc.m_area = area;
	// debug builds checkings
	PATHLIB_ASSERT( m_instanceAreas.Find( areaDesc ) == m_instanceAreas.End(),  TXT("There shouldn't be any area at this id (cause we do header checking)!") );
	PATHLIB_ASSERT( m_instanceGuidMapping.Find( area->GetOwnerGUID() ) == m_instanceGuidMapping.End(),  TXT("There shouldn't be any area with this guid (cause we do header checking)!") );

	m_instanceAreas.Insert( areaDesc );
	const CGUID& ownerGuid = area->GetOwnerGUID();
	if ( !ownerGuid.IsZero() )
	{
		m_instanceGuidMapping.Insert( area->GetOwnerGUID(), areaId );
	}

	if ( m_instanceMap.IsInitialized() )
	{
		m_instanceMap.AddInstance( area );
		m_streamingManager->OnAreaDynamicallyAdded( area );
	}
}

void CPathLibWorld::RemoveInstanceArea( PathLib::AreaId areaId )
{
	SInstanceAreaDesc areaDesc;
	areaDesc.m_id = areaId;
	areaDesc.m_area = NULL;
	auto itFind = m_instanceAreas.Find( areaDesc );
	if ( itFind == m_instanceAreas.End() )
	{
		PATHLIB_ASSERT( false, TXT( "This shouldn't be hit. Inform Michal Slapa plz.") );
		return;
	}

	PathLib::CNavmeshAreaDescription* naviArea = itFind->m_area;
	PATHLIB_ASSERT( naviArea );
	if ( naviArea->IsProcessing() )
	{
		PATHLIB_ERROR( TXT("Cannot remove processed area!\n") );
		return;
	}

	if ( m_visualizer )
	{
		m_visualizer->InstanceRemoved( areaId );
	}

	m_instanceMap.RemoveInstance( naviArea );
	m_streamingManager->OnAreaDynamicallyRemoved( naviArea );

	naviArea->OnRemoval();

	if ( naviArea->IsLoaded() )
	{
		naviArea->Unload();
	}

	m_instanceGuidMapping.Erase( naviArea->GetOwnerGUID() );
	m_instanceAreas.Erase( itFind );
	delete naviArea;
}

Bool CPathLibWorld::DeleteAreaFiles( CDirectory* dir, PathLib::AreaId areaId )
{

#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
	String fileName;
	Bool isNavmeshArea = (areaId & PathLib::CAreaDescription::ID_MASK_TERRAIN) == 0;
	{
		GetGenericFileName( areaId, fileName, TXT("navarea") );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			file->Unload();
			file->GetStatus();
			file->Delete( false, false );
		}
	}
	{
		GetGenericFileName( areaId, fileName, PathLib::CAreaNavgraphsRes::GetFileExtension() );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			file->Unload();
			file->GetStatus();
			file->Delete( false, false );
		}
	}
	{
		GetGenericFileName( areaId, fileName, PathLib::CObstaclesMap::GetFileExtension() );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			file->Unload();
			file->GetStatus();
			file->Delete( false, false );
		}
	}
	if ( isNavmeshArea )
	{
		GetGenericFileName( areaId, fileName, PathLib::CNavmeshRes::GetFileExtension() );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			file->Unload();
			file->GetStatus();
			file->Delete( false, false );
		}
	}
	if ( !isNavmeshArea )
	{
		GetGenericFileName( areaId, fileName, PathLib::CTerrainMap::GetFileExtension() );
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( file )
		{
			file->Unload();
			file->GetStatus();
			file->Delete( false, false );
		}
	}

	return true;

#else
	return false;
#endif

}

Bool CPathLibWorld::ReinitializeSystem()
{
	CWorld* world = GetWorld();
	const CClipMap* terrain = world->GetTerrain();
	if ( !terrain )
	{
		PATHLIB_ERROR( TXT("Pathlib world initialization error!\n") );
		return false;
	}

	m_terrainInfo.Initialize( terrain );
	Uint32 mapSize = GetTerrainInfo().GetTilesCount();
	if ( !DestroyTerrainData() )
	{
		PATHLIB_ERROR( TXT("Pathlib world initialization error!\n") );
	}
	m_streamingManager->Shutdown();
	m_instanceMap.Shutdown();
	ClearAreas();

	auto& terrainAreas = m_terrainAreas;
	terrainAreas.Resize( mapSize );
	for( Uint32 i = 0; i < mapSize; ++i )
	{
		PathLib::AreaId terrainAreaId = PathLib::AreaId( i ) | PathLib::CAreaDescription::ID_MASK_TERRAIN;
		terrainAreas[ i ]  = new PathLib::CTerrainAreaDescription( *this, terrainAreaId );
		terrainAreas[ i ]->Initialize();
	}
	m_instanceMap.Initialize();
	m_streamingManager->Initialize();

	return true;
}
void CPathLibWorld::RecalculateAllNavigationData()
{
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	for ( auto it = m_terrainAreas.Begin(), end = m_terrainAreas.End(); it != end; ++it ) 
	{
		(*it)->MarkDirty( PathLib::CAreaDescription::DIRTY_ALL );
	}
	for ( auto it = m_instanceAreas.Begin(), end = m_instanceAreas.End(); it != end; ++it )
	{
		(*it).m_area->MarkDirty( PathLib::CAreaDescription::DIRTY_ALL );
	}
#endif
}
CDirectory* CPathLibWorld::ForceGetLocalDataDirectory()
{
	if ( !m_localDir )
	{
		m_localDir = InternalGetDirectory( TXT("navi_local"), true );
	}
	return m_localDir;
}

CDirectory* CPathLibWorld::InternalGetDirectory( const String& dirName, Bool createIfMissing ) const
{
	CDiskFile* worldFile = GetWorld()->GetFile();
	if ( !worldFile )
	{
		return NULL;
	}

	return worldFile->GetDirectory()->CreateNewDirectory( dirName.AsChar() );
}

Bool CPathLibWorld::InitializeTerrain()
{
	CWorld* world = GetWorld();
	const CClipMap* terrain = world->GetTerrain();
	if ( !terrain )
	{
		return false;
	}

	if ( terrain->GetNumTilesPerEdge() == 0 )
	{
		return false;
	}

	m_terrainInfo.Initialize( terrain );
	ReinitializeSystem();
	return true;
	//if ( !DestroyTerrainData() )
	//{
	//	return false;
	//}

	//m_tileData.Resize( mapSize );

	//// mark all tiles modified
	//return true;
}

void CPathLibWorld::MarkTileSurfaceModified( Uint32 x, Uint32 y )
{
	PathLib::CTerrainAreaDescription* area = GetTerrainAreaDescription( m_terrainInfo.GetTileIdFromCoords( x, y ) );
	if ( area )
	{
		area->MarkDirty( PathLib::CAreaDescription::DIRTY_SURFACE );
#ifndef PATHLIB_NO_TERRAIN_HEIGHTCOMPUTATIONCONTEXT
		area->GetHeightData().Invalidate();
#endif
	}
}
void CPathLibWorld::MarkTileCollisionsModified( Uint32 x, Uint32 y )
{
	PathLib::CTerrainAreaDescription* area = GetTerrainAreaDescription( m_terrainInfo.GetTileIdFromCoords( x, y ) );
	if ( area )
	{
		area->MarkDirty( PathLib::CAreaDescription::DIRTY_COLLISIONDATA );
	}
}
void CPathLibWorld::MarkTileNavigationModified( Uint32 x, Uint32 y )
{
	PathLib::CTerrainAreaDescription* area = GetTerrainAreaDescription( m_terrainInfo.GetTileIdFromCoords( x, y ) );
	if ( area )
	{
		area->MarkDirty( PathLib::CAreaDescription::DIRTY_GENERATE );
	}
}
Bool CPathLibWorld::DestroyTerrainData()
{
	return true;
}
const Box& CPathLibWorld::GetStreamedWorldBox()
{
	return m_streamingManager->GetStreamedWorldBox();
}
void CPathLibWorld::NotifyOfInstanceAttached( CNavmeshComponent* instanceComponent )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind != m_instanceGuidMapping.End() )
	{
		// area already exist
		// TODO: Check consistency while in editor mode
		return;
	}

	if ( !instanceComponent->GetNavmesh() )
	{
		// no navmesh == we don't care
		return;
	}

	PathLib::CNavmeshAreaDescription* area = NULL;
	// Try to attach component to not-connected navmesh area
	PathLib::AreaId componentAreaId = instanceComponent->GetPathLibAreaId();
	Bool isNewArea = false;
	if ( componentAreaId != PathLib::INVALID_AREA_ID )
	{
		area = GetInstanceAreaDescription( componentAreaId );
		if ( area )
		{
			const CGUID& existingGuid = area->GetOwnerGUID();
			if ( !existingGuid.IsZero() )
			{
				auto itFind = m_instanceGuidMapping.Find( guid );
				if ( itFind != m_instanceGuidMapping.End() )
				{
					m_instanceGuidMapping.Erase( itFind );
				}
			}
			m_instanceGuidMapping.Insert( instanceComponent->GetGUID4PathLib(), componentAreaId );
		}
	}
	if ( !area )
	{
		PathLib::AreaId areaId = componentAreaId != PathLib::INVALID_AREA_ID ? componentAreaId : GetFreeInstanceId();
		isNewArea = true;

		if ( instanceComponent->IsNavmeshUsingTransformation() )
		{
			area = new PathLib::CNavmeshTransformedAreaDescription( *this, areaId );
		}
		else
		{
			area = new PathLib::CNavmeshAreaDescription( *this, areaId );
		}
	}

	CDirectory* saveDir = ForceGetLocalDataDirectory();

	area->NoticeEngineComponent( instanceComponent );
	area->MarkDirty( PathLib::CAreaDescription::DIRTY_GENERATE );

	if ( isNewArea )
	{
		AddInstanceArea( area );
	}

	if ( saveDir )
	{
		area->Save( saveDir, false );
		SaveSystemConfiguration();
	}

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	m_generationManager->RecalculateWaypoints( instanceComponent->GetBoundingBox() );
#endif
}
void CPathLibWorld::NotifyOfInstanceUpdated( CNavmeshComponent* instanceComponent, Bool areaChanged, Uint32 areaDirtyFlags )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind == m_instanceGuidMapping.End() )
	{
		NotifyOfInstanceAttached( instanceComponent );
		return;
	}
	PathLib::AreaId areaId = itFind->m_second;

	if ( !instanceComponent->GetNavmesh() )
	{
		// no navmesh == erase shitty instance
		RemoveInstanceArea( areaId );
		return;
	}

	// mark area as dirty
	PathLib::CNavmeshAreaDescription* naviArea = GetInstanceAreaDescription( areaId );
	naviArea->NoticeEngineComponent( instanceComponent );
	
	if ( areaDirtyFlags )
	{
		naviArea->MarkDirty( areaDirtyFlags, 1.f );
	}
	if ( areaChanged )
	{
		naviArea->OnNavmeshUpdated();
		m_instanceMap.UpdateInstance( naviArea );
		m_streamingManager->OnAreaDynamicallyUpdated( naviArea );
	}
	if ( m_visualizer )
	{
		m_visualizer->InstanceUpdated( areaId );
	}
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	m_generationManager->RecalculateWaypoints( instanceComponent->GetBoundingBox() );
#endif
}

void CPathLibWorld::NotifyOfInstanceRemoved( CNavmeshComponent* instanceComponent )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind == m_instanceGuidMapping.End() )
	{
		return;
	}
	NotifyOfInstanceRemoved( itFind->m_second );
}

void CPathLibWorld::NotifyOfInstanceRemoved( PathLib::AreaId areaId )
{
	RemoveInstanceArea( areaId );

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	CDirectory* sourceDir = GetSourceDataDirectory();
	if ( sourceDir )
	{
		DeleteAreaFiles( sourceDir, areaId );
	}
	CDirectory* localDir = GetLocalDataDirectory();
	if ( localDir )
	{
		DeleteAreaFiles( localDir, areaId );
	}
	SaveSystemConfiguration();
	
#endif
}

PathLib::AreaId CPathLibWorld::GetEngineComponentInstanceAreaMapping( CNavmeshComponent* instanceComponent )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind == m_instanceGuidMapping.End() )
	{
		return PathLib::INVALID_AREA_ID;
	}
	return itFind->m_second;
}

Bool CPathLibWorld::StartInstanceProcessing( CNavmeshComponent* instanceComponent )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind == m_instanceGuidMapping.End() )
	{
		return true;
	}
	PathLib::AreaId areaId = itFind->m_second;
	PathLib::CNavmeshAreaDescription* naviArea = GetInstanceAreaDescription( areaId );
	if ( !naviArea )
	{
		return true;
	}
	if ( naviArea->IsProcessing() )
	{
		return false;
	}
	naviArea->StartProcessing();
	return true;
}

Bool CPathLibWorld::IsInstanceProcessing( CNavmeshComponent* instanceComponent )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind == m_instanceGuidMapping.End() )
	{
		return false;
	}
	PathLib::AreaId areaId = itFind->m_second;
	PathLib::CNavmeshAreaDescription* naviArea = GetInstanceAreaDescription( areaId );
	if ( !naviArea )
	{
		return false;
	}
	return naviArea->IsProcessing();
}

void CPathLibWorld::EndInstanceProcessing( CNavmeshComponent* instanceComponent )
{
	const CGUID& guid = instanceComponent->GetGUID4PathLib();

	auto itFind = m_instanceGuidMapping.Find( guid );
	if ( itFind == m_instanceGuidMapping.End() )
	{
		return;
	}
	PathLib::AreaId areaId = itFind->m_second;
	PathLib::CNavmeshAreaDescription* naviArea = GetInstanceAreaDescription( areaId );
	if ( !naviArea )
	{
		return;
	}
	naviArea->EndProcessing();
}

void CPathLibWorld::NotifyOfComponentAttached( PathLib::IComponent* component )
{
	if ( m_isGameRunning )
	{
		if ( !component->IsNoticableInGame( PathLib::CProcessingEvent::TYPE_ATTACHED ) )
		{
			return;
		}
	}
	else
	{
		if ( m_isCookerMode || !m_isProcessingObstacles || !component->IsNoticableInEditor( PathLib::CProcessingEvent::TYPE_ATTACHED ) )
		{
			return;
		}
	}
	m_mapper->NotifyOfComponentAttached( component );
}
void CPathLibWorld::NotifyOfComponentUpdated( PathLib::IComponent* component, Bool force )
{
	if ( !m_isGameRunning && !m_isCookerMode && m_isProcessingObstacles && ( force || component->IsNoticableInEditor( PathLib::CProcessingEvent::TYPE_UPDATED ) ) )
	{
		m_mapper->NotifyOfComponentUpdated( component );
	}
}
void CPathLibWorld::NotifyOfComponentDetached( PathLib::IComponent* component )
{
	if ( m_isGameRunning )
	{
		if ( !component->IsNoticableInGame( PathLib::CProcessingEvent::TYPE_DETACHED ) )
		{
			return;
		}
	}
	else
	{
		if ( m_isCookerMode || !m_isProcessingObstacles || !component->IsNoticableInEditor( PathLib::CProcessingEvent::TYPE_DETACHED ) )
		{
			return;
		}
	}
	m_mapper->NotifyOfComponentDetached( component );
}
void CPathLibWorld::NotifyOfComponentRemoved( PathLib::IComponent* component )
{
	if ( m_isGameRunning )
	{
		if ( !component->IsNoticableInGame( PathLib::CProcessingEvent::TYPE_REMOVED ) )
		{
			return;
		}
	}
	else
	{
		if ( m_isCookerMode || !m_isProcessingObstacles || !component->IsNoticableInEditor( PathLib::CProcessingEvent::TYPE_REMOVED ) )
		{
			return;
		}
	}
	m_mapper->NotifyOfComponentRemoved( component );
}

void CPathLibWorld::OnLayerEnabled( CLayerGroup* layerGroup, Bool isEnabled )
{
	m_worldLayers.OnLayerEnabled( *this, layerGroup, isEnabled );
}

void CPathLibWorld::UpdateObstacles( Box& bbox )
{
	// Get static mesh components and update them
	for ( WorldAttachedComponentsIterator it( GetWorld() ); it; ++it )
	{
		CComponent* component = *it;
		PathLib::IComponent* pathlibComponent = component->AsPathLibComponent();
		if ( !pathlibComponent )
		{
			continue;
		}

		CBoundedComponent* boundedComponent = ::Cast< CBoundedComponent >( component );
		if ( boundedComponent )
		{
			if ( !boundedComponent->GetBoundingBox().Touches( bbox ) )
			{
				continue;
			}
		}
		else
		{
			if ( !bbox.Contains( component->GetWorldPositionRef() ) )
			{
				continue;
			}
		}

		m_mapper->NotifyOfComponentUpdated( pathlibComponent );
	}
}
Bool CPathLibWorld::UpdateInstancesMarking( const Vector2& vecMin, const Vector2& vecMax )
{
	Int32 tileMinX, tileMinY, tileMaxX, tileMaxY;
	m_terrainInfo.GetTileCoordsAtPosition( vecMin, tileMinX, tileMinY );
	m_terrainInfo.GetTileCoordsAtPosition( vecMax, tileMaxX, tileMaxY );

	// update marking on all terrain areas in the range
	Bool dirty = false;
	for ( Int32 tileY = tileMinY; tileY <= tileMaxY; ++tileY )
	{
		for ( Int32 tileX = tileMinX; tileX <= tileMaxX; ++tileX )
		{
			PathLib::CTerrainAreaDescription* terrainArea = GetTerrainAreaDescription( m_terrainInfo.GetTileIdFromCoords( tileX, tileY ) );
			if ( !terrainArea )
			{
				continue;
			}
			if ( !terrainArea->IsReady() )
			{
				// TODO: Mark dirty
				continue;
			}
			Vector2 localMin = vecMin;
			Vector2 localMax = vecMax;
			terrainArea->WorldToLocal( localMin );
			terrainArea->WorldToLocal( localMax );
			dirty = terrainArea->RemarkInstancesAtLocation( localMin, localMax ) || dirty;
		}
	}
	return dirty;
}

void CPathLibWorld::ReadConfiguration()
{
	m_useLocalFolder = false;

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	Bool isTaskManagerEnabled = false;
	if ( !SConfig::GetInstance().GetLegacy().ReadParam( TXT("path_lib"), TXT("TaskManager"), TXT("IsEnabled"), isTaskManagerEnabled ) )
	{
		isTaskManagerEnabled = false;
	}
	Bool isObstacleGenerationEnabled = false;
	if ( !SConfig::GetInstance().GetLegacy().ReadParam( TXT("path_lib"), TXT("TaskManager"), TXT("ObstacleGeneration"), isObstacleGenerationEnabled ) )
	{
		isObstacleGenerationEnabled = false;
	}
	m_generationManager->TurnOn( isTaskManagerEnabled, PathLib::CGenerationManager::DISABLE_TOOL );
	m_isProcessingObstacles = isObstacleGenerationEnabled;
#endif

	if ( !SConfig::GetInstance().GetLegacy().ReadParam( TXT("path_lib"), TXT("LocalFolder"), TXT("Use"), m_useLocalFolder ) )
	{
		m_useLocalFolder = false;
	}

}

Bool CPathLibWorld::IsLocalNavdataFolderForced()
{
	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		return pathlib->m_useLocalFolder;
	}
	Bool isVersionControlDisabled = false;
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("path_lib"), TXT("LocalFolder"), TXT("Use"), isVersionControlDisabled );
	return isVersionControlDisabled;
}

void CPathLibWorld::ForceLocalNavdataFolder( Bool b )
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("path_lib"), TXT("LocalFolder"), TXT("Use"), b );
	SConfig::GetInstance().GetLegacy().Save( false );
	
	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : nullptr;
	if ( pathlib && pathlib->m_useLocalFolder != b )
	{
		pathlib->m_useLocalFolder = b;
		if ( pathlib->m_useLocalFolder )
		{
			pathlib->ForceGetLocalDataDirectory();
			if ( !pathlib->IsGameRunning() )
			{
				pathlib->Reload();
			}
		}
	}
}

#ifndef NO_EDITOR_PATHLIB_SUPPORT

Bool CPathLibWorld::ForceSynchronousAreaProcessing( PathLib::AreaId areaId )
{
	PathLib::CAreaDescription* area = GetAreaDescription( areaId );
	if ( !area )
	{
		return false;
	}

	PATHLIB_LOG( TXT("Area synchronous processing.") );

	// TODO: Bring it back somehow!!! PathLib::CAreaHandler handler( area, PathLib::CAreaHandler::SYNCHRONOUS );
	if ( !area->IsReady() )
	{
		return false;
	}

	Int32 iterationsLeft = 15;
	while ( area->SyncProcessing( NULL, true ) && --iterationsLeft > 0 ) {}

	PATHLIB_LOG( TXT("Synchronous processing completed.") );

	return true;
}

Bool CPathLibWorld::IsTaskManagerEnabled()
{
	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		return !pathlib->m_generationManager->IsDisabledByTools();
	}
	Bool isTaskManagerEnabled = false;
	if ( !SConfig::GetInstance().GetLegacy().ReadParam( TXT("path_lib"), TXT("TaskManager"), TXT("IsEnabled"), isTaskManagerEnabled ) )
	{
		isTaskManagerEnabled = false;
	}
	return isTaskManagerEnabled;
}
Bool CPathLibWorld::IsObstacleGenerationEnabled()
{
	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		return pathlib->m_isProcessingObstacles;
	}
	Bool isObstacleGenerationEnabled = false;
	if ( !SConfig::GetInstance().GetLegacy().ReadParam( TXT("path_lib"), TXT("TaskManager"), TXT("ObstacleGeneration"), isObstacleGenerationEnabled ) )
	{
		isObstacleGenerationEnabled = false;
	}
	return isObstacleGenerationEnabled;
}
void CPathLibWorld::EnableTaskManager( Bool on )
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("path_lib"), TXT("TaskManager"), TXT("IsEnabled"), on );
	SConfig::GetInstance().GetLegacy().Save( false );

	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		pathlib->m_generationManager->TurnOn( on, PathLib::CGenerationManager::DISABLE_TOOL );
	}
}
void CPathLibWorld::EnableObstacleGeneration( Bool on )
{
	SConfig::GetInstance().GetLegacy().WriteParam( TXT("path_lib"), TXT("TaskManager"), TXT("ObstacleGeneration"), on );
	SConfig::GetInstance().GetLegacy().Save( false );

	CWorld* world = GGame->GetActiveWorld();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( pathlib )
	{
		pathlib->m_isProcessingObstacles = on;
		if ( on && !pathlib->IsGameRunning() )
		{
			pathlib->Reload();
		}
	}
}
Bool CPathLibWorld::SaveSystemConfiguration()
{
	PathLib::CPathLibConfiguration configuration;

	configuration.Reserve( m_terrainAreas.Size() + m_instanceAreas.Size() );
	{
		TerrainAreasIterator it( *this );
		while ( it )
		{
			if ( *it )
			{
				configuration.AddArea( *it );
			}
			
			++it;
		}
	}
	
	{
		InstanceAreasIterator it( *this );
		while ( it )
		{
			configuration.AddArea( *it );

			++it;
		}
	}
	configuration.SetTerrainInfo( m_terrainInfo );
	configuration.SetWorldLayersInfo( m_worldLayers );
	Bool ok = configuration.Save( *this );
	configuration.Clear();
	return ok;
}
#endif

PathLib::CNavmeshAreaDescription* CPathLibWorld::GetInstanceAreaDescription(PathLib::AreaId areaId) const
{
	SInstanceAreaDesc desc;
	desc.m_id = areaId;
	auto it = m_instanceAreas.Find( desc );
	if ( it != m_instanceAreas.End() )
	{
		return it->m_area;
	}
	return NULL;
}

PathLib::CTerrainAreaDescription* CPathLibWorld::GetTerrainAreaDescription(PathLib::AreaId areaId) const
{
	Uint32 index = areaId & PathLib::CAreaDescription::ID_MASK_INDEX;
	return index < m_terrainAreas.Size() ? &(*m_terrainAreas[ index ]) : NULL;
}

PathLib::CAreaDescription* CPathLibWorld::GetAreaDescription(PathLib::AreaId areaId) const
{
	if ( areaId & PathLib::CAreaDescription::ID_MASK_TERRAIN )
	{
		return GetTerrainAreaDescription( areaId );
	}
	return GetInstanceAreaDescription( areaId );
}
void CPathLibWorld::GenerateEditorFragments( CRenderFrame* frame )
{
	const auto& frameInfo = frame->GetFrameInfo();
	if ( frameInfo.IsShowFlagOn( SHOW_NavTerrain ) || frameInfo.IsShowFlagOn( SHOW_NavGraph ) || frameInfo.IsShowFlagOn( SHOW_NavMesh ) || frameInfo.IsShowFlagOn( SHOW_NavObstacles ) || frameInfo.IsShowFlagOn( SHOW_NavTerrainHeight ) )
	{
		if ( !m_visualizer )
		{
			m_visualizer = new PathLib::CVisualizer( this );
		}
		if ( m_visualizer )
		{
			m_visualizer->GenerateEditorFragments( frame );
		}
	}

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	if ( m_generationManager )
	{
		Bool isProcessing = false;
		const Int32 x = frame->GetFrameOverlayInfo().m_width - 350;
		if ( m_generationManager && m_generationManager->IsProcessing() )
		{
			String taskDescription;
			PathLib::CGenerationManager::CAsyncTask* job = m_generationManager->GetCurrentTask();
			job->DescribeTask( taskDescription );
			if ( !taskDescription.Empty() )
			{
				Int32 taskProgress = Clamp( Int32(job->GetTaskProgrees() * 100.f + 0.5f), 0, 100 );
				frame->AddDebugScreenFormatedText( x, 50, Color::LIGHT_GREEN, TXT("PathLib: %" ) RED_PRIWs TXT( " ... %0.2d%% completed"), taskDescription.AsChar(), taskProgress );
				isProcessing = true;
			}
		}
		if ( !isProcessing  )
		{
			if ( m_generationManager->IsDirty() && !GGame->IsActive() )
			{
				frame->AddDebugScreenText( x, 50, TXT("PathLib: pending tasks"), Color::LIGHT_RED );
			}
			else if ( m_generationManager->IsOn() )
			{
				frame->AddDebugScreenText( x, 50, TXT("PathLib: OK"), Color::LIGHT_BLUE );
			}
		}
	}
	
#endif
}
PathLib::AreaId CPathLibWorld::GetFreeInstanceId()
{
	Bool isUsed;
	PathLib::AreaId areaId;
	do
	{
		areaId = GEngine->GetRandomNumberGenerator().Get< PathLib::AreaId >() & PathLib::CAreaDescription::ID_MASK_INDEX;
		
		SInstanceAreaDesc desc;
		desc.m_id = areaId; 
		desc.m_area = NULL;
		isUsed = m_instanceAreas.Find( desc ) != m_instanceAreas.End();
	}
	while( isUsed );
	return areaId;
}

CDiskFile* CPathLibWorld::GetFile4Load( const String& fileName ) const
{
	CDiskFile* cookedFile = m_cookedDir ? m_cookedDir->FindLocalFile( fileName ) : nullptr;
#ifndef NO_EDITOR
	if ( m_useLocalFolder )
	{
		CDiskFile* localFile = m_localDir ? m_localDir->FindLocalFile( fileName ) : nullptr;

		if ( !localFile )
		{
			return cookedFile;
		}
		if ( !cookedFile )
		{
			return localFile;
		}
		return localFile->GetFileTime() > cookedFile->GetFileTime()
			? localFile
			: cookedFile;
	}
#endif
	return cookedFile;
}
CDirectory* CPathLibWorld::GetSaveDirectory()
{
	if ( m_isCookerMode )
	{
		return m_cookedDir;
	}
	return ForceGetLocalDataDirectory();
}
CDiskFile* CPathLibWorld::GetFile4Save( const String& fileName )
{
	CDirectory* dir = GetSaveDirectory();

	if ( dir )
	{
		CDiskFile* file = dir->FindLocalFile( fileName );
		if ( !file )
		{
			file = dir->CreateNewFile( fileName );
		}
		return file;
	}

	return nullptr;
}

void CPathLibWorld::GetGenericFileName( PathLib::AreaId areaId, String& outFilename, const Char* extension )
{
	if (areaId & PathLib::CAreaDescription::ID_MASK_TERRAIN)
	{
		Int32 x, y;
		m_terrainInfo.GetTileCoordsFromId( areaId, x, y );
		// terrain tile name
		outFilename = String::Printf( TXT( "terrain_%dx%d.%") RED_PRIWs, x, y, extension );
	}
	else
	{
		// instance tile name
		outFilename = String::Printf( TXT( "instance_%04x.%") RED_PRIWs, areaId, extension );
	}
}
void CPathLibWorld::GetNavgraphFileName( PathLib::AreaId areaId, String& outFilename )
{
	GetGenericFileName( areaId, outFilename, PathLib::CAreaNavgraphsRes::GetFileExtension() );
}
void CPathLibWorld::GetTerrainFileName( PathLib::AreaId areaId, String& outFilename )
{
	GetGenericFileName( areaId, outFilename, PathLib::CTerrainMap::GetFileExtension() );
}
void CPathLibWorld::GetObstaclesFileName( PathLib::AreaId areaId, String& outFilename )
{
	GetGenericFileName( areaId, outFilename, PathLib::CObstaclesMap::GetFileExtension() );
}
PathLib::AreaId CPathLibWorld::GetInstanceAreaIdFromFileName( const String& filename )
{
	Uint32 id = PathLib::INVALID_AREA_ID;

	if ( swscanf( filename.AsChar(), TXT( "instance_%04x" ), &id ) == 0 )
	{
		return PathLib::INVALID_AREA_ID;
	}

	return PathLib::AreaId( id );
}
RED_INLINE PathLib::CAreaDescription* CPathLibWorld::GetAreaAtPosition( const Vector3& v ) const
{
	// instances
	{
		PathLib::CNavmeshAreaDescription* area = m_instanceMap.GetInstanceAt( v );
		if ( area && area->IsReady() )
		{
			if ( area->IsUsingTransformation() )
			{
				if ( area->AsTransformedNavmeshArea()->ContainsPoint( v ) )
				{
					return area;
				}
			}
			else
			{
				if ( area->ContainsPoint( v ) )
				{
					return area;
				}
			}
		}
	}
	
	// terrain area
	return GetTerrainAreaAtPosition( v );
}

PathLib::CTerrainAreaDescription* CPathLibWorld::GetTerrainAreaAtPosition( const Vector3& v ) const
{
	PathLib::AreaId tileId = m_terrainInfo.GetTileIdAtPosition( v.AsVector2() );
	PathLib::CTerrainAreaDescription* terrainArea = GetTerrainAreaDescription( tileId );
	if ( terrainArea && terrainArea->IsReady() )
	{
		return terrainArea;
	}
	return NULL;
}

PathLib::CAreaDescription* CPathLibWorld::GetAreaAtPosition( const Vector3& v, PathLib::AreaId& hint ) const
{
	// try to skip instance map check if we are already in some instance
	if ( (hint & PathLib::CAreaDescription::ID_MASK_TERRAIN) == 0 )
	{
		PathLib::CNavmeshAreaDescription* area = GetInstanceAreaDescription( hint );
		if ( area && area->IsReady() )
		{
			if ( area->IsUsingTransformation() )
			{
				if ( area->AsTransformedNavmeshArea()->ContainsPoint( v ) )
				{
					return area;
				}
			}
			else
			{
				if ( area->ContainsPoint( v ) )
				{
					return area;
				}
			}
		}
	}
	
	PathLib::CAreaDescription* area = GetAreaAtPosition( v );
	if ( area )
	{
		hint = area->GetId();
		return area;
	}
	return NULL;
}
Bool CPathLibWorld::IsLocationLoaded( const Vector3& v )
{
	return GetAreaAtPosition( v ) != NULL;
}
Bool CPathLibWorld::IsLocationLoaded( const Vector3& v, PathLib::AreaId& outAreaId )
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v );
	if ( area )
	{
		outAreaId = area->GetId();
		return true;
	}
	return false;
}
PathLib::AreaId	CPathLibWorld::GetReadyAreaAtPosition( const Vector3& v )
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v );
	if ( area )
	{
		return area->GetId();
	}
	return PathLib::INVALID_AREA_ID;
}
PathLib::CNavmeshAreaDescription* CPathLibWorld::GetClosestInstanceArea( const Vector3& v )
{
	PathLib::AreaId id = m_instanceMap.GetClosestIntance( v, 100.f );
	if ( id != PathLib::INVALID_AREA_ID )
	{
		return GetInstanceAreaDescription( id );
	}
	return NULL;
}

Uint32 CPathLibWorld::CollectAreasAt( const Box& bbox, PathLib::CAreaDescription** areaList, Uint32 maxOutputSize, Bool getNavmeshAreas ) const
{
	struct Collector : public PathLib::CInstanceMap::CInstanceFunctor
	{
		Box							m_bbox;
		PathLib::CAreaDescription**	m_areaList;
		Uint32						m_maxOutputSize;
		Uint32						m_areasFound;

		Collector( const Box& bbox, PathLib::CAreaDescription** areaList, Uint32 maxOutputSize )
			: m_bbox( bbox )
			, m_areaList( areaList )
			, m_maxOutputSize( maxOutputSize )
			, m_areasFound( 0 ) {}

		Bool operator()( PathLib::CAreaDescription* area )
		{
			if ( m_bbox.Touches( area->GetBBox() ) )
			{
				if ( m_areasFound < m_maxOutputSize )
				{
					m_areaList[ m_areasFound++ ] = area;
				}
			}

			return true;
		}

		Bool Handle( PathLib::CNavmeshAreaDescription* naviArea ) override
		{
			return (*this)( naviArea );
		}
	};

	Collector collector( bbox, areaList, maxOutputSize );

	if ( getNavmeshAreas )
	{
		// Support instances
		m_instanceMap.IterateAreasAt( bbox, &collector );
	}

	const PathLib::CTerrainInfo& info = m_terrainInfo;
	Int32 minTileX, minTileY, maxTileX, maxTileY;
	info.GetTileCoordsAtPosition( collector.m_bbox.Min.AsVector2(), minTileX, minTileY );
	info.GetTileCoordsAtPosition( collector.m_bbox.Max.AsVector2(), maxTileX, maxTileY );
	for ( Int32 x = minTileX; x <= maxTileX; ++x )
	{
		for ( Int32 y = minTileY; y <= maxTileY; ++y )
		{
			PathLib::CTerrainAreaDescription* terrainArea = GetTerrainAreaDescription( info.GetTileIdFromCoords( x, y ) );
			if ( terrainArea )
			{
				collector( terrainArea );
			}
		}
	}

	return collector.m_areasFound;
}

template < class TQuery >
Bool CPathLibWorld::SpatialQuery( PathLib::CAreaDescription* area, TQuery& query ) const
{
	return area->VSpatialQuery( query );
}
template < class TQuery >
Bool CPathLibWorld::SpatialQuery( PathLib::CAreaDescription* area, PathLib::TMultiAreaSpatialQuery< TQuery >& query ) const
{
	return area->TMultiAreaQuery( query );
}
template < class TQuery >
Bool CPathLibWorld::SpatialQuery( TQuery& query ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( query.m_basePos );
	if ( area )
	{
		return area->VSpatialQuery( query );
	}
	return false;
}

template < class TQuery >
Bool CPathLibWorld::SpatialQuery( TQuery& query, PathLib::AreaId& areaId ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( query.m_basePos, areaId );
	if ( area )
	{
		return area->VSpatialQuery( query );
	}
	return false;
}

template < class TQuery >
Bool CPathLibWorld::SpatialQuery( PathLib::TMultiAreaSpatialQuery< TQuery >& query ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( query.m_query.m_basePos );
	if ( area )
	{
		return area->TMultiAreaQuery( query );
	}
	return false;
}
template < class TQuery >
Bool CPathLibWorld::SpatialQuery( PathLib::TMultiAreaSpatialQuery< TQuery >& query, PathLib::AreaId& areaId ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( query.m_query.m_basePos, areaId );
	if ( area )
	{
		return area->TMultiAreaQuery( query );
	}
	return false;
}
Bool CPathLibWorld::TestLine( const Vector3& v1, const Vector3& v2, Uint32 collisionFlags ) const
{
	PathLib::CLineQueryData::MultiArea query( PathLib::CLineQueryData( collisionFlags, v1, v2 ) );
	return SpatialQuery( query );
}
Bool CPathLibWorld::TestLine( const Vector3& v1, const Vector3& v2, Float personalSpace, Uint32 collisionFlags ) const
{
	PathLib::CWideLineQueryData::MultiArea query( PathLib::CWideLineQueryData( collisionFlags, v1, v2, personalSpace ) );
	return SpatialQuery( query );
}
Bool CPathLibWorld::TestLocation( const Vector3& v1, Float personalSpace, Uint32 collisionFlags ) const
{
	PathLib::CCircleQueryData::MultiArea query( PathLib::CCircleQueryData( collisionFlags, v1, personalSpace ) );
	return SpatialQuery( query );
}
Bool CPathLibWorld::TestLocation( const Vector3& v1, Uint32 collisionFlags ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v1 );
	if ( area )
	{
		return area->VTestLocation( v1, collisionFlags );
	}
	return false;
}
Float CPathLibWorld::GetClosestObstacle( const Vector3& v, Float personalSpace, Vector3& outCollisionPoint, Uint32 collisionFlags ) const
{
	PathLib::CClosestObstacleCircleQueryData::MultiArea query( PathLib::CClosestObstacleCircleQueryData( collisionFlags, v, personalSpace ) );
	SpatialQuery( query );
	if ( query.GetSubQuery().HasHit() )
	{
		outCollisionPoint = query.m_query.m_pointOut;
		return sqrt( query.m_query.m_closestDistSq );
	}
	else
	{
		return FLT_MAX;
	}
}
Float CPathLibWorld::GetClosestObstacle( const Vector3& v1, const Vector3& v2, Float personalSpace, Vector3& outCollisionPoint, Vector3& outPointOnLine, Uint32 collisionFlags ) const
{
	PathLib::CClosestObstacleWideLineQueryData::MultiArea query( PathLib::CClosestObstacleWideLineQueryData( collisionFlags, v1, v2, personalSpace ) );
	SpatialQuery( query );
	if ( query.GetSubQuery().HasHit() )
	{
		outPointOnLine = query.m_query.m_closestPointOnSegment;
		outCollisionPoint = query.m_query.m_closestGeometryPoint;
		return sqrt( query.m_query.m_closestDistSq );
	}
	else
	{
		return FLT_MAX;
	}
}

Bool CPathLibWorld::CollectGeometryAtLocation( const Vector3& v, Float personalSpace, Uint32 collisionFlags, TDynArray< Vector >& output ) const
{
	PathLib::CCollectGeometryInCirceQueryData::MultiArea query( PathLib::CCollectGeometryInCirceQueryData( collisionFlags, v, personalSpace ) );
	Bool ret = SpatialQuery( query );

	output = Move( query.m_query.m_output );
	
	return ret;
}

Bool CPathLibWorld::FindClosestWalkableSpotInArea( const Box& bbox, const Vector3& desiredSpot, Float personalSpace, Vector3& outPosition, PathLib::AreaId& outAreaId, CustomPositionFilter* filter ) const
{
	return false;
	//struct AreaFunctor : public PathLib::CInstanceMap::CInstanceFunctor, PathLib::CNodeFinder::Acceptor
	//{
	//	AreaFunctor( const Box& bbox, const Vector3& desiredSpot, Float personalSpace, CustomPositionFilter* filter )
	//		: Acceptor( false )
	//		, m_foundSomething( false )
	//		, m_bbox( bbox )
	//		, m_desiredSpot( desiredSpot )
	//		, m_personalSpace( personalSpace)
	//		, m_filter( filter )
	//	{
	//		Vector2 maxPointDist(
	//			Max( Abs( desiredSpot.X - bbox.Min.X ), Abs( desiredSpot.X - bbox.Max.X ) ),
	//			Max( Abs( desiredSpot.Y - bbox.Min.Y ), Abs( desiredSpot.Y - bbox.Max.Y ) )
	//			);

	//		m_closestSoFar = maxPointDist.Mag();
	//	}

	//	Bool Accept( const PathLib::CNavNode& node ) override
	//	{
	//		const Vector3& pos = node.GetPosition();
	//		if ( m_bbox.Contains( pos ) && (!m_filter || m_filter->TestPosition( pos ) ) )
	//		{
	//			return true;
	//		}
	//		return false;
	//	}

	//	void operator()( PathLib::CAreaDescription* area )
	//	{
	//		if ( !area->IsReady() )
	//		{
	//			return;
	//		}
	//		PathLib::CNavGraph* navgraph = area->GetNavigationGraph( 0 );
	//		if ( !navgraph )
	//		{
	//			return;
	//		}
	//		const PathLib::CCentralNodeFinder& nodeFinder = navgraph->GetNodeFinder();
	//		
	//		m_area = area;
	//		Float testDist = m_foundSomething ? m_closestSoFar : m_closestSoFar + area->GetMaxNodesDistance();
	//		// usage of PathLib::CNodeFinder::AcceptAll
	//		PathLib::CPathNode* pathNode = nodeFinder.FindClosestNode( m_desiredSpot, testDist, *this, PathLib::CCentralNodeFinder::AnyRegion( false, false ) );
	//		if ( pathNode )
	//		{
	//			m_outPosition = pathNode->GetPosition();
	//			m_closestSoFar = ( m_outPosition.AsVector2() - m_desiredSpot.AsVector2() ).Mag();
	//			m_outArea = area;
	//			m_foundSomething = true;
	//		}
	//	}

	//	// PathLib::CInstanceMap::CInstanceFunctor interface
	//	Bool Handle( PathLib::CNavmeshAreaDescription* naviArea ) override
	//	{
	//		(*this)( naviArea );
	//		return false;
	//	}

	//	Float				m_closestSoFar;
	//	Bool				m_foundSomething;
	//	const Box&			m_bbox;
	//	const Vector3&		m_desiredSpot;
	//	Float				m_personalSpace;
	//	Vector3				m_outPosition;
	//	PathLib::CAreaDescription*		m_outArea;
	//	PathLib::CAreaDescription*		m_area;
	//	CustomPositionFilter*			m_filter;
	//} finder( bbox, desiredSpot, personalSpace, filter );
	//Float closestPosDistSq = FLT_MAX;

	//Int32 tileMinX, tileMaxX, tileMinY, tileMaxY;
	//m_terrainInfo.GetTileCoordsAtPosition( bbox.Min.AsVector2(), tileMinX, tileMinY );
	//m_terrainInfo.GetTileCoordsAtPosition( bbox.Max.AsVector2(), tileMaxX, tileMaxY );
	//
	//for ( Int32 y = tileMinY; y <= tileMaxY; ++y )
	//{
	//	for ( Int32 x = tileMinX; x <= tileMaxX; ++x )
	//	{
	//		PathLib::CTerrainAreaDescription* terrainArea = GetTerrainAreaDescription( m_terrainInfo.GetTileIdFromCoords( x, y ) );
	//		if ( terrainArea )
	//		{
	//			finder( terrainArea );
	//		}
	//	}
	//}

	//m_instanceMap.IterateAreasAt( bbox, &finder );

	//if ( finder.m_foundSomething )
	//{
	//	PathLib::CAreaDescription* bestArea = finder.m_outArea;
	//	const Vector3& bestPosition = finder.m_outPosition;
	//	outAreaId = bestArea->GetId();

	//	Vector3 betterPosition;
	//	// try to move position as much as possible to desired spot
	//	if ( !GetClearLineInDirection( outAreaId, bestPosition,desiredSpot, personalSpace, betterPosition ) )
	//	{
	//		PATHLIB_ASSERT( false, TXT("Shouldn't ever happen!") );
	//		return false;
	//	}
	//	if ( filter && !filter->TestPosition( betterPosition ) )
	//	{
	//		outPosition = bestPosition;
	//	}
	//	else
	//	{
	//		outPosition = betterPosition;
	//	}
	//	
	//	return true;
	//}
	//return false;
}

Bool CPathLibWorld::TestLine( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Uint32 collisionFlags ) const
{
	PathLib::CLineQueryData::MultiArea query( PathLib::CLineQueryData( collisionFlags, v1, v2 ) );
	//PathLib::CLineQueryData query( collisionFlags, v1, v2 );
	return SpatialQuery( query, areaId );
}
Bool CPathLibWorld::TestLine(PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Float personalSpace, Uint32 collisionFlags = PathLib::CT_DEFAULT) const
{	
	PathLib::CWideLineQueryData::MultiArea query( PathLib::CWideLineQueryData( collisionFlags, v1, v2, personalSpace ) );
	//PathLib::CWideLineQueryData query( collisionFlags, v1, v2, personalSpace );
	return SpatialQuery( query, areaId );
}
Bool CPathLibWorld::TestLocation(PathLib::AreaId& areaId, const Vector3& v1, Float personalSpace, Uint32 collisionFlags ) const
{
	PathLib::CCircleQueryData::MultiArea query( PathLib::CCircleQueryData( collisionFlags, v1, personalSpace ) );
	//PathLib::CCircleQueryData query( collisionFlags, v1, personalSpace );
	return SpatialQuery( query, areaId );
}

PathLib::EClearLineTestResult CPathLibWorld::GetClearLineInDirection( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Float personalSpace, Vector3& outPos, Uint32 collisionFlags ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v1, areaId );
	if ( !area )
	{
		outPos = v1;
		return PathLib::CLEARLINE_INVALID_START_POINT;
	}

	PathLib::CClearWideLineInDirectionQueryData::MultiArea query( PathLib::CClearWideLineInDirectionQueryData( collisionFlags | PathLib::CT_NO_ENDPOINT_TEST, v1, v2, personalSpace ) );
	area->TMultiAreaQuery( query );

	if ( query.GetSubQuery().m_isFailedAtBasePos )
	{
		outPos = v1;
		return PathLib::CLEARLINE_INVALID_START_POINT;
	}

	Bool outcome = query.GetSubQuery().GetHitPos( outPos );

	if ( !area->VComputeHeightFrom( outPos.AsVector2(), v1, outPos.Z, true ) )
	{
		Float zMin = Min( v1.Z, v2.Z ) - 1.f;
		Float zMax = Max( v1.Z, v2.Z ) + 1.f;
		ComputeHeight( outPos.AsVector2(), zMin, zMax, outPos.Z, areaId, true );
	}

	return outcome
		? PathLib::CLEARLINE_WAS_HIT
		: PathLib::CLEARLINE_SUCCESS;
}

Bool CPathLibWorld::CollectCollisionPoints( PathLib::AreaId& areaId, const Vector3& v, Float personalSpace, PathLib::CCollectCollisionPointsInCircleProxy& proxy, PathLib::CollisionFlags collisionFlags ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v, areaId );
	if ( !area )
	{
		return PathLib::CLEARLINE_INVALID_START_POINT;
	}

	PathLib::CCollectCollisionPointsInCircleQueryData::MultiArea query( PathLib::CCollectCollisionPointsInCircleQueryData( collisionFlags, v, personalSpace, proxy ) );
	return area->TMultiAreaQuery( query );
}


Bool CPathLibWorld::TestLocation(PathLib::AreaId& areaId, const Vector3& v1, Uint32 collisionFlags ) const
{
	PathLib::CAreaDescription* area = GetAreaAtPosition( v1, areaId );
	if ( area )
	{
		return area->VTestLocation( v1, collisionFlags );
	}
	return false;
}
Bool CPathLibWorld::CustomCollisionTest( const Vector3& basePos, PathLib::SCustomCollisionTester& collisionTester ) const
{
	PathLib::CCustomTestQueryData::MultiArea query( PathLib::CCustomTestQueryData( PathLib::CT_DEFAULT, basePos, collisionTester ) );
	//PathLib::CCustomTestQueryData query( PathLib::CT_DEFAULT, basePos, collisionTester );
	return SpatialQuery( query );
}

Bool CPathLibWorld::ConvexHullCollisionTest( const TDynArray< Vector > & convexHull, const Box& convexHullBBox, const Vector & basePosition ) const
{
	//convex hull is defined in local coordinates translated by basePosition (no scaling and rotation)
	struct SConvexHullTester : public PathLib::SCustomCollisionTester
	{
		TDynArray< Vector >	conHull;
		Box					bbox;
		SConvexHullTester( const TDynArray< Vector > & convexHull, const Box& convexHullBBox, const Vector & basePosition)
			: conHull( convexHull )
			, bbox( convexHullBBox )
		{
			for ( auto it = conHull.Begin(), end = conHull.End(); it != end; ++it )
			{
				*it += basePosition;
			}
			bbox.Min += basePosition;
			bbox.Max += basePosition;
		}
		Bool IntersectLine( const Vector2& point1, const Vector2& point2 ) override
		{
			return MathUtils::GeometryUtils::TestIntersectionPolygonLine2D<Vector>( conHull, point1, point2 );
		}
		Bool IntersectRect( const Vector2& rectMin, const Vector2& rectMax ) override
		{
			return MathUtils::GeometryUtils::TestIntersectionPolygonRectangle2D<Vector>( conHull, rectMin, rectMax, 0.0001f );
		}
		void ComputeBBox( Box& outBBox ) override
		{
			outBBox = bbox;
		}
	};
	SConvexHullTester tester( convexHull, convexHullBBox, basePosition );
	return CustomCollisionTest( basePosition, tester );
}

Float CPathLibWorld::GetClosestObstacle( PathLib::AreaId& areaId, const Vector3& v, Float personalSpace, Vector3& outCollisionPoint, Uint32 collisionFlags ) const
{
	PathLib::CClosestObstacleCircleQueryData::MultiArea query( PathLib::CClosestObstacleCircleQueryData( collisionFlags, v, personalSpace ) );
	if ( !SpatialQuery( query, areaId ) )
	{
		outCollisionPoint = v;
		return 0.f;
	}
	if ( query.GetSubQuery().HasHit() )
	{
		outCollisionPoint = query.m_query.m_pointOut;
		return sqrt( query.m_query.m_closestDistSq );
	}
	else
	{
		return FLT_MAX;
	}
}
Float CPathLibWorld::GetClosestObstacle( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Float personalSpace, Vector3& outCollisionPoint, Vector3& outPointOnLine, Uint32 collisionFlags ) const
{
	PathLib::CClosestObstacleWideLineQueryData::MultiArea query( PathLib::CClosestObstacleWideLineQueryData( collisionFlags, v1, v2, personalSpace ) );
	if ( !SpatialQuery( query, areaId ) )
	{
		outPointOnLine = v1;
		outCollisionPoint = v1;
		return 0.f;
	}
	if ( query.GetSubQuery().HasHit() )
	{
		outPointOnLine = query.m_query.m_closestPointOnSegment;
		outCollisionPoint = query.m_query.m_closestGeometryPoint;
		return sqrt( query.m_query.m_closestDistSq );
	}
	else
	{
		return FLT_MAX;
	}
}
CWorld*	CPathLibWorld::GetWorld() const	
{ 
	return static_cast< CWorld* >( GetParent() ); 
}
