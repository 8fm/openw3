/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_LEAK_TRACKER_H
#define _RED_MEMORY_LEAK_TRACKER_H
#pragma once

#include "redMemoryFrameworkTypes.h"
#include "redMemoryThreads.h"
#include "redMemoryMetricsCallstack.h"
#include "../redSystem/bitUtils.h"
#include "../redMath/numericalutils.h"

namespace Red { namespace MemoryFramework {

// Tracked allocations
struct TrackedAllocation
{
	AllocationID m_allocID;
	void* m_ptr;
	TrackedAllocation* m_nextAllocation;
	Red::System::MemSize m_allocSize;
	MemoryClass m_memoryClass;
	MetricsCallstack m_callStack;
};

///////////////////////////////////////////////////////////////////1
// MemoryLeakTracker
//	Track active allocations for a single pool
class MemoryLeakTracker
{
public:
	MemoryLeakTracker();
	~MemoryLeakTracker();

	void OnAllocation( void* ptr, Red::System::MemSize size, MemoryClass memoryClass, const MetricsCallstack& callstack );
	void OnFree( const void* ptr, Red::System::MemSize size );

	// These lock / unlock the internal mutex
	TrackedAllocation* AquireUnfreedAllocationsList( Red::System::Int32 bucketIndex );
	void UnacquireUnfreedAllocationsList();
	void ResetLeaksList();
	void EnableAllocationTracking();
	void DisableAllocationTracking();

	static const Red::System::Int32 c_BucketCount = 32;			// Active lists are stored in bucket lists dependent on the allocation size
private:
	AllocationID GetNewAllocationID();
	Red::System::Int32 CalculateBucket( Red::System::MemSize allocSize );

	static const Red::System::Uint32 c_maximumAllocationsToTrack = 8192;
	TrackedAllocation m_nodeBuffer[ c_maximumAllocationsToTrack ];	// Static list of allocation nodes
	TrackedAllocation* m_deadNodes;									// Linked list of empty nodes
	TrackedAllocation* m_activeNodes[ c_BucketCount ];				// Linked list of active nodes
	CMutex m_mutex;													// Slow, but its for debug
	AllocationID m_nextAllocationID;								// The next active allocation ID
	Red::System::Bool m_isTrackingAllocations;						// Flag to enable / disable alloc tracking
};

///////////////////////////////////////////////////////////////////
// CalculateBucket
//	Bucket index = Log2(size)
RED_INLINE Red::System::Int32 MemoryLeakTracker::CalculateBucket( Red::System::MemSize allocSize )
{
	const Red::System::MemSize c_maximumSize = ( 1u << (c_BucketCount-1) );
	return ( Red::System::Int32 )Red::System::BitUtils::BitScanReverse< Red::System::MemSize >( Red::Math::NumericalUtils::Min( allocSize, c_maximumSize ) );
}

///////////////////////////////////////////////////////////////////
// GetNewAllocationID
//	Return a new allocation ID
//	Note! The mutex should be locked when calling this!
RED_INLINE AllocationID MemoryLeakTracker::GetNewAllocationID()
{
	return m_nextAllocationID++;
}

} }

#endif