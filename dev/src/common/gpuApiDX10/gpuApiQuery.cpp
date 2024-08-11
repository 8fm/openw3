/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "gpuApi.h"


namespace GpuApi
{

	void InitQueries( Bool )
	{
		GpuApi::SDeviceData& dd = GetDeviceData();
#ifndef RED_FINAL_BUILD
		dd.m_frameStartQuery = CreateQuery(QT_Timestamp);
		dd.m_frameEndQuery = CreateQuery(QT_Timestamp);
#endif
	}

	void ShutQueries( Bool )
	{
		GpuApi::SDeviceData& dd = GetDeviceData();
#ifndef RED_FINAL_BUILD
		GpuApi::SafeRelease( dd.m_frameStartQuery );
		GpuApi::SafeRelease( dd.m_frameEndQuery );

		GpuApi::SafeRelease( dd.m_frameQueryDisjoint );
		GpuApi::SafeRelease( dd.m_frameQueryDisjointPending );
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
			// No manual memory allocations for queries, so we can Destroy directly, without QueueForDestroy.
			Destroy( query );
		}
		return refCount;
	}

	void Destroy( const QueryRef &query )
	{
		SDeviceData &dd = GetDeviceData();
		GPUAPI_ASSERT( dd.m_Queries.IsInUse(query) );

		SQueryData &data = dd.m_Queries.Data( query );

		// Release resources
		GPUAPI_ASSERT( NULL != data.m_pQuery );
		SAFE_RELEASE( data.m_pQuery );

		// Destroy shit
		dd.m_Queries.Destroy( query );
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
			GPUAPI_LOG_WARNING( TXT( "Failed to allocate query structure." ) );
			return QueryRef::Null();
		}

		// Create d3d query
		ID3D11Query *d3dQuery = NULL;
		D3D11_QUERY_DESC qDesc;

		switch ( queryType )
		{
		case QT_Occlusion:			qDesc.Query = D3D11_QUERY_OCCLUSION; break;
		case QT_PipelineStats:		qDesc.Query = D3D11_QUERY_PIPELINE_STATISTICS; break;
		case QT_CommandsFinished:	qDesc.Query = D3D11_QUERY_EVENT; break;
		case QT_Timestamp:			qDesc.Query = D3D11_QUERY_TIMESTAMP; break;
		case QT_TimestampDisjoint:	qDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT; break;
		default:	GPUAPI_HALT( "invalid or not available" ); return QueryRef::Null();
		}
		
		qDesc.MiscFlags = 0;
		HRESULT hr = GetDevice()->CreateQuery( &qDesc, &d3dQuery );
		if ( !SUCCEEDED( hr ) )
		{
			GPUAPI_LOG_WARNING( TXT("Failed to create d3d query: 0x%08x"), (Uint32)hr );
			return QueryRef::Null();
		}
		GPUAPI_ASSERT( NULL != d3dQuery );

		// Create GpuApi query
		Uint32 newQueryId = dd.m_Queries.Create( 1 );
		if ( !newQueryId )
		{
			SAFE_RELEASE( d3dQuery );
			GPUAPI_HALT( "Failed to create gpuapi query despite it was tested as possible" );
			return QueryRef::Null();
		}

		// Initialize new query
		SQueryData &data = dd.m_Queries.Data( newQueryId );
		data.m_pQuery = d3dQuery;
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
		const SQueryData &data = GetDeviceData().m_Queries.Data( query );
		if ( !data.m_pQuery )
		{
			return false;
		}

		// Ignore query if calling Begin is not valid for it
		if ( GpuApi::QT_CommandsFinished == data.m_queryType || GpuApi::QT_Timestamp == data.m_queryType )
		{
			return true;
		}
		
		// Begin query
		GetDeviceContext()->Begin(data.m_pQuery);

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
		const SQueryData &data = GetDeviceData().m_Queries.Data( query );
		if ( !data.m_pQuery )
		{
			return false;
		}

		// End query
		GetDeviceContext()->End(data.m_pQuery);

		// Finish
		return true;
	}

	eQueryResult GetQueryResult( const QueryRef &query, Uint64& outResult, Bool forceImmediate )
	{
		if ( !query )
		{
			return QR_Error;
		}

		// Get data
		GPUAPI_ASSERT( IsInit() );
		const SQueryData &data = GetDeviceData().m_Queries.Data( query );
		if ( !data.m_pQuery )
		{
			return QR_Error;
		}

		GPUAPI_ASSERT( data.m_queryType == QT_Occlusion || data.m_queryType == QT_Timestamp );

		// Get data
		Uint64 resultValue = 0;
		HRESULT result;
		do 
		{
			result = GetDeviceContext()->GetData( data.m_pQuery, &resultValue, sizeof(resultValue), (!forceImmediate ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) );
		}
		while ( forceImmediate && S_FALSE == result );

		// Do result interpretation
		switch ( result )
		{
		case S_OK:
			outResult = resultValue;
			return QR_Success;

		case S_FALSE:
			return QR_Pending;

		default:
			return QR_Error;
		}
	}

	eQueryResult GetQueryResult( const QueryRef &query, SPipelineStatistics& outStats, Bool forceImmediate )
	{
		if ( !query )
		{
			return QR_Error;
		}

		// Get data
		GPUAPI_ASSERT( IsInit() );
		const SQueryData &data = GetDeviceData().m_Queries.Data( query );
		if ( !data.m_pQuery )
		{
			return QR_Error;
		}

		GPUAPI_ASSERT( data.m_queryType == QT_PipelineStats );

		// Get data
		D3D11_QUERY_DATA_PIPELINE_STATISTICS resultStats;
		HRESULT result;
		do 
		{
			result = GetDeviceContext()->GetData( data.m_pQuery, &resultStats, sizeof(resultStats), (!forceImmediate ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) );
		}
		while ( forceImmediate && S_FALSE == result );

		// Do result interpretation
		switch ( result )
		{
		case S_OK:
			
			outStats.VerticesRead						= resultStats.IAVertices;
			outStats.PrimitivesRead						= resultStats.IAPrimitives;
			outStats.VertexShaderInvocations			= resultStats.VSInvocations;
			outStats.GeometryShaderInvocations			= resultStats.GSInvocations;
			outStats.PrimitivesOutputByGeometryShader	= resultStats.GSPrimitives;
			outStats.PrimitivesSentToRasterizer			= resultStats.CInvocations;
			outStats.PrimitivesRendered					= resultStats.CPrimitives;
			outStats.PixelShaderInvocations				= resultStats.PSInvocations;
			outStats.HullShaderInvocations				= resultStats.HSInvocations;
			outStats.DomainShaderInvocations			= resultStats.DSInvocations;
			outStats.ComputeShaderInvocations			= resultStats.CSInvocations;

			return QR_Success;

		case S_FALSE:
			return QR_Pending;

		default:
			return QR_Error;
		}
	}

	eQueryResult GetQueryResult( const QueryRef &query, Uint64& frequency, Bool& disjoint, Bool forceImmediate )
	{
		if ( !query )
		{
			return QR_Error;
		}

		// Get data
		GPUAPI_ASSERT( IsInit() );
		const SQueryData &data = GetDeviceData().m_Queries.Data( query );
		if ( !data.m_pQuery )
		{
			return QR_Error;
		}

		GPUAPI_ASSERT( data.m_queryType == QT_TimestampDisjoint );

		// Get data
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT resultStats;
		HRESULT result;
		do 
		{
			result = GetDeviceContext()->GetData( data.m_pQuery, &resultStats, sizeof(resultStats), (!forceImmediate ? D3D11_ASYNC_GETDATA_DONOTFLUSH : 0) );
		}
		while ( forceImmediate && S_FALSE == result );

		// Do result interpretation
		switch ( result )
		{
		case S_OK:

			frequency = resultStats.Frequency;
			disjoint = resultStats.Disjoint != 0;
			return QR_Success;

		case S_FALSE:
			return QR_Pending;

		default:
			return QR_Error;
		}
	}

	Uint8 GetNumberOfDelayedFrames()
	{
		return 5;
	}
}