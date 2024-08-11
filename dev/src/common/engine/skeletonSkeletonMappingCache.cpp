/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "skeletonProvider.h"
#include "skeleton.h"
#include "skeletonSkeletonMappingCache.h"

//#pragma optimize ("", off)

SSkeletonSkeletonCacheEntryData SSkeletonSkeletonCacheEntryData::FAKE_DATA;

CSkeletonSkeletonMappingCache::CSkeletonSkeletonMappingCache( const CSkeleton* parentSkeleton )
	: m_skeleton( parentSkeleton )
	, m_numBones( parentSkeleton->GetBonesNum() )
{
}

CSkeletonSkeletonMappingCache::~CSkeletonSkeletonMappingCache()
{
	Reset();
}

void CSkeletonSkeletonMappingCache::Reset()
{
	Red::Threads::CScopedLock< TLock > lock( m_lock );

	m_numBones = m_skeleton->GetBonesNum();
	m_entries.ClearPtrFast();
}

const SSkeletonSkeletonCacheEntryData* CSkeletonSkeletonMappingCache::GetMappingEntry( const ISkeletonDataProvider* parentSkeleton )
{
	Red::Threads::CScopedLock< TLock > lock( m_lock );

	// Bone count in mesh changed - reset cache
	if ( m_numBones != m_skeleton->GetBonesNum() )
	{
		m_numBones = m_skeleton->GetBonesNum();
		m_entries.ClearPtrFast();
	}

	// No skeleton, create bullshit data (per thread)
	if ( !parentSkeleton )
	{
		return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
	}

	// Get the skeleton ID
	const Uint32 skeletonId = parentSkeleton->GetRuntimeCacheIndex();
	for ( const auto& entryData : m_entries )
	{
		if ( entryData->m_skeletonIndex == skeletonId )
			return &entryData->m_data;
	}

	// Add new data
	Entry* entryData = new Entry();
	m_entries.PushBack( entryData );

	// Initialize entry mapping data
	entryData->m_skeletonIndex = skeletonId;
	entryData->m_data.m_boneMapping.Clear();
	entryData->m_data.m_boneMapping.Reserve( m_numBones );

	// Get bones from source skeleton
	TAllocArrayCreate( ISkeletonDataProvider::BoneInfo, childBones, 512 );
	m_skeleton->GetBones( childBones );

	// Get bones from destination skeleton
	TAllocArrayCreate( ISkeletonDataProvider::BoneInfo, parentBones, 512 );
	parentSkeleton->GetBones( parentBones );

	// bone counts
	const Uint32 numChildBones = m_numBones;
	const Uint32 numParentBones = parentSkeleton->GetBonesNum();

	// Build hash map to speed up matching
	THashMap< CName, Int32 > parentBonesMap;
	parentBonesMap.Reserve( numParentBones );
	for ( Uint32 i = 0; i < numParentBones; ++i )
	{
		parentBonesMap.Insert( parentBones[ i ].m_name, i );
	}

	// Map bones between child and parent
	for ( Uint32 i = 0; i < numChildBones; ++i )
	{
		if ( const Int32* parentBoneIndexPtr = parentBonesMap.FindPtr( childBones[ i ].m_name ) )
		{
			SBoneMapping bm;
			bm.m_boneA = i;						// Child
			bm.m_boneB = *parentBoneIndexPtr;	// Parent
			entryData->m_data.m_boneMapping.PushBack( bm );
		}
	}

	// Restore some memory
	entryData->m_data.m_boneMapping.Shrink();

	// Return cached entry data
	return &entryData->m_data;
}