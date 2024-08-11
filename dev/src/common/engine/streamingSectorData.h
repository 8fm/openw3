#pragma once
#ifndef STREAMING_SECTOR_DATA_H_
#define STREAMING_SECTOR_DATA_H_

#include "../core/idAllocator.h"

class CEntity;
class CEntityStreamingProxy;
class CStreamingPositionQuantizer;
class CStreamingEntryMask;
class CStreamingGrid;

//////////////////////////////////////////////////////////////////////////

struct SStreamingDebugData
{
	Uint32 m_numProxiesRegistered;
	Uint32 m_numProxiesInRange;
	Uint32 m_numProxiesStreamed;
	Uint32 m_numProxiesStreaming;
	Uint32 m_numProxiesLocked;
	Uint32 m_numGridLevels;
	Uint32 m_numGridBucketsMax;
	Uint32 m_numGridBucketsUsed;
	Float  m_worldSize;
	Vector m_referencePosition;
	Vector m_lastStreamingPosition;
	Bool   m_wasOverBudgetLastFrame;

	SStreamingDebugData()
		: m_numProxiesRegistered(0)
		, m_numProxiesInRange(0)
		, m_numProxiesStreamed(0)
		, m_numProxiesStreaming(0)
		, m_numProxiesLocked(0)
		, m_numGridLevels(0)
		, m_numGridBucketsMax(0)
		, m_numGridBucketsUsed(0)
		, m_referencePosition(0,0,0)
		, m_lastStreamingPosition(0,0,0)
		, m_wasOverBudgetLastFrame( false )
		, m_worldSize(1.0f)
	{}
};

//////////////////////////////////////////////////////////////////////////

class CStreamingIgnoreList
{
public:
	CStreamingIgnoreList();

	// Ignore the given entity when looking for entities to stream in/out
	void IgnoreEntity( CEntity* entity );

	// Unignore the given entity
	void UnignoreEntity( CEntity* entity );

	// Stop ignoring entities (clears ignore list)
	void ClearIgnoreList();

	// Returns the ignored entities
	void GetIgnoredEntities( TDynArray< CEntity* >& entities );

	// Returns true if the given entity is in the ignore list
	RED_INLINE Bool IsEntityIgnored( CEntity* entity ) const { return m_ignoredEntities.Exist( entity ); }

private:
	typedef THashSet< THandle< CEntity > >			TIngoredEntities;
	TIngoredEntities								m_ignoredEntities;
};

//////////////////////////////////////////////////////////////////////////

class CStreamingSectorData
{
public:
	CStreamingSectorData( CWorld* world );
	~CStreamingSectorData();

	// Update camera position
	void SetReferencePosition( const Vector& position );

	// Request full streaming update on next update tick
	void RequestFullUpdate();

	// Updates the layers in relation to the current streaming position, with what needs to be added and removed from the streaming layers.
	// force - update streaming regardless of proximity to the last streaming position
	// flush - make sure all streaming is finished before returning
	void UpdateStreaming( const Bool force = false, const Bool flush = false );

	// Create streaming proxy for given entity
	CEntityStreamingProxy* RegisterEntity( CEntity* entity, const Vector& worldPosition );

	// Update position of existing entry
	void UpdateEntity( CEntityStreamingProxy* entity, const Vector& worldPosition );

	// Unregister entity from streaming
	void UnregisterEntity( CEntityStreamingProxy* streamingProxy );

	// Get debug information
	void GetDebugInfo( SStreamingDebugData& outInfo ) const;

	// Get grid layout
	void GetGridDebugInfo( const Uint32 level, struct SStreamingGridDebugData& outData ) const;

	// Force stream all entities visible from given point, NOTE: if this point if far away entities will be unstreamed next frame
	void ForceStreamForPoint( const Vector& point );

	// Force stream all entities in given area, NOTE: if this area if far away entities will be unstreamed next frame
	void ForceStreamForArea( const Box& area );

	// Lock streaming for area, will clear previous area, call with NULL, 0 to clear
	void SetStreamingLock( const Box* bounds, const Uint32 numBounds );

#ifndef NO_EDITOR
	RED_INLINE const CStreamingIgnoreList& GetIgnoreList() const { return m_ignoreList; }
	RED_INLINE CStreamingIgnoreList& GetIgnoreList() { return m_ignoreList; }
#endif

private:
	// Notify entities about streaming
	void NotifyStreamedInEntities();

	// Prepare the streaming grid sizes
	void PrepareDataStructures();

	typedef TDynArray< CEntityStreamingProxy*, MC_EntityManager >		TAllProxies;
	typedef TDynArray< Uint32, MC_EntityManager >						TInRangeProxies;
	typedef TDynArray< Uint32, MC_EntityManager >						TStreamingProxies;
	typedef TDynArray< Uint32, MC_EntityManager >						TLockedProxies;

	Float							m_worldSize;
	class CWorld*					m_world;
	Red::Threads::CLightMutex		m_lock;

	Vector							m_referencePosition;
	Vector							m_lastStreamingPosition;
	Bool							m_updateRequested;
	Bool							m_positionValid;
	Bool							m_requestFullUpdate;
	Bool							m_wasOverBudget;

	// update
	Bool							m_isUpdating;
	TAllProxies						m_toUnregister;

	// streaming grid support
	CStreamingGrid*					m_grid;
	CStreamingPositionQuantizer*	m_quantizer;

	Uint32							m_queryInRangeCount;
	CStreamingEntryMask*			m_queryMask;
	CStreamingEntryMask*			m_inRangeMask; // entities currently in streaming range
	TInRangeProxies					m_inRangeProxies;

	CStreamingEntryMask*			m_streamingMask; // entities currently streaming
	TStreamingProxies				m_streamingProxies;

	CStreamingEntryMask*			m_lockedMask;
	TLockedProxies					m_lockedProxies;
	Bool							m_hasStreamingLock;

	// ID allocator
	IDAllocatorDynamic				m_proxyIdAllocator;	
	TAllProxies						m_proxies;
	
#ifndef NO_EDITOR
	CStreamingIgnoreList			m_ignoreList;
#endif
};

#endif // STREAMING_SECTOR_DATA_H_
