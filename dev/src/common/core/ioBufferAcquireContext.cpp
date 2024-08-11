#include "build.h"
#include "ioBufferAcquireContext.h"


//////////////////////////////////////////////////////////////////////////
CIOBufferAcquireContext::CIOBufferAcquireContext( const Uint32 bufferSize, const Uint32 flags, const Uint32 spanCount )
	: m_requiredSize( bufferSize )
	, m_flags( flags )
{
	m_bufferSpans.Reserve( spanCount );
}

//////////////////////////////////////////////////////////////////////////
CIOBufferAcquireContext::~CIOBufferAcquireContext()
{

}

//////////////////////////////////////////////////////////////////////////
void CIOBufferAcquireContext::AddBufferSpan( const Uint32 start, const Uint32 end )
{
	m_bufferSpans.PushBack( CBufferSpan( start, end ) );
}

const Uint32 CIOBufferAcquireContext::GetBufferSpanCount() const
{
	return m_bufferSpans.Size();
}

//////////////////////////////////////////////////////////////////////////
const Uint32 CIOBufferAcquireContext::GetSize() const
{
	return m_requiredSize;
}

//////////////////////////////////////////////////////////////////////////
const Uint32 CIOBufferAcquireContext::GetFlags() const
{
	return m_flags;
}

//////////////////////////////////////////////////////////////////////////
const BufferSpanCollection& CIOBufferAcquireContext::GetBufferSpans() const
{
	return m_bufferSpans;
}