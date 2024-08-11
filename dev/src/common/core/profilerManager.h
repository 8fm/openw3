/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "profilerTypes.h"
#include "profilerToolPool.h"
#include "configVar.h"

namespace Config
{
	extern TConfigVar<String> cvActiveProfilers;
	extern TConfigVar<Int32, Validation::IntRange<0,4> > cvProfilingLevel;
	extern TConfigVar<Int32> cvProfilingChannels;
	extern TConfigVar<String> cvProfilerServerName;
}

class CProfilerHandle
{
public:
	const char* m_name;
	Int32 m_level;
	Int32 m_channels;
	Uint32 m_index;

public:
	CProfilerHandle( const char* name, Uint32 level, Uint32 channels, Uint32 index )
		: m_name( name )
		, m_level( level )
		, m_channels( channels )
		, m_index( index )
	{
	}

};

const Uint32 PROFILER_MAX_SCOPES = 4096;

class IProfilerTool
{
public:
	virtual ~IProfilerTool() {}
	virtual SProfilerFuncPackage GetFuncPackage() = 0;

	virtual const Char* GetName() = 0;
	virtual void RefreshCustomHandleData( CProfilerHandle* handleTable[PROFILER_MAX_SCOPES], Uint32 activeHandleCount ) = 0;
	virtual void InitializeCustomHandleData( CProfilerHandle* handleTable ) = 0;
	virtual void Tick() = 0;
};

class CProfilerBlock;

class CProfilerManager
{
public:
	typedef TDynArray<SProfilerFuncPackageTable*, MC_Profiler> ProfilerFuncTableArray;

private:
	CProfilerHandle*						m_handles[PROFILER_MAX_SCOPES];
	Red::Threads::CAtomic< Uint32 >			m_activeHandlerCount;
	
	SProfilerFuncPackageTable*				m_toolFuncTable;
	ProfilerFuncTableArray					m_toolFuncOldTable;
	CProfilerToolPool						m_toolPool;

	CProfilerToolPool::ProfilerToolArray	m_tools;

private:
	SProfilerFuncPackageTable* CreateDelegateTable();

public:
	CProfilerManager();
	~CProfilerManager();

	CProfilerHandle* RegisterHandler( const char* name, const Uint32 level, Uint32 channels );
	void StartProfilerBlock( CProfilerBlock* block ); //const;	FIX const after profiling profiler is done
	void StopProfilerBlock( CProfilerBlock* block ); //const;
	void Message( CProfilerBlock* block, const Char* msg );
	SProfilerFuncPackageTable* GetLastestFuncTable() const;

	Bool RegisterTool( IProfilerTool* tool );
	Bool RegisterTool( const CProfilerToolPool::ProfilerToolArray& tools );
	void SwitchTools( const CProfilerToolPool::ProfilerToolArray& tools );
	void UnregisterTool( IProfilerTool* tool );
	void UnregisterAllTools();

	void Tick();

	CProfilerToolPool& GetToolPool();
	const CProfilerToolPool::ProfilerToolArray& GetActiveProfilerTools() const;

};

typedef TSingleton<CProfilerManager> UnifiedProfilerManager;

/***** Profiler Block *****/
class CProfilerBlock : public Red::System::NonCopyable
{
public:
	const CProfilerHandle* m_handle;
	SProfilerFuncPackageTable* m_localFuncTable;
	Bool m_canExecute;

public:
	CProfilerBlock( CProfilerHandle* handle )
		: m_handle( handle )
		, m_localFuncTable( UnifiedProfilerManager::GetInstance().GetLastestFuncTable() )
	{
		UnifiedProfilerManager::GetInstance().StartProfilerBlock( this );
	}

	~CProfilerBlock()
	{
		UnifiedProfilerManager::GetInstance().StopProfilerBlock( this );
	}
};
