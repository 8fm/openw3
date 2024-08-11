/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dependencygraph.h"

// Test code! Turn off in release / final builds
#define TEST_FOR_CIRCULAR_DEPENDENCIES

namespace Red
{
	namespace Core
	{
		//////////////////////////////////////////////////////////////////////////
		// CResourceGraphNode
		//////////////////////////////////////////////////////////////////////////

		////////////////////////////////////////////////////////////////////////// 
		Bool CResourceGraph::Node::IsLinked( ESet setIndex, const Node* nodes, TIndexType linkIndex ) const
		{
			const LinkSet& set = m_links[ setIndex ];

			for( TIndexType i = 0; i < set.m_count; ++i )
			{
				TIndexType currentIndex = set.m_links[ i ];
				if( currentIndex == linkIndex )
				{
					return true;
				}

				if( nodes[ currentIndex ].IsLinked( setIndex, nodes, linkIndex ) )
				{
					return true;
				}
			}

			return false;
		}

		//////////////////////////////////////////////////////////////////////////
		// CResourceGraph
		//////////////////////////////////////////////////////////////////////////

		//////////////////////////////////////////////////////////////////////////
		void CResourceGraph::ReleaseMemory()
		{
			Bool buffersOwnedByNodes = m_serialisedLinks == nullptr;
			for( TIndexType i = 0; i < m_nodes.Size(); ++i )
			{
				m_nodes[ i ].Release( buffersOwnedByNodes );
			}

			if( !buffersOwnedByNodes )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_ResourceDatabase, m_serialisedLinks );
				m_serialisedLinks = nullptr;
			}

			m_nodes.Clear();
		}

#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING	

		//////////////////////////////////////////////////////////////////////////
		// Interface for adding resource entries to the graph during runtime (returns the index of the new node)
		CResourceGraph::TIndexType CResourceGraph::AddNewNode()
		{
			RED_ASSERT( m_serialisedLinks == nullptr, TXT( "Cannot add nodes to a serialised dependency graph. Make a new graph if you want to do this" ) );
			if( m_serialisedLinks == nullptr )
			{
				TIndexType newIndex = m_nodes.Size();
				Node newNode( newIndex );
				m_nodes.PushBack( newNode );
				RED_ASSERT( newIndex < m_nodes.Size(), TXT( "Failed to add new node. This will go bang" ) );
				return newIndex;
			}
			return c_InvalidNodeIndex;
		}

		//////////////////////////////////////////////////////////////////////////
		// Interface for adding dependencies between nodes
		void CResourceGraph::AddLink( TIndexType parentIndex, TIndexType childIndex )
		{
			RED_ASSERT( m_serialisedLinks == nullptr, TXT( "Cannot add dependencies to a serialised dependency graph. Make a new graph if you want to do this" ) );
			RED_ASSERT( parentIndex < m_nodes.Size(), TXT( "Source object index is bad!" ) );
			RED_ASSERT( childIndex < m_nodes.Size(), TXT( "Dependency object index is bad!" ) );

			// If the node child list is too small, reallocate it
			Node& parentNode = m_nodes[ parentIndex ];
			Node& childNode = m_nodes[ childIndex ];

			parentNode.LinkToUnique( childNode );

#ifdef TEST_FOR_CIRCULAR_DEPENDENCIES
			RED_ASSERT( !parentNode.HasCircularDependency( m_nodes.TypedData() ), TXT( "Circular dependency detected! This is BAD!" ) );
#endif
		}

#endif	// ENABLE_DEPENDENCY_GRAPH_EDITING

		//////////////////////////////////////////////////////////////////////////
		void CResourceGraph::WriteToFile( IFile& theFile )
		{
			// How many nodes to store
			System::Uint32 nodeCount = m_nodes.Size();
			theFile << nodeCount;

			// First, calculate the final size of the child array (so it can be pre-allocated in the loading)
			System::Uint32 totalLinkCount = 0u;

			for( TIndexType i = 0; i < nodeCount; ++i )
			{
				totalLinkCount += m_nodes[ i ].GetLinkCount();
			}

			// The size of the link array
			theFile << totalLinkCount;

			// Save each node along with their corresponding child array offsets
			System::Uint32 linkArrayOffset = 0u;
			for( TIndexType i = 0; i < nodeCount; ++i )
			{
				m_nodes[ i ].SerialiseCounts( theFile );

				System::Uint32 linkCount = m_nodes[ i ].GetLinkCount();

				if( linkCount > 0 )
				{
					theFile << linkArrayOffset;
					linkArrayOffset += linkCount;
				}
			}

			// Save the children into (seemingly) one large buffer
			// If this is too slow, we can build the buffer in advance then save it with one serialise() call
			for( TIndexType i = 0; i < nodeCount; ++i )
			{
				m_nodes[ i ].WriteLinksToFile( theFile );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		void CResourceGraph::ReadFromFile( IFile& theFile )
		{
			RED_ASSERT( m_serialisedLinks == nullptr && m_nodes.Size() == 0, TXT( "You really shouldn't reload a graph directly! Leaks incoming" ) );

			System::Uint32 nodeCount = 0;
			System::Uint32 linkCount = 0;
			theFile << nodeCount;
			theFile << linkCount;

			if( linkCount == 0 )
			{
				return;
			}

			// Pre-allocate memory
			m_serialisedLinks = static_cast< TIndexType* >( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_ResourceDatabase, linkCount * sizeof( TIndexType ) ) );
			RED_ASSERT( m_serialisedLinks != NULL, TXT( "Failed to allocate child index buffer. Prepare for crashes!" ) );
			m_nodes.Resize( nodeCount );

			// Load each node. Child pointers are fixed-up on the fly
			for( TIndexType i = 0; i < nodeCount; ++i )
			{
				m_nodes[ i ].SerialiseCounts( theFile );

				if( m_nodes[ i ].GetLinkCount() > 0 )
				{
					System::Uint32 offset;
					theFile << offset;

					TIndexType* buffer = m_serialisedLinks + offset;
					m_nodes[ i ].SetBuffer( buffer );
				}
			}

			// Load the child index array in one big chunk
			theFile.Serialize( m_serialisedLinks, sizeof( TIndexType ) * linkCount );
		}

		//////////////////////////////////////////////////////////////////////////
		void CResourceGraph::Serialise( IFile& theFile )
		{
			if( theFile.IsWriter() )
			{
				WriteToFile( theFile );
			}
			else
			{
				ReadFromFile( theFile );
			}
		}
	}
}
