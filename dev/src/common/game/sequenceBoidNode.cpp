#include "build.h"
#include "sequenceBoidNode.h"
#include "atomicBoidNode.h"

CSequenceBoidNode::CSequenceBoidNode()
	: CBaseBoidNode( E_TYPE )
{

}
CSequenceBoidNode::~CSequenceBoidNode()
{

}

Bool CSequenceBoidNode::ParseXML( const SCustomNode & sequenceBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	const CBaseBoidNode * sequenceNext	= next;
	if (sequenceBoidNode.m_subNodes.Size() != 0)
	{
		// Building the list back to front !
		const TDynArray< SCustomNode >::const_iterator animBegin	= sequenceBoidNode.m_subNodes.Begin();
		TDynArray< SCustomNode >::const_iterator  animIt			= sequenceBoidNode.m_subNodes.End();
	
		do 
		{
			--animIt;
			const SCustomNode & animNode = *animIt;
			CBaseBoidNode * baseAnimNode = BaseAnimParseXML( animNode, sequenceNext, boidSpecies );
			if (baseAnimNode == NULL)
			{
				delete next;
				return false;
			}
			sequenceNext			= baseAnimNode;
		}
		while(  animIt != animBegin );
	}

	return CBaseBoidNode::ParseXML( sequenceBoidNode, sequenceNext, boidSpecies );
}