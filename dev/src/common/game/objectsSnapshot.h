
#pragma once

#include "../core/memoryFileAnalizer.h"

struct DebugInternalClassInfo
{
	CClass*		m_class;
	Int32			m_memory;
	Int32			m_memoryStatic;
	Int32			m_count;

	static Int32 SortFunc( const void* a, const void* b )
	{
		const DebugInternalClassInfo* dataA = (const DebugInternalClassInfo*)a;
		const DebugInternalClassInfo* dataB = (const DebugInternalClassInfo*)b;

		if ( dataA->m_memory > dataB->m_memory ) return -1;
		if ( dataA->m_memory < dataB->m_memory ) return 1;
		return 0;
	}

	static Int32 SortFuncStatic( const void* a, const void* b )
	{
		const DebugInternalClassInfo* dataA = (const DebugInternalClassInfo*)a;
		const DebugInternalClassInfo* dataB = (const DebugInternalClassInfo*)b;

		if ( dataA->m_memoryStatic > dataB->m_memoryStatic ) return -1;
		if ( dataA->m_memoryStatic < dataB->m_memoryStatic ) return 1;
		return 0;
	}

	static void PrintToLog( TDynArray< DebugInternalClassInfo, MC_Debug >& infos )
	{
		// Display
		LOG_GAME( TXT("===========================================================================") );
		LOG_GAME( TXT("| Object class                     |       Bytes |       Bytes |    Count |") );
		LOG_GAME( TXT("===========================================================================") );

		for ( Uint32 i=0; i<infos.Size(); ++i )
		{
			DebugInternalClassInfo& info = infos[ i ];

			if ( info.m_memory != 0 || info.m_memoryStatic != 0 )
			{
				LOG_GAME( TXT("| %37s | %9i | %9i |   %6i |"), info.m_class->GetName().AsString().AsChar(), info.m_memory, info.m_memoryStatic, info.m_count );
			}
		}

		LOG_GAME( TXT("===========================================================================") );
	}
};

class ObjectsSnapshot
{
	TDynArray< Uint64, MC_Debug > m_objectsIds;
	TDynArray< String, MC_Debug > m_objectsNames;

public:
	ObjectsSnapshot() {}

	void DoSnapshot()
	{
		for ( BaseObjectIterator it; it; ++it )
		{
			CObject* object = *it;

			//m_objectsIds.PushBack( object->CalcHash() );
			m_objectsNames.PushBack( object->GetFriendlyName() );
		}

		if ( !CheckHashList() )
		{
			ASSERT( 0 );
		}
	}

	static void Compare( const ObjectsSnapshot& a, const ObjectsSnapshot& b )
	{
		LOG_GAME( TXT("Compare objects snapshots") );

		const Uint32 sizeA = a.m_objectsIds.Size();
		const Uint32 sizeB = b.m_objectsIds.Size();

		for ( Uint32 itA=0; itA<sizeA; ++itA )
		{
			Bool found = false;

			Uint64 valA = a.m_objectsIds[ itA ];

			for ( Uint32 itB=0; itB<sizeB; ++itB )
			{
				Uint64 valB = b.m_objectsIds[ itB ];

				if ( valA == valB )
				{
					found = true;
					break;
				}
			}

			if ( !found )
			{
				// A && ~B
				LOG_GAME( TXT("1. %s"), a.m_objectsNames[ itA ].AsChar() );
			}
		}

		for ( Uint32 itB=0; itB<sizeB; ++itB )
		{
			Bool found = false;

			Uint64 valB = b.m_objectsIds[ itB ];

			for ( Uint32 itA=0; itA<sizeA; ++itA )
			{
				Uint64 valA = a.m_objectsIds[ itA ];

				if ( valA == valB )
				{
					found = true;
					break;
				}
			}

			if ( !found )
			{
				// ~A && B
				LOG_GAME( TXT("2. %s"), a.m_objectsNames[ itB ].AsChar() );
			}
		}
	}

private:
	Bool CheckHashList()
	{
		Bool ret = true;

		const Uint32 size = m_objectsIds.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			Uint64 val = m_objectsIds[ i ];

			for ( Uint32 j=i+1; j<size; ++j )
			{
				if ( val == m_objectsIds[ j ] )
				{
					LOG_GAME( TXT("%s == %s"), m_objectsNames[ i ].AsChar(), m_objectsNames[ j ].AsChar() );

					ret = false;
				}
			}
		}

		return ret;
	}
};

class ObjectsByClassesSnapshot
{
	TDynArray< DebugInternalClassInfo, MC_Debug > m_classInfos;

public:
	ObjectsByClassesSnapshot() {}

	void DoSnapshot()
	{
		Uint32 total = 0;
		Uint32 totalStatic = 0;

		for ( BaseObjectIterator it; it; ++it )
		{
			CObject* object = *it;

			if ( object && !object->HasFlag( OF_DefaultObject ) )
			{
				DebugInternalClassInfo* theInfo = NULL;

				for ( Uint32 j=0; j<m_classInfos.Size(); ++j )
				{
					if ( m_classInfos[j].m_class == object->GetClass() )
					{
						theInfo = &m_classInfos[j];
						break;
					}
				}

				// Create new
				if ( !theInfo )
				{
					theInfo = new ( m_classInfos ) DebugInternalClassInfo;
					theInfo->m_class = object->GetClass();
					theInfo->m_memory = 0;
					theInfo->m_memoryStatic = 0;
					theInfo->m_count = 0;
				}

				// Update count
				theInfo->m_memory += CObjectMemoryAnalizer::CalcObjectSize( object );
				theInfo->m_memoryStatic += object->GetClass()->GetSize();
				theInfo->m_count += 1;
			}
		}

		for ( Uint32 i=0; i<m_classInfos.Size(); ++i )
		{
			const DebugInternalClassInfo& theInfo = m_classInfos[ i ];

			total += theInfo.m_memory;
			totalStatic += theInfo.m_memoryStatic;
		}

		// Sort
		qsort( m_classInfos.TypedData(), m_classInfos.Size(), sizeof( DebugInternalClassInfo ), DebugInternalClassInfo::SortFunc );

		// Print
		DebugInternalClassInfo::PrintToLog( m_classInfos );

		LOG_GAME( TXT("| Total memory                                                | %5i |"), total );
		LOG_GAME( TXT("===========================================================================") );

		// Sort
		qsort( m_classInfos.TypedData(), m_classInfos.Size(), sizeof( DebugInternalClassInfo ), DebugInternalClassInfo::SortFuncStatic );

		// Print
		DebugInternalClassInfo::PrintToLog( m_classInfos );

		LOG_GAME( TXT("| Total memory static                                         | %5i |"), totalStatic );
		LOG_GAME( TXT("===========================================================================") );
	}

	static void Compare( const ObjectsByClassesSnapshot& a, const ObjectsByClassesSnapshot& b )
	{
		TDynArray< DebugInternalClassInfo, MC_Debug > diffClassInfos;

		const Uint32 sizeA = a.m_classInfos.Size();
		const Uint32 sizeB = b.m_classInfos.Size();

		Int32 totalMemoryDiff = 0;
		Int32 totalMemoryStaticDiff = 0;

		for ( Uint32 itA=0; itA<sizeA; ++itA )
		{
			Bool found = false;

			const DebugInternalClassInfo& infoA = a.m_classInfos[ itA ];

			for ( Uint32 itB=0; itB<sizeB; ++itB )
			{
				const DebugInternalClassInfo& infoB = b.m_classInfos[ itB ];

				if ( infoA.m_class == infoB.m_class )
				{
					found = true;

					DebugInternalClassInfo* diffInfo = new ( diffClassInfos ) DebugInternalClassInfo;
					diffInfo->m_class = infoA.m_class;
					diffInfo->m_memory = (Int32)infoB.m_memory - (Int32)infoA.m_memory;
					diffInfo->m_memoryStatic = (Int32)infoB.m_memoryStatic - (Int32)infoA.m_memoryStatic;
					diffInfo->m_count = (Int32)infoB.m_count - (Int32)infoA.m_count;

					totalMemoryDiff += diffInfo->m_memory;
					totalMemoryStaticDiff += diffInfo->m_memoryStatic;

					break;
				}
			} // B

			if ( !found )
			{
				DebugInternalClassInfo* diffInfo = new ( diffClassInfos ) DebugInternalClassInfo;
				diffInfo->m_class = infoA.m_class;
				diffInfo->m_memory = -((Int32)infoA.m_memory);
				diffInfo->m_memoryStatic = -((Int32)infoA.m_memoryStatic);
				diffInfo->m_count = -((Int32)infoA.m_count);

				totalMemoryDiff += diffInfo->m_memory;
				totalMemoryStaticDiff += diffInfo->m_memoryStatic;
			}

		} // A

		for ( Uint32 itB=0; itB<sizeB; ++itB )
		{
			Bool found = false;

			const DebugInternalClassInfo& infoB = b.m_classInfos[ itB ];

			for ( Uint32 itA=0; itA<sizeA; ++itA )
			{
				const DebugInternalClassInfo& infoA = a.m_classInfos[ itA ];

				if ( infoA.m_class == infoB.m_class )
				{
					found = true;
					break;
				}
			} // A

			if ( !found )
			{
				DebugInternalClassInfo* diffInfo = new ( diffClassInfos ) DebugInternalClassInfo;
				diffInfo->m_class = infoB.m_class;
				diffInfo->m_memory = (Int32)infoB.m_memory;
				diffInfo->m_memoryStatic = (Int32)infoB.m_memoryStatic;
				diffInfo->m_count = (Int32)infoB.m_count;

				totalMemoryDiff += diffInfo->m_memory;
				totalMemoryStaticDiff += diffInfo->m_memoryStatic;
			}

		} // B

		// Sort
		qsort( diffClassInfos.TypedData(), diffClassInfos.Size(), sizeof( DebugInternalClassInfo ), DebugInternalClassInfo::SortFunc );
		
		// Print
		DebugInternalClassInfo::PrintToLog( diffClassInfos );

		LOG_GAME( TXT("| Total memory diff                                           | %5i |"), totalMemoryDiff );
		LOG_GAME( TXT("===========================================================================") );

		LOG_GAME( TXT("") );
		LOG_GAME( TXT("") );

		// Sort
		qsort( diffClassInfos.TypedData(), diffClassInfos.Size(), sizeof( DebugInternalClassInfo ), DebugInternalClassInfo::SortFuncStatic );

		// Print
		DebugInternalClassInfo::PrintToLog( diffClassInfos );

		LOG_GAME( TXT("| Total memory diff                                           | %5i |"), totalMemoryStaticDiff );
		LOG_GAME( TXT("===========================================================================") );
	}
};
