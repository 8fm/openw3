/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/posePool.h"
#include "../../common/engine/skeleton.h"
#include "../unitTestsFramework/attorney.h"
#include "../../common/engine/poseBlock.h"
#include "poseMocks.h"

using ::testing::_;
using ::testing::Return;

struct PosePoolFixture : ::testing::Test
{
	PosePoolFixture()
		:	skeletonMock( new CSkeletonMock )
	{
		ON_CALL( *skeletonMock, IsValid() ).WillByDefault( Return( true ) );
	}

	void SetupDefaultInitialization( Int32 count )
	{
		pool.SetInternalBlockCount( count );
		pool.SetInternalPosePerBlockCount( count );
		
		for( Int32 index = 0; index != count; ++index )
		{
			Red::TUniquePtr< CPoseBlockMock > block( new CPoseBlockMock );
			pool.PushInternalPoseBlock( block.Get() );
			blockMocks.PushBack( std::move( block ) );
		}
	}

	CPosePool pool;

	Red::TScopedPtr< CSkeletonMock > skeletonMock;

	TDynArray< Red::TUniquePtr< CPoseBlockMock > > blockMocks;
};

TEST( PosePool, TakeBlock_return_null_if_not_initialized )
{
	CPosePool pool;
	EXPECT_TRUE( pool.TakeBlock( nullptr ) == nullptr );
}

TEST_F( PosePoolFixture, TakeBlock_initialize_block_before_retuning_it )
{
	SetupDefaultInitialization( 1 );

	EXPECT_CALL( *blockMocks[ 0 ], Initialize( skeletonMock.Get(), _, _ ) ).Times( 1 );

	CPoseBlock * block = pool.TakeBlock( skeletonMock.Get() );
	EXPECT_EQ( blockMocks[ 0 ].Get(), block );
}

TEST_F( PosePoolFixture, TakeBlock_assert_if_skeleton_is_null )
{
	SetupDefaultInitialization( 1 );
	EXPECT_DEATH_IF_SUPPORTED( pool.TakeBlock( nullptr ), "" );
}  

TEST_F( PosePoolFixture, TakeBlock_return_block_if_available )
{
	SetupDefaultInitialization( 1 );
	EXPECT_TRUE( pool.TakeBlock( skeletonMock.Get() ) != nullptr );
}

TEST_F( PosePoolFixture, TakeBlock_return_null_if_all_block_are_taken )
{
	SetupDefaultInitialization( 2 );

	EXPECT_TRUE( pool.TakeBlock( skeletonMock.Get() ) != nullptr );
	EXPECT_TRUE( pool.TakeBlock( skeletonMock.Get() ) != nullptr );
	EXPECT_TRUE( pool.TakeBlock( skeletonMock.Get() ) == nullptr );
}

TEST_F( PosePoolFixture, GiveBlock_increment_avaialble_block_count )
{
	CPoseBlockMock block;
	EXPECT_CALL( block, IsAllPoseAvailable() ).WillOnce( Return( true ) );
	pool.GiveBlock( &block );
	EXPECT_EQ( 1, pool.GetAvailableBlockCount() );
}

TEST_F( PosePoolFixture, TakeBlock_return_last_block_given_via_GiveBlock )
{
	CPoseBlockMock blockMock;

	EXPECT_CALL( blockMock, Initialize( skeletonMock.Get(), _, _ ) ).Times( 1 );
	EXPECT_CALL( blockMock, IsAllPoseAvailable() ).WillOnce( Return( true ) );

	pool.GiveBlock( &blockMock );
	CPoseBlock * block = pool.TakeBlock( skeletonMock.Get() );
	EXPECT_EQ( &blockMock, block );
}

TEST_F( PosePoolFixture, GiveBlock_push_block_to_quarantine_zone_if_some_pose_are_leaked )
{
	CPoseBlockMock blockMock;

	EXPECT_CALL( blockMock, IsAllPoseAvailable() ).WillOnce( Return( false ) );

	pool.GiveBlock( &blockMock );
	
	EXPECT_EQ( 1, pool.GetInternalQuarantineBlockCount() );
}

TEST_F( PosePoolFixture, TakeBlock_push_block_from_quarantine_zone_to_available_zone_if_leak_is_resolve )
{
	CPoseBlockMock blockMock;

	EXPECT_CALL( blockMock, IsAllPoseAvailable() ).WillOnce( Return( false ) );

	pool.GiveBlock( &blockMock );

	EXPECT_CALL( blockMock, IsAllPoseAvailable() ).WillOnce( Return( true ) );

	CPoseBlock * block = pool.TakeBlock( skeletonMock.Get() );

	EXPECT_EQ( block, &blockMock );
	EXPECT_EQ( 0, pool.GetInternalQuarantineBlockCount() );
}
