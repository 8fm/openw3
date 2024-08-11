#include "build.h"
#include "../../common/core/idAllocator.h"
#include "../../common/core/hashset.h"
#include "../../common/core/dynarray.h"

typedef IDAllocator< 666 > TTestAllocator;
typedef IDAllocator< 65535 > TTestAllocatorBig;

TEST( IDAllocator, Empty )
{
	TTestAllocator alloc;
	EXPECT_TRUE( alloc.GetNumAllocated() == 1 );
}

TEST( IDAllocator, LinearAlloc )
{
	TTestAllocator alloc;

	// linear allocation should be predictable
	Uint32 prevId = 0;
	for ( Uint32 i=1; i<alloc.GetCapacity(); ++i )
	{
		const Uint32 id = alloc.Alloc();
		EXPECT_EQ( prevId+1, id );
		EXPECT_EQ( i, id );
		prevId = id;
	}

	// no more allocation should be possible
	const Uint32 newID = alloc.Alloc();
	EXPECT_EQ( newID, 0 );
	EXPECT_EQ( alloc.IsFull(), true );
	EXPECT_EQ( alloc.GetCapacity(), alloc.GetNumAllocated() );
}

TEST( IDAllocator, SingleRelease )
{
	TTestAllocator alloc;

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();		
	}

	// should be full
	EXPECT_EQ( alloc.GetCapacity(), alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), true );

	// release single element
	const Uint32 idToFree = 123;
	alloc.Release( idToFree );

	// should no longer be full
	EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity()-1 );
	EXPECT_EQ( alloc.IsFull(), false );

	// allocate the ID back - we should be the same ID as the freed one
	const Uint32 newId = alloc.Alloc();
	EXPECT_EQ( newId, idToFree );
	EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() );
	EXPECT_EQ( alloc.IsFull(), true );
}

TEST( IDAllocator, MonotonicRelease )
{
	TTestAllocator alloc;

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();		
	}

	// should be full
	EXPECT_EQ( alloc.GetCapacity(), alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), true );

	// release elements with falling
	const Int32 firstIdToFree = alloc.GetCapacity() - 10;
	const Int32 lastIdToFree = 50;
	const Int32 freeStep = 67;
	Uint32 numReleased = 0;
	Uint32 lastFreedId = 0;
	for ( Int32 id = firstIdToFree; id >= lastIdToFree; id -= freeStep )
	{
		lastFreedId = (Uint32) id;
		numReleased += 1;

		alloc.Release( lastFreedId );
	}

	// should no longer be full
	EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() - numReleased );
	EXPECT_EQ( alloc.IsFull(), false );

	// when allocating the IDs back we should get the IDs in monotonic order
	for ( Uint32 i=0; i<numReleased; ++i )
	{
		const Uint32 expectedID = lastFreedId + (i * freeStep );
		const Uint32 newId = alloc.Alloc();
		EXPECT_EQ( expectedID, newId );
	}

	// we should be full again
	EXPECT_EQ( alloc.IsFull(), true );
}

TEST( IDAllocator, FullCycleForward )
{
	TTestAllocator alloc;

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();		
	}

	// should be full
	EXPECT_EQ( alloc.GetCapacity(), alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), true );

	// release all of the IDs - forward
	for ( Uint32 i=0; i<alloc.GetCapacity(); ++i )
		alloc.Release(i);

	// should be empty
	EXPECT_EQ( 1, alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), false );

	// reallocate IDs - should be monotonic
	Uint32 prevId = 0;
	while ( !alloc.IsFull() )
	{
		const Uint32 newId = alloc.Alloc();		
		EXPECT_EQ( prevId+1, newId );
		prevId = newId;
	}

	// we should be full again
	EXPECT_EQ( alloc.IsFull(), true );
}

TEST( IDAllocator, FullCycleBack )
{
	TTestAllocator alloc;

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();		
	}

	// should be full
	EXPECT_EQ( alloc.GetCapacity(), alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), true );

	// release all of the IDs - forward
	for ( Uint32 i=0; i<alloc.GetCapacity(); ++i )
		alloc.Release( (alloc.GetCapacity()-1) - i );

	// should be empty
	EXPECT_EQ( 1, alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), false );

	// reallocate IDs - should be monotonic
	Uint32 prevId = 0;
	while ( !alloc.IsFull() )
	{
		const Uint32 newId = alloc.Alloc();		
		EXPECT_EQ( prevId+1, newId );
		prevId = newId;
	}

	// we should be full again
	EXPECT_EQ( alloc.IsFull(), true );
}

TEST( IDAllocator, SwissCheese )
{
	TTestAllocator alloc;

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();		
	}

	// should be full
	EXPECT_EQ( alloc.GetCapacity(), alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), true );

	// free all even IDs
	for ( Uint32 i=0; i<alloc.GetCapacity(); i += 2 )
	{
		alloc.Release( i );
	}

	// should be exactly half full
	EXPECT_EQ( alloc.GetCapacity() - ((alloc.GetCapacity()-1)/2), alloc.GetNumAllocated() );
	EXPECT_EQ( alloc.IsFull(), false );

	// reallocate IDs - should be monotonic and in matching pattern
	Uint32 prevId = 0;
	while ( !alloc.IsFull() )
	{
		const Uint32 newId = alloc.Alloc();		
		EXPECT_EQ( prevId+2, newId );
		prevId = newId;
	}

	// we should be full again
	EXPECT_EQ( alloc.IsFull(), true );
}

TEST( IDAllocator, SwissCheese2 )
{
	TTestAllocatorBig alloc;

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();
	}

	// we should be full again
	EXPECT_EQ( alloc.IsFull(), true );
	EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() );

	// iterate
	for ( Uint32 i=0; i<10; ++i )
	{
		// free random IDs
		TDynArray< Uint32 > freedIndicesOrder;
		THashSet< Uint32 > freedIndices;
		for ( Uint32 i=0; i<(alloc.GetCapacity() / 2); ++i )
		{
			const Uint32 index = (rand() + (rand() * RAND_MAX)) % alloc.GetCapacity();

			if ( !freedIndices.Exist( index ) && index )
			{
				freedIndicesOrder.PushBack( index );
				freedIndices.Insert( index );
				alloc.Release( index );

				EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() - freedIndicesOrder.Size() );				
			}
		}

		// resort freed indices - we should allocate them back in that order
		::Sort( freedIndicesOrder.Begin(), freedIndicesOrder.End() );

		// reallocate them, we should get exactly the same indices
		for ( Uint32 i=0; i<freedIndicesOrder.Size(); ++i )
		{
			const Uint32 allocatedIndex = alloc.Alloc();
			const Uint32 allocatedIndexTest = freedIndicesOrder[i];
			EXPECT_EQ( allocatedIndex, allocatedIndexTest );
		}

		// we need to be full again
		EXPECT_EQ( alloc.IsFull(), true );
		EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() );
	}
}

#pragma optimize("",off)

TEST( IDAllocator, SwissCheese2Dynamic )
{
	IDAllocatorDynamic alloc;
	alloc.Resize( 1024*128 );

	// allocate all IDs
	while ( !alloc.IsFull() )
	{
		alloc.Alloc();
	}

	// we should be full again
	EXPECT_EQ( alloc.IsFull(), true );
	EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() );

	// iterate
	for ( Uint32 i=0; i<10; ++i )
	{
		// free random IDs
		TDynArray< Uint32 > freedIndicesOrder;
		THashSet< Uint32 > freedIndices;
		for ( Uint32 i=0; i<(alloc.GetCapacity() / 2); ++i )
		{
			const Uint32 index = (rand() + (rand() * RAND_MAX)) % alloc.GetCapacity();

			if ( !freedIndices.Exist( index ) && index )
			{
				freedIndicesOrder.PushBack( index );
				freedIndices.Insert( index );
				alloc.Release( index );

				EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() - freedIndicesOrder.Size() );				
			}
		}

		// resort freed indices - we should allocate them back in that order
		::Sort( freedIndicesOrder.Begin(), freedIndicesOrder.End() );

		// reallocate them, we should get exactly the same indices
		for ( Uint32 i=0; i<freedIndicesOrder.Size(); ++i )
		{
			const Uint32 allocatedIndex = alloc.Alloc();
			const Uint32 allocatedIndexTest = freedIndicesOrder[i];
			EXPECT_EQ( allocatedIndex, allocatedIndexTest );
		}

		// we need to be full again
		EXPECT_EQ( alloc.IsFull(), true );
		EXPECT_EQ( alloc.GetNumAllocated(), alloc.GetCapacity() );
	}
}
