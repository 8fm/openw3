/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "profilerConfiguration.h"
#include "profilerChannels.h"
#include "profilerManager.h"
#include "configVar.h"

namespace Config
{
	TConfigVar<String> cvActiveProfilers( "Profiler", "ActiveProfilers", TXT("none"), eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<Int32, Validation::IntRange<0,4> > cvProfilingLevel( "Profiler", "ProfilingLevel", 4, eConsoleVarFlag_Developer );
	TConfigVar<Int32> cvProfilingChannels( "Profiler", "ProfilingChannels", Int32(PBC_ALL), eConsoleVarFlag_Developer );
	TConfigVar<String> cvProfilerServerName( "Profiler", "ProfilerServerName", TXT("localhost"), eConsoleVarFlag_Developer );
}

//#define PROFILE_THE_PROFILER

#ifdef PROFILE_THE_PROFILER
namespace
{
	Red::Threads::CAtomic<Uint32> g_hitsPerFrame_starts;
	Red::Threads::CAtomic<Uint32> g_elapsedTime_cur_starts;
	Red::Threads::CAtomic<Uint32> g_elapsedTime_max_starts;
	Red::Threads::CAtomic<Uint32> g_elapsedTime_min_starts;

	Red::Threads::CAtomic<Uint32> g_hitsPerFrame_stops;
	Red::Threads::CAtomic<Uint32> g_elapsedTime_cur_stops;
	Red::Threads::CAtomic<Uint32> g_elapsedTime_max_stops;
	Red::Threads::CAtomic<Uint32> g_elapsedTime_min_stops;
}
#endif // PROFILE_THE_PROFILER

CProfilerManager::CProfilerManager()
{
	m_toolFuncTable = CreateDelegateTable();

#ifdef PROFILE_THE_PROFILER
	g_elapsedTime_max_starts.SetValue( 0 );
	g_elapsedTime_min_starts.SetValue( 100000000 );
	g_elapsedTime_max_stops.SetValue( 0 );
	g_elapsedTime_min_stops.SetValue( 100000000 );
#endif // PROFILE_THE_PROFILER
}

CProfilerManager::~CProfilerManager()
{
	Uint32 count = m_activeHandlerCount.GetValue();
	for( Uint32 i=0; i<count; ++i )
	{
		delete m_handles[i];
	}

	for( Uint32 i=0; i<m_toolFuncOldTable.Size(); ++i )
	{
		delete m_toolFuncOldTable[i];
	}

	delete m_toolFuncTable;
}

SProfilerFuncPackageTable* CProfilerManager::CreateDelegateTable()
{
	Uint32 toolCount = m_tools.Size();

	SProfilerFuncPackageTable* table = new SProfilerFuncPackageTable( toolCount );

	if( toolCount != 0 )
	{
		table->Initialize();

		// Fill with existing tool delegates
		for( Uint32 i=0; i<toolCount; ++i )
		{

			table->funcs[i] = m_tools[i]->GetFuncPackage();
		}
	}

	return table;
}

CProfilerHandle* CProfilerManager::RegisterHandler(const char* name, const Uint32 level, Uint32 channels)
{
	Uint32 idx = m_activeHandlerCount.Increment();

	m_handles[idx - 1] = new CProfilerHandle( name, level, channels, idx - 1 );

	for( Uint32 i=0; i<m_tools.Size(); ++i )
	{
		m_tools[i]->InitializeCustomHandleData( m_handles[idx - 1] );
	}

	return m_handles[idx - 1];
}

void CProfilerManager::StartProfilerBlock(CProfilerBlock* block)
{
#ifdef PROFILE_THE_PROFILER
	g_hitsPerFrame_starts.Increment();
	CTimeCounter timer;
#endif

	const Int32 profilerLevel = Config::cvProfilingLevel.Get();
	const Int32 profilerActiveChannels = Config::cvProfilingChannels.Get();

	block->m_canExecute =	(block->m_handle != nullptr) &&								// there is a handle
							(block->m_handle->m_level <= profilerLevel) &&				// handle level is less or equals to current profiling level
							(block->m_handle->m_channels & profilerActiveChannels );	// there is a channel in that handle which we listen to

	if( block->m_canExecute == false )
	{
		return;
	}

	// store var
	SProfilerFuncPackageTable* const funcTab = block->m_localFuncTable;
	funcTab->refcount.Increment();
	for( Uint32 i=0; i<block->m_localFuncTable->size; ++i )
	{
		funcTab->funcs[i].begin( block );
	}

#ifdef PROFILE_THE_PROFILER
	Uint32 curTime = static_cast<Uint32>( timer.GetTimePeriodMS() * 1000000.0 );
	g_elapsedTime_cur_starts.ExchangeAdd( curTime );
	g_elapsedTime_min_starts.Exchange( curTime < g_elapsedTime_min_starts.GetValue() ? curTime : g_elapsedTime_min_starts.GetValue() );
	g_elapsedTime_max_starts.Exchange( curTime > g_elapsedTime_max_starts.GetValue() ? curTime : g_elapsedTime_max_starts.GetValue() );
#endif
}

void CProfilerManager::StopProfilerBlock(CProfilerBlock* block)
{
#ifdef PROFILE_THE_PROFILER
	g_hitsPerFrame_stops.Increment();
	CTimeCounter timer;
#endif

	if( block->m_canExecute == false )
	{
		return;
	}

	SProfilerFuncPackageTable* funcTab = block->m_localFuncTable;
	funcTab->refcount.Decrement();
	for( Uint32 i=0; i<block->m_localFuncTable->size; ++i )
	{
		funcTab->funcs[i].end( block );
	}

#ifdef PROFILE_THE_PROFILER
	Uint32 curTime = static_cast<Uint32>( timer.GetTimePeriodMS() * 1000000.0 );
	g_elapsedTime_cur_stops.ExchangeAdd( curTime );
	g_elapsedTime_min_stops.Exchange( curTime < g_elapsedTime_min_stops.GetValue() ? curTime : g_elapsedTime_min_stops.GetValue() );
	g_elapsedTime_max_stops.Exchange( curTime > g_elapsedTime_max_stops.GetValue() ? curTime : g_elapsedTime_max_stops.GetValue() );
#endif
}

void CProfilerManager::Message(CProfilerBlock* block, const Char* msg)
{
	const Int32 profilingLevel = Config::cvProfilingLevel.Get();
	const Int32 profilerActiveChannels = Config::cvProfilingChannels.Get();

	block->m_canExecute =	(block->m_handle != nullptr) &&								// there is a handle
							(block->m_handle->m_level <= profilingLevel) &&				// handle level is less or equals to current profiling level
							(block->m_handle->m_channels & profilerActiveChannels );	// there is a channel in that handle which we listen to

	if( block->m_canExecute == false )
	{
		return;
	}

	SProfilerFuncPackageTable* funcTab = block->m_localFuncTable;
	funcTab->refcount.Decrement();
	for( Uint32 i=0; i<block->m_localFuncTable->size; ++i )
	{
		funcTab->funcs[i].message( block, msg );
	}
}

SProfilerFuncPackageTable* CProfilerManager::GetLastestFuncTable() const
{
	return m_toolFuncTable;
}

// Register tool if that kind of tool is not registered yet, otherwise delete tool
Bool CProfilerManager::RegisterTool(IProfilerTool* tool)
{
	if( tool == nullptr )
	{
		return false;
	}

	// Check if tool is already in use
	for( Uint32 i=0; i<m_tools.Size(); ++i )
	{
		if( Red::System::StringCompare( tool->GetName(), m_tools[i]->GetName() ) == 0 )
		{
			return false;
		}
	}

	m_tools.PushBack( tool );
	tool->RefreshCustomHandleData( m_handles, m_activeHandlerCount.GetValue() );
	m_toolFuncOldTable.PushBack( m_toolFuncTable );
	m_toolFuncTable = CreateDelegateTable();

	return true;
}

Bool CProfilerManager::RegisterTool(const CProfilerToolPool::ProfilerToolArray& tools)
{
	if( tools.Size() == 0 )
	{
		return false;
	}

	// Check if tool is already in use
	for( Uint32 t=0; t<tools.Size(); ++t )
	{
		Bool toolRegistered = false;

		for( Uint32 i=0; i<m_tools.Size(); ++i )
		{
			if( Red::System::StringCompare( tools[t]->GetName(), m_tools[i]->GetName() ) == 0 )
			{
				toolRegistered = true;
			}
		}

		if( toolRegistered == false )
		{
			m_tools.PushBack( tools[t] );
			tools[t]->RefreshCustomHandleData( m_handles, m_activeHandlerCount.GetValue() );
		}
	}

	m_toolFuncOldTable.PushBack( m_toolFuncTable );
	m_toolFuncTable = CreateDelegateTable();

	return true;
}

void CProfilerManager::SwitchTools(const CProfilerToolPool::ProfilerToolArray& tools)
{
	m_tools.Clear();
	if( tools.Size() > 0 )
	{
		RegisterTool( tools );
	}
	else
	{
		m_toolFuncOldTable.PushBack( m_toolFuncTable );
		m_toolFuncTable = CreateDelegateTable();
	}
}

void CProfilerManager::UnregisterTool(IProfilerTool* tool)
{
	m_tools.Remove( tool );
	m_toolFuncOldTable.PushBack( m_toolFuncTable );
	m_toolFuncTable = CreateDelegateTable();
}

void CProfilerManager::UnregisterAllTools()
{
	m_tools.Clear();
	m_toolFuncOldTable.PushBack( m_toolFuncTable );
	m_toolFuncTable = CreateDelegateTable();
}

void CProfilerManager::Tick()
{
	// Check old tables and remove them if they are not used anymore
	for( Uint32 i=0; i<m_toolFuncOldTable.Size(); ++i )
	{
		if( m_toolFuncOldTable[i]->refcount.GetValue() == 0 )	// atomic compare
		{
			delete m_toolFuncOldTable[i];
			m_toolFuncOldTable.RemoveAt( i );
			--i;
		}
	}

	for( Uint32 i=0; i<m_tools.Size(); ++i )
	{
		m_tools[i]->Tick();
	}

#ifdef PROFILE_THE_PROFILER
	Double time_starts = (Double)g_elapsedTime_cur_starts.Exchange( 0 ) / (1000000.0);
	Uint32 hits_starts = g_hitsPerFrame_starts.Exchange( 0 );
	Double time_stops = (Double)g_elapsedTime_cur_stops.Exchange( 0 ) / (1000000.0);
	Uint32 hits_stops = g_hitsPerFrame_stops.Exchange( 0 );

	Double elapsedTime_starts = time_starts / (Double)hits_starts;
	Double elapsedTime_stops = time_stops / (Double)hits_stops;

	Double elapsedTime_max_starts = (Double)g_elapsedTime_max_starts.Exchange( 0 ) / (1000000.0);
	Double elapsedTime_max_stops = (Double)g_elapsedTime_max_stops.Exchange( 0 ) / (1000000.0);

	Double elapsedTime_min_starts = (Double)g_elapsedTime_min_starts.Exchange( 10000000 ) / (1000000.0);
	Double elapsedTime_min_stops = (Double)g_elapsedTime_min_stops.Exchange( 10000000 ) / (1000000.0);

	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("====== Profiler Stats [ms] ======") );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Starts Time in frame: %f"), time_starts );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Starts Hits in frame: %d"), hits_starts );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Starts Max time     : %f"), elapsedTime_max_starts );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Starts Avg time     : %f"), elapsedTime_starts );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Starts Min time     : %f"), elapsedTime_min_starts );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Stops Time in frame: %f"), time_stops );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Stops Hits in frame: %d"), hits_stops );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Stops Max time     : %f"), elapsedTime_max_stops );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Stops Avg time     : %f"), elapsedTime_stops );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Stops Min time     : %f"), elapsedTime_min_stops );
	RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Old func tables num: %d"), m_toolFuncOldTable.Size() );

	for( Uint32 i=0; i<m_toolFuncOldTable.Size(); ++i )
	{
		RED_LOG( RED_LOG_CHANNEL(Profiler), TXT("Old table%d refcount: %d"), i, m_toolFuncOldTable[i]->refcount.GetValue() );
	}

#endif // PROFILE_THE_PROFILER
}

CProfilerToolPool& CProfilerManager::GetToolPool()
{
	return m_toolPool;
}

const CProfilerToolPool::ProfilerToolArray& CProfilerManager::GetActiveProfilerTools() const
{
	return m_tools;
}

SProfilerFuncPackageTable::~SProfilerFuncPackageTable()
{
	if( size != 0 )
	{
		delete [] funcs;
	}
}

SProfilerFuncPackageTable::SProfilerFuncPackageTable(Uint32 size)
	: funcs( nullptr )
	, size( size )
{
}

SProfilerFuncPackageTable::SProfilerFuncPackageTable()
{

}

void SProfilerFuncPackageTable::Initialize()
{
	if( size != 0 )
	{
		funcs = new SProfilerFuncPackage[size];
	}
}
