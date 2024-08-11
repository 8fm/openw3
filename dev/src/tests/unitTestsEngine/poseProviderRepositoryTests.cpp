/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../unitTestsFramework/attorney.h"
#include "../../common/engine/poseProviderRepository.h"
#include "../../common/engine/poseProvider.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/poseProviderFactory.h"
#include "../../common/engine/posePool.h"
#include "poseMocks.h"


using ::testing::_;
using ::testing::Mock;
using ::testing::Return;
using ::testing::ReturnRef;

struct PoseProviderRepositoryFixture : ::testing::Test
{
	PoseProviderRepositoryFixture()
		:	factoryMock( new CPoseProviderFactoryMock )
	{
		repository.SetInternalProviderFactory( Red::TUniquePtr< CPoseProviderFactory >( factoryMock ) );
	}

	void SetupAcquireProvider( Uint32 count )
	{
		for( Uint32 index = 0; index != count; ++index )
		{
			Red::TUniquePtr< CSkeletonMock > skeleton( new CSkeletonMock );
			Red::TAtomicSharedPtr< CPoseProviderMock > provider( new CPoseProviderMock );
			provider->SetInternalSkeleton( skeleton.Get() );

			EXPECT_CALL( *factoryMock, CreatePoseProvider( skeleton.Get() ) )
				.WillOnce( Return( provider ) );
			
			skeletonMocks.PushBack( std::move( skeleton ) );
			providerMocks.PushBack( provider );
		}
	}

	CPoseProviderRepository repository;

	CPoseProviderFactoryMock * factoryMock;
	TDynArray< Red::TUniquePtr< CSkeletonMock > > skeletonMocks;
	TDynArray< Red::TAtomicSharedPtr< CPoseProviderMock > > providerMocks;
};

TEST_F( PoseProviderRepositoryFixture, AcquireProvider_assert_if_skeleton_is_null )
{
	EXPECT_DEATH_IF_SUPPORTED( repository.AcquireProvider( nullptr ), "" );
}

TEST_F( PoseProviderRepositoryFixture, AcquireProvider_return_valid_provider )
{
	SetupAcquireProvider( 1 );
	PoseProviderHandle handle = repository.AcquireProvider( skeletonMocks[ 0 ].Get() );
	EXPECT_EQ( providerMocks[ 0 ], handle );
}

TEST_F( PoseProviderRepositoryFixture, AcquireProvider_will_always_return_same_provider_for_same_skeleton )
{
	SetupAcquireProvider( 1 );

	PoseProviderHandle firstCall = repository.AcquireProvider( skeletonMocks[ 0 ].Get() );
	PoseProviderHandle secondCall = repository.AcquireProvider( skeletonMocks[ 0 ].Get() );

	EXPECT_EQ( firstCall, secondCall );
}

TEST_F( PoseProviderRepositoryFixture, AcquireProvider_return_different_provider_for_different_skeleton )
{
	SetupAcquireProvider( 2 );

	PoseProviderHandle firstCall = repository.AcquireProvider( skeletonMocks[ 0 ].Get() );
	PoseProviderHandle secondCall = repository.AcquireProvider( skeletonMocks[ 1 ].Get() );

	EXPECT_TRUE( firstCall != secondCall );
}

TEST_F( PoseProviderRepositoryFixture, Sanitize_call_sanitize_on_all_valid_provider )
{
	SetupAcquireProvider( 2 );

	EXPECT_CALL( *providerMocks[ 0 ], Sanitize() ).Times( 1 );
	EXPECT_CALL( *providerMocks[ 1 ], Sanitize() ).Times( 1 );

	PoseProviderHandle firstCall = repository.AcquireProvider( skeletonMocks[ 0 ].Get() );
	PoseProviderHandle secondCall = repository.AcquireProvider( skeletonMocks[ 1 ].Get() );

	repository.Sanitize();
}
