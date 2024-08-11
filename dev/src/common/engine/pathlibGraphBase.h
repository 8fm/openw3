/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibGraph.h"
#include "pathlibSimpleBuffers.h"


class CSimpleBufferReader;
class CSimpleBufferWriter;

namespace PathLib
{


template < class TNodeClass >
class TPathLibGraph : public CPathLibGraph
{
public:
	typedef TDynArray< TNodeClass, MC_PathLib > NodesArray;
protected:
	NodesArray								m_nodes;

	TNodeClass&				AddNode( const Vector3& position, AreaId areaId, NodeFlags flags )
	{
		Bool isOverflow = ( m_nodes.Size() == m_nodes.Capacity() && !m_nodes.Empty() );
		if ( isOverflow )
		{
			PreNodeArrayOverflow();
		}
		void* data = m_nodes.Data();
		m_nodes.PushBack( TNodeClass( position, CPathNode::Index( m_nodes.Size() ), areaId, *this, flags ) );
		ASSERT( isOverflow == (data != m_nodes.Data() && data) );
		if ( isOverflow )
		{
			OnNodeArrayOverflow();
		}
		return m_nodes.Back();
	}
	TNodeClass&				AddNode( const Vector3& position, AreaId areaId, CPathNode::NodesetIdx nodesetIdx, NodeFlags flags )
	{
		Bool isOverflow = ( m_nodes.Size() == m_nodes.Capacity() && !m_nodes.Empty() );
		if ( isOverflow )
		{
			PreNodeArrayOverflow();
		}
		CPathNode::Id nodeId;
		nodeId.m_index = CPathNode::Index( m_nodes.Size() );
		nodeId.m_nodeSetIndex = nodesetIdx;
		void* data = m_nodes.Data();
		m_nodes.PushBack( TNodeClass( position, nodeId, areaId, *this, flags ) );
		ASSERT( isOverflow == (data != m_nodes.Data() && data) );
		if ( isOverflow )
		{
			OnNodeArrayOverflow();
		}
		return m_nodes.Back();
	}
	TNodeClass&				AddNode( const TNodeClass& node )
	{
		Bool isOverflow = ( m_nodes.Size() == m_nodes.Capacity() && !m_nodes.Empty() );
		if ( isOverflow )
		{
			PreNodeArrayOverflow();
		}
		CPathNode::Index index = CPathNode::Index( m_nodes.Size() );
		void* data = m_nodes.Data();
		m_nodes.PushBack( Move( node ) );
		TNodeClass& retNode = m_nodes.Back();
		SetNodeIndex( retNode, index );
		ASSERT( isOverflow == (data != m_nodes.Data() && data) );
		if ( isOverflow )
		{
			OnNodeArrayOverflow();
		}
		return retNode;
	}

	RED_INLINE Bool		ConvertLinksToIds( Bool supportConnectors = false );
	RED_INLINE Bool		ConvertLinksToPointers( Bool supportConnectors = false );
		
	RED_INLINE Bool		Debug_CheckAllLinksTwoSided();

public:
	TPathLibGraph()
		: CPathLibGraph()													{}


	const TNodeClass*		GetNode( CPathNode::Index idx ) const			{ return &m_nodes[ idx ]; }
	TNodeClass*				GetNode( CPathNode::Index idx )					{ return &m_nodes[ idx ]; }

	const NodesArray&		GetNodesArray() const							{ return m_nodes; }
	NodesArray&				GetNodesArray()									{ return m_nodes; }

	void					Unload() override								{ m_nodes.Clear(); }
	void					ReserveNodes( Uint32 nodesCount )				{ m_nodes.Reserve( nodesCount ); }

	RED_INLINE void			WriteToBuffer( CSimpleBufferWriter& writer ) const;
	RED_INLINE Bool			ReadFromBuffer( CSimpleBufferReader& reader, CPathNode::NodesetIdx nodeSetIndex = CPathNode::INVALID_INDEX );

	// :( can't parametrize DeleteMarked imlementation from outside
	template < class Implementation >
	RED_INLINE Bool			DeleteMarked( Implementation& handler )
	{
		CPathNode::Index currIndex = 0;
		CPathNode::Index nodesCount = CPathNode::Index( m_nodes.Size() );
		CPathNode::Index indexShift = 0;
		for ( ; currIndex < nodesCount; ++currIndex )
		{
			if ( m_nodes[ currIndex ].HaveAnyFlag( NF_MARKED_FOR_DELETION ) )
			{
				handler.PreNodeTrashing( m_nodes[ currIndex ] );
				ASSERT( m_nodes[ currIndex ].GetLinksArray() == INVALID_LINK_BUFFER_INDEX, TXT("Pathlib error. Node marked for deletion has some links.") );
				++indexShift;
				++currIndex;
				break;
			}
		}
		if ( indexShift )
		{
			handler.PreDeletion();

			for ( ; currIndex < nodesCount; ++currIndex )
			{
				if ( m_nodes[ currIndex ].HaveAnyFlag( NF_MARKED_FOR_DELETION ) )
				{
					handler.PreNodeTrashing( m_nodes[ currIndex ] );
					ASSERT(  m_nodes[ currIndex ].GetLinksArray() == INVALID_LINK_BUFFER_INDEX, TXT("Pathlib error. Node marked for deletion has some links.") );
					++indexShift;
				}
				else
				{
					// change node index
					CPathNode::Index shiftedIndex = currIndex - indexShift;
					{
						TNodeClass& originalNode = m_nodes[ currIndex ];

						handler.PreNodeIndexChange( originalNode, shiftedIndex );

						// change all links
						for ( LinksIterator it( originalNode ); it; ++it )
						{
							CPathLink& link = *it;
							ASSERT( link.HaveFlag( NF_DESTINATION_IS_ID ), TXT("PathLib error. We are fucked!\n") );
							CPathLinkModifier modifier( originalNode, link );

							modifier.ChangeNodeIndex( currIndex, shiftedIndex );
						}
						m_nodes[ shiftedIndex ] = Move( originalNode );
					}

					TNodeClass& shiftedNode = m_nodes[ shiftedIndex ];
					shiftedNode.InternalChangeIndex( shiftedIndex );
				}
			}

			handler.PostDeletion();

			m_nodes.ResizeFast( nodesCount - indexShift );
			return true;
		}
		
		return false;
	}
};

RED_INLINE const CPathNode* CSearchNode::AsPathNode() const
{
	return static_cast< const CPathNode* >( this );
}
RED_INLINE CPathNode* CSearchNode::ModifyPathNode()
{
	return static_cast< CPathNode* >( this );
}
RED_INLINE const Vector3& CSearchNode::GetPosition() const
{
	return AsPathNode()->GetPosition();
}



};			// namespace PathLib