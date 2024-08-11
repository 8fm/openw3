/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_ENGINE_POSE_BLOCK_H_
#define _RED_ENGINE_POSE_BLOCK_H_

#include "poseHandle.h"
#include "../core/uniqueBuffer.h"

struct SBehaviorGraphOutput;
struct CAnimationEventFired;
class CSkeleton;

class CPoseBlock
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_Default, MC_PoseManagement );

public:
	CPoseBlock();
	RED_MOCKABLE ~CPoseBlock();

	RED_MOCKABLE void Initialize( const CSkeleton * skeleton, Red::UniqueBuffer & boneBuffer, Red::UniqueBuffer & trackBuffer );

	RED_MOCKABLE CPoseHandle TakePose();
	
	RED_MOCKABLE Bool IsPoseAvailable() const;
	RED_MOCKABLE Bool IsAllPoseAvailable() const;
	Uint32 GetPoseCount() const;
	Uint32 GetPoseAvailableCount() const;
	Uint32 GetMemoryUsage() const;
	Uint32 GetMemoryConsumed() const;

	void SignalPoseAvailable();

	void SetPoses( Uint32 poseCount, SBehaviorGraphOutput * poses, CAnimationEventFired * events );

	Red::UniqueBuffer ReleaseBoneBuffer();
	Red::UniqueBuffer ReleaseFloatTrackBuffer();

private:

	SBehaviorGraphOutput * m_poses;
	CAnimationEventFired * m_events;
	Uint32 m_poseCount;
	Red::Threads::CAtomic< Int32 > m_poseAvailable;
	
	Red::UniqueBuffer m_boneBuffer;
	Red::UniqueBuffer m_floatTrackBuffer;
};

#include "poseBlock.inl"

#endif
