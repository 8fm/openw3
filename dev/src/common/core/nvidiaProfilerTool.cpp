/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "nvidiaProfilerTool.h"

#ifdef USE_NVIDIA_TOOLS

#include "../../../external/nvToolsExt/include/nvToolsExt.h"

#ifdef _WIN64
#pragma comment( lib, "../../../external/nvToolsExt/lib/x64/nvToolsExt64_1.lib" )
#else
#pragma comment( lib, "../../../external/nvToolsExt/lib/x86/nvToolsExt32_1.lib" )
#endif

const Char* CNvtxProfiler::st_name = TXT( "NvtxProfiler" );

void CNvtxProfiler::EndEvent(CProfilerBlock* block)
{
	RED_UNUSED( block );
	nvtxRangePop();
}

void CNvtxProfiler::BeginEvent(CProfilerBlock* block)
{
	nvtxRangePushA( block->m_handle->m_name );
}

void CNvtxProfiler::InitializeCustomHandleData(CProfilerHandle* handle)
{
	RED_UNUSED( handle );
}

void CNvtxProfiler::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	RED_UNUSED( handleTable );
	RED_UNUSED( activeHandleCount );
}

IProfilerTool* CNvtxProfiler::Create()
{
	return new CNvtxProfiler();
}

const Char* CNvtxProfiler::GetName()
{
	return st_name;
}

SProfilerFuncPackage CNvtxProfiler::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CNvtxProfiler, &CNvtxProfiler::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CNvtxProfiler, &CNvtxProfiler::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*, const Char*>::Create<CNvtxProfiler, &CNvtxProfiler::Message>( this ) );
}

void CNvtxProfiler::Tick()
{

}

void CNvtxProfiler::Message(CProfilerBlock* block, const Char* msg)
{

}

#endif
