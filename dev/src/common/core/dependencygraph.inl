/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		// SLinkSet
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceGraph::Node::LinkSet::LinkSet()
		:	m_count( 0 )
		,	m_max( 0 )
		,	m_links( nullptr )
		{

		}

#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING
		RED_INLINE System::Bool CResourceGraph::Node::LinkSet::Add( TIndexType link )
		{
			if( m_count >= m_max )
			{
				// Storage is full, expand
				m_max = Math::NumericalUtils::Max( 1u, m_max * c_GrowthMultiplier );
				m_links = static_cast< TIndexType* >( RED_MEMORY_REALLOCATE( MemoryPool_Default, m_links, MC_ResourceDatabase, m_max * sizeof( TIndexType ) ) );
			}

			m_links[ m_count ] = link;
			++m_count;

			return true;
		}
#endif // ENABLE_DEPENDENCY_GRAPH_EDITING

		RED_INLINE System::Bool CResourceGraph::Node::LinkSet::Has( TIndexType link ) const
		{
			for( TIndexType i = 0; i < m_count; ++i )
			{
				if( m_links[ i ] == link )
				{
					return true;
				}
			}

			return false;
		}

		RED_INLINE void CResourceGraph::Node::LinkSet::Release( Bool freeMemory )
		{
			// The process of freeing memory is conditional as we may have been given pre allocated pointers
			if( freeMemory )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceDatabase, m_links );
			}

			m_count = 0;
			m_max = 0;
			m_links = nullptr;
		}


		//////////////////////////////////////////////////////////////////////////
		// CResourceGraphNode
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceGraph::Node::Node()
		:	m_index( c_InvalidNodeIndex )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceGraph::Node::Node( TIndexType index )
		:	m_index( index )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceGraph::Node::~Node()
		{
		}

#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING
		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceGraph::Node::LinkTo( Node& child )
		{
			RED_ASSERT( !m_links[ Set_Children ].Has( child.m_index ), TXT( "Node has already been linked as a child" ) );
			RED_ASSERT( !child.m_links[ Set_Parents ].Has( m_index ), TXT( "Node has already been linked as a parent" ) );

			m_links[ Set_Children ].Add( child.m_index );
			child.m_links[ Set_Parents ].Add( m_index );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceGraph::Node::LinkToUnique( Node& child )
		{
			if( !m_links[ Set_Children ].Has( child.m_index ) )
			{
				LinkTo( child );
			}
		}
#endif // ENABLE_DEPENDENCY_GRAPH_EDITING

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Bool CResourceGraph::Node::IsLinked( const Node* nodes, TIndexType linkIndex ) const
		{
			return IsAncestor( nodes, linkIndex ) || IsDescendant( nodes, linkIndex );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Bool CResourceGraph::Node::IsDescendant( const Node* nodes, TIndexType descendantIndex ) const
		{
			return IsLinked( Set_Children, nodes, descendantIndex );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Bool CResourceGraph::Node::IsAncestor( const Node* nodes, TIndexType ancestorIndex ) const
		{
			return IsLinked( Set_Parents, nodes, ancestorIndex );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Bool CResourceGraph::Node::HasCircularDependency( const Node* nodes ) const
		{
			return IsLinked( nodes, m_index );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Uint32 CResourceGraph::Node::GetChildCount() const
		{
			return m_links[ Set_Children ].m_count;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE Uint32 CResourceGraph::Node::GetLinkCount() const
		{
			return m_links[ Set_Children ].m_count + m_links[ Set_Parents ].m_count;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceGraph::Node::SerialiseCounts( IFile& file )
		{
			for( TIndexType i = 0; i < Set_Max; ++i )
			{
				file << m_links[ i ].m_count;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceGraph::Node::WriteLinksToFile( IFile& file ) const
		{
			for( TIndexType i = 0; i < Set_Max; ++i )
			{
				file.Serialize( m_links[ i ].m_links, m_links[ i ].m_count * sizeof( TIndexType ) );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceGraph::Node::SetBuffer( void* buffer )
		{
			m_links[ Set_Children ].m_links = static_cast< TIndexType* >( buffer );
			m_links[ Set_Parents ].m_links = m_links[ Set_Children ].m_links + m_links[ Set_Children ].m_count;
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE void CResourceGraph::Node::Release( Bool freeMemory )
		{
			for( TIndexType i = 0; i < Set_Max; ++i )
			{
				m_links[ i ].Release( freeMemory );
			}

			m_index = c_InvalidNodeIndex;
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename TContainer >
		RED_INLINE void CResourceGraph::Node::GetLinks( ESet setIndex, const Node* nodes, TContainer& links, Bool recurse ) const
		{
			const LinkSet& set = m_links[ setIndex ];

			for( TIndexType i = 0; i < set.m_count; ++i )
			{
				links.Insert( set.m_links[ i ] );

				if( recurse )
				{
					nodes[ set.m_links[ i ] ].GetLinks( setIndex, nodes, links, true );
				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename TContainer >
		RED_INLINE void CResourceGraph::Node::GetChildren( const Node* nodes, TContainer& children ) const
		{
			GetLinks( Set_Children, nodes, children );
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename TContainer >
		RED_INLINE void CResourceGraph::Node::GetParents( const Node* nodes, TContainer& parents ) const
		{
			GetLinks( Set_Parents, nodes, parents );
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename TContainer >
		RED_INLINE void CResourceGraph::Node::GetDescendants( const Node* nodes, TContainer& descendants ) const
		{
			GetLinks( Set_Children, nodes, descendants, true );
		}

		//////////////////////////////////////////////////////////////////////////
		template< typename TContainer >
		RED_INLINE void CResourceGraph::Node::GetAncestors( const Node* nodes, TContainer& ancestors ) const
		{
			GetLinks( Set_Parents, nodes, ancestors, true );
		}


		//////////////////////////////////////////////////////////////////////////
		// CResourceGraphNode
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceGraph::CResourceGraph()
		:	m_serialisedLinks( nullptr )
		{
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE CResourceGraph::~CResourceGraph()
		{
			ReleaseMemory();
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns an array of all the immediate dependencies for a node (direct children)
		template< typename TContainer >
		RED_INLINE void CResourceGraph::GetChildren( TIndexType parent, TContainer& results ) const
		{
			RED_ASSERT( parent < m_nodes.Size(), TXT( "Root node index is bad!" ) );

			m_nodes[ parent ].GetChildren( m_nodes.TypedData(), results );
		}

		//////////////////////////////////////////////////////////////////////////
		// Returns an array of all the dependencies for a node
		template< typename TContainer >
		RED_INLINE void CResourceGraph::GetDescendents( TIndexType ancestor, TContainer& results ) const
		{
			RED_ASSERT( ancestor < m_nodes.Size(), TXT( "Root node index is bad!" ) );

			m_nodes[ ancestor ].GetDescendants( m_nodes.TypedData(), results );
		}

		//////////////////////////////////////////////////////////////////////////
		RED_INLINE System::Uint32 CResourceGraph::GetChildCount( TIndexType parent ) const
		{
			if( parent < m_nodes.Size() )
			{
				return m_nodes[ parent ].GetChildCount();
			}

			return 0;
		}
	}
}
