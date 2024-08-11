/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/bundleDataCache.h"
#include "../../common/core/math.h"


struct BundleDataCacheFixture : ::testing::Test
{
	BundleDataCacheFixture()
		:	bufferSize( 0 ),
			bufferCount( 0 )
	{}

	void Setup( Uint32 size, Uint32 count )
	{
		bufferSize = size;
		bufferCount = count;
		bundleDataCache.Initialize( bufferSize, bufferCount );
	}

	CBundleDataCache bundleDataCache;
	Uint32 bufferSize;
	Uint32 bufferCount;
};

TEST_F( BundleDataCacheFixture, AcquireBuffer_return_valid_buffer_if_size_less_than_pool_entry )
{
	Setup( 1024, 1 );
	EXPECT_TRUE( bundleDataCache.AcquireBuffer( 256 ) );
	EXPECT_EQ( 1, bundleDataCache.GetMetrics().acquireBufferCount );
}

TEST_F( BundleDataCacheFixture, Acquire_return_valid_buffer_if_size_is_same_than_pool_entry )
{
	Setup( 1024, 1 );
	EXPECT_TRUE( bundleDataCache.AcquireBuffer( bufferSize ) );
	EXPECT_EQ( 1, bundleDataCache.GetMetrics().acquireBufferCount );
}

TEST_F( BundleDataCacheFixture, Acquire_return_valid_buffer_if_size_is_bigger_than_pool_entry )
{
	Setup( 1024, 1 );
	EXPECT_TRUE( bundleDataCache.AcquireBuffer(  bufferSize * 2 ) );
	EXPECT_EQ( 1, bundleDataCache.GetMetrics().acquireBufferFailureSizeTooBig );
}

TEST_F( BundleDataCacheFixture, Acquire_return_valid_buffer_if_no_buffer_available )
{
	Setup( 1024, 1 );
	BundleDataHandle handle1 = bundleDataCache.AcquireBuffer(  256 );
	BundleDataHandle handle2 = bundleDataCache.AcquireBuffer(  256 );
	EXPECT_TRUE( handle1 );
	EXPECT_TRUE( handle2 );
	EXPECT_EQ( 1, bundleDataCache.GetMetrics().acquireBufferFailureNoBufferAvailable );
}
