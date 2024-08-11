/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_ENGINE_POSE_POOL_H_
#define _RED_ENGINE_POSE_POOL_H_

#include "poseTypes.h"
#include "../core/uniqueBuffer.h"

class CPoseBlock;
struct SBehaviorGraphOutput;
struct CAnimationEventFired;
class CSkeleton;

class CPosePool
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_PoseManagement );

public:

	CPosePool();
	RED_MOCKABLE ~CPosePool();

	void Initialize();

	RED_MOCKABLE CPoseBlock * TakeBlock( const CSkeleton * skeleton );
	RED_MOCKABLE void GiveBlock( CPoseBlock * block );

	Uint32 GetAvailableBlockCount() const;

	void SetInternalBlockCount( Uint32 count );
	void SetInternalPosePerBlockCount( Uint32 count );
	Uint32 GetInternalQuarantineBlockCount() const;
	void PushInternalPoseBlock( CPoseBlock * block );

private:

	class Bucket
	{
	public:
		Bucket();
		
		void Initialize( Uint32 size, Uint32 maxAvailable );
		Red::UniqueBuffer Acquire();
		void Push( Red::UniqueBuffer buffer );

		Uint32 GetSize() const;
	
	private:
	
		typedef TDynArray< void *, MC_PoseManagement > BufferContainer;

		Uint32 m_size;
		BufferContainer m_buffers;
		Red::UniqueBuffer m_buffer;
	};

	typedef TDynArray< Bucket, MC_PoseManagement > BucketContainer;
	typedef TDynArray< CPoseBlock*, MC_PoseManagement > PoseBlockContainer;

	void InitializeBucketContainer();
	Red::UniqueBuffer AcquireBuffer( Uint32 size ); 
	Bucket * GetBucket( Uint32 size );
	void ReclaimQuarantineBlock();

	PoseBlockContainer m_freePoseBlockContainer;
	PoseBlockContainer m_quarantineBlockContainer;
	BucketContainer m_bucketContainer;

	CPoseBlock * m_poseBlockBuffer;
	SBehaviorGraphOutput * m_poseBuffer;
	CAnimationEventFired * m_eventFiredBuffer;

	Red::Threads::CAtomic< Int32 > m_availableBlock;

	Uint32 m_blockCount;
	Uint32 m_posePerBlock;

	Red::Threads::CSpinLock m_bucketLock;
	Red::Threads::CSpinLock m_poseLock;
};

PosePoolHandle CreatePosePool();

#endif
