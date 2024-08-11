/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/engine/appearanceComponent.h"

#include "storySceneCutsceneSection.h"
#include "storySceneInput.h"
#include "storySceneInputStructureListener.h"
#include "storySceneActorMap.h"
#include "sceneLog.h"
#include "communitySystem.h"
#include "storySceneSystem.h"
#include "storySceneFlowSwitch.h"
#include "storySceneRandomizer.h"
#include "storySceneFlowCondition.h"
#include "storySceneScriptingBlock.h"

IMPLEMENT_RTTI_ENUM( EStorySceneForcingMode )
IMPLEMENT_RTTI_ENUM( ESoundStateDuringScene )
IMPLEMENT_ENGINE_CLASS( CStorySceneInput )

#define DEFAULT_MAPPING			0
#define FIRST_USABLE_MAPPING	1
#define PIVOT_MAPPING			FIRST_USABLE_MAPPING
#define PIVOT_ACTOR				0

/************************************************************************/
/* CStorySceneInput                                                     */
/************************************************************************/
CStorySceneInput::CStorySceneInput()
	: m_inputName(TXT("Input"))
	, m_isImportant( false )
	, m_isGameplay( true )
	, m_musicState( SOUND_DEFAULT )
	, m_ambientsState( SOUND_DEFAULT )
	, m_sceneNearPlane( NP_Further40cm )
	, m_sceneFarPlane( FP_DefaultEnv )
	, m_dontStopByExternalSystems( true )
	, m_maxActorsStaryingDistanceFromPlacement( 25.0f )
	, m_maxActorsStartingDistanceFormPlayer( 50.0f )
	, m_enableIntroVehicleDismount( true )
	, m_enableIntroLookAts( false )
	, m_introTotalTime( 2.0f )
	, m_enableIntroFadeOut( true )
	, m_introFadeOutStartTime( 1.5f )
	, m_blockSceneArea( true )
	, m_enableDestroyDeadActorsAround( true )
{
}

void CStorySceneInput::OnPostLoad()
{
	m_isImportant = true;
	m_isGameplay  = true;

	// DIALOG_TOMSIN_TODO - bo dostane zawalu...
	TDynArray<const CStorySceneLinkElement*> elements;
	GetAllNextLinkedElements( elements );
	for ( Uint32 i = 0; (m_isGameplay || m_isImportant) && i < elements.Size(); ++i )
	{
		const CStorySceneSection * section = Cast<const CStorySceneSection>( elements[i] );
		if ( section )
		{
			if ( ! section->IsImportant() )
			{
				m_isImportant = false;
			}
			if ( section->IsGameplay() == false && section->IsValid() == true )
			{
				m_isGameplay = false;
			}
		}
	}
}

String CStorySceneInput::GetName() const
{
	return m_inputName;
}

String CStorySceneInput::GetFriendlyName() const
{
	if( GetScene() == NULL )
	{
		return TBaseClass::GetFriendlyName();
	}

	return String::Printf( TXT( "%s, %s" ), GetScene()->GetDepotPath().AsChar(), m_inputName.AsChar() );
}

void CStorySceneInput::SetName( String name )
{
	m_inputName = name;
	NotifyAboutNameChanged();
}

void CStorySceneInput::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT( "inputName" ) )
	{
		NotifyAboutNameChanged();
	}
}

void CStorySceneInput::AddListener( IStorySceneInputStructureListener* listener )
{
	m_listeners.PushBack( listener );
}

void CStorySceneInput::NotifyAboutNameChanged()
{
	for ( TDynArray< IStorySceneInputStructureListener* >::iterator listener = m_listeners.Begin();
		listener != m_listeners.End(); ++listener )
	{
		(*listener)->OnNameChanged( this );
	}
}

void CStorySceneInput::RemoveListener( IStorySceneInputStructureListener* listener )
{
	m_listeners.Remove( listener );
}

void CStorySceneInput::OnConnected( CStorySceneLinkElement* linkedToElement )
{
	for ( TDynArray< IStorySceneInputStructureListener* >::iterator listener = m_listeners.Begin();
		listener != m_listeners.End(); ++listener )
	{
		(*listener)->OnLinksChanged( this );
	}
}

void CStorySceneInput::OnDisconnected( CStorySceneLinkElement* linkedToElement )
{
	for ( TDynArray< IStorySceneInputStructureListener* >::iterator listener = m_listeners.Begin();
		listener != m_listeners.End(); ++listener )
	{
		(*listener)->OnLinksChanged( this );
	}
}

void CStorySceneInput::GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const
{
	if ( m_dialogsetInstanceName )
	{
		if ( const CStorySceneDialogsetInstance* dialogset = GetScene()->GetDialogsetByName( m_dialogsetInstanceName ) )
		{
			dialogset->CollectRequiredTemplates( requiredTemplates );
		}
	}
}

void CStorySceneInput::CollectVoiceTags( TDynArray< CName >& voiceTags ) const
{
	voiceTags.Clear();
	TDynArray< const CStorySceneLinkElement* > connectedElements;
	GetAllNextLinkedElements( connectedElements );
	const CStoryScene* scene = GetScene();
	if ( !scene )
	{
		return;
	}
	if ( m_dialogsetInstanceName )
	{
		const CStorySceneDialogsetInstance* sectionDialogset = scene->GetDialogsetByName( m_dialogsetInstanceName );		
		if ( sectionDialogset )
		{
			sectionDialogset->GetAllActorNames( voiceTags );
		}
	}
	for ( Uint32 i = 0; i < connectedElements.Size(); ++i )
	{
		if ( connectedElements[ i ]->IsA< CStorySceneCutsceneSection >() == true )
		{
			const CStorySceneCutsceneSection* cutsceneSection = Cast< const CStorySceneCutsceneSection >( connectedElements[ i ] );
			cutsceneSection->GetVoiceTags( voiceTags, true );
		}
		else if ( connectedElements[ i ]->IsExactlyA< CStorySceneSection >() == true )
		{
			const CStorySceneSection* section = static_cast< const  CStorySceneSection* >( connectedElements[ i ] );			
			section->GetVoiceTags( voiceTags, true );
			if( section->GetDialogsetChange() )
			{
				const CStorySceneDialogsetInstance* sectionDialogset = scene->GetDialogsetByName( section->GetDialogsetChange() );		
				if ( sectionDialogset )
				{
					sectionDialogset->GetAllActorNames( voiceTags );
				}
			}
		}
	}
}


void CStorySceneInput::FillVoicetagMappings()
{
	TDynArray<CName> voicetags;
	CollectVoiceTags( voicetags );

	TDynArray<CName> mappedVoicetags;
	Bool             hasDefaultEntry = false;

	for ( Uint32 i = m_voicetagMappings.Size(); i > 0; --i )
	{
		CStorySceneVoicetagMapping & mapping = m_voicetagMappings[ i-1 ];
		
		if ( mapping.m_voicetag == CName::NONE && hasDefaultEntry == false && i == 0 )
		{
			hasDefaultEntry = true;
		}
		else
		if ( voicetags.Exist( mapping.m_voicetag ) == false )
		{
			m_voicetagMappings.EraseFast( m_voicetagMappings.Begin() + (i-1) );
		}
		else
		{
			mappedVoicetags.PushBack( mapping.m_voicetag );
		}
	}

	m_voicetagMappings.Reserve( voicetags.Size()+1 );

	if ( ! hasDefaultEntry )
	{
		m_voicetagMappings.Insert( DEFAULT_MAPPING, CStorySceneVoicetagMapping() );
	}

	for ( Uint32 i = 0; i < voicetags.Size(); ++i )
	{
		if ( mappedVoicetags.Exist( voicetags[i] ) == false )
		{
			m_voicetagMappings.PushBack( CStorySceneVoicetagMapping( voicetags[i] ) );
		}
	}
}

/************************************************************************/
/* Some other methods                                                   */
/************************************************************************/

void CStorySceneInput::Get2dArrayPropertyAdditionalProperties( IProperty *property, SConst2daValueProperties &valueProperties )
{
}
