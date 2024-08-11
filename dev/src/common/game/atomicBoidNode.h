#pragma once
#include "baseBoidNode.h"

class IPlayedAnimationListener;

class CAtomicBoidNode : public CBaseBoidNode
{
public:

	CAtomicBoidNode();

	virtual void	Activate(CBoidInstance *const boidInstance, CLoopedAnimPriorityQueue & loopedAnimPriorityQueue, Float time, Bool startRandom )const = 0;
	virtual void	Deactivate(CBoidInstance *const boidInstance)const = 0;
public:

	enum EType
	{
		TYPE_ATOMIC_BOID_NODE	= TYPE_BASE_BOID_NODE | FLAG( 1 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_ATOMIC_BOID_NODE, 
	};
};

