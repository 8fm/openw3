
#pragma once

#include "storySceneEventsCollector_events.h"
#include "storySceneIncludes.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

class CStorySceneEventsCollectorFilter
{
public:
	virtual Bool DoFilterOut( const CStorySceneEvent* e ) const = 0;
};

class CStorySceneEventsCollector
{
	friend class CStorySceneEventsExecutor;

	template< typename T > 
	struct SingleEvent
	{
		T		m_event;
		Bool	m_isSet;

		void Reset() 
		{ 
			m_isSet = false; 
			m_event = T(); 
		}

		void Set( const T& e )
		{
			if ( m_isSet )
			{
				SCENE_ASSERT( e.m_eventTimeAbs >= m_event.m_eventTimeAbs );
				m_event = e;
			}
			else
			{
				m_isSet = true;
				m_event = e;
			}
		}
	};

private:
	Bool																m_empty;

	TDynArray< StorySceneEventsCollector::BodyAnimation >				m_bodyAnimations;
	TDynArray< StorySceneEventsCollector::MimicsAnimation >				m_mimicsAnimations;
	TDynArray< StorySceneEventsCollector::MimicPose >					m_mimicsPoses;
	TDynArray< StorySceneEventsCollector::BodyPose >					m_bodyPoses;
	TDynArray< StorySceneEventsCollector::ActorChangeState >			m_actorChangeState;

	TDynArray< StorySceneEventsCollector::ActorVisibility >				m_actorVisibility;
	TDynArray< StorySceneEventsCollector::ActorApplyAppearance >		m_actorAppearances;
	TDynArray< StorySceneEventsCollector::ActorUseHiresShadows >		m_actorHiresShadows;
	TDynArray< StorySceneEventsCollector::ActorMimicLod >				m_actorMimicLod;
	TDynArray< StorySceneEventsCollector::ActorLodOverride >			m_actorLodOverrides;
	TDynArray< StorySceneEventsCollector::ActorItem >					m_actorItems;
	TDynArray< StorySceneEventsCollector::ActorMorph >					m_actorMorph;
	TDynArray< StorySceneEventsCollector::ActorDisablePhysicsCloth >	m_actorDisablePhysicsCloth;
	TDynArray< StorySceneEventsCollector::ActorDisableDangle >			m_actorDisableDangle;
	TDynArray< StorySceneEventsCollector::ActorDanglesShake >			m_actorDanglesShakes;
	TDynArray< StorySceneEventsCollector::ActorResetClothAndDangles >	m_actorResetClothAndDangles;
	TDynArray< StorySceneEventsCollector::ActorChangeGameState >		m_actorChangeGameState;
	TDynArray< StorySceneEventsCollector::ActorPrepareToTalk >			m_actorPrepareToTalk;
	TDynArray< StorySceneEventsCollector::ActorFinishTalk >				m_actorFinishTalk;
	TDynArray< StorySceneEventsCollector::ActorLookAt >					m_actorLookAts;
	TDynArray< StorySceneEventsCollector::ActorLookAtTick >				m_actorLookAtTicks;
	TDynArray< StorySceneEventsCollector::ActorGameplayLookAt >			m_actorLookAtGameplay;
	TDynArray< StorySceneEventsCollector::ActorPlacement >				m_actorPlacement;
	TDynArray< StorySceneEventsCollector::ActorMotion >					m_actorMotion;

	TDynArray< StorySceneEventsCollector::PropPlacement >				m_propPlacements;
	TDynArray< StorySceneEventsCollector::PropVisibility >				m_propVisibility;
	TDynArray< StorySceneEventsCollector::AttachPropToBone >			m_attachPropToBoneEvents;
	TDynArray< StorySceneEventsCollector::TimeMultiplier >				m_timeMultiplier;
	TDynArray< StorySceneEventsCollector::VideoOverlay	> 				m_videoOverlays;

	TDynArray< StorySceneEventsCollector::LightProperty >				m_lightProperties;
	TDynArray< StorySceneEventsCollector::CameraLightProp >				m_cameralightProperties;

	TDynArray< StorySceneEventsCollector::CameraPrefetch >				m_cameraPrefetches;

	SingleEvent< StorySceneEventsCollector::CameraShot >					m_cameraShot;
	SingleEvent< StorySceneEventsCollector::CameraBlend >					m_cameraBlend;
	SingleEvent< StorySceneEventsCollector::CameraStartBlendToGameplay >	m_cameraStartBlendToGameplay;
	TDynArray< StorySceneEventsCollector::CameraAnimation >					m_cameraAnimations;
	SingleEvent< StorySceneEventsCollector::Fade >							m_fade;

	TDynArray< StorySceneEventsCollector::PlayDialogLine >				m_playDialogLineEvents;

	TDynArray< StorySceneEventsCollector::PlayEffect >					m_playEffectEvents;
	TDynArray< StorySceneEventsCollector::ActorItemVisibility >			m_actorItemVisibility;
	TDynArray< StorySceneEventsCollector::DoorChangeState >				m_doorChangeStates;
	TDynArray< StorySceneEventsCollector::EnvChange >					m_envChanges;

	TDynArray< StorySceneEventsCollector::DisplayDebugComment >			m_displayDebugCommentEvents;
	TDynArray< StorySceneEventsCollector::HideDebugComment >			m_hideDebugCommentEvents;

	SingleEvent< StorySceneEventsCollector::SyncItemInfo>				m_syncItemInfo;

public:
	CStorySceneEventsCollector();

	void Clear();

	Bool Empty() const;

public:
	void AddEvent( const StorySceneEventsCollector::BodyAnimation& event );
	void RemoveEvent( const StorySceneEventsCollector::BodyAnimation& event );
	void AddEvent( const StorySceneEventsCollector::MimicsAnimation& event );
	void RemoveEvent( const StorySceneEventsCollector::MimicsAnimation& event );
	void AddEvent( const StorySceneEventsCollector::CameraAnimation& event );
	void RemoveEvent( const StorySceneEventsCollector::CameraAnimation& event );

	void AddEvent( const StorySceneEventsCollector::MimicPose& event );
	void AddEvent( const StorySceneEventsCollector::BodyPose& event );
	void RemoveEvent( const StorySceneEventsCollector::MimicPose& event );
	void RemoveEvent( const StorySceneEventsCollector::BodyPose& event );

	void AddEvent( const StorySceneEventsCollector::ActorPlacement& event );
	void AddEvent( const StorySceneEventsCollector::ActorMotion& event );

	void AddEvent( const StorySceneEventsCollector::ActorChangeState& event );
	void AddEvent( const StorySceneEventsCollector::PlayDialogLine& event );

	void AddEvent( const StorySceneEventsCollector::ActorVisibility& event );
	void AddEvent( const StorySceneEventsCollector::ActorApplyAppearance& event );
	void AddEvent( const StorySceneEventsCollector::ActorUseHiresShadows& event );
	void AddEvent( const StorySceneEventsCollector::ActorMimicLod& event );
	void AddEvent( const StorySceneEventsCollector::ActorLodOverride& event );
	void AddEvent( const StorySceneEventsCollector::ActorItem& event );
	void AddEvent( const StorySceneEventsCollector::ActorItemVisibility& event );
	void AddEvent( const StorySceneEventsCollector::ActorMorph& event );
	void AddEvent( const StorySceneEventsCollector::ActorDisablePhysicsCloth& event );
	void AddEvent( const StorySceneEventsCollector::ActorDisableDangle& event );
	void AddEvent( const StorySceneEventsCollector::ActorDanglesShake& event );
	void AddEvent( const StorySceneEventsCollector::ActorResetClothAndDangles& event );
	void AddEvent( const StorySceneEventsCollector::ActorLookAt& event );
	void AddEvent( const StorySceneEventsCollector::ActorLookAtTick& event );
	void AddEvent( const StorySceneEventsCollector::ActorGameplayLookAt& event );
	void AddEvent( const StorySceneEventsCollector::ActorChangeGameState& event );
	void AddEvent( const StorySceneEventsCollector::ActorPrepareToTalk& event );
	void AddEvent( const StorySceneEventsCollector::ActorFinishTalk& event );

	void AddEvent( const StorySceneEventsCollector::PropPlacement& event );
	void AddEvent( const StorySceneEventsCollector::PropVisibility& event );	
	void AddEvent( const StorySceneEventsCollector::LightProperty& event );
	void AddEvent( const StorySceneEventsCollector::CameraLightProp& event );

	void AddEvent( const StorySceneEventsCollector::CameraPrefetch& event );

	void AddEvent( const StorySceneEventsCollector::CameraShot& event );
	void AddEvent( const StorySceneEventsCollector::CameraBlend& event );
	void AddEvent( const StorySceneEventsCollector::CameraStartBlendToGameplay& event );

	void AddEvent( const StorySceneEventsCollector::AttachPropToBone& event );
	void AddEvent( const StorySceneEventsCollector::PlayEffect& event );
	void AddEvent( const StorySceneEventsCollector::Fade& event );

	void AddEvent( const StorySceneEventsCollector::DisplayDebugComment& event );
	void AddEvent( const StorySceneEventsCollector::HideDebugComment& event );
	void AddEvent( const StorySceneEventsCollector::TimeMultiplier& event );
	void AddEvent( const StorySceneEventsCollector::VideoOverlay& event );
	void AddEvent( const StorySceneEventsCollector::DoorChangeState& event );
	void AddEvent( const StorySceneEventsCollector::EnvChange& event );
	void AddEvent( const StorySceneEventsCollector::SyncItemInfo& event );

private:
	template< typename T >
	void InsertEventByTime( TDynArray< T >& arr, const T& event )
	{
		const Uint32 size = arr.Size();
		Uint32 place = 0;
		for ( ; place<size; ++place )
		{
			const T& evtIt = arr[ place ];

			//flow controller collects events from different sections. they shouldnt be mixed together
			Bool sameSection = true;
#ifndef NO_EDITOR
			if( evtIt.GetId() && event.GetId() )
			{
				const CStorySceneElement* ele1 = evtIt.GetId()->GetSceneElement();
				const CStorySceneElement* ele2 = event.GetId()->GetSceneElement();
				if( ele1 && ele2 )
				{
					sameSection = ele1->GetSection() == ele2->GetSection();
				}				
			}
#endif
			if ( event.m_eventTimeAbs < evtIt.m_eventTimeAbs && sameSection )
			{
				break;
			}
		}

		arr.Insert( place, event );

		#ifdef DEBUG_SCENES
		for ( Uint32 i=1; i<size+1; ++i )
		{
			const T& evtItA = arr[ i-1 ];
			const T& evtItB = arr[ i ];

			SCENE_ASSERT( evtItA.m_eventTimeAbs <= evtItB.m_eventTimeAbs );
		}
		#endif
	}

	template< typename T >
	void AddOnlyOneByActor( TDynArray< T >& arr, const T& event )
	{
		m_empty = false;

		const Uint32 size = arr.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const T& evtIt = arr[ i ];

			if ( evtIt.m_actorId == event.m_actorId )
			{
				SCENE_ASSERT( event.m_eventTimeAbs >= evtIt.m_eventTimeAbs );

				arr.RemoveAtFast( i );

				break;
			}
		}

		InsertEventByTime( arr, event );
	}

	template< typename T >
	void MergeOnlyOneByActor( TDynArray< T >& arr, const T& event )
	{
		m_empty = false;

		const Uint32 size = arr.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const T& evtIt = arr[ i ];

			if ( evtIt.m_actorId == event.m_actorId )
			{
				SCENE_ASSERT__FIXME_LATER( event.m_eventTimeAbs >= evtIt.m_eventTimeAbs );

				arr[ i ].Merge( evtIt );

				return;
			}
		}

		InsertEventByTime( arr, event );
	}

	template< typename T >
	void AddOnlyOneById( TDynArray< T >& arr, const T& event )
	{
		m_empty = false;

		if ( event.GetId() != nullptr )
		{
			const Uint32 size = arr.Size();
			for ( Uint32 i=0; i<size; ++i )
			{
				const T& evtIt = arr[ i ];

				if ( evtIt.GetId() != nullptr && evtIt.GetId() == event.GetId() )
				{
					SCENE_ASSERT( event.m_eventTimeAbs >= evtIt.m_eventTimeAbs );

					arr.RemoveAtFast( i );

					break;
				}
			}
		}

		InsertEventByTime( arr, event );
	}

	template< typename T >
	Bool RemoveById( TDynArray< T >& arr, const T& event )
	{
		const Uint32 size = arr.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const T& evtIt = arr[ i ];
			if ( evtIt.GetId() != nullptr && evtIt.GetId() == event.GetId() )
			{
				arr.RemoveAtFast( i );
				
				return true;
			}
		}

		return false;
	}
};

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif
