/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogEditorKeyframeCtrl.h"

#include "dialogEditor.h"
#include "storyScenePreviewPlayer.h"
#include "popupNotification.h"
#include "dialogEditorHelperEntitiesCtrl.h"

#include "../../common/engine/renderCommands.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneEvent.h"
#include "../../common/game/storySceneEventOverridePlacement.h"
#include "../../common/game/storySceneEventScenePropPlacement.h"
#include "../../common/game/storySceneEventWorldPropPlacement.h"
#include "../../common/game/storySceneEventEnhancedCameraBlend.h"
#include "../../common/game/storySceneItems.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/itemIterator.h"
#include "../../common/game/storySceneEventLightProperties.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

CEdSceneKeyframeCtrl::CEdSceneKeyframeCtrl( CEdSceneEditor* mediator )
	: m_mediator( mediator )
	, m_selectedEntity( nullptr )
	, m_selectedEvent( nullptr )
{
	m_selectedEntityHelper = CGUID::Create();
}

void CEdSceneKeyframeCtrl::Init()
{
	// create a default helper for selected entities
	RecreateDefaultHelper();
}

void CEdSceneKeyframeCtrl::OnNewKeyframe()
{
	const CStorySceneEvent* newEvent = nullptr;

	CEntity* selectedEntity = m_selectedEntity.Get();
	if ( !selectedEntity )
	{
		if ( CStorySceneEvent* selectedEvent = m_mediator->OnKeyframeCtrl_GetSelectedEvent() )
		{
			if ( selectedEvent->GetClass()->IsA< CStorySceneEventLightProperties >() )
			{
				newEvent = m_mediator->OnKeyframeCtrl_CloneEvent( selectedEvent );
			}
		}

		if ( !newEvent )
		{
			return;
		}
	}

	// TODO don't call functions directly! - CreateAndSelectEventAndItemWithAnimation or SelectItemWithAnimation

	if ( const CActor* actor = AsSceneActor( selectedEntity ) )
	{
		CName id( actor->GetVoiceTag() );
		if ( !m_mediator->OnKeyframeCtrl_HasAnyEventNow( ClassID< CStorySceneEventOverridePlacement >(), id ) )
		{
			newEvent = m_mediator->CreateAndSelectEventAndItemWithAnimation( ClassID< CStorySceneEventOverridePlacement >(), id );
		}
	}
	else if ( const CEntity* prop = AsSceneProp( selectedEntity ) )
	{
		CName id( prop->GetName() );
		if ( !m_mediator->OnKeyframeCtrl_HasAnyEventNow( ClassID< CStorySceneEventScenePropPlacement >(), id ) )
		{
			newEvent = Cast< CStorySceneEventScenePropPlacement >( m_mediator->CreateAndSelectEventAndItemWithAnimation( ClassID< CStorySceneEventScenePropPlacement >(), id ) );
		}
	}
	else if ( const CEntity* light = AsSceneLight( selectedEntity ) )
	{
		CName id( light->GetName() );
		if ( !m_mediator->OnKeyframeCtrl_HasAnyEventNow( ClassID< CStorySceneEventLightProperties >(), id ) )
		{
			newEvent = Cast< CStorySceneEventLightProperties >( m_mediator->CreateAndSelectEventAndItemWithAnimation( ClassID< CStorySceneEventLightProperties >(), id ) );
		}
	}

	if ( newEvent )
	{
		m_mediator->SelectItemWithAnimation( newEvent );

		SEdPopupNotification::GetInstance().Show( m_mediator, TXT("New Event"), newEvent->GetClass()->GetName().AsString() );
	}
}

const CActor* CEdSceneKeyframeCtrl::AsSceneActor( const CEntity* e ) const
{
	return m_mediator->OnKeyframeCtrl_AsSceneActor( e );
}

const CEntity* CEdSceneKeyframeCtrl::AsSceneProp( const CEntity* e ) const
{
	return m_mediator->OnKeyframeCtrl_AsSceneProp( e );
}

const CEntity* CEdSceneKeyframeCtrl::AsSceneLight( const CEntity* e ) const
{
	return m_mediator->OnKeyframeCtrl_AsSceneLight( e );
}

Bool CEdSceneKeyframeCtrl::OnHelperEntityRefreshedProperty( const CGUID& id, Bool pos, Bool rot, Bool scale )
{
	// update selected entity transform
	CEntity* ent = m_selectedEntity.Get();
	if( id == m_selectedEntityHelper && ent )
	{
		if ( pos )
		{
			ent->SetPosition( m_selectedEntityTransform.GetPosition() );
		}
		if ( rot )
		{
			ent->SetRotation( m_selectedEntityTransform.GetRotation() );
		}		
		if ( scale )
		{
			ent->SetScale( m_selectedEntityTransform.GetScale() );
		}
		ent->ScheduleUpdateTransformNode();
		return true;
	}

	return false;
}

void CEdSceneKeyframeCtrl::HighlightEntity( CEntity* entity, bool isSelected )
{
	SCENE_ASSERT( entity );
	if ( !entity )
	{
		return;
	}

	if ( CActor* a = Cast< CActor >( entity ) )
	{
		for ( EntityWithItemsComponentIterator< CDrawableComponent > it( a ); it; ++it )
		{
			CDrawableComponent* c = *it;

			if ( c->GetRenderProxy() )
			{
				( new CRenderCommand_SetSelectionFlag( c->GetRenderProxy(), isSelected ) )->Commit();
			}
		}
	}
	else
	{
		for ( ComponentIterator< CDrawableComponent > it( entity ); it; ++it )
		{
			CDrawableComponent* c = *it;

			if ( c->GetRenderProxy() )
			{
				( new CRenderCommand_SetSelectionFlag( c->GetRenderProxy(), isSelected ) )->Commit();
			}
		}
	}
}

CEdSceneHelperEntity* CEdSceneKeyframeCtrl::SelectEntity( CEntity* entity, Bool eventSelected, const SSceneHelperReferenceFrameSettings* frameInfo )
{
	CEntity* prevSelectedEntity = m_selectedEntity.Get();
	if ( prevSelectedEntity )
	{
		HighlightEntity( prevSelectedEntity, false );
	}

	m_selectedEntity = entity;
	if ( entity )
	{
		HighlightEntity( entity, true );
	}

	if ( !eventSelected )
	{
		// update selected entity helper
		m_mediator->OnKeyframeCtrl_DeselectAllHelperEntities();
		CEdSceneHelperEntity* hEnt = m_mediator->OnKeyframeCtrl_FindHelperEntity( m_selectedEntityHelper );
		if ( frameInfo )
		{
			hEnt->SetFrameSettings( *frameInfo );
		}		
		if( hEnt )
		{
			if( entity )
			{
				// can't be named the same since that interfers with entity creation (duplicate checks will fail)
				hEnt->SetName( TXT( "_" ) + entity->GetName() + TXT( "_" ) );

				const Vector pos = entity->GetPosition();
				const EulerAngles rot = entity->GetRotation();

				// Don\t call SetPosition etc. because of entity helper's callback
				m_selectedEntityTransform.SetPosition( pos );
				m_selectedEntityTransform.SetRotation( rot );
				hEnt->SelectHelper();

				return hEnt;
			}
			else
			{
				hEnt->DeselectHelper();
			}
		}
	}

	return nullptr;
}

void CEdSceneKeyframeCtrl::MoveDefaultHelperToSelectedEntity()
{
	CEntity* ent = m_selectedEntity.Get();
	if( ent )
	{
		CEdSceneHelperEntity* helper = m_mediator->OnKeyframeCtrl_FindHelperEntity( m_selectedEntityHelper );
		if( helper )
		{
			const Vector pos = ent->GetWorldPosition();
			const EulerAngles rot = ent->GetWorldRotation();

			// Don\t call SetPosition etc. because of entity helper's callback
			helper->Move( pos, rot );
		}
	}
}

void CEdSceneKeyframeCtrl::RecreateDefaultHelper()
{
	// re-create default helper in new world
	m_selectedEntityTransform.Identity();	
	m_mediator->OnKeyframeCtrl_HelperEntityCreate( m_selectedEntityHelper, m_selectedEntityTransform );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
