/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderThread.h"
#include "../engine/renderCommandInterface.h"
#include "../engine/renderSettings.h"
#include "../engine/viewport.h"
#include "../engine/platformViewport.h"
#include "../engine/renderCommandBuffer.h"
#include "../engine/renderFrame.h"
#include "../redMemoryFramework/redMemorySystemMemoryStats.h"
#include "../engine/renderScaleformCommands.h"
#include "guiRenderSystemScaleform.h"
#include "renderMaterial.h"
#include "renderLoadingScreen.h"
#include "renderVideoPlayer.h"
#include "renderLoadingOverlay.h"
#include "renderTextureStreaming.h"
#include "renderEnvProbeManager.h"

#include "renderLoadingBlur.h"
#include "../engine/baseEngine.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// 1/32.f isn't quite enough to keep from dipping under 30fps
#define VSYNC_FRAME (1/35.0f)

CRenderThread *GRenderThread = NULL;

CRenderThread::CRenderThread( CRenderInterface* renderer, Bool hasMessagePump )
	: m_renderer( renderer )
	, m_hasMessagePump( hasMessagePump )
	, m_suspended( false )
{
	m_videoPlayer = new CRenderVideoPlayer;
	m_loadingScreen = new CRenderLoadingScreen;
	m_loadingOverlay = new CRenderLoadingOverlay;
	m_loadingScreenBlur = new CLoadingScreenBlur();

}

CRenderThread::~CRenderThread()
{
	Shutdown();
}

// This first nulls pointer, and then delete it
#define SAFE_DELETE(a)				\
if( a != nullptr )					\
{									\
	auto tmp = a;					\
	a = nullptr;					\
	delete tmp;						\
}

void CRenderThread::Shutdown()
{
	SAFE_DELETE(m_loadingScreenBlur);
	SAFE_DELETE(m_loadingOverlay);
	SAFE_DELETE(m_loadingScreen);
	SAFE_DELETE(m_videoPlayer);
}

void CRenderThread::RequestExit()
{
	// Write null command to exit the render thread
	{
		void* mem = GetCommandBuffer().Alloc( 16 );
		*(uintptr_t*)( mem ) = 0;
		GetCommandBuffer().Commit( mem );
	}
}

Bool CRenderThread::IsNonInteractiveRenderingEnabled() const
{
	return ( m_loadingScreen->IsActive() || m_loadingScreenBlur->IsActive() || m_videoPlayer->IsPlayingBumpers() ) && !m_suspended;
}

void CRenderThread::ProcessLoadingScreenBlur( Float timeDelta )
{
	RED_FATAL_ASSERT( m_loadingScreenBlur != nullptr , "It shouldn't be nullptr" );

	// This can happen when we unloaded some resources
	if(  m_loadingScreenBlur->GetViewport() == nullptr )
	{
		return;
	}

	IViewport* const nonInteractiveViewport = m_loadingScreenBlur->GetViewport();
	RED_FATAL_ASSERT( nonInteractiveViewport, "Loading screen fence has no viewport!" );

	// Initialize fake frame
	// Use this ctor to avoid crashing on RefreshCameraParams and EndGame when the world is unloading
	CRenderFrameInfo info;
	info.SetViewport( nonInteractiveViewport );
	info.m_width = nonInteractiveViewport->GetWidth();
	info.m_height = nonInteractiveViewport->GetHeight();
	info.m_renderingMode = nonInteractiveViewport->GetRenderingMode();

	Red::System::MemoryCopy( info.m_renderingMask, nonInteractiveViewport->GetRenderingMask(), sizeof( Bool ) * SHOW_MAX_INDEX );
	info.SetShowFlag( SHOW_GUI, false );
	info.SetShowFlag( SHOW_PostProcess, false );	
	info.m_isNonInteractiveRendering = true;
	info.m_isLastFrameForced = true;
#if MICROSOFT_ATG_DYNAMIC_SCALING	
	GpuApi::RevertToPreviousDynamicScale();
#endif

	// Create frame to render
	CRenderFrame* fakeFrame = GetRenderer()->CreateFrame( NULL, info );
	if ( fakeFrame )
	{
		GetRenderer()->RenderFrame( fakeFrame, NULL );

		m_loadingScreenBlur->Tick( timeDelta );

		fakeFrame->Release();
	}

#ifdef RED_PLATFORM_WINPC
	Red::Threads::SleepOnCurrentThread( 5 );
#endif


}

#ifdef USE_ANSEL
Bool CRenderThread::IsPlayingVideo() const
{
	if ( !m_videoPlayer )
	{
		return false;
	}

	return m_videoPlayer->IsPlayingVideo();
}
#endif // USE_ANSEL

void CRenderThread::ProcessNonInteractiveRendering( Float timeDelta, Float unclampedTimeDelta )
{
	RED_FATAL_ASSERT( m_loadingScreen != nullptr && m_loadingScreen != nullptr && m_videoPlayer != nullptr , "It shouldn't be nullptr" );

	// Redraw the non interactive viewport
	RED_FATAL_ASSERT( m_loadingScreen->IsActive() || m_videoPlayer->IsPlayingBumpers(), "ProcessNonInteractiveRendering() called while no loading screen active. DEBUG THIS NOW" );

	IViewport* const nonInteractiveViewport = m_loadingScreen->GetViewport() ? m_loadingScreen->GetViewport() : QuickBoot::g_quickInitViewport.Get();
	RED_FATAL_ASSERT( nonInteractiveViewport, "Loading screen fence has no viewport!" );

	// Initialize fake frame
	// Use this ctor to avoid crashing on RefreshCameraParams and EndGame when the world is unloading
	CRenderFrameInfo info;
	info.SetViewport( nonInteractiveViewport );
	info.m_width = nonInteractiveViewport->GetWidth();
	info.m_height = nonInteractiveViewport->GetHeight();
	info.m_renderingMode = nonInteractiveViewport->GetRenderingMode();

	Red::System::MemoryCopy( info.m_renderingMask, nonInteractiveViewport->GetRenderingMask(), sizeof( Bool ) * SHOW_MAX_INDEX );
	info.SetShowFlag( SHOW_GUI, false );
	info.SetShowFlag( SHOW_PostProcess, false );	
	info.m_isNonInteractiveRendering = true;

	if (m_videoPlayer->IsPlayingBumpers())
	{
		// simpler rendering code path which is perfect for rendering intro movies...
		info.m_isLastFrameForced = true;
	}

	// Create frame to render
	CRenderFrame* fakeFrame = GetRenderer()->CreateFrame( NULL, info );
	if ( fakeFrame )
	{
#ifndef RED_FINAL_BUILD
		CRenderLoadingScreen::SDebugInfo& debugInfo = m_loadingScreen->GetDebugInfo();

		Uint32 top = fakeFrame->GetFrameOverlayInfo().m_height - 130;

		Char stats[ 256 ];

		// do not attempt to render text if the game is still booting
		if (!m_videoPlayer->IsPlayingBumpers())
		{
			// Draw some debug text
			fakeFrame->AddDebugScreenText( 40, top, debugInfo.m_caption.AsChar() ); top += 15;
			//fakeFrame->AddDebugScreenText( 40, top, m_loadingStatus); top += 15;
		
			// Video info
			{
				const Char* state = TXT("");
				Color clr = Color::WHITE;
				if ( debugInfo.m_videoInfo.m_videoFailed )
				{
					state = TXT("(FAILED)");
					clr = Color::RED;
				}
				else if ( debugInfo.m_videoInfo.m_videoFinished )
				{
					state = TXT("(Finished)");
					clr = Color::YELLOW;
				}

				const Char* videoName = debugInfo.m_videoInfo.m_videoToPlay.AsChar();
				Red::System::SNPrintF( stats, ARRAY_COUNT_U32( stats ), TXT("Video: %ls %ls"), videoName, state );

				fakeFrame->AddDebugScreenText( 40, top, stats, clr ); top += 15;
			}

			// Memory
			{
				Red::MemoryFramework::SystemMemoryInfo memoryInfo = Red::MemoryFramework::SystemMemoryStats::RequestStatistics();
				Red::System::SNPrintF( stats, ARRAY_COUNT_U32( stats ), TXT("Physical: %1.2f MB Free ( %1.2f MB total )"), memoryInfo.m_freePhysicalBytes / ( 1024.0f * 1024.0f ), memoryInfo.m_totalPhysicalBytes  / ( 1024.0f * 1024.0f ) );
				fakeFrame->AddDebugScreenText( 40, top, stats ); top += 15;

#				ifdef ENABLE_GAMESAVE_POOL_DEBUG
					const Float oneMB = 1024*1024;
					const Float invOneMB = 1.0f/oneMB;
					const Int32 c_oneLineHeight = 12;
					Red::MemoryFramework::RuntimePoolMetrics gameSavePoolMetrics = SRedMemory::GetInstance().GetMetricsCollector().GetMetricsForPool( MemoryPool_GameSave );
					Red::MemoryFramework::AllocatorInfo gameSavePoolInfo;
					RED_MEMORY_GET_ALLOCATOR( MemoryPool_GameSave )->RequestAllocatorInfo( gameSavePoolInfo );

					String gameSavePoolSummary = String::Printf( TXT( "GameSave Pool: %.2fmb free (%.2fmb free at peak usage)" ), 
																( gameSavePoolInfo.GetBudget() - gameSavePoolMetrics.m_totalBytesAllocated ) * invOneMB,
																( gameSavePoolInfo.GetBudget() - gameSavePoolMetrics.m_totalBytesAllocatedPeak ) * invOneMB );
					fakeFrame->AddDebugScreenText( 40, top, gameSavePoolSummary, Color::LIGHT_YELLOW );	top += c_oneLineHeight;
#				endif
			}

			// Timing
			{
				const Float timeSoFar = ( Float )m_loadingScreen->GetLoadingDuration();
				const Uint64 bytesSoFar = 0;//CDependencyLoader::GetNumBytesLoadedSoFar() - m_loadingStartBytes;

				// Format text
				// FIXME: collectin I/O stats need updating
	// 				Red::System::SNPrintF( stats, ARRAY_COUNT_U32( stats ), TXT("IO: %4.2fMB"), bytesSoFar / 1048576.0f );
	// 				fakeFrame->AddDebugScreenText( 40, top, stats ); top += 15;

				// Format text
				Red::System::SNPrintF( stats, ARRAY_COUNT_U32( stats ), TXT("Loading time: %2.1fs"), timeSoFar );
				fakeFrame->AddDebugScreenText( 40, top, stats ); top += 15;
			}
		}
#endif
		const Bool wasLoadingScreenActive = m_loadingScreen->IsActive();
		if ( wasLoadingScreenActive )
		{
			m_loadingScreen->Tick( timeDelta, unclampedTimeDelta );
		}


		if ( !m_loadingScreen->IsActive() && wasLoadingScreenActive )
		{
			// when fading out to another color, don't briefly flash the black non-interative rendering background
			fakeFrame->GetFrameInfo().m_forceFade = true;
		}

		// Draw fake frame
		GetRenderer()->RenderFrame( fakeFrame, NULL );
		fakeFrame->Release();
	}
}

namespace Hacks
{
	extern void RenderAudioForLoadingScreen();
}

/// Fast time counter - EngineTime was to slow
class RenderThreadTime
{
public:
	RED_FORCE_INLINE RenderThreadTime()
		: m_time( 0 )
	{}

	RED_FORCE_INLINE RenderThreadTime( const Double time )
	{
		m_time = (Uint64)( time * s_freq );
	}

	RED_FORCE_INLINE RenderThreadTime( Uint64 ticks )
		: m_time( ticks )
	{
	}

	RED_FORCE_INLINE static RenderThreadTime GetNow()
	{
		RenderThreadTime result;
		result.SetNow();
		return result;
	}

	RED_FORCE_INLINE Double operator-( const RenderThreadTime& other ) const
	{
		return ((Double)m_time - (Double)other.m_time) / s_freq;
	}

	RED_FORCE_INLINE const Bool operator<( const RenderThreadTime& other ) const
	{
		return m_time < other.m_time;
	}

	RED_FORCE_INLINE const Bool operator>( const RenderThreadTime& other ) const
	{
		return m_time > other.m_time;
	}

	RED_FORCE_INLINE RenderThreadTime operator+( const RenderThreadTime& other ) const
	{
		return RenderThreadTime( m_time + other.m_time );
	}

	RED_FORCE_INLINE void SetNow()
	{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
		QueryPerformanceCounter( (LARGE_INTEGER*) &m_time);
#elif defined( RED_PLATFORM_ORBIS )
		m_time = ::sceKernelReadTsc();
#endif
	}

	static void Init()
	{
#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
		LARGE_INTEGER freq;
		QueryPerformanceFrequency( &freq );
		s_freq  = Double( freq.QuadPart );
#elif defined( RED_PLATFORM_ORBIS )
		s_freq = ::sceKernelGetTscFrequency();
#endif
	}

private:
	static Double	s_freq;
	Uint64			m_time;
};

Double RenderThreadTime::s_freq = 0.0;

// On a define until tested for other platforms!
#ifdef RED_PLATFORM_DURANGO
#define UNTHROTTLE_RENDER_COMMANDS_DURING_LOADING
#endif

Bool CRenderThread::ProcessCommandBatch( const RenderThreadTime& currentTime )
{
	PC_SCOPE( ProcessingRenderCommands );

	const RenderThreadTime minTimeLimit = currentTime + RenderThreadTime( 0.005f ); // always spin for at least 5ms
	const RenderThreadTime maxTimeLimit = currentTime + RenderThreadTime( VSYNC_FRAME ); // do not allow burst of commands longer than 1/32 of a frame without a tick

	for ( ;; )
	{
		Bool breakCmds = false;

		const Uint32 RENDER_COMMAND_BATCH = 100;
		Uint32 commandNum = 0;
		while ( GetCommandBuffer().HasData() )
		{
			PC_SCOPE( ExecuteCommand );

			// Pop and process message
			IRenderCommand* command = ( IRenderCommand* ) GetCommandBuffer().ReadData();

			// No command, exit
			if ( *(uintptr_t*)( command ) == 0 )
			{
				LOG_RENDERER( TXT("Rendering thread stopped") );
				return false;
			}

			// Process command
			command->Execute();

			// Destroy command
			command->~IRenderCommand();

#ifdef UNTHROTTLE_RENDER_COMMANDS_DURING_LOADING
			if( IsNonInteractiveRenderingEnabled() || (m_videoPlayer && m_videoPlayer->IsPlayingVideo()) )
			{
				const RenderThreadTime time = RenderThreadTime::GetNow();
				if ( time > maxTimeLimit )
				{
					breakCmds = true;
					break;
				}
			}
			if ( !IsNonInteractiveRenderingEnabled() && commandNum++ >= RENDER_COMMAND_BATCH )
			{
				breakCmds = true;
				break;
			}
#else
			if ( commandNum++ >= RENDER_COMMAND_BATCH )
			{
				breakCmds = true;
				break;
			}
			if( IsNonInteractiveRenderingEnabled() || (m_videoPlayer && m_videoPlayer->IsPlayingVideo()) )
			{
				const RenderThreadTime time = RenderThreadTime::GetNow();
				if ( time > maxTimeLimit )
				{
					breakCmds = true;
					break;
				}
			}
#endif
		}

		if ( breakCmds )
		{
			break;
		}

		// reevaluate
		const RenderThreadTime time = RenderThreadTime::GetNow();
		if ( time < minTimeLimit )
		{
			// keep going regardless if we have data or not
		}
		else if ( time > maxTimeLimit || !GetCommandBuffer().HasData() )
		{
			// limit reached or no data
			break;
		}
	}

	return true;
}

void CRenderThread::ThreadFunc()
{
	Memory::RegisterCurrentThread();

	// Thread started
	LOG_RENDERER( TXT("Rendering thread started") );
	GRenderThread = this;

	// Initialize exact timer
	RenderThreadTime::Init();

	// Time of last tick
	RenderThreadTime lastRenderTick = RenderThreadTime::GetNow();

	// Message process function
	Bool running = true;

	while ( running )
	{
		PC_SCOPE( CRenderThreadFunc );

		Bool canRender = false;
		const Float minTimeDelta = Config::cvVSync.Get() ? VSYNC_FRAME : 1.0f / 30.0f;
		const RenderThreadTime currentTime = RenderThreadTime::GetNow();
		Float clampedTimeDelta = 0.f;
		Float unclampedTimeDelta = static_cast< Float >( currentTime - lastRenderTick );

		if ( unclampedTimeDelta >= minTimeDelta )
		{
			canRender = true;
			clampedTimeDelta = Min< Float >( unclampedTimeDelta, 1.0f / 15.0f );
			lastRenderTick = currentTime;
		}

		if ( canRender || m_videoPlayer->IsPlayingVideo() )
		{
			// Tick regular amount to avoid mapping/locking a texture just to not bother updating it. Makes a big difference on PC.
			// Move the decoding into another thread to be less dependent on the vsync
			m_videoPlayer->Tick( unclampedTimeDelta );
			m_videoPlayer->LockVideoThread( m_loadingScreen->IsActive() || m_videoPlayer->IsPlayingBumpers(), eVideoThreadIndex_GameThread );
		}

		if ( canRender )
		{
			m_loadingOverlay->Tick( clampedTimeDelta, unclampedTimeDelta );

			// Prefetch tick
			if ( running && GetRenderer()->GetEnvProbeManager() )
			{
				PC_SCOPE( EnvProbesPrefetch );
				GetRenderer()->GetEnvProbeManager()->UpdatePrefetch();
			}
		}

		running = ProcessCommandBatch( currentTime );

#ifdef RED_PLATFORM_ORBIS
		// This is a hack intended to shut the fuck up PS4 with it's "SubmitDone not called". Since they developed a solution that's rubbish (quoting their own engineers ...), I'm introducing this rubbish solution to
		// just make it quiet. Cert saved...
		static RenderThreadTime lastTime = currentTime;
		const Float timeDifference = static_cast<Float>( currentTime - lastTime ); 
		lastTime = currentTime;
		if ( GpuApi::PS4_GetAndUpdateTimeSinceSubmitDone( timeDifference ) > 1.0f )
		{
			GpuApi::PS4_SubmitDone();
			LOG_RENDERER( TXT("[PS4 rubbish] Called submitDone because having to periodically call some function with no true reason is good, the world is saved.") );
		}
#endif

		// Non interactive rendering
		if ( IsNonInteractiveRenderingEnabled() )
		{
			PC_SCOPE( NonInteractiveRendering );

			// Always update if playing video to avoid stuttering
			if( (canRender && m_loadingScreen->IsActive()) || m_videoPlayer->IsPlayingBumpers() )
			{
				// Process and draw the loading screen
				ProcessNonInteractiveRendering( clampedTimeDelta, unclampedTimeDelta );
			}
			else if( canRender && m_loadingScreenBlur->IsActive() )
			{
				// Process and draw the loading screen blur effect
				ProcessLoadingScreenBlur( clampedTimeDelta );
			}

			if (!m_videoPlayer->IsPlayingBumpers())
			{
				// Don't spam this or else we'll drop FPS
				if ( canRender )
				{
					// Update texture streaming - needed because we can prefetch textures under the loading screen
					GetRenderer()->GetTextureStreamingManager()->TickDuringSuppressedRendering();
				}
			}
			else
			{
				Hacks::RenderAudioForLoadingScreen();
			}
		}
	}

	// Delete thread
	GRenderThread = NULL;
}


