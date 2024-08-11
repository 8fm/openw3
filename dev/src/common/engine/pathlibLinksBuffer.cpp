/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibLinksBuffer.h"

#include "pathlibGraph.h"


namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CLinksBuffer
///////////////////////////////////////////////////////////////////////////////
LinkBufferIndex CLinksBuffer::Allocate( const CPathLink& link )
{
	if ( m_freeList != INVALID_LINK_BUFFER_INDEX )
	{
		LinkBufferIndex idx = m_freeList;
		CPathLink& bufLink = m_buffer[ idx ];
		m_freeList = bufLink.m_next;
		bufLink = link;

		return idx;
	}
	LinkBufferIndex idx = m_buffer.Size();
	m_buffer.PushBack( link );
	return idx;
}

void CLinksBuffer::Resize( LinkBufferIndex count, LinkBufferIndex extraSpace )
{
	m_buffer.Reserve( count + extraSpace );
	m_buffer.ResizeFast( count );
}

void CLinksBuffer::AddFlagsToEveryLink( Uint32 flags )
{
	for ( CPathLink& link : m_buffer )
	{
		link.m_flags |= flags;
	}
}
void CLinksBuffer::ClearFlagsToEveryLink( Uint32 flags )
{
	for ( CPathLink& link : m_buffer )
	{
		link.m_flags &= ~flags;
	}
}

///////////////////////////////////////////////////////////////////////////////
// LinksIterator
///////////////////////////////////////////////////////////////////////////////
LinksIterator::LinksIterator( CPathNode& pathNode )
	: m_links( pathNode.GetGraph().GetLinksBuffer().m_buffer )
	, m_index( pathNode.GetLinksArray() )
{

}

///////////////////////////////////////////////////////////////////////////////
// LinksErasableIterator
///////////////////////////////////////////////////////////////////////////////
LinksErasableIterator::LinksErasableIterator( CPathNode& pathNode )
	: m_buffer( pathNode.GetGraph().GetLinksBuffer() )
	, m_index( &pathNode.m_links )
{

}

void LinksErasableIterator::Erase()
{
	ASSERT( (*this) == true );
	LinkBufferIndex removedIndex = *m_index;
	CPathLink& removedLink = m_buffer.m_buffer[ removedIndex ];
	// remove current element from the list
	*m_index = removedLink.m_next;
	// add element to detached list (on top)
	removedLink.m_next = m_buffer.m_freeList;
	m_buffer.m_freeList = removedIndex;

}


///////////////////////////////////////////////////////////////////////////////
// ConstLinksIterator
///////////////////////////////////////////////////////////////////////////////
ConstLinksIterator::ConstLinksIterator( const CPathNode& pathNode )
	: m_links( pathNode.GetGraph().GetLinksBuffer().m_buffer )
	, m_index( pathNode.GetLinksArray() )
{

}



///////////////////////////////////////////////////////////////////////////////
// CLinksList
///////////////////////////////////////////////////////////////////////////////
//void CLinksList::ClearAll( CLinksBuffer& linksBuffer )
//{
//	LinksErasableIterator it( linksBuffer, m_first );
//	while ( it )
//	{
//		it.Erase();
//	}
//}


};				// namespace PathLib


