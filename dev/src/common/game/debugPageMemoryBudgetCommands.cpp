
#include "build.h"
#include "debugPageMemoryBudgetCommands.h"
#include "debugPageMemoryBudget.h"

#ifndef NO_DEBUG_PAGES

#ifdef ENABLE_EXTENDED_MEMORY_METRICS
void CDebugMemSnapshotCreateCommandBox::Process()
{
	m_page->OnMemSnapshotAdded();
}

void CDebugMemSnapshotDeleteCommandBox::Process()
{
	m_page->OnMemSnapshotDeleted();
}

void CDebugMemSnapshotCompareCommandBox::Process()
{
	m_page->OnMemSnapshotCompare();
}

#endif

void CDebugDumpAnimEventsSizeByClassCommandBox::Process()
{
	Uint32 total = 0;
	Uint32 totalStatic = 0;

	TDynArray< DebugInternalClassInfo, MC_Debug > classInfos;

	for ( BaseObjectIterator it; it; ++it )
	{
		CObject* obj = *it;

		if ( obj && obj->IsA< CAnimEventSerializer >() )
		{
			CAnimEventSerializer* events = static_cast< CAnimEventSerializer* >( obj );

			for ( Uint32 i=0; i<events->m_events.Size(); ++i )
			{
				CExtAnimEvent* evt = events->m_events[i];

				DebugInternalClassInfo* theInfo = NULL;

				for ( Uint32 j=0; j<classInfos.Size(); ++j )
				{
					if ( classInfos[j].m_class == evt->GetClass() )
					{
						theInfo = &classInfos[j];
						break;
					}
				}

				// Create new
				if ( !theInfo )
				{
					theInfo = new ( classInfos ) DebugInternalClassInfo;
					theInfo->m_class = evt->GetClass();
					theInfo->m_memory = 0;
					theInfo->m_memoryStatic = 0;
					theInfo->m_count = 0;
				}

				// Update count
				theInfo->m_memory += CObjectMemoryAnalizer::CalcObjectSize( evt->GetClass(), evt );
				theInfo->m_memoryStatic += evt->GetClass()->GetSize();
				theInfo->m_count += 1;
			}
		}
	}

	for ( Uint32 i=0; i<classInfos.Size(); ++i )
	{
		const DebugInternalClassInfo& theInfo = classInfos[ i ];

		total += theInfo.m_memory;
		totalStatic += theInfo.m_memoryStatic;
	}

	// Sort
	qsort( classInfos.TypedData(), classInfos.Size(), sizeof( DebugInternalClassInfo ), DebugInternalClassInfo::SortFunc );

	// Print
	DebugInternalClassInfo::PrintToLog( classInfos );

	LOG_GAME( TXT("| Total memory                                                | %5i |"), total );
	LOG_GAME( TXT("===========================================================================") );

	// Sort
	qsort( classInfos.TypedData(), classInfos.Size(), sizeof( DebugInternalClassInfo ), DebugInternalClassInfo::SortFuncStatic );

	// Print
	DebugInternalClassInfo::PrintToLog( classInfos );

	LOG_GAME( TXT("| Total memory static                                         | %5i |"), totalStatic );
	LOG_GAME( TXT("===========================================================================") );
}

#endif
