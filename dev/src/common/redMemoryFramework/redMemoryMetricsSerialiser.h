/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_METRICS_SERIALISER_H
#define _RED_MEMORY_METRICS_SERIALISER_H

#include "redMemoryFrameworkTypes.h"
#include "redMemoryFrameworkPlatform.h"
#include "redMemoryAllocatorInfo.h"
#include "redMemoryThreads.h"
#include "redMemoryFileWriter.h"
#include "redMemoryCallstackCachedWriter.h"

namespace Red { namespace MemoryFramework {

class MetricsCallstack;

//////////////////////////////////////////////////////////////////////////
// This class will push alloc / free / tags to a memory dump file
// We may also want to embed some other bits and bobs in here (date / time, etc)
// Since the serialiser needs to know about the allocator areas, we make it an area walker
class MetricsSerialiser : public AllocatorWalker
{
public:
	MetricsSerialiser();
	virtual ~MetricsSerialiser();

	// Dump control
	void BeginDump( const Red::System::Char* fileName );
	void EndDump( );

	// Allocator header writing (the serialiser does not have direct access to the allocators)
	void BeginHeader();
	void BeginAllocatorHeader( PoolLabel allocatorLabel );
	void EndAllocatorHeader();
	void EndHeader();
	
#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	// Pass a pool and class name lookup table so the metrics can report strings rather than labels
	void SetMetricsNameLookupTables( PoolNamesList* poolNames, MemoryClassNamesList* classNames );
#endif

	// Pool area (footprint resizing)
	RED_INLINE	void OnAreaAdded( PoolLabel label, Red::System::MemUint lowAddress, Red::System::MemUint highAddress );
	RED_INLINE	void OnAreaRemoved( PoolLabel label, Red::System::MemUint lowAddress, Red::System::MemUint highAddress );

	// Named pool alloc / free
	RED_INLINE	void OnAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, MetricsCallstack& callstack, AnsiChar* dbgString, Int32 dbgStringLength );
	RED_INLINE	void OnFree( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack );

	// Static pool alloc / free
	RED_INLINE	void OnStaticAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, MetricsCallstack& callstack );
	RED_INLINE	void OnStaticFree( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack );

	// Overflow pool alloc / free
	RED_INLINE	void OnOverflowAllocation( PoolLabel label, MemoryClass memoryClass, void* address, Red::System::MemSize size, MetricsCallstack& callstack );
	RED_INLINE	void OnOverflowFree( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack );

	RED_INLINE	Red::System::Bool	IsWritingDump() const;

private:
	virtual void OnMemoryArea( Red::System::MemUint address, Red::System::MemSize size );
	RED_INLINE void WriteAddress( const void* address );
	RED_INLINE void WriteAllocInfo( PoolLabel label, MemoryClass memoryClass, const void* address, Red::System::MemSize size, MetricsCallstack& callstack );
	void WriteDumpHeader();
	void WriteDumpHeader_Platform();						// Implement this for each platform. Enumerate any data required for the dump tools
	void Flush();

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
	PoolNamesList* m_memoryPoolNames;						// Lookup table for memory pool names
	MemoryClassNamesList* m_memoryClassNames;				// Lookup table for memory class names
#endif
	Red::System::Bool m_isWritingDump;
	OSAPI::FileWriter m_fileWriter;
	CallstackCachedWriter m_callstackWriter;
	CMutex m_mutex;
	Red::System::Timer m_timer;
};

} }

#include "redMemoryMetricsSerialiser.inl"

#endif