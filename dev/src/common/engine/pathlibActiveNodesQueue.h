/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibGraph.h"

namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// Little explanation. Basically I need a heap. The same heap we got, but with support to have external pointers (or indexes) pointing into, as my search nodes
// has to point out into a heap to keep their current index. This is important, otherwise 'update heap' functionality becomes A* bottleneck. So basically below
// I do reimplementation of THeap. I didn't want to overextend my task, but could as well modify THeap implementation into supporting move constructor, and then
// using it along C++ magic to update heap indexes.
class CActiveNodesQueue
{
protected:
	TDynArray< CSearchNode* >			m_queue;

	RED_INLINE void			QueueSwap( Uint32 i1, Uint32 i2 );

	RED_INLINE Bool			IsHeap();
public:
	CActiveNodesQueue()																			{ m_queue.Reserve( 512 ); }
	~CActiveNodesQueue()																		{}

	RED_INLINE static Bool	Order( const CSearchNode* c1, const CSearchNode* c2 )			{ return c1->GetTotalCost() < c2->GetTotalCost(); }

	RED_INLINE Bool			Empty()															{ return m_queue.Empty(); }
	RED_INLINE void			Push( CSearchNode* node );
	RED_INLINE CSearchNode*	Pop();
	RED_INLINE void			Update( CSearchNode* node );
	RED_INLINE void			Clear();
};

};				// namespace PathLib

#include "pathlibActiveNodesQueue.inl"