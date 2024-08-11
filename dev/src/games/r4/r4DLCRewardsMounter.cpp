#include "build.h"
#include "r4DLCRewardsMounter.h"
#include "../../common/core/depot.h"

IMPLEMENT_ENGINE_CLASS( CR4RewardsDLCMounter );

CR4RewardsDLCMounter::CR4RewardsDLCMounter()
	: m_xmlFileLoaded( false )
{
}

bool CR4RewardsDLCMounter::OnCheckContentUsage()
{
	return false;
}

void CR4RewardsDLCMounter::OnGameStarting()
{
	Activate();
}

void CR4RewardsDLCMounter::OnGameEnding()
{
	Deactivate();
}

#ifndef NO_EDITOR

void CR4RewardsDLCMounter::OnEditorStarted()
{
	Activate();
}

void CR4RewardsDLCMounter::OnEditorStopped()
{
	Deactivate();
}

Bool CR4RewardsDLCMounter::DoAnalyze( CAnalyzerOutputList& outputList )
{
	if( !m_rewordsXmlFilePath.Empty() )
	{
		if( GDepot->FileExist( m_rewordsXmlFilePath ) )
		{
			const StringAnsi actualDepotPathAnsi( UNICODE_TO_ANSI( m_rewordsXmlFilePath.AsChar() ) );
			outputList.AddFile( actualDepotPathAnsi );
		}
	}

	return true;
}

#endif // !NO_EDITOR

void CR4RewardsDLCMounter::Activate()
{
	if( !m_rewordsXmlFilePath.Empty() )
	{
		if( GDepot->FileExist( m_rewordsXmlFilePath ) )
		{
			CRewardManager* rewardManager = GCommonGame->GetRewardManager();
			if( rewardManager )
			{
				m_xmlFileLoaded = rewardManager->LoadRewardsFromFile( m_rewordsXmlFilePath, &m_loadedRewardNames );
			}
		}
		else
		{
			RED_LOG_WARNING( RED_LOG_CHANNEL(DLC), TXT( "Rewards XML file '%ls' not present." ), m_rewordsXmlFilePath.AsChar() );
		}
	}
}

void CR4RewardsDLCMounter::Deactivate()
{
	if( m_xmlFileLoaded )
	{
		CRewardManager* rewardManager = GCommonGame->GetRewardManager();
		if( rewardManager )
		{
			rewardManager->RemoveRewards( m_loadedRewardNames );
			m_loadedRewardNames.Clear();
		}
		m_xmlFileLoaded = false;
	}
}
