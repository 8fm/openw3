#pragma once

#include "spawnTreeNode.h"

template < class TList, class TChildType >
void ISpawnTreeBaseNode::TListOperations< TList, TChildType >::GetRootClassForChildren( TDynArray< CClass* >& rootClasses )
{
	rootClasses.PushBack( TChildType::GetStaticClass() );
}
template < class TList, class TChildType >
void ISpawnTreeBaseNode::TListOperations< TList, TChildType >::AddChild( TList& list, IEdSpawnTreeNode* child, Int32 index /*= -1*/ )
{
	if ( index < 0 )
	{
		list.PushBack( static_cast< TChildType* >( child ) );
	}
	else
	{
		list.Insert( index, static_cast< TChildType* >( child ) );
	}
}
template < class TList, class TChildType >
void ISpawnTreeBaseNode::TListOperations< TList, TChildType >::MoveChild( TList& list, Uint32 indexFrom, Uint32 indexTo )
{
	auto entry = list[ indexFrom ];
	list.RemoveAt( indexFrom );
	if ( indexFrom < indexTo )
	{
		--indexTo;
	}
	list.Insert( indexTo, Move( entry ) );
}

template < class TList, class TChildType >
Bool ISpawnTreeBaseNode::TListOperations< TList, TChildType >::UpdateChildrenOrder( TList& list )
{
#ifndef NO_EDITOR_GRAPH_SUPPORT
	Bool dirty = false;

	// insertion sort
	for( Uint32 i = 1, n = list.Size(); i < n; ++i )
	{
		Uint32 index = i;
		while ( list[ index ]->GetGraphPosX() < list[ index-1 ]->GetGraphPosX() )
		{
			dirty = true;
			::Swap( list[ index ], list[ index-1 ] );

			if ( index == 1 )
			{
				break;
			}
			--index;
		}
	}
	if ( dirty )
	{
		// shift locked list members 'to right'
		for( Uint32 i = 0, n = list.Size(); i < n-1; ++i )
		{
			auto lockedItem = list[ i ];
			if ( lockedItem->IsLocked() )
			{
				for ( Uint32 j = list.Size()-1; j > i; --j )
				{
					auto notLockedItem = list[ j ];
					if ( !notLockedItem->IsLocked() )
					{
						lockedItem->SetGraphPosition( notLockedItem->GetGraphPosX() + 5, lockedItem->GetGraphPosY() );
						list.RemoveAt( i );
						list.Insert( j, lockedItem );
						for ( Uint32 k = j+1; k < n; ++k )
						{
							list[ k ]->SetGraphPosition( list[ k ]->GetGraphPosX() + 5, list[ k ]->GetGraphPosY() );
						}

					}
				}
			}
		}
		return true;
	}
#endif
	return false;
}

template < class TList, class TChildType >
void ISpawnTreeBaseNode::TListOperations< TList, TChildType >::RemoveChild( TList& list, IEdSpawnTreeNode* child )
{
	for( Uint32 i = 0, n = list.Size(); i < n; ++i )
	{
		if ( list[ i ] == child )
		{
			list.RemoveAt( i );
			break;
		}
	}
}

template < class TList, class TChildType >
void ISpawnTreeBaseNode::TListOperations< TList, TChildType >::RemoveChildHandle( TList& list, IEdSpawnTreeNode* child )
{
	for( Uint32 i = 0, n = list.Size(); i < n; ++i )
	{
		if ( list[ i ].Get() == child )
		{
			list.RemoveAt( i );
			break;
		}
	}
}