/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/engine/foliageScene.h"
#include "../../common/engine/engineTypeRegistry.h"
#include "foliageMocks.h"

using testing::Return;
using testing::Mock;
using testing::_;

struct FoliageSceneFixture : testing::Test
{
	static void SetUpTestCase()
	{
	}

	static void TearDownTestCase()
	{
	}

	FoliageSceneFixture()
		:	broker( new CFoliageBrokerMock ),
			resourceHandler( new CFoliageResourceHandlerMock ),
			renderCommandDipatcher( new CFoliageRenderCommandDispatcherMock )
	{
		scene.SetInternalStreamingConfig( true );
		scene.SetInternalFoliageBroker( broker );
		scene.SetInternalFoliageResourceHandler( Red::TUniquePtr< CFoliageResourceHandler >( resourceHandler ) );
		scene.SetInternalFoliageRenderCommandDispatcher( Red::TUniquePtr< IFoliageRenderCommandDispatcher >( renderCommandDipatcher ) );
	}

	~FoliageSceneFixture()
	{
		scene.SetFlag( OF_Finalized );
	}

	void ValidateMocks()
	{
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( broker.Get() ) );
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( resourceHandler ) );
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( renderCommandDipatcher ) );
	}

	void ExpectTick( Uint32 tickCount )
	{
		EXPECT_CALL( *resourceHandler, Tick() ).Times( tickCount );
		EXPECT_CALL( *broker, Tick() ).Times( tickCount );
	}

	void ExpectPrefetchSync( const Vector & position, Uint32 count )
	{
		EXPECT_CALL( *broker, PrefetchPosition( position ) ).Times( count );
		EXPECT_CALL( *resourceHandler, UpdateCurrentPosition( position ) ).Times( count );
		ExpectTick( count );
	}


	Red::TSharedPtr< CFoliageBrokerMock > broker;
	CFoliageResourceHandlerMock * resourceHandler;
	CFoliageRenderCommandDispatcherMock * renderCommandDipatcher;
	CFoliageScene scene;
};

TEST_F( FoliageSceneFixture, IsLoading_return_true_if_handler_is_loading )
{
	EXPECT_CALL( *resourceHandler, InstancesPendingInsertion() ).WillOnce( Return( true ) );
	EXPECT_TRUE( scene.IsLoading() );
}

TEST_F( FoliageSceneFixture, IsLoading_return_true_handler_is_done_but_not_broker )
{
	EXPECT_CALL( *resourceHandler, InstancesPendingInsertion() ).WillOnce( Return( false ) );
	EXPECT_CALL( *broker, IsLoading() ).WillOnce( Return( true ) );

	EXPECT_TRUE( scene.IsLoading() );
}

TEST_F( FoliageSceneFixture, IsLoading_return_false_if_handler_and_broker_are_done )
{
	EXPECT_CALL( *resourceHandler, InstancesPendingInsertion() ).WillOnce( Return( false ) );
	EXPECT_CALL( *broker, IsLoading() ).WillOnce( Return( false ) );

	EXPECT_FALSE( scene.IsLoading() );
}

TEST_F( FoliageSceneFixture, Tick_tick_broker_and_handler )
{
	ExpectTick( 1 );
	scene.Tick();
}

TEST_F( FoliageSceneFixture, PrefecthPositionSync_do_nothing_if_streaming_config_is_off )
{
	scene.SetInternalStreamingConfig( false );

	Vector position( 1.0f, 100.f, 0.0f ); 
	ExpectPrefetchSync( position, 0 );

	scene.PrefetchPositionSync( Vector( 1.0f, 100.f, 0.0f ) );

	ValidateMocks();
}

TEST_F( FoliageSceneFixture, PrefecthPositionSync_do_process_prefetch_request_if_streaming_config_is_on )
{
	Vector position( 1.0f, 100.f, 0.0f ); 

	ExpectPrefetchSync( position, 1 );

	scene.PrefetchPositionSync( position );

	ValidateMocks();
}

TEST_F( FoliageSceneFixture, PrefecthPositionSync_set_current_position )
{
	Vector position( 1.0f, 100.f, 0.0f ); 

	ExpectPrefetchSync( position, 1 );

	scene.PrefetchPositionSync( position );

	EXPECT_EQ( position, scene.GetCurrentPosition() );
}
