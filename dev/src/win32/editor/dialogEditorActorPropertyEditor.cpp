/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "dialogEditorActorPropertyEditor.h"
#include "dialogTimeline.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneElement.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneItems.h"
#include "dialogEditor.h"

CEdDialogEditorActorPropertyEditor::CEdDialogEditorActorPropertyEditor( CPropertyItem* propertyItem, Int32 propertyType  )
	: ISelectionEditor( propertyItem )
	, m_propertyType( propertyType )
{

}

void CEdDialogEditorActorPropertyEditor::FillChoices()
{
	// Fill tree with entries from dialog timeline
	// NOTE: this is for use only from within CEdSceneEditor.
	wxClassInfo* sceneEditorClassInfo = wxClassInfo::FindClass( wxT( "CEdSceneEditor" ) );
	ASSERT( sceneEditorClassInfo != NULL );
	ASSERT( m_propertyItem != NULL );

	wxWindow* sceneEditorWindow = m_propertyItem->GetPage();
	while ( sceneEditorWindow != NULL && sceneEditorWindow->IsKindOf( sceneEditorClassInfo ) == false )
	{
		sceneEditorWindow = sceneEditorWindow->GetParent();
	}

	ASSERT( sceneEditorWindow != NULL );
	CEdSceneEditor* sceneEditor = static_cast< CEdSceneEditor* >( sceneEditorWindow );

	if ( sceneEditor == NULL )
	{
		return;
	}

	TDynArray< CName > tags = sceneEditor->OnVoicetagSelector_GetActorIds( m_propertyType );
	
	if ( CStoryScene* storyScene = sceneEditor->HACK_GetStoryScene() )
	{

#define COLLECT_IDS( _type, _array )								\
	if ( m_propertyType & _type  )									\
	{																\
		for ( auto i = _array.Begin(); i != _array.End(); ++i )		\
		{															\
			if(*i)													\
			{														\
				tags.PushBackUnique( (*i)->m_id );					\
			}														\
		}															\
	} 

		COLLECT_IDS( AT_ACTOR, storyScene->GetSceneActorsDefinitions() ) 
		COLLECT_IDS( AT_PROP, storyScene->GetScenePropDefinitions() ) 
		COLLECT_IDS( AT_EFFECT, storyScene->GetSceneEffectDefinitions() ) 
		COLLECT_IDS( AT_LIGHT, storyScene->GetSceneLightDefinitions() ) 
#undef COLLECT_IDS

	}

	m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	for( TDynArray< CName >::const_iterator tagIter = tags.Begin();
		tagIter != tags.End(); ++tagIter )
	{
		m_ctrlChoice->AppendString( tagIter->AsString().AsChar() );
	}

	// Set previous value
	CName value;
	m_propertyItem->Read( &value );
	m_ctrlChoice->SetStringSelection( value.AsString().AsChar() );
}

CEdDialogEditorActorVoiceTagEditor::CEdDialogEditorActorVoiceTagEditor( CPropertyItem* propertyItem )
	: ISelectionEditor( propertyItem )
{
}

void CEdDialogEditorActorVoiceTagEditor::FillChoices()
{
	// NOTE: this is for use only from within CEdSceneEditor.
	wxClassInfo* sceneEditorClassInfo = wxClassInfo::FindClass( wxT( "CEdSceneEditor" ) );
	ASSERT( sceneEditorClassInfo != NULL );
	ASSERT( m_propertyItem != NULL );

	wxWindow* sceneEditorWindow = m_propertyItem->GetPage();
	while ( sceneEditorWindow != NULL && sceneEditorWindow->IsKindOf( sceneEditorClassInfo ) == false )
	{
		sceneEditorWindow = sceneEditorWindow->GetParent();
	}

	ASSERT( sceneEditorWindow != NULL );
	CEdSceneEditor* sceneEditor = static_cast< CEdSceneEditor* >( sceneEditorWindow );

	if ( sceneEditor == NULL )
	{
		return;
	}

	CStoryScene* storyScene = sceneEditor->HACK_GetStoryScene();
	if ( storyScene == NULL )
	{
		return;
	}

	// get actor def
	CStorySceneActor* actorDef = Cast<CStorySceneActor>( m_propertyItem->GetParentObject( 0 ).AsObject() );
	m_ctrlChoice->AppendString( CName::NONE.AsString().AsChar() );
	if ( actorDef && actorDef->m_entityTemplate.Get() )
	{
		// get voice tags stored in actor def entity template
		const TDynArray< VoicetagAppearancePair >& voicetagAppearances = actorDef->m_entityTemplate.Get()->GetVoicetagAppearances();
		for ( Uint32 i = 0; i < voicetagAppearances.Size(); ++i )
		{
			if( voicetagAppearances[ i ].m_voicetag == CName::NONE )
			{
				continue;
			}

			m_ctrlChoice->AppendString( voicetagAppearances[ i ].m_voicetag.AsString().AsChar() );
		}
	}
	else if( storyScene )
	{
		// fall back... list all actor defs in scene...
		TDynArray< CName > tags;
		storyScene->CollectVoicetags( tags, true );
		for( TDynArray< CName >::const_iterator tagIter = tags.Begin(); tagIter != tags.End(); ++tagIter )
		{
			m_ctrlChoice->AppendString( tagIter->AsString().AsChar() );
		}
	}


	// Set previous value
	CName value;
	m_propertyItem->Read( &value );
	m_ctrlChoice->SetStringSelection( value.AsString().AsChar() );
}
