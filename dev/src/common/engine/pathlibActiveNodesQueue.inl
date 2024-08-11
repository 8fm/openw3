/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

namespace PathLib
{

RED_INLINE void CActiveNodesQueue::QueueSwap( Uint32 i1, Uint32 i2 )
{
	CSearchNode*& q1 = m_queue[ i1 ];
	CSearchNode*& q2 = m_queue[ i2 ];
	q1->SwapHeapIndex( q2 );
	::Swap( q1, q2 );
}

RED_INLINE Bool CActiveNodesQueue::IsHeap()
{
	for ( Uint32 index = 1, n = m_queue.Size(); index < n; ++index )
	{
		// Check if this node parent is lower
		if ( Order( m_queue[ index ], m_queue[ ( (index-1) >> 1 ) ] ) )
		{
			return false;
		}
	}
	return true;
}

RED_INLINE void CActiveNodesQueue::Push( CSearchNode* node )
{
	m_queue.PushBack( node );

	Int32 index = m_queue.Size() - 1;
	node->SetHeapIndex( index );

	while ( index > 0 )
	{
		Uint32 parentIndex = (index - 1) >> 1;
		if ( Order( m_queue[ parentIndex ], m_queue[ index ] ) )
		{
			break;
		}
		QueueSwap( parentIndex, index );
		index = parentIndex;
	}
}
RED_INLINE CSearchNode* CActiveNodesQueue::Pop()
{
	CSearchNode* bestNode = m_queue[ 0 ];
	Uint32 size = m_queue.Size();
	// pop-heap
	if ( size > 1 )
	{
		// push last element to front
		CSearchNode* currentNode = m_queue[ size-1 ];
		currentNode->SetHeapIndex( 0 );
		m_queue[ 0 ] = currentNode;
		Uint32 index = 0;
		--size;
		// roll it down
		while ( (index << 1)+1 < size )
		{
			Uint32 index1 = (index << 1) + 1;
			Uint32 index2 = (index << 1) + 2;
			// check if we are in leaf or almost-leaf node
			Uint32 nextIndex =
				( index2 < size && Order( m_queue[ index2 ], m_queue[ index1 ] ) )
				? index2
				: index1;
			if ( !Order( m_queue[ nextIndex ], m_queue[ index ] ) )
			{
				break;
			}
			QueueSwap( nextIndex, index );
			index = nextIndex;
		}
		
	}
	
	bestNode->SetHeapIndex( CSearchNode::INVALID_HEAP_INDEX );
	m_queue.PopBackFast();

	return bestNode;
}
RED_INLINE void CActiveNodesQueue::Update( CSearchNode* node )
{
	Uint32 index = node->GetHeapIndex();
	//Uint32 size = m_queue.Size();
	// Update heap
	// first test if element comes up
	if ( index > 0 && Order( m_queue[ index ], m_queue[ (index-1) >> 1 ] ) )
	{
		do 
		{
			Uint32 nextIndex = (index-1) >> 1;
			QueueSwap( index, nextIndex );
			index = nextIndex;
		}
		while ( index > 0 && Order( m_queue[ index ], m_queue[ (index-1) >> 1 ] ) );
	}
	// NOTICE: A* heap elements never 'rolls down' (as we only rise our priority
	// test if elements rolls down
	//else
	//{
	//	while ( (index << 1)+1 < size )
	//	{
	//		Uint32 index1 = (index << 1) + 1;
	//		Uint32 index2 = (index << 1) + 2;
	//		Uint32 nextIndex =
	//			( index2 < size && Order( m_queue[ index2 ], m_queue[ index1 ] ) )
	//			? index2
	//			: index1;
	//		if ( !Order( m_queue[ nextIndex ], m_queue[ index ] ) )
	//		{
	//			break;
	//		}
	//		QueueSwap( nextIndex, index );
	//		index = nextIndex;
	//	}
	//}
}

RED_INLINE void CActiveNodesQueue::Clear()
{
	for ( auto it = m_queue.Begin(), end = m_queue.End(); it != end; ++it )
	{
		(*it)->SetHeapIndex( CSearchNode::INVALID_HEAP_INDEX );
	}
	m_queue.ClearFast();
}

};				// namespace PathLib


