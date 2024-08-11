#include "build.h"
#include "flyingCrittersParams.h"
#include "r4BoidSpecies.h"
#include "r4SwarmSound.h"
#include "../../common/core/feedback.h"


/////////////////////////////////////////////////////////////////////////////////
//					CFlyingCritterLairParams
/////////////////////////////////////////////////////////////////////////////////

const CFlyingCritterLairParams CFlyingCritterLairParams::sm_defaultParams = CFlyingCritterLairParams(false);


CFlyingCritterLairParams::CFlyingCritterLairParams(Bool isValid)
	: CSwarmLairParams( E_TYPE, CNAME( FlyingCritterLair ), isValid )
	, m_wallsDistance( 4.f )
	, m_wallsRepulsion( 3.f )
	, m_livesUnderWater( false )
	, m_waterSurfaceCollisionForce( 1.0f )
{

}

CFlyingCritterLairParams::CFlyingCritterLairParams( const CFlyingCritterLairParams & params )
	: CSwarmLairParams( E_TYPE, CNAME( FlyingCritterLair ), false )
{
	*this = params;
}

CFlyingCritterLairParams::~CFlyingCritterLairParams()
{

}

Bool CFlyingCritterLairParams::VirtualCopyTo(CBoidLairParams* const params)const
{
	CFlyingCritterLairParams *const targetParams = params->As<CFlyingCritterLairParams>();
	if (targetParams)
	{
		*targetParams = *this;
		return true;
	}
	return false;
}

Uint32  CFlyingCritterLairParams::GetGroupStateIndexFromName( CName stateName )const
{
	for ( Uint32 i=0; i<m_groupStateArray.Size(); ++i )
	{
		if ( m_groupStateArray[i]->m_stateName == stateName )
		{
			return i;
		}
	}
	return (Uint32)-1;
}
Bool CFlyingCritterLairParams::ParseXmlNode( const SCustomNode & node, CBoidSpecies *const boidSpecies )
{
	CR4BoidSpecies *const r4BoidSpecies = static_cast< CR4BoidSpecies* >(boidSpecies);
	
	if ( node.m_nodeName == CNAME( groupStates ) )
	{
		const TDynArray< SCustomNode >::const_iterator groupStateEnd	= node.m_subNodes.End();
		TDynArray< SCustomNode >::const_iterator  groupStateIt;
		for(  groupStateIt = node.m_subNodes.Begin();  groupStateIt != groupStateEnd; ++groupStateIt )
		{
			const SCustomNode & groupStateNode = *groupStateIt;
			if ( groupStateNode.m_nodeName == CNAME( state ) )
			{
				const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= groupStateNode.m_attributes.End();
				TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
				CName groupStateName = CName::NONE;
				for ( attIt = groupStateNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
				{
					const SCustomNodeAttribute & att	= *attIt;
					if ( att.m_attributeName == CNAME( state_name ) )
					{
						groupStateName = att.m_attributeValueAsCName;
					}
				}
				if ( groupStateName == CName::NONE )
				{
					GFeedback->ShowError(TXT("Boid XML Error: flying group state name not specified"));
					return false;
				}

				const Uint32 groupStateIndexFromParent	= GetGroupStateIndexFromName( groupStateName );
				CGroupState *const groupState			= new CGroupState();

				// copy data if has a parent :
				if ( groupStateIndexFromParent != (Uint32)-1 )
				{
					*groupState	= *m_groupStateArray[ groupStateIndexFromParent ];
				}
				if ( groupState->ParseXML( groupStateNode, r4BoidSpecies ) == false )
				{
					delete groupState;
					WARN_R4( TXT("problem with file boid_species.xml, poi node is specified incorrectly") );
					return false;
				}
				r4BoidSpecies->AddGroupState( groupState ); // adding it there for proper deletion

				if ( groupStateIndexFromParent != (Uint32)-1 )
				{
					m_groupStateArray[ groupStateIndexFromParent ] = groupState;
				}
				else
				{
					m_groupStateArray.PushBack( groupState );
				}
			}
			else
			{
				GFeedback->ShowError( TXT("Boid XML Error: undefined node %s"), groupStateNode.m_nodeName.AsString().AsChar() );
				return false;
			}
		}
	}
	else
	{
		if ( CBoidLairParams::ParseXmlNode( node, boidSpecies ) == false )
		{
			return false;
		}
	}
	return true;
}

Bool CFlyingCritterLairParams::OnParsePoiConfig( const SCustomNode & node, CBoidSpecies *const boidSpecies )
{
	CR4BoidSpecies *const r4BoidSpecies = static_cast< CR4BoidSpecies*const >( boidSpecies );
	CFlyingPoiConfig *const poiConfig	= new CFlyingPoiConfig();
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= node.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	CName poiType = CName::NONE;
	for ( attIt = node.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;

		if ( att.m_attributeName == CNAME( type_name ) )
		{
			poiType = att.m_attributeValueAsCName;
		}
	}

	if ( poiType == CName::NONE )
	{
		delete poiConfig;
		GFeedback->ShowError( TXT("Boid XML Error: poiConfig does have a type defined") );
		return false;
	}

	CFlyingPoiConfig_Map::iterator flyingPoiConfigIt = m_flyingPoiConfigMap.Find( poiType );
	if ( flyingPoiConfigIt != m_flyingPoiConfigMap.End() )
	{
		*poiConfig = *flyingPoiConfigIt->m_second;
	}
	if ( poiConfig->ParseXML(node, boidSpecies, this ) == false )
	{
		delete poiConfig;
		return false;
	}
	
	// Add for safe deletion
	r4BoidSpecies->AddFlyingPoiConfig( poiConfig );

	if ( flyingPoiConfigIt != m_flyingPoiConfigMap.End() )
	{
		flyingPoiConfigIt->m_second = poiConfig;
	}
	else
	{
		m_flyingPoiConfigMap.Insert( poiType, poiConfig );
	}
	return CBoidLairParams::OnParsePoiConfig( node, boidSpecies );
}
Bool CFlyingCritterLairParams::ParseXML( const SCustomNode & paramsNode, CBoidSpecies *const boidSpecies )
{
	if ( CSwarmLairParams::ParseXML( paramsNode, boidSpecies) == false )
	{
		return false;
	}
	m_flyingPoiConfigMap.Sort();

	// Building m_groupStateToPoiConfig from m_flyingPoiConfigMap now
	// First adding as many entry as there are group states :
	m_groupStateToPoiConfig.Resize( m_groupStateArray.Size() );
	for ( Uint32 i = 0; i < m_groupStateToPoiConfig.Size(); ++i )
	{
		CPoiConfigByGroup_Map& poiConfigByGroup_Map = m_groupStateToPoiConfig[ i ];
		for ( Uint32 j=0; j<m_flyingPoiConfigMap.Size(); ++j )
		{
			const CName & poiType							= m_flyingPoiConfigMap[j].m_first;
			poiConfigByGroup_Map.PushBack( TPair< CName, const CPoiConfigByGroup* >( poiType, nullptr ) );
		}
		poiConfigByGroup_Map.Sort();
	}

	for ( Uint32 i = 0; i < m_flyingPoiConfigMap.Size(); ++i )
	{
		const CName & poiType					= m_flyingPoiConfigMap[i].m_first;
		CFlyingPoiConfig*const flyingPoiConfig	= m_flyingPoiConfigMap[i].m_second;
		
		// each flyingPoiConfig holds an array of config ( one for each group state )
		for ( Uint32 j = 0; j < flyingPoiConfig->m_poiConfigByGroupArray.Size(); ++j )
		{
			const CPoiConfigByGroup & poiConfigByGroup = *flyingPoiConfig->m_poiConfigByGroupArray[ j ];
			if ( poiConfigByGroup.m_groupStateId != (Uint32)-1 )
			{
				CPoiConfigByGroup_Map& poiConfigByGroupMap	= m_groupStateToPoiConfig[ poiConfigByGroup.m_groupStateId ];
				CPoiConfigByGroup_Map::iterator poiConfigIt = poiConfigByGroupMap.Find( poiType );
				ASSERT( poiConfigIt != poiConfigByGroupMap.End() );
				poiConfigIt->m_second = &poiConfigByGroup;
			}
			else // if groupStateId is -1 then this is the default config for all groupStates 
			{
				// setting this config to all group states :
				for ( Uint32 k = 0; k < m_groupStateToPoiConfig.Size(); ++k )
				{
					CPoiConfigByGroup_Map& poiConfigByGroupMap	= m_groupStateToPoiConfig[ k ];
					CPoiConfigByGroup_Map::iterator poiConfigIt = poiConfigByGroupMap.Find( poiType );
					ASSERT( poiConfigIt != poiConfigByGroupMap.End() );
					// Must not set it if something has been set before ( default should not override )
					if ( poiConfigIt->m_second == NULL )
					{
						poiConfigIt->m_second = &poiConfigByGroup;
					}
				}
			}
		}
	}
	return true;
}

Bool CFlyingCritterLairParams::ParseXmlAttribute( const SCustomNodeAttribute & att )
{
	if (att.m_attributeName == CNAME( wallsDistance ))
	{
		if (att.GetValueAsFloat( m_wallsDistance ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: wallsDistance is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( wallsRepulsion ))
	{
		if (att.GetValueAsFloat( m_wallsRepulsion ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: wallsRepulsion is not defined as a float"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( livesUnderWater ))
	{
		if (att.GetValueAsBool( m_livesUnderWater ) == false)
		{
			GFeedback->ShowError(TXT("Boid XML Error: livesUnderWater is not defined as a bool"));
			return false;
		}	
	}
	else if (att.m_attributeName == CNAME( waterSurfaceCollisionForce ) )
	{
		att.GetValueAsFloat( m_waterSurfaceCollisionForce );		// Default is 1.0f;
	}
	else
	{
		return CSwarmLairParams::ParseXmlAttribute(att);
	}
	return true;
}

CSwarmSoundConfig *const CFlyingCritterLairParams::CreateSoundConfig()
{
	return new CR4SwarmSoundConfig();
}

//////////////////////////////////////////////////////////////////
//		CGroupState												//
//////////////////////////////////////////////////////////////////
CGroupState::CGroupState()
	: m_stateName( CName::NONE ) 
	, m_ignorePoiAfterReach( false )
	, m_cozyFlyDist( 2.0f )
	, m_maxVelocity( 20.0f )
	, m_minVelocity( 10.0f )
	, m_frictionMultiplier( 0.1f )
	, m_tooCloseMultiplier( 1.0f )
	, m_tooFarMultiplier( 0.0f )
	, m_randomDirMultiplier( 0.0f )
	, m_thinnessMultiplier( 1.0f )
	, m_collisionMultiplier( 6.0f )
	, m_randomNeighboursRatio( 1.0f )
	, m_randomDirRatio( 0.0f )
	, m_neighbourCount( 20 )
	, m_glidingPercentage( 0.75 )

	, m_velocityVariation( 0.0f )	
	, m_tooCloseVariation( 0.4f )
	, m_tooFarVariation( 0.4f )		
	, m_genericVariation( 0.4f )
{

}

Bool CGroupState::ParseXML( const SCustomNode & poiStateNode, CR4BoidSpecies *const boidSpecies )
{
	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= poiStateNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for ( attIt = poiStateNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( state_name ) )
		{
			m_stateName = att.m_attributeValueAsCName;
		}
		else if ( att.m_attributeName == CNAME( ignorePoiAfterReach ) )
		{
			if ( att.GetValueAsBool(m_ignorePoiAfterReach) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: ignorePoiAfterReach is not defined as a bool") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( cozyFlyDist ) )
		{
			if ( att.GetValueAsFloat(m_cozyFlyDist) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: cozyFlyDist is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( maxVelocity ) )
		{
			if ( att.GetValueAsFloat(m_maxVelocity) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: maxVelocity is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( minVelocity ) )
		{
			if ( att.GetValueAsFloat(m_minVelocity) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: minVelocity is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( frictionMultiplier ) )
		{
			if ( att.GetValueAsFloat(m_frictionMultiplier) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: frictionMultiplier is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( tooCloseMultiplier ) )
		{
			if ( att.GetValueAsFloat(m_tooCloseMultiplier) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: tooCloseMultiplier is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( tooFarMultiplier ) )
		{
			if ( att.GetValueAsFloat(m_tooFarMultiplier) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: tooFarMultiplier is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( randomDirMultiplier ) )
		{
			if ( att.GetValueAsFloat(m_randomDirMultiplier) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: randomDirMultiplier is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( thinnessMultiplier ) )
		{
			if ( att.GetValueAsFloat(m_thinnessMultiplier) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: thinnessMultiplier is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( collisionMultiplier ) )
		{
			if ( att.GetValueAsFloat(m_collisionMultiplier) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: collisionMultiplier is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( randomNeighboursRatio ) )
		{
			if ( att.GetValueAsFloat(m_randomNeighboursRatio) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: randomNeighboursRatio is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( randomDirRatio ) )
		{
			if ( att.GetValueAsFloat(m_randomDirRatio) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: randomDirRatio is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( neighbourCount ) )
		{
			Int32 val;
			if ( att.GetValueAsInt(val) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: neighbourCount is not defined as a Int") );
				return false;
			}
			ASSERT( val >= 0 );
			m_neighbourCount = (Uint32) val;
		}
		else if ( att.m_attributeName == CNAME( glidingPercentage ) )
		{
			if ( att.GetValueAsFloat( m_glidingPercentage ) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: glidingPercentage is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( velocityVariation ) )
		{
			if ( att.GetValueAsFloat(m_velocityVariation) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: maxVelocityVariation is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( tooCloseVariation ) )
		{
			if ( att.GetValueAsFloat(m_tooCloseVariation) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: tooCloseVariation is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( tooFarVariation ) )
		{
			if ( att.GetValueAsFloat(m_tooFarVariation) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: tooFarVariation is not defined as a Float") );
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( genericVariation ) )
		{
			if ( att.GetValueAsFloat(m_genericVariation) == false )
			{
				GFeedback->ShowError( TXT("Boid XML Error: genericVariation is not defined as a Float") );
				return false;
			}
		}
		else
		{
			GFeedback->ShowError( TXT("Boid XML Error: poiState unrekognized attribute: %s"), att.m_attributeName.AsString().AsChar() );
			return false;
		}
	}
	return true;
}
