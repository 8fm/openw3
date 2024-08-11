/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneSectionPreload.h"

#include "../engine/mesh.h"
#include "../engine/localizationManager.h"
#include "../engine/bitmapTexture.h"
#include "../engine/soundSystem.h"

#include "storySceneSection.h"
#include "storySceneSectionPlayingPlan.h"
#include "storyScenePlayer.h"
#include "storySceneSystem.h"
#include "storySceneChoice.h"
#include "storySceneChoiceLine.h"
#include "storySceneDebugger.h"
#include "storySceneInput.h"

// For texture pre-streaming
#include "../engine/renderCommands.h"
#include "../engine/renderTextureStreamRequest.h"
#include "storySceneCutsceneSection.h"
#include "extAnimItemEvents.h"
#include "storySceneEventEquipItem.h"
#include "storySceneEventDespawn.h"
#include "../engine/renderFramePrefetch.h"
#include "../engine/extAnimCutsceneBodyPartEvent.h"
#include "itemIterator.h"
#include "../engine/meshTypeComponent.h"
#include "gameTypeRegistry.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif


void CollectAppearanceTemplate( const CEntity* ent, const CEntityTemplate* entTempl, CName appearanceName, TDynArray< TSoftHandle< CResource > >& templates )
{
	const CEntityAppearance* appearance = entTempl->GetAppearance( appearanceName, true );
	for ( const auto appearanceTemplate : appearance->GetIncludedTemplates() )
	{
		if ( appearanceTemplate.IsValid() )
		{
			templates.PushBack( appearanceTemplate.Get() );
		}
	}
	//TDynArray<String> res;
	//ent->CollectResourcesInStreaming( res, appearanceName );
	//for ( String& path : res )
	//{
	//	templates.PushBackUnique( path );
	//}
}

// Add an item's entity template to a list of templates.
void CStorySceneSectionLoader::CollectItemTemplate( const CEntity* entity, CName itemName, TDynArray< TSoftHandle< CResource > >& templates )
{
	if ( !entity )
	{
		return;
	}

	if( const CGameplayEntity* gpent = Cast< const CGameplayEntity>(entity) )
	{
		if( const CInventoryComponent* inv = gpent->GetInventoryComponent() )
		{
			const String& templateName = inv->GetTemplate( itemName );
			const String& templatePath = GCommonGame->GetDefinitionsManager()->TranslateTemplateName( templateName );
			if ( !templatePath.Empty() && !SItemEntityManager::GetInstance().GetPreloadedEntityTemplate( itemName, templatePath ) )
			{
				templates.PushBack( TSoftHandle< CResource >( templatePath ) );
			}
		}
	}
}

const Float CStorySceneSectionLoader::WAIT_FOR_PROC_TIMEOUT = 10.f;

CStorySceneSectionLoader::CStorySceneSectionLoader()
	: m_currentLoadedSection( nullptr )
	, m_currentLoadedSectionDialogset( nullptr )
	, m_scenePlayer( nullptr )
	, m_preloadState( SSPS_None )
	, m_createdPlayingPlan( -1 )
	, m_asyncLoading( true )
	, m_procesEntitiesTimer( 0.f )
	, m_animLoadingTime( 0.f )
{}

CStorySceneSectionLoader::~CStorySceneSectionLoader()
{
	ClearPreloads();
}

/*
Request playing plan load or boosts priority of existing request.

\param highPriority True - request (either new or existing) will be placed at the beginning of a request queue.
False - if request is new then it will be placed at the end of a request queue, if request already exists then its priority is not changed.
*/
void CStorySceneSectionLoader::RequestPlayingPlan( const CStorySceneSection* section, CStoryScenePlayer* scenePlayer, Bool highPriority, const CStorySceneDialogsetInstance* currDialogset )
{
	// Plan is loading
	if ( m_currentLoadedSection != nullptr && m_currentLoadedSection->GetSectionUniqueId() == section->GetSectionUniqueId() )
	{
		return;
	}

	// Ignore if plan is already loaded
	if ( m_preloadedSectionPlans.KeyExist( section->GetSectionUniqueId() ) == true )
	{
		return;
	}

	// Check if such request already exists. If it does then boost its priority if requested and return as there's nothing more to do.
	for ( auto reqIt = m_sectionRequestQueue.Begin(), end = m_sectionRequestQueue.End(); reqIt != end; ++reqIt )
	{
		if ( ( *reqIt ).m_section->GetSectionUniqueId() == section->GetSectionUniqueId() )
		{
			if ( highPriority )
			{
				// Boost priority of existing request.
				SStorySceneSectionRequest req = *reqIt;
				m_sectionRequestQueue.Erase( reqIt );
				m_sectionRequestQueue.PushFront( req );
			}

			// Nothing more to do.
			return;
		}
	}

	// Create new request and put it at the beginning or at the end of request queue depending on its priority.
	SStorySceneSectionRequest newReq( section, currDialogset, scenePlayer );
	if ( highPriority )
	{
		m_sectionRequestQueue.PushFront( newReq );
	}
	else
	{
		m_sectionRequestQueue.PushBack( newReq );
	}
}

Bool CStorySceneSectionLoader::MakeRequestASAP( const CStorySceneSection* section )
{
	// Plan is loading
	if ( m_currentLoadedSection != nullptr && m_currentLoadedSection->GetSectionUniqueId() == section->GetSectionUniqueId() )
	{
		return true;
	}

	// Ignore if plan is already loaded
	if ( m_preloadedSectionPlans.KeyExist( section->GetSectionUniqueId() ) == true )
	{
		return true;
	}

	// Check if such request already exists. If it does then boost its priority if requested and return as there's nothing more to do.
	for ( auto reqIt = m_sectionRequestQueue.Begin(), end = m_sectionRequestQueue.End(); reqIt != end; ++reqIt )
	{
		if ( ( *reqIt ).m_section->GetSectionUniqueId() == section->GetSectionUniqueId() )
		{
			// Boost priority of existing request.
			SStorySceneSectionRequest req = *reqIt;
			m_sectionRequestQueue.Erase( reqIt );
			m_sectionRequestQueue.PushFront( req );

			// Nothing more to do.
			return true;
		}
	}

	return false;
}

Bool CStorySceneSectionLoader::HasPlanRequested( const CStorySceneSection* section ) const
{
	// Plan is loading
	Int32 ret( 0 );

	if ( m_currentLoadedSection != nullptr && m_currentLoadedSection->GetSectionUniqueId() == section->GetSectionUniqueId() )
	{
		ret++;
	}

	// Ignore if plan is already loaded
	if ( m_preloadedSectionPlans.KeyExist( section->GetSectionUniqueId() ) == true )
	{
		ret++;
	}

	// Check if such request already exists. If it does then boost its priority if requested and return as there's nothing more to do.
	for ( auto reqIt = m_sectionRequestQueue.Begin(), end = m_sectionRequestQueue.End(); reqIt != end; ++reqIt )
	{
		if ( ( *reqIt ).m_section->GetSectionUniqueId() == section->GetSectionUniqueId() )
		{
			ret++;
		}
	}

	SCENE_ASSERT( ret <= 1 );

	return ret > 0;
}

Float CStorySceneSectionLoader::GetLoadingTime( const CStorySceneSection* section )
{
	if ( m_currentLoadedSection == section )
	{
		return (Float) ( Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_currentLoadedSectionStartTime );
	}

	for ( auto it = m_sectionRequestQueue.Begin(); it != m_sectionRequestQueue.End(); ++it )
	{
		if ( it->m_section->GetSectionUniqueId() == section->GetSectionUniqueId() )
		{
			return it->GetLoadingTime();
		}
	}
	return 0.0f;
}

Bool CStorySceneSectionLoader::HasPlanById( Int32 id ) const
{
	SCENE_ASSERT( id != -1 );

	const Uint32 size = m_plans.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_plans[ i ]->m_id == id )
		{
			return true;
		}
	}

	return false;
}

/*
Returns playing plan with specified id.

\param id Id of a playing plan to return.
\return Playing plan with specified id.
*/
CStorySceneSectionPlayingPlan* CStorySceneSectionLoader::FindPlanById( Int32 id )
{
	SCENE_ASSERT__FIXME_LATER( id != -1 );

	const Uint32 size = m_plans.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_plans[ i ]->m_id == id )
		{
			return m_plans[ i ];
		}
	}

	SCENE_ASSERT__FIXME_LATER( 0 );

	return nullptr;
}

/*
Returns playing plan with specified id.

\param id Id of a playing plan to return.
\return Playing plan with specified id.
*/
const CStorySceneSectionPlayingPlan* CStorySceneSectionLoader::FindPlanById( Int32 id ) const
{
	SCENE_ASSERT__FIXME_LATER( id != -1 );

	const Uint32 size = m_plans.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_plans[ i ]->m_id == id )
		{
			return m_plans[ i ];
		}
	}

	SCENE_ASSERT__FIXME_LATER( 0 );

	return nullptr;
}

/*
Deletes playing plan with specified id.

\param id Id of a playing plan to delete.
\return True - plan deleted. False - no such plan.
*/
Bool CStorySceneSectionLoader::DeletePlanById( Int32 id )
{
	SCENE_ASSERT( id != -1 );

	const Uint32 size = m_plans.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_plans[ i ]->m_id == id )
		{
			m_plans[ i ]->Cleanup();
			delete m_plans[ i ];
			m_plans.RemoveAt( i );
			return true;
		}
	}
	return false;
}

/*
Returns id of a playing plan associated with section.

\param section Section for which to get associated playing plan id.
\return Id of a playing plan associated with section.
*/
Int32 CStorySceneSectionLoader::GetPlayingPlanId( const CStorySceneSection* section )
{
	Int32 id = -1;
	m_preloadedSectionPlans.Find( section->GetSectionUniqueId(), id );

	SCENE_ASSERT( id != -1 );

	return id;
}

/*
Returns playing plan associated with section.

\param section Section for which to get associated playing plan.
\return Playing plan associated with section.
*/
CStorySceneSectionPlayingPlan* CStorySceneSectionLoader::GetPlayingPlan( const CStorySceneSection* section )
{
	Int32 id = -1;

	if ( m_preloadedSectionPlans.Find( section->GetSectionUniqueId(), id ) )
	{
		SCENE_ASSERT( id >= 0 );
		return FindPlanById( id );
	}

	return nullptr;
}

const CStorySceneSectionPlayingPlan* CStorySceneSectionLoader::GetPlayingPlan( const CStorySceneSection* section ) const
{
	Int32 id = -1;

	if ( m_preloadedSectionPlans.Find( section->GetSectionUniqueId(), id ) )
	{
		SCENE_ASSERT( id >= 0 );
		return FindPlanById( id );
	}

	return nullptr;
}

Bool CStorySceneSectionLoader::ForgetPlayingPlan( Uint64 sectionId )
{
	Int32 id = -1;

	if ( m_preloadedSectionPlans.Find( sectionId, id ) )
	{
		SCENE_ASSERT( id >= 0 );
		m_preloadedSectionPlans.Erase( sectionId );

		SCENE_ASSERT( !HasPlanInMapById( id ) );

		return DeletePlanById( id );
	}

	SCENE_ASSERT( 0 );

	return false;
}

void CStorySceneSectionLoader::ClearPreloads()
{
	m_sectionRequestQueue.Clear();

	// Cleanup preloaded sections
	ForgetPreloadedPlans();
}

void CStorySceneSectionLoader::GetTemplates( SSectionLoaderContext& context )
{
	//SSPS_GetTemplates;

	// Gather all required templates
	m_templatesRequiredBySection.Clear();
	m_currentLoadedSection->GetRequiredTemplates( m_templatesRequiredBySection );	
}

void CStorySceneSectionLoader::PreloadStrings( SSectionLoaderContext& context )
{
	//SSPS_PreloadStrings;

	// Get strings required to preload for section
	m_stringsRequiredBySection.Clear();
	m_currentLoadedSection->GetLocalizedStringIds( m_stringsRequiredBySection );

	if ( m_asyncLoading )
	{
		SLocalizationManager::GetInstance().PreloadLanguagePacksAsync( m_currentLoadedSection->GetSectionUniqueId(), m_stringsRequiredBySection );
	}
}

Bool CStorySceneSectionLoader::WaitForTemplates( SSectionLoaderContext& context )
{
	//SSPS_WaitForTemplates;

	// Wait for all templates to load
	Bool allTemplatesLoaded = true;

	for ( TSoftHandle< CResource >& templateHandle : m_templatesRequiredBySection )
	{
		if ( m_asyncLoading )
		{
			// Get async can also fail - eg. wrong resource
			if( templateHandle.GetAsync() == BaseSoftHandle::ALR_InProgress )
			{
				allTemplatesLoaded = false;
			}
		}
		else
		{
			templateHandle.Load();
		}
	}

	if ( !allTemplatesLoaded )
	{
		SCENE_ASSERT( m_asyncLoading );
	}

	return allTemplatesLoaded;
}

Bool CStorySceneSectionLoader::WaitForStrings( SSectionLoaderContext& context )
{
	//SSPS_WaitForStrings;

	// Wait for all strings to load
	Bool stringsLoaded = true;

	if ( m_asyncLoading )
	{
		stringsLoaded = SLocalizationManager::GetInstance().PreloadLanguagePacksAsync( m_currentLoadedSection->GetSectionUniqueId(), m_stringsRequiredBySection );
	}
	else
	{
		stringsLoaded = SLocalizationManager::GetInstance().PreloadLanguagePacksSync( m_currentLoadedSection->GetSectionUniqueId(), m_stringsRequiredBySection );
		SCENE_ASSERT( stringsLoaded );
	}
	if ( stringsLoaded == true )
	{		
		
	}
	else
	{
		SCENE_ASSERT( m_asyncLoading );
		return false;
	}
	return true;
}

void CStorySceneSectionLoader::EnsureActors( SSectionLoaderContext& context )
{
	//SSPS_EnsureActors;

	// Ensure existence of actors
	if ( m_currentLoadedSection->GetDialogsetChange() )
	{
		const CStorySceneDialogsetInstance* sectionSetting = m_currentLoadedSection->GetScene()->GetDialogsetByName( m_currentLoadedSection->GetDialogsetChange() );
		SCENE_ASSERT( sectionSetting );
		SCENE_ASSERT( sectionSetting == m_currentLoadedSectionDialogset );
	}

	if ( !m_currentLoadedSection->IsGameplay() )
	{
		SCENE_ASSERT( m_currentLoadedSectionDialogset );
	}

	if ( m_currentLoadedSectionDialogset != nullptr && m_currentLoadedSection->UsesSetting() == true )
	{
		// Do we need this?
		m_scenePlayer->GetSceneDirector()->EnsureExistenceOfSettingActors( m_currentLoadedSectionDialogset, false );
	}
	m_scenePlayer->GetSceneDirector()->EnsureExistenceOfProps();
}

void CStorySceneSectionLoader::StartTextureStream( SSectionLoaderContext& context )
{
	//SSPS_StartTextureStream;

	RED_ASSERT( m_textureStreamRequest == nullptr, TXT("Still have a pending texture stream request. How?") );
	SAFE_RELEASE( m_textureStreamRequest );
	RED_ASSERT( m_renderFramePrefetch == nullptr, TXT("Still have a pending frame prefetch. How?") );
	SAFE_RELEASE( m_renderFramePrefetch );

	// Go through the actors in this section, and request streaming for their entities.

	m_textureStreamRequest = GRender->CreateTextureStreamRequest( true );
	RED_ASSERT( m_textureStreamRequest != nullptr, TXT("Failed to create texture stream request") );

	if ( m_textureStreamRequest != nullptr )
	{
		m_scenePlayer->CollectPropEntitiesForStream( m_textureStreamRequest );

		if ( m_currentLoadedSectionDialogset != nullptr )
		{
			TDynArray< CName > actorNames;
			m_currentLoadedSectionDialogset->GetAllActorNames( actorNames );
			for ( const CName& name : actorNames )
			{
				// If there's already an entity created for this actor, then we can just add that directly.
				const CEntity* ent = m_scenePlayer->GetSceneActorEntity( name );
				if ( ent != nullptr )
				{
					m_textureStreamRequest->AddEntity( ent );
				}
				else
				{
					// No ready-made entity. So first, try and get the spawn definition from the story scene.
					THandle< CEntityTemplate > templ = nullptr;
					CName appearance;
					Bool byVoicetag;

					if ( m_currentLoadedSection->IsA< CStorySceneCutsceneSection >() )
					{
						const CStorySceneCutsceneSection* csSection = static_cast< const CStorySceneCutsceneSection* >( m_currentLoadedSection );
						if ( CCutsceneTemplate* csTemplate = csSection->GetCsTemplate() )
						{
							const SCutsceneActorDef* actorDef = csTemplate->GetActorDefinition( name );
							if ( actorDef != nullptr )
							{
								templ = actorDef->m_template.Get();
								appearance = actorDef->m_appearance;
							}
						}
					}
					else 
					{
						m_currentLoadedSection->GetScene()->GetActorSpawnDefinition( name, templ, appearance, byVoicetag );
					}

					// Add the template and possible appearance.
					if ( templ != nullptr )
					{
						m_textureStreamRequest->AddEntityTemplate( templ );
						m_textureStreamRequest->AddEntityAppearance( templ, appearance );
					}
				}
			}
		}

		// Didn't get definition from the scene, so check if maybe we're loading a cutscene section, and
		// maybe the cutscene defines an actor...
		// TODO : Is this needed? I guess if the actor is in the dialog set, it should have a definition.
		if ( const CStorySceneCutsceneSection* csSection = Cast< const CStorySceneCutsceneSection >( m_currentLoadedSection ) )
		{
			if ( CCutsceneTemplate* csTemplate = csSection->GetCsTemplate() )
			{
				TDynArray< String > actorNames;
				csTemplate->GetActorsName( actorNames );
				for ( const String& name : actorNames )
				{
					const SCutsceneActorDef* actorDef = csTemplate->GetActorDefinition( name );
					if ( actorDef == nullptr )
					{
						continue;
					}

					if ( CEntity* ent = m_scenePlayer->GetSceneActorEntity( actorDef->m_voiceTag ) )
					{
						m_textureStreamRequest->AddEntity( ent );
					}
					else
					{
						THandle< CEntityTemplate > templ = actorDef->m_template.Get();
						CName appearance = actorDef->m_appearance;

						// Add the template and possible appearance.
						if ( templ != nullptr )
						{
							m_textureStreamRequest->AddEntityTemplate( templ );
							m_textureStreamRequest->AddEntityAppearance( templ, appearance );
						}
					}
				}
			}
		}

		// Also add any base templates that were gathered and preloaded earlier. Many of these will probably have already
		// been picked up above, but there could be more than just the actors.
		for ( auto& handle : m_templatesRequiredBySection )
		{
			if ( CResource* res = handle.Get() )
			{
				if ( CEntityTemplate* templ = Cast< CEntityTemplate >( res ) )
				{
					m_textureStreamRequest->AddEntityTemplate( templ );
				}
			}
		}

		if ( !m_textureStreamRequest->IsReady() )
		{
			( new CRenderCommand_StartTextureStreamRequest( m_textureStreamRequest ) )->Commit();
		}
	}


	// Start a frame prefetch for the first camera in the section. We only do the first camera, and not all of the beginning
	// events (anything that would be prefetched before the start of the section), because we aren't necessarily starting
	// yet and there may be multiple next sections. This just hopes to get things mostly ready if this section is chosen, so
	// that when we actually change to it, there's less chance of having to wait.
	if ( !m_currentLoadedSection->IsGameplay() )
	{
		m_renderFramePrefetch = m_scenePlayer->StartSectionFirstFramePrefetch( FindPlanById( m_createdPlayingPlan ) );
	}
}



void CStorySceneSectionLoader::CreatePlan( SSectionLoaderContext& context )
{
	//SSPS_CreatePlan;

	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const CStorySceneSectionVariantId sectionVariantId = m_currentLoadedSection->GetVariantUsedByLocale( currentLocaleId );

	// Create and initialize playing plan
	CStorySceneSectionPlayingPlan* newPlan = new CStorySceneSectionPlayingPlan();

	{
		const Int32 newId = newPlan->m_id;
		SCENE_ASSERT( FindPlanById( newId ) == nullptr );
		SCENE_ASSERT( HasPlanInMapById( newId ) == false );
	}

	m_plans.PushBack( newPlan );

	SCENE_ASSERT( m_createdPlayingPlan == -1 );
	m_createdPlayingPlan = newPlan->m_id;
	newPlan->Create( m_currentLoadedSection, m_scenePlayer, sectionVariantId );
}

Bool CStorySceneSectionLoader::WaitForProcessing( SSectionLoaderContext& context )
{
	//SSPS_WaitForProcessing;

	if ( FindPlanById( m_createdPlayingPlan )->OnLoading_ProcessUsedEntities( m_scenePlayer, m_asyncLoading ) || m_procesEntitiesTimer > WAIT_FOR_PROC_TIMEOUT )
	{
		if ( m_procesEntitiesTimer > WAIT_FOR_PROC_TIMEOUT ) 
		{
			SCENE_LOG( TXT("Processing entities for scene timeout - (some item could not spawn?) ") );
		}
		m_procesEntitiesTimer = 0.f;
	}
	else
	{
		SCENE_ASSERT( m_asyncLoading );
		m_procesEntitiesTimer += context.timeDelta;
		return false;
	}
	return true;
}

Bool CStorySceneSectionLoader::WaitForSoundbanks( SSectionLoaderContext& context )
{
	//SSPS_WaitForSoundbanks;

	const TDynArray< CName > & banksDependency = m_scenePlayer->GetStoryScene()->GetBanksDependency();
	Uint32 banksCount = banksDependency.Size();
	for( Uint32 i = 0; i != banksCount; ++i )
	{
		CSoundBank* soundBank = CSoundBank::FindSoundBank( banksDependency[ i ] );
		if( !soundBank ) continue;

		if( !soundBank->IsLoaded() )
		{
			if( m_asyncLoading )
			{
				return false;
			}
			else
			{
				const Float waitTimeout = 5.0f;
				Red::System::Timer timer;
				while( !soundBank->IsLoadingFinished() && timer.GetSeconds() < waitTimeout )
				{
					Red::Threads::SleepOnCurrentThread( 10 );
				}
				RED_ASSERT( soundBank->IsLoaded(), TXT("CStorySceneSectionLoader::WaitForSoundbanks - sound bank didn't load properly - bank: [%ls] - result [%ls] - current section name: [%ls]"), 
					soundBank->GetFileName().AsChar(), soundBank->GetLoadingResultString().AsChar(), m_currentLoadedSection->GetName().AsChar() );
			}
		}
	}
	return true;
}

void CStorySceneSectionLoader::PreloadAnims( SSectionLoaderContext& context )
{
	//SSPS_PreloadAnims;
	CStorySceneAnimationContainer& c = m_scenePlayer->GetAnimationContainer();

	if ( m_currentLoadedSectionDialogset )
	{
		m_currentLoadedSectionDialogset->CollectUsedAnimations( c );
	}

	m_currentLoadedSection->CollectUsedAnimations( c );

	struct SCustomPreloadedAnimFunc : CStorySceneAnimationContainer::SPreloadedAnimFunc
	{
		Bool collectedNewTemplates;
		TDynArray< TSoftHandle< CResource > >& templatesRequiredBySection;

		SCustomPreloadedAnimFunc( TDynArray< TSoftHandle< CResource > >& templ ) : templatesRequiredBySection( templ ), collectedNewTemplates( false )
		{}
		virtual void OnAnimationPreloaded( const CEntity* e, const CSkeletalAnimationSetEntry* a )
		{
			for( auto iter = CSkeletalAnimationSetEntry::EventsIterator( a ); iter; ++iter )
			{
				if ( const CExtAnimItemEvent* evt = Cast<CExtAnimItemEvent>( *iter ) )
				{
					if ( CName itemName = evt->GetItemName() )
					{
						CStorySceneSectionLoader::CollectItemTemplate( e, itemName, templatesRequiredBySection );
						collectedNewTemplates = true;
					}
				}
			}
		}
	} pred( m_templatesRequiredBySection );

	c.PreloadNewAnimations( m_scenePlayer, &pred );

	m_collectedNewTemplates = m_collectedNewTemplates || pred.collectedNewTemplates;
}

void CStorySceneSectionLoader::CollectIncludedTemplates( SSectionLoaderContext& context )
{
	//SSPS_CollectIncludedTemplates;
	Uint32 oldSize = m_templatesRequiredBySection.Size();

	if ( const CStorySceneCutsceneSection* csSection = Cast< const CStorySceneCutsceneSection >( m_currentLoadedSection ) )
	{
		if ( CCutsceneTemplate* csTemplate = csSection->GetCsTemplate() )
		{
			TDynArray< CExtAnimEvent* > events;

			const Uint32 numAnimations = csTemplate->GetNumAnimations();
			for ( Uint32 i = 0; i < numAnimations; ++i )
			{
				CName animName = csTemplate->GetAnimationName( i );
				String actorName = csTemplate->GetActorName( animName );
				const SCutsceneActorDef* actorDef = csTemplate->GetActorDefinition( actorName );
				CEntity* ent = actorDef ? m_scenePlayer->GetSceneActorEntity( actorDef->m_voiceTag ) : nullptr;
				const TSoftHandle< CEntityTemplate > templ = actorDef->m_template;
				const CEntityTemplate* entTempl = templ.Get();
				if( !ent && ( actorDef->m_type == CAT_Actor || actorDef->m_type == CAT_Prop ) )
				{
					ent = entTempl->GetEntityObject();
				}
				if (!ent)
				{
					continue;
				}

				events.ClearFast();
				csTemplate->GetEventsForAnimation( animName, events );
				for ( const CExtAnimEvent* evt : events )
				{
					// Item change
					if ( const CExtAnimItemEvent* itemEvt = Cast< CExtAnimItemEvent >( evt ) )
					{
						// Only care about mounting a new item.
						if ( itemEvt->GetAction() != IA_Unmount )
						{
							CName itemName = itemEvt->GetItemName();
							CollectItemTemplate( ent, itemName, m_templatesRequiredBySection );
						}
						else
						{
							CGameplayEntity* gpent = Cast<CGameplayEntity>(ent);
							if( gpent && itemEvt->GetCategory() )
							{
								if( CInventoryComponent* inv = gpent->GetInventoryComponent() )
								{
									CName defItem = inv->GetCategoryDefaultItem( itemEvt->GetCategory() );
									CollectItemTemplate( ent, defItem, m_templatesRequiredBySection );
								}
							}
						}
					}
					else if( const CExtAnimCutsceneBodyPartEvent* appearanceEvt = Cast< CExtAnimCutsceneBodyPartEvent >( evt ) )
					{			
						CollectAppearanceTemplate( ent, entTempl, appearanceEvt->GetAppearanceName(), m_templatesRequiredBySection );
					}
				}
			}
		}
	}
	// Also check for normal story scene events. // and for item templates from animation events in animations
	{
		CStorySceneSectionVariantId variantId = m_currentLoadedSection->GetVariantUsedByLocale( SLocalizationManager::GetInstance().GetCurrentLocaleId() );
		const auto& events = m_currentLoadedSection->GetEvents( variantId );
		for ( CGUID evtGuid : events )
		{
			const CStorySceneEvent* evt = m_currentLoadedSection->GetEvent( evtGuid );
			if ( const CStorySceneEventEquipItem* equipEvt = Cast< const CStorySceneEventEquipItem >( evt ) )
			{
				CName actorName = equipEvt->GetActorName();
				CEntity* ent = m_scenePlayer->GetSceneActorEntity( actorName );

				CName leftItem = equipEvt->GetLeftItem();
				CName rightItem = equipEvt->GetRightItem();

				CollectItemTemplate( ent, leftItem, m_templatesRequiredBySection );
				CollectItemTemplate( ent, rightItem, m_templatesRequiredBySection );
			}
			else if( const CStorySceneEventApplyAppearance* ev = Cast< const CStorySceneEventApplyAppearance >( evt ) ) 
			{				
				if( CEntity* ent = m_scenePlayer->GetSceneActorEntity( ev->GetActor() ) )
				{
					if( const CEntityTemplate* entTempl = ent->GetEntityTemplate() )
					{
						CollectAppearanceTemplate( ent, entTempl, ev->GetAppearance(), m_templatesRequiredBySection );
					}
				}
			}
		}
	}		

	m_collectedNewTemplates = oldSize < m_templatesRequiredBySection.Size();
}

Bool CStorySceneSectionLoader::WaitForAnims( SSectionLoaderContext& context )
{
	//SSPS_WaitForAnims;
	if ( m_asyncLoading )
	{
		const CStorySceneAnimationContainer& c = m_scenePlayer->GetAnimationContainer();

		const Bool isLoaded = c.WaitForAllAnimations();
		if ( !isLoaded )
		{
			m_animLoadingTime += context.timeDelta;

			if ( m_animLoadingTime < 5.f )
			{
				return false;
			}
			else
			{
				SCENE_ASSERT( 0 );
			}
		}
	}

	return true;
}

void CStorySceneSectionLoader::RequestRenderResources( SSectionLoaderContext& context )
{
	//for ( auto& handle : m_templatesRequiredBySection )
	//{
	//	CResource* res = handle.Get();
	//	if ( res )
	//	{
	//		if ( res->IsExactlyA< CEntityTemplate >() )
	//		{
	//			for ( ComponentIterator< CMeshTypeComponent > it( static_cast< CEntityTemplate* >(res)->GetEntityObject() ); it; ++it )
	//			{
	//				if( CMeshTypeResource* resource = (*it)->GetMeshTypeResource() )
	//				{
	//					resource->GetRenderResource();
	//				}
	//			}
	//		}
	//		if ( res->IsExactlyA< CMesh >() )
	//		{
	//			static_cast< CMesh* >( res )->GetRenderResource();
	//		}
	//		//else if ( res->IsExactlyA< CBitmapTexture >() )
	//		//{
	//		//	static_cast< CBitmapTexture* >(res)->GetRenderResource();
	//		//}
	//	}
	//}
}

void CStorySceneSectionLoader::SerializeForGC( IFile& file )
{
	RED_FATAL_ASSERT( file.IsGarbageCollector(), "GC Only" );

	for ( auto& handle : m_templatesRequiredBySection )
	{
		CDiskFile* depotFile = GDepot->FindFile( handle.GetPath() );
		if ( depotFile )
		{
			CResource* res = depotFile->GetResource();
			file << res;
		}
		else
		{
			BaseSoftHandle& baseHandle = (BaseSoftHandle&) handle;
			THandle< CResource > res = (THandle< CResource >&) baseHandle.GetHandle();
			file << res;
		}
	}
}

void CStorySceneSectionLoader::OnTick( Float timeDelta )
{
	SSectionLoaderContext context;
	context.timeDelta = timeDelta;

	if ( m_currentLoadedSection == nullptr )
	{
		// Start loading next plan
		if ( m_sectionRequestQueue.Empty() == false )
		{
			SStorySceneSectionRequest& sectionRequest = m_sectionRequestQueue.Front();

			m_currentLoadedSection = sectionRequest.m_section;
			m_currentLoadedSectionDialogset = sectionRequest.m_dialogset;
			m_scenePlayer = sectionRequest.m_player;
			m_currentLoadedSectionStartTime = sectionRequest.m_startTime;
			m_preloadState = SSPS_Init;

			m_sectionRequestQueue.PopFront();
		}
	}

	if ( m_currentLoadedSection != nullptr )
	{
		if ( m_preloadState == SSPS_Init )
		{
#ifdef USE_STORY_SCENE_LOADING_STATS
			m_loadingStats.Init( m_currentLoadedSection->GetName() );
			m_loadingStats.StartA();
#endif

			BEGIN_SS_LOADING( m_loadingStats, "BackgroundLoading" );
			BEGIN_SS_LOADING( m_loadingStats, "Part 1" );

			// Go to new state
			m_preloadState = SSPS_GetTemplates;
		}

#ifdef USE_STORY_SCENE_LOADING_STATS
		SCENE_ASSERT( m_loadingStats.IsInitialized() );
#endif

		if ( m_preloadState == SSPS_GetTemplates )
		{
			SS_LOADING_SCOPE( m_loadingStats, "GetTemplates" );

			GetTemplates( context );

			// Go to new state
			m_preloadState = SSPS_PreloadStrings;
		}

		if ( m_preloadState == SSPS_PreloadStrings )
		{
			SS_LOADING_SCOPE( m_loadingStats, "PreloadStrings" );

			PreloadStrings( context );

			// Go to new state
			m_preloadState = SSPS_WaitForTemplates;
			BEGIN_SS_LOADING( m_loadingStats, "WaitForTemplates" );
		}

		if ( m_preloadState == SSPS_WaitForTemplates )
		{
			if ( WaitForTemplates( context ) )
			{
				// Go to new state
				m_preloadState = SSPS_EnsureActors;
				END_SS_LOADING( m_loadingStats, "WaitForTemplates" );
			}
		}

		if ( m_preloadState == SSPS_EnsureActors )
		{
			SS_LOADING_SCOPE( m_loadingStats, "EnsureActors" );

			EnsureActors( context );

			// Go to new state
			m_preloadState = SSPS_CollectIncludedTemplates;
		}

		if ( m_preloadState == SSPS_CollectIncludedTemplates )
		{
			SS_LOADING_SCOPE( m_loadingStats, "CollectIncludedTemplates" );

			CollectIncludedTemplates( context );

			// Go to new state
			m_preloadState = SSPS_PreloadAnims;

			END_SS_LOADING( m_loadingStats, "Part 1" );
			BEGIN_SS_LOADING( m_loadingStats, "Part 2" );
		}

		if ( m_preloadState == SSPS_PreloadAnims )
		{
			SS_LOADING_SCOPE( m_loadingStats, "PreloadAnims" );

			PreloadAnims( context );

			if ( m_collectedNewTemplates )
			{
				// Go to new state
				m_preloadState = SSPS_WaitForIncludedTemplates;
				BEGIN_SS_LOADING( m_loadingStats, "WaitForIncludedTemplates" );
			}
			else
			{
				m_animLoadingTime = 0.f;

				// Go to new state
				m_preloadState = SSPS_WaitForAnims;
			}
		}

		if ( m_preloadState == SSPS_WaitForIncludedTemplates )
		{
			if ( WaitForTemplates( context ) )
			{
				// Go to new state
				m_preloadState = SSPS_RequestRenderResources;
				END_SS_LOADING( m_loadingStats, "WaitForIncludedTemplates" );
			}
		}

		if ( m_preloadState == SSPS_RequestRenderResources )
		{
			SS_LOADING_SCOPE( m_loadingStats, "RequestRenderResources" );

			RequestRenderResources( context );

			// Go to new state
			m_preloadState = SSPS_WaitForAnims;
		}

		if ( m_preloadState == SSPS_WaitForAnims )
		{
			BEGIN_SS_LOADING_ONCE( m_loadingStats, "WaitForAnims" );

			if ( WaitForAnims( context ) )
			{
				// Go to new state
				m_preloadState = SSPS_WaitForStrings;
				END_SS_LOADING( m_loadingStats, "WaitForAnims" );
				END_SS_LOADING( m_loadingStats, "Part 2" );
				BEGIN_SS_LOADING( m_loadingStats, "Part 3" );
				BEGIN_SS_LOADING( m_loadingStats, "WaitForStrings" );
			}
		}

		if ( m_preloadState == SSPS_WaitForStrings )
		{
			if ( WaitForStrings( context ) )
			{
				// Go to new state
				m_preloadState = SSPS_WaitForSoundbanks;
				END_SS_LOADING( m_loadingStats, "WaitForStrings" );
				BEGIN_SS_LOADING( m_loadingStats, "WaitForSoundbanks" );
			}
		}

		if ( m_preloadState == SSPS_WaitForSoundbanks )
		{
			if ( WaitForSoundbanks( context ) )
			{
				// Go to new state
				m_preloadState = SSPS_CreatePlan;
				END_SS_LOADING( m_loadingStats, "WaitForSoundbanks" );
			}
		}

		if ( m_preloadState == SSPS_CreatePlan )
		{
			SS_LOADING_SCOPE( m_loadingStats, "CreatePlan" );
			CreatePlan( context );

			// Go to new state
			m_preloadState = SSPS_WaitForInit;
		}

		if ( m_preloadState == SSPS_WaitForInit )
		{
			// Go to new state
			m_preloadState = SSPS_WaitForProcessing;
			BEGIN_SS_LOADING( m_loadingStats, "WaitForProcessing" );
		}

		if ( m_preloadState == SSPS_WaitForProcessing )
		{
			if ( WaitForProcessing( context ) )
			{
				// Go to new state
				m_preloadState = SSPS_StartTextureStream;
				END_SS_LOADING( m_loadingStats, "WaitForProcessing" );
			}
		}

		if ( m_preloadState == SSPS_StartTextureStream )
		{
			SS_LOADING_SCOPE( m_loadingStats, "StartTextureStream" );
			StartTextureStream( context );

			// Go to new state
			m_preloadState = SSPS_Cleanup;
		}

		if ( m_preloadState == SSPS_Cleanup )
		{
			END_SS_LOADING( m_loadingStats, "Part 3" );
			END_SS_LOADING( m_loadingStats, "BackgroundLoading" );

#ifdef USE_STORY_SCENE_LOADING_STATS
			m_loadingStats.StopA();
#endif

			GCommonGame->GetSystem< CStorySceneSystem >()->FlushLoadingVisibleData();

			SCENE_ASSERT( FindPlanById( m_createdPlayingPlan )->GetSection()->GetScene() );

			m_preloadedSectionPlans.Insert( FindPlanById( m_createdPlayingPlan )->GetSection()->GetSectionUniqueId(), FindPlanById( m_createdPlayingPlan )->m_id );
			SCENE_ASSERT( HasPlanById( m_createdPlayingPlan ) );

			CStorySceneSectionPlayingPlan* p = FindPlanById( m_createdPlayingPlan );
			p->SetValid();
			p->SetTextureStreamRequest( m_textureStreamRequest );
			p->SetRenderFramePrefetch( m_renderFramePrefetch );
#ifdef USE_STORY_SCENE_LOADING_STATS
			p->m_loadingStats = m_loadingStats;
			m_loadingStats.Deinit();
#endif

			m_textureStreamRequest = nullptr;
			m_renderFramePrefetch = nullptr;

			// Release all templates
			for ( TSoftHandle< CResource >& templateHandle : m_templatesRequiredBySection )
			{
				templateHandle.Release();
			}

			// Cleanup temporary loading data
			m_stringsRequiredBySection.Clear();

			m_createdPlayingPlan = -1;
			m_currentLoadedSection = nullptr;
			m_currentLoadedSectionDialogset = nullptr;
			m_scenePlayer = nullptr;

			m_preloadState = SSPS_None;
		}
	}

	if ( !m_asyncLoading )
	{
		SCENE_ASSERT( m_preloadState == SSPS_None );
	}
}

Bool CStorySceneSectionLoader::IsPlanReady( Uint64 sectionId ) const
{
	return m_preloadedSectionPlans.KeyExist( sectionId );
}

Bool CStorySceneSectionLoader::IsAsync() const
{
	return m_asyncLoading;
}

Bool CStorySceneSectionLoader::HasPlanInMapById( Int32 id ) const
{
	for ( THashMap< Uint64, Int32 >::const_iterator planIter = m_preloadedSectionPlans.Begin(); planIter != m_preloadedSectionPlans.End(); ++planIter ) 
	{
		if ( planIter->m_second == id )
		{
			return true;
		}
	}

	return false;
}

Bool CStorySceneSectionLoader::HasPlanInQueue( const CStorySceneSection* section ) const
{
	for ( auto it = m_sectionRequestQueue.Begin(); it != m_sectionRequestQueue.End(); ++it )
	{
		const SStorySceneSectionRequest& s = *it;
		if ( s.m_section == section )
		{
			return true;
		}
	}

	return false;
}

void CStorySceneSectionLoader::ForgetPreloadedPlans( CStorySceneSectionPlayingPlan* planToIgnore /*= NULL */ )
{
	TDynArray< CStorySceneSectionPlayingPlan* > plansToKeep;
	if ( planToIgnore )
	{
		plansToKeep.PushBack( planToIgnore );
	}

	// Cleanup preloaded sections
	for ( THashMap< Uint64, Int32 >::iterator planIter = m_preloadedSectionPlans.Begin(); planIter != m_preloadedSectionPlans.End(); ++planIter ) 
	{
		const Int32 id = planIter->m_second;

		CStorySceneSectionPlayingPlan* plan = FindPlanById( id );

		if ( plan == nullptr || plan == planToIgnore )
		{
			continue;
		}

		if ( plan->GetPreventRemoval() )
		{
			plansToKeep.PushBackUnique( plan );
			continue;
		}

		planIter->m_second = -1;

		// Cancel the plan's texture stream request.
		IRenderTextureStreamRequest* texRequest = plan->GetTextureStreamRequest();
		plan->SetTextureStreamRequest( nullptr );
		( new CRenderCommand_CancelTextureStreamRequest( texRequest ) )->Commit();
		SAFE_RELEASE( texRequest );

		DeletePlanById( id );

		SCENE_ASSERT( !HasPlanInMapById( id ) );
	}
	m_preloadedSectionPlans.Clear();

	for ( auto it = plansToKeep.Begin(), end = plansToKeep.End(); it != end; ++it )
	{
		CStorySceneSectionPlayingPlan* plan = *it;

		m_preloadedSectionPlans.Insert( plan->GetSection()->GetSectionUniqueId(), plan->m_id );
		SCENE_ASSERT( HasPlanById( plan->m_id  ) );
	}

	SCENE_ASSERT( m_plans.Size() == m_preloadedSectionPlans.Size() );

	if ( m_textureStreamRequest != nullptr )
	{
		// Also cancel any texture request we may have had in-progress.
		( new CRenderCommand_CancelTextureStreamRequest( m_textureStreamRequest ) )->Commit();
		SAFE_RELEASE( m_textureStreamRequest );
	}
	SAFE_RELEASE( m_renderFramePrefetch );
}

Bool CStorySceneSectionLoader::EnsureElementsAreLoaded( const CStorySceneSection* section )
{
	if ( section == nullptr )
	{
		return true;
	}

	Bool elementsAreLoaded = true;
	TDynArray< Uint32 > elementStrings;
	section->GetLocalizedStringIds( elementStrings );

	for ( TDynArray< Uint32 >::const_iterator stringIter = elementStrings.Begin();
		stringIter != elementStrings.End(); ++stringIter )
	{
		elementsAreLoaded &= SLocalizationManager::GetInstance().ValidateLanguagePackLoad( *stringIter );
	}

	return elementsAreLoaded;
}

Bool CStorySceneSectionLoader::EnsureNextElementsHaveSpeeches( const CStorySceneSectionPlayingPlan* playingPlan )
{
	return playingPlan->OnLoading_EnsureNextElementsHaveSpeeches();
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif
