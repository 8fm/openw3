#pragma once

#include "sectorDataGlobals.h"

#include "../core/hashmap.h"
#include "../core/idAllocator.h"
#include "../core/bitset.h"

class CSectorData;
class CSectorDataMerged;
class CSectorDataResourceLoader;
class CSectorDataObjectWrapper;
class CSectorDataStreamingThread;

class ISectorDataObject;

/// A magic runtime pair - object and it's ID
/// Note that ID is NOT stored in the object, this saves us a lot of time because we can treat the m_object as just data
struct CSectorDataObjectInfo
{
	Uint32					m_id;
	ISectorDataObject*		m_object;

	CSectorDataObjectInfo( const Uint32 id, ISectorDataObject* object )
		: m_id( id )
		, m_object( object )
	{}
};

template <>
struct TCopyableType< CSectorDataObjectInfo >
{
	enum
	{
		Value = true
	};
};


/// Data to have on thread
struct CSectorDataStreamingContextThreadData
{
	// internal bitmasks
	TBitSet64< SectorData::MAX_OBJECTS >	m_queryMask;

	// objects that failed sync work and will have to be processed later
	TStaticArray< CSectorDataObjectInfo, SectorData::MAX_STREAMED_OBJECTS >			m_syncStreamObjects;
	TStaticArray< CSectorDataObjectInfo, SectorData::MAX_STREAMED_OBJECTS >			m_syncUnstreamObjects;

	// new lists
	TStaticArray< CSectorDataObjectInfo, SectorData::MAX_STREAMED_OBJECTS >			m_tempInRangeObjectList;
	TStaticArray< CSectorDataObjectInfo, SectorData::MAX_STREAMED_OBJECTS >			m_tempStreamedObjectList;
};

/// Handler for sector data streaming - used on both streaming thread and main thread
class CSectorDataStreamingContext
{
public:
	CSectorDataStreamingContext( const Float worldSize, class IRenderScene* renderScene, class CPhysicsWorld* physicsScene, class CDynamicLayer* dynamicLayer, const Bool hasPrefetchData, class CClipMap* terrainClipMap );
	~CSectorDataStreamingContext();

	// Get number of loading resources
	const Uint32 GetNumStreamingResources() const;

	// Get number of object that are not yet streamed in
	const Uint32 GetNumStreamingObjects() const;

	// Get number of object that are currently streamed
	const Uint32 GetNumStreamedObjects() const;

	// Get number of locked objects
	const Uint32 GetNumLockedObjects() const;

	// Get number of object in range
	const Uint32 GetNumInrangeObjects() const;

	// Get total number of objects
	const Uint32 GetNumTotalObjects() const;

	// Flush loading of resources for streaming
	void FlushResourceLoading();

	// Load resources for given point in given range
	void PreloadResources( const Vector& referenceCameraPosition, const Float range );

	// Set lock to allow/disallow new resources from being loaded
	void SetLoadingLock( const Bool isLoadingLocked );

	// Process asynchronous streaming
	void ProcessAsync( CSectorDataStreamingContextThreadData& threadData, const Vector& referenceCameraPosition, Bool instantUnloads = false, const Bool forceStream = false );

	// Process synchronous streaming
	void ProcessSync( CSectorDataStreamingContextThreadData& threadData, Bool instantUnloads = false, const Bool forceStream = false );

	// Add sector data to the streaming
	// Note: the sector data itself is never modified
	Uint32 AttachSectorData( const Uint64 contentID, const CSectorData* sectorData, const Bool isVisible );

	// Remove sector data from streaming
	void RemoveSectorData( const Uint32 sectorId );

	// Toggle visibility of sector data
	void ToggleSectorDataVisibility( const Uint32 sectorId, const Bool isVisible );

	// Lock streaming for area, will clear previous area, call with NULL, 0 to clear
	void SetStreamingLock( const Box* bounds, const Uint32 numBounds );

private:
	typedef TStaticArray< CSectorDataObjectInfo, SectorData::MAX_STREAMED_OBJECTS >			TObjects;
	typedef TStaticArray< Uint32, SectorData::MAX_STREAMED_OBJECTS >						TObjectIndices;
	typedef TBitSet64< SectorData::MAX_OBJECTS >											TObjectBits;
	typedef TBitSet64< SectorData::MAX_SECTORS >											TSectorBits;
	typedef Red::Threads::CAtomic< Bool >													TBoolFlag;

	// sector Ids
	THashMap< Uint64, Uint32 >		m_sectorDataIds;

	// static state
	Float							m_worldSize;
	class IRenderScene*				m_renderScene;
	class CPhysicsWorld*			m_physicsScene;
	class CClipMap*					m_terrainClipMap;
	class CDynamicLayer*			m_dynamicLayer;

	// merged data - all objects
	CSectorDataMerged*				m_runtimeData;
	CSectorDataResourceLoader*		m_runtimeResourceLoader;
	CSectorDataObjectWrapper*		m_runtimeObjectManager;

	Bool			m_loadingLocked;		//!< Is loading of new stuff locked

	TSectorBits     m_visibleSectors;		//!< General bitmask with visible sectors

	TObjectBits		m_visibilityMask;		//!< Bitmask with visible objects (only visible objects are allowed to stream in)
	TBoolFlag		m_visibilityMaskDirty;	//!< Refresh visibility mask

	TObjects		m_inRangeObjects;		//!< Objects that are currently in range and pending streaming (the need Stream() to finish to get promoted to streamed list)
	TObjectBits		m_inRangeMask;			//!< Mask of objects that are in range

	TObjects		m_streamedObjects;		//!< Objects that are currently streamed
	TObjectBits		m_streamedMask;			//!< Mask of objects that are streamed (they need Unstream())

	TObjectIndices	m_lockedObjectIndices;	// objects locked by streaming
	TObjectBits		m_lockedMask;			// mask of objects locked by streaming

	void RefreshVisibilityMask();
	void UnstreamAll();
};
