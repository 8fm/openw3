/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "intelProfilerTool.h"

#ifdef USE_INTEL_ITT

#ifdef _WIN64
#define INTELITT_LIB_PATH "..\\..\\..\\external\\IntelITT\\libs\\x64\\"
#else
#define INTELITT_LIB_PATH "..\\..\\..\\external\\IntelITT\\libs\\Win32\\"
#endif

#define USE_INTEL_2014_R2

#ifdef USE_INTEL_2014_R2
#pragma comment (lib, INTELITT_LIB_PATH "libittnotify.lib")
#else	// USE_INTEL_2014_R2
#pragma comment (lib, INTELITT_LIB_PATH "gpasdk_s.lib")
#endif	// USE_INTEL_2014_R2

const Char* CIntelITTProfiler::st_name = TXT( "IntelITTProfiler" );

void CIntelITTProfiler::EndEvent(CProfilerBlock* block)
{
	RED_UNUSED( block );
	__itt_task_end( m_intelITTDomain );
}

void CIntelITTProfiler::BeginEvent(CProfilerBlock* block)
{
	__itt_task_begin( m_intelITTDomain, __itt_null, __itt_null, m_customHandleData[block->m_handle->m_index] );
}

void CIntelITTProfiler::InitializeCustomHandleData(CProfilerHandle* handle)
{
	m_customHandleData[handle->m_index] = __itt_string_handle_createA( handle->m_name );
}

void CIntelITTProfiler::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	for( Uint32 i=0; i<activeHandleCount; ++i )
	{
		if( m_customHandleData[i] == nullptr )
		{
			InitializeCustomHandleData( handleTable[i] );
		}
	}
}

IProfilerTool* CIntelITTProfiler::Create()
{
	return new CIntelITTProfiler();
}

const Char* CIntelITTProfiler::GetName()
{
	return st_name;
}

SProfilerFuncPackage CIntelITTProfiler::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CIntelITTProfiler, &CIntelITTProfiler::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CIntelITTProfiler, &CIntelITTProfiler::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*, const Char*>::Create<CIntelITTProfiler, &CIntelITTProfiler::Message>( this ) );
}

CIntelITTProfiler::CIntelITTProfiler()
{
	CIntelITTProfiler::m_intelITTDomain = __itt_domain_createA( "GameDomain" );
}

void CIntelITTProfiler::Tick()
{

}

void CIntelITTProfiler::Message(CProfilerBlock* block, const Char* msg)
{

}

#endif
