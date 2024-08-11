/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "animatedAttachment.h"

/// Mapping entry - do not keep locally
struct SSkeletonSkeletonCacheEntryData
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_SkinningMapping );

public:
	BoneMappingContainer	m_boneMapping;

	static SSkeletonSkeletonCacheEntryData FAKE_DATA;
};

/// Mapping between two skeletons
class CSkeletonSkeletonMappingCache
{
public:
	CSkeletonSkeletonMappingCache( const CSkeleton* parentSkeleton );
	~CSkeletonSkeletonMappingCache();

	/// Reset bone mapping cache
	void Reset();

	/// Get entry for given mesh <-> skeleton pair, use the token to speed up lookups
	/// The valid entry is ALWAYS returned and it ALWAYS contains the number of entries matching the number of bones in the mesh
	const SSkeletonSkeletonCacheEntryData* GetMappingEntry( const ISkeletonDataProvider* parentSkeleton );

private:
	/// Cache entry
	struct Entry
	{
		Uint32								m_skeletonIndex;
		SSkeletonSkeletonCacheEntryData		m_data;
	};

	/// Skeleton is owner of this structure, we don't need a handle to it
	const CSkeleton*	m_skeleton;

	/// Cache entries
	typedef TDynArray< Entry*, MC_SkinningMapping >		TEntries;
	TEntries		m_entries;

	/// Number of our bones 
	Uint32			m_numBones;

	/// Internal data lock - we CAN be called from different threads
#ifdef RED_PLATFORM_ORBIS
	typedef Red::MemoryFramework::CAdaptiveMutexBase< Red::MemoryFramework::OSAPI::CAdaptiveMutexImpl > TLock;
#else
	typedef Red::Threads::CLightMutex		TLock;
#endif

	TLock		m_lock;
};