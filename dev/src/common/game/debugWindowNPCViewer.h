/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/redGuiWindow.h"

class CNewNPC;

namespace DebugWindows
{
	class CDebugWindowNPCViewer : public RedGui::CRedGuiWindow
	{
		enum ESpawnTypeTab
		{
			STT_Community,
			STT_Encounter,
			STT_Spawner,

			STT_Count,
		};

		enum EInfoViewerTypeTab
		{
			IVTT_General,
			IVTT_Stats,
			IVTT_Inventory,
			IVTT_Movement,
			IVTT_QuestLocks,
			IVTT_NoticedObjects,
			IVTT_Attitudes,
			IVTT_MeshComponents,
			IVTT_StoryInformations,
			IVTT_Morale,
			IVTT_Loot,

			IVTT_Count,
		};

	public:
		CDebugWindowNPCViewer();	
		~CDebugWindowNPCViewer();

	private:
		virtual void OnWindowClosed(CRedGuiControl* control);
		virtual void OnWindowOpened(CRedGuiControl* control);

		CNewNPC* GetNPCFromIndex( Uint32 index );
		void RefreshInformation();
		void SyncToNewNPC( CNewNPC* npc );
		void NotifyViewportCalculateCamera( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, CRenderCamera& camera );
		void NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data );
		Vector CalculateCameraOrbit( CNewNPC* npc );
		void ResetAllInformation();

		void NotifyOnButtonClicked( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnButtonDoubleClicked( RedGui::CRedGuiEventPackage& eventPackage, Int32 value);
		void NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value);
		void NotifyMouseButtonDoubleClickOnMeshComponent( RedGui::CRedGuiEventPackage& eventPackage, Int32 value);
		void NotifyEventToggleButtonChange( RedGui::CRedGuiEventPackage& eventPackage, Bool value);
		void NotifyEventButtonClickedOnOpenEntity( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyEventButtonClickedOnTeleportPlayerToLastPoint( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOnFrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta);
		void NotifyKillNPC( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyStunNPC( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyDestroyNPC( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOpenSpawnset( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOpenActiveAP( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyOpenItemDefinitionFile( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifySetItemParameters( RedGui::CRedGuiEventPackage& eventPackage, Int32 index );

		// create gui
		void CreateGeneralTab();
		void CreateStatsTab();
		void CreateInventoryTab();
		void CreateMovementTab();
		void CreateQuestLocksTab();
		void CreateNoticedObjectsTab();
		void CreateAttitudesTab();
		void CreateMeshesTab();
		void CreateStoryTab();
		void CreateMoraleTab();
		void CreateLootTab();

		// fill gui
		void ShowInfoAboutNPC( CNewNPC* npc );
		void SetGeneralInformation( CNewNPC* npc );
		void SetStatsInformation( CNewNPC* npc );
		void SetScriptInformation( CNewNPC* npc );
		void SetCommunityInformation( CNewNPC* npc );
		void SetInventoryInformation( CNewNPC* npc );
		void SetMovementInformation( CNewNPC* npc );
		void SetQuestInformation( CNewNPC* npc );
		void SetNoticedInformation( CNewNPC* npc );
		void SetAttitudesInformation( CNewNPC* npc );
		void SetMeshInformation( CNewNPC* npc );
		void SetStoryInformation( CNewNPC* npc );

		// helpers
		void UpdateBaseStatEntry( CNewNPC* npc, RedGui::CRedGuiProgressBar* progressBar, Uint32 enumIndex );
		void UpdateResistStatEntry( CNewNPC* npc, RedGui::CRedGuiProgressBar* progressBar, Uint32 enumIndex );
		void UpdateAttackStatEntry( CNewNPC* npc, RedGui::CRedGuiLabel* label, Uint32 enumIndex, const String& startText );
		void UpdateRegenStatEntry( CNewNPC* npc, RedGui::CRedGuiLabel* label, Uint32 enumIndex, const String& startText );
		void UpdateGlobalDamageEntry( CNewNPC* npc, RedGui::CRedGuiLabel* label );

		RedGui::CRedGuiTab*					m_npcTab;
		RedGui::CRedGuiList*				m_communityNPCList;
		RedGui::CRedGuiList*				m_encounterNPCList;
		RedGui::CRedGuiList*				m_spawnersNPCList;
		RedGui::CRedGuiButton*				m_refreshList;
		RedGui::CRedGuiButton*				m_teleportToNPC;
		RedGui::CRedGuiButton*				m_stopRotate;
		RedGui::CRedGuiButton*				m_releaseCamera;
		RedGui::CRedGuiPanel*				m_npcInfoPanel;
		RedGui::CRedGuiTab*					m_npcInfoCategories;

		// General
		RedGui::CRedGuiLabel*				m_generalTemplateName;
		RedGui::CRedGuiLabel*				m_generalMemoryUsageByObject;
		RedGui::CRedGuiLabel*				m_generalMemoryUsageByTemplate;
		RedGui::CRedGuiLabel*				m_generalTags;
		RedGui::CRedGuiLabel*				m_generalAppearance;
		RedGui::CRedGuiLabel*				m_generalVoiceTag;
		RedGui::CRedGuiLabel*				m_generalProfileRecursive;
		RedGui::CRedGuiLabel*				m_generalIsAlive;
		RedGui::CRedGuiLabel*				m_generalBehMachine;
		RedGui::CRedGuiLabel*				m_generalEncounter;
		RedGui::CRedGuiLabel*				m_generalEncounterActiveEntry;
		RedGui::CRedGuiButton*				m_generalOpenEntityTemplate;

		// script
		RedGui::CRedGuiLabel*				m_scriptStateName;
		RedGui::CRedGuiLabel*				m_scriptTopFunction;

		// Community and work
		RedGui::CRedGuiLabel*				m_communityIsInCommunity;
		RedGui::CRedGuiLabel*				m_communityStoryPhase;
		RedGui::CRedGuiLabel*				m_communitySpawnsetName;
		RedGui::CRedGuiLabel*				m_communityCurrentStubState;
		RedGui::CRedGuiLabel*				m_communityActiveAP;
		RedGui::CRedGuiLabel*				m_communityLastAP;
		RedGui::CRedGuiLabel*				m_communityUsingLastAP;
		RedGui::CRedGuiLabel*				m_communityWorkingInAP;
		RedGui::CRedGuiLabel*				m_communityWorkState;
		RedGui::CRedGuiLabel*				m_communityIsInfinite;
		RedGui::CRedGuiLabel*				m_communityExecutionMode;
		RedGui::CRedGuiLabel*				m_communityAPName;
		RedGui::CRedGuiButton*				m_generalOpenSpawnset;
		RedGui::CRedGuiButton*				m_generalOpenAP;

		// Stats
		RedGui::CRedGuiProgressBar*			m_statVitality;
		RedGui::CRedGuiProgressBar*			m_statEssence;
		RedGui::CRedGuiProgressBar*			m_statStamina;
		RedGui::CRedGuiProgressBar*			m_statToxicity;
		RedGui::CRedGuiProgressBar*			m_statFocus;
		RedGui::CRedGuiProgressBar*			m_statMorale;
		RedGui::CRedGuiProgressBar*			m_statDrunkenness;
		RedGui::CRedGuiProgressBar*			m_statAir;

		RedGui::CRedGuiLabel*				m_statAttackPower;
		RedGui::CRedGuiLabel*				m_statSpellPower;
		RedGui::CRedGuiLabel*				m_statDamageBonus;

		RedGui::CRedGuiLabel*				m_statVitalityRegen;
		RedGui::CRedGuiLabel*				m_statEssenceRegen;
		RedGui::CRedGuiLabel*				m_statMoraleRegen;
		RedGui::CRedGuiLabel*				m_statToxicityRegen;
		RedGui::CRedGuiLabel*				m_statDrunkennessRegen;
		RedGui::CRedGuiLabel*				m_statStaminaRegen;
		RedGui::CRedGuiLabel*				m_statAirRegen;

		RedGui::CRedGuiProgressBar*			m_statNoneRes;
		RedGui::CRedGuiProgressBar*			m_statPhysicalRes;
		RedGui::CRedGuiProgressBar*			m_statBleedingRes;
		RedGui::CRedGuiProgressBar*			m_statPoisonRes;
		RedGui::CRedGuiProgressBar*			m_statFireRes;
		RedGui::CRedGuiProgressBar*			m_statFrostRes;
		RedGui::CRedGuiProgressBar*			m_statShockRes;
		RedGui::CRedGuiProgressBar*			m_statForceRes;
		RedGui::CRedGuiProgressBar*			m_statFreezeRes;
		RedGui::CRedGuiProgressBar*			m_statWillRes;
		RedGui::CRedGuiProgressBar*			m_statBurningRes;

		// Inventory
		RedGui::CRedGuiLabel*				m_inventoryState;
		RedGui::CRedGuiLabel*				m_inventorySize;
		RedGui::CRedGuiList*				m_inventoryItemList;
		RedGui::CRedGuiLabel*				m_inventoryDefineFile;
		RedGui::CRedGuiButton*				m_inventoryOpenDefinitionFile;
		RedGui::CRedGuiLabel*				m_inventoryCategory;
		RedGui::CRedGuiLabel*				m_inventoryQuantity;
		RedGui::CRedGuiLabel*				m_inventoryDurability;
		RedGui::CRedGuiLabel*				m_inventoryIsWeapon;
		RedGui::CRedGuiLabel*				m_inventoryIsStackable;
		RedGui::CRedGuiLabel*				m_inventoryIsLootable;


		// Movement
		RedGui::CRedGuiLabel*				m_movementHasComponent;
		RedGui::CRedGuiLabel*				m_movementState;
		RedGui::CRedGuiLabel*				m_movementLastTeleport;
		RedGui::CRedGuiLabel*				m_movementTopRepresentation;
		RedGui::CRedGuiLabel*				m_movementPriority;
		RedGui::CRedGuiLabel*				m_movementQueuePosition;
		RedGui::CRedGuiLabel*				m_movementStaticRotation;
		RedGui::CRedGuiList*				m_movementLocomotionLines;
		RedGui::CRedGuiButton*				m_movementTeleportPlayerToLastPoint;

		// Quest locks
		RedGui::CRedGuiList*				m_questLocksList;

		// Noticed objects 
		RedGui::CRedGuiList*				m_noticedObjectsList;

		// Attitudes
		RedGui::CRedGuiLabel*				m_attitudesState;
		RedGui::CRedGuiList*				m_attitudesList;

		// Meshes
		RedGui::CRedGuiList*				m_meshesList;

		// Story
		RedGui::CRedGuiList*				m_storyQuestList;

		// Morale
		RedGui::CRedGuiLabel*				m_moraleInfo;

		// Loot
		RedGui::CRedGuiLabel*				m_lootInfo;

		// logic
		TDynArray< THandle< CNewNPC > >		m_allNpcs;				//!< All NPCs in the world
		THandle< CNewNPC >					m_active;				//!< Active NPC
		Vector								m_cameraTarget;			//!< Current camera target
		Vector								m_cameraPosition;		//!< Current camera position
		Float								m_cameraRotation;		//!< NPC camera rotation
		Int32								m_verticalOffset;		//!< Verical offset of text
		Bool								m_pauseRotation;		//!< 
		Bool								m_teleportPlayer;		//!< 
		Bool								m_lockRotation;			//!< 
	};
}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
