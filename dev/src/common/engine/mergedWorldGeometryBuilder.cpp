/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "layer.h"
#include "world.h"
#include "mergedMeshComponent.h"
#include "mergedWorldGeometry.h"
#include "mergedWorldGeometryEntity.h"
#include "mergedWorldGeometryBuilder.h"
#include "../core/feedback.h"
#include "../core/objectGC.h"

#ifndef NO_RESOURCE_IMPORT

namespace MergedWorldGeometryBuilder
{
	const Bool Build( CLayer* containerLayer, CDirectory* contentDir, const Uint32 gridSize, const TDynArray< IMergedWorldGeometryData* >& mergers, const IMergedWorldGeometrySupplier* worldDataSupplier, const Vector& worldCenter, const Float worldRadius )
	{
		// No geometry supplier
		if ( !worldDataSupplier )
			return false;

		// Get world
		CWorld* world = containerLayer->GetWorld();
		if ( !world )
			return false;

		// Container layer is not attached
		if ( !containerLayer )
			return false;

		// Detach layer
		const Bool isAttached = containerLayer->IsAttached();
		if ( isAttached )
		{
			containerLayer->DetachFromWorld();
		}

		// Compute world bounds
		const Float worldSize = worldDataSupplier->GetWorldSize();
		const Float worldBase = -(worldSize / 2.0f);
		LOG_ENGINE( TXT("World size: %f meters"), worldSize );

		/// Compute world bounds
		const Int32 numGridCells = ((Int32)worldSize + (gridSize-1)) / gridSize;
		LOG_ENGINE( TXT("Grid cells per axis: %d"), numGridCells );
		LOG_ENGINE( TXT("Grid cells in world: %d"), numGridCells*numGridCells );

		/// Map existing grid
		TSortedMap< CMergedWorldGeometryGridCoordinates, THandle< CMergedWorldGeometryEntity > > gridEntities;
		{
			TDynArray< CEntity* > layerEntities;
			containerLayer->GetEntities( layerEntities );

			for ( THandle< CEntity > layerEntity : layerEntities )
			{
				THandle< CMergedWorldGeometryEntity > gridEntity = Cast< CMergedWorldGeometryEntity >( layerEntity );
				if ( gridEntity )
				{
					RED_ASSERT( gridEntity->GetGridCoordinates().IsValid(), TXT("Grid entity has invalid grid coordinates") );
					RED_ASSERT( !gridEntities.KeyExist( gridEntity->GetGridCoordinates() ), TXT("Dupliated grid entity") );

					gridEntities.Insert( gridEntity->GetGridCoordinates(), gridEntity );
				}
			}
		}
		LOG_ENGINE( TXT("Found %d existing grid entities"), gridEntities.Size() );

		/// Process grid areas
		Bool state = true;
		GFeedback->UpdateTaskName( TXT("Processing world grid") );
		for ( Int32 gridY = 0; (gridY < numGridCells) && state && !GFeedback->IsTaskCanceled(); ++gridY )
		{
			Bool workDone = false;
			for ( Int32 gridX = 0; (gridX < numGridCells) && state && !GFeedback->IsTaskCanceled(); ++gridX )
			{
				/// compute grid area
				Box worldBounds;
				worldBounds.Min.X = worldBase + (gridX * gridSize);
				worldBounds.Min.Y = worldBase + (gridY * gridSize);
				worldBounds.Min.Z = -10000.0f; // hardcoded, I would like to avoid using FLT_MAX here
				worldBounds.Max.X = worldBounds.Min.X + gridSize;
				worldBounds.Max.Y = worldBounds.Min.Y + gridSize;
				worldBounds.Max.Z = 10000.0f; // hardcoded, I would like to avoid using FLT_MAX here

				/// Is this grid within the build area ?
				if ( worldBounds.CalcCenter().DistanceTo2D( worldCenter ) > worldRadius )
					continue;

				// Update progress stats
				GFeedback->UpdateTaskInfo( TXT("Processing grid [%d,%d]"), gridX, gridY );
				GFeedback->UpdateTaskProgress( gridX + gridY*numGridCells, numGridCells*numGridCells );

				/// Get all entities from area
				TDynArray< THandle< CEntity > > entities;
				worldDataSupplier->GetEntitiesFromArea( worldBounds, entities );

				/// Let's have a list of components for each merger
				TDynArray< TDynArray< THandle< CComponent > > > componentsToMerge;
				componentsToMerge.Resize( mergers.Size() );

				/// get stuff to merge for each data merger
				Uint32 numComponentsCollected = 0;
				for ( Uint32 mergerIndex = 0; mergerIndex < mergers.Size(); ++mergerIndex )
				{
					IMergedWorldGeometryData* merger = mergers[mergerIndex];

					for ( THandle< CEntity > entity : entities )
					{
						for ( THandle< CComponent > comp : entity->GetComponents() )
						{
							if ( merger->CanMerge( comp ) )
							{
								componentsToMerge[ mergerIndex ].PushBack( comp );
								numComponentsCollected += 1;
							}
						}
					}
				}

				/// there's no data for this grid cell
				if ( !numComponentsCollected )
					continue;

				/// stats
				LOG_ENGINE( TXT("Found %d mergable components in grid [%d,%d]"), numComponentsCollected, gridX, gridY );

				// destroy old entity and the old data with it
				const CMergedWorldGeometryGridCoordinates coordinates( gridX, gridY );
				THandle< CMergedWorldGeometryEntity > gridEntity;
				if ( gridEntities.Find( coordinates, gridEntity ) )
				{
					gridEntity->Destroy();
					gridEntities.Erase( coordinates );
				}

				// calculate entity position - use the center of the grid box projected on the terrain to create a stable and deterministic reference
				const Vector gridCellCenter = worldBounds.CalcCenter();
				const Vector gridReference = worldDataSupplier->ProjectOnTerrain( gridCellCenter );

				// create new entity
				EntitySpawnInfo spawnInfo;
				spawnInfo.m_entityClass = ClassID< CMergedWorldGeometryEntity >();
				spawnInfo.m_spawnPosition = gridReference;
				spawnInfo.m_name = String::Printf( TXT("grid_%d_%d"), gridX, gridY );
				gridEntity = Cast< CMergedWorldGeometryEntity >( containerLayer->CreateEntitySync( spawnInfo ) );
				RED_ASSERT( gridEntity, TXT("Unable to create grid entity for area") );
				if ( !gridEntity )
					continue;

				// setup grid entity
				gridEntity->SetPosition( gridReference );
				gridEntity->Setup( 0, worldBounds, coordinates );
				gridEntity->ForceUpdateTransformNodeAndCommitChanges();

				TDynArray<String> outCorruptedMeshDepotPaths;

				// merge data
				for ( Uint32 mergerIndex = 0; mergerIndex < mergers.Size(); ++mergerIndex )
				{
					IMergedWorldGeometryData* merger = mergers[mergerIndex];
					if ( !merger->Merge( contentDir, gridEntity, componentsToMerge[mergerIndex], outCorruptedMeshDepotPaths ) )
					{
						if ( !GFeedback->AskYesNo( TXT("Failed to merge content for grid cell [%d,%d]. The data will be corrupted. Continue (will not ask again) ?"), gridX, gridY ) )
						{
							state = false;
							break;
						}
					}
				}

				// found corrupted meshes on the way
				if( outCorruptedMeshDepotPaths.Size()>0 )
				{
					const String hackLogFilename = TXT("corruptedMeshLog.txt");
					LOG_ENGINE( TXT("Found %d corrupted meshes! PLEASE FIX, mesh list saved in bin//%ls"), outCorruptedMeshDepotPaths.Size(), hackLogFilename.AsChar() );							

					if( GFileManager->FileExist(hackLogFilename) ) GFileManager->DeleteFileW(hackLogFilename);
					for(Uint32 i=0; i< outCorruptedMeshDepotPaths.Size(); ++i) GFileManager->SaveStringToFile( hackLogFilename, outCorruptedMeshDepotPaths[i], true );
				}

				// generate streaming data for entity
				gridEntity->SetStreamed( true );
				gridEntity->UpdateStreamedComponentDataBuffers(true);
				gridEntity->UpdateStreamingDistance();

				// unstream the entity, saving memory
				gridEntity->DestroyStreamedComponents( SWN_DoNotNotifyWorld );

				// we did some work on this cell
				workDone = true;
			}

			// cleanup temporary objects that may have been created
			if ( workDone )
			{
				GObjectGC->CollectNow();
			}
		}

		// Reattach layer back to world
		if ( isAttached )
		{
			containerLayer->AttachToWorld( world );
		}

		// Return final state
		return state;
	}

} // MergedWorldGeometryBuilder

#endif