#include "build.h"
#include "questContextDialogBlock.h"
#include "questGraphSocket.h"
#include "questsSystem.h"
#include "storySceneInput.h"
#include "storySceneOutput.h"
#include "storyScenePlaybackListener.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "storySceneSection.h"
#include "questSceneBlocksValidator.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../core/feedback.h"
#include "../engine/graphConnectionRebuilder.h"
#include "storySceneDebug.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

namespace // anonymous
{
	class CQuestStoryEndListener : public IStoryScenePlaybackListener
	{
	private:
		TSoftHandle< CStoryScene >	m_targetScene;
		TSoftHandle< CStoryScene >	m_injectedScene;
		Bool						m_ended;
		Bool						m_injectedEnded;
		CName						m_outputName;

	public:
		CQuestStoryEndListener( TSoftHandle< CStoryScene > injectedScene, TSoftHandle< CStoryScene > targetScene )
			: m_targetScene( targetScene )
			, m_injectedScene( injectedScene )
			, m_ended( false )
			, m_injectedEnded( false )
		{
		}

		virtual ~CQuestStoryEndListener()
		{
		}

		virtual void OnEnd( CStoryScenePlayer* player, Bool stopped ) override
		{
			if ( stopped == false )
			{
				SCENE_ASSERT( player );

				m_ended = true;
				if ( !m_injectedEnded )
				{
					m_outputName = player->GetActivatedOutputName();
				}
				player->RemovePlaybackListener( this );
			}
		}

		virtual void OnInjectedReturnDialogEnd( CStoryScenePlayer* player, const CName activatedOutputName, TSoftHandle< CStoryScene > _injectedScene, TSoftHandle< CStoryScene > _targetScene ) override
		{
			SCENE_ASSERT( !m_injectedScene.IsEmpty() );
			SCENE_ASSERT( !m_targetScene.IsEmpty() );

			if ( m_injectedScene == _injectedScene && m_targetScene == _targetScene )
			{
				m_injectedEnded = true;
				m_outputName = activatedOutputName;
				GCommonGame->GetSystem< CQuestsSystem >()->UnregisterContextDialog( _injectedScene, _targetScene, this );
			}
		}

		RED_INLINE Bool HasEnded() const { return m_ended; }
		RED_INLINE const CName& GetOutput() const { return m_outputName; }
		RED_INLINE void Activate() { m_ended = false; m_injectedEnded = false; }
	};
} // anonymous

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQuestContextDialogBlock )

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CQuestContextDialogBlock::GetCaption() const
{
	return m_name;
}

void CQuestContextDialogBlock::SetScene( CStoryScene* scene )
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
		ERR_GAME( TXT( "CQuestContextDialogBlock class is corrupt - 'scene' property not found" ) );
		return;
	}

	TSoftHandle< CStoryScene > handle = scene;
	sceneProperty->Set( this, &handle );
}

void CQuestContextDialogBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	// Create sockets
	//CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( Cut ), LSD_Input, LSP_Center ) );

	// Create scene interface sockets (if a scene is set)
	if ( m_scene.Get() != NULL )
	{
		RebuildSceneInterfaceSockets();
	}
}

void CQuestContextDialogBlock::RebuildSceneInterfaceSockets()
{
	CStoryScene* scene = m_scene.Get();
	if ( scene == NULL )
	{
		return;
	}

	// inputs
	TDynArray< CStorySceneInput* > sceneInputs;
	scene->CollectControlParts< CStorySceneInput >( sceneInputs );

	Uint32 inputCount = sceneInputs.Size();
	for ( Uint32 i = 0; i < inputCount; ++i )
	{
		CreateSocket( CQuestGraphSocketSpawnInfo( CName( sceneInputs[ i ]->GetName() ), LSD_Input, LSP_Left ) );
		SSceneInjectedChoiceLineInfo injectionInfo;
		if ( GetInjectedDialogDescription( sceneInputs[ i ], injectionInfo ) == false )
		{
			GFeedback->ShowWarn( TXT( "Input '%ls' does not have a injected choice description. It will be ignored in gameplay" ), sceneInputs[ i ]->GetName().AsChar() );
		}
	}

	// outputs
	TDynArray< CStorySceneOutput* > sceneOutputs;
	scene->CollectControlParts< CStorySceneOutput >( sceneOutputs );

	Uint32 outputCount = sceneOutputs.Size();
	for ( Uint32 i = 0; i < outputCount; ++i )
	{
		if ( sceneOutputs[ i ]->IsQuestOutput() )
		{
			CreateSocket( CQuestGraphSocketSpawnInfo( CName( sceneOutputs[ i ]->GetName() ), LSD_Output, LSP_Right ) );
		}
	}

	// release the resource for now - it won't be needed until the execution
	m_scene.Release();
}

void CQuestContextDialogBlock::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("scene") )
	{
		OnRebuildSockets();
	}
}

Bool CQuestContextDialogBlock::NeedsUpdate() const
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

void CQuestContextDialogBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_playbackListener;
	compiler << i_activated;
}

void CQuestContextDialogBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );

	instanceData[ i_playbackListener ] = 0;
	instanceData[ i_activated ] = PHASE_NotActive;
}

void CQuestContextDialogBlock::OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const
{
	TBaseClass::OnActivate( data, inputName, parentThread );

	if( data[ i_activated ] != PHASE_NotActive )
	{
		return;
	}

	ASSERT( data[ i_playbackListener ] == NULL );

	// load the resource in async, we'll just poll for it in OnExecute()
	m_scene.GetAsync( true );

	// mark	as "loading"
	data[ i_activated ] = PHASE_LoadingScene;
}

void CQuestContextDialogBlock::OnExecute( InstanceBuffer& data ) const
{
	TBaseClass::OnExecute( data );	
	ASSERT( data[ i_activated ] != PHASE_NotActive );

	if ( data[ i_activated ] == PHASE_LoadingScene )
	{
		BaseSoftHandle::EAsyncLoadingResult res = m_scene.GetAsync( true );
		if ( res == BaseSoftHandle::ALR_Failed )
		{
			// Should be a data err or sth, but there's no time
			return;
		}

		if ( res == BaseSoftHandle::ALR_InProgress )
		{
			return;
		}

		if ( res == BaseSoftHandle::ALR_Loaded )
		{
			CQuestStoryEndListener* listener = new CQuestStoryEndListener( m_scene, m_targetScene );
			data[ i_playbackListener ] = reinterpret_cast< TGenericPtr >( listener );
			data[ i_activated ] = PHASE_Activating;
		}
	}

	ASSERT( data[ i_playbackListener ] );
	CQuestStoryEndListener* listener = reinterpret_cast< CQuestStoryEndListener* >( data[ i_playbackListener ] );

	if ( data[ i_activated ] == PHASE_Activating )
	{
		TDynArray< String > inputNames;
		GetInputNames( data, inputNames );

		THashMap< String, SSceneInjectedChoiceLineInfo > injectedChoices;
		if ( GetChoices( inputNames, injectedChoices ) )
		{
			if ( listener )
			{
				listener->Activate();
			}

			GCommonGame->GetSystem< CQuestsSystem >()->RegisterContextDialog( m_scene, m_targetScene, listener, injectedChoices );
			data[ i_activated ] = PHASE_Active;
		}
		else
		{
			return;
		}
	}

	if ( listener && listener->HasEnded() )
	{
		const CName& output = listener->GetOutput();
		if ( ActivateOutput( data, output, true ) == false )
		{
			listener->Activate();
		}
	}

}

void CQuestContextDialogBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TBaseClass::OnDeactivate( data );

	CQuestStoryEndListener* listener = reinterpret_cast< CQuestStoryEndListener* >( data[ i_playbackListener ] );
	CStorySceneSystem* storySceneSystem = GCommonGame->GetSystem< CStorySceneSystem >();
	if ( storySceneSystem != nullptr && listener && listener->HasEnded() == false )
	{
		storySceneSystem->UnregisterPlaybackListenerFromActiveScenes( listener );
	}

	data[ i_playbackListener ] = NULL;
	data[ i_activated ] = PHASE_NotActive;

	CQuestsSystem* questsSystem = GCommonGame->GetSystem< CQuestsSystem >();
	if ( questsSystem != nullptr )
	{
		questsSystem->UnregisterContextDialog( m_scene, m_targetScene, listener );
	}

	delete listener;

	m_scene.Release();
}

Bool CQuestContextDialogBlock::GetChoices( THashMap< String, SSceneInjectedChoiceLineInfo >& outChoices ) const
{
	TDynArray< String > inputNames;
	return GetChoices( inputNames, outChoices );
}

Bool CQuestContextDialogBlock::GetChoices( const TDynArray< String >& activeInputs, THashMap< String, SSceneInjectedChoiceLineInfo >& outChoices ) const
{
	if ( m_scene.GetAsync( true ) == BaseSoftHandle::ALR_InProgress )
	{
		return false;
	}

	CStoryScene* loadedScene = m_scene.Get();

	if ( !loadedScene )
	{
		m_scene.Release();
		return true;
	}

	TDynArray< CStorySceneInput* > sceneInputs;
	loadedScene->CollectControlParts< CStorySceneInput >( sceneInputs );
	Uint32 count = sceneInputs.Size();
	if ( count == 0 )
	{
		m_scene.Release();
		return true;
	}

	String description;
	Bool isAnyLineToReturnToChoice = false;
	for ( Uint32 j = 0; j < count; ++j )
	{
		// Collect injections only from activated inputs. If no specific inputs are given, then collect injections from all scene inputs
		if ( activeInputs.Empty() == false && activeInputs.Exist( sceneInputs[ j ]->GetName() ) == false )
		{
			continue;
		}

		SSceneInjectedChoiceLineInfo injectedLineInfo;
		if ( GetInjectedDialogDescription( sceneInputs[ j ], injectedLineInfo ) == true )
		{
			outChoices.Insert( sceneInputs[ j ]->GetName(), injectedLineInfo );
			if ( injectedLineInfo.m_returnToChoice )
			{
				isAnyLineToReturnToChoice = true;
			}
		}
	}

	if ( !isAnyLineToReturnToChoice )
	{
		m_scene.Release();
	}
	return true;
}

Bool CQuestContextDialogBlock::GetInjectedDialogDescription( CStorySceneInput* input, SSceneInjectedChoiceLineInfo& injectedLineInfo ) const
{
	ASSERT( input );
	if ( !input )
	{
		return false;
	}

	TDynArray< const CStorySceneLinkElement* > linkedElements;
	input->GetAllNextLinkedElements( linkedElements );

	const CStorySceneSection* section;

	Uint32 linkedElemsCount = linkedElements.Size();
	for ( Uint32 i = 0; i < linkedElemsCount; ++i )
	{
		section = Cast< const CStorySceneSection >( linkedElements[ i ] );
		if ( !section )
		{
			continue;
		}

		if ( section->HasQuestChoiceLine() )
		{
			section->GetQuestChoiceLine()->FillInjectedLineInfo( injectedLineInfo );
			return true;
		}
	}

	return false;

}

void CQuestContextDialogBlock::GetInputNames( InstanceBuffer& instanceData, TDynArray< String >& inputNames ) const
{
	const TDynArray< CName >& activeInputs = GetActiveInputs( instanceData );
	if ( activeInputs.Size() == 1 && activeInputs[ 0 ] == CNAME( In ) )
	{
		// Backwards compatibility:
		// If the only block input is 'In' then we have block in previous version - so do not return any specific inputs, this will result in including all inputs
		return;
	}

	for ( TDynArray< CName >::const_iterator nameIter = activeInputs.Begin(); nameIter != activeInputs.End(); ++nameIter )
	{
		inputNames.PushBack( nameIter->AsString() );
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
