#include "build.h"
#include "pointOfInterestSpeciesConfig.h"
#include "boidLairEntity.h"
#include "../core/feedback.h"


CPointOfInterestSpeciesConfig::CPointOfInterestSpeciesConfig()
	: m_type( CName::NONE )
	, m_gravity( 0.0f )
	, m_action( false )
	, m_boidStateIndex( (Uint32)-1 )
	, m_actionTimeOut( -1.0f )
	, m_wanderTimeOut( -1.0f)
	, m_timeVariation( 10.0f)
{
	
}

Bool CPointOfInterestSpeciesConfig::ParseXML( const SCustomNode & POIConfigNode, const CBoidLairParams *const params )
{
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= POIConfigNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = POIConfigNode.m_attributes.Begin();  attIt !=  attEnd; ++ attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( type_name ) )
		{
			m_type = att.m_attributeValueAsCName;
		}
		else if ( att.m_attributeName == CNAME( gravity ) ) 
		{
			if ( att.GetValueAsFloat(m_gravity) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: gravity is not defined as a float"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( action ) ) 
		{
			if ( att.GetValueAsBool(m_action) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: action is not defined as a Bool"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( state_name ) ) 
		{
			m_boidStateIndex = params->GetBoidStateIndexFromName( att.m_attributeValueAsCName );
			if ( m_boidStateIndex == (Uint32) -1 )
			{
				GFeedback->ShowError( TXT("Boid XML Error: boid state specified in boid POI config doesn't exist: %s"), att.m_attributeValueAsCName.AsString().AsChar() );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( actionTimeOut ) ) 
		{
			if ( att.GetValueAsFloat(m_actionTimeOut) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: stateTime is not defined as a float"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( wanderTimeOut ) ) 
		{
			if ( att.GetValueAsFloat(m_wanderTimeOut) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: stateTime is not defined as a float"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( timeOutVariation ) ) 
		{
			if ( att.GetValueAsFloat(m_timeVariation) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: timeVariation is not defined as a float"));
				return false;
			}
		}
		else
		{
			GFeedback->ShowError(TXT("Boid XML Error: problem with xml attribute %s"), att.m_attributeName.AsString().AsChar());
			return false;
		}
	}
	return true;
}