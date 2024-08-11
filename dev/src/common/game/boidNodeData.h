#pragma once
#include "../core/heap.h"

class CBoidInstance;
class CAtomicBoidNode;


class CBoidNodeData
{
public:
	CBoidNodeData(CBoidInstance *const	boidInstance = NULL);

	const CAtomicBoidNode *	m_currentAtomicAnimNode;
	// In the case of a looped animation the time out defines when it ends
	Float					m_timeOut;
	Uint32					m_stateCounter;

	CBoidInstance *const GetBoidInstance()const{return m_boidInstance;}

	// Enabling heap sort
	Bool operator<( const CBoidNodeData& e ) const
	{
		return m_timeOut < e.m_timeOut;
	}
private:
	CBoidInstance *	m_boidInstance;
};

typedef THeap<CBoidNodeData> CLoopedAnimPriorityQueue;