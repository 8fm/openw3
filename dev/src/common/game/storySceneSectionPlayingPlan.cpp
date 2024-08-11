/**
* Copyright c 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneSectionPlayingPlan.h"
#include "storySceneCutsceneSection.h"
#include "storySceneEventEnterActor.h"
#include "storySceneEventExitActor.h"
#include "storySceneEventInterpolation.h"
#include "storySceneEventBlend.h"
#include "storySceneLine.h"
#include "storySceneChoice.h"
#include "storySceneChoiceLine.h"
#include "storySceneIncludes.h"
#include "storySceneUtils.h"
#include "storySceneControlPartsUtil.h"
#include "storySceneAbstractLine.h"
#include "storySceneCutscene.h"
#include "storySceneEventCamera.h"
#include "../engine/renderCommands.h"
#include "../engine/localizationManager.h"
#include "../engine/meshTypeComponent.h"
#include "../engine/meshTypeResource.h"
#include "../engine/renderProxyIterator.h"
#include "../engine/renderTextureStreamRequest.h"
#include "../engine/renderFramePrefetch.h"
#include "../engine/inGameConfig.h"
#include "questsSystem.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

namespace Config
{
	TConfigVar<Float> cvMaxLeadingSilencePercentage( "Scenes/SceneScaling", "MaxLeadingSilencePercentage", 0.0f, eConsoleVarFlag_ReadOnly );
	TConfigVar<Float> cvMaxLeadingSilenceInSeconds( "Scenes/SceneScaling", "MaxLeadingSilenceInSeconds", 0.0f, eConsoleVarFlag_ReadOnly );
	TConfigVar<Float> cvMaxTrailingSilencePercentage( "Scenes/SceneScaling", "MaxTrailingSilencePercentage", 0.1f, eConsoleVarFlag_ReadOnly );
	TConfigVar<Float> cvMaxTrailingSilenceInSeconds( "Scenes/SceneScaling", "MaxTrailingSilenceInSeconds", 0.5f, eConsoleVarFlag_ReadOnly );
}

Int32 CStorySceneSectionPlayingPlan::ID = 0;

CStorySceneSectionPlayingPlan::CStorySceneSectionPlayingPlan()
	: m_currElementIndex( -1 )
	, m_section( nullptr )
	, m_sectionVariantId( -1 )
	, m_localVoMatchApprovedVo( false )
	, m_player( nullptr )
	, m_valid( false )
	, m_preventRemoval( false )
	, m_textureStreamRequest( nullptr )
	, m_renderFramePrefetch( nullptr )
{
	m_id = ID++;
}

CStorySceneSectionPlayingPlan::~CStorySceneSectionPlayingPlan()
{
	SAFE_RELEASE( m_textureStreamRequest );
	SAFE_RELEASE( m_renderFramePrefetch );
}

void CStorySceneSectionPlayingPlan::Create( const CStorySceneSection* section, CStoryScenePlayer* player, CStorySceneSectionVariantId sectionVariantId )
{
	SCENE_ASSERT( section != NULL );
	SCENE_ASSERT( player != NULL );
	SCENE_ASSERT( !IsValid() );

	m_section = const_cast< CStorySceneSection* >( section );
	m_sectionVariantId = sectionVariantId;
	m_player = player;

	if ( !section->IsValid() )
	{
		return;
	}

	if ( !m_section->GetScene() )
	{
		SCENE_ASSERT( m_section->GetScene() );
		return;
	}

	m_sectionInstanceData.m_data = m_section->GetScene()->CreateInstanceBuffer( m_player, m_section, m_sectionVariantId, m_sectionInstanceData.m_layout );
	SCENE_ASSERT( m_sectionInstanceData.m_data );

	Uint32 numNestedElement = 0;

	// For each element (except for choice element) call OnStart()
	// and collect all events associated with that element.
	const Uint32 sectionElementsNumber = section->GetNumberOfElements();
	for ( Uint32 i = 0; i < sectionElementsNumber; ++i )
	{
		const CStorySceneElement* sceneElement = section->GetElement( i );
		if ( !sceneElement )
		{
			SCENE_ASSERT( sceneElement );
			continue;
		}

		// DIALOG_TOMSIN_TODO - co to oznacza???
		// Temporary solution before rewriting descriptors
		TDynArray< const CStorySceneElement* > nestedElements;
		sceneElement->OnGetSchedulableElements( nestedElements );

		for( Uint32 j = 0; j < nestedElements.Size(); ++j )
		{
			const CStorySceneElement* nestedElement = nestedElements[ j ];

			if ( nestedElement->IsPlayable() )
			{
				IStorySceneElementInstanceData* elementInstance2 = nestedElement->OnStart( player );

				if ( elementInstance2 )
				{
					m_sectionInstanceData.m_elements.PushBack( nestedElement );
					m_sectionInstanceData.m_elemData.PushBack( elementInstance2 );

					TEventInstance eventInstanceData;
					eventInstanceData.m_first = elementInstance2;
					m_section->GetEventsForElement( eventInstanceData.m_second, nestedElement, m_sectionVariantId );
					m_sectionInstanceData.m_eventData.PushBack( eventInstanceData );

					numNestedElement++;
				}
			}	
		}
	}

	if ( section->IsA< CStorySceneCutsceneSection >() )
	{
		if ( section->GetChoice() )
		{
			SCENE_ASSERT( numNestedElement == 0 );
			SCENE_ASSERT( GetNumElements() == 0 );
		}
		else
		{
			SCENE_ASSERT( numNestedElement == 1 );
			SCENE_ASSERT( GetNumElements() == 1 );
		}
	}

	// If section contains choice element then call OnStart() and collect all events
	// associated with it (the same what was done earlier for all other elements).
	if ( section->GetChoice() )
	{
		IStorySceneElementInstanceData* elementInstance2 = section->GetChoice()->OnStart( player );
		if ( elementInstance2 )
		{
			m_sectionInstanceData.m_elements.PushBack( section->GetChoice() );
			m_sectionInstanceData.m_elemData.PushBack( elementInstance2 );

			TEventInstance eventInstanceData;
			eventInstanceData.m_first = elementInstance2;
			m_section->GetEventsForElement( eventInstanceData.m_second, section->GetChoice(), m_sectionVariantId );
			m_sectionInstanceData.m_eventData.PushBack( eventInstanceData );
		}
	}

	// Collect possible next sections.
	TDynArray< const CStorySceneSection* > elem;
	StorySceneControlPartUtils::GetNextElementsOfType( m_section, elem );
	for ( Uint32 i = 0; i < elem.Size(); i++  )
	{
		const CStorySceneSection* nextSection = elem[i];
		if ( nextSection )
		{
			m_possibleNextSections.PushBackUnique( nextSection );
		}
	}

	if( m_section->GetChoice() )
	{
		TDynArray< SSceneChoice >	avaiableChoiceLines;
		GCommonGame->GetSystem< CQuestsSystem >()->GetContextDialogChoices( avaiableChoiceLines, m_player->GetStoryScene(), nullptr, m_section->GetChoice() );
		for( SSceneChoice& choice : avaiableChoiceLines )
		{			
			if( choice.link )
			{
				elem.ClearFast();
				StorySceneControlPartUtils::GetNextElementsOfType( choice.link, elem );
				for ( Uint32 i = 0; i < elem.Size(); i++  )
				{
					const CStorySceneSection* nextSection = elem[i];
					if ( nextSection )
					{
						m_possibleNextSections.PushBackUnique( nextSection );
					}
				}
			}	
		}
	}	

	if ( m_sectionInstanceData.m_data )
	{
		m_player->GetStoryScene()->InitInstanceBuffer( m_sectionInstanceData.m_data, m_section, m_sectionVariantId );
	}

	InitElements();
	InitEvents();

	// Put all events associated with playing plan elements into one container and sort them by start time.
	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		TDynArray< const CStorySceneEvent* >& elementEvents = m_sectionInstanceData.m_eventData[ i ].m_second;
		m_sectionInstanceData.m_evts.PushBack( elementEvents );
	}
	auto sortByStartTime = [ this ] ( const CStorySceneEvent* a, const CStorySceneEvent* b ) -> Bool
	{
		return a->GetInstanceStartTime( *m_sectionInstanceData.m_data ) < b->GetInstanceStartTime( *m_sectionInstanceData.m_data );
	};
	::Sort( m_sectionInstanceData.m_evts.Begin(), m_sectionInstanceData.m_evts.End(), sortByStartTime );

	// Assert that all associated arrays have the same number of elements.
	SCENE_ASSERT( GetNumElements() == m_sectionInstanceData.m_elemData.Size() );
	SCENE_ASSERT( GetNumElements() == m_sectionInstanceData.m_eventData.Size() );
}

/*
Computes scaling factor that should be applied to specified event.

\param event Event for which to compute scaling factor.
*/
Float CStorySceneSectionPlayingPlan::ComputeScalingFactor( const CStorySceneEvent& event ) const
{
	if( !Cast< CStorySceneEventDuration >( &event ) )
	{
		// Scaling factor is 1.0f for point events, interpolation events and blend events.
		return 1.0f;
	}

	// ref - assume reference locale and accepted durations
	// curr - assume current locale and up-to-date durations

	// Get duration of event before scaling.
	const Float eventRefDuration = event.GetInstanceDuration( *m_sectionInstanceData.m_data );
	if( eventRefDuration <= NumericLimits< Float >::Epsilon() )
	{
		// Duration events whose duration is 0.0f get scaling factor of 1.0f.
		return 1.0f;
	}

	// Find/compute following pieces of information assuming reference locale and using accepted reference durations:
	// 1. Start time of scene element at which event starts.
	// 2. Event start time and end time.
	// 3. Scene element at which event ends and its start time.
	
	const CStorySceneElement* startElement = event.GetSceneElement();
	Uint32 startElementIndex = static_cast< Uint32 >( -1 );
	Float startElementRefStartTime = -1.0f;

	Float eventRefEndTime = -1.0f;

	const CStorySceneElement* endElement = nullptr;
	Uint32 endElementIndex = static_cast< Uint32 >( -1 );
	Float endElementRefStartTime = -1.0f;

	Float elementRefStartTime = 0.0f;
	Float elementRefEndTime = 0.0f;
	
	for( Uint32 iElement = 0, numElements = m_sectionInstanceData.m_elements.Size(); iElement < numElements; ++iElement )
	{
		const CStorySceneElement* element = m_sectionInstanceData.m_elements[ iElement ];

		// Compute start time, end time and reference duration of current element.
		// Element reference duration is the one that was accepted by the user.
		// If accepted reference duration is not set then we use current duration
		// which means that this element will not influence scaling.
		Float elementRefDuration = m_section->GetElementApprovedDuration( m_sectionVariantId, element->GetElementID() );
		if( elementRefDuration < 0.0f )
		{
			elementRefDuration = m_sectionInstanceData.m_elemData[ iElement ]->GetDuration();
		}
		elementRefStartTime = elementRefEndTime;
		elementRefEndTime += elementRefDuration;

		if( element == startElement )
		{
			// Now we know start element start time and we can compute event end time.
			startElementRefStartTime = elementRefStartTime;
			startElementIndex = iElement;
			eventRefEndTime = startElementRefStartTime + event.GetStartPosition() * elementRefDuration + eventRefDuration;
		}

		if( eventRefEndTime > -1.0f && eventRefEndTime < elementRefEndTime )
		{
			// Now we know end element and its start time.
			endElement = element;
			endElementIndex = iElement;
			endElementRefStartTime = elementRefStartTime;

			break;
		}
	}

	if( !endElement )
	{
		// We didn't find element at which event ends. This means that event falls beyond last scene element.
		// In such case we consider last scene element to be the one at which event ends.
		const Uint32 lastElementIndex = m_sectionInstanceData.m_elements.Size() - 1;
		endElement = m_sectionInstanceData.m_elements[ lastElementIndex ];
		endElementIndex = lastElementIndex;
		endElementRefStartTime = elementRefStartTime;
	}

	// Compute end element reference duration. If no duration is accepted then we use current duration.
	Float endElementRefDuration = m_section->GetElementApprovedDuration( m_sectionVariantId, endElement->GetElementID() );
	if( endElementRefDuration < 0.0f )
	{
		endElementRefDuration = m_sectionInstanceData.m_elemData[ endElementIndex ]->GetDuration();
	}

	// Compute relative start and relative end positions of event in reference locale (and using accepted reference durations).
	// Note that relativeEndPos may be >= 1.0f for events that fall beyond last element of a section - we are ok with this.
	const Float relativeStartPos = event.GetStartPosition();
	const Float relativeEndPos = ( eventRefEndTime - endElementRefStartTime ) / endElementRefDuration;

	// Compute what would be duration of event in current locale (and using most up-to-date
	// durations) if we kept the same relative start position and relative end position.

	const IStorySceneElementInstanceData* startElementInst = m_sectionInstanceData.m_elemData[ startElementIndex ];
	const Float eventCurrStartTime = startElementInst->GetStartTime() + relativeStartPos * startElementInst->GetDuration();

	const IStorySceneElementInstanceData* endElementInst = m_sectionInstanceData.m_elemData[ endElementIndex ];
	const Float eventCurrEndTime = endElementInst->GetStartTime() + relativeEndPos * endElementInst->GetDuration();

	const Float eventCurrDuration = eventCurrEndTime - eventCurrStartTime;

	// Now we can finally compute scaling factor.
	const Float scalingFactor = eventCurrDuration / eventRefDuration;

	return scalingFactor;
}

void CStorySceneSectionPlayingPlan::ForceSkip()
{
	SCENE_ASSERT( IsValid() );

	m_currElementIndex = m_sectionInstanceData.m_elemData.SizeInt();
}

void CStorySceneSectionPlayingPlan::StopAllInstances()
{
	SCENE_ASSERT( IsValid() );

	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		IStorySceneElementInstanceData* data = m_sectionInstanceData.m_elemData[ i ];
		if( data->IsRunning() )
		{
			m_sectionInstanceData.m_elemData[ i ]->Stop();
		}		
	}
}

void CStorySceneSectionPlayingPlan::InitElements()
{
	const Uint32 currentLocaleId = SLocalizationManager::GetInstance().GetCurrentLocaleId();
	const String& currentLocaleStr = SLocalizationManager::GetInstance().GetCurrentLocale();

	// if variant is played in its base locale..
	if( currentLocaleId == m_section->GetVariantBaseLocale( m_sectionVariantId ) )
	{
		// assume local VO match approved VO - we'll check this later
		m_localVoMatchApprovedVo = true;

		Float elementStartTime = 0.0f;
		for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
		{
			IStorySceneElementInstanceData* data = m_sectionInstanceData.m_elemData[ i ];
			data->Init( currentLocaleStr, elementStartTime );

			const Float elementApprovedDuration = m_section->GetElementApprovedDuration( m_sectionVariantId, m_sectionInstanceData.m_elements[ i ]->GetElementID() );

			// check whether local VO matches approved VO
			if( Abs( data->GetDuration() - elementApprovedDuration ) > NumericLimits< Float >::Epsilon() )
			{
				m_localVoMatchApprovedVo = false;
			}

			if( m_player->UseApprovedVoDurations() )
			{
				if( elementApprovedDuration >= 0.0f )
				{
					data->SetDuration( elementApprovedDuration );
				}
				// else - element duration was never approved so use its current (local) duration
			}

			// calculate start time of next element
			elementStartTime += data->GetDuration();
		}
	}
	// if variant is played not in its base locale..
	else
	{
		const Float maxLeadingSilencePercentage = Config::cvMaxLeadingSilencePercentage.Get();
		const Float maxLeadingSilenceInSeconds = Config::cvMaxLeadingSilenceInSeconds.Get();
		const Float maxTrailingSilencePercentage = Config::cvMaxTrailingSilencePercentage.Get();
		const Float maxTrailingSilenceInSeconds = Config::cvMaxTrailingSilenceInSeconds.Get();

		m_localVoMatchApprovedVo = false;

		Float elementStartTime = 0.0f;
		for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
		{
			IStorySceneElementInstanceData* data = m_sectionInstanceData.m_elemData[ i ];
			data->Init( currentLocaleStr, elementStartTime );

			const Float elementApprovedDuration = m_section->GetElementApprovedDuration( m_sectionVariantId, m_sectionInstanceData.m_elements[ i ]->GetElementID() );

			// if duration of dialog line is smaller than approved duration then make it longer by adding some silence at both ends
			if( data->GetElement()->IsA< CStorySceneLine >() && data->GetDuration() < elementApprovedDuration )
			{
				StorySceneLineInstanceData* lineInst = static_cast< StorySceneLineInstanceData* >( data );

				Float silenceNeeded = elementApprovedDuration - lineInst->GetDuration();

				const Float trailingSilence = Min( silenceNeeded, Min( maxTrailingSilencePercentage * elementApprovedDuration, maxTrailingSilenceInSeconds ) );
				lineInst->SetTrailingSilence( trailingSilence );

				silenceNeeded -= trailingSilence;

				const Float leadingSilence = Min( silenceNeeded, Min( maxLeadingSilencePercentage * elementApprovedDuration, maxLeadingSilenceInSeconds ) );
				lineInst->SetLeadingSilence( leadingSilence );
			}

			// calculate start time of next element
			elementStartTime += data->GetDuration();
		}
	}

	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		IStorySceneElementInstanceData* data = m_sectionInstanceData.m_elemData[ i ];
		if( const CStorySceneLine* line = Cast< const CStorySceneLine >( data->GetElement() ) )
		{
			const Float lineStartTimeMarker = data->GetStartTime();
			const Float lineEndTimeMarker = lineStartTimeMarker + data->GetDuration();

			new ( m_sectionInstanceData.m_cachedLineMarkers ) LineMarker( lineStartTimeMarker, lineEndTimeMarker, line->GetVoiceTag() );
		}
		else if( const CStorySceneCutscenePlayer* csPlayer = Cast< const CStorySceneCutscenePlayer >( data->GetElement() ) )
		{
			// Create CameraMarkers for any camera cuts in this cutscene.
			if ( const CCutsceneTemplate* csTemplate = csPlayer->GetCutscene() )
			{
				const CAnimatedComponent* ac = nullptr;
				const CSkeletalAnimation* anim = nullptr;
				Int32 boneIndex;

				if ( csTemplate->GetCameraAnimation( anim, ac, boneIndex ) )
				{
					SBehaviorGraphOutput pose;
					pose.Init( anim->GetBonesNum(), anim->GetTracksNum() );

					TDynArray< Float > cuts = csTemplate->GetCameraCuts();
					for ( Float cut : cuts )
					{
						anim->Sample( cut, pose.m_numBones, pose.m_numFloatTracks, pose.m_outputPose, pose.m_floatTracks  );

						Matrix camMtx = pose.GetBoneModelMatrix( ac, boneIndex );
						Float camFov = pose.m_floatTracks[ SBehaviorGraphOutput::FTT_FOV ];

						new ( m_sectionInstanceData.m_cachedCameraMarkers ) CameraMarker( data->GetStartTime() + cut, camMtx, camFov );
					}
				}
			}
		}
	}
}

void CStorySceneSectionPlayingPlan::InitEvents()
{
	// Note: this is called after InitElements() so we can safely use GetStartTime()
	// and GetDuration() methods of IStorySceneElementInstanceData.

	// first pass - init all events and set their start times
	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		const IStorySceneElementInstanceData* elemInst = m_sectionInstanceData.m_elemData[ i ];

		TDynArray< const CStorySceneEvent* >& events = m_sectionInstanceData.m_eventData[ i ].m_second;
		for ( Uint32 j = 0; j < events.Size(); ++j )
		{
			const CStorySceneEvent* e = events[ j ];

			// note: only events from chosen section variant are handled here (that's good)
			ASSERT( m_section->GetEventVariant( e->GetGUID() ) == m_sectionVariantId );

			e->EventInit( *m_sectionInstanceData.m_data, m_player );

			const Float eventStartTime = elemInst->GetStartTime() + e->GetStartPosition() * elemInst->GetDuration();
			e->SetInstanceStartTime( *m_sectionInstanceData.m_data, eventStartTime );
		}
	}

	// second pass - set unscaled durations
	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		const IStorySceneElementInstanceData* elemInst = m_sectionInstanceData.m_elemData[ i ];

		TDynArray< const CStorySceneEvent* >& events = m_sectionInstanceData.m_eventData[ i ].m_second;
		for ( Uint32 j = 0; j < events.Size(); ++j )
		{
			const CStorySceneEvent* e = events[ j ];

			// note: only events from chosen section variant are handled here (that's good)
			ASSERT( m_section->GetEventVariant( e->GetGUID() ) == m_sectionVariantId );

			Float duration = 0.0f;

			if( const CStorySceneEventDuration* durationEvent = Cast< const CStorySceneEventDuration >( e ) )
			{
				duration = durationEvent->GetDurationProperty();
			}
			else if( const CStorySceneEventInterpolation* interpolationEvent = Cast< const CStorySceneEventInterpolation >( e ) )
			{
				const CStorySceneEvent* firstKey = m_section->GetEvent( interpolationEvent->GetKeyGuid( 0 ) );
				const CStorySceneEvent* lastKey = m_section->GetEvent( interpolationEvent->GetKeyGuid( interpolationEvent->GetNumKeys() - 1 ) );
				
				duration = lastKey->GetInstanceStartTime( *m_sectionInstanceData.m_data ) - firstKey->GetInstanceStartTime( *m_sectionInstanceData.m_data );
			}
			else if( const CStorySceneEventBlend* blendEvent = Cast< const CStorySceneEventBlend >( e ) )
			{
				const CStorySceneEvent* firstKey = m_section->GetEvent( blendEvent->GetKey( 0 ) );
				const CStorySceneEvent* lastKey = m_section->GetEvent( blendEvent->GetKey( blendEvent->GetNumberOfKeys() - 1 ) );

				duration = lastKey->GetInstanceStartTime( *m_sectionInstanceData.m_data ) - firstKey->GetInstanceStartTime( *m_sectionInstanceData.m_data );
			}
			// else point event and duration is 0.0f

			e->SetInstanceDuration( *m_sectionInstanceData.m_data, duration );
		}
	}

	// third pass - compute scaling factor, set scaled durations and mark instance data as valid
	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		TDynArray< const CStorySceneEvent* >& events = m_sectionInstanceData.m_eventData[ i ].m_second;
		for ( Uint32 j = 0; j < events.Size(); ++j )
		{
			const CStorySceneEvent* e = events[ j ];

			// note: only events from chosen section variant are handled here (that's good)
			ASSERT( m_section->GetEventVariant( e->GetGUID() ) == m_sectionVariantId );

			const Float scalingFactor = ComputeScalingFactor( *e );
			e->SetInstanceScalingFactor( *m_sectionInstanceData.m_data, scalingFactor );

			const Float scaledDuration = scalingFactor * e->GetInstanceDuration( *m_sectionInstanceData.m_data );
			e->SetInstanceDuration( *m_sectionInstanceData.m_data, scaledDuration );

			e->MarkInstanceDataSet( *m_sectionInstanceData.m_data );
		}
	}

	// fourth pass - refresh blend events
	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		for ( const CStorySceneEvent* e : m_sectionInstanceData.m_eventData[ i ].m_second )
		{
			// note: only events from chosen section variant are handled here (that's good)
			ASSERT( m_section->GetEventVariant( e->GetGUID() ) == m_sectionVariantId );

			if ( const CStorySceneEventCurveBlend* curveBlendEvent = Cast< const CStorySceneEventCurveBlend >( e ) )
			{
				CStorySceneEventCurveBlend* hackNonConstCurveBlendEvent = const_cast< CStorySceneEventCurveBlend* >( curveBlendEvent );
				hackNonConstCurveBlendEvent->UpdateCurveTotalTimeFromDuration( e->GetInstanceDuration( *m_sectionInstanceData.m_data ) );
			}
		}
	}


	// Collect camera events and add CameraMarkers for each. This way we'll be able to prefetch those frames.
	// This is probably doing more than just actual camera cuts, but that shouldn't be a problem, and in fact
	// may help with getting stuff ready that'll be coming in after a pan or something.
	for ( const auto& eventData : m_sectionInstanceData.m_eventData )
	{
		for ( const CStorySceneEvent* e : eventData.m_second )
		{
			// note: only events from chosen section variant are handled here (that's good)
			SCENE_ASSERT( m_section->GetEventVariant( e->GetGUID() ) == m_sectionVariantId );

			if ( const CStorySceneEventCamera* camEvt = Cast< const CStorySceneEventCamera >( e ) )
			{
				if ( const StorySceneCameraDefinition* camDefn = camEvt->GetCameraDefinition() )
				{
					Matrix camMtx;
					camDefn->m_cameraTransform.CalcLocalToWorld( camMtx );

					Float camFov = camDefn->m_cameraFov;
					Float evtTime = camEvt->GetInstanceStartTime( *m_sectionInstanceData.m_data );

					new ( m_sectionInstanceData.m_cachedCameraMarkers ) CameraMarker( evtTime, camMtx, camFov );
				}
			}
		}
	}
}

void CStorySceneSectionPlayingPlan::DeinitAllEventInstaces()
{
	SCENE_ASSERT( IsValid() );

	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		TDynArray< const CStorySceneEvent* >& events = m_sectionInstanceData.m_eventData[ i ].m_second;
		for ( Uint32 j=0; j<events.Size(); ++j )
		{
			const CStorySceneEvent* e = events[ j ];
			e->EventDeinit( *m_sectionInstanceData.m_data, m_player );
		}
	}

	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		IStorySceneElementInstanceData* data = m_sectionInstanceData.m_elemData[ i ];
		data->Deinit();
	}

	if ( m_sectionInstanceData.m_data )
	{
		m_section->GetScene()->ReleaseInstanceBuffer( m_sectionInstanceData.m_data, m_section, m_sectionVariantId );
	}
}

Bool CStorySceneSectionPlayingPlan::OnLoading_EnsureNextElementsHaveSpeeches() const
{
	SCENE_ASSERT( IsValid() );

	Bool elementsAreLoaded = true;
	TDynArray< Uint32 > nextElementStrings;

	Uint32 begin = (Uint32)Max< Int32 >(0, m_currElementIndex);
	
	for ( Uint32 i = begin, numElements = GetNumElements(); i < numElements; ++i )
	{
		const IStorySceneElementInstanceData* elementInstance = m_sectionInstanceData.m_elemData[i];
		elementInstance->GetElement()->GetLocalizedStringIds( nextElementStrings );
	}

	for ( TDynArray< Uint32 >::const_iterator stringIter = nextElementStrings.Begin();
		stringIter != nextElementStrings.End(); ++stringIter )
	{
		elementsAreLoaded &= SLocalizationManager::GetInstance().ValidateLanguagePackLoad( *stringIter );
	}

	return elementsAreLoaded;
}

Bool CStorySceneSectionPlayingPlan::HasElements() const
{
	SCENE_ASSERT( IsValid() );

	return GetNumElements() > 0;
}

Bool CStorySceneSectionPlayingPlan::HasNextElement() const
{
	SCENE_ASSERT( IsValid() );

	return static_cast< Uint32 >( m_currElementIndex ) + 1 < GetNumElements();
}

Bool CStorySceneSectionPlayingPlan::HasNextSection() const
{
	SCENE_ASSERT( IsValid() );

	SCENE_ASSERT( m_player->m_internalState.m_currentSection == m_section );

	return m_player->GetSectionNextElement( m_section ) != NULL;
}

IStorySceneElementInstanceData* CStorySceneSectionPlayingPlan::GetCurrElement()
{
	SCENE_ASSERT( IsValid() );

	return m_currElementIndex >= 0 && static_cast< Uint32 >( m_currElementIndex ) < GetNumElements() ? m_sectionInstanceData.m_elemData[ m_currElementIndex ] : NULL;
}

const IStorySceneElementInstanceData* CStorySceneSectionPlayingPlan::GetElement( Uint32 index ) const
{
	SCENE_ASSERT( IsValid() );

	return m_sectionInstanceData.m_elemData[ index ];
}

const IStorySceneElementInstanceData* CStorySceneSectionPlayingPlan::GetCurrElement() const
{
	SCENE_ASSERT( IsValid() );

	return m_currElementIndex >= 0 && static_cast< Uint32 >( m_currElementIndex ) < GetNumElements() ? m_sectionInstanceData.m_elemData[ m_currElementIndex ] : NULL;
}

IStorySceneElementInstanceData* CStorySceneSectionPlayingPlan::GoToNextElement() 
{
	SCENE_ASSERT( IsValid() );

	m_currElementIndex++;
	SCENE_ASSERT( static_cast< Uint32 >( m_currElementIndex ) <= GetNumElements() );
	return GetCurrElement();
}

IStorySceneElementInstanceData* CStorySceneSectionPlayingPlan::GoToFirstElement()
{
	SCENE_ASSERT( IsValid() );

	m_currElementIndex = 0;
	return GetCurrElement();
}

IStorySceneElementInstanceData* CStorySceneSectionPlayingPlan::GoToPrevElement()
{
	SCENE_ASSERT( IsValid() );

	SCENE_ASSERT( m_currElementIndex > 0 );
	m_currElementIndex--;
	return GetCurrElement();
}

// TODO: Rename to GetCurrentTime()
Float CStorySceneSectionPlayingPlan::GetCurrentSectionTime() const
{
	SCENE_ASSERT( IsValid() );

	if ( m_currElementIndex < 0 || static_cast< Uint32 >( m_currElementIndex ) >= GetNumElements() )
	{
		return 0.f;
	}

	const Float timeStart = m_sectionInstanceData.m_elemData[ m_currElementIndex ]->GetStartTime();
	const Float elemTime = m_sectionInstanceData.m_elemData[ m_currElementIndex ]->GetCurrentTime();
	return timeStart + elemTime;
}

/*
This has convoluted functionality - it checks whether timeToTest is in range of current element
and if it is then it returns offset from element start time. Rework this to two functions.
*/
Bool CStorySceneSectionPlayingPlan::CalcCurrElemTimeOffset( Float timeToTest, Float& timeOffset ) const
{
	SCENE_ASSERT( IsValid() );

	if ( m_currElementIndex < 0 || static_cast< Uint32 >( m_currElementIndex ) >= GetNumElements() )
	{
		timeOffset = 0.f;
		return false;
	}

	const Float timeStart = m_sectionInstanceData.m_elemData[ m_currElementIndex ]->GetStartTime();
	const Float timeEnd = timeStart + m_sectionInstanceData.m_elemData[ m_currElementIndex ]->GetDuration();

	const Bool inRange = timeToTest >= timeStart && timeToTest < timeEnd;
	timeOffset = timeToTest - timeStart;

	if ( inRange )
	{
		SCENE_ASSERT( timeOffset >= 0.f );
		SCENE_ASSERT( timeOffset < m_sectionInstanceData.m_elemData[ m_currElementIndex ]->GetDuration() );
	}

	return inRange;
}

void CStorySceneSectionPlayingPlan::Cleanup()
{
	SCENE_ASSERT( IsValid() );
	SCENE_ASSERT( m_player );

	StopAllInstances();
	DeinitAllEventInstaces();

	if ( m_sectionInstanceData.m_data )
	{
		m_section->GetScene()->DestroyInstanceBuffer( m_sectionInstanceData.m_data );

		m_sectionInstanceData.m_data = nullptr;
	}

	for ( Uint32 i = 0, numElements = GetNumElements(); i < numElements; ++i )
	{
		SCENE_DELETE( m_sectionInstanceData.m_elemData[ i ] );
	}

	m_sectionInstanceData.m_elemData.Clear();
	m_sectionInstanceData.m_eventData.Clear();

	m_section = NULL;
	m_player = NULL;
	m_currElementIndex = -1;
	m_valid = false;
}

Bool CStorySceneSectionPlayingPlan::OnLoading_AreAllElementsReady() const
{
	SCENE_ASSERT( IsValid() );

	Bool allElementsReady = true;
	for ( TDynArray< IStorySceneElementInstanceData* >::const_iterator elementIter = m_sectionInstanceData.m_elemData.Begin();
		elementIter != m_sectionInstanceData.m_elemData.End(); ++elementIter )
	{
		IStorySceneElementInstanceData* data = *elementIter;
		SCENE_ASSERT( data );

		if ( !data->IsReady() )
		{
			allElementsReady = false;
			break;
		}
	}
	return allElementsReady;
}

/*

CStorySceneSectionPlayingPlan::Create() must have already been called before calling this function.
*/
Bool CStorySceneSectionPlayingPlan::OnLoading_ProcessUsedEntities( CStoryScenePlayer* actorProvider, Bool asyncLoading ) const
{
	if ( m_section->IsGameplay() )
	{
		return true;
	}

	const Bool isScenePrecached = actorProvider->IsScenePrecached();

	TDynArray< CEntity* > sectionEntities;
	TDynArray< CEntity* > entitiesWhichNeedAnimationPrestream;	

	for ( auto elementIter : m_sectionInstanceData.m_elemData )
	{
		elementIter->GetUsedEntities( sectionEntities );
	}

	for( auto iter : actorProvider->GetSceneActors() )
	{
		if ( CEntity* e = iter.m_second.Get() )
		{
			sectionEntities.PushBackUnique( e );
		}
	}

	for ( CEntity* entityIter : sectionEntities )
	{
		if ( OnLoading_CanProcessUsedEntity( entityIter, asyncLoading ) == false )
		{
			return false;
		}
	}

	for ( CEntity* entityIter : sectionEntities )
	{
		OnLoading_ProcessUsedEntity( entityIter, isScenePrecached );
	}

	return true;
}

void CStorySceneSectionPlayingPlan::OnLoading_ProcessUsedEntity( CEntity* entity, Bool isScenePrecached ) const
{
	SCENE_ASSERT( entity );

	for ( ComponentIterator< CMeshTypeComponent > it( entity ); it; ++it )
	{
		CMeshTypeComponent* meshComponent = *it;
		if ( meshComponent && meshComponent->GetMeshTypeResource() )
		{
			meshComponent->GetMeshTypeResource()->GetRenderResource();
		}
	}

	if ( entity->IsA< CActor >() )
	{
		SCENE_ASSERT( !m_section->IsGameplay() );

		// Actor must have mimic high
		CActor* actor = static_cast< CActor* >( entity );
		
		if ( actor->MimicOn() == false )
		{
			CEntityTemplate* templ = actor->GetEntityTemplate();
			LOG_GAME( TXT("Actor '%ls' will not have mimic high - resource '%ls'"), actor->GetFriendlyName().AsChar(), templ->GetFriendlyName().AsChar() );
		}
	}
}

Bool CStorySceneSectionPlayingPlan::OnLoading_CanProcessUsedEntity( CEntity* entity, Bool asyncLoading ) const
{
	SCENE_ASSERT( entity );

	if ( CGameplayEntity* gameplayEntity = Cast< CGameplayEntity >( entity ) )
	{
		if ( CInventoryComponent* inventoryComponent = gameplayEntity->GetInventoryComponent() )
		{
			if ( asyncLoading && inventoryComponent->AreAllMountedItemsSpawned() == false )
			{
				// Not all items are spawned so we cannot process the entity
				return false;
			}
		}
	}
	return true;
}

/*
Returns number of scene elements in playing plan.

\return Number of scene elements in playing plan.
*/
Uint32 CStorySceneSectionPlayingPlan::GetNumElements() const
{
	return m_sectionInstanceData.m_elements.Size();
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
