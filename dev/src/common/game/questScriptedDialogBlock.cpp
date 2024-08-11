/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questScriptedDialogBlock.h"

#include "../../common/game/questGraphSocket.h"
#include "questSceneBlocksValidator.h"

#include "../../common/game/questsSystem.h"
#include "questScenePlayer.h"
#include "questExternalScenePlayer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestScriptedDialogBlock );

CQuestScriptedDialogBlock::CQuestScriptedDialogBlock()
{
	m_name = TXT( "Scripted dialog" );
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestScriptedDialogBlock::SetScene( CStoryScene* scene )
{
	ASSERT( scene );
	if ( scene == NULL )
	{
		return;
	}

	CProperty* sceneProperty = this->GetClass()->FindProperty( CNAME( scene ) );
	ASSERT( sceneProperty );
	if ( sceneProperty == NULL )
	{
		ERR_GAME( TXT( "CQuestScriptedDialogBlock class is corrupt - 'scene' property not found" ) );
		return;
	}

	TSoftHandle< CStoryScene > handle = scene;
	sceneProperty->Set( this, &handle );
}

void CQuestScriptedDialogBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("scene") )
	{
		OnRebuildSockets();
	}
}

void CQuestScriptedDialogBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
}

String CQuestScriptedDialogBlock::GetCaption() const
{
	String caption = m_name;

	if ( m_actorTags.Empty() == false )
	{
		caption += m_actorTags.ToString();
	}

	return caption;
}

Bool CQuestScriptedDialogBlock::NeedsUpdate() const
{
	CStoryScene* scene = m_scene.Get();
	if ( scene == NULL )
	{
		return false;
	}

	CQuestSceneBlocksValidator validator;
	Bool needsUpdate = validator.IsOutdated( scene, this );
	m_scene.Release();

	return needsUpdate;
}

#endif

void CQuestScriptedDialogBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_players;
}

void CQuestScriptedDialogBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
}

void CQuestScriptedDialogBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CQuestScenePlayer* player = new CQuestScenePlayer( m_scene, SSFM_SpawnWhenNeeded, String::EMPTY, false, false );
	data[ i_players ].PushBack( reinterpret_cast< TGenericPtr >( player ) );

	GCommonGame->GetSystem< CQuestsSystem >()->GetScriptedDialogPlayer()->RegisterDialog( *player, GetGUID(), m_actorTags );
}

void CQuestScriptedDialogBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	TDynArray< TGenericPtr >& players = data[ i_players ];
	Uint32 count = players.Size();
	CQuestScenePlayer* player;
	for ( Uint32 i = 0; i < count; ++i )
	{
		player = reinterpret_cast< CQuestScenePlayer* >( players[ i ] );
		player->Execute();
	}
}

void CQuestScriptedDialogBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	TDynArray< TGenericPtr >& players = data[ i_players ];
	Uint32 count = players.Size();
	CQuestScenePlayer* player;
	for ( Uint32 i = 0; i < count; ++i )
	{
		player = reinterpret_cast< CQuestScenePlayer* >( players[ i ] );
		GCommonGame->GetSystem< CQuestsSystem >()->GetScriptedDialogPlayer()->UnregisterDialog( *player );
		delete player;
	}
	players.Clear();
}