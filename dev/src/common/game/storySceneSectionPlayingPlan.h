/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storySceneIncludes.h"
#include "storySceneElement.h"
#include "storySceneEvent.h"
#include "storySceneSectionVariant.h"
#include "storyScenePlayerStats.h"

class CStorySceneSection;
class CStoryScenePlayer;
class IRenderTextureStreamRequest;
class IRenderFramePrefetch;

class CStorySceneSectionPlayingPlan : public IStorySceneObject
{
	static Int32									ID;

public:
	Int32											m_id;
	Bool											m_valid;
	CStoryScenePlayer*								m_player;

#ifdef USE_STORY_SCENE_LOADING_STATS
	CStorySceneLoadingStat							m_loadingStats;
#endif

	Int32											m_currElementIndex;			// Index of current element. Used with m_sectionInstanceData.m_elements array (and associated arrays).
																				// May also be equal to -1 or to number of elements in m_sectionInstanceData.m_elements.
	CStorySceneSection*								m_section;
	CStorySceneSectionVariantId						m_sectionVariantId;			// Section variant used by this playing plan.
	Bool											m_localVoMatchApprovedVo;	// Indicates whether local VO match approved VO.

	TDynArray< const  CStorySceneSection* >			m_possibleNextSections;

	Bool											m_preventRemoval;

	IRenderTextureStreamRequest*					m_textureStreamRequest;		// Important texture request. Mainly for characters that need to be loaded.
	IRenderFramePrefetch*							m_renderFramePrefetch;

public:
	typedef TPair< IStorySceneElementInstanceData*, TDynArray< const CStorySceneEvent* > >	TEventInstance;

	struct LineMarker
	{
		CName	m_actorId;
		Float	m_startTimeMS;
		Float	m_endTimeMS;

		LineMarker( Float startMS, Float endMS, CName actor ) : m_startTimeMS( startMS ), m_endTimeMS( endMS ), m_actorId( actor ) {}
	};

	struct CameraMarker
	{
		Matrix	m_cameraMatrixSceneLocal;
		Float	m_cameraFov;
		Float	m_eventTime;

		CameraMarker( Float time, const Matrix& mtx, Float fov ) : m_eventTime( time ), m_cameraMatrixSceneLocal( mtx ), m_cameraFov( fov ) {}
	};

	struct InstanceData
	{
		TDynArray< const CStorySceneElement* >			m_elements;			// Note that this also contains choice element if section has one.
		TDynArray< IStorySceneElementInstanceData* >	m_elemData;			// Note that m_elemData[i] is associated with m_elements[i].
		TDynArray< TEventInstance >						m_eventData;		// Note that m_eventData[i] is associated with m_elements[i]. Only events from chosen section variant are here.
		TDynArray< const CStorySceneEvent* >			m_evts;				// Contains all events associated with playing plan elements, sorted by start time. Only events with proper variant id are here.
		TDynArray< LineMarker >							m_cachedLineMarkers;
		TDynArray< CameraMarker >						m_cachedCameraMarkers;

		InstanceDataLayout								m_layout;
		InstanceBuffer*									m_data;
	};
	InstanceData									m_sectionInstanceData;

private:
	//CStorySceneSectionPlayingPlan( const CStorySceneSectionPlayingPlan& );
	//CStorySceneSectionPlayingPlan& operator=( const CStorySceneSectionPlayingPlan& rhs );

public:
	CStorySceneSectionPlayingPlan();
	~CStorySceneSectionPlayingPlan();

	void Create( const CStorySceneSection* section, CStoryScenePlayer* player, CStorySceneSectionVariantId sectionVariantId );
	void Cleanup();

	Bool IsValid() const { return m_valid; }
	void SetValid() { ASSERT( !IsValid() ); m_valid = true; }

	void ForceSkip();
	void StopAllInstances();

	Bool CalcCurrElemTimeOffset( Float timeToTest, Float& timeOffset ) const;

	Uint32 GetNumElements() const;
	Bool HasElements() const;
	Bool HasNextElement() const;
	Bool HasNextSection() const;

	const IStorySceneElementInstanceData* GetElement( Uint32 index ) const;
	const IStorySceneElementInstanceData* GetCurrElement() const;
	IStorySceneElementInstanceData* GetCurrElement();

	IStorySceneElementInstanceData* GoToNextElement();
	IStorySceneElementInstanceData* GoToPrevElement();
	IStorySceneElementInstanceData* GoToFirstElement();

	Float GetCurrentSectionTime() const;

	RED_INLINE const CStorySceneSection* GetSection() const { return m_section; }

	Bool LocalVoMatchApprovedVo() const;

	// Assign and assume ownership -- doesn't addref, but will release later.
	RED_INLINE void SetTextureStreamRequest( IRenderTextureStreamRequest* request ) { m_textureStreamRequest = request; }
	RED_INLINE IRenderTextureStreamRequest* GetTextureStreamRequest() const { return m_textureStreamRequest; }
	// Assign and assume ownership -- doesn't addref, but will release later.
	RED_INLINE void SetRenderFramePrefetch( IRenderFramePrefetch* request ) { m_renderFramePrefetch = request; }
	RED_INLINE IRenderFramePrefetch* GetRenderFramePrefetch() const { return m_renderFramePrefetch; }

public: // HACKS
	void SetPreventRemoval( Bool preventRemoval ) { m_preventRemoval = preventRemoval; }
	Bool GetPreventRemoval() const { return m_preventRemoval; }

	RED_INLINE const TDynArray< const CStorySceneSection* >& GetNextSections() const { return m_possibleNextSections; }
	RED_INLINE CStorySceneSection* GetSection() { return m_section; }

public: // Loading - TODO - all code should be in loader
	Bool OnLoading_AreAllElementsReady() const;
	Bool OnLoading_ProcessUsedEntities( CStoryScenePlayer* actorProvider, Bool asyncLoading ) const;
	Bool OnLoading_EnsureNextElementsHaveSpeeches() const;

protected:
	void OnLoading_ProcessUsedEntity( CEntity* entity, Bool isScenePrecached ) const;
	Bool OnLoading_CanProcessUsedEntity( CEntity* entity, Bool asyncLoading ) const;

	void InitElements();
	void InitEvents();
	void DeinitAllEventInstaces();

	Float ComputeScalingFactor( const CStorySceneEvent& event ) const;
};

RED_INLINE Bool CStorySceneSectionPlayingPlan::LocalVoMatchApprovedVo() const
{
	return m_localVoMatchApprovedVo;
}
