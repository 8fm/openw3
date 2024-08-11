#include "build.h"
#include "loopBoidNode.h"
#include "atomicBoidNode.h"

CLoopBoidNode::CLoopBoidNode()
	: CBaseBoidNode( E_TYPE )
	, m_firstNodeInLoop( NULL )
{

}
CLoopBoidNode::~CLoopBoidNode()
{
	
}

const CAtomicBoidNode *const	CLoopBoidNode::GetNextAtomicAnimNode()const
{
	if (m_firstNodeInLoop != NULL)
	{
		const CAtomicBoidNode *const pAtomicNode = m_firstNodeInLoop->As<CAtomicBoidNode>();
		if (pAtomicNode)
		{
			return pAtomicNode;
		}
		return m_firstNodeInLoop->GetNextAtomicAnimNode();
	}
	return CBaseBoidNode::GetNextAtomicAnimNode();
}

Bool CLoopBoidNode::ParseXML( const SCustomNode & loopBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	if ( CBaseBoidNode::ParseXML( loopBoidNode, next, boidSpecies ) == false )
	{
		return false;
	}

	if (loopBoidNode.m_subNodes.Size() == 0)
	{
		return true;
	}

	CBaseBoidNode * loopNext = this;
	// Building the list back to front !
	const TDynArray< SCustomNode >::const_iterator animBegin	= loopBoidNode.m_subNodes.Begin();
	TDynArray< SCustomNode >::const_iterator  animIt			= loopBoidNode.m_subNodes.End();
	do 
	{
		--animIt;
		const SCustomNode & animNode = *animIt;
		CBaseBoidNode * baseAnimNode = BaseAnimParseXML( animNode, loopNext, boidSpecies );
		if ( baseAnimNode == NULL )
		{
			return NULL;
		}
		loopNext			= baseAnimNode;
		m_firstNodeInLoop	= baseAnimNode;
	}
	while(  animIt != animBegin );

	return true;
}

