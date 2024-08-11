/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "testMemCommandlet.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/world.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"

IMPLEMENT_ENGINE_CLASS( CTestMemCommandlet )

//----

CTestMemCommandlet::Settings::Settings()
	: m_worldExtents( 4096.0f )
	, m_numTestPoints( 10 )
	, m_numIterations( 3 )
{
}

Bool CTestMemCommandlet::Settings::Parse( const CommandletOptions& options )
{
	String text;

	if ( options.GetSingleOptionValue( TXT("numpoints"), text ) )
	{
		if ( !FromString<Uint32>( text, m_numTestPoints ) )
			return false;
	}

	if ( options.GetSingleOptionValue( TXT("numiter"), text ) )
	{
		if ( !FromString<Uint32>( text, m_numIterations ) )
			return false;
	}

	if ( options.GetSingleOptionValue( TXT("world"), text ) )
	{
		m_worldPath = text;
	}
	else
	{
		ERR_WCC( TXT("Missing world file name") );
		return false;
	}

	if ( options.GetSingleOptionValue( TXT("extents"), text ) )
	{
		if ( !FromString<Float>( text, m_worldExtents ) )
			return false;
	}

	return true;
}

//----

namespace Helper
{
	static const Uint64 GetAllocatedMemorySize()
	{
		return Memory::GetPoolTotalBytesAllocated< MemoryPool_Default >();
	}
}

//----

CTestMemCommandlet::CTestMemCommandlet()
{
	m_commandletName = CName( TXT("testmem") );
}

Bool CTestMemCommandlet::Execute( const CommandletOptions& options )
{
	// parse the settings
	if ( !m_settings.Parse( options ) )
		return false;

	// load the world
	WorldLoadingContext loadingContext;
	m_world = CWorld::LoadWorld( m_settings.m_worldPath, loadingContext );
	if ( !m_world )
	{
		ERR_WCC( TXT("Could not load world '%ls'"), m_settings.m_worldPath.AsChar() );
		return false;
	}

	// disable streaming for now
	m_world->EnableStreaming( false, false );

	// preload all gathered resource
	{
		const TDynArray< CGatheredResource* >& gatheredResources = CGatheredResource::GetGatheredResources();
		for ( Uint32 i=0; i<gatheredResources.Size(); ++i )
		{
			CGatheredResource* gr = gatheredResources[i];
			if ( gr )
			{
				gr->Load();
			}
		}
	}

	// test all the layers
	TDynArray< CLayerInfo* > allLayers;
	m_world->GetWorldLayers()->GetLayers( allLayers, false, true );

	// start layer testing
	{
		LOG_WCC( TXT("--------------------------------------") );
		LOG_WCC( TXT("-- Starting single layer testing    --") );
		LOG_WCC( TXT("--------------------------------------") );

		struct LayerLoadInfo
		{
			String	m_depotPath;
			Float	m_loadTime;
			Float	m_unloadTime;
			Uint64	m_memoryStart;
			Uint64	m_memoryLoaded;
			Uint64	m_memoryEnd;

			LayerLoadInfo( const CLayerInfo* info )
				: m_memoryStart(0)
				, m_memoryLoaded(0)
				, m_memoryEnd(0)
				, m_loadTime(0.0f)
				, m_unloadTime(0.0f)
				, m_depotPath( info->GetDepotPath() )
			{}

			const Int32 GetMemoryLoadingDelta() const
			{
				return (Int32)( (Int64)m_memoryLoaded - (Int64)m_memoryStart );
			}

			const Int32 GetMemoryCycleDelta() const
			{
				return (Int32)( (Int64)m_memoryEnd - (Int64)m_memoryStart );
			}

			static Bool CompareLoadingTime( const LayerLoadInfo& lh, const LayerLoadInfo& rh )
			{
				return ( lh.m_loadTime > rh.m_loadTime );
			}

			static Bool CompareUnloadingTime( const LayerLoadInfo& lh, const LayerLoadInfo& rh )
			{
				return ( lh.m_unloadTime > rh.m_unloadTime );
			}

			static Bool CompareMemoryLoadingDelta( const LayerLoadInfo& lh, const LayerLoadInfo& rh )
			{
				return ( lh.GetMemoryLoadingDelta() > rh.GetMemoryLoadingDelta() );
			}

			static Bool CompareMemoryCycleDelta( const LayerLoadInfo& lh, const LayerLoadInfo& rh )
			{
				return ( lh.GetMemoryCycleDelta() > rh.GetMemoryCycleDelta() );
			}
		};

		// output list for layer stats
		TDynArray< LayerLoadInfo > layerLoadInfo;
		layerLoadInfo.Reserve( allLayers.Size() );

		// process layers
		for ( Uint32 i=0; i<allLayers.Size(); ++i )
		{
			CLayerInfo* info = allLayers[i];
			LOG_WCC( TXT("Testing layer '%ls'..."), info->GetDepotPath().AsChar() );

			LayerLoadInfo ret( info );

			// get memory at loading start
			ret.m_memoryStart = Helper::GetAllocatedMemorySize();

			//CObject::DebugDumpClassList();
			//RED_MEMORY_DUMP_CLASS_MEMORY_REPORT();

			// Load layer
			{
				CTimeCounter timer;
				LayerLoadingContext context;
				context.m_loadHidden = true;
				context.m_queueEvent = false;
				if ( !info->SyncLoad(context) )
				{
					ERR_WCC( TXT("Failed to load layer '%ls'!"), info->GetDepotPath().AsChar() );
					continue;;
				}

				m_world->UpdateLoadingState();				

				ret.m_loadTime = timer.GetTimePeriod();
			}

			//CObject::DebugDumpClassList();
			//RED_MEMORY_DUMP_CLASS_MEMORY_REPORT();

			// get memory after layer was loaded
			ret.m_memoryLoaded = Helper::GetAllocatedMemorySize();

			// Unload layer
			{
				CTimeCounter timer;
				info->SyncUnload();

				m_world->UpdateLoadingState();

				SGarbageCollector::GetInstance().CollectNow();

				GObjectsDiscardList->ProcessList( true );

				ret.m_unloadTime = timer.GetTimePeriod();
			}
			// get memory after layer was unloaded
			ret.m_memoryEnd = Helper::GetAllocatedMemorySize();

			//CObject::DebugDumpClassList();
			//RED_MEMORY_DUMP_CLASS_MEMORY_REPORT();

			// add to stats
			layerLoadInfo.PushBack( ret );
		}

		LOG_WCC( TXT("--------------------------------------") );
		LOG_WCC( TXT("-- Finished single layer testing    --") );
		LOG_WCC( TXT("--------------------------------------") );

		// display loading times
		{
			LOG_WCC( TXT("Layer loading times:") );

			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("| Time [ms] | Layer depot path ") );
			LOG_WCC( TXT("--------------------------------------") );

			Sort( layerLoadInfo.Begin(), layerLoadInfo.End(), LayerLoadInfo::CompareLoadingTime );

			for ( Uint32 i=0; i<layerLoadInfo.Size(); ++i )
			{
				const LayerLoadInfo& info = layerLoadInfo[i];
				LOG_WCC( TXT("| %5.3f | %ls"),
					info.m_loadTime * 1000.0f,
					info.m_depotPath.AsChar() );
			}
			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("") );
		}

		// display unloading times
		{
			LOG_WCC( TXT("Layer unloading times:") );

			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("| Time [ms] | Layer depot path ") );
			LOG_WCC( TXT("--------------------------------------") );

			Sort( layerLoadInfo.Begin(), layerLoadInfo.End(), LayerLoadInfo::CompareUnloadingTime );

			for ( Uint32 i=0; i<layerLoadInfo.Size(); ++i )
			{
				const LayerLoadInfo& info = layerLoadInfo[i];
				LOG_WCC( TXT("| %5.3f | %ls"),
					info.m_unloadTime * 1000.0f,
					info.m_depotPath.AsChar() );
			}
			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("") );
		}

		// display memory loading delta
		{
			LOG_WCC( TXT("Memory loading delta:") );

			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("|  Memory [KB]  | Layer depot path ") );
			LOG_WCC( TXT("--------------------------------------") );

			Sort( layerLoadInfo.Begin(), layerLoadInfo.End(), LayerLoadInfo::CompareMemoryLoadingDelta );

			for ( Uint32 i=0; i<layerLoadInfo.Size(); ++i )
			{
				const LayerLoadInfo& info = layerLoadInfo[i];
				LOG_WCC( TXT("| %7.3f | %ls"),
					info.GetMemoryLoadingDelta() / 1024.0f,
					info.m_depotPath.AsChar() );
			}
			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("") );
		}

		// display memory cycle delta
		{
			LOG_WCC( TXT("Memory cycle delta (leaks):") );

			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("|  Memory [KB]  | Layer depot path ") );
			LOG_WCC( TXT("--------------------------------------") );

			Sort( layerLoadInfo.Begin(), layerLoadInfo.End(), LayerLoadInfo::CompareMemoryCycleDelta );

			for ( Uint32 i=0; i<layerLoadInfo.Size(); ++i )
			{
				const LayerLoadInfo& info = layerLoadInfo[i];
				LOG_WCC( TXT("| %7.3f | %ls"),
					info.GetMemoryCycleDelta() / 1024.0f,
					info.m_depotPath.AsChar() );
			}
			LOG_WCC( TXT("--------------------------------------") );
			LOG_WCC( TXT("") );
		}

	}

	// display almost final object list
	LOG_WCC( TXT("Pre world unload class list:") );
	//CObject::DebugDumpClassList();

	// unload world
	CWorld::UnloadWorld( m_world.Get() );
	SGarbageCollector::GetInstance().CollectNow();	
	GObjectsDiscardList->ProcessList( true );

	// display almost final object list
	LOG_WCC( TXT("Final class list:") );
	//CObject::DebugDumpClassList();

	// done
	return true;
}

void CTestMemCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  testmem [params]") );
	LOG_WCC( TXT("") );
	LOG_WCC( TXT("Params:") );
	LOG_WCC( TXT("  -world     Path to the w2w file to load") );
	LOG_WCC( TXT("  -numiter   Number of test iterations") );
	LOG_WCC( TXT("  -numpoints Number of test points") );
	LOG_WCC( TXT("  -extents   Extents (around origin) to test") );
}

