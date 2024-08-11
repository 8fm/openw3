#include "build.h"

#include "dynarray.h"

#ifdef USE_ARRAY_METRICS

namespace
{
	
	struct TempMetricsEntry
	{
		TempMetricsEntry()
			: m_typeName( nullptr )
			, m_memoryClass( (Red::MemoryFramework::MemoryClass)-1 )
			, m_numArrays( 0 )
			, m_allocatedMemory( 0 )
			, m_usedMemory( 0 )
			, m_typeSize( 0 )
		{
		}
		const AnsiChar*						m_typeName;
		Red::MemoryFramework::MemoryClass	m_memoryClass;
		size_t								m_numArrays;
		size_t								m_allocatedMemory;
		size_t								m_usedMemory;
		size_t								m_typeSize;
	};

	struct TempMemClassStatsEntry 
	{
		TempMemClassStatsEntry()
			: m_numArrays(0)
			, m_usedMemory(0)
			, m_totalMemory(0)
		{
		}
		size_t	m_numArrays;
		size_t	m_usedMemory;
		size_t	m_totalMemory;
	};

	static int SortDynArrayMetricEntriesByClassType( const void* ptrA, const void* ptrB)
	{
		const Red::MemoryFramework::DynamicArrayMetrics& a = *(const Red::MemoryFramework::DynamicArrayMetrics*) ptrA;
		const Red::MemoryFramework::DynamicArrayMetrics& b = *(const Red::MemoryFramework::DynamicArrayMetrics*) ptrB;

		if ( a.m_memoryClass < b.m_memoryClass ) return -1;
		if ( a.m_memoryClass > b.m_memoryClass ) return 1;

		if ( a.m_typeName < b.m_typeName) return -1;
		if ( a.m_typeName > b.m_typeName) return 1;

		return 0;
	}

	static int SortDynArrayMetricEntriesByPool( const void* ptrA, const void* ptrB)
	{
		const Red::MemoryFramework::DynamicArrayMetrics& a = *(const Red::MemoryFramework::DynamicArrayMetrics*) ptrA;
		const Red::MemoryFramework::DynamicArrayMetrics& b = *(const Red::MemoryFramework::DynamicArrayMetrics*) ptrB;

		if( a.m_memoryPool < b.m_memoryPool ) return -1;
		else if( a.m_memoryPool > b.m_memoryPool ) return 1;

		if ( a.m_memoryClass < b.m_memoryClass ) return -1;
		else if ( a.m_memoryClass > b.m_memoryClass ) return 1;

		if ( a.m_typeName < b.m_typeName) return -1;
		else if ( a.m_typeName > b.m_typeName) return 1;

		return 0;
	}

	static int SortGroupEntriesByClassTypeSize( const void* ptrA, const void* ptrB)
	{
		const TempMetricsEntry& a = *(const TempMetricsEntry*) ptrA;
		const TempMetricsEntry& b = *(const TempMetricsEntry*) ptrB;

		if( a.m_memoryClass < b.m_memoryClass )	return -1;
		else if( a.m_memoryClass > b.m_memoryClass ) return 1;

		if( a.m_allocatedMemory > b.m_allocatedMemory ) return -1;
		else if( a.m_allocatedMemory < b.m_allocatedMemory ) return 1;

		if( a.m_typeName < b.m_typeName ) return -1;
		else if( a.m_typeName > b.m_typeName ) return 1;		

		return 0;
	}
}

Red::MemoryFramework::DynamicArrayMetrics* CBaseArray::s_arrayMetricsPreallocated = nullptr;
Uint32 CBaseArray::s_arrayMetricsPreallocSize = 1024 * 1024 * 50;

Uint32 CBaseArray::s_tempMetricsPreallocBufferSize = sizeof( TempMetricsEntry ) * 32000;
void* CBaseArray::s_tempMetricsPreallocBuffer = nullptr;

void CBaseArray::TArrayDumpToLog::DumpLine( const Char* line )
{
	LOG_CORE( TXT( "%s" ), line );
}

CBaseArray::TArrayDumpToFile::TArrayDumpToFile( const Char* filePath )
{
	Bool openSuccess = m_fileHandle.Open( filePath, Red::IO::eOpenFlag_ReadWrite | Red::IO::eOpenFlag_Create );
	RED_ASSERT( openSuccess, TXT( "Failed to open file handle" ) );
}	

CBaseArray::TArrayDumpToFile::~TArrayDumpToFile()
{
	m_fileHandle.Flush();
	m_fileHandle.Close();
}

void CBaseArray::TArrayDumpToFile::DumpLine( const Char* line )
{
	Uint32 bytesOut = 0;
	Uint32 bytesToWrite = static_cast< Uint32 >( Red::System::StringLength( line ) * sizeof( Char ) );
	m_fileHandle.Write( line, bytesToWrite, bytesOut );

	const Char* newLine = TXT( "\n" );
	const Uint32 newlineLength = static_cast< Uint32 >( Red::System::StringLength( newLine ) * sizeof( Char ) );
	m_fileHandle.Write( newLine, newlineLength, bytesOut );

	RED_UNUSED( bytesOut );
}

template< class OutputType >
void CBaseArray::DumpArrayMetricsDetailed( OutputType& outputter )
{
	// Output helper
#define DO_OUTPUT( fmt, ... )		Red::System::SNPrintF( outputBuffer, 512, fmt, ## __VA_ARGS__ );	outputter.DumpLine( outputBuffer );
	Char outputBuffer[512] = {'\0'};

	const Uint32 numMetrics = SArrayMetrics::GetInstance().GetNumMetricEntries();
	if ( numMetrics > 0 && numMetrics < s_arrayMetricsPreallocSize )
	{
		// allocate buffer for array metrics
		Red::MemoryFramework::DynamicArrayMetrics* metrics = CBaseArray::s_arrayMetricsPreallocated;
		if ( NULL != metrics )
		{
			// get array metrics data from array metrics system
			SArrayMetrics::GetInstance().GetMetricEntries( metrics, numMetrics );

			qsort( metrics, numMetrics, sizeof(Red::MemoryFramework::DynamicArrayMetrics), &SortDynArrayMetricEntriesByPool );

			Red::MemoryFramework::PoolLabel lastPool = (Red::MemoryFramework::PoolLabel)-1;
			for ( Uint32 i=0; i<numMetrics; ++i )
			{
				if( lastPool != metrics[i].m_memoryPool )
				{
					DO_OUTPUT( TXT( "-------------------------------------------------------------------------" ) );
					DO_OUTPUT( TXT( "%S" ), SRedMemory::GetInstance().GetMemoryPoolName( metrics[i].m_memoryPool ) );
					DO_OUTPUT( TXT( "MemClass\tType\tElement Count\tMax Elements\tAllocated\tData" ) );

					lastPool = metrics[i].m_memoryPool;
				}

				if( metrics[i].m_maxElements > 0 )
				{
					DO_OUTPUT( TXT( "%S\t%S\t%d\t%d\t%d" ), SRedMemory::GetInstance().GetMemoryClassName( metrics[i].m_memoryClass ), metrics[i].m_typeName, metrics[i].m_numElements, metrics[i].m_maxElements, metrics[i].m_maxElements * metrics[i].m_typeSize );
				}
			}
		}
	}
}

// Groups metrics data by mem class and inner type. returns num groups
Uint32 OrganiseMetricsEntriesByClassAndType( const Red::MemoryFramework::DynamicArrayMetrics* srcMetrics, Uint32 numMetrics, TempMetricsEntry* destGroups, MemSize& memUsed, MemSize& memTotal )
{
	// generate group entries
	TempMetricsEntry* currentGroupedEnty = destGroups;

	MemSize totalAllocated = 0, totalUsed = 0;

	const AnsiChar* lastTypeName = currentGroupedEnty->m_typeName = srcMetrics[0].m_typeName;
	Red::MemoryFramework::MemoryClass lastMemoryClass = currentGroupedEnty->m_memoryClass = srcMetrics[0].m_memoryClass;
	MemSize lastTypeSize = currentGroupedEnty->m_typeSize = srcMetrics[0].m_typeSize;
	for( Uint32 entry = 0; entry < numMetrics; ++entry )
	{
		const Red::MemoryFramework::DynamicArrayMetrics& thisEntry = srcMetrics[ entry ];

		// new memory class or type, change entry
		if( thisEntry.m_memoryClass != lastMemoryClass || thisEntry.m_typeName != lastTypeName || thisEntry.m_typeSize != lastTypeSize )
		{
			++currentGroupedEnty;
			lastMemoryClass = currentGroupedEnty->m_memoryClass = thisEntry.m_memoryClass;
			lastTypeName = currentGroupedEnty->m_typeName = thisEntry.m_typeName;
			lastTypeSize = currentGroupedEnty->m_typeSize = thisEntry.m_typeSize;
		}

		MemSize allocatedInArray = thisEntry.m_maxElements * thisEntry.m_typeSize;
		MemSize usedInArray = thisEntry.m_numElements * thisEntry.m_typeSize;

		currentGroupedEnty->m_allocatedMemory += allocatedInArray;
		currentGroupedEnty->m_usedMemory += usedInArray;
		currentGroupedEnty->m_numArrays++;

		totalAllocated += allocatedInArray;
		totalUsed += usedInArray;
	}

	memUsed = totalUsed;
	memTotal = totalAllocated;

	return (Uint32)( currentGroupedEnty - destGroups ) + 1;
}

void CalculatePerMemClassStats( TempMemClassStatsEntry (&memClassStats)[256], TempMetricsEntry* srcGroups, Uint32 numGroups )
{
	for( Uint32 group = 0; group < numGroups; ++group )
	{
		const TempMetricsEntry& thisGroup = srcGroups[ group ];
		memClassStats[thisGroup.m_memoryClass].m_numArrays += thisGroup.m_numArrays;
		memClassStats[thisGroup.m_memoryClass].m_usedMemory += thisGroup.m_usedMemory;
		memClassStats[thisGroup.m_memoryClass].m_totalMemory += thisGroup.m_allocatedMemory;
	}
}

template< class OutputType >
void CBaseArray::DumpArrayMetricsSummary( OutputType& outputter )
{
	// Output helper
#define DO_OUTPUT( fmt, ... )		Red::System::SNPrintF( outputBuffer, 512, fmt, ## __VA_ARGS__ );	outputter.DumpLine( outputBuffer );
	Char outputBuffer[512] = {'\0'};

	const Uint32 numMetrics = SArrayMetrics::GetInstance().GetNumMetricEntries();
	if ( numMetrics > 0  && numMetrics < s_arrayMetricsPreallocSize )
	{
		// allocate buffer for array metrics
		Red::MemoryFramework::DynamicArrayMetrics* metrics = CBaseArray::s_arrayMetricsPreallocated;
		if ( NULL != metrics )
		{
			SArrayMetrics::GetInstance().GetMetricEntries( metrics, numMetrics );

			DO_OUTPUT( TXT("--------------------------------------------------------------") );
			DO_OUTPUT( TXT("\tDynamic Array Metrics (%d total arrays)"), numMetrics );

			// Sort the elements by memory class and inner type
			qsort( metrics, numMetrics, sizeof(Red::MemoryFramework::DynamicArrayMetrics), &SortDynArrayMetricEntriesByClassType );

			TempMetricsEntry* groupedEntries = new ( CBaseArray::s_tempMetricsPreallocBuffer ) TempMetricsEntry[ 32000 ];
			MemSize totalAllocated = 0, totalUsed = 0;
			Uint32 groupCount = OrganiseMetricsEntriesByClassAndType( metrics, numMetrics, groupedEntries, totalUsed, totalAllocated );

			// output total summary
			Float wastePercentTotal =  100.0f * ( 1.0f - ( (Float)totalUsed / (Float)totalAllocated ) );
			DO_OUTPUT( TXT( "\t%1.3fkb / %1.3fkb used. %.2f%% wasted memory total." ), (Float)totalUsed / 1024.0f, (Float)totalAllocated / 1024.0f, wastePercentTotal );

			// Sort the groups by memory class, inner type, and allocated size
			qsort( groupedEntries, groupCount, sizeof( TempMetricsEntry ), SortGroupEntriesByClassTypeSize );

			// Generate per-memclass stats
			TempMemClassStatsEntry memoryClassStats[256];
			CalculatePerMemClassStats( memoryClassStats, groupedEntries, groupCount );

			// Now run through each group, outputting the stats per class
			Red::MemoryFramework::MemoryClass lastMemoryClass = (Red::MemoryFramework::MemoryClass)-1;
			for( Uint32 group = 0; group < groupCount; ++group )
			{
				const TempMetricsEntry& thisGroup = groupedEntries[ group ];
				if( thisGroup.m_memoryClass != lastMemoryClass )
				{
					Float percentWasted = 0.0f;
					if( memoryClassStats[ thisGroup.m_memoryClass ].m_totalMemory > 0 )
					{
						percentWasted = 100.0f * ( 1.0f - ((Float)memoryClassStats[ thisGroup.m_memoryClass ].m_usedMemory / (Float)memoryClassStats[ thisGroup.m_memoryClass ].m_totalMemory) );
					}

					DO_OUTPUT( TXT( "*******************************************************" ) );
					DO_OUTPUT( TXT( "\t%S: %1.3fkb / %1.3fkb used in %d arrays. %.2f%% wasted." ),	SRedMemory::GetInstance().GetMemoryClassName( thisGroup.m_memoryClass ),
																									(Float)memoryClassStats[ thisGroup.m_memoryClass ].m_usedMemory / 1024.0f,
																									(Float)memoryClassStats[ thisGroup.m_memoryClass ].m_totalMemory / 1024.0f,
																									memoryClassStats[ thisGroup.m_memoryClass ].m_numArrays,
																									percentWasted);
					lastMemoryClass = thisGroup.m_memoryClass;
				}

				Uint32 elementsUsed = static_cast< Uint32 >( thisGroup.m_usedMemory / thisGroup.m_typeSize );
				Uint32 elementsTotal = static_cast< Uint32 >( thisGroup.m_allocatedMemory / thisGroup.m_typeSize );
				Float memoryUsedKb = (Float)thisGroup.m_usedMemory / 1024.0f;
				Float memoryTotalKb = (Float)thisGroup.m_allocatedMemory / 1024.0f;
				Float wasted = 0.0f;
				if( thisGroup.m_allocatedMemory > 0 )
				{
					wasted = ( 1.0f - ( (Float)thisGroup.m_usedMemory / (Float)thisGroup.m_allocatedMemory ) ) * 100.0f;
				}
				DO_OUTPUT( TXT( "TDynArray<%S> - %d / %d elements (%.3fkb / %.3fkb used) in %d arrays. %.2f%% wasted memory" ), thisGroup.m_typeName, elementsUsed, elementsTotal, memoryUsedKb, memoryTotalKb, thisGroup.m_numArrays, wasted );
			}
		}
	}
}

template void CBaseArray::DumpArrayMetricsSummary< CBaseArray::TArrayDumpToLog >( TArrayDumpToLog& outputter );
template void CBaseArray::DumpArrayMetricsSummary< CBaseArray::TArrayDumpToFile >( TArrayDumpToFile& outputter );
template void CBaseArray::DumpArrayMetricsDetailed< CBaseArray::TArrayDumpToLog >( TArrayDumpToLog& outputter );
template void CBaseArray::DumpArrayMetricsDetailed< CBaseArray::TArrayDumpToFile >( TArrayDumpToFile& outputter );

#endif	//USE_ARRAY_METRICS

