/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "razorProfilerTool.h"

#ifdef USE_RAZOR_PROFILER

#include <perf.h>
#pragma comment( lib, "libScePerf_stub_weak.a" )

const Char* CRazorProfilerTool::st_name = TXT( "RazorProfiler" );

void CRazorProfilerTool::EndEvent(CProfilerBlock* block)
{
	RED_UNUSED( block );
	sceRazorCpuPopMarker();
}

void CRazorProfilerTool::BeginEvent(CProfilerBlock* block)
{
	sceRazorCpuPushMarker( block->m_handle->m_name, SCE_RAZOR_COLOR_RED, SCE_RAZOR_MARKER_DISABLE_HUD );
}

void CRazorProfilerTool::InitializeCustomHandleData(CProfilerHandle* handle)
{
	RED_UNUSED( handle );
}

void CRazorProfilerTool::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	RED_UNUSED( handleTable );
	RED_UNUSED( activeHandleCount );
}

IProfilerTool* CRazorProfilerTool::Create()
{
	return new CRazorProfilerTool();
}

const Char* CRazorProfilerTool::GetName()
{
	return st_name;
}

SProfilerFuncPackage CRazorProfilerTool::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CRazorProfilerTool, &CRazorProfilerTool::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CRazorProfilerTool, &CRazorProfilerTool::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*, const Char*>::Create<CRazorProfilerTool, &CRazorProfilerTool::Message>( this ) );
}

void CRazorProfilerTool::Tick()
{

}

void CRazorProfilerTool::Message(CProfilerBlock* block, const Char* msg)
{

}

#endif
