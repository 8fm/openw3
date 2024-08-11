
#include "build.h"
#include "dialogEditorFlowCtrl.h"

#include "..\..\common\game\storySceneControlPart.h"
#include "..\..\common\game\storySceneSection.h"
#include "..\..\common\game\storySceneControlPartsUtil.h"
#include "..\..\common\game\storyScenePlayer.h"
#include "..\..\common\game\storySceneSectionPlayingPlan.h"

#include "..\..\common\game\storySceneEventsCollector_events.h"
#include "..\..\common\game\storySceneEventLightProperties.h"

#include "dialogEditor.h"
#include "storyScenePreviewPlayer.h"
#include "..\..\common\game\storySceneEventAttachPropToBone.h"
#include "..\..\common\game\storySceneEventPoseKey.h"
#include "..\..\common\game\storySceneInput.h"
#include "..\..\common\game\storySceneEventScenePropPlacement.h"
#include "..\..\common\game\storySceneEventDespawn.h"
#include "..\..\common\game\storySceneEventWorldPropPlacement.h"
#include "..\..\common\game\storySceneEventHideScabbard.h"
#include "..\..\common\game\storySceneEventCloth.h"
#include "..\..\common\game\storySceneEventDangle.h"
#include "..\..\common\game\storySceneEventOpenDoor.h"
#include "..\..\common\game\storySceneEventModifyEnv.h"
#include "..\..\common\game\storySceneEventEquipItem.h"
#include "..\..\common\game\storySceneEventCameraLight.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

CEdSceneFlowCtrl::CEdSceneFlowCtrl()
	: m_mediator( nullptr )
{

}

void CEdSceneFlowCtrl::Init( CEdSceneEditor* mediator )
{
	m_mediator = mediator;
}

Bool CEdSceneFlowCtrl::IsBlockActivated( const CStorySceneControlPart* cp ) const
{
	return cp == m_selectedSection || m_activePartsSet.Exist( cp );
}

Float CEdSceneFlowCtrl::GetBlockActivationAlpha( const CStorySceneControlPart* cp ) const
{
	if ( cp == m_selectedSection )
	{
		return 1.f;
	}

	if ( m_activePartsSet.Exist( cp ) )
	{
		return 0.5f;
	}

	return 0.f;
}

Bool CEdSceneFlowCtrl::HasFlowFor( const CStorySceneSection* section ) const
{
	return section == m_selectedSection;
}

void CEdSceneFlowCtrl::GetFlow( const CStorySceneSection* s, TDynArray< const CStorySceneLinkElement* >& flow, Bool withInputs )
{
	FindAndCollectSectionFlow( s, flow, withInputs );
}

CName CEdSceneFlowCtrl::GetDialogsetNameFromFlowFor( const CStorySceneSection* s ) const
{
	if ( s == m_selectedSection )
	{
		SCENE_ASSERT( m_currentDialogset );
		return m_currentDialogset;
	}

	SCENE_ASSERT( m_activePartsFlow.Size() == m_activePartsFlowDialogsets.Size() );

	const Uint32 numParts = m_activePartsFlow.Size();
	for ( Uint32 i=0; i<numParts; ++i )
	{
		if ( m_activePartsFlow[ i ] == s )
		{
			return m_activePartsFlowDialogsets[ i ];
		}
	}

	SCENE_ASSERT( 0 );
	return CName::NONE;
}

const CStorySceneInput* CEdSceneFlowCtrl::GetInputFor( const CStorySceneSection* s ) const
{
	if ( s )
	{
		TDynArray< const CStorySceneLinkElement* > flow;
		FindAndCollectSectionFlow( s, flow, true );

		const Int32 size = flow.SizeInt();
		if ( size > 0 && flow[ size - 1 ]->IsA< CStorySceneInput >() )
		{
			return SafeCast< const CStorySceneInput >( flow[ size - 1 ] );
		}
	}

	return nullptr;
}

const CStorySceneEventsCollector& CEdSceneFlowCtrl::GetEventsFlowCollector() const
{
	return m_flowCollector;
}

void CEdSceneFlowCtrl::RecalcFlow( CStoryScenePreviewPlayer* player )
{
	SetSection( m_selectedSection, player );
}

void CEdSceneFlowCtrl::RecalcOnlyEvents( CStoryScenePreviewPlayer* player )
{
	CollectImportantEventsForFlow( player );
}

void CEdSceneFlowCtrl::SetSection( const CStorySceneSection* section, CStoryScenePreviewPlayer* player )
{
	m_activePartsFlow.ClearFast();
	m_activePartsFlowDialogsets.ClearFast();
	m_activePartsSet.ClearFast();
	m_currentDialogset = CName::NONE;

	m_selectedSection = section;

	FindAndCollectSectionFlow( m_selectedSection, m_activePartsFlow, true );

	m_activePartsFlowDialogsets.Reserve( m_activePartsFlow.Size() );

	CName dialogSet;
	for ( Int32 i=m_activePartsFlow.SizeInt()-1; i>=0; --i )
	{
		const CStorySceneLinkElement* elem = m_activePartsFlow[ i ];

		if ( const CStorySceneInput* input = Cast< const CStorySceneInput >( elem ) )
		{
			dialogSet = input->GetDialogsetName();
		}
		else if ( const CStorySceneSection* section = Cast< const CStorySceneSection >( elem ) )
		{
			const CName changeTo = section->GetDialogsetChange();
			if ( changeTo )
			{
				dialogSet = changeTo;
			}
		}

		m_activePartsFlowDialogsets.PushBack( dialogSet );
		m_activePartsSet.Insert( elem );
	}

	if ( m_selectedSection )
	{
		const CName changeTo = m_selectedSection->GetDialogsetChange();
		if ( changeTo )
		{
			dialogSet = changeTo;
		}
	}

	m_currentDialogset = dialogSet;

	CollectImportantEventsForFlow( player );
}

namespace
{
	void FindInputFromTo( const CStorySceneSection* from, const CStorySceneSection* to, TDynArray< const CStorySceneLinkElement* >& out )
	{
		SCENE_ASSERT( from );
		SCENE_ASSERT( to );
		if ( from && to )
		{
			const Int32 size = from->GetInputPathLinks().SizeInt();
			for ( Int32 i=0; i<size; ++i )
			{
				if ( const CStorySceneLinkElement* link = from->GetInputPathLinkElement( i ) )
				{
					const CStorySceneLinkElement* next = link->GetNextElement();
					if ( next && ( next == to || next->FindParent< CStorySceneSection >() == to ) )
					{
						TDynArray< CStorySceneLinkElement* > temp = link->GetLinkedElements();
						for ( CStorySceneLinkElement* item : temp )
						{
							if( ! Cast< CStorySceneControlPart > ( item ) )
							{
								item = Cast< CStorySceneLinkElement >( item->GetParent() );
							}
							if ( item )
							{
								out.PushBack( item );
							}
						}

						return;
					}
				}
			}		
		}
	}
}

Bool CEdSceneFlowCtrl::FindAndCollectSectionFlow( TDynArray< const CStorySceneLinkElement* >& outFlow, TDynArray< const CStorySceneLinkElement* >& visited, Bool withInputs ) const
{
	TDynArray< const CStorySceneLinkElement* > linkedElements;
	const CStorySceneLinkElement* link = outFlow.Back();
	const CStorySceneLinkElement* lastEl = outFlow.Size() > 1 ? outFlow[ outFlow.Size() - 2 ] : nullptr;
	FindSectionFlow( link, lastEl, linkedElements );
	for ( Int32 i = 0; i < linkedElements.SizeInt(); ++i )
	{
		if ( linkedElements[i] && !visited.Exist( linkedElements[i] ) )
		{
			visited.PushBack( linkedElements[i] );
			if ( Cast<const CStorySceneInput>( linkedElements[i] ) )
			{
				if ( withInputs )
				{
					outFlow.PushBack( linkedElements[i] );
				}
				return true;
			}
			outFlow.PushBack( linkedElements[i] );
			if( FindAndCollectSectionFlow( outFlow, visited, withInputs ) )
			{
				return true;
			}
			else
			{
				outFlow.PopBackFast();
			}
		}
	}
	return false;
}

void CEdSceneFlowCtrl::FindAndCollectSectionFlow( const CStorySceneSection* section, TDynArray< const CStorySceneLinkElement* >& flow, Bool withInputs ) const
{
	const CStorySceneLinkElement* currElement = section;
	const CStorySceneLinkElement* nextElement = nullptr;

	TDynArray< const CStorySceneLinkElement* > visited;
	
	visited.PushBack( section );
	flow.PushBack( section );

	FindAndCollectSectionFlow( flow, visited, withInputs );

	flow.RemoveAt( 0 );

	{
		Bool hasInput = false;
		for ( const CStorySceneLinkElement* f : flow )
		{
			if ( f->GetClass()->IsA< CStorySceneInput >() )
			{
				if ( !withInputs )
				{
					SCENE_ASSERT( !withInputs );
				}

				SCENE_ASSERT( !hasInput );
				hasInput = true;
			}
		}
	}
}

void CEdSceneFlowCtrl::FindSectionFlow( const CStorySceneLinkElement* currElement, const CStorySceneLinkElement* nextElement, TDynArray< const CStorySceneLinkElement* >& out ) const
{
	SCENE_ASSERT( currElement != nextElement );	
	
	Int32 selectedInput = -1;
	if( currElement->SupportsInputSelection() )
	{
		selectedInput = (Int32)currElement->GetSelectedInputLinkElement();
		StorySceneControlPartUtils::GetPrevConnectedElements( currElement, out, selectedInput );
		return;
	}

	const CStorySceneSection* currSection = Cast< const CStorySceneSection >( currElement );
	const CStorySceneSection* nextSection = Cast< const CStorySceneSection >( nextElement );
	if ( currSection && nextSection && m_selectedSection != currSection && currSection->GetInputPathLinks().Size() > 0 )
	{
		FindInputFromTo( currSection, nextSection, out );	
		return;
	}

	StorySceneControlPartUtils::GetPrevConnectedElements( currElement, out );
}

Bool CEdSceneFlowCtrl::CFlowEventFilter::DoFilterOut( const CStorySceneEvent* e ) const
{
	if ( e->GetClass()->IsA< CStorySceneEventLightProperties >() || e->GetClass()->IsA< CStorySceneEventPoseKey >() ||
		 e->GetClass()->IsA< CStorySceneEventAttachPropToSlot >() || e->GetClass()->IsA< CStorySceneEventPropVisibility >() ||
		 e->GetClass()->IsA< CStorySceneEventScenePropPlacement >() || e->GetClass()->IsA< CStorySceneEventWorldPropPlacement >() ||
		 e->GetClass()->IsA< CStorySceneEventHideScabbard >() || e->GetClass()->IsA< CStorySceneEventOpenDoor >() ||
		 e->GetClass()->IsA< CStorySceneDisablePhysicsClothEvent >() || e->GetClass()->IsA< CStorySceneDisableDangleEvent >() ||
		 e->GetClass()->IsA< CStorySceneEventModifyEnv >() || e->GetClass()->IsA< CStorySceneEventEquipItem >() ||
		 e->GetClass()->IsA< CStorySceneEventCameraLight >() )
	{
		return false;
	}

	return true;
}

void CEdSceneFlowCtrl::CollectImportantEventsForFlow( CStoryScenePreviewPlayer* player )
{
	SCENE_ASSERT( player );

	m_flowCollector = CStorySceneEventsCollector();

	AddResetEvents( m_flowCollector, player );

	for ( Int32 i=m_activePartsFlow.SizeInt()-1; i>=0; --i )
	{
		if ( const CStorySceneSection* s = Cast< const CStorySceneSection >( m_activePartsFlow[ i ] ) )
		{
			CStorySceneSectionPlayingPlan* plan = player->HACK_GetOrCreateSectionPlayingPlan( s, m_activePartsFlow );
			SCENE_ASSERT( plan );
			if ( plan )
			{
				player->HACK_CollectAllEvents( plan, m_flowCollector, &m_flowEventsFilter );
			}
		}
	}
}

void CEdSceneFlowCtrl::AddResetEvents( CStorySceneEventsCollector& collector, CStoryScenePreviewPlayer* player )
{
	SCENE_ASSERT( player );

	if ( m_selectedSection )
	{
		// Force reset lights
		if ( CStoryScene* scene = m_selectedSection->GetScene() )
		{
			const TDynArray< CStorySceneLight* >& lightDefinitions = scene->GetSceneLightDefinitions();
			for ( const CStorySceneLight* light : lightDefinitions )
			{
				if ( light && light->m_id )
				{
					StorySceneEventsCollector::LightProperty lightEvt( nullptr, light->m_id );
					lightEvt.m_reset = true;
					collector.AddEvent( lightEvt );
				}
			}

			// Force reset props
			const TDynArray< CStorySceneProp* >& propDef = scene->GetScenePropDefinitions();
			for ( const CStorySceneProp* prop :  propDef )
			{
				if ( prop && prop->m_id )
				{
					// 1. Visibility
					{
						StorySceneEventsCollector::PropVisibility evt( nullptr, prop->m_id );
						evt.m_showHide = false;
						collector.AddEvent( evt );
					}

					// 2. Placement
					{
						StorySceneEventsCollector::PropPlacement evt( nullptr, prop->m_id );
						Matrix sceneL2W;
						player->GetSceneDirector()->GetCurrentScenePlacement().CalcLocalToWorld( sceneL2W );
						evt.m_placementWS = EngineTransform( sceneL2W );
						collector.AddEvent( evt );
					}

					// 3. Attachments
					{
						collector.AddEvent( StorySceneEventsCollector::AttachPropToBone( nullptr, prop->m_id ) );
					}

				}
			}

			// Force reset actors
			const TDynArray< CStorySceneActor* >& actorDefs = scene->GetSceneActorsDefinitions();
			for ( const CStorySceneActor* actorDef :  actorDefs )
			{
				if( actorDef && actorDef->m_id )
				{
					StorySceneEventsCollector::BodyPose bodyEvt( nullptr, actorDef->m_id );
					bodyEvt.m_reset = true;
					collector.AddEvent( bodyEvt );

					StorySceneEventsCollector::MimicPose mimicEvt( nullptr, actorDef->m_id );
					mimicEvt.m_reset = true;
					collector.AddEvent( mimicEvt );

					StorySceneEventsCollector::ActorDisablePhysicsCloth evtC( nullptr, actorDef->m_id );
					collector.AddEvent( evtC );

					StorySceneEventsCollector::ActorDisableDangle evtD( nullptr, actorDef->m_id );
					collector.AddEvent( evtD );

					StorySceneEventsCollector::ActorItem evt( nullptr, actorDef->m_id );
					collector.AddEvent( evt );	

					StorySceneEventsCollector::ActorItemVisibility evtIt( nullptr, actorDef->m_id );
					evtIt.m_reset = true;
					collector.AddEvent( evtIt );
				}				
			}

			{
				StorySceneEventsCollector::CameraLightProp evt;
				const ECameraLightModType modType = ECLT_Scene;
				evt.cameraSetup.SetModifiersAllIdentityOneEnabled( modType );
				evt.cameraSetup.m_scenesSystemActiveFactor = 1.f;
				collector.AddEvent( evt );
			}

			{
				StorySceneEventsCollector::DoorChangeState evt;
				evt.m_resetAll = true;
				collector.AddEvent( evt );
			}

			{
				StorySceneEventsCollector::EnvChange evt;
				evt.m_activate = false;
				collector.AddEvent( evt );
			}
		}
	}
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
