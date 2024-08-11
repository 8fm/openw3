/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "redProfilerTool.h"

#ifdef USE_RED_PROFILER

const Char* CRedProfiler::st_name = TXT( "RedProfiler" );

CRedProfiler::CRedProfiler()
{
	CProfiler::InitializeProfiler();
}

void CRedProfiler::EndEvent(CProfilerBlock* block)
{
	RED_UNUSED( block );
	CProfiler::StopProfile();
}

void CRedProfiler::BeginEvent(CProfilerBlock* block)
{
	CProfiler::StartProfile( block->m_handle->m_name );
}

void CRedProfiler::InitializeCustomHandleData(CProfilerHandle* handle)
{
	RED_UNUSED( handle );
}

void CRedProfiler::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	RED_UNUSED( handleTable );
	RED_UNUSED( activeHandleCount );
}

IProfilerTool* CRedProfiler::Create()
{
	return new CRedProfiler();
}

const Char* CRedProfiler::GetName()
{
	return st_name;
}

SProfilerFuncPackage CRedProfiler::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CRedProfiler, &CRedProfiler::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CRedProfiler, &CRedProfiler::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*,const Char*>::Create<CRedProfiler, &CRedProfiler::Message>( this ) );
}

void CRedProfiler::Tick()
{
	CProfiler::Tick();
}

void CRedProfiler::Message(CProfilerBlock* block, const Char* msg)
{

}

#endif
