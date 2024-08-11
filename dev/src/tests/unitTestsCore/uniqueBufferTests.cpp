/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/uniqueBuffer.h"

namespace Red
{
	struct UniqueBufferFixture : testing::Test
	{
		UniqueBufferFixture()
			:	size( 123 ),
				memClass( MC_Default ),
				rawBuffer( RED_MEMORY_ALLOCATE( MemoryPool_Default, memClass, size ) ),
				buffer( rawBuffer, size, memClass )
		{}

		Uint32 size;
		Red::MemoryFramework::MemoryClass memClass;
		void * rawBuffer;
		UniqueBuffer buffer;
	};

	TEST( UniqueBuffer, Get_return_nullptr_by_default )
	{
		UniqueBuffer buffer;
		EXPECT_EQ( nullptr, buffer.Get() );
	}

	TEST( UniqueBuffer, GetSize_return_0_by_default )
	{
		UniqueBuffer buffer;
		EXPECT_EQ( 0, buffer.GetSize() );
	}

	TEST( UniqueBuffer, GetMemoryClass_return_datablob_by_default )
	{
		UniqueBuffer buffer;
		EXPECT_EQ( MC_DataBlob, buffer.GetMemoryClass() );
	}

	TEST_F( UniqueBufferFixture, Get_return_buffer )
	{
		EXPECT_EQ( rawBuffer, buffer.Get() );
	}

	TEST_F( UniqueBufferFixture, GetSize_return_correct_size )
	{
		EXPECT_EQ( size, buffer.GetSize() );
	}

	TEST_F( UniqueBufferFixture, GetMemoryClass_return_correct_mem_class )
	{
		EXPECT_EQ( memClass, buffer.GetMemoryClass() );
	}

	TEST_F( UniqueBufferFixture, Reset_delete_and_invalidate_buffer )
	{
		// Cannot test that the buffer is actually deleted. Hopefully I can plug a leak detector between each test
		buffer.Reset();
		EXPECT_EQ( nullptr, buffer.Get() );
		EXPECT_EQ( 0, buffer.GetSize() );
	}

	TEST_F( UniqueBufferFixture, Reset_delete_and_replace_with_new_buffer )
	{
		Uint32 newSize = 321;
		Red::MemoryFramework::MemoryClass newMemClass = MC_Debug;
		void * newRawBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, memClass, size );
		buffer.Reset( newRawBuffer, newSize, newMemClass );

		EXPECT_EQ( newRawBuffer, buffer.Get() );
		EXPECT_EQ( newSize, buffer.GetSize() );
		EXPECT_EQ( newMemClass, buffer.GetMemoryClass() );
	}

	TEST_F( UniqueBufferFixture, Release_return_buffer_and_invalidate_inner_one )
	{
		void * releasedBuffer = buffer.Release();
		EXPECT_EQ( rawBuffer, releasedBuffer );
		RED_MEMORY_FREE( MemoryPool_Default, memClass, releasedBuffer );
	}

	TEST( UniqueBuffer, operator_bool_return_false_by_default )
	{
		UniqueBuffer buffer;
		EXPECT_FALSE( buffer );
	}

	TEST_F( UniqueBufferFixture, operator_bool_return_true_if_buffer_valid )
	{
		EXPECT_TRUE( buffer );
	}

	TEST( UniqueBuffer, operator_not_return_true_by_default )
	{
		UniqueBuffer buffer;
		EXPECT_TRUE( !buffer );
	}

	TEST_F( UniqueBufferFixture, operator_not_return_false_if_buffer_valid )
	{
		EXPECT_FALSE( !buffer );
	}

	TEST_F( UniqueBufferFixture, move_constructor_move_content_to_new_buffer_and_invalidate_old_one )
	{
		UniqueBuffer newBuffer( std::move( buffer ) );
		EXPECT_FALSE( buffer );

		EXPECT_EQ( rawBuffer, newBuffer.Get() );
		EXPECT_EQ( size, newBuffer.GetSize() );
		EXPECT_EQ( memClass, newBuffer.GetMemoryClass() );
	}

	TEST_F( UniqueBufferFixture, move_operator_move_content_to_new_buffer_and_invalidate_old_one )
	{
		UniqueBuffer newBuffer;
		newBuffer = std::move( buffer );
		EXPECT_FALSE( buffer );

		EXPECT_EQ( rawBuffer, newBuffer.Get() );
		EXPECT_EQ( size, newBuffer.GetSize() );
		EXPECT_EQ( memClass, newBuffer.GetMemoryClass() );
	}
}