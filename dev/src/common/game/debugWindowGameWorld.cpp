/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowGameWorld.h"
#include "gameWorld.h"
#include "commonGame.h"
#include "encounter.h"
#include "spawnTreeEntry.h"
#include "spawnTreeMultiEntry.h"
#include "entityPool.h"

#include "../engine/triggerManagerImpl.h"
#include "../engine/redGuiTab.h"
#include "../engine/redGuiList.h"
#include "../engine/redGuiListItem.h"
#include "../engine/redGuiGridLayout.h"
#include "../engine/redGuiScrollPanel.h"
#include "../engine/redGuiTreeView.h"
#include "../engine/redGuiTreeNode.h"
#include "../engine/redGuiPanel.h"
#include "../engine/redGuiButton.h"
#include "../engine/redGuiCheckBox.h"
#include "../engine/redGuiProgressBar.h"
#include "../engine/redGuiManager.h"
#include "../engine/redGuiLabel.h"
#include "../engine/layerGroup.h"
#include "../engine/layerInfo.h"
#include "../engine/worldIterators.h"
#include "../core/depot.h"


namespace DebugWindows
{
	CDebugWindowGameWorld::CDebugWindowGameWorld()
		: RedGui::CRedGuiWindow( 200, 200, 900, 600 )
	{
		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowGameWorld::NotifyOnTick );
		SetCaption(TXT("Game world"));

		CreateControls();
	}

	CDebugWindowGameWorld::~CDebugWindowGameWorld()
	{
		GRedGui::GetInstance().EventTick.Unbind( this, &CDebugWindowGameWorld::NotifyOnTick );
	}

	void CDebugWindowGameWorld::OnWindowOpened( CRedGuiControl* control )
	{
		if( m_tabs->GetActiveTabIndex() == TT_Encounters )
		{
			FillEncountersTab();
		}
	}

	void CDebugWindowGameWorld::CreateControls()
	{
		m_tabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
		m_tabs->SetMargin( Box2( 5, 5, 5 ,5 ) );
		m_tabs->SetDock( RedGui::DOCK_Fill );
		m_tabs->EventTabChanged.Bind( this, &CDebugWindowGameWorld::NotifySelectTab );
		AddChild( m_tabs );

		// create tabs
		CreateComponentsTab();
		CreateRequestsTab();
		CreateLayersTab();
		CreateEncountersTab();

		m_tabs->SetActiveTab( TT_Components );
	}

	void CDebugWindowGameWorld::CreateComponentsTab()
	{
		m_tabs->AddTab( TXT("Components") );

		RedGui::CRedGuiScrollPanel* componentsTab = m_tabs->GetTabAt( TT_Components );
		if( componentsTab != nullptr )
		{
			//left panel
			RedGui::CRedGuiPanel* leftHolderPanel = new RedGui::CRedGuiPanel( 0, 0, 270, 100 );
			leftHolderPanel->SetDock( RedGui::DOCK_Left );
			leftHolderPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
			componentsTab->AddChild( leftHolderPanel );
			{
				RedGui::CRedGuiButton* dumpAllBtn = new RedGui::CRedGuiButton( 0, 0, 15, 15 );
				dumpAllBtn->EventButtonClicked.Bind( this, &CDebugWindowGameWorld::DumpAllInfo );
				dumpAllBtn->SetText( TXT("Dump All") );
				dumpAllBtn->SetMargin( Box2( 0, 5, 0, 0 ) );
				dumpAllBtn->SetBackgroundColor( Color(255, 0, 0) );
				leftHolderPanel->AddChild( dumpAllBtn );
				dumpAllBtn->SetDock( RedGui::DOCK_Bottom );

				m_componentsList = new RedGui::CRedGuiList( 0, 0, 270, 100 );
				m_componentsList->SetDock( RedGui::DOCK_Fill );
				m_componentsList->SetSorting( true );
				m_componentsList->AppendColumn( TXT("Component name"), 200 );
				m_componentsList->AppendColumn( TXT("Count"), 70, RedGui::SA_Integer );
				m_componentsList->EventSelectedItem.Bind( this, &CDebugWindowGameWorld::NotifySelectedComponent );
				leftHolderPanel->AddChild( m_componentsList );
			}


			//right panel
			RedGui::CRedGuiButton* dumpCompInfoBtn = new RedGui::CRedGuiButton( 0, 0, 15, 15 );
			dumpCompInfoBtn->EventButtonClicked.Bind( this, &CDebugWindowGameWorld::DumpComponentsInfo );
			dumpCompInfoBtn->SetText( TXT("Dump components info") );
			dumpCompInfoBtn->SetMargin( Box2(5,5,5,5));
			dumpCompInfoBtn->SetBackgroundColor( Color(255, 0, 0) );
			componentsTab->AddChild( dumpCompInfoBtn );
			dumpCompInfoBtn->SetDock( RedGui::DOCK_Bottom );

			m_componentDescriptionList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_componentDescriptionList->SetDock( RedGui::DOCK_Fill );
			m_componentDescriptionList->SetSorting( true );
			m_componentDescriptionList->SetMargin( Box2( 5, 5, 5, 5 ) );
			
			m_componentDescriptionList->AppendColumn( TXT("Entity name"), 250, RedGui::SA_String );
			m_componentDescriptionList->AppendColumn( TXT("IsStr"), 50, RedGui::SA_String );
			m_componentDescriptionList->AppendColumn( TXT("Component name"), 200, RedGui::SA_String );
			m_componentDescriptionList->AppendColumn( TXT("Streamed"), 50, RedGui::SA_Integer );
			m_componentDescriptionList->AppendColumn( TXT("Distance"), 75, RedGui::SA_Real );
			m_componentDescriptionList->AppendColumn( TXT("Layer"), 300, RedGui::SA_String );
			m_componentDescriptionList->AppendColumn( TXT("Att"), 50, RedGui::SA_String );
			m_componentDescriptionList->AppendColumn( TXT("ET"), 300, RedGui::SA_String );
			m_componentDescriptionList->AppendColumn( TXT("StreamDistance"), 300, RedGui::SA_Integer );
			componentsTab->AddChild( m_componentDescriptionList );
		}
	}

	void CDebugWindowGameWorld::UpdateComponentsTab()
	{
		m_counts.ClearFast();

		TDynArray< CComponent* > components;

		if( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if( world != nullptr )
			{
				world->GetAttachedComponentsOfClass< CComponent >( components );

				const Uint32 componentCount = components.Size();
				for( Uint32 i=0; i<componentCount; ++i )
				{
					CName className = components[i]->GetClass()->GetName();
					Uint32 unusedKey;
					if ( m_counts.Find( className, unusedKey ) )
					{
						++m_counts[className];
					}
					else
					{
						m_counts.Insert( className, 1 );
					}
				}
			}
		}

		// update list left
		for( THashMap< CName, Uint32 >::iterator iter = m_counts.Begin(); iter != m_counts.End(); ++iter )
		{
			CName className = iter->m_first;
			Uint32 classCount = iter->m_second;

			Int32 index = m_componentsList->Find( className.AsString() );
			if( index != -1 )
			{
				m_componentsList->SetItemText( ToString( classCount ), index, 1 );
			}
			else
			{
				index = m_componentsList->AddItem( className.AsString() );
				m_componentsList->SetItemText( ToString( classCount ), index, 1 );
			}
		}
	}

	void CDebugWindowGameWorld::CreateRequestsTab()
	{
		m_tabs->AddTab( TXT("Requests") );

		RedGui::CRedGuiScrollPanel* requestsTab = m_tabs->GetTabAt( TT_Requests );
		if( requestsTab != nullptr )
		{
			RedGui::CRedGuiPanel* bottomPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 26 );
			bottomPanel->SetDock( RedGui::DOCK_Bottom );
			bottomPanel->SetBorderVisible( false );
			bottomPanel->SetBackgroundColor( Color::CLEAR );
			bottomPanel->SetMargin( Box2( 3, 3, 3, 3 ) );
			requestsTab->AddChild( bottomPanel );

			m_autoUpdate = new RedGui::CRedGuiCheckBox( 0, 0, 100, 20 );
			m_autoUpdate->SetText( TXT("Auto update") );
			m_autoUpdate->SetDock( RedGui::DOCK_Left );
			m_autoUpdate->SetMargin( Box2( 3, 3, 3, 3 ) );
			m_autoUpdate->EventCheckedChanged.Bind( this, &CDebugWindowGameWorld::NotifyChangeAutoUpdate );
			bottomPanel->AddChild( m_autoUpdate );

			RedGui::CRedGuiButton* updateButton = new RedGui::CRedGuiButton( 0, 0, 100, 20 );
			updateButton->SetText( TXT("Update") );
			updateButton->SetFitToText( true );
			updateButton->SetDock( RedGui::DOCK_Right );
			updateButton->SetMargin( Box2( 3, 3, 3, 3 ) );
			updateButton->EventButtonClicked.Bind( this, &CDebugWindowGameWorld::NotifyUpdateRequest );
			bottomPanel->AddChild( updateButton );

			m_autoUpdateTimeProgress = new RedGui::CRedGuiProgressBar( 0, 0, 100, 20 );
			m_autoUpdateTimeProgress->SetDock( RedGui::DOCK_Fill );
			m_autoUpdateTimeProgress->SetProgressRange( 60.0f );
			m_autoUpdateTimeProgress->SetShowProgressInformation( true );
			m_autoUpdateTimeProgress->SetProgressInformation( TXT("Auto update is disabled") );
			m_autoUpdateTimeProgress->SetMargin( Box2( 3, 3, 3, 3 ) );
			bottomPanel->AddChild( m_autoUpdateTimeProgress );

			m_requestList = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_requestList->SetMargin( Box2( 3, 3, 3, 3 ) );
			m_requestList->SetDock( RedGui::DOCK_Fill );
			m_requestList->AppendColumn( TXT("Entity tag"), 400 );
			m_requestList->AppendColumn( TXT("Request details"), 450 );
			m_requestList->SetDock( RedGui::DOCK_Fill );
			requestsTab->AddChild( m_requestList );
		}
	}

	void CDebugWindowGameWorld::UpdateRequestTab( Float deltaTime )
	{
		if( m_autoUpdate->GetChecked() == true )
		{
			m_autoUpdateTimer -= deltaTime;
			m_autoUpdateTimeProgress->SetProgressPosition( m_autoUpdateTimer );
			m_autoUpdateTimeProgress->SetProgressInformation( String::Printf( TXT("%d seconds to update"), (Int32)m_autoUpdateTimer ) );
			if( m_autoUpdateTimer < 0.0f )
			{
				UpdateWorldRequests();
			}
		}
	}

	void CDebugWindowGameWorld::UpdateWorldRequests()
	{
		m_requestList->RemoveAllItems();

		TDynArray< StateChangeRequestsDesc > requestsDesc;

		if( GCommonGame != nullptr )
		{
			CGameWorld* world = GCommonGame->GetActiveWorld();
			if( world != nullptr )
			{
				world->OnStateChangeRequestsDebugPage( requestsDesc );

				const Uint32 requestCount = requestsDesc.Size();
				for( Uint32 i=0; i<requestCount; ++i )
				{
					Uint32 itemIndex = m_requestList->AddItem( requestsDesc[i].m_entityTag );
					m_requestList->SetItemText( requestsDesc[i].m_requestDetails, itemIndex, 1 );
				}
			}
		}

		// set new timer
		m_autoUpdateTimer = 60.0f;
	}

	void CDebugWindowGameWorld::NotifyUpdateRequest( RedGui::CRedGuiEventPackage& eventPackage )
	{
		UpdateWorldRequests();
	}

	void CDebugWindowGameWorld::NotifyChangeAutoUpdate( RedGui::CRedGuiEventPackage& eventPackage, Bool checked )
	{
		if( checked == true )
		{
			UpdateWorldRequests();
		}
		else
		{
			m_autoUpdateTimeProgress->SetProgressPosition( 0.0f );
			m_autoUpdateTimeProgress->SetProgressInformation( TXT("Auto update is disabled") );
		}
	}

	void CDebugWindowGameWorld::CreateLayersTab()
	{
		m_tabs->AddTab( TXT("Layers") );

		RedGui::CRedGuiScrollPanel* layersTab = m_tabs->GetTabAt( TT_Layers );
		if( layersTab != nullptr )
		{
			m_layersTreeView = new RedGui::CRedGuiTreeView( 0, 0, 100, 100 );
			m_layersTreeView->SetDock( RedGui::DOCK_Fill );
			layersTab->AddChild( m_layersTreeView );
		}
	}

	void CDebugWindowGameWorld::NotifyOnTick( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == false )
		{
			return;
		}

		switch( m_tabs->GetActiveTabIndex() )
		{
		case TT_Components:
			UpdateComponentsTab();
			break;
		case TT_Layers:
			UpdateLayersTab();
			break;
		case TT_Requests:
			UpdateRequestTab( deltaTime );
			break;
		case TT_Encounters:
			UpdateEncountersTab();
			break;
		}
	}

	void CDebugWindowGameWorld::NotifySelectedComponent( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedRow )
	{
		m_componentDescriptionList->RemoveAllItems();
		const String selectedName = m_componentsList->GetItemText( selectedRow );

		if( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if( world != nullptr )
			{
				TDynArray< CComponent* > components;
				world->GetAttachedComponentsOfClass< CComponent >( components );

				THashMap< CLayer*, Uint32 >	m_layerCounts;

				for (Uint32 i = 0; i < components.Size(); ++i)
				{
					const String className = components[i]->GetClass()->GetName().AsString();
					if ( className == selectedName )
					{
						CComponent* comp = components[i];
						CEntity* entity = comp->GetEntity();
						CLayer* layer = entity->GetLayer();
						Uint32 strDist = entity->GetStreamingDistance();
						
						const Vector& camPos = world->GetCameraPosition();
						Float distFromCam = components[i]->GetLocalToWorld().GetTranslationRef().DistanceTo( camPos );

						String att = TXT("No");
						if ( comp->GetTransformParent() )
						{
							att = TXT("Yes");
						}
						String entTempName = TXT("No");
						if ( CEntityTemplate* et = entity->GetEntityTemplate() )
						{
							entTempName = et->GetFriendlyName();
						}

						Int32 index = m_componentDescriptionList->AddItem( entity->GetName() );
						m_componentDescriptionList->SetItemText( entity->ShouldBeStreamed() ? TXT("Yes") : TXT("No"), index, 1 );
						m_componentDescriptionList->SetItemText( components[i]->GetName(), index, 2 );
						m_componentDescriptionList->SetItemText( ToString( components[i]->IsStreamed() ), index, 3 );
						m_componentDescriptionList->SetItemText( ToString( distFromCam ), index, 4 );
						m_componentDescriptionList->SetItemText( layer->GetFriendlyName(), index, 5 );
						m_componentDescriptionList->SetItemText( att, index, 6 );
						m_componentDescriptionList->SetItemText( entTempName, index, 7 );
						m_componentDescriptionList->SetItemText( ToString( strDist ), index, 8 );

						Uint32 unused;
						if ( m_layerCounts.Find( layer, unused ) )
						{
							++m_layerCounts[layer];
						}
						else
						{
							m_layerCounts.Insert( layer, 1 );
						}
					}
				}
			}
		}
	}

	void CDebugWindowGameWorld::NotifySelectTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* selectedTab )
	{
		RED_UNUSED( eventPackage );

		if( selectedTab == m_tabs->GetTabAt( TT_Layers ) )
		{
			m_layersTreeView->RemoveAllNodes();
			FillLayersTab();
		}
		else if( selectedTab == m_tabs->GetTabAt( TT_Encounters ) )
		{
			FillEncountersTab();
		}
	}

	void CDebugWindowGameWorld::FillLayersTab()
	{
		if( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if ( world != nullptr )
			{
				CLayerGroup* group = world->GetWorldLayers();
				RedGui::CRedGuiTreeNode* newNode = m_layersTreeView->AddRootNode( group->GetName() );
				newNode->SetImage( REDGUI_PATH + TXT("gameworld\\world.xbm") );
				newNode->SetUserString( TXT("Type"), TXT("World") );
				newNode->SetUserString( TXT("Name"), group->GetName() );
				newNode->SetUserData( group );

				ReqursiveFillTreeView( group, newNode );
			}
		}
	}

	void CDebugWindowGameWorld::ReqursiveFillTreeView( CLayerGroup* group, RedGui::CRedGuiTreeNode* node )
	{
		const CLayerGroup::TGroupList& groups = group->GetSubGroups();
		for ( auto it = groups.Begin(); it != groups.End(); ++it )
		{
			CLayerGroup* newGroup = *it;
			RedGui::CRedGuiTreeNode* newNode = node->AddNode( newGroup->GetName() );
			newNode->SetImage( REDGUI_PATH + TXT("gameworld\\layergroup.xbm") );
			newNode->SetUserString( TXT("Type"), TXT("LayerGroup") );
			newNode->SetUserString( TXT("Name"), newGroup->GetName() );
			newNode->SetUserData( newGroup );

			ReqursiveFillTreeView( newGroup, newNode );
		}

		const CLayerGroup::TLayerList& layers = group->GetLayers();
		for ( auto it = layers.Begin(); it != layers.End(); ++it )
		{
			CLayerInfo* inf = *it;
			RedGui::CRedGuiTreeNode* newNode = node->AddNode( inf->GetShortName() );
			newNode->SetImage( REDGUI_PATH + TXT("gameworld\\layer.xbm") );
			newNode->SetUserString( TXT("Type"), TXT("Layer") );
			newNode->SetUserString( TXT("Name"), inf->GetShortName() );
			newNode->SetUserData( inf );
		}
	}

	void CDebugWindowGameWorld::UpdateLayersTab()
	{
		const Uint32 rootNodeCount = m_layersTreeView->GetRootNodeCount();
		RedGui::TreeNodeCollection& rootNodes = m_layersTreeView->GetRootNodes();
		for( Uint32 i=0; i<rootNodeCount; ++i )
		{
			ReqursiveUpdateLayersTab( rootNodes[i] );
		}
	}

	void CDebugWindowGameWorld::ReqursiveUpdateLayersTab( RedGui::CRedGuiTreeNode* node )
	{
		const String type = node->GetUserString( TXT("Type") );
		if( type == TXT("Layer") )
		{
			CLayerInfo* inf = node->GetUserData< CLayerInfo >();
			if( inf != nullptr )
			{
				if( inf->IsLoaded() == true )
				{
					node->SetImage( REDGUI_PATH + TXT("gameworld\\layervisibled.xbm") );
				}
				else
				{
					node->SetImage( REDGUI_PATH + TXT("gameworld\\layer.xbm") );
				}

				String name = node->GetUserString( TXT("Name") );
				String visible = ( inf->IsVisible() == true ) ? TXT("YES") : TXT("NO");
				String mergedVisibility = ( inf->GetMergedVisiblityFlag() == true ) ? TXT("YES") : TXT("NO");
				node->SetText( String::Printf( TXT( "%s     -     IsVisible: %s          IsMergedVisible: %s" ), name.AsChar(), visible.AsChar(),mergedVisibility.AsChar() ) );
			}
		}
		else if( type == TXT("LayerGroup") )
		{
			CLayerGroup* group = node->GetUserData< CLayerGroup >();
			if( group != nullptr )
			{
				TDynArray< CLayerInfo*> layers;
				group->GetLayers( layers, true );
				if( group->IsLoaded() == true && layers.Empty() == false )
				{
					node->SetImage( REDGUI_PATH + TXT("gameworld\\layergroupvisibled.xbm") );
				}
				else
				{
					node->SetImage( REDGUI_PATH + TXT("gameworld\\layergroup.xbm") );
				}

				Color color = group->GetErrorFlag() ? Color::RED : Color::WHITE;
				node->SetTextColor( color );

				String name = node->GetUserString( TXT("Name") );
				String visible = ( group->IsVisible() == true || layers.Empty() == false ) ? TXT("YES") : TXT("NO");
				node->SetText( String::Printf( TXT( "%s     -     IsVisible: %s" ), name.AsChar(), visible.AsChar() ) );
			}
		}

		if( node->GetExpanded() == true )
		{
			const Uint32 nodeCount = node->GetNodeCount();
			for( Uint32 i=0; i<nodeCount; ++i )
			{
				ReqursiveUpdateLayersTab( node->GetNodeAt( i ) );
			}
		}
	}

	void CDebugWindowGameWorld::CreateEncountersTab()
	{
		m_tabs->AddTab( TXT("Encounters") );

		RedGui::CRedGuiScrollPanel* encountersTab = m_tabs->GetTabAt( TT_Encounters );
		if( encountersTab != nullptr )
		{
			RedGui::CRedGuiPanel* generalInfo = new RedGui::CRedGuiPanel( 0, 0, 250, 30 );
			generalInfo->SetMargin( Box2(5, 5, 5, 5) );
			generalInfo->SetPadding( Box2(5, 5, 5, 5) );
			generalInfo->SetBackgroundColor( Color(20, 20, 20, 255) );
			generalInfo->SetDock( RedGui::DOCK_Top );
			encountersTab->AddChild( generalInfo );
			{
				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 4, 1 );
				generalInfo->AddChild( layout );

				m_totalSpawned = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_totalSpawned->SetText( TXT("Total Spawned: ") );
				layout->AddChild( m_totalSpawned );

				m_spawnedFromEncounters = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_spawnedFromEncounters->SetText( TXT("Spawned From Encounter: ") );
				layout->AddChild( m_spawnedFromEncounters );

				m_inPool = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_inPool->SetText( TXT("In Pool: ") );
				layout->AddChild( m_inPool );
			}

			m_encountersList = new RedGui::CRedGuiList( 0, 0, 300, 100 );
			m_encountersList->SetDock( RedGui::DOCK_Left );
			m_encountersList->SetSorting( true );
			m_encountersList->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_encountersList->AppendColumn( TXT("Encounter name"), 200 );
			m_encountersList->AppendColumn( TXT("Spawned"), 100, RedGui::SA_Integer );
			m_encountersList->EventSelectedItem.Bind( this, &CDebugWindowGameWorld::NotifySelectedEncounter );
			encountersTab->AddChild( m_encountersList );

			RedGui::CRedGuiPanel* entiriesInfo = new RedGui::CRedGuiPanel( 0, 0, 250, 30 );
			entiriesInfo->SetMargin( Box2(5, 5, 5, 5) );
			entiriesInfo->SetPadding( Box2(5, 5, 5, 5) );
			entiriesInfo->SetBackgroundColor( Color(20, 20, 20, 255) );
			entiriesInfo->SetDock( RedGui::DOCK_Bottom );
			encountersTab->AddChild( entiriesInfo );
			{
				RedGui::CRedGuiGridLayout* layout = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
				layout->SetDock( RedGui::DOCK_Fill );
				layout->SetDimensions( 4, 1 );
				entiriesInfo->AddChild( layout );

				m_entriesCount = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_entriesCount->SetText( TXT("Entries: ") );
				layout->AddChild( m_entriesCount );

				m_partiesCount = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_partiesCount->SetText( TXT("Parties: ") );
				layout->AddChild( m_partiesCount );

				m_otherCount = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
				m_otherCount->SetText( TXT("Other: ") );
				layout->AddChild( m_otherCount );
			}

			m_encounterTabs = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
			m_encounterTabs->SetMargin( Box2( 5, 5, 5 ,5 ) );
			m_encounterTabs->SetDock( RedGui::DOCK_Fill );
			encountersTab->AddChild( m_encounterTabs );

			// Entries tab
			{
				Uint32 tabIndex = m_encounterTabs->AddTab( TXT("Entries") );
				RedGui::CRedGuiScrollPanel* entriesTab = m_encounterTabs->GetTabAt( tabIndex );
				m_entriesList = new RedGui::CRedGuiList( 0, 0, 400, 100 );
				m_entriesList->SetDock( RedGui::DOCK_Fill );
				m_entriesList->SetSorting( true );
				m_entriesList->SetMargin( Box2( 5, 5, 5, 5 ) );
				m_entriesList->AppendColumn( TXT("Entry name"), 200 );
				m_entriesList->AppendColumn( TXT("Spawned"), 100, RedGui::SA_Integer );
				m_entriesList->AppendColumn( TXT("Max"), 100, RedGui::SA_Integer );
				entriesTab->AddChild( m_entriesList );
			}

			// Parties tab
			{
				Uint32 tabIndex = m_encounterTabs->AddTab( TXT("Parties") );
				RedGui::CRedGuiScrollPanel* partiesTab = m_encounterTabs->GetTabAt( tabIndex );
				m_partiesList = new RedGui::CRedGuiList( 0, 0, 160, 100 );
				m_partiesList->SetDock( RedGui::DOCK_Left );
				m_partiesList->SetMargin( Box2( 5, 5, 5, 5 ) );
				m_partiesList->AppendColumn( TXT("Party Name"), 80 );
				m_partiesList->AppendColumn( TXT("Is Spawned"), 80 );
				m_partiesList->EventSelectedItem.Bind( this, &CDebugWindowGameWorld::NotifySelectedParty );
				partiesTab->AddChild( m_partiesList );

				m_partyEntriesList = new RedGui::CRedGuiList( 0, 0, 300, 100 );
				m_partyEntriesList->SetDock( RedGui::DOCK_Fill );
				m_partyEntriesList->SetSorting( true );
				m_partyEntriesList->SetMargin( Box2( 5, 5, 5, 5 ) );
				m_partyEntriesList->AppendColumn( TXT("Entry name"), 200 );
				m_partyEntriesList->AppendColumn( TXT("Count"), 100, RedGui::SA_Integer );
				partiesTab->AddChild( m_partyEntriesList );
			}
		}
	}

	void CDebugWindowGameWorld::FillEncountersTab()
	{
		m_encountersList->RemoveAllItems();
		m_entriesList->RemoveAllItems();
		m_partiesList->RemoveAllItems();

		if( GGame && GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetTriggerManager() )
		{
			CTriggerManager* manager = static_cast<CTriggerManager*>( GGame->GetActiveWorld()->GetTriggerManager() );

			for ( Uint32 i = 0, size = manager->GetNumObjects(); i < size; ++i )
			{
				const CTriggerObject* object = manager->GetObject(i);
				if ( NULL != object )
				{
					CEncounter* encounter = SafeCast<CEncounter>( object->GetComponent()->GetEntity() );
					if( encounter )
					{
						m_encountersList->AddItem( encounter->GetName(), Color::WHITE, encounter );
					}
				}
			}
		}
	}

	void CDebugWindowGameWorld::NotifySelectedEncounter( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedRow )
	{
		m_entriesList->RemoveAllItems();
		m_partiesList->RemoveAllItems();
	}

	void CDebugWindowGameWorld::NotifySelectedParty( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedRow )
	{
		m_partyEntriesList->RemoveAllItems();
	}

	void CDebugWindowGameWorld::UpdateEncountersTab()
	{
		Uint32 spawnedFromEncounters = 0;
		Uint32 entriesCount = 0;
		Uint32 partiesCount = 0;
		Uint32 otherCount = 0;

		for( Uint32 i = 0, size = m_encountersList->GetItemCount(); i < size; ++i )
		{
			CEncounter* encounter = reinterpret_cast<CEncounter*>( m_encountersList->GetItemUserData( i ) );

			const Uint32 creatureCount = encounter->GetCreaturePool().GetCreaturesCount();
			spawnedFromEncounters += creatureCount;

			m_encountersList->SetItemText( ToString( creatureCount ), i, 1 );

			if( m_encountersList->GetSelection() == (Int32)i )
			{
				for( auto it = encounter->m_activeEntriesList.Begin(), end = encounter->m_activeEntriesList.End();
					it != end; ++it )
				{					
					const CEncounter::SActiveEntry* entry = *it;
					if( entry->m_entry->IsA<CCreatureEntry>() )
					{
						AddEntry( static_cast<CCreatureEntry*>(entry->m_entry), *entry->m_instanceBuffer );
						++entriesCount;
					}
					else if( entry->m_entry->IsA<CCreaturePartyEntry>() )
					{
						AddParty( static_cast<CCreaturePartyEntry*>(entry->m_entry), *entry->m_instanceBuffer );
						++partiesCount;
					}
					else
					{
						++otherCount;
					}
				}
			}
		}

		// update labels
		Uint32 totalSpawned = 0;
		Uint32 spawnLimit = 0;
		Uint32 inPool = 0;
		Uint32 poolLimit = 0;
		if( GCommonGame && GCommonGame->GetEntityPool() )
		{
			totalSpawned = GCommonGame->GetEntityPool()->GetEntitySpawnedCount();
			spawnLimit = GCommonGame->GetEntityPool()->GetMaxEntitySpawnCount();
			inPool = GCommonGame->GetEntityPool()->GetEntityCount();
			poolLimit = GCommonGame->GetEntityPool()->GetMaxEntityCount();
		}
		m_totalSpawned->SetText( String::Printf( TXT("Total Spawned: %d/%d"), totalSpawned, spawnLimit ) );
		m_spawnedFromEncounters->SetText( String::Printf( TXT("Spawned From Encounter: %d"), spawnedFromEncounters ) );
		m_inPool->SetText( String::Printf( TXT("In Pool: %d/%d"), inPool, poolLimit ) );

		m_entriesCount->SetText( String::Printf( TXT("Entries: %d"), entriesCount ) );
		m_partiesCount->SetText( String::Printf( TXT("Parties: %d"), partiesCount ) );
		m_otherCount->SetText( String::Printf( TXT("Other: %d"), otherCount ) );
	}

	void CDebugWindowGameWorld::AddEntry( CCreatureEntry* entry, CSpawnTreeInstance& instance )
	{
		Bool found = false;
		Uint32 index = 0;
		for( Uint32 i = 0, size = m_entriesList->GetItemCount(); i < size; ++i )
		{
			if( m_entriesList->GetItemUserData( i ) == (void*)entry )
			{
				found = true;
				index = i;
				break;
			}
		}

		if( !found )
		{
			index = m_entriesList->AddItem( entry->GetCreatureDefinitionName().AsString(), Color::WHITE, entry );
		}

		m_entriesList->SetItemText( ToString( entry->GetNumCreaturesSpawned( instance ) ), index, 1 );
		m_entriesList->SetItemText( ToString( entry->GetNumCreaturesToSpawn( instance ) ), index, 2 );
	}

	void CDebugWindowGameWorld::AddParty( CCreaturePartyEntry* party, CSpawnTreeInstance& instance )
	{
		Bool found = false;
		Uint32 index = 0;
		for( Uint32 i = 0, size = m_partiesList->GetItemCount(); i < size; ++i )
		{
			if( m_partiesList->GetItemUserData( i ) == (void*)party )
			{
				found = true;
				index = i;
				break;
			}
		}

		if( !found )
		{
			index = m_partiesList->AddItem( String::Printf( TXT("Party %d"), m_partiesList->GetItemCount() ), Color::WHITE, party );
		}

		m_partiesList->SetItemText( party->GetNumCreaturesSpawned( instance ) ? TXT("Yes") : TXT("No"), index, 1 );

		if( m_partiesList->GetSelection() == (Int32)index )
		{
			for( auto it = party->m_subDefinitions.Begin(), end = party->m_subDefinitions.End(); it != end; ++it )
			{
				AddPartyEntry( *it );
			}
		}
	}

	void CDebugWindowGameWorld::AddPartyEntry( CSpawnTreeEntrySubDefinition* def )
	{
		Bool found = false;
		Uint32 index = 0;
		for( Uint32 i = 0, size = m_partyEntriesList->GetItemCount(); i < size; ++i )
		{
			if( m_partyEntriesList->GetItemUserData( i ) == (void*)def )
			{
				found = true;
				index = i;
				break;
			}
		}

		if( !found )
		{
			index = m_partyEntriesList->AddItem( def->GetCreatureDefinitionName().AsString(), Color::WHITE, def );
		}

		m_partyEntriesList->SetItemText( ToString( def->GetSpawnedCreaturesCount() ), index, 1 );
	}

	void CDebugWindowGameWorld::DumpComponentsInfo( RedGui::CRedGuiEventPackage& eventPackage )
	{
		Int32 selectedCmpIndex = m_componentsList->GetSelection();
		DumpSelectedInfo( selectedCmpIndex );
	}

	void CDebugWindowGameWorld::DumpAllInfo(RedGui::CRedGuiEventPackage& eventPackage)
	{
		Uint32 size = m_componentsList->GetItemCount();
		for ( Uint32 i =0; i<size; ++i )
		{
			NotifySelectedComponent( eventPackage, i );
			DumpSelectedInfo( i );
		}
	}

	void CDebugWindowGameWorld::DumpSelectedInfo( Int32 selectedRow )
	{
		if( GGame != nullptr )
		{
			CWorld* world = GGame->GetActiveWorld();
			if( world != nullptr )
			{
				TDynArray< CComponent* > components;
				world->GetAttachedComponentsOfClass< CComponent >( components );
				const Uint32 size = m_componentDescriptionList->GetItemCount();

				String className = m_componentsList->GetItemText( selectedRow, 0 );

				String outPath = String::EMPTY;
				String depoPath = GFileManager->GetDataDirectory();
				outPath += depoPath;
				String worldPath = world->DepotPath();
				String ext = TXT(".csv");
				outPath += worldPath;
				outPath += TXT("_");
				outPath += className;
				outPath += TXT("_");
				outPath += ext;

				String outFile = String::EMPTY;
				const Uint32 colSize = m_componentDescriptionList->GetColumnCount();
				String row = String::EMPTY;

				// first row with column names
				for ( Uint32 i=0; i< colSize; ++i )
				{
					String colName = m_componentDescriptionList->GetColumnLabel( i );
					row += colName;
					row += TXT(";");
					if ( i == colSize-1 )
					{
						row += TXT("\n");
					}
				}

				outFile += row;

				//correct data
				for (Uint32 i = 0; i < size; ++i)
				{
					RedGui::CRedGuiListItem* item = m_componentDescriptionList->GetItem( i );
					row = String::EMPTY;
					for ( Uint32 j=0; j<colSize; ++j )
					{
						const String& col = item->GetText( j );
						row += col;
						row += TXT(";");
						if ( j == colSize-1 )
						{
							row += TXT("\n");
						}
					}
					outFile += row;
				}
				GFileManager->SaveStringToFile( outPath, outFile );
			}
		}
	}

}	// namespace DebugWindows

#endif	//NO_DEBUG_WINDOWS
#endif	//NO_RED_GUI
