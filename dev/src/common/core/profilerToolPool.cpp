#include "build.h"
#include "profilerToolPool.h"

#include "redProfilerTool.h"
#include "newRedProfilerTool.h"
#include "intelProfilerTool.h"
#include "pixProfilerTool.h"
#include "nvidiaProfilerTool.h"
#include "razorProfilerTool.h"
#include "telemetryTool.h"

CProfilerToolPool::CProfilerToolPool()
{
#ifdef USE_RED_PROFILER
	RegisterCreationMethod( CRedProfiler::st_name, &CRedProfiler::Create );
#endif
#ifdef USE_NEW_RED_PROFILER
	RegisterCreationMethod( CNewRedProfiler::st_name, &CNewRedProfiler::Create );
#endif
#ifdef USE_INTEL_ITT
	RegisterCreationMethod( CIntelITTProfiler::st_name, &CIntelITTProfiler::Create );
#endif
#ifdef USE_PIX
	RegisterCreationMethod( CPixProfiler::st_name, &CPixProfiler::Create );
#endif
#ifdef USE_NVIDIA_TOOLS
	RegisterCreationMethod( CNvtxProfiler::st_name, &CNvtxProfiler::Create );
#endif
#ifdef USE_RAZOR_PROFILER
	RegisterCreationMethod( CRazorProfilerTool::st_name, &CRazorProfilerTool::Create );
#endif
#ifdef USE_RAD_TELEMETRY_PROFILER
	RegisterCreationMethod( CTelemetryTool::st_name, &CTelemetryTool::Create );
#endif
}

CProfilerToolPool::~CProfilerToolPool()
{
	m_pool.ClearPtr();		// Delete all tools
}

void CProfilerToolPool::RegisterCreationMethod(const Char* profilerName, CreationStaticMethodPtr staticMethod)
{
	m_profilerNames.PushBack( profilerName );
	m_creationMethods.PushBack( staticMethod );
}

IProfilerTool* CProfilerToolPool::GetTool(Uint32 idx)
{
	// Check if tool has been created already
	for( Uint32 i=0; i<m_pool.Size(); ++i )
	{
		if( m_profilerNames[idx] == m_pool[i]->GetName() )		// Compare pointers to Char
		{
			return m_pool[i];
		}
	}

	// If not - create it
	IProfilerTool* tool = m_creationMethods[idx]();
	m_pool.PushBack(tool);
	return tool;
}

IProfilerTool* CProfilerToolPool::GetTool(String name)
{
	// Check if tool has been created already
	for( Uint32 i=0; i<m_pool.Size(); ++i )
	{
		if( name == String( m_pool[i]->GetName() ) )
		{
			return m_pool[i];
		}
	}

	// If not - create it (if the name is found)
	for( Uint32 i=0; i<m_profilerNames.Size(); ++i )
	{
		if( String( m_profilerNames[i] ) == name )
		{
			IProfilerTool* tool = m_creationMethods[i]();
			m_pool.PushBack(tool);
			return tool;
		}
	}

	return nullptr;
}

const CProfilerToolPool::ProfilerToolNames& CProfilerToolPool::GetToolNames() const
{
	return m_profilerNames;
}

const CProfilerToolPool::ProfilerToolArray CProfilerToolPool::GetDefaultTools()
{
	CProfilerToolPool::ProfilerToolArray result;

	// Default for all platforms
#ifdef USE_RED_PROFILER
	result.PushBack( GetTool( CRedProfiler::st_name ) );
#endif
#ifdef USE_NEW_RED_PROFILER
	result.PushBack( GetTool( CNewRedProfiler::st_name ) );
#endif

#if defined(RED_PLATFORM_WINPC)
	// Old and new red profilers (all platforms have them already)
#elif defined(RED_PLATFORM_DURANGO)
#ifdef USE_PIX
	result.PushBack( GetTool( CPixProfiler::st_name ) );
#endif
#elif defined(RED_PLATFORM_ORBIS)
#ifdef USE_RAZOR_PROFILER
	result.PushBack( GetTool( CRazorProfilerTool::st_name ) );
#endif
#endif

	return result;
}
