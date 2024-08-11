/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/hud.h"

#include "r4JournalManager.h"

//////////////////////////////////////////////////////////////////////////
// CInteractionManager
//////////////////////////////////////////////////////////////////////////
#ifdef USE_SCALEFORM
class CInteractionManager
{
protected:
	Bool								m_interactionsUpdated;
	THandle< CInteractionComponent >	m_activeInteraction;

public:
	CInteractionManager();
	RED_INLINE CInteractionComponent* GetActiveInteraction() const { return m_activeInteraction.Get(); }
	void SetActiveInteraction( CInteractionComponent* interaction, Bool force );
	void Tick();
	void ForceUpdate();
};
#endif //USE_SCALEFORM

//////////////////////////////////////////////////////////////////////////
// CR4Hud
//////////////////////////////////////////////////////////////////////////
class CR4Hud : public CHud
{
	DECLARE_ENGINE_CLASS( CR4Hud, CHud, 0 );

private:
#ifdef USE_SCALEFORM
	CInteractionManager					m_interactionManager;
#endif //USE_SCALEFORM

public:
	static const Float					ONELINER_TEXT_WIDTH;

public:
	virtual	void						Tick( float timeDelta ) override;
	virtual void						OnFinalize() override;

public:
	void								EnableInput( Bool enable );

public:
	void								OnQuestEvent( const CJournalQuest* quest );
	void								OnQuestObjectiveEvent( const CJournalQuest* quest, const CJournalQuestObjective* objective );
	void								OnCharacterEvent( const CJournalCharacter* character );
	void								OnCharacterDescriptionEvent( const CJournalCharacterDescription* characterDescription );
	void								OnGlossaryEvent( const CJournalGlossary* glossary );
	void								OnGlossaryDescriptionEvent( const CJournalGlossaryDescription* glossaryDescription );
	void								OnTutorialEvent( const CJournalTutorial* character );
	void								OnCreatureEvent( const CJournalCreature* creature );
	void								OnCreatureDescriptionEvent( const CJournalCreatureDescriptionEntry* creatureDescription );
	void								OnStoryBookPageEvent( const CJournalStoryBookPage* storyBookPage );
	void								OnPlaceEvent( const CJournalPlace* place );
	void								OnPlaceDescriptionEvent( const CJournalPlaceDescription* placeDescription );
	void								OnQuestTrackingStarted( const CJournalQuest* quest );
	void								OnTrackedQuestUpdated( const CJournalQuest* quest );
	void								OnTrackedQuestObjectivesUpdated( const CJournalQuestObjective* objective );
	void								OnTrackedQuestObjectiveCounterUpdated( const CJournalQuestObjective* objective );
	void								OnTrackedQuestObjectiveHighlighted( const CJournalQuestObjective* objective, Int32 objectiveIndex );
	void								OnHuntingQuestAdded();
	void								OnHuntingQuestClueFound();

public:
	void								ShowSubtitle( ISceneActorInterface * actor, const String & text, Bool alternativeUI );
	void								HideSubtitle( ISceneActorInterface * actor );
	void								DebugTextShow( const String& text );
	void								DebugTextHide();
	CInteractionComponent*				GetActiveInteraction() const;
	void								SetActiveInteraction( CInteractionComponent * interaction, Bool force );
	void								ShowOneliner( const String& plainText, const CEntity* entity );
	void								HideOneliner( const CEntity* entity );
	void								ForceUpdateInteractions();

private:
	Bool								GetActorHeadIconScreenPosition( const CActor* actor, Bool allowOffScreen, Vector& screenPos );
	Bool								GetEntityAimIconScreenPosition( const CEntity* entity, Bool allowOffScreen, Vector& screenPos );
	Bool								GetEntityIconScreenPosition( const Vector& worldPos, Bool allowOffScreen, Vector& screenPos );

private:
	void								funcGetActorHeadIconScreenPosition( CScriptStackFrame& stack, void* result );
	void								funcShowOneliner( CScriptStackFrame& stack, void* result );
	void								funcHideOneliner( CScriptStackFrame& stack, void* result );
	void								funcForceInteractionUpdate( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CR4Hud );
PARENT_CLASS( CHud );
	NATIVE_FUNCTION( "GetActorHeadIconScreenPosition", funcGetActorHeadIconScreenPosition );
	NATIVE_FUNCTION( "ShowOneliner", funcShowOneliner );
	NATIVE_FUNCTION( "HideOneliner", funcHideOneliner );
	NATIVE_FUNCTION( "ForceInteractionUpdate", funcForceInteractionUpdate );
END_CLASS_RTTI();
