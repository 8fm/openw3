#pragma once

/// Mapping entry - do not keep locally
struct SMeshSkeletonCacheEntryData
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_SkinningMapping );

public:
	// bone mapping, for every MESH bone it's the index in the SKELETON this bone is mapped to
	TDynArray< Int16, MC_SkinningMapping >	m_boneMapping;

	// don't ask me - just copy pasted this shit
	Int16					m_mappedBoneIndexEyeLeft;
	Int16					m_mappedBoneIndexEyeRight;
	Int16					m_mappedBoneIndexHead;
};

/// Cache for mapping between CSkeleton and CMesh
/// This is in every mesh
class CMeshSkeletonMappingCache
{
public:
	CMeshSkeletonMappingCache( const CMeshTypeResource* mesh );
	~CMeshSkeletonMappingCache();

	/// Reset bone mapping cache
	void Reset();

	/// Get entry for given mesh <-> skeleton pair, use the token to speed up lookups
	/// The valid entry is ALWAYS returned and it ALWAYS contains the number of entries matching the number of bones in the mesh
	const SMeshSkeletonCacheEntryData* GetMappingEntry( const ISkeletonDataProvider* skeletonDataProvider );

private:
	/// Cache entry
	struct Entry
	{
		Uint32							m_skeletonIndex;
		SMeshSkeletonCacheEntryData		m_data;
	};

	/// Mesh is owner of this structure, we don't need a handle to it
	const CMeshTypeResource*	m_mesh;

	/// Cache entries
	typedef TDynArray< Entry*, MC_SkinningMapping >		TEntries;
	TEntries		m_entries;

	/// Number of bones in mesh
	Uint32			m_numBones;

	/// Internal data lock - we CAN be called from different threads
#ifdef RED_PLATFORM_ORBIS
	typedef Red::MemoryFramework::CAdaptiveMutexBase< Red::MemoryFramework::OSAPI::CAdaptiveMutexImpl > TLock;
#else
	typedef Red::Threads::CLightMutex		TLock;
#endif

	TLock		m_lock;
};
