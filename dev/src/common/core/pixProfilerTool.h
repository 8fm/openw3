/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "profiler.h"

#ifdef USE_PIX

class CPixProfiler : public IProfilerTool
{
public:
	static const Char* st_name;
	UniChar* m_handleUnicodeNames[PROFILER_MAX_SCOPES];

	CPixProfiler();
	~CPixProfiler();
	virtual SProfilerFuncPackage GetFuncPackage();
	virtual const Char* GetName();
	static IProfilerTool* Create();
	virtual void RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount);
	virtual void InitializeCustomHandleData(CProfilerHandle* handle);
	virtual void Tick();

public:
	void BeginEvent( CProfilerBlock* block );
	void EndEvent( CProfilerBlock* block );
	void Message( CProfilerBlock* block, const Char* msg );

	Red::Threads::CMutex m_customDataInitMutex;
};

#endif
