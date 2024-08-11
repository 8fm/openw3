#include "build.h"
#include "gwintMinigame.h"
#include "r4GuiManager.h"

IMPLEMENT_RTTI_ENUM( EGwintDifficultyMode );
IMPLEMENT_RTTI_ENUM( EGwintAggressionMode );
IMPLEMENT_ENGINE_CLASS( CGwintMenuInitData );
IMPLEMENT_ENGINE_CLASS( CGwintMinigame );

CGwintMenuInitData::CGwintMenuInitData()
	: m_difficulty( EGDM_Easy )
	, m_aggression( EGAM_Defensive )
	, m_allowMultipleMatches( false )
{
}

void CGwintMenuInitData::SetParams( const CName& deckName, EGwintDifficultyMode difficulty, EGwintAggressionMode aggression, Bool allowMultipleMatches )
{
	m_deckName = deckName;
	m_difficulty = difficulty;
	m_aggression = aggression;
	m_allowMultipleMatches = allowMultipleMatches;
}

CGwintMinigame::CGwintMinigame()
	: m_difficulty( EGDM_Easy )
	, m_aggression( EGAM_Defensive )
	, m_allowMultipleMatches( false )
{
}

CGwintMinigameInstanceData::CGwintMinigameInstanceData() 
	: m_saveLock( CGameSessionManager::GAMESAVELOCK_INVALID )
{

}

void CGwintMinigameInstanceData::DisableGameSaving()
{
	SGameSessionManager::GetInstance().CreateNoSaveLock( TXT( "CGwintMinigame" ), m_saveLock, false, false );
}

void CGwintMinigameInstanceData::EnableGameSaving()
{
	SGameSessionManager::GetInstance().ReleaseNoSaveLock( m_saveLock );
}

CGwintMinigame::~CGwintMinigame()
{
}

RED_DEFINE_STATIC_NAME( GwintGame )
RED_DEFINE_STATIC_NAME( OnGwintGameRequested )
RED_DEFINE_STATIC_NAME( GetGwintMinigameState )

void CGwintMinigame::OnStartGame( CMinigameInstanceData* _data ) const
{	
	CGwintMinigameInstanceData* data = static_cast< CGwintMinigameInstanceData* >( _data );
	data->DisableGameSaving();

	CallFunction( GCommonGame->GetPlayer(), CNAME(OnGwintGameRequested), m_deckName, m_forceFaction );

	data->m_state = EMS_Started;
}

void CGwintMinigame::OnEndGame( CMinigameInstanceData* _data ) const
{
	CGwintMinigameInstanceData* data = static_cast< CGwintMinigameInstanceData* >( _data );
	data->EnableGameSaving();
}

Bool CGwintMinigame::OnIsFinished( CMinigameInstanceData* _data ) const
{
	CGwintMinigameInstanceData* data = static_cast< CGwintMinigameInstanceData* >( _data );	

	EMinigameState result = EMS_None;
	if ( CallFunctionRet< EMinigameState >( GCommonGame->GetPlayer(), CNAME( GetGwintMinigameState ), result ) )
	{
		data->m_state = result;
	}

	return data->IsFinished();
}
