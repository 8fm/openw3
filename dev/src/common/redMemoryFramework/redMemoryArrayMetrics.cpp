/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryFramework.h"
#include "redMemoryArrayMetrics.h"

namespace Red { namespace MemoryFramework {

struct ArrayMetrics::ArrayMetricsPrivateEntry : public DynamicArrayMetrics
{
	static const Uint32 MAGIC = 0xDEEDEE88;

	// Magic ID (to detect cases of corrupted memory)
	Uint32							m_magic;

	// Linked list
	ArrayMetricsPrivateEntry**		m_prev;
	ArrayMetricsPrivateEntry*		m_next;

	ArrayMetricsPrivateEntry( ArrayMetricsPrivateEntry*& list, const Red::System::AnsiChar* typeName, const size_t typeSize, const MemoryClass memoryClass, const PoolLabel memoryPool )
		: m_magic( MAGIC )
	{
		m_id = (TArrayID)this;

		m_typeName = typeName;
		m_typeSize = typeSize;
		m_numElements = 0;
		m_maxElements = 0;
		m_memoryClass = memoryClass;
		m_memoryPool = memoryPool;

		m_next = list;
		m_prev = &list;
		if ( list ) list->m_prev = &m_next;
		list = this;
	}

	void Unlink()
	{
		RED_ASSERT( m_magic == MAGIC );

		*m_prev = m_next;
		if ( m_next ) m_next->m_prev = m_prev;

		m_next = NULL;
		m_prev = NULL;
		m_magic = 0;
	}
};

struct ArrayMetrics::ArrayMetricsPrivate
{
	// Array metrics can be accessed from multiple threads
	CMutex		m_lock;

	// Head list of the array entries
	ArrayMetrics::ArrayMetricsPrivateEntry*		m_entries;
	Uint32										m_numEntries;

	ArrayMetricsPrivate()
		: m_entries( NULL )
		, m_numEntries( 0 )
	{}
};

/////////////////////////////////////////////////////////////////
// CTor
//
ArrayMetrics::ArrayMetrics()
{
	m_data = new ArrayMetricsPrivate();
}

/////////////////////////////////////////////////////////////////
// DTor
//
ArrayMetrics::~ArrayMetrics()
{
	delete m_data;
}

/////////////////////////////////////////////////////////////////
// Create array metrics data
//	
TArrayID ArrayMetrics::RegisterArray( const Red::AnsiChar* arrayType, const size_t typeSize, const MemoryClass memoryClass, const PoolLabel memoryPool )
{
	m_data->m_lock.Acquire();

	// create and register the entry
	ArrayMetricsPrivateEntry* entry = new ArrayMetricsPrivateEntry(
		m_data->m_entries,
		arrayType, typeSize, memoryClass, memoryPool );

	// udpate entries count (for easier grabbing)
	m_data->m_numEntries += 1;

	// get array ID to use
	const TArrayID id = entry->m_id;

	m_data->m_lock.Release();

	return id;
}

void ArrayMetrics::UnregisterArray( const TArrayID arrayId )
{
	if ( arrayId )
	{
		ArrayMetricsPrivateEntry* entry = (ArrayMetricsPrivateEntry*) arrayId;
		if ( entry->m_magic == ArrayMetricsPrivateEntry::MAGIC )
		{
			m_data->m_lock.Acquire();

			entry->Unlink();
			delete entry;

			// udpate entries count (for easier grabbing)
			m_data->m_numEntries -= 1;

			m_data->m_lock.Release();
		}
	}
}

void ArrayMetrics::UpdateArrayMemoryClass( const TArrayID arrayId, const MemoryClass memoryClass, const PoolLabel memoryPool )
{
	if ( arrayId )
	{
		ArrayMetricsPrivateEntry* entry = (ArrayMetricsPrivateEntry*) arrayId;
		if ( entry->m_magic == ArrayMetricsPrivateEntry::MAGIC )
		{
			entry->m_memoryClass = memoryClass;
			entry->m_memoryPool = memoryPool;
		}
	}
}

void ArrayMetrics::UpdateArrayElementCount( const TArrayID arrayId, const size_t numElements )
{
	if ( arrayId )
	{
		ArrayMetricsPrivateEntry* entry = (ArrayMetricsPrivateEntry*) arrayId;
		if ( entry->m_magic == ArrayMetricsPrivateEntry::MAGIC )
		{
			entry->m_numElements = numElements;
		}
	}
}

void ArrayMetrics::UpdateArrayMaximumCount( const TArrayID arrayId, const size_t maxElements )
{
	if ( arrayId )
	{
		ArrayMetricsPrivateEntry* entry = (ArrayMetricsPrivateEntry*) arrayId;
		if ( entry->m_magic == ArrayMetricsPrivateEntry::MAGIC )
		{
			entry->m_maxElements = maxElements;
		}
	}
}

const Uint32 ArrayMetrics::GetNumMetricEntries() const
{
	return m_data->m_numEntries;
}

const void ArrayMetrics::GetMetricEntries( DynamicArrayMetrics* outBuffer, const Uint32 numEntries ) const
{
	m_data->m_lock.Acquire();

	// cleanup output memory (so we don't leave junk)
	Red::System::MemoryZero( outBuffer, sizeof(DynamicArrayMetrics) * numEntries );

	// copy data
	Uint32 entriesLeft = numEntries;
	ArrayMetricsPrivateEntry* entry = m_data->m_entries;
	while ( NULL != entry && entriesLeft > 0 )
	{
		// copy data
		outBuffer->m_id = entry->m_id;
		outBuffer->m_maxElements = entry->m_maxElements;
		outBuffer->m_numElements = entry->m_numElements;
		outBuffer->m_typeName = entry->m_typeName;
		outBuffer->m_typeSize = entry->m_typeSize;
		outBuffer->m_memoryClass = entry->m_memoryClass;
		outBuffer->m_memoryPool = entry->m_memoryPool;

		// next entry
		outBuffer += 1;
		entry = entry->m_next;
		entriesLeft -= 1;
	}

	m_data->m_lock.Release();
}

}}