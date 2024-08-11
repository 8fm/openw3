#pragma once

#include "baseBoidNode.h"

class CSequenceBoidNode : public CBaseBoidNode
{
public:
	CSequenceBoidNode();
	virtual ~CSequenceBoidNode();

	virtual Bool ParseXML( const SCustomNode & sequenceBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );

public:
	enum EType
	{
		TYPE_SEQUENCE_BOID_NODE	= TYPE_BASE_BOID_NODE | FLAG( 4 ),
	};
	enum 
	{ 
		E_TYPE = TYPE_SEQUENCE_BOID_NODE, 
	};
};