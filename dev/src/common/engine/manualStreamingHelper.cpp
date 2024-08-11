#include "build.h"
#include "manualStreamingHelper.h"

CManualStreamingHelper::CManualStreamingHelper()
	: m_referencePosition( Vector::ZEROS )
	, m_validReferencePosition( false )
	, m_forceUpdateAll( false )
{
	m_entries.SetMaxBudgetedProcessingTime( 0.0003f );
}

void CManualStreamingHelper::Reset()
{
	m_entries.Clear();
	m_entriesSet.ClearFast();
}

void CManualStreamingHelper::Tick()
{
	PC_SCOPE_PIX( EntitiesTick );

	if ( ! m_validReferencePosition )
	{
		return;
	}

	struct StreamableProcessor
	{
		CManualStreamingHelper* m_manager;

		StreamableProcessor( CManualStreamingHelper* manager )
			: m_manager( manager )
		{}

		RED_FORCE_INLINE void Process( StreamingEntry* entry )
		{
			// Determine new LOD

			const StreamingEntry::LOD newLOD = m_manager->DetermineStreamableLOD( entry );

			// Update LOD

			if ( entry->GetLOD() != newLOD )
			{
				entry->SetLOD( newLOD );
				entry->ResetCreateSucceeded();

				// Changed to unloaded? -> destroy engine data immediately

				if ( newLOD == StreamingEntry::LOD_Unloaded )
				{
					entry->GetStreamable()->OnDestroyEngineRepresentation();
				}
			}

			// Is loaded? -> keep requesting for engine data creation until succeeded

			if ( newLOD == StreamingEntry::LOD_Loaded && !entry->DidCreateSucceed() && entry->GetStreamable()->OnCreateEngineRepresentation() )
			{
				entry->MarkCreateSucceeded();
			}
		}
	} streamableProcessor( this );

	// Process

	if ( m_forceUpdateAll )
	{
		m_entries.ProcessAll( streamableProcessor );
		m_forceUpdateAll = false;
	}
	else
	{
		m_entries.Process( streamableProcessor );
	}
}

CManualStreamingHelper::StreamingEntry::LOD CManualStreamingHelper::DetermineStreamableLOD( StreamingEntry* entry )
{
	const Float distanceSqr = m_referencePosition.DistanceSquaredTo2D( entry->GetPosition() );
	return ( ( distanceSqr < entry->GetMaxToggleDistanceSqr() && entry->GetLOD() == StreamingEntry::LOD_Loaded ) || distanceSqr < entry->GetMinToggleDistanceSqr() ) ?
		StreamingEntry::LOD_Loaded :
		StreamingEntry::LOD_Unloaded;
}

void CManualStreamingHelper::Register( CNode* streamable, Float minDistance, Float deadZone )
{
	StreamingEntry* entry = new StreamingEntry( streamable, minDistance, minDistance + deadZone );
	if ( !m_entriesSet.Insert( entry ) )
	{
		ASSERT( !"Duplicate streaming entry!" );
		delete entry;
		return;
	}
	m_entries.Add( entry, true );
}

void CManualStreamingHelper::Unregister( CNode* streamable )
{
	auto it = m_entriesSet.Find( streamable );
	if ( it != m_entriesSet.End() )
	{
		StreamingEntry* entry = *it;
		m_entriesSet.Erase( it );
		m_entries.Remove( entry, true );
		delete entry;
	}
}

void CManualStreamingHelper::RefreshPosition( CNode* streamable )
{
	if ( StreamingEntry** entry = m_entriesSet.FindPtr( streamable ) )
	{
		( *entry )->SetPosition( streamable->GetWorldPosition() );
	}
}

void CManualStreamingHelper::ResetCreateSucceeded( CNode* streamable )
{
	if ( StreamingEntry** entry = m_entriesSet.FindPtr( streamable ) )
	{
		( *entry )->ResetCreateSucceeded();
	}
}

void CManualStreamingHelper::SetReferencePosition( const Vector& position )
{
	m_referencePosition = position;
	m_validReferencePosition = true;
}
