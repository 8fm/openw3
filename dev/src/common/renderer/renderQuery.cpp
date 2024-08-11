#include "build.h"
#include "renderQuery.h"

RED_DEFINE_STATIC_NAME( RenderQuery );

CRenderQuery::CRenderQuery( GpuApi::eQueryType queryType, Bool nonAutomaticQuery )
{
	// Create query
	if ( nonAutomaticQuery )
	{
		m_queriesCount = 1;
	}
	else
	{
		m_queriesCount = GpuApi::GetNumberOfDelayedFrames();
	}

	m_lastProperQuery = -((Int32)m_queriesCount - 1);
	m_nextQuery = 0;
	m_queries.Resize( m_queriesCount );
	for ( Int32 i=0; i<m_queriesCount; ++i )
	{
		m_queries[i] = GpuApi::CreateQuery( queryType );
	}

	m_queryIssued = false;
}

CRenderQuery::~CRenderQuery()
{
	ASSERT( !m_queryIssued );

	for ( Uint32 i=0; i<m_queries.Size(); ++i )
	{
		GpuApi::SafeRelease( m_queries[i] );
	}

}

Bool CRenderQuery::IsValid()
{
	if ( m_queries.Empty() )
	{
		return false;
	}

	for ( Uint32 i=0; i<m_queries.Size(); ++i )
	{
		if ( m_queries[i].isNull() )
		{
			return false;
		}
	}

	return true;
}

CName CRenderQuery::GetCategory() const
{
	return CNAME( RenderQuery );
}

Uint32 CRenderQuery::GetUsedVideoMemory() const
{
	return 0;
}

void CRenderQuery::OnDeviceLost()
{
	ASSERT( !m_queryIssued );
	for ( Uint32 i=0; i<m_queries.Size(); ++i )
	{
		GpuApi::SafeRelease( m_queries[i] );
	}
	m_queries.Clear();
}

void CRenderQuery::OnDeviceReset()
{
	// Do not reset queries - all objects using them NEED to be recreated on engine side
}

void CRenderQuery::BeginQuery()
{
	ASSERT( !m_queryIssued );
	m_queryIssued = true;

	if ( m_queries.Empty() )
	{
		return;
	}

	if ( m_queries[m_nextQuery] )
	{
		GpuApi::BeginQuery( m_queries[m_nextQuery] );
	}
}

void CRenderQuery::EndQuery()
{
	ASSERT( m_queryIssued );
	m_queryIssued = false;

	if ( m_queries.Empty() )
	{
		return;
	}

	if ( m_queries[m_nextQuery] )
	{
		GpuApi::EndQuery( m_queries[m_nextQuery] );

		m_nextQuery++;
		m_lastProperQuery++;

		if ( m_nextQuery >= m_queriesCount )
		{
			m_nextQuery = 0;
		}

		if ( m_lastProperQuery >= m_queriesCount )
		{
			m_lastProperQuery = 0;
		}
	}
}

template< typename RESULT_TYPE >
EQueryResult CRenderQuery::GetQueryResult( RESULT_TYPE& outResult, Bool forceImmediate )
{
	ASSERT( !m_queryIssued );

	if ( m_lastProperQuery < 0 )
	{
		return EQR_Pending;
	}

	if ( m_queries.Empty() )
	{
		return EQR_Error;
	}

	switch( GpuApi::GetQueryResult( m_queries[m_lastProperQuery], outResult, forceImmediate ) )
	{
	case GpuApi::QR_Success:
		return EQR_Success;

	case GpuApi::QR_Pending:
		return EQR_Pending;

	default:
		return EQR_Error;
	}
}

template EQueryResult CRenderQuery::GetQueryResult( Uint64& outResult, Bool forceImmediate );
template EQueryResult CRenderQuery::GetQueryResult( GpuApi::SPipelineStatistics& outResult, Bool forceImmediate );