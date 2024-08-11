#pragma once

class IProfilerTool;

class CProfilerToolPool
{
public:
	typedef TDynArray<IProfilerTool*, MC_Profiler> ProfilerToolArray;
	typedef TDynArray<const Char*, MC_Profiler> ProfilerToolNames;

private:
	typedef IProfilerTool* (*CreationStaticMethodPtr)();
	typedef TDynArray<CreationStaticMethodPtr, MC_Profiler> ProfilerCreationMethods;

	ProfilerToolNames m_profilerNames;
	ProfilerCreationMethods m_creationMethods;
	ProfilerToolArray m_pool;

public:
	CProfilerToolPool();
	~CProfilerToolPool();

	IProfilerTool* GetTool( Uint32 idx );		// idx - creation method index
	IProfilerTool* GetTool( String name );		// name - tool's name
	const ProfilerToolArray GetDefaultTools();
	const ProfilerToolNames& GetToolNames() const;

private:
	void RegisterCreationMethod( const Char* profilerName, CreationStaticMethodPtr staticMethod );
};

