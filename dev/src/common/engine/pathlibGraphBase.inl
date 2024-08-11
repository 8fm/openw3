/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibGraph.h"
#include "pathlibSimpleBuffers.h"


// Heavy (in code) procedures needed for implementation of specialization of TPathLibGraph, but not required for headers

namespace PathLib
{

template < class TNodeClass >
RED_INLINE void TPathLibGraph< TNodeClass >::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	LinkBufferIndex linksCount = 0;
	Uint32 linksReservation = writer.ReserveSpace( linksCount );

	CPathNode::Index nodesCount = CPathNode::Index( m_nodes.Size() );
	writer.Put( nodesCount );

	for ( CPathNode::Index i = 0; i < nodesCount; ++i )
	{
		m_nodes[ i ].WriteToBuffer( writer, linksCount );
	}
	writer.PutReserved( linksCount, linksReservation );
}

template < class TNodeClass >
RED_INLINE Bool TPathLibGraph< TNodeClass >::ReadFromBuffer( CSimpleBufferReader& reader, CPathNode::NodesetIdx nodeSetIndex )
{
	LinkBufferIndex linksCount;
	if ( !reader.Get( linksCount ) )
	{
		return false;
	}
	LinkBufferIndex extraLinksSpace = VGetExtraLinksSpace();
	m_links.Resize( linksCount, extraLinksSpace );

	CPathNode::Index nodesCount;
	if ( !reader.Get( nodesCount ) )
	{
		return false;
	}

	m_nodes.Resize( nodesCount );

	LinkBufferIndex linksAccumulator = 0;
	AreaId areaId = VGetAreaId();
	for ( CPathNode::Index i = 0; i < nodesCount; ++i )
	{
		if ( !m_nodes[ i ].ReadFromBuffer( reader, *this, areaId, nodeSetIndex, i, linksAccumulator ) )
		{
			return false;
		}
	}
	RED_ASSERT( linksAccumulator == linksCount );

	return true;
}

template < class TNodeClass >
RED_INLINE Bool TPathLibGraph< TNodeClass >::ConvertLinksToIds( Bool supportConnectors )
{
	// make all links run as id's
	for ( auto itNodes = m_nodes.Begin(), endNodes = m_nodes.End(); itNodes != endNodes; ++itNodes )
	{
		auto& node = *itNodes;

		for ( LinksIterator itLinks( node ); itLinks; ++itLinks )
		{
			CPathLink& link = *itLinks;
			if ( !link.HaveAnyFlag( NF_DESTINATION_IS_ID ) )
			{
				CPathLinkModifier modifier( node, link );
				if ( supportConnectors && link.HaveFlag( NF_CONNECTOR ) )
				{
					modifier.ConvertConnectorToIds( this );
				}
				else
				{
					ASSERT( !link.HaveFlag( NF_CONNECTOR ) );
					modifier.ConvertToIds( this );
				}
				
			}
		}
	}

	return true;
}

template < class TNodeClass >
RED_INLINE Bool TPathLibGraph< TNodeClass >::ConvertLinksToPointers( Bool supportConnectors )
{
	// make all links run on pointers
	for ( auto itNodes = m_nodes.Begin(), endNodes = m_nodes.End(); itNodes != endNodes; ++itNodes )
	{
		auto& node = *itNodes;

		for ( LinksIterator itLinks( node ); itLinks; ++itLinks )
		{
			CPathLink& link = *itLinks;
			if ( link.HaveAnyFlag( NF_DESTINATION_IS_ID ) )
			{
				CPathLinkModifier modifier( node, link );
				if ( supportConnectors && link.HaveAnyFlag( NF_CONNECTOR ) )
				{
					modifier.ConvertConnectorToPointers( this );
				}
				else
				{
					ASSERT( !link.HaveAnyFlag( NF_CONNECTOR ) );
					modifier.ConvertToPointers( this );
				}
			}
		}
	}

	return true;
}

template < class TNodeClass >
RED_INLINE Bool TPathLibGraph< TNodeClass >::Debug_CheckAllLinksTwoSided()
{
	for ( auto it = m_nodes.Begin(), end = m_nodes.End(); it != end; ++it )
	{
		TNodeClass& node = *it;
		for ( LinksIterator itLink( node ); itLink; ++itLink )
		{
			CPathLink& link = *itLink;
			if ( !link.HaveFlag( NF_IS_ONE_SIDED ) )
			{
				CPathLinkModifier modifier( node, link );
				if ( !modifier.IsDoubleSided() )
				{
					return false;
				}
			}
		}
	}
	return true;
}


};		// namespace PathLib

