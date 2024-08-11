/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "../../common/redNetwork/fixedPoolList.h"

using namespace Red::Network;
using Red::System::Uint32;
/*
class FooTest : public testing::Test
{
protected:
	virtual void SetUp()
	{
	}
	
	virtual void TearDown() 
	{
	}
};

TEST_F(FooTest, Bar) { ... }
TEST_F(FooTest, Baz) { ... }
*/

const Uint32 size = 8;
typedef FixedPoolList< Uint32, size > TestListU32;

void FillList( TestListU32& container )
{
	// Fill cycle
	for( Uint32 i = 0; i < size; ++i )
	{
		EXPECT_EQ( i, container.GetSize() );
		EXPECT_EQ( size - i, container.GetSpaceRemaining() );

		Uint32* newValue = container.Add();

		ASSERT_NE( nullptr, newValue );

		*newValue = i;
	}
}

TEST( Network, FixedPoolList_AddRemove )
{
	TestListU32 container;

	EXPECT_EQ( nullptr, container.Front() );
	EXPECT_EQ( nullptr, container.Back() );

	FillList( container );

	EXPECT_EQ( size, container.GetSize() );

	// Check to make sure it correctly refuses to add any more items
	Uint32* overflowValue = container.Add();
	EXPECT_EQ( nullptr, overflowValue );

	// Test Cycle
	TestListU32::Iterator iter;
	Uint32 expectedValue = 0;
	for( iter = container.Begin(); iter != container.End(); ++iter )
	{
		EXPECT_EQ( expectedValue, *iter );

		++expectedValue;
	}

	for( Uint32 i = 0; i < size; ++i )
	{
		Uint32* currentValue = container.Front();
		ASSERT_NE( nullptr, currentValue );
		EXPECT_EQ( i, *currentValue );
		EXPECT_EQ( i, container.GetSpaceRemaining() );

		container.PopFront();
	}

	EXPECT_EQ( 0, container.GetSize() );
	EXPECT_EQ( size, container.GetSpaceRemaining() );

	EXPECT_EQ( nullptr, container.Front() );
	EXPECT_EQ( nullptr, container.Back() );
}

TEST( Network, FixedPoolList_RemoveDuringLoop )
{
	TestListU32 container;
	FillList( container );

	// Remove from the middle
	{
		TestListU32::Iterator previous;
		TestListU32::Iterator current = container.Begin();

		for( Uint32 i = 0; i < size / 2; ++i )
		{
			previous = current;

			EXPECT_EQ( *current, *previous );
			EXPECT_EQ( current, previous );

			++current;

			EXPECT_NE( *current, *previous );
			EXPECT_NE( current, previous );
		}

		container.Remove( current );

		EXPECT_EQ( *current, *previous );
		EXPECT_EQ( current, previous );
		EXPECT_EQ( size - 1, container.GetSize() );
		EXPECT_EQ( 1, container.GetSpaceRemaining() );
	}

	// Remove from the front
	{
		TestListU32::Iterator front = container.Begin();

		container.Remove( front );

		ASSERT_NE( container.Front(), nullptr );

		// the iterator will now point to an invalid position
		EXPECT_EQ( container.End(), front );

		++front;

		EXPECT_EQ( container.Begin(), front );
		EXPECT_EQ( *container.Front(), *front );

		EXPECT_EQ( size - 2, container.GetSize() );
		EXPECT_EQ( 2, container.GetSpaceRemaining() );
	}

	// Remove from the front
	{
		TestListU32::Iterator back = container.End();
		--back;

		TestListU32::Iterator penultimate = back;

		EXPECT_EQ( *container.Back(), *back );
		EXPECT_EQ( *container.Back(), *penultimate );
		EXPECT_EQ( back, penultimate );
		EXPECT_EQ( *back, *penultimate );

		--penultimate;
		EXPECT_NE( back, penultimate );
		EXPECT_NE( *back, *penultimate );

		container.Remove( back );

		EXPECT_EQ( *container.Back(), *back );
		EXPECT_EQ( back, penultimate );
		EXPECT_EQ( *back, *penultimate );

		++penultimate;
		EXPECT_EQ( container.End(), penultimate );

		EXPECT_EQ( size - 3, container.GetSize() );
		EXPECT_EQ( 3, container.GetSpaceRemaining() );
	}
}
