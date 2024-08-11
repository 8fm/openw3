/**
 * Copyright © 2008 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "compare.h"

RED_DISABLE_WARNING_MSC( 4201 ) // nonstandard extension used : nameless struct/union

template <
		   typename K,
		   typename CompareFunc = DefaultCompareFunc< K >
		 >
class TSet
{
public:
	typedef K			key_type;
	typedef CompareFunc compare_func;

public:
	class Node
	{
	public:
		K		m_key;

		union {
			Node*	m_child[2];
#if defined(_DEBUG)
			// Do not use this struct !!!
			// It's here because autoexp'ed debugger needs those variables
			struct {
				Node* m_child0_doNotUseIt;
				Node* m_child1_doNotUseIt;
			};
#endif
		};
	public:
		Node( const K& key, Node* child1, Node* child2 )
		: m_key( key )
		{
			m_child[0] = child1;
			m_child[1] = child2;
		}

		friend IFile& operator<<( IFile& file, Node& node )
		{
			file << node.m_key;
			file << node.m_child[0];
			file << node.m_child[1];
			return file;
		}
	};

	class iterator
	{
	public:
		Node* m_root;
		Node* m_parent;
		Node* m_node;

	public:
		RED_INLINE K& operator*() const
		{
			return m_node->m_key;
		}
		RED_INLINE K* operator->() const
		{
			return &(m_node->m_key);
		}
		RED_INLINE Bool operator==( const iterator& it )
		{
			return (m_root == it.m_root) && (m_parent == it.m_parent) && (m_node == it.m_node);
		}
		RED_INLINE Bool operator!=( const iterator& it )
		{
			return (m_root != it.m_root) || (m_parent != it.m_parent) || (m_node != it.m_node);
		}

		// Pre increment
		RED_INLINE iterator operator++()
		{
			Bool exit = false;
			*this = FindNext( m_node->m_key, m_root, NULL, m_root, exit );
			return *this;
		}

		// Post increment
		RED_INLINE iterator operator++( Int32 )
		{
			iterator oldIterator = *this;
			++(*this);
			return oldIterator;
		}
	private:
		RED_INLINE iterator End()
		{
			iterator it;
			it.m_root = m_root;
			it.m_parent = NULL;
			it.m_node = NULL;
			return it;
		}
		// Returns iterator to next in-order element or End()
		inline iterator FindNext( const K& key, Node* node, Node* parent, Node* root, Bool& exit )
		{
			if( node->m_child[0] != NULL )
			{
				// We can skip whole left subtree if last key is >= then this node's key
				if( CompareFunc::Less( key, node->m_key ) )
				{
					iterator it = FindNext( key, node->m_child[0], node, root, exit );
					if ( it != End() )
					{
						return it;
					}
				}
			}
			if ( exit == true )
			{
				iterator it;
				it.m_root = root;
				it.m_parent = parent;
				it.m_node = node;
				return it;
			}
			if( !CompareFunc::Less( node->m_key, key ) && !CompareFunc::Less( key, node->m_key ) )
			{
				exit = true;
			}
			if( node->m_child[1] != NULL )
			{
				return FindNext( key, node->m_child[1], node, root, exit );
			}
			return End();
		}
	};

	Node* m_root;

public:
	RED_INLINE TSet()
	{
		m_root = NULL;
	}
	RED_INLINE TSet( const TSet& set )
	{
		m_root = NULL;
		*this = set;
	}
	RED_INLINE const TSet& operator=( const TSet& set )
	{
		DeleteSubTree( m_root );
		m_root = NULL;
		InsertSubTree( set.m_root );
		return *this;
	}
	RED_INLINE ~TSet()
	{
		DeleteSubTree( m_root );
	}

	RED_INLINE bool Empty() const
	{
		return m_root == NULL;
	}

	RED_INLINE void Clear()
	{
		DeleteSubTree( m_root );
		m_root = NULL;
	}



	// Find key
	// Returns iterator to key or End() if key is not found
	RED_INLINE iterator Find( const K& key )
	{
		iterator it;
		it.m_root = m_root;
		it.m_parent = NULL;
		it.m_node = m_root;

		while( it.m_node )
		{
			if ( CompareFunc::Less( key, it.m_node->m_key ) )
			{
				it.m_parent = it.m_node;
				it.m_node = it.m_node->m_child[0];
			}
			else if ( CompareFunc::Less( it.m_node->m_key, key ) )
			{
				it.m_parent = it.m_node;
				it.m_node = it.m_node->m_child[1];
			}
			else
			{
				return it;
			}
		}
		return End();
	}

	// Insert key only if key doesn't not exist in set already
	RED_INLINE Bool Insert( const K& key )
	{
		Node* parent = NULL;
		Node* node = m_root;
		while ( node )
		{
			if ( CompareFunc::Less( key, node->m_key ) )
			{
				parent = node;
				node = node->m_child[0];
			}
			else if ( CompareFunc::Less( node->m_key, key ) )
			{
				parent = node;
				node = node->m_child[1];
			}
			else
			{
				return false;
			}
		}

		if ( parent != NULL )
		{
			Int32 offset = ( CompareFunc::Less( key, parent->m_key ) ) ? 0 : 1;
			void* mem = RED_MEMORY_ALLOCATE( MemoryPool_SmallObjects, MC_SetNodes, sizeof( Node ) );
			parent->m_child[offset] = new ( mem ) Node( key, NULL, NULL );
		}
		else
		{
			RED_FATAL_ASSERT( m_root == NULL, "" );
			void* mem = RED_MEMORY_ALLOCATE( MemoryPool_SmallObjects, MC_SetNodes, sizeof( Node ) );
			m_root = new ( mem ) Node( key, NULL, NULL );
		}
		return true;
	}

	RED_INLINE Bool Erase( const K& key )
	{
		iterator it = Find( key );
		if( it != End() )
		{
			return Erase( it );
		}
		return false;
	}

	RED_INLINE Bool Exist( const K& key )
	{
		return Find( key ) != End();
	}

	RED_INLINE Bool Erase( iterator& it )
	{
		RED_FATAL_ASSERT( it.m_node != NULL, "" );

		if ( it.m_node->m_child[0] != NULL && it.m_node->m_child[1] != NULL )
		{
			// FindMin return (in-order successor) node with one children, so infinity recursion is not possible
			iterator it2 = FindMin( it.m_node->m_child[1], it.m_node );
			it.m_node->m_key = it2.m_node->m_key;
			return Erase( it2 );
		} 
		else if ( it.m_node->m_child[0] == NULL && it.m_node->m_child[1] == NULL )
		{
			// Node doesn't have any child so we have to modify parent node
			if ( it.m_parent != NULL )
			{
				Int32 offset = ( it.m_parent->m_child[0] == it.m_node ) ? 0 : 1;
				it.m_parent->m_child[offset] = NULL;
			}
			else
			{
				RED_FATAL_ASSERT( it.m_node == m_root, "" );
				m_root = NULL;
			}

			RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_SetNodes, it.m_node );
		}
		else
		{
			// Node has one children, so move the children data to this node and delete old children
			Node* temp = ( it.m_node->m_child[0] != NULL ) ? it.m_node->m_child[0] : it.m_node->m_child[1];
			it.m_node->m_key = temp->m_key;
			it.m_node->m_child[0] = temp->m_child[0];
			it.m_node->m_child[1] = temp->m_child[1];

			RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_SetNodes, temp );
		}
		return true;
	}

public:
	RED_INLINE Int32 Size() const
	{
		return Size( m_root );
	}
	RED_INLINE Int32 MinHeight() const
	{
		return MinHeight( m_root );
	}
	RED_INLINE Int32 MaxHeight() const
	{
		return MaxHeight( m_root );
	}

protected:
	RED_INLINE Int32 Size( Node* node ) const
	{
		return ( node != NULL ) ? ( 1 + Size( node->m_child[0] ) + Size( node->m_child[1] ) ) : 0;
	}
	RED_INLINE Int32 MinHeight( Node* node ) const
	{
		return ( ( node->m_child[0] != NULL ) && ( node->m_child[1] != NULL ) ) ?  ( 1 + Min( MinHeight( node->m_child[0] ), MinHeight( node->m_child[1] ) ) ) : 0;
	}
	RED_INLINE Int32 MaxHeight( Node* node ) const
	{
		return ( node != NULL ) ? ( 1 + Max( MaxHeight( node->m_child[0] ), MaxHeight( node->m_child[1] ) ) ) : 0;
	}

protected:
	// Insert all the nodes below the node to this tree
	RED_INLINE void InsertSubTree( const Node* node )
	{
		if( node != NULL )
		{
			Insert( node->m_key );
			InsertSubTree( node->m_child[0] );
			InsertSubTree( node->m_child[1] );
		}
	}

	// Delete all the nodes below this node from the tree
	void DeleteSubTree( Node* node )
	{
		if( node != NULL )
		{
			DeleteSubTree( node->m_child[0] );
			DeleteSubTree( node->m_child[1] );

			RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_SetNodes, node );
		}
	}
	// Find Minimum key in the tree
	RED_INLINE iterator FindMin( Node* node, Node* parent )
	{
		while( node )
		{
			if ( node->m_child[0] != NULL )
			{
				parent = node;
				node = node->m_child[0];
			}
			else
			{
				iterator it;
				it.m_root = m_root;
				it.m_parent = parent;
				it.m_node = node;
				return it;
			}
		}
		return End();
	}

public:
	// Iterators
	RED_INLINE iterator Begin()
	{
		return FindMin( m_root, NULL );
	}

	RED_INLINE iterator End()
	{
		iterator it;
		it.m_root = m_root;
		it.m_parent = NULL;
		it.m_node = NULL;
		return it;
	}

	// Serialization
	friend IFile& operator<<( IFile& file, TSet<K, CompareFunc>& set )
	{
		file << set.m_root;
		return file;
	}
};

