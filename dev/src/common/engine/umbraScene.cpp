#include "build.h"

#include "bitmapTexture.h"
#include "decalComponent.h"
#include "dimmerComponent.h"
#include "entity.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "mesh.h"
#include "meshComponent.h"
#include "pointLightComponent.h"
#include "renderCommands.h"
#include "renderFrame.h"
#include "renderFrameInfo.h"
#include "renderVertices.h"
#include "spotLightComponent.h"
#include "stripeComponent.h"
#include "umbraIncludes.h"
#include "umbraScene.h"
#include "umbraTile.h"
#include "viewport.h"
#include "world.h"
#include "../core/taskManager.h"
#include "../core/depot.h"
#include "../core/messagePump.h"
#include "../core/memoryHelpers.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselTurningOn;
extern Bool isAnselTurningOff;
#endif // USE_ANSEL

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CUmbraSmallestHoleOverrideComponent );

Box CUmbraSmallestHoleOverrideComponent::m_unitBox = Box( Vector::ZEROS, 0.5f );

void CUmbraSmallestHoleOverrideComponent::OnUpdateBounds()
{
	// No skinning, use static bounding box
	if ( !m_transformParent || !m_transformParent->ToSkinningAttachment() )
	{
		// Use default bounding box from mesh
		m_boundingBox = GetLocalToWorld().TransformBox( m_unitBox );
	}
	else
	{
		TBaseClass::OnUpdateBounds();
	}
}

void CUmbraSmallestHoleOverrideComponent::OnAttached( CWorld* world )
{
	PC_SCOPE_PIX( CUmbraSmallestHoleOverrideComponent_OnAttached );

	// Pass to base class
	TBaseClass::OnAttached( world );

	// Add to editor fragments group
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Bboxes );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_BboxesSmallestHoleOverride );
}

void CUmbraSmallestHoleOverrideComponent::OnDetached( CWorld* world )
{
	PC_SCOPE_PIX( CUmbraSmallestHoleOverrideComponent_OnDetached );

	// Pass to base class
	TBaseClass::OnDetached( world );
	// Remove from editor fragments group
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Bboxes );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_BboxesSmallestHoleOverride );
}

#ifdef USE_UMBRA
Bool CUmbraSmallestHoleOverrideComponent::OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds )
{
#if !defined(NO_UMBRA_DATA_GENERATION) && defined(USE_UMBRA_COOKING)
	if ( umbraScene && umbraScene->IsDuringSyncRecreation() && CUmbraScene::ShouldAddComponent( this ) )
	{
		return umbraScene->AddSmallestHoleOverride( this, bounds );
	}
#endif // NO_UMBRA_DATA_GENERATION

	return false;
}
#endif // USE_UMBRA

void CUmbraSmallestHoleOverrideComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_Bboxes && IsAttached() )
	{
		frame->AddDebugBox( m_boundingBox, Matrix::IDENTITY, Color::LIGHT_YELLOW );
	}

	// Generate bounding boxes
	if ( flag == SHOW_BboxesSmallestHoleOverride && IsAttached() )
	{
		Float userThickness = frame->GetFrameInfo().GetRenderingDebugOption( VDCommon_DebugLinesThickness );
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
#ifndef NO_COMPONENT_GRAPH
			frame->AddDebugFatBox( m_unitBox, m_localToWorld, GetHitProxyID().GetColor(), userThickness );
#endif
		}
		else
		{
			frame->AddDebugFatBox( m_unitBox, m_localToWorld, IsSelected() ? Color::GREEN : Color::RED, userThickness );
		}
	}
}
//////////////////////////////////////////////////////////////////////////

#ifdef USE_UMBRA

RED_DEFINE_STATIC_NAMED_NAME( VAR_NAME_W3NewDoor, "W3NewDoor" );

IMPLEMENT_ENGINE_CLASS( CUmbraScene );

using Umbra::SceneObject;
using Umbra::Visibility;
using Umbra::Query;
using Umbra::Frustum;
using Umbra::IndexList;
using Umbra::Task;
using Umbra::MatrixFormat;

void TileSet::Set( const TUmbraTileArray* tiles )
{
	m_tiles = tiles;
	m_numLoading.SetValue( tiles->Size() );
	for ( auto tile : *m_tiles )
	{
		if ( tile->GetTome() )
		{
			RED_LOG_SPAM( UmbraInfo, TXT("!_! Reusing Tome [%d x %d]"), tile->GetCoordinates().X, tile->GetCoordinates().Y );
			tile->AddRefTome();
			OnLoaded();
		}
		else
		{
			RED_LOG_SPAM( UmbraInfo, TXT("!_! Loading Tile [%d x %d]"), tile->GetCoordinates().X, tile->GetCoordinates().Y );
			tile->RequestAsyncLoad( [tile, this]( BufferHandle handle )
			{
				RED_FATAL_ASSERT( tile->GetTome() == nullptr, "" );
				size_t tomeDataSize = handle->GetSize();
				RED_ASSERT( tomeDataSize > 0 );
				RED_LOG_SPAM( UmbraInfo, TXT("  !_! Created Tome [%d x %d], data size: %ls"), tile->GetCoordinates().X, tile->GetCoordinates().Y, String::FormatByteNumber( tomeDataSize, TString< Char >::BNF_Precise ).AsChar() );
				RED_ASSERT( handle->GetData() );
				const Umbra::Tome* tome = (const Umbra::Tome*)handle->GetData();
				RED_ASSERT( tome );
				tile->SetTome( tome, handle );
				tile->AddRefTome();
				handle.Reset();
				tile->OnFinishedLoading();
				OnLoaded();
			} 
			);
		}
	}
}

Red::Threads::CMutex	CUmbraScene::s_tomesNotificationAccessMutex;
Bool					CUmbraScene::g_useOcclusionCulling = true;
Float					CUmbraScene::DEFAULT_UMBRA_DISTANCE_MULTIPLIER = 1.5f; // 1.5 * 256.0f = 384 <- that's the radius around camera we're gathering UmbraTiles with

CUmbraScene::CUmbraScene()
	: m_tilesCount( 0 )
	, m_tileSize( 0.0f )
	, m_renderOcclusionData( nullptr )
	, m_tomeCollectionJob( nullptr )
	, m_ping( false )
	, m_distanceMultiplier( DEFAULT_UMBRA_DISTANCE_MULTIPLIER )
	, m_dataUploaded( true )
	, m_hasData( false )
	, m_referencePositionValid( false )
	, m_tickState( TS_Idle )
{
}

CUmbraScene::~CUmbraScene()
{
	m_grid.Clear();
}

void CUmbraScene::Initialize( CWorld* world, Uint32 tilesCount, Float tileSize )
{
	RED_ASSERT( world );
	RED_ASSERT( tilesCount > 0 );
	RED_ASSERT( tileSize > 0.0f );

	m_tilesCount = tilesCount;
	m_tileSize = tileSize;
	m_renderOcclusionData = nullptr;

	m_grid.InitCells( VectorI( m_tilesCount, m_tilesCount, 0 ), Vector( m_tileSize, m_tileSize, 0 ), Vector( -( m_tilesCount / 2.0f ) * m_tileSize, -( m_tilesCount / 2.0f ) * m_tileSize, 0 ) );

	const String tilesDirName = TXT( "occlusion_tiles" );

	// Find/Create final terrain tiles folder
	CDirectory* tilesDir = world->GetFile()->GetDirectory()->CreateNewDirectory( tilesDirName.AsChar() );
	RED_ASSERT( tilesDir );

	for ( Int32 y = 0; y < m_tilesCount; ++y )
	{
		for ( Int32 x = 0; x < m_tilesCount; ++x )
		{
			VectorI tileCoords( x, y, 0, 0 );
			Uint16 tileId = m_grid.GetUniqueTileID( tileCoords );
			CUmbraTile* tile = ::CreateObject< CUmbraTile >( (CObject*)nullptr );
			tile->Initialize( this, tileCoords, tileId );

			const String tileFileName = String::Printf( TXT( "%s_%ix%i.%s" ), world->GetFile()->GetFileName().AsChar(), x, y, CUmbraTile::GetFileExtension() );
			const String tileDepotPath = tilesDir->GetDepotPath() + tileFileName;			

			CDiskFile* tileFile = GDepot->FindFile( tileDepotPath );
			if ( !tileFile )
			{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
				tileFile = new CDiskFile( tilesDir, tileFileName, tile );
				tilesDir->AddFile( tileFile );
				tileFile->Save();
				tileFile->SetChangelist( GetFile()->GetChangelist() );
				tileFile->Add();
#else
				RED_HALT( "Cannot create new file for umbra tile" );
#endif
			}
			else
			{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
				if ( !tileFile->GetResource() )
				{
					tileFile->Rebind( tile );
				}
				tileFile->Save();
				if ( !tileFile->IsAdded() )
				{
					tileFile->SetChangelist( GetFile()->GetChangelist() );
					tileFile->Add();
				}
#else
				RED_HALT( "Cannot create save file for umbra tile" );
#endif
			}

			m_grid.Insert( tile, tileCoords );
		}
	}
}

void CUmbraScene::Shutdown()
{
	if ( m_tomeCollectionJob )
	{
		while ( !m_tomeCollectionJob->IsFinished() ) {RED_BUSY_WAIT();}
		SAFE_RELEASE( m_tomeCollectionJob );
	}
	RemoveUnusedTilesAndTomeCollection( true );
	m_grid.Clear();
}

void CUmbraScene::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		TOcclusionGrid::ElementList elements;
		m_grid.GetAllElements( elements );
		for ( Uint32 i = 0; i < elements.Size(); ++i )
		{
			file << elements[ i ].m_Data;
		}
	}
	else
	{
		file << m_tilesCount;
		file << m_tileSize;

		Uint32 overallMemory = 0;
		Uint32 smallestBuffer = NumericLimits< Uint32 >::Max();
		Uint32 largestBuffer = NumericLimits< Uint32 >::Min();

		if ( file.IsWriter() )
		{
			TOcclusionGrid::ElementList elements;
			m_grid.GetAllElements( elements );
			Uint32 numberOfElements = elements.Size();
			file << numberOfElements;
			for ( Uint32 i = 0; i < elements.Size(); ++i )
			{
				VectorI iPos = elements[ i ].m_Position;
				Vector position( (Float)iPos.X, (Float)iPos.Y, (Float)iPos.Z, (Float)iPos.W );
				THandle< CUmbraTile > tile = elements[ i ].m_Data;
				file << position;
				file << tile;
			}
		}
		else
		{
			m_grid.InitCells( VectorI( m_tilesCount, m_tilesCount, 0 ), Vector( m_tileSize, m_tileSize, 0 ), Vector( -( m_tilesCount / 2.0f ) * m_tileSize, -( m_tilesCount / 2.0f ) * m_tileSize, 0 ) );

			Uint32 numberOfElements = 0;
			file << numberOfElements;

			TDynArray< CUmbraTile* > tiles;
			tiles.Reserve( numberOfElements );

			CTimeCounter loadTime;

			Uint32 numberOfEntries = 0;
			for ( Uint32 i = 0; i < numberOfElements; ++i )
			{
				Vector position;
				file << position;
				CUmbraTile* tile = nullptr;
				file << tile;
				if ( tile )
				{
					VectorI coordinates = m_grid.CalcCellIndexFromPosition( position );
					tile->Initialize( this, coordinates, m_grid.GetUniqueTileID( coordinates ) );
					m_grid.Insert( tile, position );

					tiles.PushBack( tile );
					numberOfEntries += tile->GetObjectCache().Size();

					Uint32 dataSize;
					if ( tile->HasData( &dataSize ) )
					{
						m_hasData = true;
						//LOG_ENGINE( TXT("Tile [%dx%d]: %s"), coordinates.X, coordinates.Y, String::FormatByteNumber( dataSize, String::BNF_Precise ).AsChar() );
						overallMemory += dataSize;
						smallestBuffer = Min( smallestBuffer, dataSize );
						largestBuffer = Max( largestBuffer, dataSize );
					}
				}
			}

#ifndef RED_FINAL_BUILD
			Uint32 missingUmbraTiles = 0;
			for ( Int32 y = 0; y < m_tilesCount; ++y )
			for ( Int32 x = 0; x < m_tilesCount; ++x )
			{
				const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( VectorI( x, y, 0, 0 ) );
				if ( elem.Size() == 0 )
				{
					++missingUmbraTiles;
				}
			}
			if ( missingUmbraTiles > 0 )
			{
				RED_HALT( "World you are loading has %d empty UmbraTiles. Fix this immediately!", missingUmbraTiles );
			}
#endif
			// Clean tile caches in runtime (we only need it on the render side)
			// when cooking, don't clean the data so that it is stored in the bundled files
			const Bool preserveTileCaches = file.IsCooker() || GIsCooker;
			m_objectCache.Reserve( m_objectCache.Size() + numberOfEntries );
			for ( auto it : tiles )
			{
				MergeObjectCache( it->GetObjectCache() );
				if ( !preserveTileCaches )
				{
					it->GetObjectCache().Clear();
				}
			}
			const TMemSize memSize = m_objectCache.GetInternalMemSize();
			RED_LOG( UmbraInfo, TXT("ObjectCache: loaded [%s] of data in: %1.2fms"), String::FormatByteNumber( memSize, String::BNF_Precise ).AsChar(), loadTime.GetTimePeriodMS() );
		}

		//LOG_ENGINE( TXT("Overall memory: %s"), String::FormatByteNumber( overallMemory, String::BNF_Precise ).AsChar() );
		//LOG_ENGINE( TXT("Smallest buffer:: %s"), String::FormatByteNumber( smallestBuffer, String::BNF_Precise ).AsChar() );
		//LOG_ENGINE( TXT("Largest buffer: %s"), String::FormatByteNumber( largestBuffer, String::BNF_Precise ).AsChar() );
	}
}

#ifndef NO_UMBRA_DATA_GENERATION
Bool CUmbraScene::FindDependency( const VectorI& srcId, const VectorI& dstId, TileDependency*& dependency )
{
	for ( Uint32 i = 0; i < m_tileDependencies.Size(); ++i )
	{
		TileDependency& dep = m_tileDependencies[i];
		if ( dep.src.X == srcId.X && dep.src.Y == srcId.Y && dep.dst.X == dstId.X && dep.dst.Y == dstId.Y )
		{
			dependency = &m_tileDependencies[i];
			return true;
		}
	}
	return false;
}
#endif	//!NO_UMBRA_DATA_GENERATION

#ifndef NO_UMBRA_DATA_GENERATION
#ifdef USE_UMBRA_COOKING
Bool CUmbraScene::ShouldGenerateData( const VectorI& id ) const
{
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( id );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );

	return elem[ 0 ].m_Data->ShouldGenerateData();
}
#endif //USE_UMBRA_COOKING

void CUmbraScene::ClearTileData( const VectorI& tileId )
{
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( tileId );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );
	elem[ 0 ].m_Data->Clear();
}

#ifdef USE_UMBRA_COOKING

void CUmbraScene::InsertDependencies( const Box& worldSpaceBoundingBox, const Matrix& transform, const TDynArray< UmbraObjectInfo >& addedObjects )
{
	const VectorI& tileCoords = GetTileIdFromPosition( transform.V[3] );

	const VectorI& gridBounds = m_grid.GetCellCounts();
	for ( Int32 y = -2; y <= 2; ++y )
	for ( Int32 x = -2; x <= 2; ++x )
	{
		if ( x == 0 && y == 0 )
		{
			// do not test the same tile
			continue;
		}

		VectorI tileIdToCheck( tileCoords.X + x, tileCoords.Y + y );
		if ( tileIdToCheck.X < 0 || tileIdToCheck.Y < 0 || tileIdToCheck.X >= gridBounds.X || tileIdToCheck.Y >= gridBounds.Y )
		{
			// tile out of bounds
			continue;
		}

		Box bb = m_grid.GetBoundingBoxOfTile( tileIdToCheck );
		if ( (worldSpaceBoundingBox.Min.X > bb.Max.X) || (worldSpaceBoundingBox.Min.Y > bb.Max.Y) || (worldSpaceBoundingBox.Max.X < bb.Min.X) || (worldSpaceBoundingBox.Max.Y < bb.Min.Y) )
		{
			continue;
		}

		TileDependency* tileDependency = nullptr;
		if ( FindDependency( tileCoords, tileIdToCheck, tileDependency ) )
		{
			RED_ASSERT( tileDependency );
			tileDependency->data.Grow( 1 );
			ComponentWithUmbraData& data = tileDependency->data.Back();
			data.umbraObjectInfo = addedObjects;
		}
		else
		{
			m_tileDependencies.Grow( 1 );
			TileDependency& dep = m_tileDependencies.Back();
			dep.src = tileCoords;
			dep.dst = tileIdToCheck;
			dep.data.Grow( 1 );
			ComponentWithUmbraData& data = dep.data.Back();
			data.umbraObjectInfo = addedObjects;
		}
	}
}

void RaportCorruptedComponent( const CComponent* component )
{
	if ( !component )
	{
		return;
	}
	if ( CEntity* entity = component->GetEntity() )
	{
		if ( CLayer* layer = entity->GetLayer() )
		{
			if ( CLayerInfo* layerInfo = layer->GetLayerInfo() )
			{
				const Vector& pos = component->GetLocalToWorld().V[3];
				WARN_ENGINE( TXT("Entity with suspicious position!! Layer: \"%s\", entity: \"%s\", component: \"%s\", position: [%1.2f, %1.2f, %1.2f]"),
					layerInfo->GetDepotPath().AsChar(),
					entity->GetName().AsChar(),
					component->GetName().AsChar(),
					pos.X, pos.Y, pos.Z );
			}
		}
	}
}

CUmbraScene::ETileInsertResult CUmbraScene::DetermineTileToInsert( const Matrix& transform, const VectorI& bounds, THandle< CUmbraTile >& tile ) const
{
	const Vector& position = transform.V[3];
	return DetermineTileToInsert( position, bounds, tile );
}

CUmbraScene::ETileInsertResult CUmbraScene::DetermineTileToInsert( const Vector& position, const VectorI& bounds, THandle< CUmbraTile >& tile ) const
{
	VectorI tileCoords = GetTileIdFromPosition( position );

	const VectorI& gridBounds = m_grid.GetCellCounts();
	if ( tileCoords.X < 0 || tileCoords.X >= gridBounds.X ||
		 tileCoords.Y < 0 || tileCoords.Y >= gridBounds.Y )
	{
		// out of grid bounds
		return TIR_OutsideGrid;
	}

	if ( tileCoords.X < bounds.X /*minX*/ || tileCoords.X > bounds.Y /*maxX*/ ||
		tileCoords.Y < bounds.Z /*minY*/ || tileCoords.Y > bounds.W /*maxY*/ )
	{
		// out of bounds from occlusioncommandlet
		return TIR_OutsideBounds;
	}

	static const Float MAX_Z = 4000.0f;
	if ( position.Z <= -MAX_Z || position.Z >= MAX_Z )
	{
		return TIR_OutsideGrid;
	}

	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( tileCoords );
	if ( elem.Size() != 1 || !elem[ 0 ].m_Data )
	{
		return TIR_OutsideGrid;
	}

	tile = elem[0].m_Data;
	return TIR_TileDetermined;
}
Bool CUmbraScene::AddMesh( const CMeshComponent* component, const VectorI& bounds, Uint8 flags )
{
	if ( !component )
	{
		return false;
	}

	if ( component->UseWithSimplygonOnly() )
	{
		// Simplygon-only mesh
		return false;
	}

	const Matrix& transform = component->GetLocalToWorld();

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( transform, bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}

	if ( !component->IsVisible() )
	{
		flags |= CCF_TargetOnly;
	}

	const CMesh* mesh = component->GetMeshNow();
	if ( !mesh )
	{
		return false;
	}

	if ( !mesh->IsStatic() )
	{
		// don't use dynamic meshes as occluders
		return false;
	}

	if ( !mesh->IsOccluder() )
	{
		if ( flags & CCF_Gate )
		{
			// if door mesh is explicitly marked as non-occluder, just skip it in the data collection
			return false;
		}

		// don't use meshes that are explicitly marked as not being occluders
		flags |= CCF_TargetOnly;
	}

	if ( mesh->HasSmallestHoleOverride() )
	{
		flags |= CCF_OverrideComputationParameters;
	}

	//HACK exclude skybox
	CEntity* entity = component->GetEntity();
	if ( entity && entity->GetName().ContainsSubstring( TXT("skybox") ) )
	{
		return false;
	}

	// INTERIOR VOLUMES HACK!!!!!
	Bool isVolume = false;
	const String weather_volume( TXT("weather_volume") ); 

	if ( entity->GetName().ContainsSubstring( weather_volume ) )
	{
		isVolume = true;
		flags |= CCF_VolumeOnly;
	}
	if ( !isVolume )
	{
		// check layer name
		if ( CLayer* layer = entity->GetLayer() )
		{
			if ( layer->GetLayerInfo() && layer->GetLayerInfo()->GetShortName() == weather_volume )
			{
				isVolume = true;
				flags |= CCF_VolumeOnly;
			}
		}
	}
	if ( !isVolume )
	{
		CTimeCounter timer;
		CFilePath path( mesh->GetDepotPath() );
		for ( const auto& dirName : path.GetDirectories() )
		{
			if ( dirName == weather_volume )
			{
				isVolume = true;
				flags |= CCF_VolumeOnly;
				break;
			}
		}
	}
	if( !isVolume )
	{
		const String material_volume( TXT("volume") ); 
		for ( const auto& matName : mesh->GetMaterialNames() )
		{
			if ( matName == material_volume )
			{
				isVolume = true;
				flags |= CCF_VolumeOnly;
				break;
			}
		}
	}
	// END OF INTERIOR VOLUMES HACK!!!!!

	if ( mesh->IsTwoSided() || component->IsTwoSided() )
	{
		flags |= CCF_TwoSided;
	}

	TDynArray< UmbraObjectInfo > addedObjects;

	CUmbraTile::ObjectInfo objectInfo;
	objectInfo.m_id = component->GetOcclusionId();
	const Float autoHideDistance = component->GetAutoHideDistance();
	objectInfo.m_autoHideDistance = autoHideDistance > 0.0f ? autoHideDistance : component->GetMaxAutohideDistance();
	objectInfo.m_boundingBox = transform.TransformBox( mesh->GetBoundingBox() );
	objectInfo.m_boundingBoxCenter = objectInfo.m_boundingBox.CalcCenter();
	objectInfo.m_flags = flags;
	objectInfo.m_transform = transform;
	Bool res = tile->AddMesh( objectInfo, mesh, addedObjects );

	InsertDependencies( objectInfo.m_boundingBox, transform, addedObjects );

	return res;
}

Bool CUmbraScene::AddDecal( const CDecalComponent* component, const VectorI& bounds )
{
	if ( !component )
	{
		return false;
	}
	if ( !component->GetDiffuseTexture() )
	{
		return false;
	}

	const Matrix& transform = component->GetLocalToWorld();

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( transform, bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}

	if ( !component->GetDiffuseTexture() )
	{
		return false;
	}

	if ( !component->IsVisible() )
	{
		return false;
	}
	TDynArray< UmbraObjectInfo > addedObjects;

	CUmbraTile::ObjectInfo objectInfo;
	objectInfo.m_id = component->GetOcclusionId();
	objectInfo.m_autoHideDistance = component->GetAutoHideDistance();
	objectInfo.m_boundingBox = component->GetBoundingBox();
	objectInfo.m_boundingBoxCenter = objectInfo.m_boundingBox.CalcCenter();
	objectInfo.m_flags = 0;
	objectInfo.m_transform = transform;
	Bool res = tile->AddDecal( objectInfo, addedObjects );

	InsertDependencies( objectInfo.m_boundingBox, transform, addedObjects );

	return res;
}

Bool CUmbraScene::AddTerrain( const VectorI& tileCoords, const CClipMap* clipmap )
{
	RED_ASSERT( m_isDuringSyncRecreation );
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( tileCoords );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );

	return elem[ 0 ].m_Data->AddTerrain( clipmap );
}

Bool CUmbraScene::AddWaypoint( const Vector& position, const VectorI& bounds )
{
	THandle< CUmbraTile > tile;
	if ( DetermineTileToInsert( position, bounds, tile ) != TIR_TileDetermined )
	{
		// boooo... no component to raport... it's in the game project
		//RaportCorruptedComponent( component );
		return false;
	}
	return tile->AddWaypoint( position );
}

Bool CUmbraScene::AddDimmer( const CDimmerComponent* component, const VectorI& bounds )
{
	if ( !component )
	{
		return false;
	}

	const Matrix& transform = component->GetLocalToWorld();

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( transform, bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}
	
	if ( !component->IsVisible() )
	{
		return false;
	}

	TDynArray< UmbraObjectInfo > addedObjects;

	CUmbraTile::ObjectInfo objectInfo;
	objectInfo.m_id = component->GetOcclusionId();
	objectInfo.m_boundingBox = component->GetBoundingBox();
	objectInfo.m_boundingBoxCenter = objectInfo.m_boundingBox.CalcCenter();
	objectInfo.m_flags = 0;
	objectInfo.m_transform = transform;
	objectInfo.m_autoHideDistance = component->GetAutoHideDistance();

	Bool res = tile->AddDimmer( objectInfo, addedObjects );

	InsertDependencies( objectInfo.m_boundingBox, transform, addedObjects );

	return res;
}

Bool CUmbraScene::AddStripe( const CStripeComponent* component, const VectorI& bounds, const TDynArray< Vector >& vertices, const TDynArray< Uint16 >& indices, const Box& worldSpaceBBox )
{
	if ( !component )
	{
		return false;
	}

	const Matrix& transform = component->GetLocalToWorld();

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( transform, bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}

	TDynArray< UmbraObjectInfo > addedObjects;

	CUmbraTile::ObjectInfo objectInfo;
	objectInfo.m_id = component->GetOcclusionId();
	objectInfo.m_boundingBox = worldSpaceBBox; // bounding box created when geometry was generated
	objectInfo.m_boundingBoxCenter = objectInfo.m_boundingBox.CalcCenter();
	objectInfo.m_autoHideDistance = component->GetAutoHideDistance();
	objectInfo.m_flags = 0;
	objectInfo.m_transform = transform;
	Bool res = tile->AddStripe( objectInfo, vertices, indices, addedObjects );

	InsertDependencies( objectInfo.m_boundingBox, transform, addedObjects );

	return res;
}

Bool CUmbraScene::AddPointLight( const CPointLightComponent* component, const VectorI& bounds )
{
	if ( !component )
	{
		return false;
	}

	const Matrix& transform = component->GetLocalToWorld();

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( transform, bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}

	TDynArray< UmbraObjectInfo > addedObjects;

	CUmbraTile::ObjectInfo objectInfo;
	objectInfo.m_id = component->GetOcclusionId();
	// offset autohide by radius and range (range for fading the light out, radius to start calculation from the sphere)
	objectInfo.m_autoHideDistance = component->GetAutoHideDistance() + component->GetRadius() + component->GetAutoHideRange();
	objectInfo.m_boundingBox = Box( component->GetWorldPosition(), component->GetRadius() );
	objectInfo.m_boundingBoxCenter = objectInfo.m_boundingBox.CalcCenter();
	objectInfo.m_flags = 0;
	objectInfo.m_transform = transform;
	Bool res = tile->AddPointLight( objectInfo, component->GetRadius(), addedObjects );

	InsertDependencies( objectInfo.m_boundingBox, transform, addedObjects );

	return res;
}

Box CalculateSpotlightBoundingBox( const CSpotLightComponent* component )
{
	const Matrix& matrix = component->GetLocalToWorld();
	const Float radius = component->GetRadius();
	const Float outerAngle = component->GetOuterAngle();

	// Get light vectors
	const Vector& center = component->GetWorldPositionRef();
	Vector x = matrix.GetAxisX();
	Vector y = matrix.GetAxisY();
	Vector z = matrix.GetAxisZ();

	Box wsBBox( Box::EResetState::RESET_STATE );
	wsBBox.AddPoint( center );
	wsBBox.AddPoint( center + y * radius );

	// Calculate cone points
	const Uint32 numSegments = 18;
	for ( Uint32 i = 0; i < numSegments; ++i )
	{
		const Float angle = (i / (Float)numSegments) * M_PI * 2.0f;
		Vector coneVector = ( x * Red::Math::MCos( angle ) ) + ( z * Red::Math::MSin( angle ) );
		Vector conePoint = center + ( y * radius * Red::Math::MCos( DEG2RAD( outerAngle * 0.5f  ) ) ) + coneVector * Red::Math::MSin( DEG2RAD( outerAngle * 0.5f ) ) * radius;
		wsBBox.AddPoint( conePoint );
	}
	
	return wsBBox;
}

Bool CUmbraScene::AddSpotLight( const CSpotLightComponent* component, const VectorI& bounds )
{
	if ( !component )
	{
		return false;
	}

	const Matrix& transform = component->GetLocalToWorld();

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( transform, bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}

	TDynArray< UmbraObjectInfo > addedObjects;

	CUmbraTile::ObjectInfo objectInfo;
	objectInfo.m_id = component->GetOcclusionId();
	// offset autohide by radius and range (range for fading the light out, radius to start calculation from the sphere)
	objectInfo.m_autoHideDistance = component->GetAutoHideDistance() + component->GetRadius() + component->GetAutoHideRange();
	objectInfo.m_boundingBox = CalculateSpotlightBoundingBox( component );
	objectInfo.m_boundingBoxCenter = objectInfo.m_boundingBox.CalcCenter();
	objectInfo.m_flags = 0;
	objectInfo.m_transform = transform;
	Bool res = tile->AddSpotLight( objectInfo, component->GetRadius(), component->GetInnerAngle(), component->GetOuterAngle(), addedObjects );

	InsertDependencies( objectInfo.m_boundingBox, transform, addedObjects );

	return res;
}

Bool CUmbraScene::AddSmallestHoleOverride( const CUmbraSmallestHoleOverrideComponent* component, const VectorI& bounds )
{
	if ( !component )
	{
		return false;
	}

	THandle< CUmbraTile > tile;
	CUmbraScene::ETileInsertResult tileInsertResult = DetermineTileToInsert( component->GetLocalToWorld(), bounds, tile );
	if ( tileInsertResult != TIR_TileDetermined )
	{
		if ( tileInsertResult == TIR_OutsideGrid )
		{
			RaportCorruptedComponent( component );
		}
		return false;
	}

	return tile->AddSmallestHoleOverride( component->GetBoundingBox(), component->GetSmallestHoleOverride() );
}

Bool CUmbraScene::GenerateTomeForTileSync( STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData )
{
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( context.tileId );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );

	return elem[ 0 ].m_Data->GenerateTomeDataSync( context, dumpScene, dumpRawTomeData );
}

Bool CUmbraScene::DumpTomeDataForTile( const STomeDataGenerationContext& context )
{
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( context.tileId );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );

	return elem[ 0 ].m_Data->DumpTomeData( context );
}
#endif //USE_UMBRA_COOKING

#ifndef RED_FINAL_BUILD
Bool CUmbraScene::GetMemoryStatsForPosition( const Vector& position, UmbraMemoryStats& stats )
{
	RemoveUnusedTilesAndTomeCollection( true );

	CleanupActiveTomes( nullptr );
	if ( !UpdateTomeCollection( position, nullptr ) )
	{
		return false;
	}

	// load tiles
	{	
		m_tileSet.Set( m_ping ? &m_tomesForTomeCollectionGenerationPing : &m_tomesForTomeCollectionGeneration );
		while ( m_tileSet.IsLoading() ) { /* wait for the loading to finish */ }

		stats.m_umbraPoolAllocatedMemory = Memory::GetAllocatedBytesPerMemoryClass< MemoryPool_Umbra >( MC_UmbraTome );
		stats.m_tempBufferAllocatedMemoryPeak = Memory::GetAllocatedBytesPerMemoryClassPeak< MemoryPool_Default >( MC_UmbraBuffer );
	}

	// create TomeCollection
	{
		TUmbraTileArray& tomesForJob = m_ping ? m_tomesForTomeCollectionGenerationPing : m_tomesForTomeCollectionGeneration;
		stats.m_numberOfLoadedTiles = tomesForJob.Size();
		m_tomeCollectionJob = new ( CTask::Root ) CJobCreateTomeCollection( tomesForJob, nullptr, m_ping ? m_tomeCollection : m_tomeCollectionPing );
		GTaskManager->Issue( *m_tomeCollectionJob );
		while ( !m_tomeCollectionJob->IsFinished() ) { RED_BUSY_WAIT();/* wait for the job to finish */ }
	}

	// fill in TomeCollection stats
	{
		if ( m_tomeCollectionJob->GetResult() )
		{
			stats.m_tomeCollectionSize = m_tomeCollectionJob->GetTomeCollectionSize();
			stats.m_tomeCollectionScratchAllocationPeak = m_tomeCollectionJob->GetScratchAllocationPeak();
		}
		m_tomeCollectionJob->RemoveTomeCollection();
		SAFE_RELEASE( m_tomeCollectionJob );
		m_tickState = TS_Idle;
	}

	CleanupActiveTomes( nullptr );
	return true;
}
#endif // RED_FINAL_BUILD

void CUmbraScene::ProcessDependencies( const VectorI& id )
{
	for ( Uint32 i = 0; i < m_tileDependencies.Size(); ++i )
	{
		TileDependency& dep = m_tileDependencies[ i ];
		if ( dep.src.X != id.X || dep.src.Y != id.Y )
		{
			continue;
		}

		for ( Uint32 j = 0; j < dep.data.Size(); ++j )
		{
			ComponentWithUmbraData& data = dep.data[ j ];
			for ( Uint32 k = 0; k < data.umbraObjectInfo.Size(); ++k )
			{
				UmbraObjectInfo& objectInfo = data.umbraObjectInfo[k];
#ifdef USE_UMBRA_COOKING
				Bool status = InsertExistingObject( dep.dst, objectInfo );
#else
				Bool status = false;
#endif //USE_UMBRA_COOKING
				if ( !status )
				{
					RED_LOG_ERROR( UmbraError, TXT("Error processing Umbra dependency: [%d;%d]->[%d;%d]: [%d]"), dep.src.X, dep.src.Y, dep.dst.X, dep.dst.Y, objectInfo.objectId );
				}
			}
		}
	}
}

Bool CUmbraScene::ShouldAddComponent( const CComponent* component, Uint8* flags /*=nullptr*/ )
{
	if ( !component )
	{
		return false;
	}

	CEntity* entity = component->GetEntity();
	if ( !entity )
	{
		return false;
	}

	const CName& entityClassName = entity->GetClass()->GetName();
	if ( entityClassName == CNAME( VAR_NAME_W3NewDoor ) )
	{
		// cook doors as gates
		if ( flags )
		{
			*flags = CCF_Gate;
		}
		return flags != nullptr;
	}
	else
	{
		if ( !component->ShouldBeCookedAsOcclusionData() )
		{
			return false;
		}
	}

	CLayer* layer = entity->GetLayer();
	RED_ASSERT( layer );

	CLayerInfo* layerInfo = layer->GetLayerInfo();
	RED_ASSERT( layerInfo );

	ELayerType layerType = layerInfo->GetLayerType();
	if ( layerType == LT_NonStatic )
	{
		if ( flags )
		{
			(*flags) |= CCF_TargetOnly;
		}
		return true;
	}
	
	ELayerBuildTag layerBuildTag = layerInfo->GetLayerBuildTag();
	switch (layerBuildTag)
	{
		case LBT_EnvOutdoor:
		case LBT_EnvIndoor:
		case LBT_EnvUnderground:
			if ( flags )
			{
				(*flags) |= CCF_Occluder;
				(*flags) |= CCF_Target;
			}
			break;

		case LBT_Quest:
		case LBT_Communities:
		case LBT_Gameplay:
		case LBT_DLC:
			if ( flags )
			{
				(*flags) |= CCF_TargetOnly;
			}
			return true;

		case LBT_None:
		case LBT_Ignored:
		case LBT_Audio:
		case LBT_Nav:
		case LBT_Max:
			// we do not want those like you here
			return false;

		default:
			RED_HALT( "Occlusion cooking: Unsupported LayerBuildTag." );
			return false;
	}

	CLayerGroup* layerGroup = layerInfo->GetLayerGroup();
	RED_ASSERT( layerGroup );
	if ( !layerGroup->IsVisibleOnStart() )
	{
		if ( flags )
		{
			(*flags) |= CCF_TargetOnly;
		}
	}

	return true;	
}

#ifdef USE_UMBRA_COOKING
Uint32 CUmbraScene::GetTileDensity( const VectorI& id )
{
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( id );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );

	return elem[ 0 ].m_Data->GetTileDensity();
}

Bool CUmbraScene::InsertExistingObject( const VectorI& id, const UmbraObjectInfo& objectInfo )
{
	const TOcclusionGrid::ElementList& elem = m_grid.GetElementsByCellIndex( id );
	RED_ASSERT( elem.Size() == 1 );
	RED_ASSERT( elem[ 0 ].m_Data );

	return elem[ 0 ].m_Data->InsertExistingObject( objectInfo );
}
#endif //USE_UMBRA_COOKING
#endif

void CUmbraScene::GenerateEditorFragments( CRenderFrame* frame )
{
	const CRenderFrameInfo& frameInfo = frame->GetFrameInfo();

	if ( frameInfo.IsShowFlagOn( SHOW_UmbraStreamingVisualization ) )
	{
		Int32 xCellCount = m_grid.GetCellCounts().X;
		Float xOffset = m_grid.GetOffsets().X;
		Float xCellSize = m_grid.GetCellSizes().X;

		Int32 yCellCount = m_grid.GetCellCounts().Y;
		Float yOffset = m_grid.GetOffsets().Y;
		Float yCellSize = m_grid.GetCellSizes().Y;
		Float zOffset = 25.0f;

		// Generate lines for the grid.
		for ( Int32 x = 0; x < xCellCount; x++ )
		{
			for ( Int32 y = 0; y < yCellCount; y++ )
			{
				VectorI id( x, y, 0, 0 );
				const TOcclusionGrid::ElementList& elements = m_grid.GetElementsByCellIndex( id );
				ASSERT( elements.Size() == 1 );

				CUmbraTile* tile = elements[0].m_Data.Get();
				if ( !tile )
				{
					continue;
				}

				if ( !tile->HasData() )
				{
					continue;
				}

				Color cellColor = Color::GRAY;
				if ( tile->GetNumberOfTomeCollections() > 0 )
				{
					cellColor = Color::GREEN;
				}
				else
				{
					if ( tile->IsInTomeCollectionJob() )
					{
						cellColor = Color::YELLOW;
					}
				}

				cellColor.A = 128;

				Float x1, x2, y1, y2;

				x1 = x * xCellSize + 0.25f + xOffset;
				x2 = ( x + 1 ) * xCellSize - 0.25f + xOffset;
				y1 = y * yCellSize + 0.25f + yOffset;
				y2 = ( y + 1 ) * yCellSize - 0.25f + yOffset;

				Vector v00( x1, y1, zOffset );
				Vector v01( x1, y2, zOffset );
				Vector v11( x2, y2, zOffset );
				Vector v10( x2, y1, zOffset );

				TDynArray< DebugVertex > vertices;
				vertices.PushBack( DebugVertex( v00, cellColor ) );
				vertices.PushBack( DebugVertex( v01, cellColor ) );
				vertices.PushBack( DebugVertex( v10, cellColor ) );
				vertices.PushBack( DebugVertex( v11, cellColor ) );

				// create 4 triangles, both sides
				TDynArray< Uint16 > indices;
				indices.PushBack(1);
				indices.PushBack(2);
				indices.PushBack(0);

				indices.PushBack(1);
				indices.PushBack(3);
				indices.PushBack(2);

				indices.PushBack(1);
				indices.PushBack(0);
				indices.PushBack(2);

				indices.PushBack(1);
				indices.PushBack(2);
				indices.PushBack(3);

				frame->AddDebugTriangles( Matrix::IDENTITY, vertices.TypedData(), vertices.Size(), indices.TypedData(), indices.Size(), cellColor, false, true );

				/*
				Int32 noOfTC = tile->GetNumberOfTomeCollections();
				const VectorI& coords = tile->GetCoordinates();
				Uint16 tileID = tile->GetId();
				String tileInfo = String::Printf( TXT("tile%d [%dx%d] -> [%d]"), tileID, coords.X, coords.Y, noOfTC );

				Vector pos = ( v00 + v01 + v11 + v10 ) / 4.0f;
				pos.Z += 10.0f;
				frame->AddDebugText( pos, tileInfo, 0, 0, true );
				*/
			}
		}

		Color job_Color = Color::GRAY;
		if ( m_tomeCollectionJob )
		{
			job_Color = Color::LIGHT_RED;
		}
		const Int32 job_width = 100;
		const Int32 job_height = 30;
		const Int32 job_x = ( frame->GetFrameOverlayInfo().m_width / 2 ) - ( job_width / 2 );
		const Int32 job_y = 50;
		frame->AddDebugRect( job_x, job_y, job_width, job_height, job_Color );
	}

	if ( frameInfo.IsShowFlagOn( SHOW_UmbraStatistics ) )
	{
		// Generate lines for the grid.
		const Float shrinkFactor = 5.0f;
		for ( Int32 x = 0; x < m_grid.GetCellCounts().X; x++ )
		for ( Int32 y = 0; y < m_grid.GetCellCounts().Y; y++ )
		{
			Box bbox = m_grid.GetBoundingBoxOfTile( VectorI( x, y, 0, 0 ) );
			// shrink a little bit
			bbox.Min.X += shrinkFactor;
			bbox.Min.Y += shrinkFactor;
			bbox.Max.X -= shrinkFactor;
			bbox.Max.Y -= shrinkFactor;
			frame->AddDebugBox( bbox, Matrix::IDENTITY, Color::BLUE );
		}

		Int32 y = 130;
		const Int32 x = frame->GetFrameOverlayInfo().m_width - 350;

		const Vector& camPos = frameInfo.m_camera.GetPosition();
		VectorI tID = m_grid.CalcCellIndexFromPosition( camPos );

		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Camera in Tile [%d; %d]"), tID.X, tID.Y ); y += 15;

		if ( !IsUsingOcclusionCulling() )
		{
			frame->AddDebugScreenFormatedText( x, y, Color::GRAY, TXT("Umbra: OFF") );
			return;
		}

		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Umbra: ON") ); y += 15;

		Uint32 numberOfLoadedTiles = 0;
		Uint32 tileDataSize = 0;

		TDynArray< THandle< CUmbraTile > >* currentTiles = nullptr;
		if ( m_ping )
		{
			currentTiles = &m_tomesForTomeCollectionGenerationPing;
		}
		else
		{
			currentTiles = &m_tomesForTomeCollectionGeneration;
		}

		String sloadedTiles = String::EMPTY;
		Uint32 currentTilesSize = currentTiles->Size();
		for ( Uint32 i = 0; i < currentTilesSize; ++i )
		{
			CUmbraTile* tile = (*currentTiles)[ i ].Get();
			RED_ASSERT( tile );
			if ( tile->GetNumberOfTomeCollections() > 0 )
			{
				tileDataSize += tile->GetOcclusionDataSize();
				++numberOfLoadedTiles;
				const VectorI& id = tile->GetCoordinates();
				sloadedTiles += String::Printf( TXT("[%d; %d]"), id.X, id.Y );
				if ( i != currentTilesSize - 1 )
				{
					sloadedTiles += TXT(", ");
				}
			}
		}

		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("number of loaded tiles: %d"), numberOfLoadedTiles );
		y += 15;
		String sTileDataSize = String::Printf( TXT("tile data size: %s"), String::FormatByteNumber( tileDataSize, TString<Char>::BNF_Precise ).AsChar() );
		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, sTileDataSize.AsChar() );
		y += 15;
		frame->AddDebugScreenFormatedText( x, y, Color::WHITE, sloadedTiles.AsChar() );
		y += 15;
	}
}

void CUmbraScene::Tick( IRenderScene* renderScene )
{
	PC_SCOPE_PIX( UmbraSceneTick )

#ifdef USE_ANSEL
	if ( !isAnselSessionActive && !isAnselTurningOn && !isAnselTurningOff )
#endif // USE_ANSEL
	{
		// Disable umbra occlusion based on areas (EP2 hack)
		// Gains 13ms on XB/PS4
		if( CResourceSimplexTree* tresholdAreas = m_localUmbraOccThresholdMul.Get() )	
		{	
			if ( tresholdAreas->Get().FindIDAtPoint( m_referencePosition.X, m_referencePosition.Y ) == 0 )
			{
				UseOcclusionCulling( false );
			}
			else
			{
				UseOcclusionCulling( true );
			}
		}
		else
		{
			UseOcclusionCulling( true );
		}
	}
	
	if ( m_referencePositionValid && m_tickState == TS_Idle )
	{
		UpdateTomeCollection( m_referencePosition, renderScene );
	}

	switch ( m_tickState )
	{
	case TS_Idle:
		{
			PC_SCOPE_PIX( Idle )
			// do nothing
		}
		break;

	case TS_StartLoadingTiles:
		{
			PC_SCOPE_PIX( StartLoadingTiles )
			RED_ASSERT( !m_tomeCollectionJob );
			if ( !m_tileSet.IsLoading() )
			{
				m_tileSet.Set( m_ping ? &m_tomesForTomeCollectionGenerationPing : &m_tomesForTomeCollectionGeneration );
				m_tickState = TS_LoadTiles;
			}
		}
		break;

	case TS_LoadTiles:
		{
			PC_SCOPE_PIX( LoadTiles )
			if ( !m_tileSet.IsLoading() )
			{
				// finished
				m_tickState = TS_StartTomeCollectionGeneration;
			}
		}
		break;

	case TS_StartTomeCollectionGeneration:
		{
			PC_SCOPE_PIX( StartTomeCollectionGeneration )
			RED_ASSERT( !m_tomeCollectionJob );
			TUmbraTileArray& tomesForJob = m_ping ? m_tomesForTomeCollectionGenerationPing : m_tomesForTomeCollectionGeneration;
			for ( auto& tome : tomesForJob )
			{
				if ( tome )
				{
					tome->SetInTomeCollectionJob( true );
				}
			}

			{
				PC_SCOPE_PIX( CreateTomeCollectionJob )
				m_tomeCollectionJob = new ( CTask::Root ) CJobCreateTomeCollection( tomesForJob, renderScene, m_ping ? m_tomeCollection : m_tomeCollectionPing );
				RED_ASSERT( m_tomeCollectionJob );
				GTaskManager->Issue( *m_tomeCollectionJob );
			}

			{
				PC_SCOPE_PIX( NotifyDataUploaded )
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_tomesNotificationAccessMutex );
				m_dataUploaded = false;
			}
			m_tickState = TS_GenerateTomeCollection;
		}
		break;

	case TS_GenerateTomeCollection:
		{
			PC_SCOPE_PIX( GenerateTomeCollection )
			RED_ASSERT( m_tomeCollectionJob );
			if ( m_tomeCollectionJob->IsFinished() )
			{
				if ( m_tomeCollectionJob->GetResult() )
				{
					m_tomeCollectionJob->ClearJobInfoFromTomes();

					if ( m_ping )
					{
						m_tomeCollectionPing = m_tomeCollectionJob->GetTomeCollectionWrapper();
					}
					else
					{
						m_tomeCollection = m_tomeCollectionJob->GetTomeCollectionWrapper();
					}
					RecreateOcclusionDataInRenderScene( m_tomeCollectionJob->GetRenderScene(), m_tomeCollectionJob->GetRemapTable(), m_tomeCollectionJob->GetObjectIdToIndexMap() );
				}
				else
				{
					RemoveUnusedTilesAndTomeCollection();

					// leave the old tome collection as the current one
					m_ping = !m_ping;
				}
				SAFE_RELEASE( m_tomeCollectionJob );
				m_tickState = TS_Idle;
			}
		}
		break;
	}
}


void CUmbraScene::TickUntilStateOrIdle( IRenderScene* renderScene, ETickState state, Float maxTime )
{
	if ( m_tickState == TS_Idle )
	{
		return;
	}

	CTimeCounter timer;
	while ( m_tickState != state && timer.GetTimePeriod() < maxTime )
	{
		ETickState preState = m_tickState;

		Tick( renderScene );

		// If we're stuck on the same tick state, sleep a bit. We're waiting for some async task.
		if ( preState == m_tickState )
		{
			PUMP_MESSAGES_DURANGO_CERTHACK();
			Red::Threads::SleepOnCurrentThread( 3 );
		}
	}

	if ( m_tickState != state )
	{
		RED_LOG_WARNING( Umbra, TXT("UmbraScene timed out waiting for state.") );
	}
}


Bool CUmbraScene::GenerateTomeCollection( TUmbraTileArray& tomes )
{
	PC_SCOPE_PIX( GenerateTomeCollection_UmbraTileArray )
	if ( tomes.Empty() )
	{
		return false;
	}

	if ( m_tickState != TS_Idle || m_tomeCollectionJob )
	{
		// TomeCollection generation in progress, let it finish
		return false;
	}

	{
		PC_SCOPE_PIX( NotifyDataUploaded )
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_tomesNotificationAccessMutex );
		if ( !m_dataUploaded )
		{
			return false;
		}
	}
	
	{
		PC_SCOPE_PIX( SortTomes )
		struct TomeDataSorter
		{
			RED_FORCE_INLINE Bool operator()( THandle< CUmbraTile >& td1, THandle< CUmbraTile >& td2 )  const { return td1->GetId() < td2->GetId(); }
		};
		Sort( tomes.Begin(), tomes.End(), TomeDataSorter() );
	}
	
	{
		PC_SCOPE_PIX( SetTomeArrays )
		// check for source data being the same
		if ( m_ping )
		{
			// if m_ping is enabled -> last set of tomes used for TomeCollectionGeneration was m_tomesForTomeCollectionGenerationPing
			// we have to test against it to determine the potential difference
			if ( tomes == m_tomesForTomeCollectionGenerationPing )
			{
				// set of tomes did not change
				return false;
			}
		}
		else
		{
			// if m_ping is disabled -> last set of tomes used for TomeCollectionGeneration was m_tomesForTomeCollectionGeneration
			// we have to test against it to determine the potential difference
			if ( tomes == m_tomesForTomeCollectionGeneration )
			{
				// set of tomes did not change
				return false;
			}
		}

		// flip m_ping to double-buffer the TomeCollection generation
		m_ping = !m_ping;

		// save the active set of Tomes and prepare the job for TomeCollection generation
		if ( m_ping )
		{
			m_tomesForTomeCollectionGenerationPing = tomes;
		}
		else
		{
			m_tomesForTomeCollectionGeneration = tomes;
		}
	}

	m_tickState = TS_StartLoadingTiles;
	return true;
}

Bool CUmbraScene::GenerateTomeCollection( const TOcclusionGrid::ElementList& elements )
{
	PC_SCOPE_PIX( GenerateTomeCollection_ElementList );
	TUmbraTileArray tomes;
	for ( Uint32 i = 0; i < elements.Size(); ++i )
	{
		RED_ASSERT( elements[ i ].m_Data );
		if ( elements[ i ].m_Data->HasData() )
		{
			tomes.PushBack( elements[ i ].m_Data.Get() );
		}
	}
	return GenerateTomeCollection( tomes );
}

Int32 CUmbraScene::GetDifferenceInTileCount( const Vector& position ) const
{
	const TUmbraTileArray& currentArray = m_ping ? m_tomesForTomeCollectionGenerationPing : m_tomesForTomeCollectionGeneration;
	
	TOcclusionGrid::ElementList elements;
	Float radius = m_grid.GetCellSizes().X * m_distanceMultiplier;
	m_grid.GetElementsAroundPosition( position, radius, elements );
	
	Int32 diff = 0;
	for ( const auto& element : elements )
	{
		RED_ASSERT( element.m_Data );
		if ( element.m_Data->HasData() && !currentArray.Exist( element.m_Data ) )
		{
			++diff;
		}
	}

	return diff;
}

void CUmbraScene::RecreateOcclusionDataInRenderScene( IRenderScene* renderScene, TVisibleChunksIndices& remapTable, TObjectIDToIndexMap& objectIDToIndexMap )
{
	PC_SCOPE_PIX( RecreateOcclusionDataInRenderScene )
	m_renderOcclusionData = GRender->UploadOcclusionData( this );
	RED_LOG( UmbraDebugging, TXT("Created RenderOcclusionData: [0x%x]"), m_renderOcclusionData );
	( new CRenderCommand_UploadOcclusionDataToScene( this, renderScene, m_renderOcclusionData, remapTable, objectIDToIndexMap ) )->Commit();
	m_renderOcclusionData->Release();
}

void CUmbraScene::RemoveUnusedTilesAndTomeCollection( Bool forceRemoveAll /*=false*/ )
{
	PC_SCOPE_PIX( RemoveUnusedTilesAndTomeCollection )
	if ( m_ping || forceRemoveAll )
	{
		PC_SCOPE_PIX( RemoveNonPing )
		for ( auto tile : m_tomesForTomeCollectionGeneration )
		{
			RED_ASSERT( tile );
			tile->ReleaseRefTome();
		}
		m_tomesForTomeCollectionGeneration.Clear();
		if ( m_tomeCollection )
		{
			delete m_tomeCollection;
			m_tomeCollection = nullptr;
		}
	}
	
	if ( !m_ping || forceRemoveAll )
	{
		PC_SCOPE_PIX( RemovePing )
		for ( auto tile : m_tomesForTomeCollectionGenerationPing )
		{
			RED_ASSERT( tile );
			tile->ReleaseRefTome();
		}
		m_tomesForTomeCollectionGenerationPing.Clear();
		if ( m_tomeCollectionPing )
		{
			delete m_tomeCollectionPing;
			m_tomeCollectionPing = nullptr;
		}
	}

	{
		PC_SCOPE_PIX( NotifyDataUploaded )
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( s_tomesNotificationAccessMutex );
		m_dataUploaded = true;
	}
}

void CUmbraScene::MergeObjectCache( const TTileObjectCache& objectCache )
{
	for ( auto it : objectCache )
	{
		const Bool insertResult = m_objectCache.Insert( it.m_first, it.m_second );
#ifndef RED_FINAL_BUILD
		if ( !insertResult )
		{
			const TObjectCacheKeyType& key = it.m_first;
			const TObjectIdType& objectIdToInsert = it.m_second;
			TObjectIdType existingObject;
			RED_VERIFY( m_objectCache.Find( key, existingObject ) );

			Uint32 meshId;
			Uint32 posHash;
			UmbraHelpers::DecompressFromKeyType( key, meshId, posHash );

			Uint16 existingTileId;
			Uint32 existingObjectInTile;
			UmbraHelpers::DecompressObjectId( existingObject, existingTileId, existingObjectInTile );

			Uint16 newTileId;
			Uint32 newObjectInTile;
			UmbraHelpers::DecompressObjectId( objectIdToInsert, newTileId, newObjectInTile );

			RED_LOG_ERROR( UmbraError, TXT("Umbra cache key collision, key: %") RED_PRIWu64 TXT(" [%u; %u]"), key, meshId, posHash );
			RED_LOG_ERROR( UmbraError, TXT("\tobjectToInsert: %u [tileID: %u; objectInTile: %u]"), objectIdToInsert, newTileId, newObjectInTile );
			RED_LOG_ERROR( UmbraError, TXT("\texistingObject: %u [tileID: %u; objectInTile: %u]"), existingObject, existingTileId, existingObjectInTile );
		}
#endif // RED_FINAL_BUILD
	}
}

void CUmbraScene::RemoveFromObjectCache( const TTileObjectCache& objectCache )
{
	for ( auto it = objectCache.Begin(), end = objectCache.End(); it != end; ++it )
	{
		m_objectCache.Erase( it->m_first );
	}
}

Bool CUmbraScene::UpdateTomeCollection( const Vector& position, IRenderScene* renderScene )
{
	PC_SCOPE_PIX( UpdateTomeCollection );

	Bool cameraPositionInvalid = Red::Math::NumericalUtils::IsNan( position.W )
							  || Red::Math::NumericalUtils::IsNan( position.Z )
							  || Red::Math::NumericalUtils::IsNan( position.Y )
							  || Red::Math::NumericalUtils::IsNan( position.X )
							  || !Red::Math::NumericalUtils::IsFinite( position.W )
							  || !Red::Math::NumericalUtils::IsFinite( position.Z )
							  || !Red::Math::NumericalUtils::IsFinite( position.Y )
							  || !Red::Math::NumericalUtils::IsFinite( position.X );

	if ( cameraPositionInvalid )
	{
		// exit early to avoid crash
		return false;
	}
	
	{
		PC_SCOPE_PIX( GetElementsAroundPosition );
		m_elements.ClearFast();
		Float radius = m_grid.GetCellSizes().X * m_distanceMultiplier;
		m_grid.GetElementsAroundPosition( position, radius, m_elements );
		RED_ASSERT( m_elements.Size() );
	}
	
	// verify elements
	Bool dataIsValid = true;
	static Bool setInvalidMessage = true;
	{
		PC_SCOPE_PIX( Validation );
		for ( Uint32 i = 0; i < m_elements.Size(); ++i )
		{
			CUmbraTile* tile = m_elements[ i ].m_Data.Get();
			if ( !tile || ( tile->GetStatus() == EUTDS_Invalid ) )
			{
				if ( setInvalidMessage )
				{
					// log error only once
					RED_LOG_ERROR( UmbraError, TXT("Umbra: data from tile [%d; %d] is CORRUPTED. Umbra will be turned OFF, expect some performance problems."), tile->GetCoordinates().X, tile->GetCoordinates().Y );
					setInvalidMessage = false;
				}
				dataIsValid = false;
				break;
			}
		}
	}

	// inform render side
	if ( renderScene )
	{
		PC_SCOPE_PIX( RenderCommand_SetValidityOfOcclusionData );
		( new CRenderCommand_SetValidityOfOcclusionData( renderScene, dataIsValid ) )->Commit();
	}
	if ( !dataIsValid )
	{
		return false;
	}
	setInvalidMessage = true;
	return GenerateTomeCollection( m_elements );
}

VectorI CUmbraScene::GetTileIdFromPosition( const Vector& position ) const
{
	VectorI cellIndex = m_grid.CalcCellIndexFromPosition( position );
	cellIndex.Z = 0;
	cellIndex.W = 0;
	return cellIndex;
}

Uint16 CUmbraScene::GetUniqueTileIdFromPosition( const Vector& position )
{
	VectorI cellIndex = m_grid.CalcCellIndexFromPosition( position );
	cellIndex.Z = 0;
	cellIndex.W = 0;
	return m_grid.GetUniqueTileID( cellIndex );
}

void CUmbraScene::CleanupActiveTomes( IRenderScene* renderScene )
{
	if ( m_tomeCollectionJob )
	{
		if ( m_tomeCollectionJob->MarkRunning() )
		{
			// task has not been started yet, it's a perfect situation since we do not need its results anyway
		}
		else
		{
			// already started, mark as cancelled (can shorten the time of job execution) and then wait until it finishes
			m_tomeCollectionJob->MarkCancelled();
			while ( !m_tomeCollectionJob->IsFinished() )
			{
				RED_BUSY_WAIT();
				// idle until finish
			}
		}
		m_tomeCollectionJob->RemoveTomeCollection();
		m_tomeCollectionJob->ClearJobInfoFromTomes();
		SAFE_RELEASE( m_tomeCollectionJob );
	}

	// inform scene that we are once again able to recreate TomeCollection
	m_tickState = TS_Idle;
	if ( renderScene )
	{
		( new CRenderCommand_ClearOcclusionData( this, renderScene ) )->Commit();
	}
}

void CUmbraScene::SetReferencePosition( const Vector& position )
{
	m_referencePosition = position;
	m_referencePositionValid = true;
}

Bool CUmbraScene::FindInCache( const TObjectCacheKeyType& key, TObjectIdType& objectId )
{
	return m_objectCache.Find( key, objectId );
}

#endif // USE_UMBRA
