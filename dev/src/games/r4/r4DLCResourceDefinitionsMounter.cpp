#include "build.h"
#include "r4DLCResourceDefinitionsMounter.h"
#include "../../common/core/depot.h"
#include "../../common/core/resourceDefManager.h"

IMPLEMENT_ENGINE_CLASS( CR4ResourceDefinitionsDLCMounter );

CR4ResourceDefinitionsDLCMounter::CR4ResourceDefinitionsDLCMounter()
	: m_xmlFileLoaded( false )
{
	m_creatorTag = Red::System::GUID::Create();
}

bool CR4ResourceDefinitionsDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4ResourceDefinitionsDLCMounter::OnGameStarting()
{	
	Activate();
}

void CR4ResourceDefinitionsDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4ResourceDefinitionsDLCMounter::OnPostLoad()
{
	if( GIsCooker )
		Activate();
}

void CR4ResourceDefinitionsDLCMounter::OnFinalize()
{
	if( GIsCooker )
		Deactivate();
}

void CR4ResourceDefinitionsDLCMounter::OnEditorStarted()
{
	if( GIsCooker == false )
		Activate();
}

void CR4ResourceDefinitionsDLCMounter::OnEditorStopped()
{
	if( GIsCooker == false )
		Deactivate();
}

Bool CR4ResourceDefinitionsDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	if( !m_resourceDefinitionXmlFilePath.Empty() )
	{
		if( GDepot->FileExist( m_resourceDefinitionXmlFilePath ) )
		{
			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( m_resourceDefinitionXmlFilePath.AsChar() ) );
			outputList.AddFile( actualDepotPathAnsi );	

			CResourceDefManager resourceDefManager;
			resourceDefManager.LoadDefinitions( m_resourceDefinitionXmlFilePath, m_creatorTag );

			const CResourceDefManager::TResourceMap& resourceMap = resourceDefManager.GetResourceMap();
			for( auto& resDef : resourceMap )
			{
				outputList.AddFile( UNICODE_TO_ANSI( resDef.m_second.GetPath().AsChar() ) );	
			}
		}
	}
	return true;
}

#endif // !NO_EDITOR

void CR4ResourceDefinitionsDLCMounter::Activate()
{
	if( !m_resourceDefinitionXmlFilePath.Empty() )
	{
		if( GDepot->FileExist( m_resourceDefinitionXmlFilePath ) )
		{
			SResourceDefManager::GetInstance().LoadDefinitions( m_resourceDefinitionXmlFilePath, m_creatorTag );
			m_xmlFileLoaded = true;
		}
	}
}

void CR4ResourceDefinitionsDLCMounter::Deactivate()
{
	if( m_xmlFileLoaded )
	{
		SResourceDefManager::GetInstance().RemoveDefinitions( m_creatorTag );
		m_xmlFileLoaded = false;
	}
}
