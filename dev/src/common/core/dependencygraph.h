/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DEPENDENCY_GRAPH_H_
#define _RED_RESOURCE_DEPENDENCY_GRAPH_H_

#include "sortedset.h"

// This Define determines whether the dependency graphs can be edited
#ifndef NO_EDITOR
	#define ENABLE_DEPENDENCY_GRAPH_EDITING
#endif

namespace Red
{
	namespace Core
	{
		class CResourceGraph
		{
		public:
			typedef System::Uint32 TIndexType;

			static const TIndexType c_InvalidNodeIndex = static_cast< TIndexType >( -1 );

		private:
			// This class represents a node in the dependency graph. Not visible outside the graph
			class Node
			{
			private:
				struct LinkSet
				{
					static const System::Uint32 c_GrowthMultiplier = 2;

					LinkSet();

#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING
					System::Bool Add( TIndexType link );
#endif
					System::Bool Has( TIndexType link ) const;

					void Release( Bool freeMemory );

					TIndexType	m_count;	// Number of links for this node
					TIndexType	m_max;		// Size of the array (in links)
					TIndexType* m_links;	// Array of links
				};

				enum ESet
				{
					Set_Children = 0,
					Set_Parents,

					Set_Max
				};

				Bool IsLinked( ESet setIndex, const Node* nodes, TIndexType linkIndex ) const;

				template< typename TContainer >
				void GetLinks( ESet setIndex, const Node* nodes, TContainer& links, Bool recurse = false ) const;

			public:
				Node();
				Node( TIndexType index );
				~Node();

#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING
				void LinkTo( Node& child );
				void LinkToUnique( Node& child );
#endif
				//////////////////////////////////////////////////////////////////////////
				// Relationship accessors
				Bool IsLinked( const Node* nodes, TIndexType linkIndex ) const;
				Bool IsDescendant( const Node* nodes, TIndexType descendantIndex ) const;
				Bool IsAncestor( const Node* nodes, TIndexType ancestorIndex ) const;
				Bool HasCircularDependency( const Node* nodes ) const;

				Uint32 GetChildCount() const;
				Uint32 GetLinkCount() const;

				//////////////////////////////////////////////////////////////////////////
				// Connection accessors
				template< typename TContainer > void GetChildren( const Node* nodes, TContainer& children ) const;
				template< typename TContainer > void GetParents( const Node* nodes, TContainer& parents ) const;
				template< typename TContainer > void GetDescendants( const Node* nodes, TContainer& descendants ) const;
				template< typename TContainer > void GetAncestors( const Node* nodes, TContainer& ancestors ) const;

				//////////////////////////////////////////////////////////////////////////
				// Serialisation
				void SerialiseCounts( IFile& file );
				void WriteLinksToFile( IFile& file ) const;
				void SetBuffer( void* buffer );
				void Release( Bool freeMemory );

			private:
				LinkSet m_links[ Set_Max ];
				TIndexType	m_index;
			};

		public:
			CResourceGraph();
			~CResourceGraph();

#ifdef ENABLE_DEPENDENCY_GRAPH_EDITING
			// Interface for adding resource entries to the graph during runtime (returns the index of the new node)
			TIndexType AddNewNode();

			// Interface for adding dependencies between nodes
			void AddLink( TIndexType parent, TIndexType child );
#endif

			// Returns an array of all the immediate dependencies for a node (direct children)
			template< typename TContainer >
			void GetChildren( TIndexType parent, TContainer& children ) const;

			// Returns an array of all the dependencies for a node
			template< typename TContainer >
			void GetDescendents( TIndexType parent, TContainer& decendents ) const;

			// Returns number of immediate dependencies
			System::Uint32 GetChildCount( TIndexType parent ) const;

			// Removes all records and frees any memory allocate
			void ReleaseMemory();

			// Serialisation
			void Serialise( IFile& theFile );

			DECLARE_CLASS_MEMORY_ALLOCATOR( MC_ResourceDatabase );

		private:
			void WriteToFile( IFile& theFile );
			void ReadFromFile( IFile& theFile );

			// A flat list of all the graph nodes (no empty slots!)
			TDynArray< Node, MC_ResourceDatabase > m_nodes;	
			
			// If the object has been serialised in the game, then we don't allocate / free individual node children, just this list (no editing)
			TIndexType* m_serialisedLinks;
		};
	}
}

#include "dependencygraph.inl"

#endif // _RED_RESOURCE_DEPENDENCY_GRAPH_H_
