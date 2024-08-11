#pragma once

#include "collisionCache.h"
#include "../core/resource.h"

class ILoadJob;
class CSectorDataMerged;
class CDiskFile;

/// Resource streaming helper for sector data
/// This is extracted to a separate class for code clarity
class CSectorDataResourceLoader
{
public:
	CSectorDataResourceLoader( const CSectorDataMerged* baseData );
	~CSectorDataResourceLoader();

	enum EResult
	{
		eResult_Loaded,
		eResult_NotReady,
		eResult_Failed,
	};

	// Get number of resources being loaded
	const Uint32 GetNumStreamingResources() const;

	// Get number of all loaded resources
	const Uint32 GetNumLoadedResources() const;

	// Flush resource loading
	void Flush();

	// Prefetch resource
	EResult PrefetchResource( const SectorData::PackedResourceRef res, const Int32 priorityBias=0 );

	// Prefetch collision mesh
	EResult PrefetchCollision( const SectorData::PackedResourceRef res, const Int32 priorityBias=0 );

	// Prefetch list of resources
	void PrefetchResources( const SectorData::PackedResourceRef* resources, const Uint32 numResources );

	// Get resolved resource file, NO REFERENCE IS ADDED
	CDiskFile* GetResourceFileNoRef( const SectorData::PackedResourceRef res );

	// Get resource bounding box
	Box GetResourceBoxNoRef( const SectorData::PackedResourceRef res );

	// Get resource FOR USE, this automatically increments reference count
	THandle< CResource > GetResourceAddRef( const SectorData::PackedResourceRef res );

	// Get compiled collision FOR USE, this automatically increments reference count
	CompiledCollisionPtr GetCollisionAddRef( const SectorData::PackedResourceRef res );

	// Cancel prefetch
	void CancelPrefetch( const SectorData::PackedResourceRef res );

	// Add resource reference
	void Release( const SectorData::PackedResourceRef res );

private:
	// NOTE: right now we are still using the ILoadJob oldschool system

	// Invalid resource marker
	static const Uint32 INVALID_RESOURCE = 0xFFFFFFFF;

	// Resource states
	struct RuntimeResource
	{
		Uint32					m_numRefs;				// number of references to object
		CDiskFile*				m_file;					// mapped disk file ALWAYS THERE FOR VALID RESOURCES
		THandle< CResource >	m_resource;				// loaded resource
		CompiledCollisionPtr	m_collision;			// loaded collision data (meshes only)
		ILoadJob*				m_loadingJob;			// loading job for the resource
		Bool					m_loadingCollision;		// did we request for collision
	};

	// Re

	// Source data
	const class CSectorDataMerged*		m_sourceData;

	// Runtime resource data
	static const Uint32 MAX_RESOURCES = 8192;
	RuntimeResource						m_resources[ MAX_RESOURCES ];

	// Counters
	Red::Threads::CAtomic< Int32 >		m_numStreamingResources; // job count
	Red::Threads::CAtomic< Int32 >		m_numLoadedResources;

	// Prepare resource for access, can fail
	Bool PrepareResource( const Uint32 mergedResourceID );
};
