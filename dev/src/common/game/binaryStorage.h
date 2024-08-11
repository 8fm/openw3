/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../redSystem/threads.h"

// Enables custom quadtree asserts even in Release
//#define QUAD_TREE_PARANOID_THREAD_SAFETY_ENABLED

#if defined( RED_ASSERTS_ENABLED )
	#define QUAD_TREE_THREAD_SAFETY_ENABLED
#endif

#if defined( QUAD_TREE_PARANOID_THREAD_SAFETY_ENABLED )
	extern void CQuadTreeStorage_ThreadSafetyAssert();
	#define QUAD_TREE_THREAD_SAFETY_ASSERT( condition ) { if ( !( condition ) ) { CQuadTreeStorage_ThreadSafetyAssert(); } }
#elif defined( QUAD_TREE_THREAD_SAFETY_ENABLED )
	#define QUAD_TREE_THREAD_SAFETY_ASSERT( condition ) RED_ASSERTS_ENABLED( condition )
#else
	#define QUAD_TREE_THREAD_SAFETY_ASSERT( condition )
#endif

// An interface used to filter out the entities
class INodeFilter
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Gameplay );

public:
	virtual Bool IsFilterConditionFulfilled( const CNode* node ) const = 0;
};

// Pointer wrapper
template< typename T >
struct TPointerWrapper
{
	T*	m_ptr;

	RED_INLINE TPointerWrapper( T* ptr )
		: m_ptr( ptr )
	{
	}

	RED_INLINE T* Get() const { return m_ptr; }

	RED_INLINE Bool operator==( const TPointerWrapper< T >& other )
	{
		return m_ptr == other.m_ptr;
	}
};

// Possible results of the UpdatePosition() function in CQuadTreeStorage
enum EQuadTreeStorageUpdateResult
{
	EQuadTreeStorageUpdateResult_NotFound,			// Object wasn't stored in quad-tree -> nothing (apart from hashmap lookup) was done
	EQuadTreeStorageUpdateResult_BoxUnchanged,		// Bounding box of the object didn't change -> quad-tree update was entirely skipped
	EQuadTreeStorageUpdateResult_NodeUnchanged,		// Bounding box of the object changed but the new bounding box still fits in the same quad-tree node -> quad-tree update was entirely skipped
	EQuadTreeStorageUpdateResult_Updated			// Bounding box changed much enough that the object had to be moved in quad-tree
};

/**
 *	Quad-tree based storage for entities.
 *
 *	Notes on implementation:
 *	- nodes only get created when necessary (i.e. the number of entries in parent exceeds certain threshold)
 *	- empty nodes are automatically removed
 *	- entries that can't be inserted into any child node are stored in parent node
 *	- only uses 2 memory allocations for all of its internal data (nodes and entries)
 */
template< typename T, typename TElement >
class CQuadTreeStorage
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_AI );
public:
	// Entry within node
	struct Entry
	{
		TElement m_element;			// The actual stored element
		Box m_box;					// Stored element box
		Uint32 m_nextEntryId;		// Id of the next entry on the list

		RED_FORCE_INLINE Entry( const TElement& element )
			: m_element( element )
		{}
	};

	typedef THashMap< T*, Uint32 >	Nodes;

	struct DefaultFunctor : public Red::System::NonCopyable
	{
		enum { SORT_OUTPUT = true };

		RED_INLINE Bool operator()( const TElement& element )				{ ASSUME( false ); }
	};

protected:

	enum : Uint32 { INVALID_INDEX = 0xFFFFFFFFU };			// Invalid index
	static const Uint32 MAX_TREE_DEPTH		= 32;			// Max tree depth
	static const Uint32 NUM_NODE_CHILDREN	= 4;			// (Maximum) number of child nodes for each node

	// Tree node
	struct Node
	{
		Uint32 m_childrenIds[ NUM_NODE_CHILDREN ];		// Ids of the child nodes
		Uint32 m_firstLockedEntryId;	// Locked entries are the ones that can't be fully inserted into any of the node's children; they're on separate list to avoid repetitive splitting (since there's no point in splitting them - they all would fail)
		Uint32 m_firstEntryId;			// Regular entries stored in node
		Uint32 m_numEntries;			// Number of entries

		RED_FORCE_INLINE Node()
		{
			m_childrenIds[ 0 ] = INVALID_INDEX;
			m_childrenIds[ 1 ] = INVALID_INDEX;
			m_childrenIds[ 2 ] = INVALID_INDEX;
			m_childrenIds[ 3 ] = INVALID_INDEX;
			m_firstLockedEntryId = INVALID_INDEX;
			m_firstEntryId = INVALID_INDEX;
			m_numEntries = 0;
		}

		RED_FORCE_INLINE Uint32 HasChildren() const
		{
			return
				m_childrenIds[ 0 ] != INVALID_INDEX ||
				m_childrenIds[ 1 ] != INVALID_INDEX ||
				m_childrenIds[ 2 ] != INVALID_INDEX ||
				m_childrenIds[ 3 ] != INVALID_INDEX;
		}
	};

	Nodes	m_nodeToEntry;			// Maps nodes to their entries

	Box		m_rootBox;				// Root area bounding box
	Uint32	m_rootId;				// Root node id
	CPool	m_nodes;				// Pool of nodes
	CPool	m_entries;				// Pool of entries
	Uint32	m_maxEntriesPerNode;	// Max number of entries stored in single node; the actual number might be greater if (a) reached max tree depth or (b) entries overlap with splitting axis
	Uint32	m_maxDepth;				// Max tree depth

	struct CollectOutputFunctor : public DefaultFunctor
	{
		CollectOutputFunctor( TDynArray< TElement >& output, Int32 maxElems )
			: m_output( output )
			, m_maxElems( maxElems )
		{}

		enum { SORT_OUTPUT = true };

		RED_INLINE Bool operator()( const TElement& element )
		{
			m_output.PushBack( element );
			return (--m_maxElems) > 0;
		}

		TDynArray< TElement >&					m_output;
		Int32									m_maxElems;
	};

public:
	CQuadTreeStorage(
		const Box& boundingBox = Box( Vector( -16384.f, -16384.f, -16384.f) , Vector( 16384.f, 16384.f, 16384.f ) ),
		Uint32 maxDepth = MAX_TREE_DEPTH,
		Uint32 maxEntriesPerNode = 8 );
	virtual ~CQuadTreeStorage();

	// Gets the number of elements in the storage
	RED_FORCE_INLINE Uint32 Size() const { return m_nodeToEntry.Size(); }
	// Removes all elements; doesn't reallocate memory
	void ClearFast();
	// Gets nodes
	RED_FORCE_INLINE const Nodes& GetNodes() { return m_nodeToEntry; }

	// Adds new element to the storage
	void Add( const TElement& elem );
	// Removes an element from the storage.
	void Remove( const TElement& elem );
	// Updates position of specific node
	EQuadTreeStorageUpdateResult UpdatePosition( T* node );
	// Updates position of all stored nodes
	void UpdatePositionForAllNodes();

	// Finds nodes closest to the selected actor.
	//
	// @param aabb  an axis-aligned box relative
	// to the selected node, over which the query will be executed,
	//
	// @param maxElems	maximum number of actors we want to find
	template < class Functor >
	void TQuery( const T&				element, 
		Functor&						functor, 
		const Box&						aabb,
		Bool							useZBounds,
		const INodeFilter**				filters,
		Uint32							numFilters );

	void Query( const T&				element, 
		TDynArray< TElement >&			output, 
		const Box&						aabb,
		Bool							useZBounds,
		Uint32							maxElems,
		const INodeFilter**				filters,
		Uint32							numFilters )						{ CollectOutputFunctor functor( output, maxElems ); TQuery( element, functor, aabb, useZBounds, filters, numFilters ); }


	// Finds nodes closest to the selected position.
	//
	// @param aabb	an axis-aligned box relative
	// to the selected position, over which the query will be executed,
	//
	// @param maxElems	maximum number of actors we want to find
	template < class Functor >
	void TQuery( const Vector&			position, 
		Functor&						functor, 
		const Box&						aabb,
		Bool							useZBounds, 	
		const INodeFilter**				filters,
		Uint32							numFilters );

	void Query( const Vector&			position, 
		TDynArray< TElement >&			output, 
		const Box&						aabb,
		Bool							useZBounds, 	
		Uint32							maxElems,
		const INodeFilter**				filters,
		Uint32							numFilters )						{ CollectOutputFunctor functor( output, maxElems ); TQuery( position, functor, aabb, useZBounds, filters, numFilters ); }

	// Find all elements passing given filters
	template < class Functor >
	void TQuery( Functor&				functor, 
		const INodeFilter**				filters,
		Uint32							numFilters );

	void Query( TDynArray< TElement >&	output, 
		Uint32							maxElems,
		const INodeFilter**				filters,
		Uint32							numFilters )						{ CollectOutputFunctor functor( output, maxElems ); TQuery( functor, filters, numFilters ); }

	// Find all elements for specified search params
	template < class Functor, class SearchParams >
	void TQuery( const T&				elem,
		Functor&						functor,
		const SearchParams&				params );

	template < class Functor, class SearchParams >
	void TQuery( const Vector&			position,
		Functor&						functor,
		const SearchParams&				params );

	template < class Functor, class SearchParams  >
	void TQuery( Functor&				functor,
		const SearchParams&				params );

	// Gets elements in given (absolute) bounding box
	template < class Functor >
	void TQuery(
		Functor&						functor, 
		const Box&						aabb,
		Bool							useZBounds, 
		const INodeFilter**				filters,
		Uint32							numFilters );

	// PS4 FIXME!
	//void Debug_DumpTree( Node* node = nullptr, Box box = Box(), Uint32 depth = 0 );

	void ReserveEntry( Uint32 elementCount )
	{
		const Uint32 oldCapacity = m_entries.Capacity();
		if( oldCapacity < elementCount )
		{
			void* oldMemory = m_entries.GetMemory();

			void* newMemory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_AI, sizeof( Entry ) * elementCount );

			m_entries.Upsize< Entry >( newMemory, elementCount );

			if ( oldMemory )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_AI, oldMemory );
			}
		}	
	}

	void ReserveNode( Uint32 nodeCount )
	{
		const Uint32 oldCapacity = m_nodes.Capacity();
		if( oldCapacity < nodeCount )
		{
			void* oldMemory = m_nodes.GetMemory();

			void* newMemory = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_AI, sizeof( Node ) * nodeCount );

			m_nodes.Upsize< Node >( newMemory, nodeCount );

			if ( oldMemory )
			{
				RED_MEMORY_FREE( MemoryPool_Default, MC_AI, oldMemory );
			}
		}
	}

	void ReserveNodeToEntry( Uint32 count )
	{
		m_nodeToEntry.Reserve( count );
	}

protected:
	RED_FORCE_INLINE Node* GetNode( Uint32 index )
	{
		return ( Node* ) m_nodes.GetBlock( index );
	}
	RED_FORCE_INLINE Entry* GetEntry( Uint32 index )
	{
		return ( Entry* ) m_entries.GetBlock( index );
	}

private:
	void Add( const TElement& elem, const Box& box );
	Bool RemoveIfNotContained( const TElement& elem, const Box* boxToCheck = nullptr );

	RED_INLINE Uint32 CreateNode( Node** nodeOut = nullptr )
	{
		// Resize node pool if needed

		if ( !m_nodes.HasFreeBlocks() )
		{
			const Uint32 oldCapacity = m_nodes.Capacity();
			const Uint32 newCapacity = !oldCapacity ? 1 : oldCapacity + ( ( oldCapacity + 1 ) / 2 );
			ReserveNode( newCapacity );
		}

		// Create node
		
		const Uint32 index = m_nodes.AllocateBlockIndex();
		Node* node = new( m_nodes.GetBlock( index ) ) Node();
		if ( nodeOut )
		{
			*nodeOut = node;
		}

		return index;
	}

	RED_INLINE Uint32 CreateEntry( Entry*& entry, const TElement& element, const Box& box )
	{
		// Resize entry pool if needed

		if ( !m_entries.HasFreeBlocks() )
		{
			const Uint32 oldCapacity = m_entries.Capacity();
			const Uint32 newCapacity = !oldCapacity ? 1 : oldCapacity + ( ( oldCapacity + 1 ) / 2 );
			ReserveEntry( newCapacity );
		}

		// Create entry

		const Uint32 index = m_entries.AllocateBlockIndex();
		entry = new( m_entries.GetBlock( index ) ) Entry( element );
		entry->m_box = box;

		return index;
	}

	// Gets quad tree node's child box at given index
	static RED_FORCE_INLINE void GetChildBox( Box& childBox, const Box& parentBox, const Vector& parentBoxCenter, Uint32 childIndex )
	{
		childBox.Min = Vector(
			( childIndex & 1 ) ? parentBoxCenter.X : parentBox.Min.X,
			( childIndex & 2 ) ? parentBoxCenter.Y : parentBox.Min.Y,
			parentBox.Min.Z );

		childBox.Max = Vector(
			( childIndex & 1 ) ? parentBox.Max.X : parentBoxCenter.X,
			( childIndex & 2 ) ? parentBox.Max.Y : parentBoxCenter.Y,
			parentBox.Max.Z );
	}

	// Gets index of the quad tree node's child that fully contains given box
	static RED_FORCE_INLINE Uint32 GetContainingChildIndex( const Box& box, const Vector& parentBoxCenter )
	{
		return
			( ( box.Min.X <= parentBoxCenter.X && parentBoxCenter.X <= box.Max.X ) || ( box.Min.Y <= parentBoxCenter.Y && parentBoxCenter.Y <= box.Max.Y ) ) ?	// Does box intersect center in X or Y?
				INVALID_INDEX :																																// No child node contains this box
				( ( box.Min.X > parentBoxCenter.X ) ? 1 : 0 ) | ( ( box.Min.Y > parentBoxCenter.Y ) ? 2 : 0 );												// Figure out child node index
	}

	// Does the parent node contain (exlusively i.e. excluding edges) given box?
	static RED_FORCE_INLINE Bool ContainsExclusive( const Box& box, const Box& nodeBox )
	{
		return
			nodeBox.Min.X < box.Min.X && box.Max.X < nodeBox.Max.X &&
			nodeBox.Min.Y < box.Min.Y && box.Max.Y < nodeBox.Max.Y;
	}

	static RED_FORCE_INLINE Bool BoxEquals2D( const Box& a, const Box& b )
	{
		return
			a.Min.X == b.Min.X && a.Min.Y == b.Min.Y &&
			a.Max.X == b.Max.X && a.Max.Y == b.Max.Y;
	}

	struct RemoveStackElement
	{
		Uint32 m_nodeId;
		Uint32 m_childIndex;

		RED_FORCE_INLINE RemoveStackElement( Uint32 nodeId, Uint32 childIndex )
			: m_nodeId( nodeId )
			, m_childIndex( childIndex )
		{}
	};

	struct SearchStackElement
	{
		Node* m_node;
		Uint32 m_processedChildNodeIndex;
		Box m_box;
		Vector m_boxCenter;

		SearchStackElement( Node* node, const Box& box )
			: m_node( node )
			, m_processedChildNodeIndex( ( Uint32 ) -1 )
			, m_box( box )
			, m_boxCenter( box.CalcCenter() )
		{}
	};

	struct Candidate
	{
		Candidate()
			: m_element( nullptr )
			, m_distSquared( 0.0f )
		{}

		Candidate( const Entry* entry, const Vector& center )
			: m_element( &entry->m_element )
			, m_distSquared( entry->m_box.CalcCenter().DistanceSquaredTo2D( center ) )
		{}

		const TElement*		m_element;
		Float				m_distSquared;

		struct DistPred
		{
			Bool operator()( const Candidate& c1, const Candidate& c2 ) const
			{
				return c1.m_distSquared < c2.m_distSquared;
			}
		};
	};

	struct EntryUpdateInfo
	{
		Box m_newBox;
		Uint32 m_entryIndex;

		EntryUpdateInfo() {}
		EntryUpdateInfo( const Box& newBox, Uint32 entryIndex )
			: m_newBox( newBox )
			, m_entryIndex( entryIndex )
		{}
	};

#ifdef QUAD_TREE_THREAD_SAFETY_ENABLED
	Bool m_enableThreadValidation;
	Red::System::Internal::ThreadId	m_ownerThreadId;		// Used for debugging only to make sure only one thread uses it at any time
public:
	void EnableThreadValidation()
	{
		m_enableThreadValidation = true;
	}
	void SetOwnerThreadId( Red::System::Internal::ThreadId threadId )
	{
		m_ownerThreadId = threadId;
	}
	Red::System::Internal::ThreadId GetOwnerThreadId() const
	{
		return m_ownerThreadId;
	}
	Bool ValidateThread() const
	{
		return !m_enableThreadValidation || m_ownerThreadId == Red::System::Internal::ThreadId::CurrentThread();
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////

template< typename T, typename TElement >
CQuadTreeStorage< T, TElement >::CQuadTreeStorage( const Box& boundingBox, Uint32 maxDepth, Uint32 maxEntriesPerNode )
{
	ASSERT( maxDepth <= MAX_TREE_DEPTH );

#ifdef QUAD_TREE_THREAD_SAFETY_ENABLED
	m_enableThreadValidation = false;
	m_ownerThreadId = Red::System::Internal::ThreadId::CurrentThread();
#endif

	m_entries.Init( NULL, 0, sizeof( Entry ) );
	m_nodes.Init( NULL, 0, sizeof( Node ) );

	m_maxEntriesPerNode = maxEntriesPerNode;
	m_maxDepth = maxDepth;

	// Initialize root

	m_rootBox = boundingBox;
	m_rootId = CreateNode();
}

template< typename T, typename TElement >
CQuadTreeStorage< T, TElement >::~CQuadTreeStorage()
{
	if ( void* ptr = m_entries.GetMemory() )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_AI, ptr );
	}

	if ( void* ptr = m_nodes.GetMemory() )
	{
		RED_MEMORY_FREE( MemoryPool_Default, MC_AI, ptr );
	}
}

template< typename T, typename TElement >
void CQuadTreeStorage< T, TElement >::ClearFast()
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	m_nodeToEntry.ClearFast();
	m_nodes.Clear();
	m_entries.Clear();

	m_rootId = CreateNode();
}

template< typename T, typename TElement >
void CQuadTreeStorage< T, TElement >::Add( const TElement& element )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	Box box;
	element.Get()->GetStorageBounds( box );

	Add( element, box );
}

template< typename T, typename TElement >
void CQuadTreeStorage< T, TElement >::Add( const TElement& element, const Box& box )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	// Early out if node is already stored

	Uint32& entryId = m_nodeToEntry.GetRef( element.Get(), INVALID_INDEX );
	if ( entryId != INVALID_INDEX )
	{
		return;
	}

	// Find node to add new entry to

	Box nodeBox = m_rootBox;
	Uint32 nodeId = m_rootId;
	Node* node;

	Bool isLocked = false;
	Uint32 depth = 0;

	while ( 1 )
	{
		node = GetNode( nodeId );
		const Vector nodeCenter = nodeBox.CalcCenter();

		// Move down all child entries if node is full

		if ( node->m_numEntries == m_maxEntriesPerNode && depth + 1 < m_maxDepth )
		{
			// Process all entries

			Uint32 entryId = node->m_firstEntryId;
			while ( entryId != INVALID_INDEX )
			{
				Entry* entry = GetEntry( entryId );
				const Uint32 nextEntryId = entry->m_nextEntryId;

				const Uint32 childIndex = GetContainingChildIndex( entry->m_box, nodeCenter );
				if ( childIndex != INVALID_INDEX )
				{
					// Get/create child node

					Node* childNode;
					if ( node->m_childrenIds[ childIndex] == INVALID_INDEX )
					{
						const Uint32 childNodeId = CreateNode( &childNode );
						node = GetNode( nodeId ); // Node pool might have been reallocated (see line above) -> refresh our node pointer
						node->m_childrenIds[ childIndex ] = childNodeId;
					}
					else
					{
						childNode = GetNode( node->m_childrenIds[ childIndex ] );
					}

					// Move into child node

					entry->m_nextEntryId = childNode->m_firstEntryId;
					childNode->m_firstEntryId = entryId;
					++childNode->m_numEntries;
				}

				// Not moved to any child? - put into locked list

				else
				{
					entry->m_nextEntryId = node->m_firstLockedEntryId;
					node->m_firstLockedEntryId = entryId;
				}

				entryId = nextEntryId;
			}

			// Clear node entry list

			node->m_firstEntryId = INVALID_INDEX;
			node->m_numEntries = 0;
		}

		// No children? - insert directly into node

		if ( !node->HasChildren() )
		{
			break;
		}

		// Check what child node to add entry to

		const Uint32 childIndex = GetContainingChildIndex( box, nodeCenter );
		if ( childIndex != INVALID_INDEX )
		{
			// Get/create child node

			if ( node->m_childrenIds[ childIndex ] == INVALID_INDEX )
			{
				const Uint32 childNodeId = CreateNode();
				node = GetNode( nodeId ); // Node pool might have been reallocated (see line above) -> refresh our node pointer
				node->m_childrenIds[ childIndex ] = childNodeId;
			}

			// Recurse into child

			Box childNodeBox;
			GetChildBox( childNodeBox, nodeBox, nodeCenter, childIndex );

			nodeBox = childNodeBox;
			nodeId = node->m_childrenIds[ childIndex ];
		}

		// Failed to find fully containing child? - put into locked entry list

		else
		{
			isLocked = true;
			break;
		}

		++depth;
	}

	// Create new entry

	Entry* entry;
	entryId = CreateEntry( entry, element, box );

	// Add new entry to node

	Uint32* entryListId;
	if ( !isLocked )
	{
		entryListId = &node->m_firstEntryId;
		++node->m_numEntries;
	}
	else
	{
		entryListId = &node->m_firstLockedEntryId;
	}
	entry->m_nextEntryId = *entryListId;
	*entryListId = entryId;
}

template< typename T, typename TElement >
void CQuadTreeStorage< T, TElement >::Remove( const TElement& elem )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	RemoveIfNotContained( elem, nullptr );
}

template< typename T, typename TElement >
Bool CQuadTreeStorage< T, TElement >::RemoveIfNotContained( const TElement& elem, const Box* boxToCheck )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	// Find node in global map

	auto entryIdIt = m_nodeToEntry.Find( elem.Get() );
	if ( entryIdIt == m_nodeToEntry.End() )
	{
		return true;
	}
	const Uint32 entryId = entryIdIt->m_second;

	// Determine entry for node

	Entry* entry = GetEntry( entryId );

	// Initialize removal stack

	TStaticArray< RemoveStackElement, MAX_TREE_DEPTH > stack;
	stack.PushBack( RemoveStackElement( m_rootId, INVALID_INDEX ) );

	// Start searching from root node

	Box nodeBox = m_rootBox;
	Uint32 nodeId = m_rootId;

	while ( 1 )
	{
		Node* node = GetNode( nodeId );
		const Vector nodeCenter = nodeBox.CalcCenter();

		// First check entries in node (if any)

		for ( Uint32 entrySetIndex = 0; entrySetIndex < 2; entrySetIndex++ )
		{
			Uint32* currEntryId = entrySetIndex ? &node->m_firstEntryId : &node->m_firstLockedEntryId;
			while ( *currEntryId != INVALID_INDEX )
			{
				Entry* currEntry = GetEntry( *currEntryId );
				if ( *currEntryId == entryId )
				{
					if ( boxToCheck && ContainsExclusive( *boxToCheck, nodeBox ) )
					{
						return false;
					}

					// Remove from global map

					m_nodeToEntry.Erase( entryIdIt );

					// Destroy entry

					currEntry->m_element.~TElement();
					*currEntryId = currEntry->m_nextEntryId;
					m_entries.FreeBlock( currEntry );

					// Update node

					if ( entrySetIndex )
					{
						--node->m_numEntries;
					}

					// Remove empty nodes all the way up the root

					while ( !node->m_numEntries && !node->HasChildren() && stack.Size() >= 2 && node->m_firstLockedEntryId == INVALID_INDEX )
					{
						const RemoveStackElement stackElement = stack.PopBackFast();

						const Uint32 parentNodeId = stack.Back().m_nodeId;
						Node* parentNode = GetNode( parentNodeId );
						parentNode->m_childrenIds[ stackElement.m_childIndex ] = INVALID_INDEX;

						m_nodes.FreeBlock( node );

						nodeId = parentNodeId;
					}

					return true;
				}
				else
				{
					currEntryId = &currEntry->m_nextEntryId;
				}
			}
		}

		// Find child node containing entry

		const Uint32 childIndex = GetContainingChildIndex( entry->m_box, nodeCenter );
		if ( childIndex != INVALID_INDEX )
		{
			const Uint32 childId = node->m_childrenIds[ childIndex ];
			if ( childId == INVALID_INDEX )
			{
				continue;
			}

			// Recurse into child node

			Box childNodeBox;
			GetChildBox( childNodeBox, nodeBox, nodeCenter, childIndex );

			nodeBox = childNodeBox;
			nodeId = childId;

			stack.PushBack( RemoveStackElement( nodeId, childIndex ) );
		}
	}

	// Not found in the tree but found in a map - should never happen

	m_nodeToEntry.Erase( entryIdIt );	// Remove from global map

	return true;
}

template< typename T, typename TElement >
EQuadTreeStorageUpdateResult CQuadTreeStorage< T, TElement >::UpdatePosition( T* node )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	if ( const Uint32* entryIndex = m_nodeToEntry.FindPtr( node ) )
	{
		Entry* entry = GetEntry( *entryIndex );

		Box newBox;
		entry->m_element.Get()->GetStorageBounds( newBox );
		if ( BoxEquals2D( entry->m_box, newBox ) )
		{
			entry->m_box = newBox; // Might still differ in Z
			return EQuadTreeStorageUpdateResult_BoxUnchanged;
		}

		TElement element = entry->m_element;
		if ( RemoveIfNotContained( element, &newBox ) )
		{
			Add( element, newBox );
			return EQuadTreeStorageUpdateResult_Updated;
		}

		entry->m_box = newBox;
		return EQuadTreeStorageUpdateResult_NodeUnchanged;
	}

	return EQuadTreeStorageUpdateResult_NotFound;
}

template< typename T, typename TElement >
void CQuadTreeStorage< T, TElement >::UpdatePositionForAllNodes()
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	// Collect entries to update

	TDynArray< EntryUpdateInfo > entriesToUpdate;
	for ( auto it = m_nodeToEntry.Begin(), end = m_nodeToEntry.End(); it != end; ++it )
	{
		Entry* entry = GetEntry( it->m_second );

		Box newBox;
		entry->m_element.Get()->GetStorageBounds( newBox );
		if ( BoxEquals2D( entry->m_box, newBox ) )
		{
			entry->m_box = newBox; // Might still differ in Z
			continue;
		}

		entriesToUpdate.PushBack( EntryUpdateInfo( newBox, it->m_second ) );
	}

	for ( const EntryUpdateInfo& info : entriesToUpdate )
	{
		Entry* entry = GetEntry( info.m_entryIndex );

		TElement element = entry->m_element;
		if ( RemoveIfNotContained( element, &info.m_newBox ) )
		{
			Add( element, info.m_newBox );
		}
		else
		{
			entry->m_box = info.m_newBox;
		}
	}
}

template< typename T, typename TElement >
template< class Functor >
void CQuadTreeStorage< T, TElement >::TQuery( const T& node, 
										   Functor& functor, 
										   const Box& aabb,
										   Bool useZBounds, 						  
										   const INodeFilter** filters,
										   Uint32 numFilters )
{
	const Box newAABB( aabb.Min + node.GetWorldPositionRef(), aabb.Max + node.GetWorldPositionRef() );
	TQuery( functor, newAABB, useZBounds, filters, numFilters );
}

template< typename T, typename TElement >
template< class Functor >
void CQuadTreeStorage< T, TElement >::TQuery( const Vector& position, 
										   Functor& functor, 
										   const Box& aabb,
										   Bool useZBounds, 					
										   const INodeFilter** filters,
										   Uint32 numFilters )
{
	const Box newAABB( aabb.Min + position, aabb.Max + position );
	TQuery( functor, newAABB, useZBounds, filters, numFilters );
}

template< typename T, typename TElement >
template < class Functor >
void CQuadTreeStorage< T, TElement >::TQuery(
	Functor& functor, 
	const Box& aabb,
	Bool useZBounds, 
	const INodeFilter** filters,
	Uint32 numFilters )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	// Optional result candidates (only used if output is to be sorted)

	TStaticArray< Candidate, 1024 > candidates;

	// Initialize search stack

	TStaticArray< SearchStackElement, MAX_TREE_DEPTH > stack;
	stack.PushBack( SearchStackElement( GetNode( m_rootId ), m_rootBox ) );

	// Search recursively

	const Vector center = aabb.CalcCenter();

	while ( !stack.Empty() )
	{
		SearchStackElement& element = stack.Back();
		Node* node = element.m_node;

		// Process entries linked to this node

		if ( element.m_processedChildNodeIndex == ( Uint32 ) -1 ) // Only do this once (i.e. when no child nodes have been processed before)
		{
			for ( Uint32 entrySetIndex = 0; entrySetIndex < 2; entrySetIndex++ )
			{
				Uint32 entryId = entrySetIndex ? node->m_firstEntryId : node->m_firstLockedEntryId;
				while ( entryId != INVALID_INDEX )
				{
					Entry* entry = GetEntry( entryId );
					const T* node;

					// Process entry

					const Bool intersects = useZBounds ? aabb.Touches( entry->m_box ) : entry->m_box.Touches2D( aabb );
					if ( !intersects )
					{
						goto SkipEntry;
					}

					node = entry->m_element.Get();
					for ( Uint32 i = 0; i < numFilters; ++i )
					{
						if ( !filters[ i ]->IsFilterConditionFulfilled( node ) )
						{
							goto SkipEntry;
						}
					}
					if ( Functor::SORT_OUTPUT )
					{
						if ( candidates.Size() < candidates.Capacity() )
						{
							candidates.PushBack( Candidate( entry, center ) );
						}
					}
					else
					{
						if ( !functor( entry->m_element ) )
						{
							return;
						}
					}

					// Go to next element
SkipEntry:

					entryId = entry->m_nextEntryId;
				}
			}
		}

		// Process child nodes

		while ( 1 )
		{
			++element.m_processedChildNodeIndex;

			// Processed all children? - return to parent node

			if ( element.m_processedChildNodeIndex >= NUM_NODE_CHILDREN )
			{
				stack.PopBackFast();
				break;
			}

			// Check if child node exists

			const Uint32 childIndex = element.m_processedChildNodeIndex;
			const Uint32 childId = node->m_childrenIds[ childIndex ];
			if ( childId == INVALID_INDEX )
			{
				continue;
			}

			// Check intersection with child node box

			Box childBox;
			GetChildBox( childBox, element.m_box, element.m_boxCenter, childIndex );
			if ( !childBox.Touches2D( aabb ) )
			{
				continue;
			}

			// Recurse into child

			Node* childNode = GetNode( childId );
			stack.PushBack( SearchStackElement( childNode, childBox ) );
			break;
		}
	}

	// Handle sorted output request

	if ( Functor::SORT_OUTPUT )
	{
		::Sort( candidates.Begin(), candidates.End(), typename Candidate::DistPred() );

		for( Uint32 i = 0, n = candidates.Size(); i < n; ++i )
		{
			if ( !functor( *candidates[i].m_element ) )
			{
				break;
			}
		}
	}
}

template< typename T, typename TElement >
template< class Functor >
void CQuadTreeStorage< T, TElement >::TQuery(	Functor& functor, 
										   const INodeFilter** filters,
										   Uint32 numFilters )
{
	QUAD_TREE_THREAD_SAFETY_ASSERT( ValidateThread() );

	for ( auto it = m_nodeToEntry.Begin(), end = m_nodeToEntry.End(); it != end; ++it )
	{
		const Entry* entry;
		for ( Uint32 i = 0; i < numFilters; ++i )
		{
			if ( !filters[ i ]->IsFilterConditionFulfilled( it->m_first ) )
			{
				goto SkipToNext;
			}
		}
		entry = GetEntry( it->m_second );
		if ( !functor( entry->m_element ) )
		{
			break;
		}
SkipToNext:
		;
	}
}

template< typename T, typename TElement >
template< class Functor, class SearchParams >
void CQuadTreeStorage< T, TElement >::TQuery( const T& elem, Functor& functor, const SearchParams& params )
{
	TQuery( elem,
		functor,
		params.GetTestBox(),
		params.ShouldUseZBounds(),
		params.GetFilters(),
		params.GetFiltersCount() );	

}

template< typename T, typename TElement >
template< class Functor, class SearchParams >
void CQuadTreeStorage< T, TElement >::TQuery( const Vector& position, Functor& functor, const SearchParams& params )
{
	TQuery( position,
		functor,
		params.GetTestBox(),
		params.ShouldUseZBounds(),
		params.GetFilters(),
		params.GetFiltersCount() );	

}

template< typename T, typename TElement >
template< class Functor, class SearchParams >
void CQuadTreeStorage< T, TElement >::TQuery( Functor& functor, const SearchParams& params )
{
	TQuery( params.GetOrigin(),
		functor,
		params.GetTestBox(),
		params.ShouldUseZBounds(),
		params.GetFilters(),
		params.GetFiltersCount() );	
}

///////////////////////////////////////////////////////////////////////////////

// PS4 FIXME!
// template< typename T, typename TElement >
// void CQuadTreeStorage< T, TElement >::Debug_DumpTree( Node* node, Box box, Uint32 depth )
// {
// 	if ( !node )
// 	{
// 		RED_LOG( BinaryStorage, TXT("=========== TREE ============") );
// 		node = &m_nodes.GetBlock< Node >( m_rootId );
// 		box = m_rootBox;
// 	}
// 
// 	{
// 		Char indent[ MAX_TREE_DEPTH + 1 ];
// 		for ( Uint32 i = 0; i < depth; ++i )
// 		{
// 			indent[ i ] = TXT(' ');
// 		}
// 		indent[ depth ] = 0;
// 
// 		RED_LOG( BinaryStorage, TXT("%sNODE: %f %f -> %f %f"), indent, box.Min.X, box.Min.Y, box.Max.X, box.Max.Y);
// 
// 		for ( Uint32 entrySetIndex = 0; entrySetIndex < 2; entrySetIndex++ )
// 		{
// 			Uint32 entryId = entrySetIndex ? node->m_firstEntryId : node->m_firstLockedEntryId;
// 			if ( entryId != INVALID_INDEX )
// 			{
// 				const Char* entrySetName = entrySetIndex ? TXT("") : TXT(" L");
// 
// 				while ( entryId != INVALID_INDEX )
// 				{
// 					Entry& entry = m_entries.GetBlock< Entry >( entryId );
// 
// 					RED_LOG( BinaryStorage, TXT("%sENTRY%s: %f %f -> %f %f"), indent, entrySetName, entry.m_box.Min.X, entry.m_box.Min.Y, entry.m_box.Max.X, entry.m_box.Max.Y);
// 
// 					entryId = entry.m_nextEntryId;
// 				}
// 			}
// 		}
// 	}
// 
// 	const Vector boxCenter = box.CalcCenter();
// 	for ( Uint32 i = 0; i < 4; i++ )
// 	{
// 		const Uint32 childId = node->m_childrenIds[ i ];
// 		if ( childId == INVALID_INDEX )
// 		{
// 			continue;
// 		}
// 
// 		Box childBox;
// 		GetChildBox( childBox, box, boxCenter, i );
// 
// 		Node* childNode = &m_nodes.GetBlock< Node >( childId );
// 		Debug_DumpTree( childNode, childBox, depth + 1 );
// 	}
// }
