#include "fenceManagerTest.h"
#ifdef RED_PLATFORM_ORBIS
#include "../../common/gpuApiGnm/gpuApi.h"
#endif


Bool CFenceManagerTest::Execute( CTestUtils& context )
{
	TDynArray< Bool > stageResults;

#ifdef RED_PLATFORM_ORBIS

	// TODO : Write tests for multiple contexts

	GpuApi::CFenceManager< 1, 8, 8 > fencemgr;

	Uint32 valuesToWrite[ 1024 ];
	Uint32* locationsToWrite[ 1024 ];
	Uint64 fences[ 1024 ];

	// 510 fences will fill both buckets (255 each, since counter value 0 is not used).
	LOG_GAT( TXT("Getting 510 fences") );
	for ( Uint32 i = 0; i < 510; ++i )
	{
		fences[ i ] = fencemgr.GetFence( 0, locationsToWrite[ i ], valuesToWrite[ i ] );
	}

	LOG_GAT( TXT("Checking... all should be pending") );
	{
		Bool pass = true;
		for ( Uint32 i = 0; i < 510; ++i )
		{
			if ( !fencemgr.IsFencePending( fences[ i ] ) )
			{
				ERR_GAT( TXT("Fence should be pending: %")RED_PRIWu64, fences[ i ] );
				pass = false;
				break;
			}
		}
		stageResults.PushBack( pass );
	}

	// Set value for first bunch of fences. These are all in the first bucket.
	LOG_GAT( TXT("Writing fence values 0-129") );
	for ( Uint32 i = 0; i < 130; ++i )
	{
		*locationsToWrite[ i ] = valuesToWrite[ i ];
	}

	LOG_GAT( TXT("Checking... 0-129 should be finished") );
	{
		Bool pass = true;
		for ( Uint32 i = 0; i < 510; ++i )
		{
			const Bool shouldBePending = i >= 130;
			if ( shouldBePending != fencemgr.IsFencePending( fences[ i ] ) )
			{
				ERR_GAT( TXT("Fence state wrong [130]: %")RED_PRIWu64, fences[ i ] );
				pass = false;
				break;
			}
		}
		stageResults.PushBack( pass );
	}

	// Progress through more fences. This batch starts into the second bucket.
	LOG_GAT( TXT("Writing fence values 130-299") );
	for ( Uint32 i = 130; i < 300; ++i )
	{
		*locationsToWrite[ i ] = valuesToWrite[ i ];
	}

	LOG_GAT( TXT("Checking... 0-299 should be finished") );
	{
		Bool pass = true;
		for ( Uint32 i = 0; i < 510; ++i )
		{
			const Bool shouldBePending = i >= 300;
			if ( shouldBePending != fencemgr.IsFencePending( fences[ i ] ) )
			{
				ERR_GAT( TXT("Fence state wrong [300]: %")RED_PRIWu64, fences[ i ] );
				pass = false;
				break;
			}
		}
		stageResults.PushBack( pass );
	}

	// And progress through the rest of them.
	LOG_GAT( TXT("Writing fence values 300-509") );
	for ( Uint32 i = 300; i < 510; ++i )
	{
		*locationsToWrite[ i ] = valuesToWrite[ i ];
	}

	LOG_GAT( TXT("Checking... all should be finished") );
	{
		Bool pass = true;
		for ( Uint32 i = 0; i < 510; ++i )
		{
			if ( fencemgr.IsFencePending( fences[ i ] ) )
			{
				ERR_GAT( TXT("Fence should not be pending: %")RED_PRIWu64, fences[ i ] );
				pass = false;
				break;
			}
		}
		stageResults.PushBack( pass );
	}

	// Get more fences. This will get from buckets 2 and 3, and the first bunch of fences will be expired.
	LOG_GAT( TXT("Getting 510 more fences") );
	for ( Uint32 i = 510; i < 1020; ++i )
	{
		fences[ i ] = fencemgr.GetFence( 0, locationsToWrite[ i ], valuesToWrite[ i ] );
	}

	LOG_GAT( TXT("Checking... 0-509 should be old. Others pending.") );
	{
		Bool pass = true;
		for ( Uint32 i = 0; i < 1020; ++i )
		{
			const Bool shouldBePending = i >= 510;
			const Bool shouldBeOld = i < 510;
			if ( shouldBeOld != fencemgr.IsFenceExpired( fences[ i ] ) )
			{
				ERR_GAT( TXT("Fence should be expired: %")RED_PRIWu64, fences[ i ] );
				pass = false;
				break;
			}
			if ( !shouldBeOld && shouldBePending != fencemgr.IsFencePending( fences[ i ] ) )
			{
				ERR_GAT( TXT("Fence should be pending: %")RED_PRIWu64, fences[ i ] );
				pass = false;
				break;
			}
		}
		stageResults.PushBack( pass );
	}

	// Check that the first fence is not pending. We already know that it's expired, from above.
	LOG_GAT( TXT("Checking if old is pending (will cause warning to be logged, that's okay)") );
	{
		Bool pass = true;
		if ( fencemgr.IsFencePending( fences[ 0 ] ) )
		{
			ERR_GAT( TXT("Old fence should not be pending") );
			pass = false;
		}
		stageResults.PushBack( pass );
	}

	// Get a lot more fences. 65280 is the maximum number of fences we can get from this fencemgr (each bucket provides
	// 255, and there are 256 buckets total).
	LOG_GAT( TXT("Getting fences 1020-65279") );
	{
		Bool pass = true;
		for ( Uint32 i = 1020; i < 65280; ++i )
		{
			Uint32* ptr;
			Uint32 val;

			if ( !fencemgr.CanGetFence( 0 ) )
			{
				ERR_GAT( TXT("Should be able to get a fence in iteration %u"), i );
				pass = false;
			}

			Uint64 fence = fencemgr.GetFence( 0, ptr, val );
		}
		stageResults.PushBack( pass );
	}

	// We shouldn't be able to get any more fences. If we were to do a GetFence at this point, we would get a fatal assert.
	{
		Bool pass = true;
		if ( fencemgr.CanGetFence( 0 ) )
		{
			ERR_GAT( TXT("Shouldn't be able to get a fence. Should be completely run out now") );
			pass = false;
		}
		stageResults.PushBack( pass );
	}


#else // RED_PLATFORM_ORBIS

	// Nothing to test for non-ps4, so just push the appropriate number of passes.
	for ( Uint32 i = 0; i < 8; ++i )
	{
		stageResults.PushBack( true );
	}

#endif // !RED_PLATFORM_ORBIS


	context.GetEffect( ET_ColoredTriangleGrid )->SetShaders();

	RED_FATAL_ASSERT( stageResults.Size() <= 64, "Too many stages in this test (%u)! Either simplify it, or maybe make coloredTriangleGrid use a larger grid", stageResults.Size() );
	CBColoredTriangleGrid colorsCB;
	for ( Uint32 i = 0; i < stageResults.Size(); ++i )
	{
		colorsCB.m_colors[ i ] = stageResults[ i ] ? RedVector4( 0, 1, 0, 1 ) : RedVector4( 1, 0, 0, 1 );
	}

	GpuApi::BufferInitData bufInitData;
	bufInitData.m_buffer = &colorsCB;
	bufInitData.m_elementCount = 1;
	GpuApi::BufferRef constantBufferRef = GpuApi::CreateBuffer(sizeof(colorsCB), GpuApi::BCC_Constant, GpuApi::BUT_Immutable, 0, &bufInitData );
	GpuApi::BindConstantBuffer(0, constantBufferRef, GpuApi::VertexShader);

	GpuApi::SetDrawContext( GpuApi::DRAWCONTEXT_SimpleNoCull, 1 );

	GpuApi::DrawInstancedPrimitiveNoBuffers( GpuApi::PRIMTYPE_TriangleList, 3, stageResults.Size() );

	GpuApi::SafeRelease( constantBufferRef );


	return true;
}
