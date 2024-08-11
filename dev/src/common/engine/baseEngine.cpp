/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "baseEngine.h"

#include "renderCommands.h"
#include "scaleformSystem.h"
#include "soundSystem.h"
#include "animationCache2.h"
#include "dependencyLoaderSRT.h"
#include "../physics/physicsDebugger.h"
#include "debugServerManager.h"
#include "occlusionSystem.h"
#include "remoteConnection.h"
#include "videoDebugHud.h"
#include "../core/debugPageServer.h"
#include "../core/deferredDataBufferKickoff.h"
#include "../core/fileSystemProfilerWrapper.h"

#include "../core/deferredDataBufferOOMQueue.h"

#if defined( RED_PLATFORM_WINPC ) || defined( RED_PLATFORM_DURANGO )
	#include "../redSystem/windowsDebuggerWriter.h"
	typedef Red::System::Log::WindowsDebuggerWriter LogTTYWriter;
#elif defined( RED_PLATFORM_ORBIS )
	#include <system_service.h>
	#include "../redSystem/ttyWriter.h"
	typedef Red::System::Log::TTYWriter LogTTYWriter;
#endif

#include "../redMemoryFramework/redMemorySystemMemoryStats.h"
#include "../gpuApiUtils/gpuApiMemory.h"

#ifndef NO_RED_GUI
#include "redGuiManager.h"
#ifndef NO_DEBUG_WINDOWS
#include "debugWindowsManager.h"
#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI

#ifndef NO_TEST_FRAMEWORK
#	include "testFramework.h"
#endif

#ifdef USE_PHYSX
#include "../physics/physXEngine.h"
#endif

#include "../physics/physicsEngine.h"
#include "textureCache.h"

#ifndef NO_MARKER_SYSTEMS
#include "markersSystem.h"
#endif	// NO_MARKER_SYSTEMS
#include "debugPageManagerBase.h"
#include "collisionCache.h"
#include "particlePool.h"
#include "shaderCache.h"
#include "debugPageManagerTabbed.h"
#include "renderSettings.h"
#include "animationManager.h"
#include "renderMemoryPages.h"
#include "renderFrame.h"
#include "world.h"

#include "../redSystem/cpuid.h"
#include "../core/objectDiscardList.h"
#include "../core/objectGC.h"
#include "../core/gatheredResource.h"
#include "../core/loadingJobManager.h"
#include "../core/taskManager.h"
#include "../core/depot.h"
#include "../core/configVarSystem.h"
#include "../core/configVarLegacyWrapper.h"
#include "../core/version.h"
#include "../core/feedback.h"
#include "../core/softHandleProcessor.h"
#include "../core/resourceUnloading.h"
#include "../core/scriptingSystem.h"
#include "rawInputManager.h"
#include "gameResource.h"
#include "../core/tokenizer.h"
#include "../core/contentManager.h"
#include "../core/configVar.h"
#include "videoPlayer.h"
#include "localizationManager.h"

#ifdef USE_PIX
#include <pix.h>
#endif
#include "inGameConfigRefreshEvent.h"

#ifdef USE_ANSEL
extern Bool isAnselSessionActive;
extern Bool isAnselTurningOn;
extern Bool isAnselTurningOff;
#endif // USE_ANSEL

// The engine
CBaseEngine* GEngine = NULL;

namespace Config
{
	TConfigVar< Int32, Validation::IntRange< 0, INT_MAX > >			cvLimitFPS( "Engine", "LimitFPS", 60, eConsoleVarFlag_Save );

	TConfigVar< Bool >												cvDebugDumpNameList( "Debug", "DumpNameList", false );
}

Bool GDataAsserts = true;

#ifdef RED_LOGGING_ENABLED
	static LogTTYWriter logTTYWriter;
#endif

CBaseEngine::CBaseEngine()
	: m_rawEngineTime( 0.0f )
	, m_lastTimeDelta( 0.0f )
	, m_lastTimeDeltaUnclamped( 0.0f )
	, m_lastTickRate( 0.0f )
	, m_minTickRateOver1Sec( 0.0f )
	, m_minTickRateToShow( 0.0f )
	, m_requestExit( false )
	, m_activeSubsystems( ES_Particles | ES_Scripts | ES_Physics | ES_Animation | ES_Rendering )
	, m_currentEngineTick( 0 )
	, m_meshColoringScheme( NULL )
	, m_postInitialized( false )
	, m_returnValue( 0 )
	, m_inputDeviceManager( nullptr )
	, m_usingBundles( false )
#ifndef NO_MARKER_SYSTEMS
	, m_markerSystemsManager( nullptr )
#endif	// NO_MARKER_SYSTEMS
	, m_hasDebugNetworkInitialized(0)
	, m_hasSoundInitialized(0)
	, m_state( BES_Initializing )
{
	// Initialize object system
	CObject::InitializeSystem();

	// Initialize timer
	EngineTime::Init();

	RED_LOG( PLM, TXT("BES_Initializing") );

	InGameConfig::GRefreshEventDispatcher::GetInstance().RegisterForEvent( CNAME( refreshEngine ), &m_refreshListener );
	InGameConfig::GRefreshEventDispatcher::GetInstance().RegisterForEvent( CNAME( refreshViewport ), &m_refreshListener );
	InGameConfig::GRefreshEventDispatcher::GetInstance().RegisterForEvent( CNAME( RefreshOutputMonitor ), &m_refreshListener );
}

CBaseEngine::~CBaseEngine()
{
	InGameConfig::GRefreshEventDispatcher::GetInstance().UnregisterFromAllEvents( &m_refreshListener );
}

IInputDeviceManager* CBaseEngine::GetInputDeviceManager() const
{
	return m_inputDeviceManager;
}

void CBaseEngine::OnSuspend( Bool constrained )
{
	// Guard against GGame not existing yet (early in boot)
	if( GGame )
	{
		GGame->OnSuspend();

		// we are suspending without first getting constrained, this should not happen
		if (!constrained)
		{
			CVideoPlayer* videoPlayer = GGame->GetVideoPlayer();
			if ( videoPlayer )
			{
				videoPlayer->TogglePause( true );
			}
			GSoundSystem->SoundEvent("system_suspend");
		}
	}

	// Guard against GUserProfileManager not existing yet (early in boot)
	if ( GUserProfileManager )
	{
		GUserProfileManager->OnEnterSuspendState();
	}
	
	GContentManager->OnSuspend();
	
	// Guard against GRender not existing yet (early in boot)
	if ( GRender )
	{
		( new CRenderCommand_SuspendRendering() )->Commit();

		GRender->Flush();
	}

	m_state = BES_Suspended;
	RED_LOG( PLM, TXT("BES_Suspended") );
}

void CBaseEngine::OnResume( Bool constrained )
{
	// Guard against GRender not existing yet (early in boot)
	if ( GRender )
	{
		( new CRenderCommand_ResumeRendering() )->Commit();

		GRender->Flush();
	}

	// Guard against GGame not existing yet (early in boot)
	if( GGame )
	{
		GGame->OnResume();
	}

	// Guard against GUserProfileManager not existing yet (early in boot)
	if ( GUserProfileManager )
	{
		GUserProfileManager->OnExitSuspendState();
	}

	GContentManager->OnResume();

	// we are returning right away to running mode without being constrained anymore, this should not happen
	if (!constrained)
	{
		if( GGame )
		{
			if(!GGame->IsAnyMenu())
			{
				GSoundSystem->SoundEvent("system_resume");
			}
			else
			{
				GSoundSystem->SoundEvent("system_resume_music_only");
			}
		}
		else
		{
			GSoundSystem->SoundEvent("system_resume");
		}

		CVideoPlayer* videoPlayer = GGame->GetVideoPlayer();
		if ( videoPlayer )
		{
			videoPlayer->TogglePause( false );
		}
	}

	// We are not setting the state here, because we will either enter constrained mode or running and this will set the state properly
	//m_state = BES_Running;
	//RED_LOG( PLM, TXT("BES_Running") );
} 

void CBaseEngine::OnEnterNormalRunningMode()
{
	m_state = BES_Running;
	RED_LOG( PLM, TXT("BES_Running") );
}

void CBaseEngine::OnEnterConstrainedRunningMode()
{
	m_state = BES_Constrained;
	RED_LOG( PLM, TXT("BES_Constrained") );

	if (GUserProfileManager)
	{
		GUserProfileManager->OnEnterConstrainedState();
	}

	if ( GContentManager )
	{
		GContentManager->OnEnterConstrain();
	}

	if (GSoundSystem)
	{
		GSoundSystem->SoundEvent("system_suspend");
	}

	if (GGame)
	{
		GGame->EnteredConstrainedMode();

		// TT 114924 [Game][TL][XR] XR-001-01/116-01 - Constraining on loading screen may result in non-interactive pause lasting up to 40 seconds
		// If we hit a prefetch, then it'll be a long time before we pump messages again. Pumping messages during the prefetch is also not a good
		// solution at this point. Result: don't pause the video on constrain if during a loading screen.
		if ( !GGame->IsLoadingScreenShown() )
		{
			CVideoPlayer* videoPlayer = GGame->GetVideoPlayer();
			if ( videoPlayer )
			{
				videoPlayer->TogglePause( true );
			}
		}
	}
}

void CBaseEngine::OnExitConstrainedRunningMode()
{
	if (GGame)
	{
		CVideoPlayer* videoPlayer = GGame->GetVideoPlayer();
		if ( videoPlayer )
		{
			videoPlayer->TogglePause( false );
		}

		GGame->ExitedConstrainedMode();
	}

	if ( GContentManager )
	{
		GContentManager->OnExitConstrain();
	}

	if (GUserProfileManager)
	{
		GUserProfileManager->OnExitConstrainedState();
	}
	
	if (GSoundSystem && GGame)
	{
		if(!GGame->IsInMainMenu())
		{
			GSoundSystem->SoundEvent("system_resume");
		}
		else
		{
			GSoundSystem->SoundEvent("system_resume_music_only");
		}
	}
	else if ( GSoundSystem )
	{
		GSoundSystem->SoundEvent("system_resume");
	}

	// We are not setting the state here, because we will either enter suspended mode or running and this will set the state properly
	//m_state = BES_Running;
	//RED_LOG( PLM, TXT("BES_Running") );
}

void CBaseEngine::RequestExit( Int32 programReturnValue ) 
{
	if ( !m_requestExit )
	{
		LOG_ENGINE( TXT("Requesting exit") );
		Red::System::Error::Handler::GetInstance()->SetAssertFlag( Red::System::Error::AC_ContinueAlways, true );
		m_requestExit = true;
		m_returnValue = programReturnValue;
	}
}

Bool CBaseEngine::GracefulExit( const Char* failMessage )
{
	// Signal other stuff
	GIsClosing = true;

#ifdef RED_NETWORK_ENABLED
	// Shutdown network
	if ( m_hasDebugNetworkInitialized )
	{
		m_hasDebugNetworkInitialized = false;
		m_network.ImmediateShutdown();
	}
#endif // RED_NETWORK_ENABLED

	// Close the sound system
	if ( !GSoundSystem && m_hasSoundInitialized )
	{
		m_hasSoundInitialized = false;
		GSoundSystem = new CSoundSystem();
		GSoundSystem->PostInit();
		GSoundSystem->Shutdown();
		delete GSoundSystem;
		GSoundSystem = NULL;
	}

	// ALWAYS RETURN FALSE
	ERR_ENGINE( TXT("Fail reason: %ls"), failMessage );
	return false;
}

Uint32 CountSetBits(size_t bitMask)
{
    Uint32 LSHIFT = sizeof(size_t)*8 - 1;
    Uint32 bitSetCount = 0;
    size_t bitTest = (size_t)1 << LSHIFT;    
    Uint32 i;
    
    for (i = 0; i <= LSHIFT; ++i)
    {
        bitSetCount += ((bitMask & bitTest)?1:0);
        bitTest/=2;
    }

    return bitSetCount;
}



void CBaseEngine::SetActiveSubsystem( Uint32 flag, Bool active )
{
	// Toggle flag
	if ( active )
	{
		m_activeSubsystems |= flag; 
	}
	else
	{
		m_activeSubsystems &= ~flag; 
	}
}

void CBaseEngine::OnFrameStart()
{
#ifndef RED_PLATFORM_CONSOLE
	// This is more for x86 PC than anything. ON x64 PC, we don't worry about running out of virtual memory 
	// and on consoles all the pools are fixed size
	const Red::System::MemSize c_memoryCleanupThreshold = 1024 * 1024 * 256;		// If we have < this memory free, ask the allocators to release what they can
	Red::MemoryFramework::SystemMemoryInfo memoryInfo = Red::MemoryFramework::SystemMemoryStats::RequestStatistics();
	if( memoryInfo.m_freeVirtualBytes < c_memoryCleanupThreshold || memoryInfo.m_freePhysicalBytes < c_memoryCleanupThreshold )
	{
		Memory::ReleaseFreeMemoryToSystem();
		GpuApi::GpuApiMemory::GetInstance()->ReleaseFreeMemoryToSystem();
	}
#endif

#ifndef DISABLE_LOW_MEMORY_WARNINGS
	const Red::System::Uint64 c_warningLevel = 1024 * 1024 * 32;
	Red::MemoryFramework::SystemMemoryStats::WarnOnLowMemory( c_warningLevel );
#endif
	
	// In order to ensure the memory metrics are as accurate as possible, we do this in here
	Memory::OnFrameStart();
#ifndef RED_PLATFORM_CONSOLE
	GpuApi::GpuApiMemory::GetInstance()->OnFrameStart();
#endif
}

void CBaseEngine::OnFrameEnd()
{
	// In order to ensure the memory metrics are as accurate as possible, we do this in here
	Memory::OnFrameEnd();
#ifndef RED_PLATFORM_CONSOLE
	GpuApi::GpuApiMemory::GetInstance()->OnFrameEnd();
#endif
}

Bool CBaseEngine::MainLoopSingleTick()
{
	PC_SCOPE_LV0( MainLoopIteration, PBC_CPU );

	// Calculate time delta
#ifdef CONTINUOUS_SCREENSHOT_HACK
	Double curTime = Red::System::Clock::GetInstance().GetTimer().NextFrameGameTimeHack();
#else
	Double curTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
#endif
	m_lastTimeDeltaUnclamped = (Float)( curTime - m_oldTickTime );
	m_oldTickTime = curTime;

#ifdef RED_FINAL_BUILD 
	float timeDelta = Min( m_lastTimeDeltaUnclamped, 0.066f );
#else
	float timeDelta = Min( m_lastTimeDeltaUnclamped, 0.1f ); // so it is "playable" with low framerate - although issues related to physics may appear
#endif

#ifndef NO_TEST_FRAMEWORK
	// Test framework
	if( STestFramework::GetInstance().IsActive() )
	{
		STestFramework::GetInstance().Tick( m_currentEngineTick, timeDelta );
	}
#endif

	// Advance internal timers
	m_rawEngineTime += timeDelta;
	m_lastTimeDelta = timeDelta;

	Float currentTickRate = 1.f / timeDelta;

	if (m_minTickRateOver1Sec > currentTickRate || m_minTickRateOver1Sec == 0.f)
	{
		m_minTickRateOver1Sec = currentTickRate;
	}

	// Tick rate calculation
	if ( curTime > (m_prevTickRateTime + 1.0) )
	{
		m_lastTickRate = m_numTicks / (Float)( curTime - m_prevTickRateTime );
		m_minTickRateToShow = m_minTickRateOver1Sec;
		m_minTickRateOver1Sec = 0.f;
		m_prevTickRateTime = curTime;
		m_numTicks = 0;
	}
	
	// Tick the engine
	if ( timeDelta > 0.0f )
	{
		OnFrameStart();
		Tick( timeDelta );
		OnFrameEnd();
	}

	// Count ticks
	m_numTicks += 1;

#ifdef USE_PROFILER
	UnifiedProfilerManager::GetInstance().Tick();
#endif

	return !m_requestExit;
}

void CBaseEngine::MainLoop()
{
	m_oldTickTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	m_prevTickRateTime = m_oldTickTime;
	m_numTicks = 0;
	// Keep the main loop
	while ( MainLoopSingleTick() )
	{
		continue;
	}
	//////////////////////////////////////////////////////////////////////////

	// The info
	LOG_ENGINE( TXT("Exited main loop") );
}

void CBaseEngine::Tick( Float timeDelta )
{
	PC_SCOPE_PIX( BaseEngineTick );
	RedIOProfilerBlock ioBlock( TXT("MainTick") );

	// DEMO HACKS - limit FPS if needed
	if ( Config::cvLimitFPS.Get() > 0 )
	{
		PC_SCOPE_PIX( LimitFPS );

		static EngineTime lastTickTime = EngineTime();
		static bool lastTickTimeValid = false;

		if ( lastTickTimeValid )
		{
			const Float minimalTickTime = 1.0f / (Float)Config::cvLimitFPS.Get();
			const EngineTime expectedFrameEnd = lastTickTime + minimalTickTime;
			
			// #hack: Check when we started the spin-loop to avoid practically "infinite loops" where the user sets the system clock back
			// Since only called on main thread, shouldn't have to worry about any fudge factor for different cores
			// Ultimately, the problem is our engine time isn't guaranteed to be monotonically increasing, and maybe we should
			// change it to sceKernelGetProcessTime() or something like that instead
			for (;;)
			{
				// do nothing on this thread
				// note: we DO NOT DO BUSY WAIT BECAUSE IT CAN RESULT IN UNPREDICTABLE BEHAVIOR
				const EngineTime now = EngineTime::GetNow();
				if ( now < lastTickTime )
				{
					LOG_ENGINE(TXT("Too high clock diff detected! Continuing engine tick!"));
					break;
				}
				else if ( now >= expectedFrameEnd )
				{
					break;
				}
			}
		}

		lastTickTime = EngineTime::GetNow();
		lastTickTimeValid = true;
	}
	
	if( GEngine != nullptr && GIsGame == true )
	{
#ifdef RED_FINAL_BUILD
		if( GEngine->GetState() == BES_Constrained )
#else
		if( GEngine->GetState() == BES_Constrained || GRedGui::GetInstance().GetEnabled() )
#endif
		{
			GGame->GetViewport()->SetMouseMode( MM_Normal );
		}
		else
		{
			GGame->GetViewport()->SetMouseMode( MM_ClipAndCapture );
		}
	}

	// Reschedule deferred data buffers that failed due to the out of memory conditions, we may have memory now
	SDeferredDataOOMQueue::GetInstance().Reschedule();
	
	// Check if need to refresh engine, drawable components, textures, etc.
	if( m_refreshListener.GetAndClearRefreshGraphics() )
	{
		RefreshEngine();
	}

	if( m_refreshListener.GetAndClearRefreshViewport() )
	{
		RequestRefreshViewport();
	}

	// Keep DDB loading busy
	SDeferredDataBufferKickOffList::GetInstance().KickNewJobs();

	// Keep memory at bay - if we run low in the object pool call emergency GC
	GObjectGC->Tick();

	// If renderer is not ready eg. has lost device, do not tick game
	if ( !GRender->PrepareRenderer() )
	{
		PC_SCOPE_PIX( RenderNotPrepared );
		Red::Threads::SleepOnCurrentThread( 100 );
		return;
	}
	else
	{
		( new CRenderCommand_NewFrame() )->Commit();
	}

	RED_ASSERT( GTaskManager );
	{
		PC_SCOPE_PIX( TaskManagerTick );
		GTaskManager->Tick();
	}
	
	// Process pending events from event manager
#ifndef NO_EDITOR_EVENT_SYSTEM
	{
		PC_SCOPE_PIX( ProcessPendingEvents );
		SEvents::GetInstance().ProcessPendingEvens();
	}
	
#endif

	{
		PC_SCOPE( ContentManagerUpdate )
		GContentManager->Update();
	}

	// Process disk file based resource management
	CDiskFile::NextFrame();

#ifdef RED_NETWORK_ENABLED
	m_scriptHandler.Update();
#endif

	// Update remote command handler
#ifdef RED_NETWORK_ENABLED
	if ( GRemoteConnection )
	{
		PC_SCOPE_PIX( RemoteConnection );
		GRemoteConnection->Flush();
	}
#endif

	// Update debug page service
#ifdef RED_NETWORK_ENABLED
	{
		PC_SCOPE_PIX( DebugPageServer );
		CDebugPageServer::GetInstance().ProcessServiceCommands();
	}
#endif

	// Update inputs
#ifndef NO_TEST_FRAMEWORK
	if( !STestFramework::GetInstance().TickInput() )
#endif
	{
		PC_SCOPE_PIX( ProcessInput );
		SRawInputManager::GetInstance().ProcessInput();
	}

	SScreenshotSystem::GetInstance().Tick();

	// Tick game if compiled scripts are valid
	if ( GScriptingSystem->IsValid() && GGame )
	{
		// Tick game engine	
		Float gameTimeDelta = timeDelta * GGame->GetTimeScale();

		GGame->Tick( gameTimeDelta );

#ifdef USE_ANSEL
		if ( !isAnselSessionActive && !isAnselTurningOn && !isAnselTurningOff )
#endif // USE_ANSEL
		{
			// Tick PhysXEngine
			GPhysicEngine->Update( gameTimeDelta );
		}
	}
	
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 )
	if( GUserProfileManager )
	{
		GUserProfileManager->Update();
	}
#endif

	// Flush collision cache (internally the flush is done only if the cache is dirty)
	GCollisionCache->Flush();

	// Tick debug page
#ifndef NO_DEBUG_PAGES
	RED_ASSERT( IDebugPageManagerBase::GetInstance() != nullptr, TXT("Using debug pages and no debug page manager was created") );
	if ( IDebugPageManagerBase::GetInstance() != nullptr )
	{
		PC_SCOPE( DebugPageManager );
		IDebugPageManagerBase::GetInstance()->OnTick( timeDelta );
	}
#endif

	// RTTI memory dump - windows only
#if defined(RED_PLATFORM_WIN32) || defined(RED_PLATFORM_WIN64)
	{
		static Bool lastKeyState = false;
		const Bool shiftState = 0 != (0x8000 & GetAsyncKeyState( VK_LSHIFT ) );
		const Bool ctrlState = 0 != (0x8000 & GetAsyncKeyState( VK_LCONTROL ) );
		const Bool altState = 0 != (0x8000 & GetAsyncKeyState( VK_LMENU ) );
		const Bool keyState = 0 != (0x8000 & GetAsyncKeyState( 'O' ) );
		if ( keyState && shiftState && ctrlState && altState && !lastKeyState )
		{
			SRTTI::GetInstance().DumpRTTIMemory( NULL );
			//SGarbageCollector::GetInstance().TryCollectNow();
		}
		lastKeyState = keyState;
	}
#endif

	// ehh
	//SGarbageCollector::GetInstance().CollectNow();

	// Tick renderer
	{
		PC_SCOPE_PIX( RenderTick );
		GRender->Tick( timeDelta );
		( new CRenderCommand_EndFrame() )->Commit();
	}

	// Tick sound system
	{
		PC_SCOPE_PIX( SoundTick );
		GSoundSystem->Tick( timeDelta );
	}

	// Resource unloading
	{
		PC_SCOPE_PIX( ResourceUnloaderTick );		
		SResourceUnloader::GetInstance().Update();
	}

	// General GC (emergency mode)
	{
		PC_SCOPE_LV0( GarbageCollectorTick, PBC_CPU );		
		GObjectGC->Tick();
	}

	// Object cleanup
	{
		PC_SCOPE_PIX( ObjectsDiscardListTick );		
		GObjectsDiscardList->ProcessList();
	}

#ifdef USE_UMBRA
	// PC_SCOPE_PIX( OcclusionSystemTick ); it's too small
	SOcclusionSystem::GetInstance().Tick();
#endif // USE_UMBRA

	// perform loading game resource / savegame
	GGame->HandleInGameSavesLoading();

#ifndef NO_MARKER_SYSTEMS
	{
		PC_SCOPE( MarkerSystemsManager );
		m_markerSystemsManager->Tick( timeDelta );
	}
#endif	// NO_MARKER_SYSTEMS

#ifndef NO_DEBUG_PAGES
	if ( GVideoDebugHud )
	{
		GVideoDebugHud->Tick( timeDelta );
	}
#endif

	// Flush logs
	{
		PC_SCOPE_PIX( LogFlush );
	}

	// Update current engine tick
	++m_currentEngineTick;
}/*

void RenderXb1SafeArea( CRenderFrame* frame, Int32 left, Int32 right, Int32 top, Int32 bottom, Float leftWidth, Float TopHeight, const Colour& colour )
{
	const Int32 frameWidth	= right - left;
	const Int32 frameHeight	= bottom - top;

	const Int32 leftSizeActual	= static_cast< Int32 >( leftWidth * ( frameWidth / 1920.0f ) );
	const Int32 topSizeActual	= static_cast< Int32 >( TopHeight * ( frameHeight / 1080.0f ) );

	// Left
	frame->AddDebugRect( left, top, leftSizeActual, frameHeight, colour );

	// Right
	const Int32 rightStartX = right - leftSizeActual;
	frame->AddDebugRect( rightStartX, top, leftSizeActual, frameHeight, colour );

	// Top
	const Int32 topX = left + leftSizeActual;
	const Int32 topWidth = frameWidth - ( leftSizeActual * 2 );
	frame->AddDebugRect( topX, top, topWidth, topSizeActual );

	// Bottom
	const Int32 bottomY = frameHeight - topSizeActual;
	frame->AddDebugRect( topX, bottomY, topWidth, topSizeActual );
}*/

void CBaseEngine::GenerateDebugFragments( CRenderFrame* frame )
{
	// Mesh coloring stuff
	if ( m_meshColoringScheme )
	{
		m_meshColoringScheme->GenerateEditorFragments( frame );
	}

	// Material debug stuff
	if ( m_materialDebugMode != MDM_None )
	{
		frame->SetMaterialDebugMode( m_materialDebugMode );
	}

	const CRenderFrameInfo& frameInfo = frame->GetFrameInfo();

	if ( frameInfo.IsShowFlagOn( SHOW_XB1SafeArea ) )
	{
		const Int32 screenWidth			= static_cast< Int32 >( frame->GetFrameInfo().m_width );
		const Int32 screenHeight		= static_cast< Int32 >( frame->GetFrameInfo().m_height );

		const Int32 overscanLeftSizeActual	= static_cast< Int32 >( 64.0f * ( screenWidth / 1920.0f ) );
		const Int32 overscanTopSizeActual	= static_cast< Int32 >( 32.0f * ( screenHeight / 1080.0f ) );

		// Overscan border (Cannot guarantee that this will be visible on a TV)
		{
			const Int32 left		= 0;
			const Int32 top			= 0;
			const Int32 frameWidth	= screenWidth;
			const Int32 frameHeight	= screenHeight;


			const Color colour( 61, 191, 33, 128 );

			// Left
			frame->AddDebugRect( left, top, overscanLeftSizeActual, frameHeight, colour );

			// Right
			const Int32 rightStartX = frameWidth - overscanLeftSizeActual;
			frame->AddDebugRect( rightStartX, top, overscanLeftSizeActual, frameHeight, colour );

			// Top
			const Int32 topX = left + overscanLeftSizeActual;
			const Int32 topWidth = frameWidth - ( overscanLeftSizeActual * 2 );
			frame->AddDebugRect( topX, top, topWidth, overscanTopSizeActual, colour );

			// Bottom
			const Int32 bottomY = frameHeight - overscanTopSizeActual;
			frame->AddDebugRect( topX, bottomY, topWidth, overscanTopSizeActual, colour );
		}

		// Action safe (No important information in this area, also may contain system UI)
		{
			const Int32 left		= overscanLeftSizeActual;
			const Int32 top			= overscanTopSizeActual;
			const Int32 frameWidth	= screenWidth - ( overscanLeftSizeActual * 2 );
			const Int32 frameHeight	= screenHeight - ( overscanTopSizeActual * 2 );

			const Int32 leftSizeActual	= static_cast< Int32 >( 36.0f * ( screenWidth / 1920.0f ) );
			const Int32 topSizeActual	= static_cast< Int32 >( 8.0f * ( screenHeight / 1080.0f ) );

			//const Color colour( 61, 191, 33 );
			const Color colour( 255, 0, 0, 128 );

			// Left
			frame->AddDebugRect( left, top, leftSizeActual, frameHeight, colour );

			// Right
			const Int32 rightStartX = ( screenWidth - overscanLeftSizeActual ) - leftSizeActual;
			frame->AddDebugRect( rightStartX, top, leftSizeActual, frameHeight, colour );

			// Top
			const Int32 topX = left + leftSizeActual;
			const Int32 topWidth = frameWidth - ( leftSizeActual * 2 );
			frame->AddDebugRect( topX, top, topWidth, topSizeActual, colour );

			// Bottom
			const Int32 bottomY = ( screenHeight - overscanTopSizeActual ) - topSizeActual;
			frame->AddDebugRect( topX, bottomY, topWidth, topSizeActual, colour );
		}
	}

	if ( frameInfo.IsShowFlagOn( SHOW_PS4SafeArea ) )
	{
		const Int32 screenWidth			= static_cast< Int32 >( frame->GetFrameInfo().m_width );
		const Int32 screenHeight		= static_cast< Int32 >( frame->GetFrameInfo().m_height );

#ifdef RED_PLATFORM_ORBIS
		Float ratio = 0.9f;
		SceSystemServiceDisplaySafeAreaInfo info;
		if( sceSystemServiceGetDisplaySafeAreaInfo( &info ) == SCE_OK )
		{
			ratio = info.ratio;
		}
#else
		Float ratio = 0.9f;
#endif

		const Int32 leftSizeActual	= ( screenWidth - static_cast< Int32 >( screenWidth * ratio ) ) / 2;
		const Int32 topSizeActual	= ( screenHeight - static_cast< Int32 >( screenHeight * ratio ) ) / 2;

		{
			const Int32 left		= 0;
			const Int32 top			= 0;
			const Int32 frameWidth	= screenWidth;
			const Int32 frameHeight	= screenHeight;


			const Color colour( 61, 191, 33, 128 );

			// Left
			frame->AddDebugRect( left, top, leftSizeActual, frameHeight, colour );

			// Right
			const Int32 rightStartX = frameWidth - leftSizeActual;
			frame->AddDebugRect( rightStartX, top, leftSizeActual, frameHeight, colour );

			// Top
			const Int32 topX = left + leftSizeActual;
			const Int32 topWidth = frameWidth - ( leftSizeActual * 2 );
			frame->AddDebugRect( topX, top, topWidth, topSizeActual, colour );

			// Bottom
			const Int32 bottomY = frameHeight - topSizeActual;
			frame->AddDebugRect( topX, bottomY, topWidth, topSizeActual, colour );
		}
	}
}

#ifndef NO_DEBUG_PAGES

void CBaseEngine::RegisterDebugCounter( IDebugCounter* counter )
{
	m_debugCounter.PushBack( counter );
}

void CBaseEngine::UnregisterDebugCounter( IDebugCounter* counter )
{	
	m_debugCounter.Remove( counter );
}

#endif


void CBaseEngine::RefreshEngine()
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "Engine refresh should be called from main thread." );

	// Reload rendering shit that may depend on performance profile
	if ( GRender )
	{
		GRender->ReloadTextures();

		//shadowmap sizes are related to preformance platform
		GRender->RecreateShadowmapResources();
	}

	// Recalculate max mip for streaming, so render textures already streamed in are refreshed
	GRender->RecalculateTextureStreamingSettings();

#ifndef NO_EDITOR
	// Update rendering crap
	CDrawableComponent::RecreateProxiesOfRenderableComponents();
#else
	CDrawableComponent::RecreateProxiesOfFurComponents();
#endif

	EDITOR_QUEUE_EVENT( CNAME( EnginePerformancePlatformChanged ), NULL );
}

void CBaseEngine::RequestRefreshViewport()
{
#ifdef RED_PLATFORM_WINPC
	Int32 width;
	Int32 height;
	EViewportWindowMode windowMode = (EViewportWindowMode)Config::cvFullScreenMode.Get();

	Config::Helper::ExtractDisplayModeValues( Config::cvResolution.Get(), width, height );

	Int32 bestWidth = width;
	Int32 bestHeight = height;

	if( windowMode == VWM_Fullscreen )
	{
		Config::Helper::GetBestMatchingResolution( width, height, bestWidth, bestHeight );

		// If best resolution is different than set resolution, this means we have to update config
		// this happens in situation like switching to monitor with different resolutions support
		if( bestWidth != width || bestHeight != height )
		{
			Config::cvResolution.Set( Config::Helper::PackDisplayModeValues( bestWidth, bestHeight ) );
		}
	}

	if( GGame != nullptr )
	{
		GGame->GetViewport()->RequestWindowSize( bestWidth, bestHeight );
		GGame->GetViewport()->RequestWindowMode( windowMode );
		GGame->GetViewport()->RequestOutputMonitor( Config::Helper::GetCurrentOutputMonitorConfig() );
	}

	if( windowMode == VWM_Fullscreen )
	{
		GInGameConfig::GetInstance().ActivateTag(CNAME(fullscreen));
	}
	else
	{
		GInGameConfig::GetInstance().DeactivateTag(CNAME(fullscreen));
	}
#endif
}

void CBaseEngine::OnGameEnded()
{
}

void CBaseEngine::SetMeshColoringScheme( IMeshColoringScheme* newColoringScheme )
{
	// Remove old mesh coloring scheme
	if ( m_meshColoringScheme )
	{
		delete m_meshColoringScheme;
		m_meshColoringScheme = NULL;
	}

	m_meshColoringScheme = newColoringScheme;
}



void CBaseEngine::FlushAllLoading()
{
	RED_ASSERT( ::SIsMainThread() );

	// Flush render
	GRender->Flush();

	// Flush any background jobs, including IO jobs and blow jobs
	SJobManager::GetInstance().FlushPendingJobs();

	GTaskManager->Flush();

	// Final flush - eg. some proxy attaches
	GRender->Flush();
}


void CBaseEngine::FlushJobs()
{
	RED_ASSERT( ::SIsMainThread() );

	// Flush render since it can issue jobs
	// But don't flush for now given how this was used.
	// Up to the flusher to call GRender->Flush or call FlushAllLoading
	//	GRender->Flush();

	// Flush this too
	SJobManager::GetInstance().FlushPendingJobs();

	GTaskManager->Flush();
}

void CBaseEngine::OnUserEvent(const EUserEvent& event)
{
	RED_FATAL_ASSERT( ::SIsMainThread(), "User events can be handled only on main thread." );

	if( event == EUserEvent::UE_SignedOut )
	{
		SConfig::GetInstance().Save();
		// After this point any 'save user settings' request will be ignored, but all requests before that, even if
		// not yet finished (or started) will be fine, as user profile manager stores settings state from before save request
		GUserProfileManager->SetIgnoreSavingUserSettings( true );		// This should be changed to UE_PostSignOut event in future
		SConfig::GetInstance().ResetUserSettings( Config::eConfigVarSetMode_Reload );	// Go back to default settings
	}
	else if( event == EUserEvent::UE_LoadSaveReady )
	{
		SConfig::GetInstance().Reload();
		GUserProfileManager->SetIgnoreSavingUserSettings( false );		// This should be changed to UE_PostSignOut event in future
	}
}

//////////////////////////////////////////////////////////////////////////

// variables for playing movies before all systems are initialised
namespace QuickBoot
{
	ViewportHandle g_quickInitViewport = ViewportHandle();
	CFlashPlayer* g_quickInitFlashPlayer = nullptr;
	IRenderVideo* g_lastBumperVideo = nullptr;
}

CBaseEngineRefreshListener::CBaseEngineRefreshListener()
	: m_refreshGraphicsRequest( false )
	, m_refreshViewportRequest( false )
{

}

void CBaseEngineRefreshListener::OnRequestRefresh(const CName& eventName)
{
	if( eventName == CNAME( refreshEngine ) )
	{
		m_refreshGraphicsRequest.SetValue( true );
	}
	else if( eventName == CNAME( refreshViewport ) )
	{
		m_refreshViewportRequest.SetValue( true );
	}
}

Bool CBaseEngineRefreshListener::GetAndClearRefreshGraphics()
{
	return m_refreshGraphicsRequest.CompareExchange( false, true );
}

Bool CBaseEngineRefreshListener::GetAndClearRefreshViewport()
{
	return m_refreshViewportRequest.CompareExchange( false, true );
}
