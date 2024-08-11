/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "r4Hud.h"

#include "../../common/game/interactionsManager.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/interactionComponent.h"
#include "../../common/game/interactionsManager.h"
#include "../../common/game/bgNpc.h"
#include "../../common/engine/flashPlayerScaleform.h"
#include "r4GuiManager.h"
#include "../../common/engine/viewport.h"

RED_DEFINE_STATIC_NAME( OnCharacterEvent );
RED_DEFINE_STATIC_NAME( OnCharacterDescriptionEvent );
RED_DEFINE_STATIC_NAME( OnGlossaryEvent );
RED_DEFINE_STATIC_NAME( OnGlossaryDescriptionEvent );
RED_DEFINE_STATIC_NAME( OnTutorialEvent );
RED_DEFINE_STATIC_NAME( OnCreatureEvent );
RED_DEFINE_STATIC_NAME( OnCreatureDescriptionEvent );
RED_DEFINE_STATIC_NAME( OnStoryBookPageEvent );
RED_DEFINE_STATIC_NAME( OnPlaceEvent );
RED_DEFINE_STATIC_NAME( OnPlaceDescriptionEvent );
RED_DEFINE_STATIC_NAME( OnQuestEvent );
RED_DEFINE_STATIC_NAME( OnQuestObjectiveEvent );
RED_DEFINE_STATIC_NAME( OnQuestTrackingStarted );
RED_DEFINE_STATIC_NAME( OnTrackedQuestUpdated );
RED_DEFINE_STATIC_NAME( OnTrackedQuestObjectivesUpdated );
RED_DEFINE_STATIC_NAME( OnTrackedQuestObjectiveCounterUpdated );
RED_DEFINE_STATIC_NAME( OnTrackedQuestObjectiveHighlighted );
RED_DEFINE_STATIC_NAME( OnHuntingQuestAdded );
RED_DEFINE_STATIC_NAME( OnHuntingQuestClueFound );

RED_DEFINE_STATIC_NAME( OnInteractionsUpdated );
RED_DEFINE_STATIC_NAME( OnSubtitleAdded )
RED_DEFINE_STATIC_NAME( OnSubtitleRemoved )
RED_DEFINE_STATIC_NAME( OnCreateOneliner )
RED_DEFINE_STATIC_NAME( OnRemoveOneliner )
RED_DEFINE_STATIC_NAME( OnDebugTextShown )
RED_DEFINE_STATIC_NAME( OnDebugTextHidden )


namespace // anonymous
{
	Bool GetScreenPosition( const Matrix& worldToScreen, const Vector& position, Vector& screenPosition, Float guiWidth, Float guiHeight, Bool allowOffScreen )
	{
		Vector actorPosition = position;
		actorPosition.W =  1.f;
		Vector screenSpacePoint = worldToScreen.TransformVectorWithW( actorPosition );

		if ( screenSpacePoint.W < 0.001f )
		{
			return false;
		}

		screenSpacePoint.Div4( screenSpacePoint.W );

		if ( !allowOffScreen )
		{
			if ( screenSpacePoint.X < -1.f || screenSpacePoint.X > 1.f ||
				 screenSpacePoint.Y < -1.f || screenSpacePoint.Y > 1.f )
			{
				return false;
			}
		}

		screenPosition.X = (  screenSpacePoint.X + 1.0f ) * guiWidth  * 0.5f;
		screenPosition.Y = ( -screenSpacePoint.Y + 1.0f ) * guiHeight * 0.5f;
		screenPosition.Z = screenSpacePoint.Z;
		return true;
	}
}

//////////////////////////////////////////////////////////////////////////
// CInteractionManager
//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
CInteractionManager::CInteractionManager()
	: m_interactionsUpdated( false )
	, m_activeInteraction( nullptr )
{
}

void CInteractionManager::SetActiveInteraction( CInteractionComponent * interaction, Bool force )
{
	if ( force || interaction != m_activeInteraction )
	{
		m_activeInteraction = interaction;
		m_interactionsUpdated = true;
	}
}

void CInteractionManager::Tick()
{
	if ( !m_interactionsUpdated )
	{
		return;
	}

	CR4GuiManager* guiManager = Cast< CR4GuiManager >( GCommonGame->GetGuiManager() );
	if ( !guiManager )
	{
		return;
	}
	CR4Hud* hud = Cast< CR4Hud >( guiManager->GetHud() );
	if ( !hud )
	{
		return;
	}

	m_interactionsUpdated = false;
	hud->CallEvent( CNAME( OnInteractionsUpdated ), m_activeInteraction );
}

void CInteractionManager::ForceUpdate()
{
	m_interactionsUpdated = true;
}
#endif // USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CR4Hud );

const Float CR4Hud::ONELINER_TEXT_WIDTH = 300.f;

void CR4Hud::Tick( float timeDelta )
{
	TBaseClass::Tick( timeDelta );

#ifdef USE_SCALEFORM
	m_interactionManager.Tick();
#endif //USE_SCALEFORM
}

void CR4Hud::OnFinalize()
{
	TBaseClass::OnFinalize();
}

void CR4Hud::EnableInput( Bool enable )
{
	CFlashMovie* movie = GetFlashMovie();
	if ( movie )
	{
		movie->SetInputSourceFlags( enable? CFlashMovie::ISF_All: CFlashMovie::ISF_None );
	}
}

void CR4Hud::OnCharacterEvent( const CJournalCharacter* character )
{
	CallEvent( CNAME( OnCharacterEvent ), THandle< CJournalCharacter >( character ) );
}

void CR4Hud::OnCharacterDescriptionEvent( const CJournalCharacterDescription* characterDescription )
{
	CallEvent( CNAME( OnCharacterDescriptionEvent ), THandle< CJournalCharacterDescription >( characterDescription ) );
}

void CR4Hud::OnGlossaryEvent( const CJournalGlossary* glossary )
{
	CallEvent( CNAME( OnGlossaryEvent ), THandle< CJournalGlossary >( glossary ) );
}

void CR4Hud::OnGlossaryDescriptionEvent( const CJournalGlossaryDescription* glossaryDescription )
{
	CallEvent( CNAME( OnGlossaryDescriptionEvent ), THandle< CJournalGlossaryDescription >( glossaryDescription ) );
}

void CR4Hud::OnTutorialEvent( const CJournalTutorial* tutorial )
{
	CallEvent( CNAME( OnTutorialEvent ), THandle< CJournalTutorial >( tutorial ) );
}

void CR4Hud::OnCreatureEvent( const CJournalCreature* creature )
{
	CallEvent( CNAME( OnCreatureEvent ), THandle< CJournalCreature >( creature ) );
}

void CR4Hud::OnCreatureDescriptionEvent( const CJournalCreatureDescriptionEntry* creatureDescription )
{
	CallEvent( CNAME( OnCreatureDescriptionEvent ), THandle< CJournalCreatureDescriptionEntry >( creatureDescription ) );
}

void CR4Hud::OnStoryBookPageEvent( const CJournalStoryBookPage* storyBookPage )
{
	CallEvent( CNAME( OnStoryBookPageEvent ), THandle< CJournalStoryBookPage >( storyBookPage ) );
}

void CR4Hud::OnPlaceEvent( const CJournalPlace* place )
{
	CallEvent( CNAME( OnPlaceEvent ), THandle< CJournalPlace >( place ) );
}

void CR4Hud::OnPlaceDescriptionEvent( const CJournalPlaceDescription* placeDescription )
{
	CallEvent( CNAME( OnPlaceDescriptionEvent ), THandle< CJournalPlaceDescription >( placeDescription ) );
}

void CR4Hud::OnQuestEvent( const CJournalQuest* quest )
{
	CallEvent( CNAME( OnQuestEvent ), THandle< CJournalQuest >( quest ) );
}

void CR4Hud::OnQuestObjectiveEvent( const CJournalQuest* quest, const CJournalQuestObjective* objective )
{
	CallEvent( CNAME( OnQuestObjectiveEvent ), THandle< CJournalQuest >( quest ), THandle< CJournalQuestObjective >( objective ) );
}

void CR4Hud::OnQuestTrackingStarted( const CJournalQuest* quest )
{
	CallEvent( CNAME( OnQuestTrackingStarted ), THandle< CJournalQuest >( quest ) );
}

void CR4Hud::OnTrackedQuestUpdated( const CJournalQuest* quest )
{
	CallEvent( CNAME( OnTrackedQuestUpdated ), THandle< CJournalQuest >( quest ) );
}

void CR4Hud::OnTrackedQuestObjectivesUpdated( const CJournalQuestObjective* objective )
{
	CallEvent( CNAME( OnTrackedQuestObjectivesUpdated ), THandle< CJournalQuestObjective >( objective ) );
}

void CR4Hud::OnTrackedQuestObjectiveCounterUpdated( const CJournalQuestObjective* objective )
{
	CallEvent( CNAME( OnTrackedQuestObjectiveCounterUpdated ), THandle< CJournalQuestObjective >( objective ) );
}

void CR4Hud::OnTrackedQuestObjectiveHighlighted( const CJournalQuestObjective* objective, Int32 objectiveIndex )
{
	CallEvent( CNAME( OnTrackedQuestObjectiveHighlighted ), THandle< CJournalQuestObjective >( objective ), objectiveIndex );
}

void CR4Hud::OnHuntingQuestAdded()
{
	CallEvent( CNAME( OnHuntingQuestAdded ) );
}

void CR4Hud::OnHuntingQuestClueFound()
{
	CallEvent( CNAME( OnHuntingQuestClueFound ) );
}

void CR4Hud::ShowSubtitle( ISceneActorInterface * actor, const String & text, Bool alternativeUI )
{
	const CEntity* entity = actor->GetSceneParentEntity();
	if ( entity )
	{
		// guids are not unique on consoles!
		CallEvent( CNAME( OnSubtitleAdded ), ::GetHash( reinterpret_cast< Uint64 >( entity ) ), entity->GetDisplayName(), text, alternativeUI );
	}
}

void CR4Hud::HideSubtitle( ISceneActorInterface * actor )
{
	const CEntity* entity = actor->GetSceneParentEntity();
	if ( entity )
	{
		// guids are not unique on consoles!
		CallEvent( CNAME( OnSubtitleRemoved ), ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );
	}
}

void CR4Hud::DebugTextShow( const String& text )
{
	CallEvent( CNAME( OnDebugTextShown ), text );
}

void CR4Hud::DebugTextHide()
{
	CallEvent( CNAME( OnDebugTextHidden ) );
}

CInteractionComponent* CR4Hud::GetActiveInteraction() const
{
#ifdef USE_SCALEFORM
	return m_interactionManager.GetActiveInteraction();
#else
	return nullptr;
#endif //USE_SCALEFORM
}

void CR4Hud::SetActiveInteraction( CInteractionComponent * interaction, Bool force )
{
#ifdef USE_SCALEFORM
	m_interactionManager.SetActiveInteraction( interaction, force );
#endif //USE_SCALEFORM
}

void CR4Hud::ShowOneliner( const String& plainText, const CEntity* entity )
{
	// guids are not unique on consoles!
	Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );

	CallEvent( CNAME( OnCreateOneliner ), THandle< CEntity >( entity ), plainText, id );
}

void CR4Hud::HideOneliner( const CEntity* entity )
{
	// guids are not unique on consoles!
	Int32 id = static_cast< Int32 >( ::GetHash( reinterpret_cast< Uint64 >( entity ) ) );

	CallEvent( CNAME( OnRemoveOneliner ), id );
}

Bool CR4Hud::GetActorHeadIconScreenPosition( const CActor* actor, Bool allowOffScreen, Vector& screenPos )
{
	Vector worldPos;

	if ( !actor )
	{
		return false;
	}
	worldPos = actor->GetHeadPosition();
	worldPos.Z += 0.5f;

	return GetEntityIconScreenPosition( worldPos, allowOffScreen, screenPos );
}

Bool CR4Hud::GetEntityAimIconScreenPosition( const CEntity* entity, Bool allowOffScreen, Vector& screenPos )
{
	Vector worldPos;
	if ( entity->IsA< CGameplayEntity >() )
	{
		worldPos = Cast< CGameplayEntity >( entity )->GetAimPosition();
	}
	else
	{
		worldPos = entity->GetWorldPosition(); // just in case
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// TEMPORARY FIX FOR PREALPHA - interaction arrow a bit up to entity, but it should be set for each entity separately!
	worldPos.Z += 0.5f;
	//
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	return GetEntityIconScreenPosition( worldPos, allowOffScreen, screenPos );
}

Bool CR4Hud::GetEntityIconScreenPosition( const Vector& worldPos, Bool allowOffScreen, Vector& screenPos )
{
	// TODO: Clean up the witchergame loop instead of recreating the frameInfo and matrix here
	IViewport* vp = GGame->GetViewport();
	const CRenderFrameInfo frameInfo( vp );

	const Matrix worldToScreen = frameInfo.m_camera.GetWorldToScreen();

	if ( GetScreenPosition( worldToScreen, worldPos, screenPos, static_cast< Float >( vp->GetWidth() ), static_cast< Float >( vp->GetHeight() ), allowOffScreen ) )
	{
		screenPos.X = screenPos.X / vp->GetWidth();
		screenPos.Y = screenPos.Y / vp->GetHeight();
		return true;
	}
	return false;
}

void CR4Hud::ForceUpdateInteractions()
{
    m_interactionManager.ForceUpdate();
}

void CR4Hud::funcGetActorHeadIconScreenPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, actor, 0 );
	GET_PARAMETER( Bool, allowOffScreen, false );
	GET_PARAMETER_REF( Vector, screenPos, Vector::ZEROS );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetActorHeadIconScreenPosition( actor.Get(), allowOffScreen, screenPos ) );
}

void CR4Hud::funcShowOneliner( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, plainText, String::EMPTY );
	GET_PARAMETER( THandle< CEntity >, entity, 0 );
	FINISH_PARAMETERS;

	ShowOneliner( plainText, entity.Get() );
}

void CR4Hud::funcHideOneliner( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, entity, 0 );
	FINISH_PARAMETERS;

	HideOneliner( entity.Get() );
}

void CR4Hud::funcForceInteractionUpdate( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

#ifdef USE_SCALEFORM
	m_interactionManager.ForceUpdate();
#endif
}
