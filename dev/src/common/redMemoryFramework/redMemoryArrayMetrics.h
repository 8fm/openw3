/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef _RED_MEMORY_ARRAY_METRICS_H
#define _RED_MEMORY_ARRAY_METRICS_H

namespace Red { namespace MemoryFramework {

/////////////////////////////////////////////////////////////////////
// ID of the array in the array metrics system (could be pointer, but it's more general this way)
typedef Red::System::Uint64		TArrayID;

/////////////////////////////////////////////////////////////////////
// Dynamic array metrics (type, memory class, memory pool, number of elements, capacity, etc)
struct DynamicArrayMetrics
{
	// Array ID (just for debug, sorting, etc)
	TArrayID						m_id;

	// Name of the array type
	const Red::System::AnsiChar*	m_typeName;

	// Size of the array element
	size_t							m_typeSize;

	// Number of elements in the array
	size_t							m_numElements;

	// Array capacity
	size_t							m_maxElements;

	// Memory class the array memory was originally allocated from
	MemoryClass						m_memoryClass;

	// Memory pool the array memory
	PoolLabel						m_memoryPool;
};

/////////////////////////////////////////////////////////////////////
// ArrayMetrics is used to get a run-time breakdown of allocations for dynamic arrays
// Note that is noticable performance penalty of using this code so use it only when needed
class ArrayMetrics
{
public:
	ArrayMetrics();
	~ArrayMetrics();
	
	//! Create array descriptor
	TArrayID RegisterArray( const Red::AnsiChar* arrayType, const size_t typeSize, const MemoryClass memoryClass, const PoolLabel memoryPool );

	//! Destroy array descriptor
	void UnregisterArray( const TArrayID arrayId );

	//! Update array memory class and pool
	void UpdateArrayMemoryClass( const TArrayID arrayId, const MemoryClass memoryClass, const PoolLabel memoryPool );

	//! Update number of elements in array
	void UpdateArrayElementCount( const TArrayID arrayId, const size_t numElements );

	//! Update array capacity (buffer size)
	void UpdateArrayMaximumCount( const TArrayID arrayId, const size_t numElements );

	//! Get number of array metric elements
	const Uint32 GetNumMetricEntries() const;

	//! Copy current array metrics into specified buffer
	const void GetMetricEntries( DynamicArrayMetrics* outBuffer, const Uint32 numEntries ) const;

private:
	struct ArrayMetricsPrivate;
	struct ArrayMetricsPrivateEntry;

	ArrayMetricsPrivate* m_data;
};

} }

#endif