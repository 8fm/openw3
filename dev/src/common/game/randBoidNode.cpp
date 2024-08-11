#include "build.h"
#include "randBoidNode.h"
#include "atomicBoidNode.h"

CRandBoidNode::CRandBoidNode()
	: CBaseBoidNode( E_TYPE )
{

}
CRandBoidNode::~CRandBoidNode()
{

}

const CAtomicBoidNode *const CRandBoidNode::GetNextAtomicAnimNode()const
{
	const Float randVar	= GEngine->GetRandomNumberGenerator().Get< Float >();
	Float cumulProba	= 0.0f;
	for (Uint32 i=0; i<_childrenArray.Size(); ++i)
	{
		const CBaseBoidNode *const baseAnimNode = _childrenArray[i];
		const Float &nodeProba					= baseAnimNode->GetProba();
		const Float proba						= nodeProba != -1.0f ? nodeProba : 1.0f / _childrenArray.Size();
		cumulProba								+= proba;
		if (randVar < cumulProba)
		{
			const CAtomicBoidNode *const pAtomicNode = baseAnimNode->As<CAtomicBoidNode>();
			if (pAtomicNode)
			{
				return pAtomicNode;
			}
			return baseAnimNode->GetNextAtomicAnimNode();
		}
	}

	if (m_next)
	{
		return m_next->GetNextAtomicAnimNode();
	}
	return NULL;
}


Bool CRandBoidNode::ParseXML( const SCustomNode & randBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	if ( CBaseBoidNode::ParseXML( randBoidNode, next, boidSpecies ) == false )
	{
		return false;
	}

	if (randBoidNode.m_subNodes.Size() == 0)
	{
		return true;
	}

	// Building the list back to front !
	const TDynArray< SCustomNode >::const_iterator animEnd		= randBoidNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  animIt			= randBoidNode.m_subNodes.Begin();
	while(  animIt != animEnd ) 
	{
		const SCustomNode & animNode = *animIt;
		CBaseBoidNode * baseAnimNode = BaseAnimParseXML( animNode, next, boidSpecies );

		if (baseAnimNode == NULL)
		{
			delete next;
			return false;
		}
		_childrenArray.PushBack(baseAnimNode);
		++animIt;
	}
	return true;
}