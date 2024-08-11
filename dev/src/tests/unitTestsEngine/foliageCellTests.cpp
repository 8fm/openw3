/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageMocks.h"

#include "../../common/engine/foliageCell.h"

#include "../../common/core/scopedPtr.h"

using testing::Return;
using testing::DefaultValue;

class SoftHandleMock : public CSoftHandleProxy
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:

	MOCK_CONST_METHOD0( GetAsync, BaseSoftHandle::EAsyncLoadingResult() );
	MOCK_CONST_METHOD0( Get, CFoliageResource * () );
	MOCK_CONST_METHOD0( IsLoaded, bool() );
};

const Vector2 CellPosition = Vector2( 64.0f, 64.0f );


struct FoliageCellFixture : public ::testing::Test
{
	FoliageCellFixture()
		:	loaderMock( new CFoliageResourceLoaderMock ),
			resourceMock( new SoftHandleMock ),
			cell( new CFoliageCell )
	{
		ON_CALL( *resourceMock, Get() ).WillByDefault( Return( &resource ) );
	}

	static void SetUpTestCase()
	{
	}

	static void TearDownTestCase()
	{
	}


	void SetupCell_InvalidResource()
	{
		EXPECT_CALL( *loaderMock, GetResourceHandle( CellPosition ) )
			.Times( 1 )
			.WillOnce( Return( Red::TSharedPtr< SoftHandleMock >() ) );

		cell->Setup( CellPosition, loaderMock.Get() );
	}
	
	void SetupCell_ValidResource( bool resourceAlreadyLoaded )
	{
		EXPECT_CALL( *loaderMock, GetResourceHandle( CellPosition ) )
			.Times( 1 )
			.WillOnce( Return( resourceMock ) );

		EXPECT_CALL( *resourceMock, IsLoaded() ).WillOnce( Return( resourceAlreadyLoaded ) );

		EXPECT_CALL( *loaderMock, ResourceAcquired( resourceMock->Get(), testing::_ ) )
			.Times( resourceAlreadyLoaded ? 1 : 0 );

		cell->Setup( CellPosition, loaderMock.Get() );
	}
	
	
	Red::TScopedPtr< CFoliageResourceLoaderMock > loaderMock;
	Red::TSharedPtr< SoftHandleMock > resourceMock;
	
	Red::TScopedPtr< CFoliageCell > cell;

	CFoliageResourceMock resource;
};

TEST_F( FoliageCellFixture, GetWorldCoordinate_return_correct_position )
{
	const Vector2 position( -128.0f, 1024.0f );
	
	EXPECT_CALL( *loaderMock, GetResourceHandle( position ) )
		.Times( 1 )
		.WillOnce( Return( Red::TSharedPtr< SoftHandleMock >() ) );

	cell->Setup( position, loaderMock.Get() );
	EXPECT_EQ( position, cell->GetWorldCoordinate() );
}

TEST_F( FoliageCellFixture, Setup_trigger_GetResourceHandle_request )
{
	SetupCell_InvalidResource();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
}

TEST_F( FoliageCellFixture, Tick_request_GetAsync_on_softhandle )
{	
	SetupCell_ValidResource( false );

	EXPECT_CALL( *resourceMock, GetAsync( ) )
		.Times( 1 )
		.WillOnce( Return( BaseSoftHandle::ALR_InProgress ) );

	cell->Tick();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( resourceMock.Get() ) );
	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
}

TEST_F( FoliageCellFixture, Tick_do_not_request_GetAsync_again_if_resource_is_loaded )
{
	SetupCell_ValidResource( false );

	EXPECT_CALL( *resourceMock, GetAsync( ) )
		.Times( 1 )
		.WillOnce( Return( BaseSoftHandle::ALR_Loaded ) );

	EXPECT_CALL( *loaderMock, ResourceAcquired( resourceMock->Get(), testing::_ ) )
		.Times( 1 );

	cell->Tick();
	cell->Tick();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( resourceMock.Get() ) );
	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
}

TEST_F( FoliageCellFixture, Tick_signal_resource_acquired_if_loaded_for_first_time )
{
	SetupCell_ValidResource( false );

	EXPECT_CALL( *resourceMock, GetAsync( ) )
		.Times( 1 )
		.WillOnce( Return( BaseSoftHandle::ALR_Loaded ) );

	EXPECT_CALL( *loaderMock, ResourceAcquired( resourceMock->Get(), testing::_ ) )
		.Times( 1 );

	cell->Tick();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( resourceMock.Get() ) );
	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
}

TEST_F( FoliageCellFixture, Destructor_do_not_trigger_unload_request_if_resource_not_loaded )
{
	SetupCell_ValidResource( false );

	EXPECT_CALL( *loaderMock, ResourceReleased( resourceMock->Get() ) )
		.Times( 0 );

	cell.Reset();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
}

TEST_F( FoliageCellFixture, Destructor_do_trigger_unload_request_if_resource_is_loaded )
{
	SetupCell_ValidResource( true );

	EXPECT_CALL( *loaderMock, ResourceReleased( resourceMock->Get() ) )
		.Times( 1 );
	
	cell.Reset();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
}

TEST_F( FoliageCellFixture, GetFoliageResource_return_null_if_resourcehandle_is_null )
{
	CFoliageCell cell;
	EXPECT_TRUE( cell.GetFoliageResource() == nullptr );
}

TEST_F( FoliageCellFixture, GetFoliageResource_return_null_if_resource_is_not_loaded_yet )
{
	SetupCell_ValidResource( false );

	EXPECT_TRUE( cell->GetFoliageResource() == nullptr );
}

TEST_F( FoliageCellFixture, GetFoliageResource_return_resource_if_loaded )
{
	SetupCell_ValidResource( true );

	EXPECT_EQ( &resource, cell->GetFoliageResource() );
}

TEST_F( FoliageCellFixture, AcquireFoliageResource_force_create_foliage_resource )
{
	SetupCell_InvalidResource();

	EXPECT_CALL( *loaderMock, CreateResource( CellPosition ) )
		.Times( 1 )
		.WillOnce( Return( resourceMock ) );
	
	EXPECT_CALL( *loaderMock, ResourceAcquired( resourceMock->Get(), testing::_ ) )
		.Times( 1 );

	cell->AcquireFoliageResource();

	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( loaderMock.Get() ) );
	EXPECT_TRUE( ::testing::Mock::VerifyAndClearExpectations( resourceMock.Get() ) );
}

TEST_F( FoliageCellFixture, IsLoading_return_false_if_resource_is_invalid )
{
	SetupCell_InvalidResource();
	EXPECT_FALSE( cell->IsLoading() );
}

TEST_F( FoliageCellFixture, IsLoading_return_false_if_resource_valid_and_loaded )
{
	SetupCell_ValidResource( true );
	EXPECT_FALSE( cell->IsLoading() );
}

TEST_F( FoliageCellFixture, IsLoading_return_true_if_resource_valid_and_not_loaded )
{
	SetupCell_ValidResource( false );
	EXPECT_TRUE( cell->IsLoading() );
}
