/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "posePool.h"
#include "poseBlock.h"
#include "behaviorGraphOutput.h"
#include "animationEvent.h"
#include "skeleton.h"
#include "../redSystem/unitTestMode.h"

const Uint32 c_blockCount = 800;
const Uint32 c_posePerBlock = 16;
const Uint32 c_bufferAlignment = 16;

#ifdef CORE_USES_DEBUG_ALLOCATOR
	#define POSE_POOL_TRACK_MEMORY_STOMP
#endif

CPosePool::Bucket::Bucket()
	:	m_size( 0 )
{}

void CPosePool::Bucket::Initialize( Uint32 size, Uint32 maxAvailable )
{
	m_size = size;
	m_buffers.Reserve( maxAvailable );
	m_buffer = Red::CreateUniqueBuffer( m_size * maxAvailable, c_bufferAlignment, MC_PoseBuffer );
	
	Uint8 * buffer = static_cast< Uint8* >( m_buffer.Get() );

	for( Uint32 index = 0; index != maxAvailable; ++index )
	{
		m_buffers.PushBack( buffer + ( index * size ) );
	}
}

Red::UniqueBuffer CPosePool::Bucket::Acquire()
{
	RED_WARNING_ONCE( !m_buffers.Empty(), "CPosePool Out of Buffer for size %d", m_size );

	if( !m_buffers.Empty() )
	{
		return Red::UniqueBuffer( m_buffers.PopBack(), m_size, MC_PoseBuffer );
	}

	// Budget busted, fallback!
	return Red::CreateUniqueBuffer( m_size, c_bufferAlignment, MC_PoseBuffer );
}

void CPosePool::Bucket::Push( Red::UniqueBuffer buffer )
{
	Uint8 * internalBuffer = static_cast< Uint8 * >( m_buffer.Get() );
	if( buffer.Get() >= internalBuffer && buffer.Get() < internalBuffer + m_buffer.GetSize() )
	{
		m_buffers.PushBack( buffer.Release() );
	}
	else
	{ /* Not one of mine. Budget was most likely busted. */ }
}

Uint32 CPosePool::Bucket::GetSize() const
{
	return m_size;
}

CPosePool::CPosePool()
	:	m_poseBlockBuffer( nullptr ),
		m_poseBuffer( nullptr ),
		m_eventFiredBuffer(nullptr ),
		m_availableBlock( 0 ),
		m_blockCount( c_blockCount ),
		m_posePerBlock( c_posePerBlock )
{}

CPosePool::~CPosePool()
{
	delete [] m_poseBlockBuffer;
	delete [] m_poseBuffer;
	delete [] m_eventFiredBuffer;
}

void CPosePool::Initialize()
{
	InitializeBucketContainer();

#ifndef POSE_POOL_TRACK_MEMORY_STOMP

	m_poseBlockBuffer = new CPoseBlock[ m_blockCount ];
	m_poseBuffer = new SBehaviorGraphOutput[ m_blockCount * m_posePerBlock ];
	m_eventFiredBuffer = new CAnimationEventFired[ m_blockCount * m_posePerBlock * c_eventFiredDefaultCount ];

	m_freePoseBlockContainer.Reserve( m_blockCount );
	for( Uint32 index = 0; index != m_blockCount; ++index )
	{
		CPoseBlock & block = m_poseBlockBuffer[ index ];
		SBehaviorGraphOutput * poses = m_poseBuffer + ( m_posePerBlock * index );
		CAnimationEventFired * events = m_eventFiredBuffer + ( m_posePerBlock * c_eventFiredDefaultCount * index );

		block.SetPoses( m_posePerBlock, poses, events );

		m_freePoseBlockContainer.PushBack( m_poseBlockBuffer + index );
	}

	m_availableBlock.SetValue( m_blockCount );

#endif
}

void CPosePool::InitializeBucketContainer()
{
	// ctremblay This is shit. I tweaked the value manually ...
	// Also, I'm not happy with this "solution" but since buffer are quite random size, 
	// it is difficult to implement a correct solution without heap management and fighting with fragmentation. 
	// To revisit !

	m_bucketContainer.Resize( 9 );
	
#ifndef POSE_POOL_TRACK_MEMORY_STOMP	
	m_bucketContainer[ 0 ].Initialize( 2 * 1024, 160 );
	m_bucketContainer[ 1 ].Initialize( 4 * 1024, 128 );
	m_bucketContainer[ 2 ].Initialize( 8 * 1024, 128 );
	m_bucketContainer[ 3 ].Initialize( 16 * 1024, 128 );
	m_bucketContainer[ 4 ].Initialize( 32 * 1024, 16 );
	m_bucketContainer[ 5 ].Initialize( 64 * 1024, 16 );
	m_bucketContainer[ 6 ].Initialize( 96 * 1024, 4 );
	m_bucketContainer[ 7 ].Initialize( 128 * 1024, 4 );
	m_bucketContainer[ 8 ].Initialize( 160 * 1024, 96 );
#endif
}

CPoseBlock * CPosePool::TakeBlock( const CSkeleton * skeleton )
{
	CPoseBlock * block = nullptr;

	ReclaimQuarantineBlock();
	
	if( m_availableBlock.GetValue() != 0 )
	{
		void * poseBuffer = nullptr;

		{
			// This can be made without lock, but I doubt it will be faster than a basic spin lock as I will anyway have to CompareExchange a bunch of atomic.
			// However worst case could be limited to only check last know free block. Need to profile...
			Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_poseLock );
			if( !m_freePoseBlockContainer.Empty() )
			{
				m_availableBlock.Decrement();
				block = m_freePoseBlockContainer.PopBack();
			}
		}

		RED_WARNING_ONCE( block, "No more CPoseBlock available. Increase budget. CPoseProvider will Fallback on dynamic allocation." );

		if( block )
		{
			RED_FATAL_ASSERT( skeleton, "Cannot initialize CPoseBlock without CSkeleton." );

			const Uint32 boneBufferSize = skeleton->GetBonesNum() * m_posePerBlock * sizeof( AnimQsTransform );
			const Uint32 floatTrackBufferSize = skeleton->GetTracksNum() * m_posePerBlock * sizeof( AnimFloat );

			Red::UniqueBuffer boneBuffer = AcquireBuffer( boneBufferSize );
			Red::UniqueBuffer floatTrackBuffer = AcquireBuffer( floatTrackBufferSize );

			block->Initialize( skeleton, boneBuffer, floatTrackBuffer );
		}
		else
		{ /* another thread beat us and took last block. */ }
	}
	
	return block;
}

void CPosePool::ReclaimQuarantineBlock()
{
	if( !m_quarantineBlockContainer.Empty() )
	{
		PoseBlockContainer quarantineBlockContainer;
		quarantineBlockContainer.Reserve( m_quarantineBlockContainer.Size() );
		{
			Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_poseLock );
			quarantineBlockContainer.SwapWith( m_quarantineBlockContainer );
		}

		for( auto iter = quarantineBlockContainer.Begin(), end = quarantineBlockContainer.End(); iter != end; ++iter )
		{
			GiveBlock( *iter );
		}
	}
}

Red::UniqueBuffer CPosePool::AcquireBuffer( Uint32 size )
{
	Red::UniqueBuffer buffer;

	if( size )
	{
		Bucket * bucket = GetBucket( size );

		RED_WARNING_ONCE( bucket, "CPosePool -> Requested buffer size %d is bigger than max size supported.", size );

		if( bucket )
		{
			Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_bucketLock );
			return bucket->Acquire();
		}
		else
		{
			buffer = Red::CreateUniqueBuffer( size, c_bufferAlignment, MC_PoseBuffer );
		}
	}

	return buffer;
}

CPosePool::Bucket * CPosePool::GetBucket( Uint32 size )
{
	BucketContainer::iterator iter = FindIf( m_bucketContainer.Begin(), m_bucketContainer.End(), [size]( const Bucket & bucket ){ return bucket.GetSize() >= size; } );
	if( iter != m_bucketContainer.End() )
	{
		return &(*iter);
	}

	return nullptr;
}

void CPosePool::GiveBlock( CPoseBlock * block )
{
	if( !block->IsAllPoseAvailable() )
	{
		Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_poseLock ); 
		m_quarantineBlockContainer.PushBack( block );
		return;
	}

	Red::UniqueBuffer boneBuffer = block->ReleaseBoneBuffer();
	Red::UniqueBuffer floatTrackBuffer = block->ReleaseFloatTrackBuffer();
	Bucket * boneBucket = boneBuffer ? GetBucket( boneBuffer.GetSize() ) : nullptr;
	Bucket * floatTrackBucket = floatTrackBuffer ? GetBucket( floatTrackBuffer.GetSize() ) : nullptr;

	{
		{
			// This can be made without lock, If I resolve TakeBlock lock one.
			Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_poseLock ); 
			m_freePoseBlockContainer.PushBack( block ); 
		}

		{
			Red::Threads::CScopedLock< Red::Threads::CSpinLock > scopedLock( m_bucketLock );
			if( boneBucket )
			{
				boneBucket->Push( std::move( boneBuffer ) );
			}

			if( floatTrackBucket )
			{
				floatTrackBucket->Push( std::move( floatTrackBuffer ) );
			}
		}
	}
	
	m_availableBlock.Increment();
}

Uint32 CPosePool::GetAvailableBlockCount() const
{
	return m_availableBlock.GetValue();
}

void CPosePool::SetInternalBlockCount( Uint32 count )
{
	RED_FATAL_ASSERT( Red::System::UnitTestMode(), "Cannot be called outside Unit Test environment." );
	m_blockCount = count;
}

void CPosePool::SetInternalPosePerBlockCount( Uint32 count )
{
	RED_FATAL_ASSERT( Red::System::UnitTestMode(), "Cannot be called outside Unit Test environment." );
	m_posePerBlock = count;
}

void CPosePool::PushInternalPoseBlock( CPoseBlock * block )
{
	RED_FATAL_ASSERT( Red::System::UnitTestMode(), "Cannot be called outside Unit Test environment." );
	m_freePoseBlockContainer.PushBack( block );
	m_availableBlock.Increment();
}

Uint32 CPosePool::GetInternalQuarantineBlockCount() const
{
	return m_quarantineBlockContainer.Size();
}

PosePoolHandle CreatePosePool()
{
	PosePoolHandle pool( new CPosePool );
	pool->Initialize();
	return pool;
}
