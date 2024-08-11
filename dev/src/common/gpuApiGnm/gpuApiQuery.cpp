/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"
#include "../gpuApiUtils/gpuApiMemory.h"

namespace GpuApi
{

	void InitQueries( Bool )
	{
#ifndef RED_FINAL_BUILD
		GpuApi::SDeviceData& dd = GetDeviceData();
		dd.m_frameStartQuery = CreateQuery(QT_Timestamp);
		dd.m_frameEndQuery = CreateQuery(QT_Timestamp);
#endif
	}

	void ShutQueries( Bool )
	{
#ifndef RED_FINAL_BUILD
		GpuApi::SDeviceData& dd = GetDeviceData();
		SafeRelease(dd.m_frameStartQuery);
		SafeRelease(dd.m_frameEndQuery);
#endif
	}

	void AddRef( const QueryRef &query )
	{
		GPUAPI_ASSERT( GetDeviceData().m_Queries.IsInUse( query ) );
		GetDeviceData().m_Queries.IncRefCount( query );
	}

	Int32 Release( const QueryRef &query )
	{
		GPUAPI_ASSERT( query );

		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Queries.IsInUse(query) );
		GPUAPI_ASSERT( dd.m_Queries.GetRefCount(query) >= 1 );
		Int32 refCount = dd.m_Queries.DecRefCount( query );
		if ( 0 == refCount )
		{
			QueueForDestroy(query);
		}
		return refCount;
	}

	QueryRef CreateQuery( eQueryType queryType )
	{
		if ( !IsInit() )
		{
			GPUAPI_HALT( "Not init during attempt to create query" );
			return QueryRef::Null();
		}

		// If we don't have any slots left, then leave
		SDeviceData &dd = GetDeviceData();
		if ( !dd.m_Queries.IsCapableToCreate( 1 ) )
		{
			GPUAPI_LOG_WARNING( TXT( "Failed to create d3d query." ) );
			return QueryRef::Null();
		}

		sce::Gnm::OcclusionQueryResults* occlusionQuery = nullptr;
		Uint64* timestamp = nullptr;

		switch ( queryType )
		{
		case QT_Occlusion:
			{
				occlusionQuery = (sce::Gnm::OcclusionQueryResults*)GPU_API_ALLOCATE (GpuMemoryPool_SmallOnion, MC_Query, sizeof(sce::Gnm::OcclusionQueryResults), sce::Gnm::kEmbeddedDataAlignment16);
				occlusionQuery->reset();
			}
			break;
		case QT_Timestamp:
			{
				timestamp = (Uint64*)GPU_API_ALLOCATE (GpuMemoryPool_SmallOnion, MC_Query, sizeof(Uint64), sce::Gnm::kEmbeddedDataAlignment16);
			}
			break;
		case QT_TimestampDisjoint:	
			{
				// No need for any kind of work, the frequency is constant and there is no disjoint case
			}
			break;
		default:	GPUAPI_HALT( "Not able to create this type of query! (%d)" ); return QueryRef::Null();
		}

		// Create GpuApi query
		Uint32 newQueryId = dd.m_Queries.Create( 1 );
		if ( !newQueryId )
		{
			GPU_API_FREE (GpuMemoryPool_SmallOnion, MC_Query, occlusionQuery);
			GPUAPI_HALT( "Failed to create gpuapi query despite it was tested as possible" );
			return QueryRef::Null();
		}

		// Initialize new query
		SQueryData &data = dd.m_Queries.Data( newQueryId );
		data.m_pQuery = occlusionQuery;
		data.m_timeStamp = timestamp;
		data.m_queryType = queryType;

		// Finalize
		GPUAPI_ASSERT( newQueryId && dd.m_Queries.IsInUse( newQueryId ) );
		return QueryRef( newQueryId );
	}


	Bool CreateQueries( Uint32 numQueries, QueryRef* outQueries, eQueryType queryType )
	{
		if ( !IsInit() || 0==numQueries || outQueries == nullptr )
		{
			return false;
		}

		// Test if creation of given number of queries is possible
		SDeviceData &dd = GetDeviceData();
		if ( !dd.m_Queries.IsCapableToCreate( numQueries ) )
		{
			return false;
		}

		// Create queries
		Uint32 numCreated = 0;
		while ( numCreated < numQueries )
		{
			GPUAPI_ASSERT( outQueries && !outQueries[numCreated], TXT( "Invalid input" ) );
			outQueries[numCreated] = CreateQuery( queryType );
			if ( outQueries[numCreated] )
			{
				++numCreated;
			}
			else
			{
				break;
			}			
		}

		// If we didn't manage to create all needed queries, then remove whet we managed to create
		// TODO: this is because this function itself is not atomic. in practice it should be very rare case so I leave it this way.
		if ( numCreated != numQueries )
		{
			for ( Uint32 i=0; i<numCreated; ++i )
			{
				GPUAPI_ASSERT( outQueries[i] );
				SafeRelease( outQueries[i] );
				GPUAPI_ASSERT( !outQueries[i] );
			}
			return false;
		}

		// Success :)
		return true;
	}

	Bool BeginQuery( const QueryRef &query )
	{
		if ( !query )
		{
			return false;
		}
		
		// Get data
		GPUAPI_ASSERT( IsInit() );
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		const SQueryData &data = GetDeviceData().m_Queries.Data( query );
		
		// Ignore query if calling Begin is not valid for it
		if ( GpuApi::QT_CommandsFinished == data.m_queryType || GpuApi::QT_Timestamp == data.m_queryType )
		{
			return true;
		}

		if ( !data.m_pQuery )
		{
			return false;
		}

		// Begin query
		data.m_pQuery->reset();
		gfxc.writeOcclusionQuery(sce::Gnm::kOcclusionQueryOpBeginWithoutClear, data.m_pQuery);
		gfxc.setDbCountControl( sce::Gnm::kDbCountControlPerfectZPassCountsEnable, 0 );

		// Finish
		return true;
	}

	Bool EndQuery( const QueryRef &query )
	{
		if ( !query )
		{
			return false;
		}

		// Get data
		GPUAPI_ASSERT( IsInit() );
		SSwapChainData& scd = GetSwapChainData();
		sce::Gnmx::GfxContext& gfxc = scd.backBuffer->context;

		SQueryData &data = GetDeviceData().m_Queries.Data( query );

		// End query
		switch ( data.m_queryType )
		{
		case QT_Occlusion:
			{
				if ( !data.m_pQuery )
				{
					return false;
				}

				gfxc.writeOcclusionQuery(sce::Gnm::kOcclusionQueryOpEnd, data.m_pQuery);
				gfxc.setDbCountControl( sce::Gnm::kDbCountControlPerfectZPassCountsDisable, 0 );
			}
			break;
		case QT_Timestamp:
			{
				*(data.m_timeStamp) = 0; // make sure that this is 0 so we can detect when it gets written with valid data
				gfxc.writeAtEndOfPipe( sce::Gnm::kEopFlushCbDbCaches, sce::Gnm::kEventWriteDestMemory, data.m_timeStamp, sce::Gnm::kEventWriteSourceGpuCoreClockCounter, 0, sce::Gnm::kCacheActionNone, sce::Gnm::kCachePolicyLru);
			}
			break;
		case QT_TimestampDisjoint:	
			{
				// No need for any kind of work, the frequency is constant and there is no disjoint case
			}
			break;
		default:	GPUAPI_HALT( "Not able to create this type of query! (%d)" ); return QueryRef::Null();
		}

		// Finish
		return true;
	}


	eQueryResult			GetQueryResult( const QueryRef &query, Uint64& outResult, Bool forceImmediate )
	{
		if ( !query )
		{
			return QR_Error;
		}

		// Get data
		GPUAPI_ASSERT( IsInit() );
		const SQueryData &data = GetDeviceData().m_Queries.Data( query );

		GPUAPI_ASSERT( data.m_queryType == QT_Occlusion || data.m_queryType == QT_Timestamp );

		// End query
		switch ( data.m_queryType )
		{
		case QT_Occlusion:
			{
				if (!data.m_pQuery)
				{
					return QR_Error;
				}

				if (data.m_pQuery->isReady())
				{
					outResult = data.m_pQuery->getZPassCount();
					return QR_Success;
				}
				else
				{
					GPUAPI_ASSERT (!forceImmediate);	// can't handle forceImmediate!
					return QR_Pending;
				}
			}
			break;
		case QT_Timestamp:
			{
				if ( *(data.m_timeStamp) == 0)
				{
					return QR_Pending;
				}
				else
				{
					outResult = *(data.m_timeStamp);
					return QR_Success;
				}
			}
			break;
		default:	GPUAPI_HALT( "Not able to create this type of query! (%d)" ); return QR_Error;
		}
	}

	eQueryResult			GetQueryResult( const QueryRef &query, SPipelineStatistics& outStats, Bool forceImmediate )
	{
		if ( !query )
		{
			return QR_Error;
		}

		// Get data
		GPUAPI_ASSERT( IsInit() );

		// Finish (should not be reached)
		GPUAPI_HALT( "This code path should not be reached" );
		return QR_Error;
	}

	eQueryResult			GetQueryResult( const QueryRef &query, Uint64& frequency, Bool& disjoint, Bool forceImmediate )
	{
		if ( !query )
		{
			return QR_Error;
		}

		frequency = SCE_GNM_GPU_CORE_CLOCK_FREQUENCY;
		disjoint = false;

		return QR_Success;
	}

	Uint8 GetNumberOfDelayedFrames()
	{
		return 5;
	}


	void Destroy(const QueryRef& buffer)
	{
		SDeviceData &dd = GetDeviceData();
		SQueryData &data = dd.m_Queries.Data( buffer );

		GPU_API_FREE (GpuMemoryPool_SmallOnion, MC_Query, data.m_pQuery);

		// Destroy shit
		dd.m_Queries.Destroy( buffer );
	}
}