#include "build.h"

#include "../../common/core/taskDeque.h"
#include "../../common/core/taskSchedNode.h"
/*

TEST( TaskDequeTests, Empty )
{
	CTaskSchedNode* pn1;
	TTaskDequeNTS d;
		
	// Empty deque
	EXPECT_TRUE( d.Size() == 0 );
	EXPECT_TRUE( ! d.PopFront( pn1 ) && d.Size() == 0 );
	EXPECT_TRUE( ! d.PopBack( pn1 ) && d.Size() == 0 );
}

TEST( TaskDequeTests,OneNode_PushBack1 )
{
	CTaskSchedNode n1;
	CTaskSchedNode* pn1;
	TTaskDequeNTS d;

	// Push back 1
	d.PushBack( &n1 );
	EXPECT_TRUE( d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushBack( &n1 );
	EXPECT_TRUE( d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );
}

TEST( TaskDequeTests,OneNode_PushFront1 )
{
	CTaskSchedNode n1;
	CTaskSchedNode* pn1;
	TTaskDequeNTS d;

	// Push front 1
	d.PushFront( &n1 );
	EXPECT_TRUE( d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushFront( &n1 );
	EXPECT_TRUE( d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );
}

TEST( TaskDequeTests,TwoNodes_PushBack2 )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	TTaskDequeNTS d;

	// Push back 2
	d.PushBack( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );
}

TEST( TaskDequeTests,TwoNodes_PushFront2 )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	TTaskDequeNTS d;

	// Push front 2
	d.PushFront( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );
}

TEST( TaskDequeTests,TwoNodes_PushBack1Front1 )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	TTaskDequeNTS d;

	// Push back 1, push front 1
	d.PushBack( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushFront( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );
}

TEST( TaskDequeTests,TwoNodes_PushFront1Back1 )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	TTaskDequeNTS d;

	// Push front 1, push back 1
	d.PushFront( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushBack( &n2 );
	EXPECT_TRUE( d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );
}

TEST( TaskDequeTests,ThreeNodes_PushBack3 )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode n3;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	CTaskSchedNode* pn3;
	TTaskDequeNTS d;

	// Push back 3
	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 0 );

	d.PushBack( &n1 );
	d.PushBack( &n2 );
	d.PushBack( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 0 );
}

TEST( TaskDequeTests,ThreeNodes_PushFront3 )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode n3;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	CTaskSchedNode* pn3;
	TTaskDequeNTS d;

	// Push front 3
	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );

	d.PushFront( &n1 );
	d.PushFront( &n2 );
	d.PushFront( &n3 );
	EXPECT_TRUE( d.Size() == 3 );
	EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 2 );
	EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
	EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );
}

TEST( TaskDequeTests,ThreeNodes_Sequences )
{
	CTaskSchedNode n1;
	CTaskSchedNode n2;
	CTaskSchedNode n3;
	CTaskSchedNode* pn1;
	CTaskSchedNode* pn2;
	CTaskSchedNode* pn3;
	TTaskDequeNTS d;

	// Sequences
	for ( Uint32 i = 0; i < 3; ++i )
	{
		d.PushFront( &n1 );
		d.PushFront( &n2 );
		d.PushFront( &n3 );
		EXPECT_TRUE( d.Size() == 3 );
		EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 2 );
		EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
		EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 0 );
	}

	for ( Uint32 i = 0; i < 3; ++i )
	{
		d.PushFront( &n1 );
		d.PushFront( &n2 );
		d.PushFront( &n3 );
		EXPECT_TRUE( d.Size() == 3 );
		EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 2 );
		EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
		EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 0 );
	}

	for ( Uint32 i = 0; i < 3; ++i )
	{
		d.PushBack( &n1 );
		d.PushBack( &n2 );
		d.PushBack( &n3 );
		EXPECT_TRUE( d.Size() == 3 );
		EXPECT_TRUE( d.PopFront( pn1 ) && pn1 == &n1 && d.Size() == 2 );
		EXPECT_TRUE( d.PopFront( pn2 ) && pn2 == &n2 && d.Size() == 1 );
		EXPECT_TRUE( d.PopFront( pn3 ) && pn3 == &n3 && d.Size() == 0 );
	}

	for ( Uint32 i = 0; i < 3; ++i )
	{
		d.PushBack( &n1 );
		d.PushBack( &n2 );
		d.PushBack( &n3 );
		EXPECT_TRUE( d.Size() == 3 );
		EXPECT_TRUE( d.PopBack( pn3 ) && pn3 == &n3 && d.Size() == 2 );
		EXPECT_TRUE( d.PopBack( pn2 ) && pn2 == &n2 && d.Size() == 1 );
		EXPECT_TRUE( d.PopBack( pn1 ) && pn1 == &n1 && d.Size() == 0 );
	}
}	
*/