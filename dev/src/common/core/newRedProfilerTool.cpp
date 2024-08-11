/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "newRedProfilerTool.h"

#ifdef USE_NEW_RED_PROFILER

const Char* CNewRedProfiler::st_name = TXT( "NewRedProfiler" );
Bool CNewRedProfiler::m_startedFromConsole = false;

CNewRedProfiler::CNewRedProfiler()
{
	Red::MemoryZero( m_customHandleData, sizeof( CustomHandleData )  * PROFILER_MAX_SCOPES );
}

CNewRedProfiler::~CNewRedProfiler()
{
	if ( m_startedFromConsole )
	{
		SProfilerManager::GetInstance().Stop();
		SProfilerManager::GetInstance().Store( String::EMPTY );
	}
}

void CNewRedProfiler::EndEvent(CProfilerBlock* block)
{
	CustomHandleData& handleData = GetCustomData( block );
	SProfilerManager::GetInstance().EndBlock( handleData.instrumentedFunction, handleData.startTime );
}

void CNewRedProfiler::BeginEvent(CProfilerBlock* block)
{
	CustomHandleData& handleData = GetCustomData( block );
	handleData.startTime = SProfilerManager::GetInstance().StartBlock( handleData.instrumentedFunction );
}

void CNewRedProfiler::InitializeCustomHandleData(CProfilerHandle* handle)
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > sopcedLock( m_customDataInitMutex );
	CustomHandleData& handleData = GetCustomData( handle );
	if( handleData.instrumentedFunction == nullptr )
	{
		handleData.instrumentedFunction = SProfilerManager::GetInstance().RegisterInstrFunc( handle->m_name );
		handleData.startTime = 0;
	}
}

void CNewRedProfiler::RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount)
{
	for( Uint32 i=0; i<activeHandleCount; ++i )
	{
		InitializeCustomHandleData( handleTable[i] );
	}
}

IProfilerTool* CNewRedProfiler::Create()
{
	auto profiler = new CNewRedProfiler();
	m_startedFromConsole = nullptr != Red::System::StringSearch( SGetCommandLine(), TXT( "-profilerAutorun" ) );
	if ( m_startedFromConsole )
	{
		SProfilerManager::GetInstance().Start();
	}
	return profiler;
}

const Char* CNewRedProfiler::GetName()
{
	return st_name;
}

SProfilerFuncPackage CNewRedProfiler::GetFuncPackage()
{
	return SProfilerFuncPackage(
		CProfilerDelegate<CProfilerBlock*>::Create<CNewRedProfiler, &CNewRedProfiler::BeginEvent>( this ),
		CProfilerDelegate<CProfilerBlock*>::Create<CNewRedProfiler, &CNewRedProfiler::EndEvent>( this ),
		CProfilerDelegate<CProfilerBlock*, const Char*>::Create<CNewRedProfiler, &CNewRedProfiler::Message>( this ) );
}

void CNewRedProfiler::Tick()
{
	PROFILER_NextFrame();
}

void CNewRedProfiler::Message(CProfilerBlock* block, const Char* msg)
{
	CustomHandleData& handleData = GetCustomData( block );
	SProfilerManager::GetInstance().Signal( handleData.instrumentedFunction, msg );
}

#endif
