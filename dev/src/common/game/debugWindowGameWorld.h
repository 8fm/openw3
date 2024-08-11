/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "../engine/redGuiWindow.h"

class CCreatureEntry;
class CCreaturePartyEntry;
class CSpawnTreeInstance;

namespace DebugWindows
{
	class CDebugWindowGameWorld : public RedGui::CRedGuiWindow
	{
		enum ETabType
		{
			TT_Components,
			TT_Requests,
			TT_Layers,
			TT_Encounters,

			TT_Count
		};

	public:
		CDebugWindowGameWorld();
		~CDebugWindowGameWorld();

	protected:
		void OnWindowOpened( CRedGuiControl* control ) override;

	private:
		void NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime );
		void NotifySelectTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedTab );

		void CreateControls();

		// components
		void CreateComponentsTab();
		void UpdateComponentsTab();
		void NotifySelectedComponent( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedRow );
		void DumpComponentsInfo( RedGui::CRedGuiEventPackage& eventPackage );

		void DumpSelectedInfo( Int32 rowIndex );

		void DumpAllInfo( RedGui::CRedGuiEventPackage& eventPackage );

		// Requests
		void CreateRequestsTab();
		void UpdateRequestTab( Float deltaTime );
		void UpdateWorldRequests();
		void NotifyUpdateRequest( RedGui::CRedGuiEventPackage& eventPackage );
		void NotifyChangeAutoUpdate( RedGui::CRedGuiEventPackage& eventPackage, Bool checked );

		// Layers
		void CreateLayersTab();
		void FillLayersTab();
		void ReqursiveFillTreeView( CLayerGroup* group, RedGui::CRedGuiTreeNode* node );
		void UpdateLayersTab();
		void ReqursiveUpdateLayersTab( RedGui::CRedGuiTreeNode* node );

		// Encounters
		void CreateEncountersTab();
		void FillEncountersTab();
		void UpdateEncountersTab();
		void NotifySelectedEncounter( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedRow );
		void NotifySelectedParty( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedRow );
		void AddEntry( CCreatureEntry* entry, CSpawnTreeInstance& instance );
		void AddParty( CCreaturePartyEntry* party, CSpawnTreeInstance& instance );
		void AddPartyEntry( CSpawnTreeEntrySubDefinition* def );

	private:
		RedGui::CRedGuiTab*				m_tabs;

		// components
		RedGui::CRedGuiList*			m_componentsList;
		RedGui::CRedGuiList*			m_componentDescriptionList;

		// components - logic
		THashMap< CName, Uint32 >		m_counts;

		// request world
		RedGui::CRedGuiList*			m_requestList;
		RedGui::CRedGuiCheckBox*		m_autoUpdate;
		RedGui::CRedGuiProgressBar*		m_autoUpdateTimeProgress;
		// request world - logic
		Float							m_autoUpdateTimer;

		// layers
		RedGui::CRedGuiTreeView*		m_layersTreeView;

		// encounters
		RedGui::CRedGuiLabel*			m_totalSpawned;
		RedGui::CRedGuiLabel*			m_spawnedFromEncounters;
		RedGui::CRedGuiLabel*			m_inPool;
		RedGui::CRedGuiList*			m_encountersList;
		RedGui::CRedGuiTab*				m_encounterTabs;
		RedGui::CRedGuiLabel*			m_entriesCount;
		RedGui::CRedGuiLabel*			m_partiesCount;
		RedGui::CRedGuiLabel*			m_otherCount;
		RedGui::CRedGuiList*			m_entriesList;
		RedGui::CRedGuiList*			m_partiesList;
		RedGui::CRedGuiList*			m_partyEntriesList;
	};

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
