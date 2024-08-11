#include "build.h"
#include "../../common/core/depot.h"
#include "../../common/game/attitude.h"
#include "r4DLCAttitudesMounter.h"
IMPLEMENT_ENGINE_CLASS( CR4AttitudesDLCMounter );

CR4AttitudesDLCMounter::CR4AttitudesDLCMounter():
	m_attitudeGroupsTableLoaded( false )
	, m_attitudesXMLLoaded( false )
{
}

bool CR4AttitudesDLCMounter::OnCheckContentUsage()
{
	//! nothing
	return false;
}

void CR4AttitudesDLCMounter::OnGameStarting()
{
	Activate();
}

void CR4AttitudesDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4AttitudesDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4AttitudesDLCMounter::OnEditorStopped()
{
	Deactivate();
}

bool CR4AttitudesDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	// analyze
	if( !m_attitudeGroupsTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_attitudeGroupsTableFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_attitudeGroupsTableFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
		}
	}

	if( !m_attitudesXMLFilePath.Empty() )
	{
		if( GDepot->FileExist( m_attitudesXMLFilePath ) )
		{
			StringAnsi filePathAnsi = UNICODE_TO_ANSI( m_attitudesXMLFilePath.AsChar() );
			outputList.AddFile( filePathAnsi );
		}
	}
	return true;
}

#endif // !NO_EDITOR

void CR4AttitudesDLCMounter::Activate()
{
	if( !m_attitudeGroupsTableFilePath.Empty() )
	{
		if( GDepot->FileExist( m_attitudeGroupsTableFilePath ) )
		{
			m_attitudeGroupsTableLoaded = SAttitudesResourcesManager::GetInstance().Load2dArray( m_attitudeGroupsTableFilePath );
		}		
	}

	if( !m_attitudesXMLFilePath.Empty() )
	{
		if( GDepot->FileExist( m_attitudesXMLFilePath ) )
		{
			CAttitudeManager *attitudeManager = GCommonGame->GetSystem< CAttitudeManager >();
			if( attitudeManager )
			{
				m_attitudesXMLLoaded =  attitudeManager->AddAdditionalAttitudesXML( m_attitudesXMLFilePath );
				//! xml`s are loaded on CAttitudeManager::OnGameStart
			}
		}		
	}
}

void CR4AttitudesDLCMounter::Deactivate()
{
	if( m_attitudeGroupsTableLoaded )
	{
		SAttitudesResourcesManager::GetInstance().Unload2dArray( m_attitudeGroupsTableFilePath );
		m_attitudeGroupsTableLoaded = false;
	}

	if( m_attitudesXMLLoaded )
	{
		CAttitudeManager *attitudeManager = GCommonGame->GetSystem< CAttitudeManager >();
		if( attitudeManager )
		{
			attitudeManager->RemAdditionalAttitudesXML( m_attitudesXMLFilePath );
		}
		m_attitudesXMLLoaded = false;
	}
}
