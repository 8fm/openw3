/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../unitTestsFramework/attorney.h"
#include "../../common/engine/poseProvider.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/posePool.h"
#include "../../common/engine/poseBlock.h"
#include "poseMocks.h"

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;


struct PoseProviderFixture : ::testing::Test
{
	class PoseProvider : public CPoseProvider
	{
	public:
		MOCK_CONST_METHOD0( AllocateFallbackPose, CPoseHandle() );
	};

	PoseProviderFixture()
		:	skeletonMock( new CSkeletonMock ),
			poolMock( new CPosePoolMock ),
			blockMock( new CPoseBlockMock ),
			poseMock( new CPoseMock )
	{
		provider.SetInternalSkeleton( skeletonMock.Get() );
		provider.SetInternalPosePool( poolMock );
		poseMock->AddRef();
		poseMockHandle = CPoseHandle( poseMock, nullptr );
	}

	~PoseProviderFixture()
	{
		provider.SetInternalPosePool( PosePoolHandle() ); 
	}

	PoseProvider provider;
	Red::TScopedPtr< CSkeletonMock > skeletonMock;
	Red::TAtomicSharedPtr< CPosePoolMock > poolMock;
	Red::TScopedPtr< CPoseBlockMock > blockMock;
	CPoseMock * poseMock;
	CPoseHandle poseMockHandle;
};

TEST( PoseProvider, AcquirePose_assert_if_no_skeleton )
{
	CPoseProvider provider;
	EXPECT_DEATH_IF_SUPPORTED( provider.AcquirePose(), "" );
}

TEST_F( PoseProviderFixture, AcquirePose_return_correctly_initialized_fallback_pose_if_no_pose_available_anywhere )
{
	EXPECT_CALL( *poolMock, TakeBlock( skeletonMock.Get() ) )
		.WillOnce( Return( nullptr ) );

	EXPECT_CALL( provider, AllocateFallbackPose() )
		.WillOnce( Return( poseMockHandle ) );

	EXPECT_CALL( *poseMock, Init( _,_,true ) ).Times( 1 );
	EXPECT_CALL( *poseMock, Reset( skeletonMock.Get() ) ).Times( 1 );

	CPoseHandle handle = provider.AcquirePose();
	
	EXPECT_TRUE( handle );
	EXPECT_EQ( poseMockHandle, handle );
}

TEST_F( PoseProviderFixture, AcquirePose_return_pose_from_block_if_available )
{
	provider.PushInternalPoseBlock( blockMock.Get() );

	EXPECT_CALL( *blockMock, IsPoseAvailable() )
		.WillOnce( Return( true ) );

	EXPECT_CALL( *blockMock, TakePose() )
		.WillOnce( Return( poseMockHandle ) );

	CPoseHandle handle = provider.AcquirePose();
	EXPECT_TRUE( handle );
	EXPECT_EQ( poseMockHandle, handle );
}

TEST_F( PoseProviderFixture, AcquirePose_allocate_pose_block_if_none_available_and_return_pose )
{
	EXPECT_CALL( *poolMock, TakeBlock( skeletonMock.Get() ) )
		.WillOnce( Return( blockMock.Get() ) );

	EXPECT_CALL( *blockMock, IsPoseAvailable() )
		.WillOnce( Return( true ) );

	EXPECT_CALL( *blockMock, TakePose() )
		.WillOnce( Return( poseMockHandle ) );

	CPoseHandle handle = provider.AcquirePose();
	
	EXPECT_TRUE( handle );
	EXPECT_EQ( poseMockHandle, handle );
}

TEST_F( PoseProviderFixture, dtor_give_all_block_back_to_pool )
{
	provider.PushInternalPoseBlock( blockMock.Get() );
	EXPECT_CALL( *poolMock, GiveBlock( blockMock.Get() ) )
		.Times( 1 );

	provider.ReleaseInternalPoseBlocks();

	EXPECT_TRUE( Mock::VerifyAndClearExpectations( poolMock.Get() ) );
}
