/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "profiler.h"

#ifdef USE_RAD_TELEMETRY_PROFILER

#include "../../../external/telemetry-2.0/include/telemetry.h"

class CTelemetryTool : public IProfilerTool
{
public:
	CTelemetryTool(void);
	~CTelemetryTool(void);

	static const Char* st_name;

	static IProfilerTool* Create();

	virtual SProfilerFuncPackage GetFuncPackage();
	virtual const Char* GetName();

	virtual void RefreshCustomHandleData(CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount);
	virtual void InitializeCustomHandleData(CProfilerHandle* handle);

	virtual void Tick();

private:
	// TODO FIX add Tick method
	void BeginEvent( CProfilerBlock* block );
	void EndEvent( CProfilerBlock* block );
	void Message( CProfilerBlock* block, const Char* msg );

	TmU8* m_arena;			// Stores the telemetry memory, used in startup and shutdown
	HTELEMETRY m_context;

};

#endif
