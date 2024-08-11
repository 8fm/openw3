/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "profiler.h"

#ifdef USE_INTEL_ITT

#include "..\\..\\..\\external\\IntelITT\\include\\ittnotify.h"

class CIntelITTProfiler : public IProfilerTool
{
public:
	__itt_domain* m_intelITTDomain;
	__itt_string_handle* m_customHandleData[PROFILER_MAX_SCOPES];
public:
	static const Char* st_name;

	static IProfilerTool* Create();

	CIntelITTProfiler();

	virtual SProfilerFuncPackage GetFuncPackage();
	virtual const Char* GetName();

	virtual void RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount);
	virtual void InitializeCustomHandleData(CProfilerHandle* handle);

	virtual void Tick();

private:
	void BeginEvent( CProfilerBlock* block );
	void EndEvent( CProfilerBlock* block );
	void Message( CProfilerBlock* block, const Char* msg );

};

#endif
