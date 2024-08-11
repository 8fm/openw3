#include "build.h"
#include "animBoidNode.h"
#include "boidInstance.h"
#include "../core/feedback.h"

CAnimBoidNode::CAnimBoidNode()
	: CAtomicBoidNode()
	, m_animName( )
	, m_effectName( )
	, m_looped( false )
	, m_timeOut( 3.0f )
	, m_timeOutVariation( 0.0f )
	, m_speedVariation( 0.0f )
{

}

void CAnimBoidNode::Activate( CBoidInstance *const boidInstance, CLoopedAnimPriorityQueue & loopedAnimPriorityQueue, Float time, Bool startRandom )const
{
	const Float animationDuration = boidInstance->PlayAnimation( m_animName, m_looped, m_speedVariation, startRandom );
	boidInstance->PlayEffect( m_effectName );

	Float timeOut = time + animationDuration;
	if ( m_looped )
	{
		timeOut = time + m_timeOut + (m_timeOutVariation != 0.0f ? GEngine->GetRandomNumberGenerator().Get< Float >( -m_timeOutVariation , m_timeOutVariation ) : 0.0f);
	}
	CBoidNodeData animNodeData( boidInstance );
	animNodeData.m_currentAtomicAnimNode	= this;
	animNodeData.m_stateCounter				= boidInstance->GetCurrentStateCounter();
	animNodeData.m_timeOut					= timeOut;
	loopedAnimPriorityQueue.PushHeap(animNodeData);
}

void CAnimBoidNode::Deactivate( CBoidInstance *const boidInstance )const
{
	boidInstance->StopEffect(m_effectName);
}


Bool CAnimBoidNode::ParseXMLAttribute( const SCustomNodeAttribute & animAtt, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{	
	if ( animAtt.m_attributeName == CNAME( anim_name ))
	{
		m_animName = animAtt.m_attributeValueAsCName;
	}
	else if ( animAtt.m_attributeName == CNAME( fx_name ))
	{
		m_effectName = animAtt.m_attributeValueAsCName;
	}
	else if ( animAtt.m_attributeName == CNAME( loop ))
	{
		if ( animAtt.GetValueAsBool( m_looped ) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: loop is not defined as a float"));
			return false;
		}
	}
	else if ( animAtt.m_attributeName == CNAME( timeOut ))
	{
		m_looped = true;
		if ( animAtt.GetValueAsFloat( m_timeOut ) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: timeOut is not defined as a float"));
			return false;
		}
	}
	else if ( animAtt.m_attributeName == CNAME( timeOutVariation ))
	{
		if ( animAtt.GetValueAsFloat( m_timeOutVariation ) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: timeOutvariation is not defined as a float"));
			return false;
		}
	}
	else if ( animAtt.m_attributeName == CNAME( speedVariation ))
	{
		if ( animAtt.GetValueAsFloat( m_speedVariation ) == false )
		{
			GFeedback->ShowError(TXT("Boid XML Error: speedVariation is not defined as a float"));
			return false;
		}
	}
	else 
	{
		return CBaseBoidNode::ParseXMLAttribute( animAtt, next, boidSpecies );
	}
	return true;
}

Bool CAnimBoidNode::ParseXML( const SCustomNode & loopBoidNode, const CBaseBoidNode *const next, CBoidSpecies *const boidSpecies )
{
	if ( CBaseBoidNode::ParseXML(loopBoidNode, next, boidSpecies) == false )
	{
		return false;
	}
	if ( m_animName == CName::NONE )
	{
		GFeedback->ShowError(TXT("Boid XML Error: no name specified in anim node"));
		return false;
	}
	return true;
}


