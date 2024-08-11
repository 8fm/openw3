#include "Build.h"
#include "baseBoidNode.h"
#include "boidInstance.h"
#include "atomicBoidNode.h"
#include "animBoidNode.h"
#include "sequenceBoidNode.h"
#include "randBoidNode.h"
#include "loopBoidNode.h"
#include "boidSpecies.h"
#include "../core/feedback.h"


/////////////////////////////////////////////////////////////////////////////
CBaseBoidNode::CBaseBoidNode( Uint32 type )
	: m_type( type )
	, m_next( NULL )
	, m_proba( -1.0f )
{

}

CBaseBoidNode::~CBaseBoidNode()
{
	
}

const CAtomicBoidNode *const CBaseBoidNode::GetNextAtomicAnimNode()const
{
	if (m_next)
	{
		const CAtomicBoidNode *const pAtomicNode = m_next->As<CAtomicBoidNode>();
		if (pAtomicNode)
		{
			return pAtomicNode;
		}
		return m_next->GetNextAtomicAnimNode();
	}
	return NULL; 
}


Bool CBaseBoidNode::ParseXML( const SCustomNode & animNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	m_next = next;

	const TDynArray< SCustomNodeAttribute >::const_iterator animAttEnd	= animNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		animAttIt;
	for ( animAttIt = animNode.m_attributes.Begin();  animAttIt !=  animAttEnd; ++ animAttIt )
	{
		const SCustomNodeAttribute & animAtt	= *animAttIt;
		if ( ParseXMLAttribute( animAtt, next, boidSpecies ) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: problem with Xml attribute: %s"), animAtt.m_attributeName.AsString().AsChar());
			return false;
		}
	}
	return true;
}

Bool CBaseBoidNode::ParseXMLAttribute( const SCustomNodeAttribute & animAtt, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	if ( animAtt.m_attributeName == CNAME( proba ) )
	{
		if ( animAtt.GetValueAsFloat(m_proba) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: proba is not defined as a float"));
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
}

CBaseBoidNode *const BaseAnimParseXML( const SCustomNode & baseAnimXMLNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	CBaseBoidNode * baseAnimNode = NULL;
	if ( baseAnimXMLNode.m_nodeName == CNAME( anim ) )
	{
		CAnimBoidNode *const animBoidNode = new CAnimBoidNode();
		if (animBoidNode->ParseXML( baseAnimXMLNode, next, boidSpecies ) == false)
		{
			delete animBoidNode;
			return NULL;
		}
		baseAnimNode = animBoidNode;
	}
	else if ( baseAnimXMLNode.m_nodeName == CNAME( loop ) )
	{
		CLoopBoidNode *const loopBoidNode = new CLoopBoidNode();
		if (loopBoidNode->ParseXML( baseAnimXMLNode, next, boidSpecies ) == false)
		{
			delete loopBoidNode;
			return NULL;
		}
		baseAnimNode = loopBoidNode;
	}
	else if ( baseAnimXMLNode.m_nodeName == CNAME( rand ) )
	{
		CRandBoidNode *const randBoidNode = new CRandBoidNode();
		if (randBoidNode->ParseXML( baseAnimXMLNode, next, boidSpecies ) == false)
		{
			delete randBoidNode;
			return NULL;
		}
		baseAnimNode = randBoidNode;
	}
	else if ( baseAnimXMLNode.m_nodeName == CNAME( sequence ) )
	{
		CSequenceBoidNode *const sequenceBoidNode = new CSequenceBoidNode();
		if (sequenceBoidNode->ParseXML( baseAnimXMLNode, next, boidSpecies ) == false)
		{
			delete sequenceBoidNode;
			return NULL;
		}
		baseAnimNode = sequenceBoidNode;
	}
	boidSpecies->AddBaseBoidNode( baseAnimNode );
	return baseAnimNode;
}
