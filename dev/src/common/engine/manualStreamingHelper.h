/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../core/hashset.h"
#include "budgetedContainer.h"

/**
 *	Simple streaming helper.
 *
 *	Once CNode registers with streaming distances in it, it receives:
 *	1) OnCreateEngineRepresentation() continuously until succeeded if closer (from the camera) than streaming distance
 *	2) OnDestroyEngineRepresentation() once if further than streaming distance
 */
class CManualStreamingHelper
{
private:
	class StreamingEntry
	{
		DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Engine );
		DEFINE_CUSTOM_INTRUSIVE_LIST_ELEMENT( StreamingEntry, entries );
	public:
		enum LOD
		{
			LOD_Unloaded = 0,
			LOD_Loaded
		};

	private:
		CNode*	m_streamable;
		Vector	m_position;				// Cached world position; assumed not to change
		Float	m_minToggleDistanceSqr;	// Min toggle distance (dead zone start)
		Float	m_maxToggleDistanceSqr;	// Max toggle distance (dead zone end)
		Bool	m_createSucceeded;
		LOD		m_LOD;

	public:
		StreamingEntry( CNode* streamable, Float minToggleDistance, Float maxToggleDistance )
			: m_streamable( streamable )
			, m_position( streamable->GetWorldPosition() )
			, m_minToggleDistanceSqr( minToggleDistance * minToggleDistance )
			, m_maxToggleDistanceSqr( maxToggleDistance * maxToggleDistance )
			, m_createSucceeded( false )
			, m_LOD( LOD_Unloaded )
		{}

		CNode*	GetStreamable() const { return m_streamable; }

		Float	GetMinToggleDistanceSqr() const { return m_minToggleDistanceSqr; }
		Float	GetMaxToggleDistanceSqr() const { return m_maxToggleDistanceSqr; }

		LOD		GetLOD() const { return m_LOD; }
		void	SetLOD( LOD lod ) { m_LOD = lod; }

		Bool	DidCreateSucceed() const { return m_createSucceeded; }
		void	MarkCreateSucceeded() { m_createSucceeded = true; }
		void	ResetCreateSucceeded() { m_createSucceeded = false; }

		void	SetPosition( const Vector& position ) { m_position = position; }
		const Vector& GetPosition() const { return m_position; }

		struct HashFunc
		{
			static RED_FORCE_INLINE Uint32 GetHash( const StreamingEntry* entry ) { return GetPtrHash( entry->m_streamable ); }
			static RED_FORCE_INLINE Uint32 GetHash( const CNode* streamable ) { return GetPtrHash( streamable ); }
		};
		struct EqualFunc
		{
			static RED_INLINE Bool Equal( const StreamingEntry* a, const StreamingEntry* b ) { return a->m_streamable == b->m_streamable; }
			static RED_INLINE Bool Equal( const StreamingEntry* a, const CNode* b ) { return a->m_streamable == b; }
		};
	};
	typedef TBudgetedContainer< StreamingEntry, CUSTOM_INTRUSIVE_LIST_ELEMENT_ACCESSOR_CLASS( StreamingEntry, entries ) >	Entries;
	typedef THashSet< StreamingEntry*, StreamingEntry::HashFunc, StreamingEntry::EqualFunc >								EntriesSet;

	Entries				m_entries;				// Continuously processed list of streamables
	EntriesSet			m_entriesSet;			// Used for fast removal

	Vector				m_referencePosition;
	Bool				m_validReferencePosition;
	Bool				m_forceUpdateAll;

public:
	CManualStreamingHelper();
	void Reset();
	// Sets reference position used to calculate distance to streamables from
	void SetReferencePosition( const Vector& position );
	// Processes registers streamables within fixed time limit
	void Tick();
	// Causes all streamables to be processed in the next call to Tick()
	void ForceUpdateAll() { m_forceUpdateAll = true; }
	// Registers node for streaming
	void Register( CNode* streamable, Float minDistance, Float deadZone = 5.0f );
	// Unregisters node from streaming
	void Unregister( CNode* streamable );
	// Refreshes cached streamable position
	void RefreshPosition( CNode* streamable );
	// Resets creation success state for given streamable
	void ResetCreateSucceeded( CNode* streamable );

private:
	StreamingEntry::LOD DetermineStreamableLOD( StreamingEntry* entry );

	RED_FORCE_INLINE StreamingEntry* FindEntry( CNode* streamable )
	{
		auto it = m_entriesSet.Find( streamable );
		return it != m_entriesSet.End() ? *it : nullptr;
	}
};