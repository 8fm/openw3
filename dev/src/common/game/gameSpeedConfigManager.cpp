#include "build.h"
#include "gameSpeedConfigManager.h"


void CGameSpeedConfigManager::InitParams(  )
{
	for ( auto it = m_speedConfigMap.Begin(), end = m_speedConfigMap.End(); it != end; ++it )
	{
		delete it->m_second;
	}
	m_speedConfigMap.Clear();

	CDefinitionsManager *const definitionManager = GCommonGame->GetDefinitionsManager();

	Bool errorInXML = true;
	const TDynArray< SCustomNode >& nodes						= definitionManager->GetCustomNodes();
	const TDynArray< SCustomNode >::const_iterator customEnd	= nodes.End();
	TDynArray< SCustomNode >::const_iterator  customIt;
	for(  customIt = nodes.Begin();  customIt !=  customEnd; ++ customIt )
	{
		const SCustomNode & defNode = * customIt;
		if ( defNode.m_nodeName != CNAME( speedConfig ) )
		{
			continue;
		}
		ParseSpeedConfig( defNode );
		errorInXML = false;
	}
	if ( errorInXML )
	{
		LOG_GAME(TXT("Error: no speed config defined in speedConfig.xml, there is probably an error in there !"));
	}
}

void CGameSpeedConfigManager::OnDefinitionsReloaded()
{
	if ( GCommonGame->IsActive() == false )
	{
		InitParams( );
	}
}


Bool CGameSpeedConfigManager::ParseSpeedConfig( const SCustomNode & speedConfigNode )
{
	if ( speedConfigNode.m_attributes.Size() == 0 )
	{
		ERR_GAME(TXT("Error: speedConfig must specify 'id' attribute  !"));
		return false;
	}

	Float walkSpeedAbs		= 1.0f;
	Float slowRunSpeedAbs	= 1.0f;
	Float fastRunSpeedAbs	= 1.0f;
	Float sprintSpeedAbs	= 1.0f;

	Float walkSpeedRel		= 1.0f;
	Float slowRunSpeedRel	= 1.0f;
	Float fastRunSpeedRel	= 1.0f;
	Float sprintSpeedRel	= 1.0f;

	CName key;

	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= speedConfigNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for (attIt = speedConfigNode.m_attributes.Begin();  attIt !=  attEnd; ++attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( key_name ) )
		{
			key			= att.m_attributeValueAsCName;
		}
		else if ( att.m_attributeName == CNAME( walkSpeedAbs ) )
		{
			if ( att.GetValueAsFloat( walkSpeedAbs ) == false )
			{
				ERR_GAME(TXT("Error: 'walkSpeedAbs' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( slowRunSpeedAbs ) )
		{
			if ( att.GetValueAsFloat( slowRunSpeedAbs ) == false )
			{
				ERR_GAME(TXT("Error: 'slowRunSpeedAbs' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( fastRunSpeedAbs ) )
		{
			if ( att.GetValueAsFloat( fastRunSpeedAbs ) == false )
			{
				ERR_GAME(TXT("Error: 'fastRunSpeedAbs' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( sprintSpeedAbs ) )
		{
			if ( att.GetValueAsFloat( sprintSpeedAbs ) == false )
			{
				ERR_GAME(TXT("Error: 'sprintSpeedAbs' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		// Rel speed
		else if ( att.m_attributeName == CNAME( walkSpeedRel ) )
		{
			if ( att.GetValueAsFloat( walkSpeedRel ) == false )
			{
				ERR_GAME(TXT("Error: 'walkSpeedRel' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( slowRunSpeedRel ) )
		{
			if ( att.GetValueAsFloat( slowRunSpeedRel ) == false )
			{
				ERR_GAME(TXT("Error: 'slowRunSpeedRel' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( fastRunSpeedRel ) )
		{
			if ( att.GetValueAsFloat( fastRunSpeedRel ) == false )
			{
				ERR_GAME(TXT("Error: 'fastRunSpeedRel' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
		else if ( att.m_attributeName == CNAME( sprintSpeedRel ) )
		{
			if ( att.GetValueAsFloat( sprintSpeedRel ) == false )
			{
				ERR_GAME(TXT("Error: 'sprintSpeedAbs' is speedConfig.xml attribute does not represent a float !"));
				return false;
			}
		}
	}

	if ( key.Empty() )
	{
		ERR_GAME(TXT("Error: speedConfig must specify 'id' attribute  !"));
		return false;
	}
	CSpeedConfig *const speedConfig = new CSpeedConfig( key, walkSpeedAbs, slowRunSpeedAbs, fastRunSpeedAbs, sprintSpeedAbs, walkSpeedRel, slowRunSpeedRel, fastRunSpeedRel, sprintSpeedRel );	
	m_speedConfigMap.Insert( key, speedConfig );
	return true;
}
