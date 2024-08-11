/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderMeshStreaming.h"

//----

CRenderMeshStreaming::CRenderMeshStreaming()
	: m_numBuffersPending( 0 )
	, m_sizeBuffersPending( 0 )
	, m_lastFrameBuffersCount( 0 )
	, m_lastFrameBuffersSize( 0 )
{
}

void CRenderMeshStreaming::ReportBufferPending( const Uint32 size )
{
	m_numBuffersPending.Increment();
	m_sizeBuffersPending.ExchangeAdd( size );
}

void CRenderMeshStreaming::ReportBufferLoaded( const Uint32 size )
{
	m_numBuffersPending.Decrement();
	m_sizeBuffersPending.ExchangeAdd( -(Int32)size );
}

void CRenderMeshStreaming::NextFrame()
{
	m_lastFrameBuffersCount = m_numBuffersPending.GetValue();
	m_lastFrameBuffersSize = m_sizeBuffersPending.GetValue();
}

void CRenderMeshStreaming::GetStats( Uint32& outNumBuffers, Uint32& outBufferSize ) const
{
	outNumBuffers = m_lastFrameBuffersCount;
	outBufferSize = m_lastFrameBuffersSize;
}

const Bool CRenderMeshStreaming::IsLoading() const
{
	return m_numBuffersPending.GetValue() > 0;
}

//----

