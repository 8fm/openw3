#pragma once
#include "atomicBoidNode.h"

class CAnimBoidNode : public CAtomicBoidNode
{
public:
	CAnimBoidNode();

	void	Activate( CBoidInstance *const boidInstance, CLoopedAnimPriorityQueue & loopedAnimPriorityQueue, Float time, Bool startRandom )const override;
	void	Deactivate( CBoidInstance *const boidInstance )const override;

	virtual Bool	ParseXMLAttribute( const SCustomNodeAttribute & animAtt, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );
	virtual Bool	ParseXML( const SCustomNode & loopBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies );
private:

	CName	m_animName;
	CName	m_effectName;
	Bool	m_looped;
	Float	m_timeOut;
	Float	m_timeOutVariation;
	Float	m_speedVariation;
};
