/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "../../common/game/minigame.h"
#include "../../common/game/questGraphSocket.h"

#include "dicePokerBlock.h"
/*
IMPLEMENT_ENGINE_CLASS( CDicePokerBlock )

const CName CDicePokerBlock::OUT_PLAYER_WON( TXT( "Player won" ) );
const CName CDicePokerBlock::OUT_PLAYER_LOST( TXT( "Player lost" ) );

CDicePokerBlock::CDicePokerBlock()
	: CQuestGraphBlock()
	, m_minigameTemplate( NULL )
	, m_spawnPointTag()
	, m_opponentTag()
	, m_endWithBlackscreen( false )
	, i_minigame()
{
	m_name = TXT( "Dice poker" ); 
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CDicePokerBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create inputs
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );

	// Create outputs
	CreateSocket( CQuestGraphSocketSpawnInfo( OUT_PLAYER_WON, LSD_Output, LSP_Right ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( OUT_PLAYER_LOST, LSD_Output, LSP_Right ) );
}

#endif

void CDicePokerBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_minigame;
}

void CDicePokerBlock::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_minigame ] = NULL;
}

void CDicePokerBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CMinigame* minigame = StartMinigame();
	if( minigame == NULL )
	{
		// Something failed, send output anyway to prevent blockers
		ThrowErrorNonBlocking( data, OUT_PLAYER_LOST, TXT( "Cannot start minigame" ) );
		return;
	}

	data[ i_minigame ] = minigame;
}

void CDicePokerBlock::OnExecute( InstanceBuffer& data ) const
{
	CMinigame* minigame = data[ i_minigame ];
	if( minigame == NULL )
	{
		return;
	}

	if ( minigame->IsStarted() )
	{
		// Continue waiting...
		return;
	}

	// Minigame is finished
	Bool playerWon = minigame->HasPlayerWon();
	EndMinigame( minigame );
	data[ i_minigame ] = NULL;

	// Activate output
	if( playerWon )
	{
		ActivateOutput( data, OUT_PLAYER_WON );
	}
	else
	{
		ActivateOutput( data, OUT_PLAYER_LOST );
	}
}

CMinigame* CDicePokerBlock::StartMinigame() const
{
	// Check if template is present
	if( m_minigameTemplate == NULL )
	{
		MINIGAME_ERROR( TXT( "No entity template defined" ) );
		return NULL;
	}
	
	// Find opponent
	TDynArray< CEntity* > tities;
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( m_opponentTag, tities );

	if ( tities.Size() == 0 )
	{
		MINIGAME_ERROR( TXT( "Opponent with tag '%s' not found" ), m_opponentTag.AsString().AsChar() );
		return NULL;
	}
	else if( tities.Size() > 1 )
	{
		MINIGAME_WARN( TXT( "Multiple opponents with tag '%s' found, using first" ), m_opponentTag.AsString().AsChar() );
	}

	CActor* player = GCommonGame->GetPlayer();
	CActor* opponent = Cast< CActor >( tities[ 0 ] );
	ASSERT( player != NULL );
	ASSERT( opponent != NULL );

	//playerResetMovment();
	
	// Get spawn position from waypoint
	Vector spawnPosition( Vector::ZEROS );

	tities.ClearFast();
	GGame->GetActiveWorld()->GetTagManager()->CollectTaggedEntities( m_spawnPointTag, tities );

	if ( tities.Size() == 0 )
	{
		MINIGAME_WARN( TXT( "No spawn point with tag '%s' found, using (0, 0, 0)" ), m_spawnPointTag.AsString().AsChar() );
	}
	else
	{
		if( tities.Size() > 1 )
		{
			MINIGAME_WARN( TXT( "Multiple spawn points with tag '%s' found, using first" ), m_spawnPointTag.AsString().AsChar() );
		}

		spawnPosition = tities[ 0 ]->GetWorldPositionRef();
	}
	
	// Spawn game
	EntitySpawnInfo info;
	info.m_template = m_minigameTemplate.Get();
	info.m_spawnPosition = spawnPosition;

	CMinigame* minigame = static_cast< CMinigame* >( GGame->GetActiveWorld()->GetDynamicLayer()->CreateEntitySync( info ) );
	
	//theGame.EnableButtonInteractions( false );
	
	// Restore screen either it was faded or not
	GGame->StartFade( true , TXT( "Minigame" ), 2.0 );
	
	// START GAME
	TDynArray< THandle< CActor > > players;
	players.PushBack( player );
	players.PushBack( opponent );
	if ( ! minigame->StartGame( players ) )
	{
		MINIGAME_ERROR( TXT( "Dice poker failed to start" ) );
		return NULL;
	}

	return minigame;
}

void CDicePokerBlock::EndMinigame( CMinigame* minigame ) const
{
	//theGame.EnableButtonInteractions( true );

	// Despawn minigame
	minigame->Destroy();

	m_minigameTemplate.Release();
}

Bool CDicePokerBlock::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == TXT("minigameTemplate") )
	{
		CEntityTemplate *templ = *( CEntityTemplate **) readValue.GetData();

		m_minigameTemplate = templ;

		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}
*/