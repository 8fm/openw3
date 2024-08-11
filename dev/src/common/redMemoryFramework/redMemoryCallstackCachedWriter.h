/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_CALLSTACK_CACHED_WRITER_H
#define _RED_MEMORY_CALLSTACK_CACHED_WRITER_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "redMemoryFrameworkPlatform.h"
#include "redMemoryFileWriter.h"

namespace Red { namespace MemoryFramework {

class MetricsCallstack;

/////////////////////////////////////////////////////////////////
// This class keeps a small cache of callstack hashes and  only writes
// new call-stacks when it needs to
// Keep the buffer as small as we can, as this needs to be fairly fast!
class CallstackCachedWriter : Red::System::NonCopyable
{
public:
	CallstackCachedWriter( OSAPI::FileWriter& myFileWriter );
	~CallstackCachedWriter();

	void WriteCallstack( MetricsCallstack& callstack );

private:
	bool CallstackCached( Red::System::Uint64 csHash );

	static const Red::System::Uint32 c_CacheSize = 256;		// 256 x 8 bytes = 2k buffer
	Red::System::Uint64 m_hashes[c_CacheSize];				// Circular buffer of callstack hashes
	Red::System::Uint64* m_writeHead;
	Red::System::Uint32 m_count;

	OSAPI::FileWriter& m_fileWriter;
};

} }

#endif