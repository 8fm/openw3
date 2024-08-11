/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "questGraphSocket.h"

#include "questScenePrepare.h"
#include "storySceneInput.h"
#include "storySceneSection.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CQuestScenePrepareBlock )

void CQuestScenePrepareBlock::OnExecute( InstanceBuffer& data ) const
{
	Uint32& currentSceneIndex = data[ i_currentSceneIndex ];
	
	if ( currentSceneIndex >= m_storyScenes.Size() )
	{
		return;
	}

	Bool& resourcesCollected = data[ i_resourcesCollected ];
	TDynArray< TSoftHandle< CResource > >& sceneResources = data[ i_sceneResources ];

	if ( m_storyScenes[ currentSceneIndex ].m_scene.GetAsync( true ) == BaseSoftHandle::ALR_Loaded && resourcesCollected == false )
	{
		if( const CStoryScene* storyScene = m_storyScenes[ currentSceneIndex ].m_scene.Get() )
		{
			if ( const CStorySceneInput* input = storyScene->FindInput( m_storyScenes[ currentSceneIndex ].m_input.AsString() ) )
			{
				TDynArray< const CStorySceneLinkElement* > inputLinkElements;
				const_cast< CStorySceneInput* >( input )->GetAllNextLinkedElements( inputLinkElements );
				for ( Uint32 j = 0; j < inputLinkElements.Size(); ++j )
				{
					const CStorySceneControlPart* linkedSection = Cast< const CStorySceneControlPart >( inputLinkElements[ j ] );
					if ( linkedSection != NULL )
					{
						linkedSection->GetRequiredTemplates( sceneResources );
					}
				}
			}

			resourcesCollected = true;
		}	
	}

	Bool isEverythingLoaded = true;
	for ( TDynArray< TSoftHandle< CResource > >::const_iterator resourceIter = sceneResources.Begin();
		resourceIter != sceneResources.End(); ++resourceIter )
	{
		isEverythingLoaded &= resourceIter->GetAsync() == BaseSoftHandle::ALR_Loaded;
	}

	if ( isEverythingLoaded == true )
	{
		currentSceneIndex += 1;
		resourcesCollected = false;
	}
}

void CQuestScenePrepareBlock::OnDeactivate( InstanceBuffer& data ) const
{
	TDynArray< TSoftHandle< CResource > >& sceneResources = data[ i_sceneResources ];
	for ( TDynArray< TSoftHandle< CResource > >::const_iterator resourceIter = sceneResources.Begin();
		resourceIter != sceneResources.End(); ++resourceIter )
	{
		resourceIter->Release();
	}
	sceneResources.Clear();

	for ( TDynArray< StorySceneDefinition >::const_iterator sceneIter = m_storyScenes.Begin();
		sceneIter != m_storyScenes.End(); ++sceneIter )
	{
		sceneIter->m_scene.Release();
	}
}

void CQuestScenePrepareBlock::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );
	compiler << i_sceneResources;
	compiler << i_resourcesCollected;
	compiler << i_currentSceneIndex;
}

void CQuestScenePrepareBlock::OnInitInstance( InstanceBuffer& instanceData ) const
{
	TBaseClass::OnInitInstance( instanceData );
	instanceData[ i_sceneResources ].Clear();
	instanceData[ i_resourcesCollected ] = false;
	instanceData[ i_currentSceneIndex ] = 0;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CQuestScenePrepareBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	CreateSocket( CQuestGraphSocketSpawnInfo( CNAME( In ), LSD_Input, LSP_Left ) );
}

String CQuestScenePrepareBlock::GetCaption() const
{
	if ( m_storyScenes.Size() == 1 )
	{
		String sceneFileName = m_storyScenes[ 0 ].m_scene.GetPath().StringAfter( TXT( "\\" ), true ).StringBefore( TXT( "." ), true );
		return String::Printf( TXT( "Prepare scene [ %s, %s ]" ), sceneFileName.AsChar(), m_storyScenes[ 0 ].m_input.AsString().AsChar() );
	}
	return String::Printf( TXT( "Prepare %d scenes" ), m_storyScenes.Size() );
}

#endif
