#pragma once
#include "baseBoidNode.h"

class CLoopBoidNode : public CBaseBoidNode
{
public:
	CLoopBoidNode();
	virtual ~CLoopBoidNode();
	virtual const CAtomicBoidNode *const	GetNextAtomicAnimNode()const;

	virtual Bool ParseXML( const SCustomNode & loopBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );
private:

	const CBaseBoidNode *	m_firstNodeInLoop;

public:
	enum EType
	{
		TYPE_LOOP_BOID_NODE	= TYPE_BASE_BOID_NODE | FLAG( 2 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_LOOP_BOID_NODE, 
	};
};