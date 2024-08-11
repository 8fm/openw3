/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNode.h"

#include "pathlibAreaDescription.h"
#include "pathlibLinksBuffer.h"
#include "pathlibNavgraph.h"
#include "pathlibSimpleBuffers.h"
#include "pathlibWorld.h"


namespace PathLib
{


CSearchNode::Marking::Marking( CPathLibWorld& pathlib )
	: m_marker( pathlib.GetSearchEngine().ObtainNewMarker() )
{

}
Bool CSearchNode::Marking::Debug_IsMarkerValid( CPathLibWorld& pathlib )
{
	return pathlib.GetSearchEngine().IsMarkerValid( m_marker );
}


/////////////////////////////////////////////////////////////////////////
// CPathNode
/////////////////////////////////////////////////////////////////////////

const CPathNode::Id CPathNode::Id::INVALID = CPathNode::Id::Create( INVALID_INDEX, INVALID_INDEX );
const CPathNode::Id CPathNode::Id::VALUE_MIN = CPathNode::Id::Create( 0, 0 );
const CPathNode::Id CPathNode::Id::VALUE_MAX = CPathNode::Id::Create( INVALID_INDEX, INVALID_INDEX );

CPathNode& CPathNode::operator=( const CPathNode& n )
{
	m_id = n.m_id;
	m_flags = n.m_flags;
	m_areaId = n.m_areaId;
	m_region = n.m_region;
	m_links = n.m_links;
	m_position = n.m_position;
	m_graph = n.m_graph;

	return *this;
}

void CPathNode::AddFlagsToLinks( NodeFlags flags )
{
	for ( LinksIterator it( m_graph->GetLinksBuffer(), m_links )
		; it
		; ++it )
	{
		CPathLinkModifier modifier( *this, *it );
		modifier.AddFlags( flags );
	}
	AddFlags( flags );
}
void CPathNode::ClearFlagsAtLinks( NodeFlags flags )
{
	for ( LinksIterator it( m_graph->GetLinksBuffer(), m_links )
		; it
		; ++it )
	{
		CPathLinkModifier modifier( *this, *it );
		modifier.ClearFlags( flags );
	}
	ClearFlags( flags );
}

CPathLink* CPathNode::GetLinkTo(const CPathNode* destination)
{
	for ( LinksIterator it( m_graph->GetLinksBuffer(), m_links )
		; it
		; ++it )
	{
		CPathLink& link = (*it);

		if ( link.GetDestination() == destination )
		{
			return &link;
		}
	}
	return nullptr;
}

const CPathLink* CPathNode::GetLinkTo( const CPathNode* destination ) const
{
	for ( ConstLinksIterator it( m_graph->GetLinksBuffer(), m_links )
		; it
		; ++it )
	{
		const CPathLink& link = (*it);
		
		if ( link.GetDestination() == destination )
		{
			return &link;
		}
		
	}
	return nullptr;
}

Bool CPathNode::IsConnected( Id nodeId ) const
{
	for ( ConstLinksIterator it( m_graph->GetLinksBuffer(), m_links )
		; it
		; ++it )
	{
		const CPathLink& link = *it;
		if ( link.HaveFlag( NF_DESTINATION_IS_ID ) )
		{
			if ( link.GetDestinationId() == nodeId )
			{
				return true;
			}
		}
		else
		{
			if ( link.GetDestination()->GetFullId() == nodeId )
			{
				return true;
			}
		}
	}
	return false;
}

void CPathNode::EraseLink( LinksErasableIterator& itLink )
{
	CPathLink& link = *itLink;
	if ( link.HaveFlag( NF_DESTINATION_IS_ID ) )
	{
		CPathNode* node = m_graph->VGetPathNode( link.GetDestinationId() );

		LinksErasableIterator itBack( *node );

		while ( itBack )
		{
			if ( (*itBack).GetDestinationId() == m_id )
			{
				itBack.Erase();
				break;
			}
			++itBack;
		}
		itLink.Erase();
		return;
	}
	CPathNode* node = link.GetDestination();

	LinksErasableIterator itBack( *node );

	while ( itBack )
	{
		if ( itBack->GetDestination() == this )
		{
			itBack.Erase();
			break;
		}
		++itBack;
	}
	itLink.Erase();
}

void CPathNode::Unlink()
{
	LinksErasableIterator it( m_graph->GetLinksBuffer(), m_links );

	while ( it )
	{
		CPathNode* destinationNode = it->GetDestination();

		LinksErasableIterator itBack( destinationNode->GetGraph().GetLinksBuffer(), destinationNode->m_links );
		while ( itBack )
		{
			if ( itBack->GetDestination() == this )
			{
				itBack.Erase();
				break;
			}

			++itBack;
		}
		it.Erase();
	}
}

void CPathNode::Unlink( NodeFlags byFlag )
{
	for ( LinksErasableIterator it( m_graph->GetLinksBuffer(), m_links ); it; )
	{
		if ( !it->HaveAnyFlag( byFlag ) )
		{
			++it;
			continue;
		}

		CPathNode* destinationNode = it->GetDestination();

		LinksErasableIterator itBack( destinationNode->GetGraph().GetLinksBuffer(), destinationNode->m_links );
		while ( itBack )
		{
			if ( itBack->GetDestination() == this )
			{
				itBack.Erase();
				break;
			}

			++itBack;
		}
		it.Erase();
	}
}

//void CPathNode::Serialize( IFile& file )
//{
//	file << m_id.m_id;
//	file << m_flags;
//	file << m_position;
//	Uint16 linksCount = Uint16(m_links.Size());
//	file << linksCount;
//	if ( file.IsReader() )
//	{
//		m_links.Resize( linksCount );
//	}
//	for ( Uint16 i = 0; i != linksCount; ++i )
//	{
//		m_links[ i ].Serialize( file );
//	}
//}

void CPathNode::WriteToBuffer( CSimpleBufferWriter& writer, LinkBufferIndex& linksCountAccumulator ) const
{
	NodeFlags flags = m_flags & NFG_SERIALIZABLE;
	writer.Put( flags );
	writer.Put( m_position );
	writer.Put( m_region );
	Uint16 linksCount = 0;
	Uint32 linksReserved = writer.ReserveSpace( linksCount );

	for ( ConstLinksIterator it( m_graph->GetLinksBuffer(), m_links )
		; it
		; ++it )
	{
		if ( it->HaveAnyFlag( NF_CONNECTOR ) )
		{
			continue;
		}
		it->WriteToBuffer( writer );
		++linksCount;
	}
	writer.PutReserved( linksCount, linksReserved );
	linksCountAccumulator += linksCount;
}
Bool CPathNode::ReadFromBuffer( CSimpleBufferReader& reader, CPathLibGraph& graph, AreaId areaId, NodesetIdx nodeSetIdx, Index index, LinkBufferIndex& linksCountAccumulator )
{
	m_graph= &graph;

	if ( !reader.Get( m_flags ) ||
		!reader.Get( m_position ) ||
		!reader.Get( m_region ) )
	{
		return false;
	}

	Uint16 linksCount;
	if ( !reader.Get( linksCount ) )
	{
		return false;
	}

	CLinksBuffer& linksBuffer = graph.GetLinksBuffer();
	// NOTICE: mind the fact that prevIndex points to unitialized data.
	LinkBufferIndex* prevIndex = &m_links;
	for ( Uint16 i = 0; i < linksCount; ++i )
	{
		*prevIndex = linksCountAccumulator;
		CPathLink& link = linksBuffer.Get( linksCountAccumulator );
		if ( !link.ReadFromBuffer( reader ) )
		{
			return false;
		}
		prevIndex = &link.m_next;
		++linksCountAccumulator;
	}
	*prevIndex = INVALID_LINK_BUFFER_INDEX;

	m_areaId = areaId;
	m_id.m_index = index;
	m_id.m_nodeSetIndex = nodeSetIdx;

	return true;
}


/////////////////////////////////////////////////////////////////////////
// CPathLink
/////////////////////////////////////////////////////////////////////////

void CPathLink::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	CPathNode::Id id = m_destination->GetFullId();
	writer.Put( id.m_id );

	NodeFlags flags = m_flags & NFG_SERIALIZABLE;
	writer.Put( flags );
	writer.Put( m_cost );
}
Bool CPathLink::ReadFromBuffer( CSimpleBufferReader& reader )
{
	if ( !reader.Get( m_destinationId.m_id ) )
	{
		return false;
	}

	if ( !reader.Get( m_flags ) )
	{
		return false;
	}

	if ( !reader.Get( m_cost ) )
	{
		return false;
	}

	m_flags |= NF_DESTINATION_IS_ID;
	return true;
}

/////////////////////////////////////////////////////////////////////////
// CNavLinkModifier
/////////////////////////////////////////////////////////////////////////
CPathLinkModifier::CPathLinkModifier( CPathNode& node, CPathLink& link )
{
	m_link1 = &link;

	if ( link.m_flags & NF_DESTINATION_IS_ID )
	{
		CPathNode::Id baseNodeId = node.GetFullId();
		CPathNode* destinationNode;
		if ( link.HaveFlag( NF_CONNECTOR ) )
		{
			// decode connector
			destinationNode = node.GetGraph().VGetExternalPathNode( link.m_destinationId.m_index, AreaId( link.m_destinationId.m_nodeSetIndex ) );
			PATHLIB_ASSERT( destinationNode );
			for ( LinksIterator it( *destinationNode )
				; it
				; ++it )
			{
				//we look for connector that is also connected by id
				if ( it->HaveAllFlags( NF_CONNECTOR | NF_DESTINATION_IS_ID ) )
				{
					// we check target index, and area id, that is coded in m_nodeSetIndex field
					if ( it->m_destinationId.m_index == node.GetIndex() && AreaId( it->m_destinationId.m_nodeSetIndex ) == node.GetAreaId() )
					{
						m_link2 = &(*it);
						PATHLIB_ASSERT( m_link1->GetFlags() == m_link2->GetFlags() );
						PATHLIB_ASSERT( m_link1 != m_link2 );
						return;
					}
				}
			}
		}
		else
		{
			destinationNode = node.GetGraph().VGetPathNode( link.m_destinationId );
			for ( LinksIterator it( *destinationNode )
				; it
				; ++it )
			{
				if ( it->m_destinationId.m_index == baseNodeId.m_index && it->m_destinationId.m_nodeSetIndex == baseNodeId.m_nodeSetIndex )
				{
					m_link2 = &(*it);
					PATHLIB_ASSERT( m_link1->GetFlags() == m_link2->GetFlags() );
					PATHLIB_ASSERT( m_link1 != m_link2 );
					return;
				}
			}
		}

	}
	else
	{
		CPathNode* n2 = link.m_destination;
		for ( LinksIterator it( *n2 )
			; it
			; ++it )
		{
			if ( it->m_destination == &node )
			{
				m_link2 = &(*it);
				PATHLIB_ASSERT( m_link1->GetFlags() == m_link2->GetFlags() );
				ASSERT( m_link1 != m_link2 );
				return;
			}
		}
	}
	PATHLIB_ASSERT( false );
	m_link2 = nullptr;
}

void CPathLinkModifier::SetCost( NavLinkCost cost )
{
	m_link1->m_cost = cost;
	if ( m_link2 )
	{
		m_link2->m_cost = cost;
	}
}
void CPathLinkModifier::AddFlags( NodeFlags flags )
{
	m_link1->m_flags |= flags;
	if ( m_link2 )
	{
		m_link2->m_flags |= flags;
	}
}
void CPathLinkModifier::ClearFlags( NodeFlags flags )
{
	m_link1->m_flags &= ~flags;
	if ( m_link2 )
	{
		m_link2->m_flags &= ~flags;
	}
}

void CPathLinkModifier::SetFlags( NodeFlags flags )
{
	m_link1->m_flags = flags;
	if ( m_link2 )
	{
		m_link2->m_flags = flags;
	}
}

Bool CPathLinkModifier::Erase()
{
	if ( m_link1 && m_link2 )
	{
		CPathNode* node1 = m_link2->GetDestination();
		CPathNode* node2 = m_link1->GetDestination();
		for ( LinksErasableIterator it( *node1 )
			; it
			; ++it )
		{
			if ( &(*it) == m_link1 )
			{
				it.Erase();
				break;
			}
		}
		for ( LinksErasableIterator it( *node2 )
			; it
			; ++it )
		{
			if ( &(*it) == m_link2 )
			{
				it.Erase();
				break;
			}
		}
		return true;
	}
	return false;
}
Bool CPathLinkModifier::GenerationErase( CPathLibGraph* graph )
{
	if ( m_link1 && m_link2 )
	{
		Bool byId = m_link1->HaveFlag( NF_DESTINATION_IS_ID );
		CPathNode* node1 =
			byId
			? graph->VGetPathNode( m_link2->GetDestinationId() )
			: m_link2->GetDestination();
		CPathNode* node2 =
			byId
			? graph->VGetPathNode( m_link1->GetDestinationId() )
			: m_link1->GetDestination();

		for ( LinksErasableIterator it( *node1 )
			; it
			; ++it )
		{
			if ( &(*it) == m_link1 )
			{
				it.Erase();
				break;
			}
		}
		for ( LinksErasableIterator it( *node2 )
			; it
			; ++it )
		{
			if ( &(*it) == m_link2 )
			{
				it.Erase();
				break;
			}
		}
		return true;
	}
	return false;
}
void CPathLinkModifier::ChangeNodeIndex( CPathNode::Index idFrom, CPathNode::Index idTo )
{
	ASSERT( m_link2->m_destinationId.m_index == idFrom );
	m_link2->m_destinationId.m_index = idTo;
}


void CPathLinkModifier::ConvertToPointers( CPathLibGraph* graph )
{
	if ( m_link1->m_flags & NF_DESTINATION_IS_ID )
	{
		ASSERT( (m_link1->m_flags & NF_CONNECTOR) == 0 );

		m_link1->m_destination = graph->VGetPathNode( m_link1->m_destinationId );
		m_link1->m_flags &= ~NF_DESTINATION_IS_ID;
		if ( m_link2 )
		{
			m_link2->m_destination = graph->VGetPathNode( m_link2->m_destinationId );
			m_link2->m_flags &= ~NF_DESTINATION_IS_ID;
		}
	}
}

void CPathLinkModifier::ConvertToIds( CPathLibGraph* graph )
{
	if ( !(m_link1->m_flags & NF_DESTINATION_IS_ID) )
	{
		ASSERT( (m_link1->m_flags & NF_CONNECTOR) == 0 );
		ASSERT( !m_link2 || (m_link1->m_destinationId.m_id != m_link2->m_destinationId.m_id) );

		m_link1->m_destinationId = m_link1->m_destination->GetFullId();
		m_link1->m_flags |= NF_DESTINATION_IS_ID;
		if ( m_link2 )
		{
			m_link2->m_destinationId = m_link2->m_destination->GetFullId();
			m_link2->m_flags |= NF_DESTINATION_IS_ID;
		}
	}
}

void CPathLinkModifier::ConvertConnectorToPointers( CPathLibGraph* graph )
{
	if ( m_link1->m_flags & NF_DESTINATION_IS_ID )
	{
		CPathNode::Index idx1 = m_link1->m_destinationId.m_index;
		AreaId areaId1 = AreaId( m_link1->m_destinationId.m_nodeSetIndex );

		m_link1->m_destination = graph->VGetExternalPathNode( idx1, areaId1 );
		m_link1->m_flags &= ~NF_DESTINATION_IS_ID;

		RED_ASSERT( m_link1->m_destination != nullptr );

		if ( m_link2 )
		{
			CPathNode::Index idx2 = m_link2->m_destinationId.m_index;
			AreaId areaId2 = AreaId( m_link2->m_destinationId.m_nodeSetIndex );

			m_link2->m_destination = graph->VGetExternalPathNode( idx2, areaId2 );
			m_link2->m_flags &= ~NF_DESTINATION_IS_ID;

			RED_ASSERT( m_link2->m_destination != nullptr );
		}
	}
}
void CPathLinkModifier::ConvertConnectorToIds( CPathLibGraph* graph )
{
	if ( !(m_link1->m_flags & NF_DESTINATION_IS_ID) )
	{
		ASSERT( m_link1->m_flags & NF_CONNECTOR );

		CPathNode::Index index1 = m_link1->m_destination->GetIndex();
		AreaId area1 = m_link1->m_destination->GetAreaId();

		m_link1->m_destinationId.m_index = index1;
		m_link1->m_destinationId.m_nodeSetIndex = CPathNode::NodesetIdx( area1 );
		m_link1->m_flags |= NF_DESTINATION_IS_ID;
		RED_ASSERT( m_link2, TXT("One sided connector??") );
		if ( m_link2 )
		{
			CPathNode::Index index2 = m_link2->m_destination->GetIndex();
			AreaId area2 = m_link2->m_destination->GetAreaId();

			m_link2->m_destinationId.m_index = index2;
			m_link2->m_destinationId.m_nodeSetIndex = CPathNode::NodesetIdx( area2 );
			m_link2->m_flags |= NF_DESTINATION_IS_ID;
		}
	}
}

};			// namespace PathLib