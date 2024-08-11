/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "debugPage.h"
#include "debugPageManagerBase.h"
#include "testFramework.h"
#include "../core/loadingJobManager.h"
#include "../redMemoryFramework/redMemorySystemMemoryStats.h"
#include "../renderer/renderElementMap.h"
#include "../gpuApiUtils/gpuApiMemory.h"

#include "fonts.h"
#include "game.h"
#include "renderFrame.h"
#include "renderProxy.h"
#include "world.h"
#include "baseEngine.h"
#include "../core/softHandleProcessor.h"
#include "meshComponent.h"
#include "renderer.h"
#include "debugServerManager.h"
#include "streamingSectorData.h"
#include "sectorDataStreaming.h"
#include "../physics/physXEngine.h"

#ifndef NO_EDITOR
#include "environmentManager.h"
#include "weatherManager.h"
#endif

#ifndef NO_DEBUG_PAGES

#ifdef RED_PLATFORM_DURANGO
#include <apu.h>
#endif

#ifdef RED_NETWORK_ENABLED
Char* GIpAddress = nullptr;
#endif // RED_NETWORK_ENABLED


volatile Float GLastRenderFrameTime = 0.0f;
volatile Float GLastRenderFenceTime = 0.0f;

IDebugPageManagerBase* IDebugPageManagerBase::s_instance = NULL;

IDebugPageManagerBase::IDebugPageManagerBase( CGatheredResource& fontResource )
:	m_activePage( NULL ),
	m_fontResource( fontResource )
{
	// Add empty debug page
	m_debugPages.PushBack( NULL );

	ASSERT( s_instance == NULL, TXT( "Debug Page Manager has already been instanciated" ) );
	s_instance = this;
}

IDebugPageManagerBase::~IDebugPageManagerBase()
{

}

void IDebugPageManagerBase::OnTick( Float timeDelta )
{
	if( m_activePage )
	{
		m_activePage->OnTick( timeDelta );
	}
}

Bool IDebugPageManagerBase::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	if( m_activePage )
	{
		return m_activePage->OnViewportInput( view, key, action, data );
	}

	return false;
}

Bool IDebugPageManagerBase::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( m_activePage )
	{
		return m_activePage->OnViewportClick( view, button, state, x, y );
	}

	return false;
}

void IDebugPageManagerBase::DisplayMemorySummary( CRenderFrame* frame, Int32 x, Int32 y )
{
	const Int32 c_oneLineHeight = 12;

#ifdef RED_PLATFORM_DURANGO
	const Int32 c_lineCount = 4;
#elif defined( RED_PLATFORM_ORBIS )
	const Int32 c_lineCount = 3;
#else
	const Int32 c_lineCount = 2;
#endif
	
	Int32 yAxis = y - ( c_oneLineHeight * c_lineCount );

	const Float oneMB = 1024*1024;
	const Float invOneMB = 1.0f/oneMB;

#ifndef ENABLE_EXTENDED_MEMORY_METRICS
	String defaultPoolSummary = String::Printf( TXT( "Default Pool: %.2fmb free" ), 
		( Memory::GetPoolBudget< MemoryPool_Default >() - Memory::GetPoolTotalBytesAllocated< MemoryPool_Default >() ) * invOneMB );
	frame->AddDebugScreenText( x, yAxis, defaultPoolSummary, Color::WHITE );	yAxis += c_oneLineHeight;
#else
#	ifdef ENABLE_GAMESAVE_POOL_DEBUG
		String gameSavePoolSummary = String::Printf( TXT( "GameSave Pool: %.2fmb free (%.2fmb free at peak usage)" ), 
													( Memory::GetPoolBudget< MemoryPool_GameSave >() - gameSavePoolMetrics.m_totalBytesAllocated ) * invOneMB,
													( Memory::GetPoolBudget< MemoryPool_GameSave >() - gameSavePoolMetrics.m_totalBytesAllocatedPeak ) * invOneMB );
		frame->AddDebugScreenText( x, yAxis, gameSavePoolSummary, Color::LIGHT_YELLOW );	yAxis += c_oneLineHeight;
#	endif
	String defaultPoolSummary = String::Printf( TXT( "Default Pool: %.2fmb free (%.2fmb free at peak usage)" ), 
												( Memory::GetPoolBudget< MemoryPool_Default >() - Memory::GetPoolTotalBytesAllocated< MemoryPool_Default >() ) * invOneMB,
												( Memory::GetPoolBudget< MemoryPool_Default >() - Memory::GetPoolTotalBytesAllocatedPeak< MemoryPool_Default >() ) * invOneMB );
	frame->AddDebugScreenText( x, yAxis, defaultPoolSummary, Color::WHITE );	yAxis += c_oneLineHeight;
#endif

	// On all platforms, we display the total physical memory available to the title. On PC we obviously have more due to virtual memory
	Red::MemoryFramework::SystemMemoryInfo systemMemory = Red::MemoryFramework::SystemMemoryStats::RequestStatistics();
	String systemMemorySummary = String::Printf( TXT( "%.2fmb system memory free" ), systemMemory.m_freePhysicalBytes * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemorySummary, Color::WHITE );	yAxis += c_oneLineHeight;

#ifdef RED_PLATFORM_DURANGO
	// On XB1, not all the system memory is available for use
	systemMemorySummary = String::Printf( TXT( "%.2fmb used by Legacy APIs" ), 
										  systemMemory.m_platformStats.m_legacyUsed * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemorySummary, Color::WHITE );	yAxis += c_oneLineHeight;

#ifdef ENABLE_EXTENDED_XMEMALLOC_TRACKING
	ApuHeapState state;
	HRESULT result = ApuHeapGetState( &state, APU_ALLOC_CACHED );

	String systemMemoryBreakdown = String::Printf( TXT( "D3D: %.2fmb" ), systemMemory.m_platformStats.m_d3dAllocated * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemoryBreakdown, Color::WHITE );	
	yAxis += c_oneLineHeight;

	systemMemoryBreakdown = String::Printf( TXT( "Audio: %.2fmb/%.2fmb" ), systemMemory.m_platformStats.m_audioAllocated * invOneMB, state.bytesAllocated * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemoryBreakdown, ( ( systemMemory.m_platformStats.m_audioAllocated * invOneMB - 20 ) < ( state.bytesAllocated * invOneMB ) ? Color::RED : Color::WHITE ) );	
	yAxis += c_oneLineHeight;

	systemMemoryBreakdown = String::Printf( TXT( "Platform: %.2fmb" ), systemMemory.m_platformStats.m_platformAllocated * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemoryBreakdown, Color::WHITE );	
	yAxis += c_oneLineHeight;

	systemMemoryBreakdown = String::Printf( TXT( "Middleware: %.2fmb" ), systemMemory.m_platformStats.m_middlewareAllocated * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemoryBreakdown, Color::WHITE );	
	yAxis += c_oneLineHeight;
#endif

#elif defined( RED_PLATFORM_ORBIS )
	// Memory is split on PS4
	systemMemorySummary = String::Printf( TXT( "%.2fmb direct memory available, %.2fmb flexible available" ), 
										  systemMemory.m_platformStats.m_directMemoryFree * invOneMB,
										  systemMemory.m_platformStats.m_approxFlexibleMemoryFree * invOneMB );
	frame->AddDebugScreenText( x, yAxis, systemMemorySummary, Color::WHITE );	yAxis += c_oneLineHeight;
#endif

}
/*
#if MICROSOFT_ATG_DYNAMIC_SCALING
namespace GpuApi
{
	extern Uint32 g_DynamicScaleWidthFullRes;
	extern Uint32 g_DynamicScaleHeightFullRes;
}
#endif
*/
void IDebugPageManagerBase::OnViewportGenerateFragments( IViewport* view, CRenderFrame* frame )
{
	// Stats
	static Float statFrameTime = 0.0f;
	static Float statRenderTime = 0.0f;
	static Float statRenderFenceTime = 0.0f;
	static Float statGPUTime = 0.0f;

	// Averages
	static Float avgFrameTime = 0.0f;
	static Float avgRenderTime = 0.0f;
	static Float avgRenderFenceTime = 0.0f;
	static Float avgGPUTime = 0.f;

	// Accumulate
	avgFrameTime += ( GEngine->GetLastTimeDeltaUnclamped() );
	avgRenderTime += GLastRenderFrameTime;
	avgRenderFenceTime += GLastRenderFenceTime;

	extern IRender* GRender;

	if (GRender)
	{
		avgGPUTime += GRender->GetLastGPUFrameDuration();
	}

	// Hold FPS counter for a while, so it will be simpler to read
	static Uint32 counter = 0;
	const Uint32 limit = 5;
	if ( ++counter >= limit )
	{
		// Reset
		counter = 0;

		// Count stats
		statRenderFenceTime = avgRenderFenceTime / (Float)limit;
		statFrameTime = avgFrameTime / (Float)limit - statRenderFenceTime;
		statRenderTime = avgRenderTime / (Float)limit;
		statGPUTime = avgGPUTime / (Float)limit;

		// Reset
		avgFrameTime = 0.0f;
		avgRenderTime = 0.0f;
		avgRenderFenceTime = 0.0f;
		avgGPUTime = 0.f;
	}

	if( m_activePage )
	{
		if ( !m_activePage->FullScreen() )
		{
			// Generic page shit
			CFont* font = m_fontResource.LoadAndGet< CFont >();
			if ( font )
			{
				// Title
				Int32 x=0, y=0;
				Uint32 width=0, height=0;
				font->GetTextRectangle( m_activePage->GetPageName(), x, y, width, height );
				frame->AddDebugScreenText( frame->GetFrameOverlayInfo().m_width/2 - width/2, 45, m_activePage->GetPageName(), Color::WHITE, font );

				// FPS
				frame->AddDebugScreenFormatedText( 50, 33, TXT("FPS: %1.2f"), GEngine->GetLastTickRate() ); y += 15;

#ifndef NO_TEST_FRAMEWORK
				// Testing Framework overlay (if applicable)
				if ( STestFramework::GetInstance().IsActive() )
				{
					STestFramework::GetInstance().GenerateDebugFragments( frame );
				}
#endif // NO_TEST_FRAMEWORK
			}
		}

		// Draw Page itself
		m_activePage->OnViewportGenerateFragments( view, frame );
	}
	else
	{
		// average FPS
		Float avgFPS = GEngine->GetLastTickRate();
		frame->AddDebugScreenText( 50, 33, String::Printf( TXT("avg FPS: %1.2f"), avgFPS ), 0, false, Color::WHITE, Color::BLACK );		

		if (avgFPS < 20.f)
		{			
			frame->AddDebugScreenText( 140, 33, TXT(""), 0, true, Color::WHITE, Color::RED );
		}
		else if(avgFPS < 25.f)
		{			
			frame->AddDebugScreenText( 140, 33, TXT(""), 0, true, Color::WHITE, Color::LIGHT_YELLOW );
		}
		else
		{
			frame->AddDebugScreenText( 140, 33, TXT(""), 0, true, Color::WHITE, Color::GREEN );
		}

#ifndef NO_TEST_FRAMEWORK
		if ( STestFramework::GetInstance().IsActive() )
		{
			STestFramework::GetInstance().GenerateDebugFragments( frame );
		}
#endif
		// minimum FPS
		Float minFPS = GEngine->GetMinTickRate();
		frame->AddDebugScreenText( 150, 33, String::Printf( TXT("min FPS: %1.2f"), minFPS ), 0, false, Color::WHITE, Color::BLACK );

		if (minFPS < 20.f)
		{			
			frame->AddDebugScreenText( 240, 33, TXT(""), 0, true, Color::WHITE, Color::RED );
		}
		else if(minFPS < 25.f)
		{			
			frame->AddDebugScreenText( 240, 33, TXT(""), 0, true, Color::WHITE, Color::LIGHT_YELLOW );
		}
		else
		{
			frame->AddDebugScreenText( 240, 33, TXT(""), 0, true, Color::WHITE, Color::GREEN );
		}
		

		if ( statRenderTime > statFrameTime )
		{			
			frame->AddDebugScreenText( 50, 55, String::Printf( TXT("RENDER BOUND") ), 0, false, Color::YELLOW, Color::BLUE );
		}
		else
		{
			frame->AddDebugScreenText( 50, 55, String::Printf( TXT("GAMEPLAY BOUND") ), 0, false, Color::CYAN, Color::BROWN );
		}

#ifndef NO_DEBUG_SERVER
		frame->AddDebugScreenText( 50, 75, String::Printf( TXT("DbgSrv Ini %i Deb %i Res %i Att %i Run %i "), !!DBGSRV_CALL( IsInitialized() ), !!DBGSRV_CALL( IsConnected() ), !!DBGSRV_CALL( IsConnectedAsResourceServer() ), !!DBGSRV_CALL( IsAttached() ), !!DBGSRV_CALL( IsGameRunning() ) ) );
		frame->AddDebugScreenText( 50, 87, String::Printf( TXT("Recv %i Sent %i Game %i Out %i"), DBGSRV_CALL( GetReceivedCount() ), DBGSRV_CALL( GetSentCount() ), DBGSRV_CALL( GetGameThreadCommandsCount() ), Red::Network::Manager::GetInstance()->GetOutgoingPacketsCount() ) );
#endif

		DisplayMemorySummary( frame, 8, frame->GetFrameOverlayInfo().m_height );

		// Job manager status
		{
			const Bool ioIdle = SJobManager::GetInstance().IsIdle();
			const Bool ioLocked = SJobManager::GetInstance().IsLocked();
			const Bool ioBlockingGC = SJobManager::GetInstance().IsBlockingGC();
			Uint32 x = 50;
			frame->AddDebugScreenText( x, 99, TXT("IO:"), Color::GRAY ); x += 25;
			frame->AddDebugScreenText( x, 99, TXT("BUSY"), ioIdle ? Color(80,80,80) : Color::GREEN ); x += 30;
			frame->AddDebugScreenText( x, 99, TXT(" GC "), ioBlockingGC ? Color::YELLOW : Color(80,80,80) ); x += 30;
			frame->AddDebugScreenText( x, 99, TXT("LOCK"), ioLocked  ? Color::RED : Color(80,80,80) ); x += 30;
		}

#ifdef RED_NETWORK_ENABLED
		{
			if( GIpAddress )
			{
				Red::Network::Manager* network = Red::Network::Manager::GetInstance();

				if( network->IsListenSocketActive() )
				{
					frame->AddDebugScreenText( 50, 111, String::Printf( TXT( "Network IP: %ls" ), GIpAddress ), Color::WHITE );
				}
				else
				{
					frame->AddDebugScreenText( 50, 111, String::Printf( TXT( "Network Listener Inactive" ) ), Color::WHITE );
				}
			}
		}
#endif // RED_NETWORK_ENABLED
/*
#if MICROSOFT_ATG_DYNAMIC_SCALING
		static char s_DynamicResolutionString[32] = {0};

		sprintf_s(s_DynamicResolutionString, sizeof(s_DynamicResolutionString), "Res %ix%i", GpuApi::g_DynamicScaleWidthFullRes, GpuApi::g_DynamicScaleHeightFullRes);

		frame->AddDebugScreenText( 50, 55, (const Char *)s_DynamicResolutionString, 0, false, Color::YELLOW, Color::BLUE );
#endif 
*/
		// entity streaming status
		Uint32 y = 80;
		if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetStreamingSectorData())
		{
			SStreamingDebugData data;
			GGame->GetActiveWorld()->GetStreamingSectorData()->GetDebugInfo( data );

			frame->AddDebugScreenText( 840, y, 
				String::Printf( TXT( "Entities: %d in range, %d loading, %d streamed, %d locked, %d total %ls" ), 
					data.m_numProxiesInRange, data.m_numProxiesStreaming, 
					data.m_numProxiesStreamed, data.m_numProxiesLocked, data.m_numProxiesRegistered,
					data.m_wasOverBudgetLastFrame ? TXT(" - OVER BUDGET") : TXT("") ),
				0, false, data.m_wasOverBudgetLastFrame ? Color::LIGHT_RED : Color::WHITE, Color::BLUE );
			y += 14;
		}

		// sector streaming status
		if ( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->IsCooked() )
		{
			SSectorStreamingDebugData data;
			GGame->GetActiveWorld()->GetStreamingStats( data );

			frame->AddDebugScreenText( 840, y, 
				String::Printf( TXT( "Sector: %d in range, %d loading, %d streamed, %d locked, %d total %ls" ), 
				data.m_numObjectsInRange, data.m_numObjectsStreaming, data.m_numObjectsStreamed, data.m_numObjectsLocked, data.m_numObjectsRegistered, 
				data.m_wasOverBudgetLastFrame ? TXT(" - OVER BUDGET") : TXT("") ),
				0, false, data.m_wasOverBudgetLastFrame ? Color::LIGHT_RED : Color::WHITE, Color::BLUE );
			y += 14;
		}

		// mesh streaming stats
		if ( GGame && GGame->GetActiveWorld() && GRender )
		{
			Uint32 numLoadingMeshes = 0;
			Uint32 sizeofLoadingMeshes = 0;

			GRender->GetPendingStreamingMeshes( numLoadingMeshes, sizeofLoadingMeshes );
			frame->AddDebugScreenText( 840, y, 
				String::Printf( TXT( "Meshes: %d loading, %1.2f MB" ), 
				numLoadingMeshes, sizeofLoadingMeshes / (1024.0f*1024.0f) ),
				0, false, (numLoadingMeshes > 0 ) ? Color::LIGHT_YELLOW : Color::WHITE, Color::BLUE );
			y += 14;
		}

		// textures loading stats
		if ( GGame && GGame->GetActiveWorld() && GRender )
		{
			Uint32 numLoadingTextures = 0;
			Uint32 sizeofLoadingTextures = 0;

			GRender->GetPendingStreamingTextures( numLoadingTextures, sizeofLoadingTextures );
			frame->AddDebugScreenText( 840, y, 
				String::Printf( TXT( "Textures: %d loading, %1.2f MB" ), 
				numLoadingTextures, sizeofLoadingTextures / (1024.0f*1024.0f) ),
				0, false, (numLoadingTextures > 0 ) ? Color::LIGHT_YELLOW : Color::WHITE, Color::BLUE );
			y += 14;
		}
	}

	frame->AddDebugScreenText( 250, 30, String::Printf( TXT("RenderFrame: %1.3f ms"), statRenderTime * 1000.0f ), 0, false, GetColorForFrameTime( statRenderTime ), Color(0,0,0) );
	frame->AddDebugScreenText( 250, 40, String::Printf( TXT("GameplayFrame: %1.3f ms"), statFrameTime * 1000.0f ), 0, false, GetColorForFrameTime( statFrameTime ), Color(0,0,0) );
	frame->AddDebugScreenText( 250, 50, String::Printf( TXT("RenderFence: %1.3f ms"), statRenderFenceTime * 1000.0f ), 0, false, GetColorForSyncTime( statRenderFenceTime ), Color(0,0,0) );
	frame->AddDebugScreenText( 250, 60, String::Printf( TXT("RenderGPU: %1.3f ms"), statGPUTime ), 0, false, GetColorForFrameTime( statGPUTime / 1000.f ), Color(0,0,0) );
	
	static CPerfCounter* batcherRenderParticlesPerfCounter = 0;
	if( !batcherRenderParticlesPerfCounter ) batcherRenderParticlesPerfCounter = CProfiler::GetCounter( "SimulateAndDrawParticles" );

	if( batcherRenderParticlesPerfCounter )
	{
		static CPerfCounter* physXCharacterControllerMovePerfCounter = 0;
		const size_t currentPerfCountersCount = CProfiler::GetCountersCount();
		const Double freq = Red::System::Clock::GetInstance().GetTimer().GetFrequency();

		Color color;
		if ( statFrameTime < (1.0f / 30.0f) )
		{
			color = Color( 200, 255, 200 );
		}
		if ( statFrameTime < ( 1.0f / 20.0f ) )
		{
			color = Color( 255, 255, 200 );
		}
		else
		{
			color = Color( 255, 200, 200 );
		}

		static Uint64 previousTime = 0;
		const Uint64 time = batcherRenderParticlesPerfCounter->GetTotalTime();
		static Uint32 previousHit = 0;
		Uint32 hit = batcherRenderParticlesPerfCounter->GetTotalCount();
		frame->AddDebugScreenText( 250, 90, String::Printf( TXT("SimulateAndDrawParticles: %1.3f ms %i nb"), (float)((time - previousTime)/freq)*1000.0f, hit - previousHit ), 0, false, color, Color(0,0,0) );
		previousTime = time;
		previousHit = hit;
	}

	{
		Color color;
		if ( statFrameTime < (1.0f / 30.0f) )
		{
			color = Color( 200, 255, 200 );
		}
		if ( statFrameTime < ( 1.0f / 20.0f ) )
		{
			color = Color( 255, 255, 200 );
		}
		else
		{
			color = Color( 255, 200, 200 );
		}

		float physicsFetchTime = 0.0f;
		float physicsClothFetchTime = 0.0f;
		Uint32 clothAttached = 0;
		Uint32 clothSimulated = 0;
#ifdef USE_PHYSX
		GPhysXEngine->GetTimings( physicsFetchTime, physicsClothFetchTime, clothAttached, clothSimulated );
#endif
		if( physicsFetchTime != -1.0f )
		{
			frame->AddDebugScreenText( 250, 70, String::Printf( TXT("PhysicsFetchTime: %1.3f ms"), physicsFetchTime, 0, false, color, Color(0,0,0) ) );
		}

		if( physicsClothFetchTime != -1.0f )
		{
			frame->AddDebugScreenText( 250, 80, String::Printf( TXT("PhysicsClothFetchTime: %1.3f ms"), physicsClothFetchTime, 0, false, color, Color(0,0,0) ) );
		}
		frame->AddDebugScreenText( 250, 150, String::Printf( TXT("Cloth simulation/attached: %i / %i "), clothSimulated, clothAttached ), 0, false, color, Color(0,0,0) );
	}

#ifdef NEW_PROFILER_ENABLED
	const Bool started = SProfilerManager::GetInstance().IsStarted();
	frame->AddDebugScreenText( 840, 33, String::Printf( TXT("Profiler [%s] script %i frame %i buff %0.1f%%"), started ? TXT("running") : TXT("stopped"), !!SScriptProfilerManager::GetInstance().IsEnableProfileFunctionCalls(), SProfilerManager::GetInstance().GetStoredFrames(), SProfilerManager::GetInstance().GetFilledBufferFactor()*100.0f ), 0, false, started ? Color(255,70,70) : Color(0,255,0), Color(0,0,0) );	
	frame->AddDebugScreenText( 840, 44, String::Printf( TXT("blocks %i signals %i funcs %i"), SProfilerManager::GetInstance().GetStoredBlocks(), SProfilerManager::GetInstance().GetStoredSignals(), SProfilerManager::GetInstance().GetRegisteredFunctionsCount() ), 0, false, started ? Color(255,70,70) : Color(0,255,0), Color(0,0,0) );
	const Double profilingTime = SProfilerManager::GetInstance().GetProfilingTime();
	const Double internalTime = SProfilerManager::GetInstance().GetInternalTime();
	frame->AddDebugScreenText( 840, 55, String::Printf( TXT("time %0.3fs internal timer %0.3fs/%0.2f%%"), (Float)profilingTime, (Float)internalTime, profilingTime>0.0f ? ((Float)(internalTime/profilingTime)*100.0f) : 0.0f ), 0, false, started ? Color(255,0,0) : Color(0,255,0), Color(0,0,0) );
#endif

	if ( GGame->GetActiveWorld() && !m_activePage )
	{
		const SceneRenderingStats stat = GGame->GetActiveWorld()->GetRenderSceneEx()->GetRenderStats();

		Int32 y = 100;
		Int32 x = 50;
		Int32 width = 260;
		Int32 height = 0;
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriangleStats )	) height += 205;
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ShadowStats )		) height += 430 + frame->GetFrameInfo().m_requestedNumCascades * 15;
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_OcclusionStats )	) height += ( 12 * 15 + 20 + 5); // 12 lines, each is 15px + 20px free space + 5px just to be sure
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexStats )		) height += 130;
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TerrainStats )	) height += 120;
		
		if ( height > 0 )
		{
			static Color c = Color::BLACK;
			c.A = 128;
			frame->AddDebugRect( x - 10, y - 10, width, height, c );
		}
		
		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriangleStats ) )
		{
			// scene general
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Scene chunks: %d"), stat.m_numSceneChunks ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Scene batches/instanced batches: %d/%d"), stat.m_numSceneBatches, stat.m_numSceneInstancedBatches ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Scene biggest batch/instanced batch: %d/%d"), stat.m_biggestSceneBatch, stat.m_biggestSceneInstancedBatch); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Scene smallest batch/instanced batch: %d/%d"), stat.m_smallestSceneBatch, stat.m_smallestSceneInstancedBatch); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Scene triangles: %1.2f M"), (float)stat.m_numSceneTriangles / 1000000.0f); y += 15;
			y += 20;

			// particles
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Particle emitters: %d"), stat.m_numParticleEmitters ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Particle count: %1.2f K"), (float)stat.m_numParticles / 1000.0f ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Particles mesh chunks: %d"), stat.m_numParticleMeshChunks ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Particles mesh tris: %1.2f K"), (float)stat.m_numParticleMeshTriangles / 1000.0f ); y += 15;
			y += 20;

			/*// decals
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Decal proxies: %d"), stat.m_numDecalProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Decal chunks: %d"), stat.m_numDecalChunks ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Decal triangles: %1.2f K"), (float)stat.m_numDecalTriangles / 1000.0f ); y += 15;
			y += 20;*/

			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Num constant buffer updates: %i"), stat.m_numConstantBufferUpdates );
			y += 20;
#ifdef RED_PLATFORM_CONSOLE
			frame->AddDebugScreenFormatedText( x, y, stat.m_isConstantBufferSafe? Color::WHITE : Color::RED, TXT("Constant memory load: %1.2f MB"), stat.m_constantMemoryLoad / ( 1024.0f * 1024.0f ) );
			y += 20;
#endif
		}

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_FlaresData ) )
		{
			const Uint32 rowHeight = 17;

			frame->AddDebugScreenText( x, y, String::Printf( TXT("Active flares: %u"), (Uint32)stat.m_numActiveFlares ), 0, true, Color::WHITE, Color::BLACK ); y += rowHeight;
		}

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_EnvProbeStats ) )
		{
			const Uint32 rowHeight = 17;

			frame->AddDebugScreenText( x, y, String::Printf( TXT("Global probe objects: %u"), (Uint32)stat.m_totalGlobalEnvProbeObjects ), 0, true, (1 != stat.m_totalGlobalEnvProbeObjects ? Color::LIGHT_RED : Color::WHITE), Color::BLACK ); y += rowHeight;
			frame->AddDebugScreenText( x, y, String::Printf( TXT("Total probe objects: %u"), (Uint32)stat.m_totalEnvProbeObjects ), 0, true, Color::WHITE, Color::BLACK ); y += rowHeight;
			y += 5;

			if ( 0 == stat.m_numEnvProbeStats )
			{
				frame->AddDebugScreenText( x, y, TXT("No envProbe stats to display"), 0, true, Color::LIGHT_RED, Color::BLACK ); y += rowHeight;
			}
			else
			{
				const Int32 origX = x;
				const Int32 origY = y;
				for ( Uint32 probe_i=0; probe_i<stat.m_numEnvProbeStats; ++probe_i )
				{	
					x = origX + (probe_i / 4) * 220;
					y = origY + (probe_i % 4) * 160;

					const SceneRenderingStats::EnvProbeStats &envProbeStats = stat.m_envProbeStats[probe_i];
					const Color textColor = envProbeStats.m_isDuringUpdate ? (envProbeStats.m_hasProbeObject ? Color::LIGHT_GREEN : Color::LIGHT_YELLOW) : (envProbeStats.m_hasProbeObject ? Color::WHITE : Color::LIGHT_RED);
					const Color textBgColor = Color::BLACK;

					frame->AddDebugScreenText( x, y, String::Printf( TXT("DebugId: %u"), (Uint32)envProbeStats.m_debugId ), 0, true, textColor, textBgColor ); y += rowHeight;
					frame->AddDebugScreenText( x, y, String::Printf( TXT("IsGlobal: %u"), (Uint32)(envProbeStats.m_isGlobalProbe ? 1 : 0) ), 0, true, textColor, textBgColor ); y += rowHeight;
					frame->AddDebugScreenText( x, y, String::Printf( TXT("HasObject: %u"), (Uint32)(envProbeStats.m_hasProbeObject ? 1 : 0) ), 0, true, textColor, textBgColor ); y += rowHeight;
					frame->AddDebugScreenText( x, y, String::Printf( TXT("DuringUpdate: %u"), (Uint32)(envProbeStats.m_isDuringUpdate ? 1 : 0) ), 0, true, textColor, textBgColor ); y += rowHeight;
					frame->AddDebugScreenText( x, y, String::Printf( TXT("Weight: %f"), (Float)(envProbeStats.m_weightOneThousands / 1000.f) ), 0, true, textColor, textBgColor ); y += rowHeight;
					frame->AddDebugScreenText( x, y, String::Printf( TXT("UpdateDelay: %f"), (Float)(envProbeStats.m_lastUpdateDelayMillis / 1000.f) ), 0, true, textColor, textBgColor ); y += rowHeight;
					frame->AddDebugScreenText( x, y, String::Printf( TXT("UpdateDuration: %f"), (Float)(envProbeStats.m_lastUpdateDurationMillis / 1000.f)), 0, true, textColor, textBgColor ); y += rowHeight;
				}
			}
		}

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ShadowStats ) )
		{
			// general cascade stats
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Total cascade proxies: %d"), stat.m_numCascadeProxiesTotal ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Total cascade chunks: %d"), stat.m_numCascadeChunksTotal ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Total cascade triangles: %1.2f M"), (float)stat.m_numCascadeTrianglesTotal / 1000000.0f ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Proxies optimized: %d"), stat.m_numCascadeProxiesOptimized ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Proxies exclusion filtered: %d"), stat.m_numCascadeProxiesExlusionFiltered ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Proxies terrain culled: %d"), stat.m_numCascadeProxiesTerrainCulled ); y += 15;
			y += 20;

			// per-cascade stats
			const Uint32 numCascades = frame->GetFrameInfo().m_requestedNumCascades;
			for ( Uint32 i=0; i<numCascades; ++i )
			{
				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Cascade %d: %d proxies, %d chunks, %1.2f K"), 
					i, 
					stat.m_numCascadeProxies[i],
					stat.m_numCascadeChunks[i],
					(float)stat.m_numCascadeTriangles[i] / 1000.0f );
				y += 15;
			}
			y += 20;

			// dimmers			
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Num dimmers: %d"), stat.m_numDimmers ); y += 15;
			y += 20;

			// lights
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Num lights total: %d"), stat.m_numLights ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Num lights shadowed: %d"), stat.m_numLightWithShadows ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Num lights static shadows: %d"), stat.m_numLightWithStaticShadows ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Num distant lights: %d"), stat.m_numDistantLights ); y += 15;
			y += 20;

			// dynamic shadow stats
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Dymamic shadows regions: %d"), stat.m_numDynamicShadowsRegions ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Dynamic shadows proxies: %d"), stat.m_numDynamicShadowsProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Dynamic shadows mesh chunks: %d"), stat.m_numDynamicShadowsChunks ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Dynamic shadows triangles: %1.2f K"), (float)stat.m_numDynamicShadowsTriangles / 1000.0f ); y += 15;
			y += 20;

			// static shadows
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Static shadows cubes: %d"), stat.m_numStaticShadowsCubes ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Static shadows proxies: %d"), stat.m_numStaticShadowsProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Static shadows mesh chunks: %d"), stat.m_numStaticShadowsChunks ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Static shadows triangles: %1.2f K"), (float)stat.m_numStaticShadowsTriangles / 1000.0f ); y += 15;
			y += 20;

			// hi res shadows
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("HiRes shadows actors: %d"), stat.m_numHiResShadowsActors ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("HiRes shadows proxies: %d"), stat.m_numHiResShadowsProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("HiRes shadows mesh chunks: %d"), stat.m_numHiResShadowsChunks ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("HiRes shadows triangles: %1.2f KM"), (float)stat.m_numHiResShadowsTriangles / 1000.0f ); y += 15;
			y += 20;
		}

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_OcclusionStats ) )
		{
			// occlusion
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Visible objects: %d"), stat.m_visibleObjects ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Occlusion time: %1.3f ms"), stat.m_occlusionTimeQuery + stat.m_occlusionTimeDynamicObjects + stat.m_occlusionTimeVisibilityByDistance ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    query: %1.3fms"), stat.m_occlusionTimeQuery ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    dynamic objects: %1.3fms"), stat.m_occlusionTimeDynamicObjects ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    visibility by distance: %1.3fms"), stat.m_occlusionTimeVisibilityByDistance ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain") ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    building quadtree: %1.3fms"), stat.m_occlusionTimeBuildingTerrainQuadtree ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    culling quadtree: %1.3fms"), stat.m_occlusionTimeTerrain ); y += 15;
			y += 15;
			// RenderElementMap
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("RenderElementMap") ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    static proxies: %d, (%d)"), stat.m_renderedStaticProxies, stat.m_registeredStaticProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    dynamic proxies: %d / %d, (%d)"), stat.m_renderedDynamicProxies, stat.m_occludedDynamicProxies, stat.m_registeredDynamicProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    furthest proxies: %d / %d (%d)"), stat.m_renderedFurthestProxies, stat.m_occludedFurthestProxies, stat.m_registeredFurthestProxies ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("    dynamic decals: %d / %d, (%d)"), stat.m_renderedDynamicDecals, stat.m_occludedDynamicDecals, stat.m_registeredDynamicDecals ); y += 15;
			y += 20;
		}

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexStats ) )
		{
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex cloths: %i updated / %i rendered / %d shadowmap"),
				stat.m_numApexClothsUpdated, stat.m_numApexClothsRendered, stat.m_numApexClothsRenderedSM ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex destruction: %i updated / %i rendered / %d shadowmap"),
				stat.m_numApexDestructiblesUpdated, stat.m_numApexDestructiblesRendered, stat.m_numApexDestructiblesRenderedSM ); y += 15;
			y += 20;

			// Render resources
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex resources rendered: %d"), stat.m_numApexResourcesRendered ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex buffers updated (non fast path): %d VB / %d IB / %d BB"), stat.m_numApexVBUpdated, stat.m_numApexIBUpdated, stat.m_numApexBBUpdated ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex VB semantics updated: %d"), stat.m_numApexVBSemanticsUpdated ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex VB fast path update: %d"), stat.m_numApexVBUpdatedFastPath ); y += 15;
			y += 20;
		}

		if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TerrainStats ) )
		{
			// scene general
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain vertices read: %d"), stat.m_terrainVerticesRead ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain primitives read: %d"), stat.m_terrainPrimitivesRead ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain primitives sent for rasterizer: %d"), stat.m_terrainPrimitivesSentToRasterizer ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain primitives rasterized: %d"), stat.m_terrainPrimitivesRendered ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain Vertex Shader invocations: %d"), stat.m_terrainVertexShaderInvocations ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain Pixel Shader invocations: %d"), stat.m_terrainPixelShaderInvocations ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain Hull Shader invocations: %d"), stat.m_terrainHullShaderInvocations ); y += 15;
			frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Terrain Domain Shader invocations: %d"), stat.m_terrainDomainShaderInvocations ); y += 15;
			y += 20;

		}

#ifndef NO_EDITOR
		// Inform about current weather status
		const CEnvironmentManager* env = GGame->GetActiveWorld()->GetEnvironmentManager();
		if( env )
		{
			CWeatherManager* weather = env->GetWeatherManager();
			if( weather ) frame->AddDebugScreenFormatedText( 50, 200, Color::YELLOW, weather->GetDebugStatus().AsChar() );
		}
#endif	
		

#ifdef NO_EDITOR
		// Draw the camera position in world coordinates
		{
			const CRenderCamera& cam = frame->GetFrameInfo().m_camera;
			const Vector cameraPosition = cam.GetPosition();
			const EulerAngles& cameraRotation = cam.GetRotation();

			extern CGatheredResource resOnScreenTextFont;
			CFont* font = resOnScreenTextFont.LoadAndGet< CFont >();

			String text = String::Printf( TXT(" camera coords: [%0.2f, %0.2f, %0.2f %0.2f | %0.2f %0.2f %0.2f] "), 
				cameraPosition.X, cameraPosition.Y, cameraPosition.Z, cameraPosition.W, cameraRotation.Pitch, cameraRotation.Yaw, cameraRotation.Roll);

			Int32 tx, ty;
			Uint32 tw, th;
			font->GetTextRectangle( text, tx, ty, tw, th );

			frame->AddDebugScreenFormatedText( frame->GetFrameOverlayInfo().m_width - tw - 2, frame->GetFrameOverlayInfo().m_height - 2, Color::GREEN, text.AsChar() );
		}
#endif

		// General rendering ERRORS and WARNING
		{
			Uint32 x = frame->GetFrameOverlayInfo().m_width - 200;
			Uint32 y = 60;

			// dynamic shadows
			if ( stat.m_flagDynamicShadowsLimit )
			{
				frame->AddDebugScreenFormatedText( x, y, Color::RED, TXT("DYNAMIC SHADOW LIMIT") );
				y += 15;
			}
			else if ( stat.m_flagDynamicShadowsReduces )
			{
				frame->AddDebugScreenFormatedText( x, y, Color::YELLOW, TXT("DYNAMIC SHADOW REDUCED") );
				y += 15;
			}

			// static shadows
			if ( stat.m_flagStaticShadowsLimit )
			{
				frame->AddDebugScreenFormatedText( x, y, Color::RED, TXT("STATIC SHADOW LIMIT") );
				y += 15;
			}

			// hires shadows (it is slow so show as a warning)
			if ( stat.m_numHiResShadowsActors > 0 )
			{
				frame->AddDebugScreenFormatedText( x, y, Color::YELLOW, TXT("HIRES ENTITY SHADOWS ENABLED: %d"), stat.m_numHiResShadowsActors );
				y += 15;
			}
		}
	}
}

Bool IDebugPageManagerBase::OnViewportCalculateCamera( IViewport* view, CRenderCamera& camera )
{
	if( m_activePage )
	{
		return m_activePage->OnViewportCalculateCamera( view, camera );
	}

	return false;
}

void IDebugPageManagerBase::SelectDebugPage( IDebugPage* page )
{
	if ( m_activePage != page )
	{
		// Deactivate current page
		if ( m_activePage )
		{
			m_activePage->OnPageHidden();
			LOG_ENGINE( TXT("Debug page '%ls' hidden"), m_activePage->GetPageName().AsChar() );
		}

		// Select new page
		m_activePage = page;

		// Activate new page
		if ( m_activePage )
		{
			m_activePage->OnPageShown();
			LOG_ENGINE( TXT("Debug page '%ls' shown"), m_activePage->GetPageName().AsChar() );
		}
	}		
}

void IDebugPageManagerBase::RegisterDebugPage( IDebugPage* page )
{
	if ( page )
	{
		ASSERT( !m_debugPages.Exist( page ), TXT( "Trying to add debug page '%ls' to debug page manager twice" ), page->GetPageName().AsChar() );
		m_debugPages.PushBackUnique( page );
	}
	else
	{
		WARN_ENGINE( TXT( "Attemping to add NULL debug page" ) );
	}
}

void IDebugPageManagerBase::UnregisterDebugPage( IDebugPage* page )
{
	if ( page )
	{
		// Deactivate active page
		if ( m_activePage == page )
		{
			m_activePage->OnPageHidden();
			m_activePage = NULL;
		}

		// Remove from list
		ASSERT( m_debugPages.Exist( page ), TXT( "Debug page '%ls' does not exist in debug page manager" ), page->GetPageName().AsChar() );
		m_debugPages.Remove( page );
	}
}

Color IDebugPageManagerBase::GetColorForFrameTime( Float time ) const
{
	if ( time < (1.0f / 30.0f) )
	{
		return Color( 200, 255, 200 );
	}
	if ( time < ( 1.0f / 20.0f ) )
	{
		return Color( 255, 255, 200 );
	}
	else
	{
		return Color( 255, 200, 200 );
	}
}

Color IDebugPageManagerBase::GetColorForSyncTime( Float time ) const
{
	if ( time < (1.0f / 300.0f) )
	{
		return Color( 200, 255, 200 );
	}
	if ( time < ( 1.0f / 200.0f ) )
	{
		return Color( 255, 255, 200 );
	}
	else
	{
		return Color( 255, 200, 200 );
	}
}

#endif
