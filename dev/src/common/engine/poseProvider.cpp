/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "poseProvider.h"
#include "skeleton.h"
#include "posePool.h"
#include "poseBlock.h"
#include "poseProviderStats.h"
#include "../redSystem/unitTestMode.h"

typedef Red::Threads::CScopedLock< Red::Threads::CRWSpinLock > ScopedWriteLock;
typedef Red::Threads::CScopedSharedLock< Red::Threads::CRWSpinLock > ScopedReadLock;

CPoseProvider::CPoseProvider()
	:	m_skeleton( nullptr ),
		m_blockCount( 0 )
{
	m_blockContainer.Reserve( 8 );
}

CPoseProvider::~CPoseProvider()
{
	ReleaseInternalPoseBlocks();
}

CPoseHandle CPoseProvider::AcquirePose()
{
	RED_FATAL_ASSERT( m_skeleton, "CPoseProvider cannot be used without skeleton" );
	
	CPoseHandle pose = FindAvailablePose();
	if( !pose )
	{
		AcquirePoseBlock();
		pose = FindAvailablePose();

		if( !pose )
		{
			pose = CreateFallbackPose();
		}
	}

	pose->ClearEventsAndUsedAnims();

	return std::move( pose );
}

CPoseHandle CPoseProvider::FindAvailablePose()
{
	CPoseHandle handle;
	ScopedReadLock readLock( m_lock ); // this won't lock unless a new block is being requested 
	// This is not optimal. This can be heavy if you you have a lot of block with no pose available.
	// Profile and optimize if needed.
	for( Int32 reverseIndex = m_blockContainer.Size() -1, end = -1; reverseIndex != end; --reverseIndex )
	{
		CPoseBlock * block = m_blockContainer[ reverseIndex ];
		if( block->IsPoseAvailable() )
		{
			handle = block->TakePose(); 
			if( handle )
			{
				return handle;
			}
			else
			{ /* another thread beat us to take the available pose, continue searching! */ }
		}
	}
	
	return std::move( handle );
}

void CPoseProvider::AcquirePoseBlock()
{
	Int32 currentBlockCount = m_blockCount.GetValue();
	ScopedWriteLock writeLock( m_lock ); // Need to stop other thread while we add more pose.

	if( currentBlockCount == m_blockCount.GetValue() )
	{
		CPoseBlock * newBlock = m_pool->TakeBlock( m_skeleton );
		if( newBlock )
		{
			m_blockContainer.PushBack( newBlock );
			m_blockCount.Increment();
		}
		else
		{ /* No block available. This is bad ... budget is busted! */ }
	}
	else
	{ /* Another thread got the lock before us, no need to get more pose. */ }
}

CPoseHandle CPoseProvider::CreateFallbackPose()
{
	// All pose are in use. Allocate an emergency pose. No crash allowed. 
	// Error should already be logged.

	const Uint32 boneCount = m_skeleton->GetBonesNum();
	const Uint32 trackCount = m_skeleton->GetTracksNum();
	CPoseHandle pose = AllocateFallbackPose();
	pose->Init( boneCount, trackCount );
	pose->Reset( m_skeleton );
	return pose;
}

CPoseHandle CPoseProvider::AllocateFallbackPose() const
{
	CPoseHandle handle = CPoseHandle( new SBehaviorGraphOutput(), nullptr );
	handle->AddRef();
	return handle;
}

void CPoseProvider::ReleaseInternalPoseBlocks()
{
#ifndef RED_FINAL_BUILD
	for( PoseBlockContainer::iterator iter = m_blockContainer.Begin(), end = m_blockContainer.End(); iter != end; ++iter )
	{
		RED_WARNING( Red::System::UnitTestMode() || (*iter)->IsAllPoseAvailable(), "Skeleton %ls is being release, but some pose are leaked!", m_skeletonDepotPath.AsChar() );
	}
#endif

	if( m_pool ) // if false, we are most likely doing Unit Testing
	{
		ScopedWriteLock writeLock( m_lock ); 

		for( PoseBlockContainer::iterator iter = m_blockContainer.Begin(), end = m_blockContainer.End(); iter != end; ++iter )
		{
			CPoseBlock * block = *iter;
			m_pool->GiveBlock( block );
		}

		m_blockContainer.ClearFast();
	}
}

void CPoseProvider::Sanitize()
{
	// Need to stop other thread while I'm doing some cleanup. It will be very fast don't worry. 
	// Also, we are most likely in GC process. 
	ScopedWriteLock writeLock( m_lock ); 

	auto predicate = [=]( CPoseBlock * block )
	{ 
		if( block->IsAllPoseAvailable() ) 
		{ 
			m_pool->GiveBlock( block ); 
			return true; 
		} 
		return false; 
	};
	
	m_blockContainer.Erase( RemoveIf( m_blockContainer.Begin(), m_blockContainer.End(), predicate ), m_blockContainer.End() );
	m_blockCount.SetValue( m_blockContainer.Size() );
}

const CSkeleton * CPoseProvider::GetSkeleton() const
{
	return m_skeleton;
}

void CPoseProvider::GetStats( SPoseProviderStats & stats ) const
{
	Red::System::MemoryZero( &stats, sizeof( SPoseProviderStats ) );

	ScopedReadLock scopedLock( m_lock );

	for( PoseBlockContainer::const_iterator iter = m_blockContainer.Begin(), end = m_blockContainer.End(); iter != end; ++iter )
	{
		const CPoseBlock * block = *iter;
		const Uint32 poseCount = block->GetPoseCount();  
		const Uint32 availablePoseCount = block->GetPoseAvailableCount();
		const Uint32 usedPoseCount = poseCount - availablePoseCount;
		const Uint32 consumedMemory = block->GetMemoryConsumed();
		const Uint32 totalMemory = block->GetMemoryUsage();

		stats.m_numTotal += poseCount;
		stats.m_numAlloc += usedPoseCount;
		stats.m_memAlloc += consumedMemory;
		stats.m_memTotal += totalMemory;
	}
}

void CPoseProvider::SetInternalSkeleton( const CSkeleton * skeleton )
{
	m_skeleton = skeleton;

#ifndef RED_FINAL_BUILD
	m_skeletonDepotPath = m_skeleton->GetDepotPath();
#endif
}

void CPoseProvider::SetInternalPosePool( PosePoolHandle pool )
{
	m_pool = pool;
}

void CPoseProvider::PushInternalPoseBlock( CPoseBlock * block )
{
	RED_FATAL_ASSERT( Red::System::UnitTestMode(), "Cannot be called outside Unit Test environment." );

	ScopedWriteLock writeLock( m_lock );
	m_blockContainer.PushBack( block );
	m_blockCount.Increment();
}

PoseProviderHandle CreatePoseProvider( const CSkeleton * skeleton, PosePoolHandle pool )
{
	PoseProviderHandle provider( new CPoseProvider );
	provider->SetInternalSkeleton( skeleton );
	provider->SetInternalPosePool( pool );
	return provider;
}
