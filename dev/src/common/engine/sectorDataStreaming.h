#pragma once

#include "../core/math.h"

/// Debug stats for sector streaming
struct SSectorStreamingDebugData
{
	Uint32 m_numObjectsRegistered;
	Uint32 m_numObjectsInRange;
	Uint32 m_numObjectsStreamed;
	Uint32 m_numObjectsStreaming;
	Uint32 m_numObjectsLocked;
	Bool   m_wasOverBudgetLastFrame;
};

/// Handler for sector data streaming
class CSectorDataStreaming
{
public:
	CSectorDataStreaming( const Float worldSize, class IRenderScene* renderScene, class CPhysicsWorld* physicsScene, class CDynamicLayer* dynamicLayer, const Bool hasPrefetchData, class CClipMap* terrainClipMap );
	~CSectorDataStreaming();

	//! Get debug information
	void GetDebugInfo( SSectorStreamingDebugData& outInfo );

	// Update camera position
	void SetReferencePosition( const Vector& position );

	// Begin streaming 
	void BeginStreaming();

	// Finish streaming work
	void FinishStreaming();

	// Add sector data to the streaming
	// Note: the sector data itself is never modified
	Uint32 AttachSectorData( const Uint64 contentHash, const class CSectorData* sectorData, const Bool isVisible );

	// Remove sector data from streaming
	void RemoveSectorData( const Uint32 sectorId );

	// Toggle visibility of sector data
	void ToggleSectorDataVisibility( const Uint32 sectorId, const Bool isVisible );

	// Force stream all entities visible from given point, NOTE: if this point if far away entities will be unstreamed next frame
	void ForceStreamForPoint( const Vector& point );

	// Lock streaming for area, will clear previous area, call with NULL, 0 to clear
	void SetStreamingLock( const Box* bounds, const Uint32 numBounds );

private:
	// streaming reference state
	Vector									m_referencePosition;
	Bool									m_positionValid;

	// sector Ids
	THashMap< Uint64, Uint32 >				m_sectorDataIds;

	// execution
	class CSectorDataStreamingContext*				m_context;
	class CSectorDataStreamingThread*				m_thread;
	struct CSectorDataStreamingContextThreadData*	m_threadData;

	// locks
	volatile Bool							m_isStreaming;
	Red::Threads::CMutex					m_accessLock;
};
