
#include "build.h"
#include "storySceneCutscene.h"
#include "storySceneCutsceneSection.h"
#include "storySceneCutsceneBlock.h"
#include "storySceneScriptLine.h"
#include "storySceneSection.h"
#include "storySceneCameraSetting.h"
#include "storyScene.h"
#include "storySceneLine.h"
#include "storySceneChoice.h"
#include "storySceneChoiceLine.h"
#include "storySceneDialogset.h"
#include "storySceneElement.h"
#include "storySceneEventDialogLine.h"
#include "../core/diskFile.h"
#include "../engine/localizationManager.h"
#include "../engine/extAnimCutsceneDialogEvent.h"
#include "../engine/tagManager.h"
#include "storySceneEventCsCamera.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( SCutsceneActorOverrideMapping )
IMPLEMENT_ENGINE_CLASS( CStorySceneCutsceneSection )

CStorySceneCutsceneSection::CStorySceneCutsceneSection() 
	: m_looped( false )
	, m_clearActorsHands( true )
{}

void CStorySceneCutsceneSection::OnSectionCreate() 
{
	CreatePlayer();
	UpdatePlayer();
	CreateDialogEvents();
}

void CStorySceneCutsceneSection::OnPropertyPreChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );

	if ( property->GetName() == TXT( "cutscene" ) &&
		 m_cutscene && GetScene() && GetScene()->GetFile() &&
		 m_cutscene->ReleaseFileWithThisCutscene( GetScene()->GetFile()->GetDepotPath() ) )
	{
#ifndef NO_RESOURCE_IMPORT
		m_cutscene->Save();
#endif
	}
}

void CStorySceneCutsceneSection::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "cutscene" ) )
	{

#ifndef NO_RESOURCE_IMPORT
		if ( m_cutscene && GetScene() && GetScene()->GetFile() &&
			 m_cutscene->SaveFileWithThisCutscene( GetScene()->GetFile()->GetDepotPath() ) )
		{
			m_cutscene->Save();
		}

		m_actorOverrides.Clear();	
#endif

		NotifyAboutSocketsChange();
	}

	CreatePlayer();
	UpdatePlayer();

	CreateDialogEvents();
}

void CStorySceneCutsceneSection::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	CreatePlayer();
	UpdatePlayer();

	CheckContent();

	CreateDialogEvents();

#ifndef NO_EDITOR

	// Approve duration of cutscene player. Cutscene player elements are auto-approved as they don't need to be
	// protected like CStorySceneLine elements - this is because users always have latest version of a cutscene
	// taken from version control system (with CStorySceneLine elements this is not the case as we don't store
	// our voiceovers in version control system so users may have bad voiceovers or no voiceovers at all).
	const CStorySceneCutscenePlayer* cutscenePlayer = GetPlayer();
	const Float duration = cutscenePlayer->CalculateDuration( SLocalizationManager::GetInstance().GetCurrentLocale() );
	TDynArray< CStorySceneSectionVariantId > variantIds;
	EnumerateVariants( variantIds );
	for( CStorySceneSectionVariantId variantId : variantIds )
	{
		ApproveElementDuration( variantId, cutscenePlayer->GetElementID(), duration );
	}

#endif // NO_EDITOR
}

void CStorySceneCutsceneSection::CheckContent()
{
	for ( Uint32 i = 0; i < m_sceneElements.Size(); ++i )
	{
		if ( m_sceneElements[ i ] == NULL )
		{
			continue;
		}

		if ( m_sceneElements[ i ]->IsA< CStorySceneScriptLine >() )
		{
			String fileName = GetScene() ? GetScene()->GetFriendlyName() : TXT("<unknown>");
			SCENE_ASSERT( !TXT("Cutscene section has got forbidden line. DEBUG!!! - file: %s"), fileName.AsChar() );
		}
	}

	for ( Uint32 i = 0; i < m_sceneElements.Size(); ++i )
	{
		if ( m_sceneElements[ i ] && m_sceneElements[ i ]->IsA< CStorySceneLine >() )
		{
			CStorySceneLine* line = static_cast< CStorySceneLine* >( m_sceneElements[ i ] );
			line->SetAsBackgroundLine( true );
		}
	}
}

/*

Note - this function processes all variants at once.
*/
void CStorySceneCutsceneSection::CreateDialogEvents()
{
	TDynArray< CStorySceneSectionVariantId > sectionVariantIds;
	EnumerateVariants( sectionVariantIds );

	for( auto sectionVariantId : sectionVariantIds )
	{
		RemoveAllEventsByClass< CStorySceneEventDialogLine >( sectionVariantId );
		RemoveAllEventsByClass< CStorySceneEventCsCamera >( sectionVariantId );

		CStorySceneElement* csPlayerElement = GetPlayer();
		CCutsceneTemplate*  csTemplate = m_cutscene.Get();
		SCENE_ASSERT( csPlayerElement );

		if ( csTemplate && csPlayerElement )
		{
			const Float duration = csPlayerElement->CalculateDuration( SLocalizationManager::GetInstance().GetCurrentLocale() );

			TDynArray< CExtAnimCutsceneDialogEvent* > csList;
			csTemplate->GetEventsOfType< CExtAnimCutsceneDialogEvent >( csList );

			const Uint32 csSize = csList.Size();
			const Uint32 elemSize = m_sceneElements.Size();

			Float lastEvtTime = 0.f;

			Uint32 csEvtCounter = 0;
			for ( Uint32 i=0; i<elemSize; ++i )
			{
				if ( CStorySceneLine* line = Cast< CStorySceneLine >( m_sceneElements[ i ] ) )
				{
					SCENE_ASSERT( line->IsBackgroundLine() );

					Float evtTime = 0.f;
					if ( csEvtCounter < csSize )
					{
						const CExtAnimCutsceneDialogEvent* csEvt = csList[ csEvtCounter++ ];
						evtTime = csEvt->GetStartTime();
					}
					else
					{
						evtTime = lastEvtTime + ( duration / (Float)elemSize );
					}

					evtTime = Clamp( evtTime, 0.f, duration ) / duration;

					CStorySceneEventDialogLine* e = new CStorySceneEventDialogLine( TXT("Cs line"), csPlayerElement, evtTime, TXT("CUTSCENE LINES"), line );
					AddEvent( e, sectionVariantId );

					lastEvtTime = evtTime;
				}
			}

			TDynArray<Float> cuts = csTemplate->GetCameraCuts();
			for( Float cut : cuts )
			{
				const Float startPosition = cut / duration;
				CStorySceneEventCsCamera* camEvt = new CStorySceneEventCsCamera( TXT("CS camera"), csPlayerElement, startPosition, TXT( "_Camera" )  );
				AddEvent( camEvt, sectionVariantId );
			}
		}
	}
}

void CStorySceneCutsceneSection::DestroyDialogEvents()
{
	TDynArray< CStorySceneSectionVariantId > sectionVariantIds;
	EnumerateVariants( sectionVariantIds );

	for( auto sectionVariantId : sectionVariantIds )
	{
		RemoveAllEventsByClass< CStorySceneEventDialogLine >( sectionVariantId );
		RemoveAllEventsByClass< CStorySceneEventCsCamera >( sectionVariantId );
	}
}

CClass* CStorySceneCutsceneSection::GetBlockClass() const
{ 
	return ClassID< CStorySceneCutsceneSectionBlock >();
}

void CStorySceneCutsceneSection::CreatePlayer()
{
	if ( GetNumberOfElements() == 0 || !GetPlayer() )
	{
		CStorySceneCutscenePlayer* player = CreateObject< CStorySceneCutscenePlayer >( this );

		player->SetCutsceneTemplate( m_cutscene.Get() );

		AddSceneElement( player, 0 );
		NotifyAboutElementAdded( player );
	}

	CleanJunkCutscenePlayers();

	ASSERT( GetNumberOfElements() > 0 );
}

CStorySceneCutscenePlayer* CStorySceneCutsceneSection::GetPlayer()
{
	return Cast< CStorySceneCutscenePlayer >( GetElement(0) );
}

const CStorySceneCutscenePlayer* CStorySceneCutsceneSection::GetPlayer() const
{
	return Cast<const CStorySceneCutscenePlayer >( GetElement(0) );
}

String CStorySceneCutsceneSection::GetDescriptionText() const
{ 
	const CStorySceneCutscenePlayer * player = GetPlayer();
	return player ? player->GetDescriptionText() : String(); 
}

void CStorySceneCutsceneSection::UpdatePlayer()
{
	ASSERT( GetPlayer() );

	if ( GetPlayer() )
	{
		GetPlayer()->SetCutsceneTemplate( m_cutscene.Get() );
	}
}

Bool CStorySceneCutsceneSection::IsValid() const
{
	if ( m_cutscene.IsValid() )
	{
		return true;
	}
	return TBaseClass::IsValid();
}

void CStorySceneCutsceneSection::CleanJunkCutscenePlayers()
{
	TDynArray< CStorySceneElement* > junkCutscenePlayers;
	for ( Uint32 i = 1; i < m_sceneElements.Size(); ++i )
	{
		if ( m_sceneElements[ i ] != NULL &&m_sceneElements[ i ]->IsA< CStorySceneCutscenePlayer >() == true )
		{
			junkCutscenePlayers.PushBack( m_sceneElements[ i ] );
		}
	}
	for ( Uint32 j = 0; j < junkCutscenePlayers.Size(); ++j )
	{
		m_sceneElements.Remove( junkCutscenePlayers[ j ] );
	}
	junkCutscenePlayers.Clear();
}

Bool CStorySceneCutsceneSection::CanAddComment( Uint32 index, Bool after ) const
{
	return index > 0 || after ? true : false;
}

Bool CStorySceneCutsceneSection::CanAddDialogLine( Uint32 index, Bool after ) const
{
	return index > 0 || after ? true : false;
}

Bool CStorySceneCutsceneSection::CanRemoveElement( CStorySceneElement* element ) const
{
	if ( element->IsA< CStorySceneCutscenePlayer >() )
	{
		return false;
	}
	else
	{
		return true;
	}
}

void CStorySceneCutsceneSection::AddSceneElement( CStorySceneElement* element, Uint32 index )
{
	TBaseClass::AddSceneElement( element, index );

	SCENE_ASSERT( element );
	SCENE_ASSERT( GetPlayer() );

	if ( element && element->IsA< CStorySceneLine >() )
	{
		CStorySceneLine* line = static_cast< CStorySceneLine* >( element );
		line->SetAsBackgroundLine( true );
	}
}

void CStorySceneCutsceneSection::GetDialogLines( TDynArray< SCutsceneActorLine >& lines ) const
{
	for ( Uint32 i=1; i<m_sceneElements.Size(); ++i )
	{
		CStorySceneLine* line = Cast< CStorySceneLine >( m_sceneElements[i] );
		if ( line )
		{
			SCutsceneActorLine actorLine;
				
			actorLine.m_actorVoicetag = line->GetVoiceTag().AsString();
			actorLine.m_text = line->GetContent();
			actorLine.m_lineIndex = line->GetLocalizedContent()->GetIndex();
			actorLine.m_sound = line->GetVoiceFileName();
			actorLine.m_soundEventName = line->GetSoundEventName();

			lines.PushBack( actorLine );
		}
	}
	if ( GetChoice() != NULL )
	{
		String mergedChoiceLines = TXT( "Choice" );

		for ( Uint32 j = 0; j < GetChoice()->GetNumberOfChoiceLines(); ++j )
		{
			const CStorySceneChoiceLine* choiceLine = GetChoice()->GetChoiceLine( j );
			if ( choiceLine == NULL )
			{
				continue;
			}

			mergedChoiceLines += TXT( "\n" ) + choiceLine->GetChoiceLine();
		}

		SCutsceneActorLine actorLine;
		actorLine.m_actorVoicetag = String::EMPTY;
		actorLine.m_text = mergedChoiceLines;
		actorLine.m_sound = String::EMPTY;
		actorLine.m_lineIndex = 0;
		actorLine.m_soundEventName = StringAnsi::EMPTY;

		lines.PushBack( actorLine );
	}
}

Bool CStorySceneCutsceneSection::HasFadeOut() const
{
	return m_cutscene ? m_cutscene->HasFadeAfter() : false;
}

Bool CStorySceneCutsceneSection::HasFadeIn() const
{
	return m_cutscene ? m_cutscene->HasFadeBefore() : false;
}

Bool CStorySceneCutsceneSection::GetCsOffset( Matrix& offset ) const
{
	const TagList& sectionTags = m_cutscene->GetPointTag();
	if ( !sectionTags.Empty() )
	{
		TDynArray< CNode* > nodes;
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( sectionTags, nodes );

		if ( !nodes.Empty() )
		{
			offset = nodes[0]->GetRotation().ToMatrix();
			offset.SetTranslation( nodes[0]->GetPosition() );
			return true;
		}
	}

	return false;
}

Bool CStorySceneCutsceneSection::GetSceneOffset( Matrix& offset ) const
{
	const TagList& sectionTags = GetTagPoint();
	if ( !sectionTags.Empty() )
	{
		TDynArray< CNode* > nodes;
		GGame->GetActiveWorld()->GetTagManager()->CollectTaggedNodes( sectionTags, nodes );

		if ( !nodes.Empty() )
		{
			offset = nodes[0]->GetRotation().ToMatrix();
			offset.SetTranslation( nodes[0]->GetPosition() );
			return true;
		}
	}

	return false;
}

Bool CStorySceneCutsceneSection::CanModifyCamera() const
{
	return true;
}

Bool CStorySceneCutsceneSection::GetCameraSettings( StorySceneCameraSetting& cameraSettings ) const
{
	if ( !m_cutscene )
	{
		return false;
	}

	// Get offset
	cameraSettings.m_offset = Matrix::IDENTITY;
	if ( !GetCsOffset( cameraSettings.m_offset ) )
	{
		GetSceneOffset( cameraSettings.m_offset );
	}

	// Get pose
	return m_cutscene->GetCameraInitialPose( cameraSettings.m_pose );
}

SCutsceneActorDef* CStorySceneCutsceneSection::FindCutsceneActorOverrideDefinition( const String& actorName )
{
	for ( Uint32 i = 0; i < m_actorOverrides.Size(); ++i )
	{
		if ( m_actorOverrides[ i ].m_actorName == actorName && m_actorOverrides[ i ].HasRelevantData() == true )
		{
			return &(m_actorOverrides[ i ].m_cutsceneActorDef);
		}
	}
	return NULL;
}

void CStorySceneCutsceneSection::GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const
{
	if ( m_cutscene.IsValid() )
	{
		m_cutscene->GetRequiredTemplates( requiredTemplates );
	}
	
	for ( TDynArray< SCutsceneActorOverrideMapping >::const_iterator overrideIter = m_actorOverrides.Begin();
		overrideIter != m_actorOverrides.End(); ++overrideIter )
	{
		const SCutsceneActorOverrideMapping& actorOverride = *overrideIter;
		requiredTemplates.PushBackUnique( TSoftHandle< CResource >( actorOverride.m_cutsceneActorDef.m_template.GetPath() ) );
	}
}

void CStorySceneCutsceneSection::GetVoiceTags( TDynArray< CName >& voiceTags, Bool append/*=false */ ) const
{
	if ( m_cutscene.IsValid() )
	{
		m_cutscene->GetActorsVoicetags( voiceTags );
	}
}


Bool CStorySceneCutsceneSection::HasActor( CName actorId ) const
{
	if ( m_cutscene.IsValid() )
	{
		return m_cutscene->HasActor( actorId );
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool SCutsceneActorOverrideMapping::HasRelevantData() const
{
	// Validate cutscene override mapping
	return m_actorName.Empty() == false && 
		(	m_cutsceneActorDef.m_voiceTag != CName::NONE 
		||	m_cutsceneActorDef.m_tag.Empty() == false
		||	m_cutsceneActorDef.m_template.GetPath().Empty() == false );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
