#pragma once

#include "../core/loadingJob.h"
#include "../core/resource.h"
#include "collisionCache.h"

//------------------

/// Data for streamed entity
class CEntityStreamingData
{
public:
	TDynArray< THandle< CResource > >		m_precachedResources;
	TDynArray< CompiledCollisionPtr >		m_precachedCollision;

public:
	CEntityStreamingData();
};

//------------------

/// Entity streaming job
class CEntityStreamingJob : public ILoadJob
{
public:
	CEntityStreamingJob( CEntity* entity, EJobPriority priority = JP_StreamingObject );
	~CEntityStreamingJob();

	// extract entity data
	RED_FORCE_INLINE CEntityStreamingData& GetData() { return m_data; }

private:
	virtual EJobResult Process();

	virtual const Char* GetDebugName() const override { return TXT("EntityStreaming"); }

	THandle< CEntity >		m_entity;
	CEntityStreamingData	m_data;
};

//------------------

/// Entity streaming proxy
/// TODO: we should more move streaming related stuff here
class CEntityStreamingProxy
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_EntityManager );

public:
	CEntityStreamingProxy( CEntity* entity, const Uint32 gridHash, const Uint32 id );
	~CEntityStreamingProxy();

	// Get connected entity
	RED_FORCE_INLINE THandle< CEntity > GetEntity() const { return m_entity; }

	// Get streaming grid location
	RED_FORCE_INLINE const Uint32 GetGridHash() const { return m_gridHash; }

	// Get streaming proxy ID
	RED_FORCE_INLINE const Uint32 GetId() const { return m_id; }

	// Start streaming of the entity, return true if successful, false if not possible
	Bool StartStreaming();

	// Update streaming of the entity, should return true if streaming finished (or failed, we don't care)
	Bool UpdateStreaming( const Bool flush );

	// Unstream entity, return true if successful, false if not possible
	Bool Unstream();

private:
	THandle< CEntity >			m_entity;			//!< Back handle to the entity
	Uint32						m_gridHash;			//!< Location token in streaming grid
	Uint32						m_id;				//!< Internal ID
	CEntityStreamingJob*		m_job;				//!< Streaming job

	friend class CStreamingSectorData;
};

//------------------

