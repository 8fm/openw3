/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _LOADING_PROFILER_H
#define _LOADING_PROFILER_H

#include "../../common/redThreads/redThreadsThread.h"
#include "../../common/redThreads/redThreadsAtomic.h"

// generic loading profiler functionality
class CLoadingProfiler
{
public:
	CLoadingProfiler();

	// start new capture
	void Start();

	// report captured stats
	void FinishLoading( const Char* title );

	// report finished stage of loading
	void FinishStage( const Char* name );

private:
	struct CaptureInfo
	{
		const Char*			m_name;
		Double				m_time;
		Uint64				m_memoryCPU;
		Uint64				m_memoryGPU;
		Uint64				m_bytesRead;
		Uint64				m_bytesWritten;
		Uint64				m_fileOps;
		Uint64				m_filesOpened;
	};

	const static Uint32		MAX_CAPTURES	= 256;

	typedef Red::Threads::AtomicOps::TAtomic32	TCounters;
	
	CaptureInfo		m_captures[ MAX_CAPTURES ];
	TCounters		m_counter;
	Double			m_start;
};

extern CLoadingProfiler	GLoadingProfiler;

#endif
