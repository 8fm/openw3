#pragma once

#include "baseBoidNode.h"
#include "swarmUtils.h"

class CRandBoidNode : public CBaseBoidNode
{
public:
	CRandBoidNode();
	virtual ~CRandBoidNode();
	virtual const CAtomicBoidNode *const	GetNextAtomicAnimNode()const;

	virtual Bool ParseXML( const SCustomNode & randBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );
private:
	CBaseBoidNode_Array _childrenArray;

public:
	enum EType
	{
		TYPE_RAND_BOID_NODE	= TYPE_BASE_BOID_NODE | FLAG( 3 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_RAND_BOID_NODE, 
	};
};