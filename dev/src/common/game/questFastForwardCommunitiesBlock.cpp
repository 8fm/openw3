/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questFastForwardCommunitiesBlock.h"

#include "../core/instanceDataLayoutCompiler.h"

#include "../engine/graphConnectionRebuilder.h"

#include "commonGame.h"
#include "gameFastForwardSystem.h"
#include "gameWorld.h"
#include "questGraphSocket.h"

IMPLEMENT_ENGINE_CLASS( CQuestFastForwardCommunitiesBlock )

void CQuestFastForwardCommunitiesBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_timeLimit;
}
void CQuestFastForwardCommunitiesBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_timeLimit ] = EngineTime::ZERO;
}

void CQuestFastForwardCommunitiesBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	if ( m_timeLimit >= 0.f )
	{
		data[ i_timeLimit ] = GCommonGame->GetEngineTime() +  m_timeLimit;
	}

	CGameFastForwardSystem* fastForward = GCommonGame->GetSystem< CGameFastForwardSystem >();

	
	CEntity* player = GCommonGame->GetPlayerEntity();
	Vector referencePos = player
		? player->GetWorldPositionRef()
		: GCommonGame->GetActiveWorld()->GetCameraPosition();
	
	SFastForwardExecutionParameters executionParams( referencePos );
	executionParams.m_isExternallyTicked = false;
	executionParams.m_dontSpawnHostilesClose = m_dontSpawnHostilesClose;
	executionParams.m_despawnExistingGuyz = m_respawnEveryone;
	executionParams.m_handleBlackScreen = m_manageBlackscreen;

	fastForward->BeginFastForward( Move( executionParams ) );

	TBaseClass::OnActivate( data, inputName, parentThread );
}

void CQuestFastForwardCommunitiesBlock::OnExecute( InstanceBuffer& data ) const
{
	CGameFastForwardSystem* fastForward = GCommonGame->GetSystem< CGameFastForwardSystem >();

	if ( m_timeLimit >= 0.f )
	{
		if ( data[ i_timeLimit ] < GCommonGame->GetEngineTime() )
		{
			fastForward->RequestFastForwardShutdown();
		}
	}

	if ( fastForward->IsFastForwardCompleted() )
	{
		ActivateOutput( data, CNAME( Out ), true );
	}

	
}

void CQuestFastForwardCommunitiesBlock::OnDeactivate( InstanceBuffer& data ) const
{
	CGameFastForwardSystem* fastForward = GCommonGame->GetSystem< CGameFastForwardSystem >();
	fastForward->EndFastForward();
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

//! CGraphBlock interface
EGraphBlockShape CQuestFastForwardCommunitiesBlock::GetBlockShape() const
{
	return GBS_Rounded;
}
Color CQuestFastForwardCommunitiesBlock::GetClientColor() const
{
	return Color( 135, 40, 156 );
}
void CQuestFastForwardCommunitiesBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Out ), LSD_Output, LSP_Right ) );
}
String CQuestFastForwardCommunitiesBlock::GetBlockCategory() const
{
	return TXT( "Gameplay" );
}
Bool CQuestFastForwardCommunitiesBlock::CanBeAddedToGraph( const CQuestGraph* graph ) const
{
	return true;
}

#endif