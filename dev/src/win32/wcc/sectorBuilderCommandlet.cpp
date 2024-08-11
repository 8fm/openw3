/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sectorBuilderCommandlet.h"
#include "resourceGrid.h"
#include "resourceGridOptimizer.h"
#include "manualBundlesReader.h"
#include "patchUtils.h"

#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layer.h"

#include "../../common/engine/sectorData.h"
#include "../../common/engine/sectorPrefetchGrid.h"

IMPLEMENT_ENGINE_CLASS( CSectorDataBuilderCommandlet );

CSectorDataBuilderCommandlet::CSectorDataBuilderCommandlet()
{
	m_commandletName = CName( TXT("buildsectors") );
}

CSectorDataBuilderCommandlet::~CSectorDataBuilderCommandlet()
{
}

namespace Helper
{
	CObject* LoadTempObject( IFile* file )
	{
		// setup
		DependencyLoadingContext loadingContext;
		loadingContext.m_doNotLoadImports = true; // to save some time, not needed for this shit

		// load stuff
		CDependencyLoader loader( *file, nullptr );
		if ( !loader.LoadObjects( loadingContext ) )
			return nullptr;

		// no object loaded
		if ( loadingContext.m_loadedRootObjects.Empty() )
			return nullptr;

		// extract loaded object
		return loadingContext.m_loadedRootObjects.Front();
	}

	CObject* LoadTempObject( CManualBundleReader& bundles, const String& depotPath )
	{
		IFile* file = bundles.CreateReader( depotPath );
		if ( !file )
			return nullptr;

		CObject* ret = LoadTempObject( file );
		delete file;

		return ret;
	}

	String GetResourcePath( const ResourceGrid::CResourceRef* res )
	{
		const String& orgPath = res->GetDepotPath();

		if ( orgPath.EndsWith( TXT("w2mesh") ) )
		{
			return orgPath + TXT(".1.buffer");
		}

		return orgPath;
	}

	Bool LoadCoordinateListFile( const String& filePath, TDynArray< Box >& boxes )
	{
		boxes.Clear( );

		String coordListContents = String::EMPTY;
		if( !GFileManager->LoadFileToString( filePath, coordListContents, true ) )
		{
			ERR_WCC( TXT( "Error reading coordinate list file '%ls'!" ), filePath.AsChar( ) );
			return false;
		}

		TDynArray< String > coordListTokens;
		coordListContents.GetTokens( ' ', false, coordListTokens );

		if( coordListTokens.Empty( ) )
		{
			ERR_WCC( TXT( "The coordinate list is empty!" ) );
			return false;
		}

		for( Uint32 tokenIdx = 0; tokenIdx < coordListTokens.Size( ); ++tokenIdx )
		{
			if( coordListTokens[ tokenIdx ].EqualsNC( TXT( "CIRCLE" ) ) )
			{
				if( coordListTokens.Size( ) - tokenIdx < 4 )
				{
					WARN_WCC( TXT( "Invalid CIRCLE specified! Expected 'CIRCLE x y radius'. Ignoring.." ) );
					continue;
				}

				const Float circleX = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 1 ].AsChar( ) ) ) );
				const Float circleY = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 2 ].AsChar( ) ) ) );
				const Float circleR = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 3 ].AsChar( ) ) ) );

				boxes.PushBack( Box( Vector( circleX, circleY, 0.0f ), circleR ) );
				tokenIdx += 3;
			}
			else if( coordListTokens[ tokenIdx ].EqualsNC( TXT( "BOX" ) ) )
			{
				if( coordListTokens.Size( ) - tokenIdx < 5 )
				{
					WARN_WCC( TXT( "Invalid BOX specified! Expected 'BOX minx miny maxx maxy'. Ignoring.." ) );
					continue;
				}

				const Float p0x = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 1 ].AsChar( ) ) ) );
				const Float p0y = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 2 ].AsChar( ) ) ) );
				const Float p1x = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 3 ].AsChar( ) ) ) );
				const Float p1y = static_cast< Float >( atof( UNICODE_TO_ANSI( coordListTokens[ tokenIdx + 4 ].AsChar( ) ) ) );

				Box newBox( Box::RESET_STATE );
				newBox.AddPoint( Vector( p0x, p0y, 0.0f ) );
				newBox.AddPoint( Vector( p1x, p1y, 0.0f ) );

				boxes.PushBack( newBox );
				tokenIdx += 4;
			}
		}

		ERR_WCC( TXT( "Found %d entries in the coordinate list." ), boxes.Size( ) );

		return !boxes.Empty( );
	}
}

bool CSectorDataBuilderCommandlet::Execute( const CommandletOptions& options )
{
	// allocate temp block
	if ( !PatchUtils::AllocateTempMemory() )
	{
		ERR_WCC( TXT("Not enough memory to build sector data. Provide machine with bigger buffer.") );
		return false;
	}

	// get path to the world file
	String worldPath;
	if ( !options.GetSingleOptionValue( TXT("world"), worldPath ) && !options.GetSingleOptionValue( TXT("level"), worldPath ) )
	{
		ERR_WCC( TXT("Missing path to the world") );
		return false;
	}

	// get output file path (usually streaming.cache)
	String absoluteOutPath;
	if ( !options.GetSingleOptionValue( TXT("out"), absoluteOutPath ) )
	{
		ERR_WCC( TXT("Missing output data path") );
		return false;
	}

	// get path to bundles
	if ( !options.HasOption( TXT("bundles") ) )
	{
		ERR_WCC( TXT("Missing path to bundles") );
		return false;
	}

	TDynArray< Box > coordBoxes;

	// load list of coordinates if any
	String coordListPath;
	if( options.GetSingleOptionValue( TXT( "coordlist" ), coordListPath ) )
	{
		if( !Helper::LoadCoordinateListFile( coordListPath, coordBoxes ) )
		{
			ERR_WCC( TXT( "Failed to load coordinate list from '%ls'!" ), coordListPath.AsChar( ) );
			return false;
		}
	}

	// load bundles
	CManualBundleReader loadedBundles;
	auto list = options.GetOptionValues( TXT("bundles") );
	for ( auto it = list.Begin(); it != list.End(); ++it )
	{
		loadedBundles.AddBundles( *it );
	}

	// load world object
	CWorld* world = Cast< CWorld >( Helper::LoadTempObject( loadedBundles, worldPath ) );
	if ( !world || !world->GetWorldLayers() )
	{
		ERR_WCC( TXT("Unable to load world from '%ls'"), worldPath.AsChar() );
		return false;
	}

	// estimate world size
	const Float worldSize = world->GetWorldDimensions();
	LOG_WCC( TXT("World size: %f"), worldSize );

	// save the initial header
	Red::TScopedPtr< IFile > outputFile( GFileManager->CreateFileWriter( absoluteOutPath, FOF_AbsolutePath | FOF_Buffered ) );
	if ( !outputFile )
	{
		ERR_WCC( TXT("Unable to save output file '%ls'"), absoluteOutPath.AsChar() );
		return false;
	}

	// create grid representation to fill
	const Float minCell = 32.0f;
	ResourceGrid::CGrid grid( worldSize, minCell );

	// process layers
	TDynArray< CLayerInfo* > allLayers;
	world->GetWorldLayers()->GetLayers( allLayers, false );
	Uint32 layersWithSectorData = 0;
	LOG_WCC( TXT("Found %d layers in the world"), allLayers.Size() );
	for ( Uint32 i=0; i<allLayers.Size(); ++i )
	{
		CLayerInfo* info = allLayers[i];

		// global stats
		LOG_WCC( TXT("Status: Processing layer %d/%d... %ls.."), i, allLayers.Size(), info->GetDepotPath().AsChar() );

		// load the layer data
		CLayer* layer = Cast< CLayer >( Helper::LoadTempObject( loadedBundles, info->GetDepotPath() ) );
		if ( !layer )
		{
			ERR_WCC( TXT("Failed to load layer '%ls'"), info->GetDepotPath().AsChar() );
			continue;
		}

		// process layer sector data
		const CSectorData* sectorData = layer->GetSectorData();
		if ( sectorData )
		{
			// add resources
			TDynArray< ResourceGrid::CResourceRef* > mappedResources;
			mappedResources.Resize( sectorData->GetNumResources() );
			for ( Uint32 j=0; j<sectorData->GetNumResources(); ++j )
			{
				const Uint64 pathHash = sectorData->GetResourcePathHash( j );

				// resolve path
				String resolvedPath;
				ResourceGrid::CResourceRef* res = nullptr;
				if ( loadedBundles.ResolvePathHash( pathHash, resolvedPath ) )
				{
					res = grid.GetResource( resolvedPath );
				}
				else if ( pathHash )
				{
					ERR_WCC( TXT("Failed to resolve resource from hash 0x%016llX"), pathHash );
				}

				// map
				mappedResources[j] = res;
			}

			// process object and extract referenced resources with their positions
			const Uint32 numObjects = sectorData->GetNumObjects();
			for ( Uint32 j=0; j<numObjects; ++j )
			{
				const auto& obj = sectorData->GetObject(j);
				if ( obj.m_type == SectorData::eObject_Mesh )
				{
					const Int32 usedResource = sectorData->GetResourceIndexForObject( j );
					if ( usedResource != -1 )
					{
						RED_FATAL_ASSERT( usedResource < mappedResources.SizeInt(), "Invalid resource index" );
						ResourceGrid::CResourceRef* res = mappedResources[ usedResource ];
						if ( res )
						{
							const Float extendedRadius = (Float) obj.m_radius + 10.0f;

							// check if the object needs to fit in specific areas
							if( !coordBoxes.Empty() )
							{
								Box objBox( obj.m_pos, extendedRadius );

								Bool filteredOut = true;
								for( const Box& b : coordBoxes )
								{
									if( b.Touches2D( objBox ) )
									{
										filteredOut = false;
										break;
									}
								}

								// is not part of any valid area, ignore it
								if( filteredOut )
									continue;
							}

							grid.AddResourceReference( res, obj.m_pos, extendedRadius );
						}
					}
				}
			}

			// count number of sector data blocks that were processed
			layersWithSectorData += 1;
		}

		// delete the layer
		layer->Discard();
	}

	// stats
	LOG_WCC( TXT("Processed %d sectors"), layersWithSectorData );
	LOG_WCC( TXT("Found %d resources"), grid.GetAllResources().Size() );

	// top used resources
	{
		auto res = grid.GetAllResources();
		Sort( res.Begin(), res.End(), [](const ResourceGrid::CResourceRef* a, const ResourceGrid::CResourceRef* b) { return a->GetNumUniqueRefs() > b->GetNumUniqueRefs(); } );

		LOG_WCC( TXT("Top referenced resources (total):") );
		for ( Uint32 i=0; i<res.Size(); ++i )
		{
			LOG_WCC( TXT("Unique[%d]: %d refs: '%ls'"), 
				i, res[i]->GetNumUniqueRefs(), res[i]->GetDepotPath().AsChar() );
		}
	}

	// top used resources
	{
		auto res = grid.GetAllResources();
		Sort( res.Begin(), res.End(), [](const ResourceGrid::CResourceRef* a, const ResourceGrid::CResourceRef* b) { return a->GetNumSharedRefs() > b->GetNumSharedRefs(); } );

		LOG_WCC( TXT("Top referenced resources (shared in sectors):") );
		for ( Uint32 i=0; i<res.Size(); ++i )
		{
			LOG_WCC( TXT("Shared[%d]: %d refs: '%ls'"), 
				i, res[i]->GetNumSharedRefs(), res[i]->GetDepotPath().AsChar() );
		}
	}

	// final sectors to pack
	struct SectorInfo
	{
		const ResourceGrid::CGridCell*	m_orgCell;
		Uint32							m_numResources;
		Uint32							m_numResourcesWithParents;

		Uint32							m_dataSize;
		Uint32							m_dataSizeWithParents;
	};

	// optimize grid
	{
		ResourceGrid::COptimizer optimizer;
		optimizer.OptimizeGrid( grid );
	}

	// post stats
	TDynArray< SectorInfo > finalSectors;
	{
		// extract cells with data
		TDynArray< const ResourceGrid::CGridCell* > gridCells;
		grid.GetFilledCells( gridCells );
		LOG_WCC( TXT("Found %d final cells with resources"), gridCells.Size() );

		// calculate size of data for each sector
		THashMap< const ResourceGrid::CGridCell*, Uint32 > dataSizePerCell;
		THashMap< const ResourceGrid::CGridCell*, Uint32 > numResourcesPerCell;
		for ( const auto* cell : gridCells )
		{
			SectorInfo sectorInfo;
			sectorInfo.m_orgCell = cell;
			sectorInfo.m_numResources = 0;
			sectorInfo.m_numResourcesWithParents = 0;
			sectorInfo.m_dataSize = 0;
			sectorInfo.m_dataSizeWithParents = 0;

			for ( const auto& res : cell->GetResources() )
			{
				const String resPath = Helper::GetResourcePath( res );
				const Uint32 dataSize = loadedBundles.GetCompressedFileSize( resPath );
				if ( dataSize == 0 )
				{
					ERR_WCC( TXT("No resource file '%ls' found"), resPath.AsChar() );
				}
				else
				{
					sectorInfo.m_dataSize += dataSize;
					sectorInfo.m_numResources += 1;
				}
			}

			// only add sector if it's worth it
			if ( sectorInfo.m_numResources > 10 && sectorInfo.m_dataSize > 100*1024 )
			{
				finalSectors.PushBack( sectorInfo );
			}

			// count sector data size
			dataSizePerCell.Insert( cell, sectorInfo.m_dataSize );
			numResourcesPerCell.Insert( cell, sectorInfo.m_numResources);
		}

		// calculate total size with roots
		for ( auto& sector : finalSectors )
		{
			ResourceGrid::CGridCell* cell = (ResourceGrid::CGridCell*) sector.m_orgCell;
			while ( cell )
			{
				Uint32 dataSizeInCell = 0;
				dataSizePerCell.Find( cell, dataSizeInCell );

				Uint32 numResources = 0;
				numResourcesPerCell.Find( cell, numResources );

				sector.m_dataSizeWithParents += dataSizeInCell;
				sector.m_numResourcesWithParents += numResources;
				cell = cell->GetParent();
			}
		}

		// sort by data size
		{
			auto temp = finalSectors;
			Sort( temp.Begin(), temp.End(), []( const SectorInfo& a, const SectorInfo& b ) { return a.m_dataSize > b.m_dataSize; } );

			Uint64 totalSectorDataSize = 0;

			LOG_WCC( TXT("Largest %d sectors"), temp.Size() );
			for ( Uint32 i=0; i<temp.Size(); ++i )
			{
				const auto& sector = temp[i];

				LOG_WCC( TXT("PreSector[%d]: [%d,%d], level %d. %d Resources, %1.2fMB"), 
					i,
					sector.m_orgCell->GetGridX(), sector.m_orgCell->GetGridY(), sector.m_orgCell->GetLevel()->GetLevelIndex(),
					sector.m_numResources,
					sector.m_dataSize / (1024.0f*1024.0f) );

				totalSectorDataSize += (Uint64) sector.m_dataSize;
			}

			LOG_WCC( TXT("Total preload data size: %1.2f MB"), totalSectorDataSize / (1024.0f*1024.0f) );
		}

		// sort by data size with parent nodes
		{
			auto temp = finalSectors;
			Sort( temp.Begin(), temp.End(), []( const SectorInfo& a, const SectorInfo& b ) { return a.m_dataSizeWithParents > b.m_dataSizeWithParents; } );

			LOG_WCC( TXT("Largest %d sectors with parents"), temp.Size() );
			for ( Uint32 i=0; i<temp.Size(); ++i )
			{
				const auto& sector = temp[i];

				LOG_WCC( TXT("Total[%d]: [%d,%d], level %d. %d Resources, %1.2fMB"), 
					i,
					sector.m_orgCell->GetGridX(), sector.m_orgCell->GetGridY(), sector.m_orgCell->GetLevel()->GetLevelIndex(),
					sector.m_numResourcesWithParents,
					sector.m_dataSizeWithParents / (1024.0f*1024.0f) );
			}
		}
	}

	// prepare output file
	{
		CSectorPrefetchGrid gridData;
		gridData.m_worldSize = worldSize;

		// prepare cells
		TDynArray< String > resourcesToSave;
		for ( auto& sector : finalSectors )
		{
			CSectorPrefetchGrid::GridCell savedCell;
			savedCell.m_level = sector.m_orgCell->GetLevel()->GetLevelIndex();
			savedCell.m_cellX = sector.m_orgCell->GetGridX();
			savedCell.m_cellY = sector.m_orgCell->GetGridY();

			// setup resources
			const Uint16 cellIndex = (Uint16) gridData.m_cells.Size();
			savedCell.m_firstEntry = gridData.m_entries.Size();
			for ( const auto* res : sector.m_orgCell->GetResources() )
			{
				const String dataPath = Helper::GetResourcePath( res ); // get the data holder for resource
				const Uint32 dataSize = loadedBundles.GetCompressedFileSize( dataPath );
				if ( dataSize != 0 )
				{
					CSectorPrefetchGrid::GridCellEntry savedEntry;
					savedEntry.m_compressionType = (Uint16) loadedBundles.GetCompressionType( dataPath );
					savedEntry.m_uncompressedSize = loadedBundles.GetMemoryFileSize( dataPath );
					savedEntry.m_dataOffset = 0;
					savedEntry.m_dataSize = dataSize;
					savedEntry.m_cellIndex = cellIndex;
					savedEntry.m_resourceHash = Red::CalculatePathHash64( res->GetDepotPath().AsChar() ); // path is always from the true resource

					// add entry
					gridData.m_entries.PushBack( savedEntry );
					resourcesToSave.PushBack( dataPath );
				}
			}
			savedCell.m_numEntries = gridData.m_entries.Size() - savedCell.m_firstEntry;

			// not yet known
			savedCell.m_dataSize = 0;
			savedCell.m_dataOffset = 0;
			gridData.m_cells.PushBack( savedCell );
		}

		// save the initial headers
		outputFile->Seek( 0 );
		gridData.Save( outputFile.Get() );

		// store cell data
		Uint32 numEntriesStored = 0;
		for ( auto& savedCell : gridData.m_cells )
		{
			// status
			LOG_WCC( TXT("Status: Saving entry %d/%d..."), numEntriesStored++, gridData.m_cells.Size() );

			// remember start offset
			savedCell.m_dataOffset = outputFile->GetOffset();

			// store data for each entry
			for ( Uint32 localEntryIndex = 0; localEntryIndex<savedCell.m_numEntries; ++localEntryIndex )
			{
				// get entry
				const Uint32 entryIndex = savedCell.m_firstEntry + localEntryIndex;
				auto& savedEntry = gridData.m_entries[ entryIndex ];

				// get entry offset in the cell
				const Uint64 entryStartOffset = outputFile->GetOffset();
				savedEntry.m_dataOffset = (Uint32)( outputFile->GetOffset() - savedCell.m_dataOffset );

				// save the data
				const String& sourceDataPath = resourcesToSave[ entryIndex ];
				if ( savedEntry.m_compressionType != Red::Core::Bundle::CT_Doboz )
				{
					// recompress
					Red::TScopedPtr< IFile > sourceFile( loadedBundles.CreateReader( sourceDataPath ) );
					if ( sourceFile.Get() )
					{
						savedEntry.m_compressionType = Red::Core::Bundle::CT_Doboz; // override compression
						PatchUtils::CompressFileContentDoboz( *sourceFile, *outputFile );
					}
				}
				else
				{
					// direct copy
					Red::TScopedPtr< IFile > sourceFile( loadedBundles.CreateRawReader( sourceDataPath ) );
					if ( sourceFile.Get() )
					{
						PatchUtils::CopyFileContent( *sourceFile, *outputFile );
					}
				}

				// get the size of the entry data
				savedEntry.m_dataSize = (Uint32)( outputFile->GetOffset() - entryStartOffset );
			}

			// remember data size
			savedCell.m_dataSize = (Uint32)( outputFile->GetOffset() - savedCell.m_dataOffset );
			LOG_WCC( TXT("Saved cell [%d,%d], level %d @ %lld, SIZE: %d"), 
				savedCell.m_cellX, savedCell.m_cellY, savedCell.m_level,
				savedCell.m_dataOffset, savedCell.m_dataSize ); 
		}

		// resave header
		outputFile->Seek( 0 );
		gridData.Save( outputFile.Get() );

		// close file
		outputFile.Reset(nullptr);
	}

	// done
	return true;
}

void CSectorDataBuilderCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  buildsectors -bundles=<path> -world=<worldfilepath> -out=<path>" ) );
}
