#include "build.h"
#include "meshSkeletonMappingCache.h"
#include "meshTypeResource.h"
#include "skeleton.h"

RED_DEFINE_STATIC_NAME( left_eye );
RED_DEFINE_STATIC_NAME( right_eye );

CMeshSkeletonMappingCache::CMeshSkeletonMappingCache( const CMeshTypeResource* mesh )
	: m_mesh( mesh )
	, m_numBones( mesh->GetBoneCount() )
{
}

CMeshSkeletonMappingCache::~CMeshSkeletonMappingCache()
{
	Reset();
}

void CMeshSkeletonMappingCache::Reset()
{
	Red::Threads::CScopedLock< TLock > lock( m_lock );

	m_numBones = m_mesh->GetBoneCount();
	m_entries.ClearPtrFast();
}

const SMeshSkeletonCacheEntryData* CMeshSkeletonMappingCache::GetMappingEntry( const ISkeletonDataProvider* skeleton )
{
	Red::Threads::CScopedLock< TLock > lock( m_lock );

	// Bone count in mesh changed - reset cache
	if ( m_numBones != m_mesh->GetBoneCount() )
	{
		// Not calling Reset here, because that would need a reentrant lock, and on PS4 it is not reentrant.
		m_numBones = m_mesh->GetBoneCount();
		m_entries.ClearPtrFast();
	}

	// No skeleton, create bullshit data (per thread)
	if ( !skeleton )
	{
		static RED_TLS SMeshSkeletonCacheEntryData* fallbackData = nullptr;

		if ( !fallbackData )
			fallbackData = new SMeshSkeletonCacheEntryData();

		fallbackData->m_boneMapping.Resize( m_numBones );
		fallbackData->m_mappedBoneIndexEyeLeft = -1;
		fallbackData->m_mappedBoneIndexEyeRight = -1;
		fallbackData->m_mappedBoneIndexHead = -1;

		for ( Uint32 i=0; i<m_numBones; ++i )
			fallbackData->m_boneMapping[i] = -1;

		return fallbackData;
	}

	// Get the skeleton ID
	const Uint32 skeletonId = skeleton->GetRuntimeCacheIndex();
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
	entryData->m_data.m_boneMapping.Resize( m_numBones );
	entryData->m_data.m_mappedBoneIndexEyeLeft = -1;
	entryData->m_data.m_mappedBoneIndexEyeRight = -1;
	entryData->m_data.m_mappedBoneIndexHead = -1;

	// Regenerate mapping
	const Int32 skeletonBoneIndexEyeLeft = skeleton->FindBoneByName( CNAME( left_eye ) );
	const Int32 skeletonBoneIndexEyeRight = skeleton->FindBoneByName( CNAME( right_eye ) );
	const Int32 skeletonBoneIndexHead = skeleton->FindBoneByName( CNAME( head ) );
	
	// Get bones from source skeleton
	TAllocArrayCreate( ISkeletonDataProvider::BoneInfo, sourceBones, 512 );
	skeleton->GetBones( sourceBones );

	// Map mesh bones
	const CName* meshBoneNames = m_mesh->GetBoneNames();
	for ( Uint32 i=0; i<m_numBones; i++ )
	{
		const CName& meshBoneName = meshBoneNames[i];

		// Find bone in source skeleton
		Int32 mappedBoneIndex = -1;
		for ( Uint32 j=0; j<sourceBones.Size(); j++ )
		{
			const ISkeletonDataProvider::BoneInfo& sourceBone = sourceBones[j];
			if ( sourceBone.m_name == meshBoneName )
			{
				mappedBoneIndex = j;
				break;
			}
		}

		// Remember
		entryData->m_data.m_boneMapping[i] = mappedBoneIndex;

		// Update custom bones info
		// WTF IS THIS SHIT - cmon'....
		{
			if ( -1 != skeletonBoneIndexEyeLeft && skeletonBoneIndexEyeLeft == mappedBoneIndex )
			{
				RED_ASSERT( -1 == entryData->m_data.m_mappedBoneIndexEyeLeft, TXT("EyeLeft bone already mapped") );
				entryData->m_data.m_mappedBoneIndexEyeLeft = (Int16)i;
			}

			if ( -1 != skeletonBoneIndexEyeRight && skeletonBoneIndexEyeRight == mappedBoneIndex )
			{
				RED_ASSERT( -1 == entryData->m_data.m_mappedBoneIndexEyeRight, TXT("EyeRight bone already mapped") );
				entryData->m_data.m_mappedBoneIndexEyeRight = (Int16)i;
			}

			if ( -1 != skeletonBoneIndexHead && skeletonBoneIndexHead == mappedBoneIndex )
			{
				RED_ASSERT( -1 == entryData->m_data.m_mappedBoneIndexHead, TXT("Head bone already mapped") );
				entryData->m_data.m_mappedBoneIndexHead = (Int16)i;
			}
		}
	}

	// Return cached entry data
	return &entryData->m_data;
}

