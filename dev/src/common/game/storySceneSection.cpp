/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneSection.h"

#include "storySceneComment.h"
#include "storySceneChoice.h"
#include "storySceneLine.h"
#include "storySceneScriptLine.h"
#include "storyScenePauseElement.h"
#include "storySceneBlockingElement.h"
#include "storySceneSectionBlock.h"
#include "storySceneCutscene.h"
#include "storySceneCutsceneSection.h"
#include "storySceneEvent.h"
#include "storySceneInput.h"
#include "storySceneItems.h"

// to remove after resave
#include "storySceneEventDuration.h"
#include "storySceneEventAnimation.h"
#include "storySceneEventEnterActor.h"
#include "storySceneEventExitActor.h"
#include "storySceneEventLookat.h"
#include "storySceneEventCustomCamera.h"
#include "storySceneEventCustomCameraInstance.h"
#include "storySceneEventDespawn.h"
#include "storySceneEventFade.h"
#include "storySceneEventBlend.h"
#include "storySceneEventInterpolation.h"

#include "../core/fileSkipableBlock.h"
#include "storySceneEventPoseKey.h"

#include "../engine/localizationManager.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventInfo )
IMPLEMENT_ENGINE_CLASS( CStorySceneLocaleVariantMapping )
IMPLEMENT_ENGINE_CLASS( CStorySceneSection )

RED_DEFINE_NAME( GAMEPLAY )
RED_DEFINE_NAME( IMPORTANT )

CStorySceneSection::CStorySceneSection(void)
	: m_choice( NULL )
	, m_hasCinematicOneliners( false )
	, m_fadeInAtBeginning( true )
	, m_fadeOutAtEnd( true )
	, m_pauseInCombat( false )
	, m_canBeSkipped( true )
	, m_manualFadeIn( false )
	, m_allowCameraMovement( true )
	, m_numberOfInputPaths( 1 )
	, m_canHaveLookats( true )
	, m_forceDialogset( true )
	, m_streamingLock( false )
	, m_streamingUseCameraPosition( true )
	, m_streamingCameraAllowedJumpDistance( 50.0f )
	, m_contexID( 0 )
	, m_nextVariantId( 0 )
	, m_defaultVariantId( -1 )
	, m_blockMusicTriggers( false )
	, m_maxBoxExtentsToApplyHiResShadows( -1.0f )
	, m_distantLightStartOverride( -1.0f )
	#ifndef NO_EDITOR
		, m_variantIdChosenInEditor( -1 )
		, m_variantIdForcedInEditor( -1 )
	#endif // !NO_EDITOR
{
}

CStorySceneSection::~CStorySceneSection()
{
	for( CStorySceneSectionVariant* sv : m_variants )
	{
		delete sv;
	}

	for( CStorySceneEventInfo* ei : m_eventsInfo )
	{
		delete ei;
	}

	for( CStorySceneEvent* ev : m_events )
	{
		delete ev;
	}

	for( CStorySceneLocaleVariantMapping* localeVariantMapping : m_localeVariantMappings )
	{
		delete localeVariantMapping;
	}
}

CStorySceneComment* CStorySceneSection::AddComment( Uint32 index /*= -1 */ )
{
	CStorySceneComment* comment = CreateObject< CStorySceneComment >( this );
	AddSceneElement( comment, index );
	return comment;
}

CAbstractStorySceneLine* CStorySceneSection::AddDialogLine( Uint32 index /*= -1 */ )
{
	CStorySceneLine* line = CreateObject< CStorySceneLine >( this );
	AddSceneElement( line, index );
	return line;
}

CAbstractStorySceneLine* CStorySceneSection::AddDialogLineAfter( CStorySceneElement* element )
{
	Uint32 index = ( element != NULL ) ? static_cast< Uint32 >( m_sceneElements.GetIndex( element ) + 1 ) : 0;
	ASSERT( index >= 0 && index <= m_sceneElements.Size() );
	CStorySceneLine* line = CreateObject< CStorySceneLine >( this );
	AddSceneElement( line, index );
	return line;
}

CStorySceneScriptLine* CStorySceneSection::AddScriptLine( Uint32 index /*= -1 */ )
{
	CStorySceneScriptLine* line = CreateObject< CStorySceneScriptLine >( this );
	CStorySceneScript* sceneScript = CreateObject< CStorySceneScript >( line ); 
	line->SetStorySceneScript( sceneScript );
	AddSceneElement( line, index );
	return line;
}

CStorySceneScriptLine* CStorySceneSection::AddScriptLineAfter( CStorySceneElement* element )
{
	Uint32 index = ( element != NULL ) ? static_cast< Uint32 >( m_sceneElements.GetIndex( element ) + 1 ) : 0;
	ASSERT( index >= 0 && index <= m_sceneElements.Size() );

	CStorySceneScriptLine* line = CreateObject< CStorySceneScriptLine >( this );
	CStorySceneScript* sceneScript = CreateObject< CStorySceneScript >( line ); 
	line->SetStorySceneScript( sceneScript );
	AddSceneElement( line, index );
	return line;
}

CStorySceneChoice* CStorySceneSection::AddChoice()
{
	if ( !MarkModified_Pre() )
	{
		return nullptr;
	}

	if ( m_choice != NULL || m_numberOfInputPaths != 1 )
	{
		return NULL;
	}
	m_choice = CreateObject< CStorySceneChoice >( this );
	
	// Generate element ID as section requires all elements to have proper ID set.
	m_choice->GenerateElementID();
	RED_FATAL_ASSERT( !m_choice->GetElementID().Empty(), "CStorySceneSection::AddChoice(): couldn't generate ID for scene element." );

	// Insert choice into all variants.
	for( CStorySceneSectionVariant* sv : m_variants )
	{
		sv->InsertElement( m_choice );
	}

	NotifyAboutSocketsChange();

	MarkModified_Post();

	return m_choice;
}

CStorySceneQuestChoiceLine* CStorySceneSection::SetQuestChoice()
{
	ASSERT( !HasQuestChoiceLine() , TXT( "Can't have two quest choice lines in a single section." ) );

	CStorySceneQuestChoiceLine* line = CreateObject< CStorySceneQuestChoiceLine >( this );
	AddSceneElement( line, 0 );

	return line;
}

/*
Removes element and all associated events (from all section variants).

\param element Element to be removed.
*/
void CStorySceneSection::RemoveElement( CStorySceneElement* element )
{
	RED_FATAL_ASSERT( element, "CStorySceneSection::RemoveElement(): element must not be nullptr." );

	if( element == m_choice )
	{
		RemoveChoice();
		return;
	}

	if( !MarkModified_Pre() )
	{
		return;
	}

	// Get all events (from all variants) associated with element and remove them.
	TDynArray< const CStorySceneEvent* > elementEvents;
	for( CStorySceneSectionVariant* sv : m_variants )
	{
		GetEventsForElement( elementEvents, element, sv->m_id );
	}

	for( const CStorySceneEvent* ev : elementEvents )
	{
		RemoveEvent( ev->GetGUID() );
	}

	for( CStorySceneSectionVariant* sv : m_variants )
	{
		sv->RemoveElement( element->GetElementID() );
	}

	SCENE_VERIFY( m_sceneElements.Remove( element ) );

	MarkModified_Post();
}

/*
Removes choice element and all associated events (from all section variants).
*/
void CStorySceneSection::RemoveChoice()
{
	if ( !MarkModified_Pre() )
	{
		return;
	}

	if ( m_choice )
	{
		// Get all events associated with choice element.
		TDynArray< const CStorySceneEvent* > elementEvents;
		for( CStorySceneSectionVariant* sv : m_variants )
		{
			GetEventsForElement( elementEvents, m_choice, sv->m_id );
		}

		for( const CStorySceneEvent* ev : elementEvents )
		{
			RemoveEvent( ev->GetGUID() );
		}

		for( CStorySceneSectionVariant* sv : m_variants )
		{
			sv->RemoveElement( m_choice->GetElementID() );
		}
	}

	m_choice = nullptr;

	NotifyAboutSocketsChange();

	MarkModified_Post();
}

void CStorySceneSection::RemoveQuestChoice()
{
	if ( HasQuestChoiceLine() )
	{
		RemoveElement( GetElement( 0 ) );
	}
}

CStorySceneElement* CStorySceneSection::GetElement( Uint32 index )
{
	if ( index >= m_sceneElements.Size() )
	{
		return NULL;
	}
	return m_sceneElements[ index ];
}

const CStorySceneElement* CStorySceneSection::GetElement( Uint32 index ) const 
{
	if ( index >= m_sceneElements.Size() )
	{
		return NULL;
	}
	return m_sceneElements[ index ];
}

CStorySceneChoice* CStorySceneSection::GetChoice()
{
	return m_choice;
}

const CStorySceneChoice* CStorySceneSection::GetChoice() const 
{
	return m_choice;
}

const CStorySceneQuestChoiceLine* CStorySceneSection::GetQuestChoiceLine() const
{
	for ( Uint32 i = 0; i < m_sceneElements.Size(); ++i )
	{
		if( m_sceneElements[ i ] ==  NULL )
		{
			continue;
		}
		else if ( m_sceneElements[ i ]->IsExactlyA< CStorySceneQuestChoiceLine >() == true )
		{
			return Cast< CStorySceneQuestChoiceLine >( m_sceneElements[ i ] );
		}
		else if ( m_sceneElements[ i ]->IsA< CStorySceneComment >() == false )
		{
			break;
		}
	}
	return NULL;
}

/*
Gets number of scene elements (excluding section choice).

\return Number of scene elements in section (excluding section choice).

Section choice is also a scene element but it's kept separately from other
scene elements - this is why this function ignores it.
*/
Uint32 CStorySceneSection::GetNumberOfElements() const
{
	return m_sceneElements.Size();
}

String CStorySceneSection::GetName() const
{
	return m_sectionName;
}

String CStorySceneSection::GetFriendlyName() const
{
	if( GetScene() == NULL )
	{
		return TBaseClass::GetFriendlyName();
	}

	String name = m_sectionName;
	name += TXT(" in " );
	name += GetScene()->GetFriendlyName();
	return name;
}

void CStorySceneSection::SetName( String name )
{
	m_sectionName = name;
	NotifyAboutNameChanged();
}

void CStorySceneSection::AddSceneElement( CStorySceneElement* element, Uint32 index )
{
	if ( !MarkModified_Pre() )
	{
		return;
	}

	ASSERT( element );
	if( !element )
	{
		return;
	}

	// Generate element ID as section requires all elements to have proper ID set.
	if ( element->GetElementID().Empty() )
	{
		element->GenerateElementID();
		RED_FATAL_ASSERT( !element->GetElementID().Empty(), "CStorySceneSection::AddSceneElement(): couldn't generate ID for scene element." );
	}

	if ( element->IsA< CStorySceneChoice >() == true )
	{
		m_choice = Cast< CStorySceneChoice >( element );

		// Insert choice into all variants.
		for( CStorySceneSectionVariant* sv : m_variants )
		{
			sv->InsertElement( m_choice );
		}

		return;
	}

	if ( index == -1 || index > m_sceneElements.Size() )
	{
		index = m_sceneElements.Size();
	}

	m_sceneElements.Insert( index, element );

	// Insert element into all variants.
	for( CStorySceneSectionVariant* sv : m_variants )
	{
		sv->InsertElement( element, index );
	}

	NotifyAboutElementAdded( element );

	MarkModified_Post();
}

void CStorySceneSection::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT( "sectionName" ) )
	{
		NotifyAboutNameChanged();
	}
	if ( property->GetName() == TXT( "interceptRadius" ) )
	{
		// TODO: Make proper notification
		NotifyAboutSocketsChange();
	}
	if ( property->GetName() == TXT( "numberOfInputPaths" ) )
	{
		if ( m_numberOfInputPaths < 1 )
		{
			m_numberOfInputPaths = 1;
		}

		if ( m_numberOfInputPaths == 1 )
		{
			m_inputPathsElements.Clear();
			NotifyAboutSocketsChange();
		}
		else
		{
			for ( Uint32 i = 0; i < m_numberOfInputPaths; ++i )
			{
				CStorySceneLinkElement* inputPathLinkElement = CreateObject< CStorySceneLinkElement >( this );
				m_inputPathsElements.PushBack( inputPathLinkElement );
			}

			RemoveChoice();
		}
	}
}


void CStorySceneSection::NotifyAboutSocketsChange()
{
	EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( this ) );
}

void CStorySceneSection::NotifyAboutNameChanged()
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneSectionNameChanged ), CreateEventData( this ) );
}

void CStorySceneSection::NotifyAboutElementAdded( CStorySceneElement* element )
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneSectionElementAdded ), CreateEventData( SStorySceneElementInfo( element, this ) ) );

	if ( element->IsA< CStorySceneChoice >() == true )
	{
		NotifyAboutSocketsChange();
	}
}

void CStorySceneSection::NotifyAboutChoiceRemoved()
{
	EDITOR_DISPATCH_EVENT( CNAME( ScenePartBlockSocketsChanged ), CreateEventData( this ) );
}

void CStorySceneSection::GetLines( TDynArray< CAbstractStorySceneLine* >& lines ) const
{
	for ( TDynArray< CStorySceneElement* >::const_iterator i=m_sceneElements.Begin(); i!=m_sceneElements.End(); ++i )
	{
		CAbstractStorySceneLine* line = Cast< CAbstractStorySceneLine >( *i );
		if ( line )
		{
			lines.PushBack( line );
		}
	}
}

void CStorySceneSection::GetVoiceTags( TDynArray< CName >& voiceTags, Bool append ) const
{
	if( !append )
		voiceTags.Clear();
	
	for ( TDynArray< CStorySceneElement* >::const_iterator i=m_sceneElements.Begin(); i!=m_sceneElements.End(); ++i )
	{
		const CStorySceneElement* elem = *i;

		if ( elem && elem->IsA<CAbstractStorySceneLine>() )
		{
			const CAbstractStorySceneLine* line = static_cast< const CAbstractStorySceneLine* >(elem);
			CName voiceTag = line->GetVoiceTag();
			if ( voiceTag != CName::NONE )
			{
				voiceTags.PushBackUnique( voiceTag );
			}
		}
	}
}

void CStorySceneSection::OnConnected( CStorySceneLinkElement* linkedToElement )
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneSectionLinksChanged ), CreateEventData( this ) );
}

void CStorySceneSection::OnDisconnected( CStorySceneLinkElement* linkedToElement )
{
	EDITOR_DISPATCH_EVENT( CNAME( SceneSectionLinksChanged ), CreateEventData( this ) );
}

void CStorySceneSection::GenerateSectionId()
{
	m_sectionId = GetScene()->GenerateUniqueSectionID();
}

void CStorySceneSection::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( m_sectionId == 0 )
	{
		GenerateSectionId();
	}

	if ( m_tags.Empty() == false )
	{
		m_isGameplay = m_tags.HasTag( CNAME( GAMEPLAY ) );
		m_isImportant = m_tags.HasTag( CNAME( IMPORTANT ) );
		m_tags.Clear();
	}

#ifdef NO_EDITOR
	ASSERT( GetParent()->IsA< CStoryScene > () );
#endif //NO_EDITOR

	if ( !GetParent()->IsA< CStoryScene > () )
	{
		SetParent( NULL );
	}

	RemoveEmptyPointers( m_sceneElements );
	RemoveEmptyPointers( m_events );

	// Generate ID for any scene element with empty ID. Section requires all elements to have proper ID. It should no longer happen
	// that a scene element with empty ID is added to section but we may still encounter such elements when loading existing scenes.
	// Note that CStorySceneElement::OnPostLoad() does generate element ID but it's called too late.
	for( CStorySceneElement* scEl : m_sceneElements )
	{
		if( scEl->GetElementID().Empty() )
		{
			scEl->GenerateElementID();
			RED_FATAL_ASSERT( !scEl->GetElementID().Empty(), "CStorySceneSection::OnPostLoad(): couldn't generate ID for scene element." );
		}
	}
	if( m_choice && m_choice->GetElementID().Empty() )
	{
		m_choice->GenerateElementID();
		RED_FATAL_ASSERT( !m_choice->GetElementID().Empty(), "CStorySceneSection::OnPostLoad(): couldn't generate ID for scene element." );
	}

	if( const Bool oldStyleSection = m_variants.Empty() )
	{
		THashMap< Int32, CStorySceneSectionVariantId > contextIdToVariantId;

		// All existing contexts were created for EN so any variant we'll create here will also be in EN.
		Uint32 localeEn = -1;
		SLocalizationManager::GetInstance().FindLocaleId( TXT("EN"), localeEn );

		// Create default variant as a replacement for context m_contexID
		// (do this even if there are no events associated with that context).
		const CStorySceneSectionVariantId defaultVariantId = CreateVariant( localeEn );
		contextIdToVariantId.Insert( m_contexID, defaultVariantId );
		SetDefaultVariant( defaultVariantId );

		// Approve duration of all elements in default variant (use legacy durations stored in scene elements).
		for( const CStorySceneElement* scEl : m_sceneElements )
		{
			ApproveElementDuration( defaultVariantId, scEl->GetElementID(), scEl->GetApprovedDuration() );
		}
		if( m_choice )
		{
			ApproveElementDuration( defaultVariantId, m_choice->GetElementID(), m_choice->GetApprovedDuration() );
		}

		// Add all events to variants. If given variant doesn't exist then create it (we've loaded
		// an old type section. We need to manually create variants from existing events).
		// Also, create event info for each event.
		for( CStorySceneEvent* ev : m_events )
		{
			const Int32 contextId = ev->GetContexID();
			RED_FATAL_ASSERT( contextId >= 0, "CStorySceneSection::OnPostLoad(): encountered context whose id < 0 - we've removed all such contexts, right?" );

			// If there's no variant for this context then create one.
			if( !contextIdToVariantId.KeyExist( contextId ) )
			{
				const CStorySceneSectionVariantId variantId = CreateVariant( localeEn );
				contextIdToVariantId.Insert( contextId, variantId );

				// Approve duration of all elements in variant (use legacy durations stored in scene elements).
				for( const CStorySceneElement* scEl : m_sceneElements )
				{
					ApproveElementDuration( variantId, scEl->GetElementID(), scEl->GetApprovedDuration() );
				}
				if( m_choice )
				{
					ApproveElementDuration( variantId, m_choice->GetElementID(), m_choice->GetApprovedDuration() );
				}
			}

			const CStorySceneSectionVariantId variantId = contextIdToVariantId[ contextId ];
			CStorySceneSectionVariant* sv = m_idToVariant[ variantId ];
			const CGUID evGuid = ev->GetGUID();
			RED_FATAL_ASSERT( !sv->m_events.Exist( evGuid ), "CStorySceneSection::OnPostLoad(): insert event to variant - variant already contains event with this GUID." );
			sv->m_events.PushBack( evGuid );

			for( auto ei : m_eventsInfo )
			{
				RED_FATAL_ASSERT( evGuid != ei->m_eventGuid, "CStorySceneSection::OnPostLoad(): insert event info - such guid already exists in m_eventsInfo." );
			}

			// create event info
			CStorySceneEventInfo* evInfo = new CStorySceneEventInfo( evGuid, ev, variantId );
			m_eventsInfo.PushBack( evInfo );
			m_guidToEventInfo.Insert( evGuid, evInfo );
		}
	}
	else
	{
		// m_localeVariantMappings is a property and is restored but we need to restore m_localeToVariant
		for( CStorySceneLocaleVariantMapping* localeVariantMapping : m_localeVariantMappings )
		{
			m_localeToVariant.Insert( localeVariantMapping->m_localeId, localeVariantMapping );
		}

		// m_variants is a property and is restored but we need to restore m_idTovariant and m_elementIdToApprovedDuration in each variant
		for( auto sv : m_variants )
		{
			m_idToVariant.Insert( sv->m_id, sv );

			for( const CStorySceneSectionVariantElementInfo& elementInfo : sv->m_elementInfo )
			{
				sv->m_elementIdToApprovedDuration.Insert( elementInfo.m_elementId, elementInfo.m_approvedDuration );
			}
		}

		// Store pointer to each event in its event info and fill m_guidToEventInfo hash map.
		for( Uint32 iEvent = 0, numEvents = m_events.Size(); iEvent < numEvents; ++iEvent )
		{
			// Assert that m_events[ iEvent ] and m_eventsInfo[ iEvent ] correspond to each other.
			RED_FATAL_ASSERT( m_events[ iEvent ]->GetGUID() == m_eventsInfo[ iEvent ]->m_eventGuid, "CStorySceneSection::OnPostLoad(): m_events and m_eventGuid are not consistent with each other." );

			CStorySceneEventInfo* ei = m_eventsInfo[ iEvent ];
			ei->m_eventPtr = m_events[ iEvent ];
			m_guidToEventInfo.Insert( ei->m_eventGuid, ei );
		}
	}

	RED_FATAL_ASSERT( m_events.Size() == m_eventsInfo.Size(), "CStorySceneSection::OnPostLoad(): m_events and m_eventsInfo are not consistent with each other." );
	RED_FATAL_ASSERT( m_events.Size() == m_guidToEventInfo.Size(), "CStorySceneSection::OnPostLoad(): m_events and m_guidToEventInfo are not consistent with each other." );

	// remove events that are not associated with any element (this should not happen and we should use assert)
	TDynArray< CGUID > strayEvents;
	for( auto ev : m_events )
	{
		if( !ev->GetSceneElement() )
		{
			strayEvents.PushBack( ev->GetGUID() );
		}
	}
	for( auto evGuid : strayEvents )
	{
		RemoveEvent( evGuid );
	}

	// Call OnPostLoad() for each event.
	for( auto ev : m_events )
	{
		ev->OnPostLoad();
	}

#ifndef NO_EDITOR
	{
		TDynArray< CStorySceneEvent* > toRemove;

		const Uint32 num = m_events.Size();
		for ( Uint32 i=0; i<num; ++i )
		{
			CStorySceneEvent* e = m_events[ i ];

			if ( e->HasLinkParent() )
			{
				CStorySceneEvent* parentLink = GetEvent( e->GetLinkParentGUID() );
				if ( !parentLink )
				{
					e->ResetLinkParent();
					continue;
				}

				Bool breakLink = false;

				CStorySceneEvent* parentBlend = e->HasBlendParent() ? GetEvent( e->GetBlendParentGUID() ) : nullptr;
				if ( parentBlend )
				{
					breakLink = true;
				}

				if ( !breakLink && e->GetGUID() == parentLink->GetBlendParentGUID() )
				{
					breakLink = true;
				}

				if ( !breakLink && e->GetClass()->IsA< CStorySceneEventBlend >() )
				{
					breakLink = true;
				}

				if ( breakLink )
				{
					SCENE_VERIFY( parentLink->RemoveLinkChildGUID( e->GetGUID() ) );
					e->ResetLinkParent();
				}
			}

			if ( e->GetClass()->IsA< CStorySceneEventBlend >() )
			{
				CStorySceneEventBlend* blendEvt = static_cast< CStorySceneEventBlend* >( e );
				if ( blendEvt->GetNumberOfKeys() == 0 )
				{
					toRemove.PushBack( blendEvt );
				}
			}
		}

		for( auto ev : toRemove )
		{
			RemoveEvent( ev->GetGUID() );
		}
	}
#endif
}

void CStorySceneSection::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/
{
	for ( Uint32 i = 0; i < m_sceneElements.Size(); ++i )
	{
		if( m_sceneElements[ i ] )
		{
			m_sceneElements[ i ]->GetLocalizedStrings( localizedStrings );
		}
	}
	if ( m_choice != NULL )
	{
		m_choice->GetLocalizedStrings( localizedStrings );
	}
}

CClass* CStorySceneSection::GetBlockClass() const
{ 
	return ClassID< CStorySceneSectionBlock >();
}

Bool CStorySceneSection::IsValid() const
{
	return GetNumberOfElements() > 0 || GetChoice() != NULL;
}

CStorySceneSection* CStorySceneSection::DrawInterceptSection()
{
	return m_interceptSections.Empty() ? nullptr : m_interceptSections[ GEngine->GetRandomNumberGenerator().Get< Uint32 >() % m_interceptSections.Size() ];
}

CStorySceneLinkElement* CStorySceneSection::GetInputPathLinkElement( Uint32 inputPathIndex )
{
	if ( inputPathIndex >= m_inputPathsElements.Size() )
	{
		return this;
	}

	return m_inputPathsElements[ inputPathIndex ];
}

const CStorySceneLinkElement* CStorySceneSection::GetInputPathLinkElement( Uint32 inputPathIndex ) const
{
	if ( inputPathIndex >= m_inputPathsElements.Size() )
	{
		return this;
	}

	return m_inputPathsElements[ inputPathIndex ];
}

const TDynArray< CStorySceneLinkElement* >& CStorySceneSection::GetInputPathLinks() const
{
	return m_inputPathsElements;
}

Bool CStorySceneSection::CanAddChoice() const
{
	if ( m_choice == NULL && m_numberOfInputPaths == 1 )
	{
		return true;
	}
	return false;
}

void CStorySceneSection::SetScenesElementsAsCopy( Bool isCopy )
{
	for ( TDynArray< CStorySceneElement* >::iterator sceneElement = m_sceneElements.Begin();
		  sceneElement != m_sceneElements.End();
		  ++sceneElement )
	{
		if( (*sceneElement) )
		{
			(*sceneElement)->SetSceneElementCopy( isCopy );
		}
	}

	if ( m_choice )
	{
		m_choice->SetSceneElementCopy( isCopy );
	}
}

Bool CStorySceneSection::CanModifyCamera() const
{
	return false;
}

Bool CStorySceneSection::GetCameraSettings( StorySceneCameraSetting& cameraSettings ) const
{
	return false;
}

void CStorySceneSection::MakeUniqueElementsCopies()
{
	if ( !MarkModified_Pre() )
	{
		// SCENE_TOMSIN_TODO - co to robi i kiedy?
		SCENE_ASSERT( 0 );
	}

	THashMap< String, String > oldIdToNewId;

	for ( auto itEl = m_sceneElements.Begin(), itElEnd = m_sceneElements.End(); itEl != itElEnd; ++itEl )
	{
		CStorySceneElement* el = *itEl;
		RED_FATAL_ASSERT( el, "CStorySceneSection::MakeUniqueElementsCopies(): scene element must not be nullptr." );

		const String oldId = el->GetElementID();
		el->MakeCopyUnique();
		oldIdToNewId.Insert( oldId, el->GetElementID() );
	}

	if ( m_choice )
	{
		const String oldId = m_choice->GetElementID();
		m_choice->MakeCopyUnique();
		oldIdToNewId.Insert( oldId, m_choice->GetElementID() );
	}

	// Update id of all elements in all variants.
	for( CStorySceneSectionVariant* sv : m_variants )
	{
		sv->m_elementIdToApprovedDuration.Clear();

		for( CStorySceneSectionVariantElementInfo& elInfo : sv->m_elementInfo )
		{
			elInfo.m_elementId = oldIdToNewId[ elInfo.m_elementId ];
			sv->m_elementIdToApprovedDuration.Insert( elInfo.m_elementId, elInfo.m_approvedDuration );
		}
	}

	MarkModified_Post();
}

Bool CStorySceneSection::CanAddQuestChoice( Uint32 index, Bool after ) const
{
	return index == 0 && !after && !HasQuestChoiceLine();
}

Bool CStorySceneSection::HasQuestChoiceLine() const
{
	for ( Uint32 i = 0; i < m_sceneElements.Size(); ++i )
	{
		if( m_sceneElements[ i ] == NULL)
		{
			continue;
		}
		else if ( m_sceneElements[ i ]->IsExactlyA< CStorySceneQuestChoiceLine >() == true )
		{
			return true;
		}
		else if ( m_sceneElements[ i ]->IsA< CStorySceneComment >() == false )
		{
			break;
		}
	}
	return false;
}

CStorySceneLinkElement* CStorySceneSection::GetSectionNextElement() const
{
	return GetNextElement();
}

/*
Gets all events associated with specified element in specified section variant.

\param events (out) Container receiving events associated with specified element in specified section variant. Not cleared before use.
\param element Element for which to get events.
\param sectionVariantId Section variant id for which to get events.
*/
void CStorySceneSection::GetEventsForElement( TDynArray< const CStorySceneEvent* >& events, const CStorySceneElement* element, CStorySceneSectionVariantId sectionVariantId ) const
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( sectionVariantId ), "CStorySceneSection::GetEventsForElement(): specified section variant doesn't exist.");

	const CStorySceneSectionVariant& sv = *m_idToVariant[ sectionVariantId ];
	for( auto itEvent = sv.m_events.Begin(), endEvents = sv.m_events.End(); itEvent != endEvents; ++itEvent )
	{
		const CStorySceneEvent* event = GetEvent( *itEvent );
		RED_FATAL_ASSERT( event, "Event exists in section variant but doesn't exist in section itself." );

		if( event->GetSceneElement() == element ) // TODO: we should use element GUID here
		{
			events.PushBack( event );
		}
	}
}

/*
Returns event with specified GUID.

\param guid GUID of event that is to be found.
\return Event with specified GUID or nullptr if no such event was found.
*/
CStorySceneEvent* CStorySceneSection::GetEvent( CGUID guid )
{
	CStorySceneEventInfo* ei = nullptr;
	m_guidToEventInfo.Find( guid, ei );

	return ei? ei->m_eventPtr : nullptr;
}

/*
Returns event with specified GUID.

\param guid GUID of event that is to be found.
\return Event with specified GUID or nullptr if no such event was found.
*/
const CStorySceneEvent* CStorySceneSection::GetEvent( CGUID guid ) const
{
	CStorySceneEventInfo* ei = nullptr;
	m_guidToEventInfo.Find( guid, ei );

	return ei? ei->m_eventPtr : nullptr;
}

/*
Gets normalized start time of specified event.

\param evGuid Event whose normalized start time to get. Must belong to this section.
\return Normalized start time of specified event.

Normalized start time is calculated as follows:
normalizedStartTime = sceneElementIndex + sceneEventRelativeStartPosition

Normalized start time is suitable for determining order of events in section
without knowing their actual absolute start times.
*/
Float CStorySceneSection::GetEventNormalizedStartTime( CGUID evGuid ) const
{
	const CStorySceneEvent* ev = GetEvent( evGuid );
	SCENE_ASSERT( ev ); // assert that specified event belongs to this section

	// get index of element to which specified event belongs
	// (choice element is treated as a last scene element)
	CStorySceneElement* el = ev->GetSceneElement();
	Uint32 elIndex = static_cast< Uint32 >( m_sceneElements.GetIndex( el ) );
	if( elIndex == -1 )
	{
		// event belongs to choice element
		SCENE_ASSERT( el == m_choice );
		elIndex = m_sceneElements.Size();
	}

	// calculate normalized startTime
	const Float evNormalizedStartTime = elIndex + ev->GetStartPosition();

	return evNormalizedStartTime;
}

Bool CStorySceneSection::RemoveEvent( CGUID evGuid )
{
	Bool ret = false;

	if ( MarkModified_Pre() )
	{
		const CStorySceneSectionVariantId sectionVariantId = m_guidToEventInfo[ evGuid ]->m_sectionVariantId;
		CStorySceneSectionVariant* sectionVariant = m_idToVariant[ sectionVariantId ];
		sectionVariant->m_events.Remove( evGuid );

		CStorySceneEventInfo* evInfo = m_guidToEventInfo[ evGuid ];

		ret = m_events.Remove( evInfo->m_eventPtr );
		m_eventsInfo.Remove( evInfo );
		m_guidToEventInfo.Erase( evGuid );

		// TODO: Here we should also release this event but other code is not ready for this.
		// delete evInfo->m_eventPtr;
		delete evInfo;

		MarkModified_Post();
	}

	return ret;
}

/*
Removes all events associated with specified section variant.
*/
void CStorySceneSection::RemoveAllEvents( CStorySceneSectionVariantId sectionVariantId )
{
	if ( MarkModified_Pre() )
	{
		const TDynArray< CGUID > evGuids = GetEvents( sectionVariantId );
		for ( auto evGuid : evGuids )
		{
			RemoveEvent( evGuid );
		}

		MarkModified_Post();
	}
}

/*
Adds event to section.

\param event Event to add. Must have unique GUID. Section acquires ownership of this event.
\param sectionVariantId Id of a section variant to which to add event. Section variant with specified id must already exist.
*/
void CStorySceneSection::AddEvent( CStorySceneEvent* event, CStorySceneSectionVariantId sectionVariantId )
{
	if ( MarkModified_Pre() )
	{
		RED_FATAL_ASSERT( m_idToVariant.KeyExist( sectionVariantId ), "CStorySceneSection::AddEvent(): specified section variant doesn't exist.");
		RED_FATAL_ASSERT( !GetEvent( event->GetGUID() ), "CStorySceneSection::AddEvent(): event with specified GUID already exists." );

		CStorySceneEventInfo* eventInfo = new CStorySceneEventInfo( event->GetGUID(), event, sectionVariantId );
		m_events.PushBack( event );
		m_eventsInfo.PushBack( eventInfo );
		m_guidToEventInfo.Insert( event->GetGUID(), eventInfo );

		// Add event to section variant.
		m_idToVariant[ sectionVariantId ]->m_events.PushBackUnique( event->GetGUID() );

		MarkModified_Post();
	}
}

void CStorySceneSection::OnSerialize( IFile& file )
{
	// Serialize base
	TBaseClass::OnSerialize( file );

	// Write to file
	if( file.IsWriter() )
	{
		// Write number of events
		Uint32 numOfEvents = m_events.Size();
		file << numOfEvents;

		// Serialize events
		for( TDynArray< CStorySceneEvent* >::iterator eventIter = m_events.Begin();
			eventIter != m_events.End(); ++eventIter )
		{
			CStorySceneEvent* event = *eventIter;

			CFileSkipableBlock block( file );

			// Serialize type
			CName type = event->GetClass()->GetName();
			file << type;
			
			// Serialize object
			event->Serialize( file );
		}
	}
	// Read from file
	else if ( file.IsReader() )
	{
		// Read number of events
		Uint32 serializedEvents;
		file << serializedEvents;

		// Read events
		for( Uint32 i = 0; i < serializedEvents; ++i )
		{
			CFileSkipableBlock block( file );

			// Read type
			CName eventType;
			file << eventType;

			// Find type in RTTI
			CClass* theClass = SRTTI::GetInstance().FindClass( eventType );
			if( theClass == NULL )
			{
#ifdef RED_FINAL
				SCENE_ASSERT( theClass );
#endif
				WARN_GAME( TXT( "Loading dialog event '%ls' failed" ), eventType.AsString().AsChar() );
				block.Skip();
				continue;
			}

			SCENE_ASSERT( theClass->IsA< CStorySceneEvent >() );

			// Create object
		    RED_DISABLE_WARNING_MSC( 4344 ) // use of explicit template arguments results in call...
			CStorySceneEvent* event = theClass->CreateObject< CStorySceneEvent >();
			SCENE_ASSERT( event );

			// Serialize object
			event->Serialize( file );

			// Add to the list
			m_events.PushBack( event );
		}
	}
}

void CStorySceneSection::CollectUsedAnimations( CStorySceneAnimationContainer& container ) const
{
	for ( const CStorySceneEvent* e : m_events )
	{
		if ( e /*&& e->GetContexID() == GetContexID()*/ )
		{
			e->CollectUsedAnimations( container );
		}
	}
}

Uint64 CStorySceneSection::GetSectionUniqueId() const
{
	Uint64 uniqueId = 0;
	CStoryScene* parentScene = GetScene();
	SCENE_ASSERT( parentScene );
	if ( parentScene != NULL )
	{
		uniqueId = parentScene->GetSceneIdNumber();
		uniqueId = uniqueId << 32;
	}
	uniqueId |= m_sectionId;
	return uniqueId;
}

void CStorySceneSection::GetRequiredTemplates( TDynArray< TSoftHandle< CResource > >& requiredTemplates ) const
{
	if( CStoryScene* scene = GetScene() ) 
	{
		const TDynArray< CStorySceneProp* >& propsSpawnDef = scene->GetScenePropDefinitions();
		for ( auto iter = propsSpawnDef.Begin(); iter != propsSpawnDef.End(); ++iter )
		{
			if( CStorySceneProp* sceneProp = ( *iter ) )
			{
				const String& path = sceneProp->GetTemplatePath();
				if ( !path.Empty() )
				{
					requiredTemplates.PushBackUnique( TSoftHandle< CResource >( path ) );
				}
			}
		}
	}

	if ( UsesSetting() )
	{
		if ( m_dialogsetChangeTo )
		{
			if ( const CStorySceneDialogsetInstance* dialogset = GetScene()->GetDialogsetByName( m_dialogsetChangeTo ) )
			{
				dialogset->CollectRequiredTemplates( requiredTemplates );
			}
		}
		else
		{
			if( CStoryScene* scene = GetScene() ) 
			{
				const TDynArray< CStorySceneActor* >& actorDefs = scene->GetSceneActorsDefinitions();
				for ( CStorySceneActor* iter : actorDefs )
				{
					if( iter )
					{
						const String& path = iter->m_entityTemplate.GetPath();
						if ( !path.Empty() )
						{
							requiredTemplates.PushBackUnique( TSoftHandle< CResource >( path ) );
						}
					}
				}
			}
		}
	}
}

void CStorySceneSection::GetLocalizedStringIds( TDynArray< Uint32 >& stringIds ) const
{
	for ( TDynArray< CStorySceneElement* >::const_iterator elementIter = m_sceneElements.Begin();
		elementIter != m_sceneElements.End(); ++elementIter )	
	{
		CStorySceneElement* element = *elementIter;
		if( element )
		{
			element->GetLocalizedStringIds( stringIds );
		}		
	}
	if ( m_choice != NULL )
	{
		m_choice->GetLocalizedStringIds( stringIds );
	}
}

#ifndef NO_EDITOR

Bool CStorySceneSection::SupportsInputSelection() const
{
	return m_numberOfInputPaths > 1;
}

void CStorySceneSection::ToggleSelectedInputLinkElement()
{
	++m_selectedLinkedElement;

	if ( m_selectedLinkedElement >= m_inputPathsElements.Size() )
	{
		m_selectedLinkedElement = 0;
	}
}

void CStorySceneSection::SetDialogsetChange( const CName& dialosetChange )
{
	m_dialogsetChangeTo = dialosetChange;
	EDITOR_DISPATCH_EVENT( CNAME( SceneSettingChanged ), CreateEventData( this ) );
}
#endif // NO_EDITOR

void CStorySceneSection::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler, CStorySceneSectionVariantId variantId )
{
	const Uint32 numElems = m_sceneElements.Size();
	for ( Uint32 i=0; i<numElems; ++i )
	{
		CStorySceneElement* elem = m_sceneElements[ i ];
		if ( elem )
		{
			elem->OnBuildDataLayout( compiler );
		}

		SCENE_ASSERT( elem );
	}

	for( CGUID evGuid : GetEvents( variantId ) )
	{
		CStorySceneEvent* ev = GetEvent( evGuid );
		ev->OnBuildDataLayout( compiler );
	}
}

void CStorySceneSection::OnInitInstance( CStorySceneInstanceBuffer& data, CStorySceneSectionVariantId variantId ) const
{
	const Uint32 numElems = m_sceneElements.Size();
	for ( Uint32 i=0; i<numElems; ++i )
	{
		const CStorySceneElement* elem = m_sceneElements[ i ];
		if ( elem )
		{
			elem->OnInitInstance( data );
		}
	}

	for( CGUID evGuid : GetEvents( variantId ) )
	{
		const CStorySceneEvent* ev = GetEvent( evGuid );
		ev->OnInitInstance( data );
	}
}

void CStorySceneSection::OnReleaseInstance( CStorySceneInstanceBuffer& data, CStorySceneSectionVariantId variantId ) const
{
	const Uint32 numElems = m_sceneElements.Size();
	for ( Uint32 i=0; i<numElems; ++i )
	{
		const CStorySceneElement* elem = m_sceneElements[ i ];
		if ( elem )
		{
			elem->OnReleaseInstance( data );
		}
	}

	for( CGUID evGuid : GetEvents( variantId ) )
	{
		const CStorySceneEvent* ev = GetEvent( evGuid );
		ev->OnReleaseInstance( data );
	}
}

#ifndef NO_EDITOR
const CStorySceneInput* CStorySceneSection::GetFirstSceneInput() const
{
	TDynArray< const CStorySceneLinkElement* > stack;
	TDynArray< const CStorySceneLinkElement* > visited;
	stack.PushBack( this );

	const CStorySceneInput* result = nullptr;
	while( !result && !stack.Empty() )
	{
		const CStorySceneLinkElement* link = stack.PopBack();
		result = Cast<const CStorySceneInput>( link );
		if( link && !result )
		{
			visited.PushBack( link );
			TDynArray< CStorySceneLinkElement* > prevElements = link->GetLinkedElements();
			for ( Int32 i = 0; i < prevElements.SizeInt(); ++i )
			{			
				if ( prevElements[i] && !visited.Exist( prevElements[i] ) )
				{
					stack.PushBack( prevElements[i] );
				}				
			}
		}
	}
	return result;
}
#endif

#ifndef NO_EDITOR

/*
Removes invalid blend events.

\param outNumFixedBlendEvents (out) Number of invalid blend events that were fixed.
\param outNumRemovedBlendEvents (out) Number of invalid blend events that couldn't be fixed and had to be removed.

Invalid blend events are those that reference nonexistent keys. Such blend events can be encountered
due to bugs in some parts of Scene Editor (e.g. blend event may become broken after user deletes/cuts
dialog line that has blend event key associated with it).
*/
void CStorySceneSection::FixInvalidBlendEvents( Uint32& outNumFixedBlendEvents, Uint32& outNumRemovedBlendEvents )
{
	outNumFixedBlendEvents = 0;
	outNumRemovedBlendEvents = 0;

	TDynArray< CStorySceneEventBlend* > invalidBlendEvents;

	for( CStorySceneEvent* event : m_events )
	{
		CStorySceneEventBlend* blendEvent = Cast< CStorySceneEventBlend >( event );
		if( blendEvent )
		{
			// Get list of keys that are referenced but doesn't exist.
			TDynArray< CGUID > nonexistentKeys;
			for( Uint32 iKey = 0, numKeys = blendEvent->GetNumberOfKeys(); iKey < numKeys; ++iKey )
			{
				CGUID keyGuid = blendEvent->GetKey( iKey );
				const CStorySceneEvent* keyEvent = GetEvent( keyGuid );
				if( !keyEvent )
				{
					nonexistentKeys.PushBack( keyGuid );
				}
			}

			if( nonexistentKeys.Size() > 0 )
			{
				const Uint32 numValidKeys = blendEvent->GetNumberOfKeys() - nonexistentKeys.Size();
				if( numValidKeys >= 2 )
				{
					// Fix blend event by removing references to nonexistent keys.
					for( CGUID key : nonexistentKeys )
					{
						blendEvent->RemoveKey( key );
					}
					++outNumFixedBlendEvents;
				}
				else
				{
					// Mark blend event for deletion as it can't be fixed (blend event has to have at least two valid keys).
					invalidBlendEvents.PushBack( blendEvent );
				}
			}
		}
	}

	// Remove invalid blend events.
	for( CStorySceneEventBlend* invalidBlendEvent : invalidBlendEvents )
	{
		// Detach all valid keys from blend event.
		for( Uint32 iKey = 0, numKeys = invalidBlendEvent->GetNumberOfKeys(); iKey < numKeys; ++iKey )
		{
			CGUID keyGuid = invalidBlendEvent->GetKey( iKey );
			CStorySceneEvent* keyEvent = GetEvent( keyGuid );
			if( keyEvent )
			{
				keyEvent->SetBlendParentGUID( CGUID::ZERO );
			}
		}

		if( invalidBlendEvent->HasLinkChildren() )
		{
			// Remove all link children.
			const TDynArray< CGUID >& children = invalidBlendEvent->GetLinkChildrenGUID();
			for( CGUID guid : children )
			{
				CStorySceneEvent* child = GetEvent( guid );
				child->ResetLinkParent();
			}
			invalidBlendEvent->RemoveAllLinkChildren();
		}

		RemoveEvent( invalidBlendEvent->GetGUID() );
		++outNumRemovedBlendEvents;
	}
}

/*
Fixes stray interpolation keys by making them non-key events.

\return Number of stray interpolation keys found and fixed.

Stray interpolation keys are those that reference nonexistent interpolation events.
*/
Uint32 CStorySceneSection::FixStrayInterpolationKeys()
{
	Uint32 numStrayKeys = 0;

	for( CStorySceneEvent* event : m_events )
	{
		if( event->IsInterpolationEventKey() )
		{
			const CStorySceneEvent* interpolationEvent = GetEvent( event->GetInterpolationEventGUID() );

			// if key event references nonexistent interpolation event then fix it by making it non-key event
			if( !interpolationEvent )
			{
				event->SetInterpolationEventGUID( CGUID::ZERO );
				++numStrayKeys;
			}
		}
	}

	return numStrayKeys;
}

/*
Fixes stray blend keys by making them non-key events.

\return Number of stray blend keys found and fixed.

Stray blend keys are those that reference nonexistent blend events.
*/
Uint32 CStorySceneSection::FixStrayBlendKeys()
{
	Uint32 numStrayKeys = 0;

	for( CStorySceneEvent* event : m_events )
	{
		if( event->HasBlendParent() )
		{
			const CStorySceneEvent* blendEvent = GetEvent( event->GetBlendParentGUID() );

			// if key event references nonexistent blend event then fix it by making it non-key event
			if( !blendEvent )
			{
				event->SetBlendParentGUID( CGUID::ZERO );
				++numStrayKeys;
			}
		}
	}

	return numStrayKeys;
}

/*
Removes bad links between events.

\return Number of bad links removed.

Bad links are those that reference nonexistent events.
*/
Uint32 CStorySceneSection::RemoveBadLinksBetweenEvents()
{
	Uint32 numBadLinks = 0;

	for( CStorySceneEvent* event : m_events )
	{
		if( event->HasLinkChildren() )
		{
			// remove links to nonexistent child events
			const TDynArray< CGUID >& linkChildren = event->GetLinkChildrenGUID();
			for( Uint32 iChild = 0, numChildren = linkChildren.Size(); iChild < numChildren; ++iChild )
			{
				const Uint32 iChildBack = numChildren - iChild - 1;
				if( !GetEvent( linkChildren[ iChildBack ] ) )
				{
					event->RemoveLinkChildGUID( linkChildren [ iChildBack ] );
					++numBadLinks;
				}
			}
		}

		if( event->HasLinkParent() )
		{
			// remove link to nonexistent parent event
			if( !GetEvent( event->GetLinkParentGUID() ) )
			{
				event->ResetLinkParent();
				++numBadLinks;
			}
		}
	}

	return numBadLinks;
}

#endif // !NO_EDITOR

/*
Sets locale to variant mapping.

\param localeId Locale part of mapping.
\param variantId Variant part of mapping. Pass -1 to specify default variant.
*/
void CStorySceneSection::SetLocaleVariantMapping( Uint32 localeId, CStorySceneSectionVariantId variantId )
{
	if( variantId != -1 )
	{
		CStorySceneLocaleVariantMapping* localeVariantMapping = nullptr;
		if( m_localeToVariant.Find( localeId, localeVariantMapping ) )
		{
			// mapping for this locale already exists - update it
			localeVariantMapping->m_variantId = variantId;
		}
		else
		{
			CStorySceneLocaleVariantMapping* newLocaleVariantMapping = new CStorySceneLocaleVariantMapping;
			newLocaleVariantMapping->m_localeId = localeId;
			newLocaleVariantMapping->m_variantId = variantId;

			m_localeVariantMappings.PushBack( newLocaleVariantMapping );
			m_localeToVariant[ localeId ] = newLocaleVariantMapping;
		}
	}
	else
	{
		// remove language -> variant mapping if it exists
		CStorySceneLocaleVariantMapping* localeVariantMapping = nullptr;
		if( m_localeToVariant.Find( localeId, localeVariantMapping ) )
		{
			m_localeToVariant.Erase( localeId );
			m_localeVariantMappings.Remove( localeVariantMapping );
			delete localeVariantMapping;
		}
	}
}

/*
Gets locale to variant mapping.

\param localeId Locale part of mapping.
\return Variant part of mapping. -1 means default variant.
*/
CStorySceneSectionVariantId CStorySceneSection::GetLocaleVariantMapping( Uint32 localeId ) const
{
	CStorySceneSectionVariantId sectionVariantId = -1;

	CStorySceneLocaleVariantMapping* localeVariantMapping = nullptr;
	if( m_localeToVariant.Find( localeId, localeVariantMapping ) )
	{
		sectionVariantId = localeVariantMapping->m_variantId;
	}

	return sectionVariantId;
}

/*
Returns variant that is used by specified locale.

\param localeId Locale in question.
\return Id of a variant used by specified locale. If locale is using default variant
then id of that variant will be returned (this function never returns -1).
*/
CStorySceneSectionVariantId CStorySceneSection::GetVariantUsedByLocale( Uint32 localeId ) const
{
	#ifndef NO_EDITOR

		if( m_variantIdChosenInEditor != -1 )
		{
			return m_variantIdChosenInEditor;
		}
		else if( m_variantIdForcedInEditor != -1 )
		{
			return m_variantIdForcedInEditor;
		}

	#endif // !NO_EDITOR

	CStorySceneSectionVariantId variantId = GetLocaleVariantMapping( localeId );
	if( variantId == -1 )
	{
		variantId = m_defaultVariantId;
	}

	return variantId;
}

Uint32 CStorySceneSection::GetVariantBaseLocale( CStorySceneSectionVariantId variantId ) const
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( variantId ), "CStorySceneSection::GetVariantBaseLocale(): section variant doesn't exist." );
	CStorySceneSectionVariant& sv = *m_idToVariant[ variantId ];
	return sv.m_localeId;
}

/*
Sets variant locale.

Changing variant locale doesn't change duration of elements stored in variant.
Callers needs to do this manually if this is desired.
*/
void CStorySceneSection::SetVariantBaseLocale( CStorySceneSectionVariantId variantId, Uint32 baseLocaleId )
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( variantId ), "CStorySceneSection::GetVariantBaseLocale(): section variant doesn't exist." );
	CStorySceneSectionVariant& sv = *m_idToVariant[ variantId ];
	sv.m_localeId = baseLocaleId;
}

/*
Gets variant containing specified event.

\param event Event in question. Must belong to this section.
\return Variant containing specified event.
*/
CStorySceneSectionVariantId CStorySceneSection::GetEventVariant( CGUID evGuid ) const
{
	RED_FATAL_ASSERT( m_guidToEventInfo.KeyExist( evGuid ), "CStorySceneSection::GetVariantId(): event doesn't belong to this section." );
	return m_guidToEventInfo[ evGuid ]->m_sectionVariantId;
}

/*
Enumerates section variants.

\param outVariants (out) Container receiving enumerated variants. It's not cleared before use.
*/
void CStorySceneSection::EnumerateVariants( TDynArray< CStorySceneSectionVariantId >& outVariants ) const
{
	outVariants.Reserve( outVariants.Size() + m_variants.Size() );
	for( const CStorySceneSectionVariant* sv : m_variants )
	{
		outVariants.PushBack( sv->m_id );
	}
}

/*
Creates new (and empty) section variant.

\param localeId Locale id with for which this variant is created.
\return Id of created section variant.

No scene element will have its duration approved in newly created variant.
Caller should approve element durations by himself (see ApproveElementDuration()).

NO_EDITOR note
It's safe to call this function for any locale even if NO_EDITOR is defined.
It's because this function doesn't calculate duration of any scene element.
*/
CStorySceneSectionVariantId CStorySceneSection::CreateVariant( Uint32 localeId )
{
	CStorySceneSectionVariant* sv = new CStorySceneSectionVariant;
	sv->m_id = m_nextVariantId++;
	sv->m_localeId = localeId;

	for( const CStorySceneElement* el : m_sceneElements )
	{
		sv->InsertElement( el );
	}
	if( m_choice )
	{
		sv->InsertElement( m_choice );
	}

	m_variants.PushBack( sv );
	m_idToVariant.Insert( sv->m_id, sv );

	return sv->m_id;
}

/*
Clones specified variant.

\param orgVariantId Id of a variant that is to be cloned.
\return Id of a new variant that was created as a clone of specified variant.
*/
CStorySceneSectionVariantId CStorySceneSection::CloneVariant( CStorySceneSectionVariantId orgVariantId )
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( orgVariantId ), "CStorySceneSection::CloneVariant(): original variant doesn't exist." );

	class Clone
	{
	public:
		Clone( CStorySceneEvent* clone, CGUID orgGuid ) : m_ev( clone ), m_orgGuid( orgGuid ) {}

		CStorySceneEvent* m_ev;
		CGUID m_orgGuid;
	};

	const TDynArray< CGUID >& orgGuids = GetEvents( orgVariantId );
	THashMap< CGUID, CGUID > orgGuidToCloneGuid( orgGuids.Size() );
	TDynArray< Clone > clones;
	clones.Reserve( orgGuids.Size() );

	// Clone all events from original variant.
	for( CGUID orgEventGuid : orgGuids )
	{
		const CStorySceneEvent* orgEvent = GetEvent( orgEventGuid );
		CStorySceneEvent* cloneEvent = orgEvent->Clone();

		clones.PushBack( Clone( cloneEvent, orgEventGuid ) );
		orgGuidToCloneGuid.Insert( orgEventGuid, cloneEvent->GetGUID() );
	}

	// Reestablish all relationships between cloned events.
	for( Clone clone : clones )
	{
		CStorySceneEvent* orgEvent = GetEvent( clone.m_orgGuid );

		// Clone of a blend event key is not a blend event key. Make it a key of blend event clone.
		if( orgEvent->HasBlendParent() )
		{
			clone.m_ev->SetBlendParentGUID( orgGuidToCloneGuid[ orgEvent->GetBlendParentGUID() ] );
		}
		// Clone of an interpolation event key is not an interpolation event key. Make it a key of interpolation event clone.
		else if( orgEvent->IsInterpolationEventKey() )
		{
			clone.m_ev->SetInterpolationEventGUID( orgGuidToCloneGuid[ orgEvent->GetInterpolationEventGUID() ] );
		}
		// Clone of a blend event uses original events as keys. Make it use clones of original key events.
		else if( IsType< CStorySceneEventBlend >( orgEvent ) )
		{
			CStorySceneEventBlend* blendEventClone = static_cast< CStorySceneEventBlend* >( clone.m_ev );
			for( Uint32 iKey = 0, numKeys = blendEventClone->GetNumberOfKeys(); iKey < numKeys; ++iKey )
			{
				CGUID oldGuid = blendEventClone->GetKey( iKey );
				blendEventClone->OnGuidChanged( oldGuid, orgGuidToCloneGuid[ oldGuid ] );
			}
		}
		// Clone of an interpolation event uses original events as keys. Make it use clones of original key events.
		else if( IsType< CStorySceneEventInterpolation >( orgEvent ) )
		{
			CStorySceneEventInterpolation* interpolationEventClone = static_cast< CStorySceneEventInterpolation* >( clone.m_ev );
			for( Uint32 iKey = 0, numKeys = interpolationEventClone->GetNumKeys(); iKey < numKeys; ++iKey )
			{
				CGUID oldGuid = interpolationEventClone->GetKeyGuid( iKey );
				interpolationEventClone->OnGuidChanged( oldGuid, orgGuidToCloneGuid[ oldGuid ] );
			}
		}

		// Clone is not linked to any event. Link it to clone of original link parent.
		if( orgEvent->HasLinkParent() )
		{
			clone.m_ev->SetLinkParent( orgGuidToCloneGuid[ orgEvent->GetLinkParentGUID() ], orgEvent->GetLinkParentTimeOffset() );
		}

		#ifndef  NO_EDITOR
			// Clone has no events linked to it. Link with it clones of original link children.
			for( CGUID linkChildGuid : orgEvent->GetLinkChildrenGUID() )
			{
				clone.m_ev->AddLinkChildGUID( orgGuidToCloneGuid[ linkChildGuid ] );
			}
		#endif // !NO_EDITOR
	}
	
	const CStorySceneSectionVariant* orgSv = m_idToVariant[ orgVariantId ];

	// Create new variant as a clone of original variant, except for events.
	CStorySceneSectionVariant* newSv = new CStorySceneSectionVariant;
	newSv->m_id = m_nextVariantId++;
	newSv->m_localeId = orgSv->m_localeId;
	newSv->m_elementInfo = orgSv->m_elementInfo;
	newSv->m_elementIdToApprovedDuration = orgSv->m_elementIdToApprovedDuration;

	// Add new variant to section.
	m_variants.PushBack( newSv );
	m_idToVariant.Insert( newSv->m_id, newSv );

	// Add all clones to new variant.
	for( Clone clone : clones )
	{
		AddEvent( clone.m_ev, newSv->m_id );
	}

	return newSv->m_id;
}

/*
Destroys empty section variant.

\param sectionVariantId Id of an empty section variant to destroy. Section variant has to exist, has to be empty,
must not be selected as default section variant, must not be used by any locale.
*/
void CStorySceneSection::DestroyVariant( CStorySceneSectionVariantId sectionVariantId )
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( sectionVariantId ), "CStorySceneSection::DestroySectionVariant(): section variant doesn't exist." );
	RED_FATAL_ASSERT( m_idToVariant[ sectionVariantId ]->m_events.Empty(), "CStorySceneSection::DestroySectionVariant(): section variant is not empty." );
	RED_FATAL_ASSERT( sectionVariantId != m_defaultVariantId, "CStorySceneSection::DestroySectionVariant(): section variant is selected as default section variant." );

	CStorySceneSectionVariant* sv = m_idToVariant[ sectionVariantId ];
	m_idToVariant.Erase( sectionVariantId );
	m_variants.Remove( sv );
	delete sv;

	#ifndef NO_EDITOR

		if( sectionVariantId == m_variantIdChosenInEditor )
		{
			m_variantIdChosenInEditor = -1;
		}

		if( sectionVariantId == m_variantIdForcedInEditor )
		{
			m_variantIdForcedInEditor = -1;
		}

	#endif // !NO_EDITOR
}

void CStorySceneSection::SetDefaultVariant( CStorySceneSectionVariantId variantId )
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( variantId ), "CStorySceneSection::SetDefaultVariant(): variant with specified id doesn't exist" );
	m_defaultVariantId = variantId;
}

CStorySceneSectionVariantId CStorySceneSection::GetDefaultVariant() const
{
	return m_defaultVariantId;
}

#ifndef NO_EDITOR

void CStorySceneSection::SetVariantChosenInEditor( CStorySceneSectionVariantId variantId )
{
	m_variantIdChosenInEditor = variantId;
}

CStorySceneSectionVariantId CStorySceneSection::GetVariantChosenInEditor() const
{
	return m_variantIdChosenInEditor;
}

void CStorySceneSection::SetVariantForcedInEditor( CStorySceneSectionVariantId variantId )
{
	m_variantIdForcedInEditor = variantId;
}

CStorySceneSectionVariantId CStorySceneSection::GetVariantForcedInEditor() const
{
	return m_variantIdForcedInEditor;
}

#endif // !NO_EDITOR

/*

This function also accepts choice element as choice is a scene element too
(although it's kept separately from the rest of scene elements).
*/
void CStorySceneSection::ApproveElementDuration( CStorySceneSectionVariantId variantId, const String& elementId, Float duration )
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( variantId ), "CStorySceneSection::ApproveElementDuration(): variant with specified id doesn't exist." );
	RED_FATAL_ASSERT( m_idToVariant[ variantId ]->m_elementIdToApprovedDuration.KeyExist( elementId ), "CStorySceneSection::ApproveElementDuration(): element with specified id doesn't exist in this variant." );

	CStorySceneSectionVariant* sv = m_idToVariant[ variantId ];

	sv->m_elementIdToApprovedDuration[ elementId ] = duration;
	for( CStorySceneSectionVariantElementInfo& info : sv->m_elementInfo )
	{
		if( info.m_elementId == elementId )
		{
			info.m_approvedDuration = duration;
			break;
		}
	}
}

Float CStorySceneSection::GetElementApprovedDuration( CStorySceneSectionVariantId variantId, const String& elementId ) const
{
	RED_FATAL_ASSERT( m_idToVariant.KeyExist( variantId ), "CStorySceneSection::GetElementApprovedDuration(): variant with specified id doesn't exist" );

	CStorySceneSectionVariant* sv = m_idToVariant[ variantId ];

	RED_FATAL_ASSERT( sv->m_elementIdToApprovedDuration.KeyExist( elementId ), "CStorySceneSection::GetElementApprovedDuration(): variant doesn't have duration of this element (should have them all)" );

	Float duration = -1.0f;
	sv->m_elementIdToApprovedDuration.Find( elementId, duration );
	return duration;
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
