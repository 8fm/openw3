/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/poseBlock.h"

struct PoseBlockFixture : ::testing::Test
{
	PoseBlockFixture()
		:	poseBuffer( nullptr ),
			events( nullptr )
	{}

	~PoseBlockFixture()
	{
		delete [] poseBuffer;
		delete [] events;
	}

	void SetupBlock( Int32 count, Int32 boneCount, Int32 floatTrackCount )
	{
		poseBuffer = new SBehaviorGraphOutput[ count ];
		events = new CAnimationEventFired[ count * c_eventFiredDefaultCount ];
		Red::UniqueBuffer boneBuffer = Red::CreateUniqueBuffer( boneCount * count * sizeof( AnimQsTransform ), 16 );
		Red::UniqueBuffer floatTrackBuffer = Red::CreateUniqueBuffer( floatTrackCount * count * sizeof( AnimFloat ), 16 );
		block.SetPoses( count, poseBuffer, events );
	}

	CPoseBlock block;
	SBehaviorGraphOutput * poseBuffer;
	CAnimationEventFired * events;
};

TEST_F( PoseBlockFixture, IsPoseAvailable_return_false_by_default )
{
	EXPECT_FALSE( block.IsPoseAvailable() );
}

TEST_F( PoseBlockFixture, IsPoseAvailable_return_true_if_some_pose_are_available )
{
	SetupBlock( 3, 1, 1 );
	EXPECT_TRUE( block.IsPoseAvailable() );
}

TEST_F( PoseBlockFixture, TakePose_return_invalid_handle_if_no_pose_available )
{
	CPoseHandle handle = block.TakePose();
	EXPECT_FALSE( handle );
}

TEST_F( PoseBlockFixture, TakePose_return_valid_handle_if_available )
{
	SetupBlock( 3, 1, 1 );
	CPoseHandle handle = block.TakePose();

	EXPECT_TRUE( handle );
}

TEST_F( PoseBlockFixture, TakePose_return_invalid_after_all_pose_are_taken )
{
	SetupBlock( 2, 1, 1 );
	CPoseHandle firstHandle = block.TakePose();
	CPoseHandle secondHandle = block.TakePose();
	CPoseHandle thirdHandle = block.TakePose();

	EXPECT_TRUE( firstHandle );
	EXPECT_TRUE( secondHandle );
	EXPECT_TRUE( firstHandle != secondHandle );
	EXPECT_FALSE( thirdHandle );
}

TEST_F( PoseBlockFixture, IsPoseAvailable_return_false_after_all_pose_are_taken )
{
	SetupBlock( 2, 1, 1 );
	CPoseHandle firstHandle = block.TakePose();
	CPoseHandle secondHandle = block.TakePose();
	
	EXPECT_FALSE( block.IsPoseAvailable() );
}

TEST_F( PoseBlockFixture, TakePose_return_valid_handle_after_a_pose_became_available )
{
	SetupBlock( 2, 1, 1 );
	CPoseHandle firstHandle = block.TakePose();
	CPoseHandle secondHandle = block.TakePose();

	secondHandle.Reset();

	CPoseHandle thirdHandle = block.TakePose();
	EXPECT_TRUE( thirdHandle );
}

TEST_F( PoseBlockFixture, IsPoseAvailable_return_true_after_a_pose_became_available )
{
	SetupBlock( 2, 1, 1 );
	CPoseHandle firstHandle = block.TakePose();
	CPoseHandle secondHandle = block.TakePose();

	secondHandle.Reset();

	EXPECT_TRUE( block.IsPoseAvailable() );
}
