/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageMocks.h"
#include "../../common/engine/foliageGrid.h"

using testing::_;
using testing::Return;

struct FoliageGridFixture : public ::testing::Test
{
	FoliageGridFixture()
	{
		ON_CALL( resourceLoaderMock, GetResourceHandle(_) )
			.WillByDefault( Return( FoliageResourceHandle() ) ); 
	}

	void SingleCellGridSetup()
	{
		SFoliageGridSetupParameters param = { Vector2( 128.0f, 128.0f ), Vector2( 128.0f, 128.0f ), &resourceLoaderMock };
		grid.Setup( param );
	}

	void GridSetup_9Cell()
	{
		SFoliageGridSetupParameters param = { Vector2( 192.0f, 192.0f ), Vector2( 64.0f, 64.0f ), &resourceLoaderMock };
		grid.Setup( param );
	}

	CFoliageGrid grid;
	CFoliageResourceLoaderMock resourceLoaderMock;
};

#if 0 // too heavy ...
TEST_F( FoliageGridFixture, Setup_Assert_if_cell_dimension_are_less_than_1 )
{
	SFoliageGridSetupParameters param = { Vector2( 128.0f, 128.0f ), Vector2( 0.0f, 0.0f ) };
	EXPECT_DEATH_IF_SUPPORTED( grid.Setup( param ), "" );
}
#endif

TEST_F( FoliageGridFixture, Setup_create_a_minimum_of_one_cell )
{
	SFoliageGridSetupParameters param = { Vector2( 128.0f, 128.0f ), Vector2( 256.0f, 256.0f ) }; // cell 2 times bigger than whole world
	grid.Setup( param );
	EXPECT_EQ( 1, grid.GetCellCountX() );
	EXPECT_EQ( 1, grid.GetCellCountY() );
}

TEST_F( FoliageGridFixture, Setup_create_correct_amount_of_cells )
{
	SFoliageGridSetupParameters param = { Vector2( 128.0f, 128.0f ), Vector2( 32.0f, 16.0f ) };
	grid.Setup( param );
	EXPECT_EQ( 4, grid.GetCellCountX() );
	EXPECT_EQ( 8, grid.GetCellCountY() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_empty_list_by_default )
{
	const Vector position = Vector2( 0.0f, 0.0f );
	EXPECT_TRUE( grid.AcquireVisibleCells( position, 0 ).Empty() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_single_cells_if_only_one_cell )
{
	SingleCellGridSetup();
	const Vector2 position = Vector2( 0.0f, 0.0f );
	EXPECT_EQ( 1, grid.AcquireVisibleCells( position, 0 ).Size() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_valid_cells )
{
	SingleCellGridSetup();
	const Vector2 position = Vector2( 0.0f, 0.0f );
	CellHandleContainer cells = grid.AcquireVisibleCells( position, 0 );
	ASSERT_EQ( 1, cells.Size() );
	EXPECT_TRUE( cells[ 0 ] != nullptr );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_correct_range_if_visibility_depth_is_more_than_1 )
{
	GridSetup_9Cell();
	const Vector2 position = Vector2( 0.0f, 0.0f );
	CellHandleContainer cells = grid.AcquireVisibleCells( position, 1 );
	ASSERT_EQ( 9, cells.Size() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_correct_range_if_top_left_corner_selected )
{
	GridSetup_9Cell();
	const Vector2 position = Vector2( -192.0f, 192.0f ); // Also testing out of range!
	CellHandleContainer cells = grid.AcquireVisibleCells( position, 1 );
	ASSERT_EQ( 4, cells.Size() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_correct_range_if_top_right_corner_selected )
{
	GridSetup_9Cell();
	const Vector2 position = Vector2( 192.0f, 192.0f ); // Also testing out of range!
	CellHandleContainer cells = grid.AcquireVisibleCells( position, 1 );
	ASSERT_EQ( 4, cells.Size() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_correct_range_if_bottom_right_corner_selected )
{
	GridSetup_9Cell();
	const Vector2 position = Vector2( 192.0f, -192.0f ); // Also testing out of range!
	CellHandleContainer cells = grid.AcquireVisibleCells( position, 1 );
	ASSERT_EQ( 4, cells.Size() );
}

TEST_F( FoliageGridFixture, AcquireVisibleCells_return_correct_range_if_bottom_left_corner_selected )
{
	GridSetup_9Cell();
	const Vector2 position = Vector2( -192.0f, -192.0f ); // Also testing out of range!
	CellHandleContainer cells = grid.AcquireVisibleCells( position, 1 );
	ASSERT_EQ( 4, cells.Size() );
	EXPECT_EQ( Vector2( -96.0f, -96.0f ), cells[ 0 ]->GetWorldCoordinate() );
}

TEST_F( FoliageGridFixture, AcquireCell_return_correct_cell )
{
	GridSetup_9Cell();
	EXPECT_EQ( Vector2( -96.0f, 32.0f ), grid.AcquireCell( Vector2( -90.0f, 33.0f ) )->GetWorldCoordinate() );
	EXPECT_EQ( Vector2( -96.0f, -32.0f ), grid.AcquireCell( Vector2( -33.0f, 0.0f ) )->GetWorldCoordinate() );
	EXPECT_EQ( Vector2( -96.0f, -96.0f ), grid.AcquireCell( Vector2( -50.0f, -50.0f ) )->GetWorldCoordinate() );
}

TEST_F( FoliageGridFixture, AcquireCells_return_correct_cell_if_box_is_exact_size_of_one_cell )
{
	GridSetup_9Cell();
	Box box( Vector2( -96.0f, -96.0f ), Vector2( -32.0f, -32.0f ) );
	CellHandleContainer cells = grid.AcquireCells( box );
	ASSERT_EQ( 1, cells.Size() );
	EXPECT_EQ(  Vector2( -96.0f, -96.0f ), cells[ 0 ]->GetWorldCoordinate() );
}
