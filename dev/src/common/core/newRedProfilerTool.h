/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "profiler.h"

#ifdef USE_NEW_RED_PROFILER

class CNewRedProfiler : public IProfilerTool
{
public:


	CNewRedProfiler();
	~CNewRedProfiler();
	static const Char* st_name;

	static IProfilerTool* Create();

	virtual SProfilerFuncPackage GetFuncPackage();
	virtual const Char* GetName();

	virtual void RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount);
	virtual void InitializeCustomHandleData(CProfilerHandle* handle);

	virtual void Tick();

private:
	void BeginEvent( CProfilerBlock* block );
	void EndEvent( CProfilerBlock* block );
	void Message( CProfilerBlock* block, const Char* msg );

	struct CustomHandleData
	{
		NewRedProfiler::InstrumentedFunction* instrumentedFunction;
		Uint64 startTime;
	};

	RED_INLINE CustomHandleData& GetCustomData( CProfilerBlock* block )
	{
		auto& handle = block->m_handle;
		Uint32 index = handle->m_index;
		return m_customHandleData[ index ];
	}

	RED_INLINE CustomHandleData& GetCustomData( CProfilerHandle* handle )
	{
		Uint32 index = handle->m_index;
		return m_customHandleData[ index ];
	}

private:
	Red::Threads::CMutex m_customDataInitMutex;
	static Bool m_startedFromConsole;
	CustomHandleData m_customHandleData[PROFILER_MAX_SCOPES];

};

#endif
