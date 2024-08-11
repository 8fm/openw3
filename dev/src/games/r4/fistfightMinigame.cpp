#include "build.h"
#include "fistfightMinigame.h"

#include "..\..\common\engine\tagManager.h"

#include "..\..\common\game\behTreeReactionManager.h"
#include "..\..\common\game\deniedAreaSaveable.h"


RED_DEFINE_STATIC_NAME( OnEndFistfightMinigame )
RED_DEFINE_STATIC_NAME( OnStartFistfightMinigame )
RED_DEFINE_STATIC_NAME( SetFistFightParams )
RED_DEFINE_STATIC_NAME( IsAlive )

IMPLEMENT_ENGINE_CLASS( CFistfightMinigame );
IMPLEMENT_ENGINE_CLASS( CFistfightOpponent );

const Float CFistfightMinigame::FADE_DURATION = 1.f;
const Float CFistfightMinigame::END_GAME_DELAY = 2.f;
const Float CFistfightMinigame::TIME_OFFSET = 0.2f;

CFistfightMinigameInstanceData::CFistfightMinigameInstanceData()
	: m_requestedGameEnd( EMS_None )
	, m_wasFadeCalled( false )
{

}

void CFistfightMinigameInstanceData::DoFade()
{
	RED_ASSERT( !m_wasFadeCalled );
	if ( !m_wasFadeCalled )
	{
		//GCommonGame->SetBlackscreen( true, TXT("Fistfight minigame with m_endsWithBlackscreen") );
		GCommonGame->StartFade( false, TXT("Fistfight minigame with m_endsWithBlackscreen"), CFistfightMinigame::FADE_DURATION );
		GCommonGame->SetFadeLock( TXT("Fistfight_minigame") );

		m_wasFadeCalled = true;
	}
}

void CFistfightMinigameInstanceData::DoBlacksceen()
{
	RED_ASSERT( !m_wasFadeCalled );
	if ( !m_wasFadeCalled )
	{
		GCommonGame->SetBlackscreen( true, TXT("Fistfight minigame with m_endsWithBlackscreen") );
		GCommonGame->SetFadeLock( TXT("Fistfight_minigame") );

		m_wasFadeCalled = true;
	}
}

void CFistfightMinigameInstanceData::RequestEndGame( EMinigameState state )
{
	RED_ASSERT( m_requestedGameEnd == EMS_None );
	RED_ASSERT( !m_gameEndRequestTime.IsValid() );

	m_requestedGameEnd = state;
	m_gameEndRequestTime = GGame->GetEngineTime();
}

Bool CFistfightMinigameInstanceData::HasGameEndRequest() const
{
#ifdef  RED_ASSERTS_ENABLED
	if ( m_requestedGameEnd != EMS_None )
	{
		RED_ASSERT( m_requestedGameEnd == EMS_End_PlayerWon || m_requestedGameEnd == EMS_End_PlayerLost );
	}
#endif

	return m_requestedGameEnd == EMS_End_PlayerWon || m_requestedGameEnd == EMS_End_PlayerLost;
}

Bool CFistfightMinigameInstanceData::WaitForGameEnd()
{
	RED_ASSERT( m_gameEndRequestTime > 0.f );
	RED_ASSERT( m_requestedGameEnd != EMS_None );

	const EngineTime currTime = GGame->GetEngineTime();
	const EngineTime delta = currTime - m_gameEndRequestTime;
	
	const Float deltaF( delta );
	RED_ASSERT( deltaF >= 0.f );

	return deltaF > CFistfightMinigame::END_GAME_DELAY;
}

Bool CFistfightMinigameInstanceData::WaitForFadeStart()
{
	RED_ASSERT( m_gameEndRequestTime > 0.f );
	RED_ASSERT( m_requestedGameEnd != EMS_None );

	const EngineTime currTime = GGame->GetEngineTime();
	const EngineTime delta = currTime - m_gameEndRequestTime;

	const Float deltaF( delta );
	RED_ASSERT( deltaF >= 0.f );

	//static_assert( ( CFistfightMinigame::TIME_OFFSET + CFistfightMinigame::FADE_DURATION ) < CFistfightMinigame::END_GAME_DELAY, "Wrong params for CFistfightMinigameInstanceData. Please debug." );

	return deltaF > CFistfightMinigame::END_GAME_DELAY - ( CFistfightMinigame::FADE_DURATION + CFistfightMinigame::TIME_OFFSET );
}

//////////////////////////////////////////////////////////////////////////

CFistfightMinigame::CFistfightMinigame()
{

}

Bool CFistfightMinigame::OnIsFinished( CMinigameInstanceData* _data ) const 
{
	CFistfightMinigameInstanceData* data = static_cast< CFistfightMinigameInstanceData* >( _data );

	CPlayer* player = GCommonGame->GetPlayer();
	Bool knockedDown = true;
	CallFunctionRet< Bool >( player, CNAME( IsKnockedUnconscious ), knockedDown );

	if ( data->HasGameEndRequest() )
	{
		// Wait for fade
		if ( m_endsWithBlackscreen && data->WaitForFadeStart() )
		{
			data->DoFade();
		}

		// Wait for minigame end
		if ( data->WaitForGameEnd() )
		{
			// End game with requested state
			data->m_state = data->m_requestedGameEnd;
			RED_ASSERT( data->IsFinished() );
		}
		else
		{
			RED_ASSERT( !data->IsFinished() );
		}
	}
	else
	{
		// Process minigame logic
		switch ( data->m_state )
		{
		case EMS_Wait_PlayerLost:
			{
				if( !knockedDown )
				{
					data->m_state = EMS_End_PlayerLost;
					//data->RequestEndGame( EMS_End_PlayerLost ); This case is handled by custom logic in scripts
				}
				break;
			}
		case EMS_Started:
			{
				if ( knockedDown )
				{
					data->m_state = EMS_Wait_PlayerLost;
				}
				else 
				{
					Bool allDefeated = true;

					for ( Uint32 i = 0; i < data->m_enemies.Size(); i++)
					{
						CNewNPC* npc = data->m_enemies[i].Get();
						Bool knockedDown = true;
						CallFunctionRet< Bool >( npc, CNAME( IsKnockedUnconscious ), knockedDown );
						Bool isAlive = false;
						CallFunctionRet< Bool >( npc, CNAME( IsAlive ), isAlive );

						allDefeated = allDefeated && ( knockedDown || !isAlive );
					}
					if ( allDefeated )
					{
						data->RequestEndGame( EMS_End_PlayerWon );
					}
				}
				break;
			}
		default:
			RED_ASSERT( 0 );
			break;
		}
	}

	return data->IsFinished();
}

void CFistfightMinigame::OnEndGame( CMinigameInstanceData* _data ) const 
{
	CFistfightMinigameInstanceData* data = static_cast< CFistfightMinigameInstanceData* >( _data );

	if ( CBehTreeReactionManager* reactions = GCommonGame->GetBehTreeReactionManager() )
	{
		reactions->SuppressScaredReactions( false );
	}

	CPlayer* player = GCommonGame->GetPlayer();
	RemoveActor( data, player );

	for ( Uint32 i=0; i < data->m_enemies.Size(); ++i )
	{
		RemoveActor( data, data->m_enemies[i].Get() );
	}

	//TODO this blackscreen seems to be redundant and possibly could be removed
	if ( !data->m_wasFadeCalled && m_endsWithBlackscreen && ! GGame->EndGameRequested() )
	{
		data->DoBlacksceen();
	}
	
	ReleaseArea( data );
}

void CFistfightMinigame::OnStartGame( CMinigameInstanceData* _data ) const 
{
	CFistfightMinigameInstanceData* data = static_cast< CFistfightMinigameInstanceData* >( _data );

	if ( CPlayer* player = GCommonGame->GetPlayer() )
	{
		CallFunction( player, CNAME( SetFistFightParams ), m_toTheDeath, m_endsWithBlackscreen );
	}

	PrepareArea( data );

	AddActor( data, CNAME(PLAYER), m_playerPosTag );

	for ( Uint32 i=0; i < m_enemies.Size(); ++i )
	{
		CActor* enemy = AddActor( data, m_enemies[i].m_npcTag, m_enemies[i].m_startingPosTag );
		if ( enemy )
		{
			data->m_enemies.PushBack( static_cast<CNewNPC*>( enemy ) );
		}
	}

	if ( data->m_activeActors.Size() == ( m_enemies.Size() + 1 ) )
	{
		GCommonGame->ResetFadeLock( TXT("Fistfight_minigame") );
		GCommonGame->StartFade( true, TXT("Fistfight_minigame_start") );
		data->m_state = EMS_Started;
	}

	if ( CBehTreeReactionManager* reactions = GCommonGame->GetBehTreeReactionManager() )
	{
		reactions->SuppressScaredReactions( true );
	}
}

void CFistfightMinigame::RemoveActor( CFistfightMinigameInstanceData* data, CActor* actor ) const
{
	if ( !actor )
	{
		data->m_state = EMS_End_Error;
		return;
	}
	actor->CallEvent( CNAME( OnEndFistfightMinigame ) );
}

CActor* CFistfightMinigame::AddActor( CFistfightMinigameInstanceData* data, CName actorTag, CName startPosTag ) const
{
	if ( data->m_activeActors.Exist( actorTag ) )
	{ 
		return nullptr;
	}

	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	CNode* startPos = ( startPosTag != CName::NONE ) ? tagMgr->GetTaggedNode( startPosTag ) : NULL;

	CActor* actor = nullptr;
	if ( actorTag == CNAME(PLAYER) )
	{
		actor = GCommonGame->GetPlayer();
	}
	else
	{
		TDynArray<CEntity*> ents;
		tagMgr->CollectTaggedEntities( actorTag, ents );
		for ( Uint32 i = 0; i < ents.Size() && !actor; i++  )
		{
			actor = Cast<CActor>( ents[i] );
		}
	}
		
	if( !actor )
	{
		return nullptr;
	}
	if ( startPosTag && !startPos )
	{
		RED_LOG_ERROR( CNAME( Assert ), TXT("Could not find actor \"%s\" or his starting pos \"%s\" "), actorTag.AsChar(), startPosTag.AsChar() ); 
		data->m_state = EMS_End_Error;
		return nullptr;
	}
	if( startPos )
	{
		actor->Teleport( startPos );
		actor->SignalGameplayEvent( CNAME( AI_ForceInterruption ) );
	}
	actor->CallEvent( CNAME( OnStartFistfightMinigame ) );
	data->m_activeActors.PushBack( actorTag );
	return actor;
}

void CFistfightMinigame::PrepareArea( CFistfightMinigameInstanceData* data ) const
{
	CWorld* minigameWorld = GGame->GetActiveWorld();
	CTagManager* tagMgr = minigameWorld->GetTagManager();

	TDynArray< CNode*> area;
	tagMgr->CollectTaggedNodes( m_fightAreaTag, area );

	if ( m_fightAreaTag && area.Empty() )
	{
		RED_LOG_ERROR( CNAME( Assert ), TXT("No denied areas found with specified tag \"%s\" "), m_fightAreaTag.AsAnsiChar() ); 
	}

	for ( Uint32 i = 0; i < area.Size(); i++)
	{
		CDeniedAreaSaveable* deniedArea = Cast< CDeniedAreaSaveable >( area[i] );
		if ( deniedArea )
		{
			deniedArea->SetEnabled( true );
			data->m_area.PushBack( deniedArea );
		}	
	}
}

void CFistfightMinigame::ReleaseArea( CFistfightMinigameInstanceData* data ) const
{
	for ( Uint32 i = 0; i < data->m_area.Size(); i++)
	{
		CDeniedAreaSaveable* deniedArea = data->m_area[i].Get();
		if ( deniedArea )
		{
			deniedArea->SetEnabled( false );
		}
	}
	data->m_area.ClearFast();
}
