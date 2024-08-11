/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "storyScenePlayerStats.h"

class CStorySceneSection;
class CStorySceneSectionPlayingPlan;
class CStoryScenePlayer;
class IRenderTextureStreamRequest;
class IRenderFramePrefetch;

struct SStorySceneSectionRequest
{
	const CStorySceneSection*	m_section;
	const CStorySceneDialogsetInstance* m_dialogset;
	CStoryScenePlayer*			m_player;		// TODO - remove this
	Double						m_startTime;

	SStorySceneSectionRequest( const CStorySceneSection* requestedSection, const CStorySceneDialogsetInstance* dialogset, CStoryScenePlayer* player )
		: m_section( requestedSection )
		, m_dialogset( dialogset )
		, m_player( player )
	{
		SCENE_ASSERT( requestedSection );
		m_startTime = Red::System::Clock::GetInstance().GetTimer().GetSeconds();
	}

	Float GetLoadingTime() const
	{
		return (Float) ( Red::System::Clock::GetInstance().GetTimer().GetSeconds() - m_startTime );
	}
};

enum EStorySectionPreloadState
{
	SSPS_None,
	SSPS_Init,
	SSPS_GetTemplates,
	SSPS_WaitForTemplates,
	SSPS_WaitForIncludedTemplates,
	SSPS_PreloadStrings,
	SSPS_WaitForStrings,
	SSPS_EnsureActors,
	SSPS_StartTextureStream,
	SSPS_CreatePlan,
	SSPS_WaitForInit,
	SSPS_WaitForProcessing,
	SSPS_Cleanup,
	SSPS_WaitForSoundbanks,
	SSPS_PreloadAnims,
	SSPS_CollectIncludedTemplates,
	SSPS_WaitForAnims,
	SSPS_RequestRenderResources,
};

class CStorySceneSectionLoader
{
private:
	static const Float  WAIT_FOR_PROC_TIMEOUT;

	struct SSectionLoaderContext
	{
		Float timeDelta;
	};

	TList< SStorySceneSectionRequest >				m_sectionRequestQueue;
	THashMap< Uint64, Int32 >						m_preloadedSectionPlans;		// Maps section id to playing plan id.
	TDynArray< CStorySceneSectionPlayingPlan* >		m_plans;

	const CStorySceneSection*						m_currentLoadedSection;
	const CStorySceneDialogsetInstance*				m_currentLoadedSectionDialogset;
	Double											m_currentLoadedSectionStartTime;
	CStoryScenePlayer*								m_scenePlayer;
	TDynArray< TSoftHandle< CResource > >			m_templatesRequiredBySection;
	Bool											m_collectedNewTemplates;
	TDynArray< Uint32 >								m_stringsRequiredBySection;
	Int32											m_createdPlayingPlan;
	Float											m_procesEntitiesTimer;
	Float											m_animLoadingTime;

	IRenderTextureStreamRequest*					m_textureStreamRequest;
	IRenderFramePrefetch*							m_renderFramePrefetch;

#ifdef USE_STORY_SCENE_LOADING_STATS
	CStorySceneLoadingStat							m_loadingStats;
#endif

public:
	EStorySectionPreloadState						m_preloadState;
	Bool											m_asyncLoading;

public:
	CStorySceneSectionLoader();
	~CStorySceneSectionLoader();

	void RequestPlayingPlan( const CStorySceneSection* requestedSection, CStoryScenePlayer* scenePlayer, Bool highPriority, const CStorySceneDialogsetInstance* currDialogset );
	Bool MakeRequestASAP( const CStorySceneSection* requestedSection );
	Bool HasPlanRequested( const CStorySceneSection* requestedSection ) const;
	Float GetLoadingTime( const CStorySceneSection* section );
	Bool HasPlanInQueue( const CStorySceneSection* section ) const;
	Bool HasAnythingInQueue() const { return !m_sectionRequestQueue.Empty(); }

	Bool HasLoadedSection() const { return m_currentLoadedSection != nullptr; }

	const CStorySceneSectionPlayingPlan* GetPlayingPlan( const CStorySceneSection* section ) const;
	CStorySceneSectionPlayingPlan* GetPlayingPlan( const CStorySceneSection* section );
	Int32 GetPlayingPlanId( const CStorySceneSection* section );

	Bool ForgetPlayingPlan( Uint64 sectionId );

	void ForgetPreloadedPlans( CStorySceneSectionPlayingPlan* planToIgnore = NULL );

	Bool EnsureElementsAreLoaded( const CStorySceneSection* section );

	Bool EnsureNextElementsHaveSpeeches( const CStorySceneSectionPlayingPlan* playingPlan );

	Bool IsPlanReady( Uint64 sectionId ) const;
	Bool IsAsync() const;

	void ClearPreloads();

	void SerializeForGC( IFile& file );

	void OnTick( Float timeDelta );

	CStorySceneSectionPlayingPlan* FindPlanById( Int32 id );
	const CStorySceneSectionPlayingPlan* FindPlanById( Int32 id ) const;

	Bool HasPlanById( Int32 id ) const;
	Bool HasPlanInMapById( Int32 id ) const;
	
	Bool DeletePlanById( Int32 id );

	static void CollectItemTemplate( const CEntity* entity, CName itemName, TDynArray< TSoftHandle< CResource > >& templates );


	void GetTemplates( SSectionLoaderContext& context );
	void PreloadStrings( SSectionLoaderContext& context );
	Bool WaitForTemplates( SSectionLoaderContext& context );
	Bool WaitForStrings( SSectionLoaderContext& context );
	void EnsureActors( SSectionLoaderContext& context );
	void StartTextureStream( SSectionLoaderContext& context );
	void CreatePlan( SSectionLoaderContext& context );
	Bool WaitForProcessing( SSectionLoaderContext& context );
	Bool WaitForSoundbanks( SSectionLoaderContext& context );
	void PreloadAnims( SSectionLoaderContext& context );
	void CollectIncludedTemplates( SSectionLoaderContext& context );
	Bool WaitForAnims( SSectionLoaderContext& context );
	void RequestRenderResources( SSectionLoaderContext& context );
};