#include "build.h"
#include "bufferSpan.h"

//////////////////////////////////////////////////////////////////////////
CBufferSpan::CBufferSpan( const Uint32 start, const Uint32 end )
	: m_start( start )
	, m_end( end )
{
	RED_FATAL_ASSERT( m_start <= m_end, "Buffer span overlap seems inverted start {%u} is greater then the end {%u}", m_start, m_end );
}

//////////////////////////////////////////////////////////////////////////
CBufferSpan::CBufferSpan( const CBufferSpan& other )
	: m_start( other.m_start )
	, m_end( other.m_end )
{
}

//////////////////////////////////////////////////////////////////////////
CBufferSpan::CBufferSpan( CBufferSpan&& other )
	: m_start( std::move( other.m_start ) )
	, m_end( std::move( other.m_end ) )
{
}

//////////////////////////////////////////////////////////////////////////
CBufferSpan::~CBufferSpan()
{
}

//////////////////////////////////////////////////////////////////////////
CBufferSpan& CBufferSpan::operator=( const CBufferSpan& other )
{
	m_start = other.m_start;
	m_end = other.m_end;
	return *this;
}

//////////////////////////////////////////////////////////////////////////
CBufferSpan& CBufferSpan::operator=( CBufferSpan&& other )
{
	m_start = std::move( other.m_start );
	m_end = std::move( other.m_end );	
	return *this;
}