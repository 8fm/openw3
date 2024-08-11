#include "build.h"
#include "flyingPoiConfig.h"
#include "flyingCrittersParams.h"
#include "r4BoidSpecies.h"
#include "../../common/core/feedback.h"

////////////////////////////////////////////////////////////
// CGravity 
Bool CGravity::ParseXML( const SCustomNode & parentNode )
{
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= parentNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = parentNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( radius ) )
		{
			if ( att.GetValueAsFloat( m_radius ) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState, distance not specified as Float"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( gravity ) )
		{
			if ( att.GetValueAsFloat( m_gravity ) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState, distance not specified as Float"));
				return false;
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////
// CPoiConfigByGroup
Float CPoiConfigByGroup::GetGravityFromDistance( Float distance, Float distanceMult )const
{
	Float gravity = m_gravity;
	if ( m_gravityArray.Size() != 0 )
	{
		for ( Uint32 i = 0; i < m_gravityArray.Size(); ++i )
		{
			const Float distanceB = m_gravityArray[i].m_radius * distanceMult;
			if ( distance <= distanceB )
			{
				const Float & gravityA	= i > 0 ? m_gravityArray[ i - 1 ].m_gravity : m_gravityArray[ i ].m_gravity;
				const Float & gravityB	= m_gravityArray[ i ].m_gravity;
				const Float & distanceA	= i > 0 ? m_gravityArray[ i - 1 ].m_radius * distanceMult : distanceB;
				const Float distanceBA	= distanceB - distanceA; 
				const Float ratio		= ( distanceBA > NumericLimits<Float>::Epsilon() ) ? ( distance - distanceA ) / ( distanceBA ) : 0.0f;
				ASSERT( 0.0f <= ratio && ratio <= 1.0f );

				gravity = (1.0f - ratio) * gravityA + ratio * gravityB;
				break;
			}
			// If nothing as been found then gravity is equal to the last keyframe
			if ( i == m_gravityArray.Size() - 1 )
			{
				gravity = m_gravityArray[ i ].m_gravity;
				break;
			}
		}
	}
	return gravity;
}

Bool CPoiConfigByGroup::ParseXML( const SCustomNode & parentNode, CFlyingCritterLairParams *const params)
{
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= parentNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = parentNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( stateID ) )
		{
			Int32 groupStateId = -1;
			if ( att.GetValueAsInt( groupStateId ) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState, stateID not specified as int"));
				return false;
			}
			m_groupStateId = (Uint32)groupStateId;
		}
		if ( att.m_attributeName == CNAME( state_name ) )
		{
			m_groupStateId = params->GetGroupStateIndexFromName( att.m_attributeValueAsCName );
		}
		else if ( att.m_attributeName == CNAME( gravity ) )
		{
			if ( att.GetValueAsFloat( m_gravity ) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState, gravity not specified as float"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( closestOnly ) )
		{
			if ( att.GetValueAsBool( m_closestOnly ) == false )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState, closestOnly not specified as Bool"));
				return false;
			}
		}
	}

	const TDynArray< SCustomNode >::const_iterator end	= parentNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  it;
	for(  it = parentNode.m_subNodes.Begin();  it != end; ++it )
	{
		const SCustomNode & node = *it;	
		if ( node.m_nodeName == CNAME( gravity ) )
		{
			m_gravityArray.PushBack( CGravity() );
			if ( m_gravityArray[ m_gravityArray.Size() - 1 ].ParseXML( node ) == false )
			{
				return false;
			}
		}
	}
	return true;
}

////////////////////////////////////////////////////////////
// CFlyingPoiConfig
Bool CFlyingPoiConfig::ParseXML( const SCustomNode & poiConfigNode, CBoidSpecies *const boidSpecies, CFlyingCritterLairParams *const params)
{
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= poiConfigNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = poiConfigNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( type_name ) )
		{
			m_poiType = att.m_attributeValueAsCName;
		}
	}
	if ( m_poiType == CName::NONE )
	{
		GFeedback->ShowError(TXT("Boid XML Error: no poi type defined in POICOnfig"));
		return false;
	}
	
	const TDynArray< SCustomNode >::const_iterator end	= poiConfigNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  it;
	for(  it = poiConfigNode.m_subNodes.Begin();  it != end; ++it )
	{
		const SCustomNode & node = *it;	
		if ( node.m_nodeName == CNAME( groupState ) )
		{
			const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= node.m_attributes.End();
			TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
			CName groupState = CName::NONE;
			for ( attIt = node.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
			{
				const SCustomNodeAttribute & att	= *attIt;
				if ( att.m_attributeName == CNAME( state_name ) )
				{
					groupState = att.m_attributeValueAsCName;
					break;
				}
			}
			if ( groupState == CName::NONE )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState name not defined in poiConfig"));
				return false;
			}
			const Uint32 groupStateIndex = params->GetGroupStateIndexFromName( groupState );
			if ( groupStateIndex == (Uint32)-1 && groupState != CNAME( default ) )
			{
				GFeedback->ShowError(TXT("Boid XML Error: groupState specified in poiconfig not defined: %s"), groupState.AsString().AsChar() );
				return false;
			}
			
			// creating the new configByGroup and copying from parent if necessary
			CPoiConfigByGroup *const poiConfigByGroup	= new CPoiConfigByGroup();
			
			if ( poiConfigByGroup->ParseXML( node, params) == false )
			{
				delete poiConfigByGroup;
				return false;
			}
			static_cast<CR4BoidSpecies *>(boidSpecies)->AddPoiConfigByGroup( poiConfigByGroup );

			Uint32 index								= GetPoiConfigByGroupIndex( groupStateIndex );
			if ( index == (Uint32)-1 )
			{
				m_poiConfigByGroupArray.PushBack( poiConfigByGroup );
			}
			else
			{
				m_poiConfigByGroupArray[ index ] = poiConfigByGroup;
			}
		}
		else if ( node.m_nodeName == CNAME( positionOffset ) )
		{
			const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= node.m_attributes.End();
			TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
			for ( attIt = node.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
			{
				const SCustomNodeAttribute & att	= *attIt;
				if ( att.m_attributeName == CNAME( x ) )
				{
					if ( att.GetValueAsFloat( m_positionOffest.X ) == false )
					{
						GFeedback->ShowError(TXT("Boid XML Error: x is not defined as float"));
					}
				}
				else if ( att.m_attributeName == CNAME( y ) )
				{
					if ( att.GetValueAsFloat( m_positionOffest.Y ) == false )
					{
						GFeedback->ShowError(TXT("Boid XML Error: x is not defined as float"));
					}
				}
				else if ( att.m_attributeName == CNAME( z ) )
				{
					if ( att.GetValueAsFloat( m_positionOffest.Z ) == false )
					{
						GFeedback->ShowError(TXT("Boid XML Error: x is not defined as float"));
					}
				}
			}
		}
	}
	return true;
}

Uint32 CFlyingPoiConfig::GetPoiConfigByGroupIndex( Uint32 groupStateId )const
{
	for ( Uint32 i = 0; i < m_poiConfigByGroupArray.Size(); ++i )
	{
		if ( m_poiConfigByGroupArray[ i ]->m_groupStateId == groupStateId )
		{
			return i;
		}
	}
	return (Uint32)-1;
}