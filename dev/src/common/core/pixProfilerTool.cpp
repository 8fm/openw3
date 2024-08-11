/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pixProfilerTool.h"

#ifdef USE_PIX

#include "pix.h"
#pragma comment( lib, "PIXEvt.lib" )

const Char* CPixProfiler::st_name = TXT( "PIXProfiler" );

CPixProfiler::CPixProfiler()
{
	Red::MemoryZero( m_handleUnicodeNames, PROFILER_MAX_SCOPES * sizeof(UniChar*) );
}

CPixProfiler::~CPixProfiler()
{
	for( Uint32 i=0; i<PROFILER_MAX_SCOPES; ++i )
	{
		if( m_handleUnicodeNames[i] != nullptr )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_Profiler, m_handleUnicodeNames[i] );
		}
	}
}

void CPixProfiler::EndEvent(CProfilerBlock* block)
{
	RED_UNUSED( block );
	PIXEndEvent();
}

void CPixProfiler::BeginEvent(CProfilerBlock* block)
{
	PIXBeginEvent( PIX_COLOR( 0, 0, 0 ), m_handleUnicodeNames[block->m_handle->m_index] );
}

void CPixProfiler::InitializeCustomHandleData(CProfilerHandle* handle)
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > sopcedLock( m_customDataInitMutex );
	if( m_handleUnicodeNames[handle->m_index] == nullptr )
	{
		UniChar* nameUnicode = ANSI_TO_UNICODE( handle->m_name );
		size_t memSize = ( Red::System::StringLength( nameUnicode, 512 ) + 1 ) * sizeof(UniChar);
		m_handleUnicodeNames[handle->m_index] = (UniChar*)RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Profiler, memSize );
		Red::System::StringCopy( m_handleUnicodeNames[handle->m_index], nameUnicode, memSize, memSize );
	}
}

void CPixProfiler::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	for( Uint32 i=0; i<activeHandleCount; ++i )
	{
		InitializeCustomHandleData( handleTable[i] );
	}
}

IProfilerTool* CPixProfiler::Create()
{
	return new CPixProfiler();
}

const Char* CPixProfiler::GetName()
{
	return st_name;
}

SProfilerFuncPackage CPixProfiler::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CPixProfiler, &CPixProfiler::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CPixProfiler, &CPixProfiler::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*, const Char*>::Create<CPixProfiler, &CPixProfiler::Message>( this ) );
}

void CPixProfiler::Tick()
{

}

void CPixProfiler::Message(CProfilerBlock* block, const Char* msg)
{

}

#endif
