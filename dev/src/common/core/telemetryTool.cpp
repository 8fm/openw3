#include "build.h"
#include "telemetryTool.h"

#ifdef USE_RAD_TELEMETRY_PROFILER

#if defined(RED_PLATFORM_WINPC)
	# ifdef RED_ARCH_X64
		#pragma comment (lib, "../../../external/telemetry-2.0/lib/win/telemetry64.link.lib")
	# else
		#pragma comment (lib, "../../../external/telemetry-2.0/lib/win/telemetry32.link.lib")
	# endif
#elif defined(RED_PLATFORM_DURANGO)
	#if defined(RED_CONFIGURATION_DEBUG)
		#pragma comment (lib, "../../../external/telemetry-2.0/lib/durango/telemetrydurangod.lib")
	#else
		#pragma comment (lib, "../../../external/telemetry-2.0/lib/durango/telemetrydurango.lib")
	#endif
#elif defined(RED_PLATFORM_ORBIS)
	#if defined(RED_CONFIGURATION_DEBUG)
		#pragma comment (lib, "../../../external/telemetry-2.0/lib/orbis/libTelemetryPS4c.a")
	#else
		#pragma comment (lib, "../../../external/telemetry-2.0/lib/orbis/libTelemetryPS4.a")
	#endif
#endif

const Char* CTelemetryTool::st_name = TXT( "Telemetry" );

CTelemetryTool::CTelemetryTool(void)
{
	TmErrorCode error = TM_OK;

	tmLoadTelemetry( TM_LOAD_CHECKED_LIBRARY );

	error = tmStartup();
	if ( error != TM_OK )
	{
		RED_HALT("Couldn't start telemetry");
	}

	const Uint32 arenaSize = 2 * 1024 * 1024;
	m_arena = (TmU8*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Profiler, arenaSize );

	error = tmInitializeContext( &m_context, m_arena, arenaSize );
	if ( error != TM_OK )
	{
		// Couldn't initialize the context
		RED_HALT("Couldn't initialize the context");
	}

	RED_LOG( RADGameToolsTelemetry, TXT("Opening connection to the server") );

	StringAnsi serverName = UNICODE_TO_ANSI( Config::cvProfilerServerName.Get().AsChar() );

	if ( tmOpen( m_context, "TheWitcher3WildHunt", __DATE__ " " __TIME__, serverName.AsChar(), TMCT_TCP, TELEMETRY_DEFAULT_PORT, TMOF_INIT_NETWORKING, 1000 ) != TM_OK )
	{
		// Couldn't connect!
		RED_LOG( RADGameToolsTelemetry, TXT("Couldn't connect to server") );
	}
}

CTelemetryTool::~CTelemetryTool(void)
{
	tmClose( m_context );
	tmShutdownContext( m_context );
	tmShutdown();
	RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_arena );
}

void CTelemetryTool::BeginEvent(CProfilerBlock* block)
{
	tmEnter( m_context, TMZF_NONE, block->m_handle->m_name );
}

void CTelemetryTool::EndEvent(CProfilerBlock* block)
{
	RED_UNUSED( block );
	tmLeave( m_context );
}

void CTelemetryTool::InitializeCustomHandleData(CProfilerHandle* handle)
{
	RED_UNUSED( handle );
}

void CTelemetryTool::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	RED_UNUSED( handleTable );
	RED_UNUSED( activeHandleCount );
}

IProfilerTool* CTelemetryTool::Create()
{
	return new CTelemetryTool();
}

const Char* CTelemetryTool::GetName()
{
	return st_name;
}

SProfilerFuncPackage CTelemetryTool::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CTelemetryTool, &CTelemetryTool::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CTelemetryTool, &CTelemetryTool::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*, const Char*>::Create<CTelemetryTool, &CTelemetryTool::Message>( this ) );
}

void CTelemetryTool::Tick()
{
	tmTick( m_context );
}

void CTelemetryTool::Message(CProfilerBlock* block, const Char* msg)
{
#ifdef UNICODE
	tmMessage( m_context, TMMF_SEVERITY_LOG|TMMF_ICON_NOTE, UNICODE_TO_ANSI( msg ) );
#else
	tmMessage( m_context, TMMF_SEVERITY_LOG|TMMF_ICON_NOTE, msg );
#endif
}

#endif
