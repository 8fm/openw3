#include "build.h"
#include "questInteractionDialogBlock.h"
#include "questGraphSocket.h"
#include "questsSystem.h"
#include "storySceneInput.h"
#include "storySceneOutput.h"
#include "questScenePlayer.h"
#include "questSceneBlocksValidator.h"
#include "questExternalScenePlayer.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestInteractionDialogBlock )

void CQuestInteractionDialogBlock::CollectContent( IQuestContentCollector& collector ) const
{
	TBaseClass::CollectContent( collector );

	if ( !m_scene.GetPath().Empty() )
	{
		collector.CollectResource( m_scene.GetPath() );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CQuestInteractionDialogBlock::GetCaption() const
{
	String caption = m_name;
	
	if ( !m_actorTags.Empty() )
	{
		caption += m_actorTags.ToString();
	}

	return caption;
}

void CQuestInteractionDialogBlock::SetScene( CStoryScene* scene )
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
		ERR_GAME( TXT( "CQuestInteractionDialogBlock class is corrupt - 'scene' property not found" ) );
		return;
	}

	TSoftHandle< CStoryScene > handle = scene;
	sceneProperty->Set( this, &handle );
}

void CQuestInteractionDialogBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );

	RebuildSceneInterfaceSockets();
}

void CQuestInteractionDialogBlock::RebuildSceneInterfaceSockets()
{
	CStoryScene* scene = m_scene.Get();
	if ( scene == NULL )
	{
		return;
	}

	// inputs
	TDynArray< CStorySceneInput* > sceneInputs;
	scene->CollectControlParts< CStorySceneInput >( sceneInputs );

	Uint32 count = sceneInputs.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CName inputName = CName( sceneInputs[ i ]->GetName() );

		CreateSocket( CQuestGraphSocketSpawnInfo( inputName, LSD_Input, LSP_Left ) );
	}

	// outputs
	TDynArray< CStorySceneOutput* > sceneOutputs;
	scene->CollectControlParts< CStorySceneOutput >( sceneOutputs );

	count = sceneOutputs.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CName( sceneOutputs[ i ]->GetName() ), LSD_Output, LSP_Right ) );
	}

	// release the resource for now - it won't be needed until the execution
	m_scene.Release();
}

void CQuestInteractionDialogBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("scene") )
	{
		OnRebuildSockets();
	}
}

Bool CQuestInteractionDialogBlock::NeedsUpdate() const
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

void CQuestInteractionDialogBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_players;
}

void CQuestInteractionDialogBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	CQuestScenePlayer* player = new CQuestScenePlayer( m_scene, SSFM_SpawnWhenNeeded, inputName.AsString(), m_interrupt, false );

	data[ i_players ].PushBack( reinterpret_cast< TGenericPtr >( player ) );

	GCommonGame->GetSystem< CQuestsSystem >()->GetInteractionDialogPlayer()->RegisterDialog( *player, GetGUID(), m_actorTags );
}

void CQuestInteractionDialogBlock::OnExecute( InstanceBuffer& data ) const 
{
	TBaseClass::OnExecute( data );

	TDynArray< TGenericPtr >& players = data[ i_players ];
	Uint32 count = players.Size();
	CQuestScenePlayer* player;
	for ( Uint32 i = 0; i < count; ++i )
	{
		player = reinterpret_cast< CQuestScenePlayer* >( players[ i ] );
		player->Execute();

		if ( player->IsOver() == false )
		{
			continue;
		}
		
		const CName& output = player->GetOutput();
		if ( ActivateOutput( data, output, true ) )
		{
			break;
		}
		
		player->Reinitialize();
	}
}

void CQuestInteractionDialogBlock::OnDeactivate( InstanceBuffer& data ) const 
{
	TBaseClass::OnDeactivate( data );

	TDynArray< TGenericPtr >& players = data[ i_players ];
	Uint32 count = players.Size();
	CQuestScenePlayer* player;
	for ( Uint32 i = 0; i < count; ++i )
	{
		player = reinterpret_cast< CQuestScenePlayer* >( players[ i ] );
		GCommonGame->GetSystem< CQuestsSystem >()->GetInteractionDialogPlayer()->UnregisterDialog( *player );
		delete player;
	}
	players.Clear();
}

