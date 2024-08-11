/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageMocks.h"

#include "../../common/engine/foliageBroker.h"
#include "../../common/engine/foliageGrid.h"
#include "../../common/engine/foliageCell.h"

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;

class FoliageBrokerFixture : public ::testing::Test
{
public:

	FoliageBrokerFixture()
		:	grid( new CFoliageGridMock ),
			loader( new CFoliageResourceLoaderMock )
	{}

	virtual void SetUp()
	{
		broker.SetInternalGrid( grid );
		broker.SetInternalLoader( loader );
	}

	CellHandleContainer CreateCells( Uint32 count )
	{
		CellHandleContainer container;
		container.Reserve( count );
		for( Uint32 index = 0; index != count; ++index )
		{
			container.PushBack( CellHandle( new CFoliageCellMock ) );
		}

		return container;
	}

	void SetupCells( Uint32 count )
	{
		cells = CreateCells( count );
	}

	void ValidateMocks()
	{
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( grid ) );
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( loader ) );

		for( Int32 index = 0, end = cells.Size(); index != end; ++index )
		{
			EXPECT_TRUE( Mock::VerifyAndClearExpectations( cells[index].Get() ) );
		}
	}

	CFoliageBroker broker;
	CFoliageGridMock * grid;
	CFoliageResourceLoaderMock * loader;
	CellHandleContainer cells; 
};

TEST_F( FoliageBrokerFixture, UpdateVisibleCells_keeps_reference_to_single_cell_visible )
{
	SetupCells( 1 );
	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 0 );

	EXPECT_CALL( *grid, AcquireVisibleCells( position, 0 ) )
		.WillOnce( Return( cells ) );

	broker.UpdateVisibileCells( position );

	EXPECT_EQ( 1, broker.VisibleCellCount() );

	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, UpdateVisibleCells_keeps_reference_to_multiple_cell_visible )
{
	SetupCells( 3 );
	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 1 );

	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	broker.UpdateVisibileCells( position );

	EXPECT_EQ( 3, broker.VisibleCellCount() );
	
	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, UpdateVisibleCells_release_cell_not_visible )
{
	const Vector2 position = Vector2( 0.0f, 0.0f );
	SetupCells( 3 );
	CellHandleContainer cells2;
	cells2.PushBack( cells[ 0 ] );
	CFoliageCellMock * visibleCell = static_cast< CFoliageCellMock *>( cells[ 0 ].Get() );
	
	{
		EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
			.WillOnce( Return( cells ) )
			.WillOnce( Return( cells2 ) )
			.RetiresOnSaturation();

		EXPECT_CALL( *static_cast< CFoliageCellMock *>( visibleCell ), Die() ).Times(0);
		EXPECT_CALL( *static_cast< CFoliageCellMock *>( cells[1].Get() ), Die() ).Times(1);
		EXPECT_CALL( *static_cast< CFoliageCellMock *>( cells[2].Get() ), Die() ).Times(1);
		
		cells.Clear();
		cells2.Clear();

		broker.SetVisibilityDepth( 1 );
		broker.UpdateVisibileCells( position );
		broker.UpdateVisibileCells( position );
	
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( grid ) );
		EXPECT_TRUE( Mock::VerifyAndClearExpectations( visibleCell ) );
	}
}

TEST_F( FoliageBrokerFixture, Tick_call_tick_on_all_visible_cell )
{
	SetupCells( 3 );
	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 1 );
	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	for( Int32 index = 0, end = cells.Size(); index != end; ++index )
	{
		EXPECT_CALL( *static_cast< CFoliageCellMock *>(cells[index].Get()), Tick() )
			.Times( 1 );
	}

	broker.UpdateVisibileCells( position );

	broker.Tick();

	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, IsLoading_return_false_if_no_visible_cell )
{
	EXPECT_FALSE( broker.IsLoading() );
}

TEST_F( FoliageBrokerFixture, IsLoading_return_false_if_all_visible_cell_are_not_loading )
{
	SetupCells( 3 );
	for( Int32 index = 0, end = cells.Size(); index != end; ++index )
	{
		EXPECT_CALL( *static_cast< CFoliageCellMock *>(cells[index].Get()), IsLoading() )
			.WillOnce( Return( false ) );
	}

	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 1 );
	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	broker.UpdateVisibileCells( position );

	EXPECT_FALSE( broker.IsLoading() );

	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, IsLoading_return_true_if_at_least_one_visible_cell_is_loading )
{
	SetupCells( 3 );

	EXPECT_CALL( *static_cast< CFoliageCellMock *>(cells[0].Get()), IsLoading() ).WillOnce( Return( false ) );
	EXPECT_CALL( *static_cast< CFoliageCellMock *>(cells[1].Get()), IsLoading() ).WillOnce( Return( false ) );
	EXPECT_CALL( *static_cast< CFoliageCellMock *>(cells[2].Get()), IsLoading() ).WillOnce( Return( true ) );

	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 1 );
	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	broker.UpdateVisibileCells( position );

	EXPECT_TRUE( broker.IsLoading() );

	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, PrefetchPosition_set_current_position )
{
	SetupCells( 3 );
	const Vector2 position( 10.0f, 100.0f );
	broker.SetVisibilityDepth( 1 );

	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	broker.PrefetchPosition( position );

	EXPECT_EQ( position, broker.GetCurrentPosition() );

	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, PrefetchPosition_keeps_reference_to_cells )
{
	SetupCells( 3 );
	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 1 );

	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	broker.PrefetchPosition( position );

	EXPECT_EQ( 3, broker.VisibleCellCount() );

	ValidateMocks();
}

TEST_F( FoliageBrokerFixture, UpdateVisibleCells_override_PrefetchPosition )
{
	SetupCells( 3 );
	CellHandleContainer newCells = CreateCells( 3 );

	const Vector2 position = Vector2( 0.0f, 0.0f );

	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) )
		.WillOnce( Return( newCells ) )
		.RetiresOnSaturation();

	EXPECT_CALL( *static_cast< CFoliageCellMock *>( cells[0].Get() ), Die() ).Times(1);
	EXPECT_CALL( *static_cast< CFoliageCellMock *>( cells[1].Get() ), Die() ).Times(1);
	EXPECT_CALL( *static_cast< CFoliageCellMock *>( cells[2].Get() ), Die() ).Times(1);

	cells.Clear();
	newCells.Clear();

	broker.SetVisibilityDepth( 1 );
	broker.PrefetchPosition( position );
	broker.UpdateVisibileCells( position );
}

TEST_F( FoliageBrokerFixture, PrefetchPosition_forward_all_cells_to_CFoliageResourceLoader_for_actual_loading )
{
	SetupCells( 3 );
	const Vector2 position = Vector2( 0.0f, 0.0f );
	broker.SetVisibilityDepth( 1 );

	EXPECT_CALL( *grid, AcquireVisibleCells( position, 1 ) )
		.WillOnce( Return( cells ) );

	EXPECT_CALL( *loader, PrefetchAllResource( cells ) )
		.Times( 1 );

	broker.PrefetchPosition( position );
}
