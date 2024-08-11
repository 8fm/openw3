#include "build.h"
#include "questGraphMinigameBlock.h"
#include "questGraphSocket.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestGraphMinigameBlock );

#ifndef NO_EDITOR_GRAPH_SUPPORT
void CQuestGraphMinigameBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Success ), LSD_Output, LSP_Right ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Failure ), LSD_Output, LSP_Right ) );
}
#endif

void CQuestGraphMinigameBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_minigameData;
}

void CQuestGraphMinigameBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_minigameData ] = nullptr;
}

void CQuestGraphMinigameBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );		
}

void CQuestGraphMinigameBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );
	CMinigameInstanceData* minigameData = (CMinigameInstanceData*)data[ i_minigameData ];
	if ( m_minigame && data[ i_minigameData ] )
	{
		m_minigame->EndGame( minigameData );
		data[ i_minigameData ] = nullptr;
	}
}

void CQuestGraphMinigameBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	if ( !m_minigame )
	{
		return;
	}

	CMinigameInstanceData* minigameData = (CMinigameInstanceData*)data[ i_minigameData ];
	if ( minigameData == nullptr )
	{
		minigameData = m_minigame->StartGame();
		data[ i_minigameData ] = TGenericPtr( minigameData );
	}	
	else if( !m_minigame->HasStarted( minigameData ) )
	{
		m_minigame->StartGame( minigameData );
	}
	else if ( m_minigame->IsFinished( minigameData ) )
	{		
		if ( minigameData->HasPlayerWon() )
		{
			ActivateOutput( data, CNAME( Success ), true );
		}
		else
		{
			ActivateOutput( data, CNAME( Failure ), true );
		}	
		m_minigame->EndGame( minigameData );
		data[ i_minigameData ] = nullptr;
	}
}

