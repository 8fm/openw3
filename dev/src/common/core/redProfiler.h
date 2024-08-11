/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "profilerTypes.h"
#include "../redThreads/redThreadsAtomic.h"

/**************************************/
/* PerfTimer						  */
/**************************************/
class CPerfCounter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Debug );

private:
	CPerfCounter*		m_parent;
	CPerfCounter*		m_child;
	CPerfCounter*		m_next;
	const char*			m_name;
	Uint32				m_recursionCount;
	Uint32				m_totalCount;
	Uint64				m_startTime;
	Uint64				m_totalTime;		//double?

public:
	//! Get parent profiler from profiler hierarchy
	RED_INLINE CPerfCounter* GetParent() const { return m_parent; }

	//! Get first child from profiler hierarchy
	RED_INLINE CPerfCounter* GetFirstChild() const { return m_child; }

	//! Get next child profiler
	RED_INLINE CPerfCounter* GetSibling() const { return m_next; }

	//! Get the profiler name
	RED_INLINE const char* GetName() const { return m_name; }

	//! Get total time spent in profiler
	RED_INLINE Uint64 GetTotalTime() const { return m_totalTime; }

	//! Get hit count
	RED_INLINE Uint32 GetTotalCount() const { return m_totalCount; }


public:
	CPerfCounter( const char* name, CPerfCounter* parent )
		: m_parent( parent )
		, m_child( NULL )
		, m_next( NULL )
		, m_name( name )
		, m_recursionCount( 0 )
		, m_totalCount( 0 )
		, m_startTime( 0 )
		, m_totalTime( 0 )
	{
		// Register in parent profiler
		if ( parent )
		{
			this->m_next = parent->m_child;
			parent->m_child = this;
		}
	}

public:
	//! Get child profiler by name
	inline CPerfCounter* GetChild( const char* name );

	//! Enter profiler scope
	inline void Enter();

	//! Leave profiler scope
	inline bool Return();

public:
	//! Reset profiler to zero, recursive
	void Reset();

public:
	//! Calculate statistics value for this counter
	static void CalcStats( CPerfCounter* counter, double& percentParent, double& time );
};

/**************************************/
/* CProfiler						  */
/**************************************/

namespace Consts
{
#ifdef USE_RED_PROFILER
	// Maximum count of root performance counters
	const Uint32 MaxRoots = 128;

	// Maximum count of profiler performance counters
	const Uint32 MaxProfilers = 32768;
#else
	// Maximum count of root performance counters
	const Uint32 MaxRoots = 0;

	// Maximum count of profiler performance counters
	const Uint32 MaxProfilers = 0;
#endif
}

/// Profiler singleton
class CProfiler
{
private:
	RED_TLS static CPerfCounter*					st_CurNode;			//!< Current profiling node
	static Uint32									st_TickCount;		//!< Current tick count, incremented every frame
	static const char*								st_ThreadRootName;	//!< Name of the thread root node
	static TDynArray< CPerfCounter*, MC_Profiler >	st_roots;			//!< Registered profiling roots
	static Red::Threads::CAtomic<Uint32>			st_rootsCount;		//!< Current count of roots, can be only incremented
	static TDynArray< CPerfCounter*, MC_Profiler >	st_profilers;		//!< All registered profilers
	static Red::Threads::CAtomic<Uint32>			st_profilersCount;	//!< Current count of profilers, can be only incremented

public:
	static void InitializeProfiler()
	{
		Red::System::MemoryZero( st_roots.Data(), st_roots.DataSize() );
		Red::System::MemoryZero( st_profilers.Data(), st_profilers.DataSize() );
	}

	//! Create profiler node
	static CPerfCounter* CreateNode( const char* name, CPerfCounter* parent = NULL )
	{
		CPerfCounter* node = nullptr;
#ifdef USE_RED_PROFILER
		// Create profiler
		node = new CPerfCounter( name, parent );

		// Register in the root list if it's a root profiler or in the general profiler list
		if ( name == st_ThreadRootName )
		{
			Uint32 index = st_rootsCount.ExchangeAdd( 1 );
			if( index < Consts::MaxRoots )
			{
				st_roots[index] = node;
			}
			else
			{
				RED_WARNING_ONCE( false, "Limit of root performance counters is reached." );
			}
		}
		else
		{
			Uint32 index = st_profilersCount.ExchangeAdd( 1 );
			if( index < Consts::MaxProfilers )
			{
				st_profilers[index] = node;
			}
			else
			{
				RED_WARNING_ONCE( false, "Limit of profiler performance counters is reached." );
			}
		}
#else
		RED_UNUSED( name );
		RED_UNUSED( parent );
#endif

		// Return created profiler
		return node;
	}

	//! Start profiling given context
	static void StartProfile( const char* name )
	{
		// Root case
		if ( !st_CurNode )
		{
			st_CurNode = CreateNode( st_ThreadRootName );
			st_CurNode = CreateNode( name, st_CurNode );
		}

		// Different profiler
		if ( st_CurNode->GetName() != name )
		{
			// Create a child
			st_CurNode = st_CurNode->GetChild( name );
		}

		// Start profiling
		st_CurNode->Enter();
	}

	//! Stop profiling given context
	static void StopProfile()
	{
		// Stop profiling
		if ( st_CurNode->Return() )
		{
			st_CurNode = st_CurNode->GetParent();
		}
	}

	//! New tick
	static void Tick()
	{
		++st_TickCount;
	}

	//! Reset profilers
	static void Reset()
	{			
		st_TickCount = 0;
		Uint32 rootsCount = GetThreadCount();
		for ( Uint32 i=0; i < rootsCount; ++i )
		{
			st_roots[i]->Reset();
		}
	}

	//! Get number of profilers
	static Uint32 GetCountersCount()
	{
		return Clamp<Uint32>( st_profilersCount.GetValue(), 0, Consts::MaxProfilers );
	}

	//! Get number of thread roots
	static Uint32 GetThreadCount()
	{
		return Clamp<Uint32>( st_rootsCount.GetValue(), 0, Consts::MaxProfilers );
	}

	//! Get n-th thread root
	static CPerfCounter* GetThreadRoot( Uint32 i )
	{
		return st_roots[ i ];
	}

	//! Get n-th profiler
	static CPerfCounter* GetPerfCounter( Uint32 i )
	{
		return st_profilers[ i ];
	}

	//! Get current tick count
	static Uint32 GetTickCount()
	{
		return st_TickCount;
	}

	//! Get counter by name
	static CPerfCounter* GetCounter( const char* name );
	static void GetCounters( const char* name, TDynArray< CPerfCounter* >& counters, Int32 root );
};
/////////////////////////////////////////////////////////////////////////

// Size of stats trace
#define STAT_SIZE 10

// Stats generator for profiler
class CProfilerStatBox
{
private:
	Uint64			m_totalTime[ STAT_SIZE ];
	Uint64			m_totalCount[ STAT_SIZE ];
	Uint64			m_lastTime;
	Uint64			m_lastCount;
	double			m_freq;
	int				m_current;
	CPerfCounter*	m_counter;

public:
	//! Get the assigned profiler
	RED_INLINE const CPerfCounter* GetPerfCounter() const { return m_counter; }

public:
	//! Initialize for given profiler
	CProfilerStatBox( CPerfCounter* counter )
		: m_counter( counter )
	{
		Reset();
	}

	//! Reset profiler stats generator
	void Reset();

	//! Tick profiler stats generator
	void Tick();

	//! Get average time during the measurement time
	double GetAverageTime() const;

	//! Get maximum time during the measurement time
	double GetMaxTime() const;

	//! Get average hit count during the measurement time
	double GetAverageHitCount() const;
};

///////////////////////////////////////////////////////////

/// Performance issue
class CPerformanceIssueCheckerScope
{
private:
	Float				m_timeLimit;		//!< Time after which we consider this scope a performance issue
	const Char*			m_category;			//!< Category
	const Char*			m_text;				//!< Warning text
	Bool				m_temporaryText;	//!< Text is owned by temporary buffer manager
	CObject*			m_context;			//!< Object context
	CTimeCounter		m_timer;			//!< Internal timer

public:
	CPerformanceIssueCheckerScope( Float timeLimit, CObject* contextObject, const Char* category, const Char* text );
	CPerformanceIssueCheckerScope( Float timeLimit, const Char* category, const Char* text, ... );
	~CPerformanceIssueCheckerScope();
};

