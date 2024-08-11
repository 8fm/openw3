#include "build.h"
#include "questGraphSocket.h"
#include "questSceneBlock.h"
#include "storySceneInput.h"
#include "storySceneOutput.h"
#include "questScenePlayer.h"
#include "questSceneBlocksValidator.h"
#include "questThread.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/gameSave.h"
#include "../engine/graphConnectionRebuilder.h"
#include "storySceneUtils.h"
#include "storySceneDebug.h"
#include "storySceneSystem.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CQuestSceneBlock );
IMPLEMENT_RTTI_ENUM( EQuestSceneSaveMode );

CQuestSceneBlock::CQuestSceneBlock() 
	: m_forcingMode( SSFM_SpawnWhenNeeded )
	, m_shouldFadeOnLoading( false )
{ 
	m_name = TXT("Scene"); 
}

void CQuestSceneBlock::SetScene( CStoryScene* scene )
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
		ERR_GAME( TXT( "CQuestSceneBlock class is corrupt - 'scene' property not found" ) );
		return;
	}

	TSoftHandle< CStoryScene > handle = scene;
	sceneProperty->Set( this, &handle );
}

void CQuestSceneBlock::CollectContent( IQuestContentCollector& collector ) const
{
	TBaseClass::CollectContent( collector );

	if ( !m_scene.GetPath().Empty() )
	{
		collector.CollectResource( m_scene.GetPath(), m_playGoChunk );
	}
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestSceneBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create mandatory sockets
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );

	// Create scene interface sockets (if a scene is set)
	RebuildSceneInterfaceSockets();
}

void CQuestSceneBlock::RebuildSceneInterfaceSockets()
{
	// if a new story has been set, we want to check the inputs and outputs
	// it has and update the block accordingly
	CStoryScene* scene = m_scene.Get();
	if ( scene == NULL )
	{
		return;
	}

	TDynArray< CStorySceneInput* > sceneInputs;
	scene->CollectControlParts< CStorySceneInput >( sceneInputs );

	Uint32 count = sceneInputs.Size();
	for ( Uint32 i = 0; i < count; ++i )
	{
		CName inputName = CName( sceneInputs[ i ]->GetName() );

		CreateSocket( CQuestGraphSocketSpawnInfo( inputName, LSD_Input, LSP_Left ) );
	}

	
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

void CQuestSceneBlock::CookSceneData()
{
	m_shouldFadeOnLoading_NamesCooked.Clear();
	m_shouldFadeOnLoading_ValuesCooked.Clear();

	if ( const CStoryScene* scene = m_scene.Get() )
	{
		StorySceneUtils::ShouldFadeOnLoading( scene, m_shouldFadeOnLoading_NamesCooked, m_shouldFadeOnLoading_ValuesCooked );

		m_scene.Release();
	}
}

void CQuestSceneBlock::CleanupSourceData()
{
	TBaseClass::CleanupSourceData();

	CookSceneData();
}

void CQuestSceneBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("scene") )
	{
		OnRebuildSockets();
	}
}

class CQuestSceneBlockPredicate
{
public:
	Bool operator()( const CStorySceneControlPart* controlPart1, const CStorySceneControlPart* controlPart2 ) const
	{
		if ( controlPart1 == controlPart2 )
		{
			return false;
		}

		const CStorySceneOutput* output1	= Cast< CStorySceneOutput >( controlPart1 );
		const CStorySceneOutput* output2	= Cast< CStorySceneOutput >( controlPart2 );
		if ( !output1 && !output2 )
		{
			return controlPart1 < controlPart2;
		}
		if ( !output1 && output2 )
		{
			return true;
		}
		if ( output1 && !output2 )
		{
			return false;
		}

		const String& outputName1	= output1->GetOutputName().AsString();
		const String& outputName2	= output2->GetOutputName().AsString();
		if ( outputName1 == outputName2 )
		{
			return output1 < output2;
		}
		return outputName1 < outputName2;
	}
};

void CQuestSceneBlock::SortOutputBlocks()
{
	CStoryScene* scene = m_scene.Get();
	if ( scene == NULL )
	{
		return;
	}
	
	if ( MarkModified() && scene->MarkModified() )
	{
		TDynArray< CStorySceneControlPart* >& controlParts = scene->GetControlParts();
		Sort( controlParts.Begin(), controlParts.End(), CQuestSceneBlockPredicate() );
	}

	m_scene.Release();
}

Bool CQuestSceneBlock::NeedsUpdate() const
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

void CQuestSceneBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_player;
}

void CQuestSceneBlock::OnInitInstance( InstanceBuffer& data ) const
{
	TBaseClass::OnInitInstance( data );

	data[ i_player ] = NULL;
}

void CQuestSceneBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if ( inputName != CName::NONE )
	{
		Bool shouldFadeOnLoading = m_shouldFadeOnLoading || ShouldFadeOnLoading_Cooked( inputName );

		const CStorySceneSystem* sss = GCommonGame->GetSystem< CStorySceneSystem >();
		if ( sss->IsCurrentlyPlayingAnyCinematicScene() && shouldFadeOnLoading )
		{
			// We cannot force black screen during playing another cinematic dialog
			shouldFadeOnLoading = false;
		}

		CQuestScenePlayer* player = new CQuestScenePlayer( m_scene, m_forcingMode, inputName.AsString(), m_interrupt, shouldFadeOnLoading );

		player->Play();
		data[ i_player ] = reinterpret_cast< TGenericPtr >( player );
	}
	else
	{
		data[ i_player ] = NULL;
	}
}

void CQuestSceneBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );

	CQuestScenePlayer* player = reinterpret_cast< CQuestScenePlayer* >( data[ i_player ] );
	if ( !player )
	{
		return;
	}

	player->Execute();

	// TBD: assume quest scene should be paused whether gameplay or not
	player->Pause( GGame->IsLoadingScreenShown() );

	WaitForSceneToEnd( data );
}

Bool CQuestSceneBlock::HasLoadedSection( const InstanceBuffer& data ) const
{
	CQuestScenePlayer* player = reinterpret_cast< CQuestScenePlayer* >( data[ i_player ] );
	if ( !player )
	{
		return true; // not not-loaded
	}

	return player->HasLoadedSection();
}

void CQuestSceneBlock::OnDeactivate( InstanceBuffer& data ) const
{
	CQuestScenePlayer* player = reinterpret_cast< CQuestScenePlayer* >( data[ i_player ] );
	delete player;
	data[ i_player ] = NULL;

	TBaseClass::OnDeactivate( data );
}

void CQuestSceneBlock::WaitForSceneToEnd( InstanceBuffer& data ) const
{
	CQuestScenePlayer* player = reinterpret_cast< CQuestScenePlayer* >( data[ i_player ] );

	// check if the scene has ended
	if ( player && player->IsOver() )
	{
		ActivateOutput( data, player->GetOutput() );

		delete player;
		data[ i_player ] = NULL;

		// unload the scene resource
		m_scene.Release();
	}
}

Bool CQuestSceneBlock::ShouldFadeOnLoading_Cooked( const CName& inputName ) const
{
	RED_FATAL_ASSERT( m_shouldFadeOnLoading_NamesCooked.Size() == m_shouldFadeOnLoading_ValuesCooked.Size(), "" );

	const Uint32 size = m_shouldFadeOnLoading_NamesCooked.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_shouldFadeOnLoading_NamesCooked[ i ] == inputName )
		{
			return m_shouldFadeOnLoading_ValuesCooked[ i ];
		}
	}

	// We can not check it here. Quest can set an input that is not exist in story scene so we wouldn't find it in cooked array. 
	//RED_FATAL_ASSERT( m_shouldFadeOnLoading_NamesCooked.Size() == 0, "CQuestSceneBlock::ShouldFadeOnLoading_Cooked - scene input was not cooked properly" );
	return false;
}

void CQuestSceneBlock::SaveGame( InstanceBuffer& data, IGameSaver* saver )
{
	TBaseClass::SaveGame( data, saver );

	// Scene input
	CQuestScenePlayer* player = reinterpret_cast< CQuestScenePlayer* >( data[ i_player ] );
	if ( player )
	{
		saver->WriteValue( CNAME( input ), player->GetInputName() );
	}
}

void CQuestSceneBlock::LoadGame( InstanceBuffer& data, IGameLoader* loader )
{
	TBaseClass::LoadGame( data, loader );

	// Close existing player
	CQuestScenePlayer* player = reinterpret_cast< CQuestScenePlayer* >( data[ i_player ] );
	if ( player )
	{
		delete player;
		data[ i_player ] = NULL;
	}

	// Load the input that was used to activate the scene
	String inputName;	
	loader->ReadValue( CNAME( input ), inputName );

	// We should have some input
	if ( !inputName.Empty() )
	{
		// Restart
		CQuestScenePlayer* player = new CQuestScenePlayer( m_scene, m_forcingMode, inputName, m_interrupt, false );
		player->Play();
		data[ i_player ] = reinterpret_cast< TGenericPtr >( player );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
