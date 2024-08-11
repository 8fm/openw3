/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityEditor.h"

#include "../../common/engine/appearanceComponent.h"

#include "../../common/game/actor.h"
#include "../../common/game/actorSpeech.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneAbstractLine.h"
#include "../../common/game/storySceneComponent.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/engine/mimicComponent.h"

namespace
{
	Bool EnumActorSpeeches( const CActor* actor, TDynArray< SEditorActorSpeech >& speeches, TDynArray< String >& names )
	{
		for ( ComponentIterator< CStorySceneComponent > it( actor ); it; ++it )
		{
			CStorySceneComponent * ssComponent = *it;
			if ( !ssComponent )
			{
				continue;
			}

			TSoftHandle< CStoryScene > sceneHandle = ssComponent->GetStoryScene();
			CStoryScene * scene = sceneHandle.Get();
			if ( !scene )
			{
				continue;
			}

			Uint32 sectionsNum = scene->GetNumberOfSections();

			for ( Uint32 i=0; i<sectionsNum; ++i )
			{
				CStorySceneSection* section	= scene->GetSection( i );

				if ( section && section->IsA< CStorySceneCutsceneSection >() == false )
				{
					TDynArray< CAbstractStorySceneLine* > dialogLines;
					section->GetLines( dialogLines );

					for ( Uint32 j=0; j<dialogLines.Size(); ++j )
					{
						if ( dialogLines[ j ]->IsA< CStorySceneLine >() )
						{
							CStorySceneLine* dialogLine = Cast< CStorySceneLine >( dialogLines[ j ] );

							if ( dialogLine->GetVoiceTag() == actor->GetVoiceTag() )
							{
								SEditorActorSpeech actorSpeech;
								actorSpeech.m_line = section->GetName() + TXT(" - ") + dialogLine->GetContent();
								actorSpeech.m_id = dialogLine->GetLocalizedContent()->GetIndex();
								speeches.PushBack( actorSpeech );
								names.PushBack( actorSpeech.m_line );
							}
						}
					}
				}
			}

			sceneHandle.Release();
		}

		return speeches.Size() > 0;
	}
}

void CEdEntityEditor::OnHeadStateNormal( wxCommandEvent& event )
{
	IActorInterface* actor = m_preview->GetEntity()->QueryActorInterface();
	if ( actor )
	{
		CMimicComponent* head = actor->GetMimicComponent();
		if ( head )
		{
			head->MimicHighOff();
		}
	}
}

void CEdEntityEditor::OnHeadStateMimicLow( wxCommandEvent& event )
{
	IActorInterface* actor = m_preview->GetEntity()->QueryActorInterface();
	if ( actor )
	{
		CMimicComponent* head = actor->GetMimicComponent();
		if ( head )
		{
			head->MimicHighOff();
		}
	}
}

void CEdEntityEditor::OnHeadStateMimicHigh( wxCommandEvent& event )
{
	IActorInterface* actor = m_preview->GetEntity()->QueryActorInterface();
	if ( actor )
	{
		CMimicComponent* head = actor->GetMimicComponent();
		if ( head )
		{
			head->MimicHighOn();
		}
	}
}

void CEdEntityEditor::OnHeadSpeechTest( wxCommandEvent& event )
{
	CActor* actor = Cast< CActor >( m_preview->GetEntity() );
	if ( actor )
	{
		Int32 flags = actor->HasMimic() ? ASM_Voice | ASM_Lipsync : ASM_Voice | ASM_Lipsync | ASM_Gameplay;
		ActorSpeechData data( m_currActorSpeech.m_id, UNICODE_TO_ANSI( m_currActorSpeech.m_line.AsChar() ), true, flags );
		actor->SpeakLine( data );
	}
}

void CEdEntityEditor::OnHeadSpeechSelect( wxCommandEvent& event )
{
	CActor* actor = Cast< CActor >( m_preview->GetEntity() );
	if ( actor )
	{
		TDynArray< SEditorActorSpeech > speaches;
		TDynArray< String >				names;

		if ( EnumActorSpeeches( actor, speaches, names ) )
		{
			SEditorActorSpeech def;
			def.m_id = DEFAULT_ACTOR_SPEECH_ID;
			def.m_line = String::EMPTY;

			speaches.PushBack( def );
			names.PushBack( TXT("Default") );

			String sel = InputComboBox( NULL, TXT("Entity Editor"), TXT("Select speech"), m_currActorSpeech.m_line, names );

			Bool found = false;

			for ( Uint32 i=0; i<speaches.Size(); ++i )
			{
				if ( speaches[ i ].m_line == sel )
				{
					m_currActorSpeech = speaches[ i ];
					found = true;
					break;
				}
			}

			if ( !found )
			{
				m_currActorSpeech = def;
			}

			wxCommandEvent fake;
			OnHeadSpeechTest( fake );
		}
	}
}
