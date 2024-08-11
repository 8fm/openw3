/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowNPCViewer.h"

#include "actorActionWork.h"
#include "communityAgentStub.h"
#include "communitySystem.h"
#include "communityUtility.h"
#include "movingPhysicalAgentComponent.h"
#include "vehicle.h"
#include "encounter.h"
#include "actionPointComponent.h"
#include "behTreeMachine.h"
#include "../engine/appearanceComponent.h"
#include "../engine/freeCamera.h"
#include "../engine/redGuiList.h"
#include "../engine/redGuiListItem.h"
#include "../engine/redGuiManager.h"
#include "../engine/redGuiTab.h"
#include "../engine/redGuiPanel.h"
#include "../engine/redGuiButton.h"
#include "../engine/redGuiScrollPanel.h"
#include "../engine/redGuiLabel.h"
#include "../engine/redGuiGroupBox.h"
#include "../engine/redGuiProgressBar.h"
#include "../engine/mesh.h"
#include "../core/fileSkipableBlock.h"
#include "../core/memoryFileAnalizer.h"
#include "../engine/meshComponent.h"
#include "spawnTreeBaseEntry.h"

RED_DEFINE_STATIC_NAME( GetStat );
RED_DEFINE_STATIC_NAME( GetStatMax );
RED_DEFINE_STATIC_NAME( GetResistValue );
RED_DEFINE_STATIC_NAME( GetPowerStatValue );
RED_DEFINE_STATIC_NAME( GetAttributeValue );

namespace DebugWindows
{
	CDebugWindowNPCViewer::CDebugWindowNPCViewer()
		: RedGui::CRedGuiWindow(50,50, 1200, 600)
		, m_cameraRotation( 0.0f )
		, m_cameraTarget( Vector::ZEROS )
		, m_cameraPosition( Vector::ZEROS )
		, m_active( nullptr )
		, m_verticalOffset( 0 )
		, m_pauseRotation(false)
		, m_lockRotation( true )
	{
		SetCaption(TXT("NPC viewer"));

		GRedGui::GetInstance().EventTick.Bind(this, &CDebugWindowNPCViewer::NotifyOnFrameTick);
		GRedGui::GetInstance().EventCalculateCamera.Bind(this, &CDebugWindowNPCViewer::NotifyViewportCalculateCamera);
		GRedGui::GetInstance().EventViewportInput.Bind(this, &CDebugWindowNPCViewer::NotifyViewportInput);

		// create left panel
		{
			RedGui::CRedGuiPanel* leftPanel = new RedGui::CRedGuiPanel(0, 0, 300, 300);
			leftPanel->SetBorderVisible(false);
			leftPanel->SetBackgroundColor(Color::CLEAR);
			leftPanel->SetMargin(Box2(10, 10, 10, 10));
			AddChild(leftPanel);
			leftPanel->SetDock(RedGui::DOCK_Left);

			// create bottom panel in left panel
			{
				RedGui::CRedGuiPanel* leftBottomPanel = new RedGui::CRedGuiPanel(0, 0, 300, 70);	
				leftBottomPanel->SetBorderVisible(false);
				leftBottomPanel->SetBackgroundColor(Color::CLEAR);
				leftPanel->AddChild(leftBottomPanel);
				leftBottomPanel->SetDock(RedGui::DOCK_Bottom);

				// first line
				{
					RedGui::CRedGuiPanel* firstLine = new RedGui::CRedGuiPanel(0, 0, 300, 25);	
					firstLine->SetBorderVisible(false);
					firstLine->SetBackgroundColor(Color::CLEAR);
					firstLine->SetMargin(Box2(5, 5, 5, 2));
					leftBottomPanel->AddChild(firstLine);
					firstLine->SetDock(RedGui::DOCK_Top);

					// buttons
					{
						m_refreshList = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_refreshList->SetMargin(Box2(5, 5, 5, 0));
						m_refreshList->SetText(TXT("Refresh list"));
						m_refreshList->EventButtonClicked.Bind(this, &CDebugWindowNPCViewer::NotifyOnButtonClicked);
						firstLine->AddChild(m_refreshList);
						m_refreshList->SetDock(RedGui::DOCK_Left);

						m_teleportToNPC = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_teleportToNPC->SetMargin(Box2(5, 5, 5, 0));
						m_teleportToNPC->SetText(TXT("Teleport to NPC"));
						m_teleportToNPC->EventButtonClicked.Bind(this, &CDebugWindowNPCViewer::NotifyOnButtonClicked);
						firstLine->AddChild(m_teleportToNPC);
						m_teleportToNPC->SetDock(RedGui::DOCK_Left);
						m_teleportToNPC->SetEnabled(false);
					}
				}

				// second line
				{
					RedGui::CRedGuiPanel* secondLine = new RedGui::CRedGuiPanel(0, 0, 300, 25);	
					secondLine->SetBorderVisible(false);
					secondLine->SetBackgroundColor(Color::CLEAR);
					secondLine->SetMargin(Box2(5, 2, 5, 5));
					leftBottomPanel->AddChild(secondLine);
					secondLine->SetDock(RedGui::DOCK_Top);

					// buttons
					{
						m_stopRotate = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_stopRotate->SetMargin(Box2(5, 5, 5, 0));
						m_stopRotate->SetText(TXT("Pause rotation"));
						m_stopRotate->SetToggleMode(true);
						m_stopRotate->SetToggleValue(false);
						m_stopRotate->EventCheckedChanged.Bind(this, &CDebugWindowNPCViewer::NotifyEventToggleButtonChange);
						secondLine->AddChild(m_stopRotate);
						m_stopRotate->SetDock(RedGui::DOCK_Left);

						m_releaseCamera = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_releaseCamera->SetMargin(Box2(5, 5, 5, 0));
						m_releaseCamera->SetText(TXT("Release camera"));
						m_releaseCamera->SetToggleMode(true);
						m_releaseCamera->SetToggleValue(false);
						m_releaseCamera->EventCheckedChanged.Bind(this, &CDebugWindowNPCViewer::NotifyEventToggleButtonChange);
						secondLine->AddChild(m_releaseCamera);
						m_releaseCamera->SetDock(RedGui::DOCK_Left);
					}
				}
			}

			// create list with NPC
			{
				m_npcTab = new RedGui::CRedGuiTab( 0, 0, 280, 500 );
				leftPanel->AddChild( m_npcTab );
				m_npcTab->SetDock( RedGui::DOCK_Fill );
				m_npcTab->AddTab( TXT("Community") );
				m_npcTab->AddTab( TXT("Encounters") );
				m_npcTab->AddTab( TXT("Spawners") );

				// create lists
				{
					// community NPC
					CRedGuiControl* communityNPCTab = m_npcTab->GetTabAt( STT_Community );
					{
						m_communityNPCList = new RedGui::CRedGuiList( 0, 0, 280, 500 );
						m_communityNPCList->AppendColumn( TXT("NPC list spawned from community"), 100 );
						m_communityNPCList->SetBorderVisible( false );
						m_communityNPCList->EventSelectedItem.Bind( this, &CDebugWindowNPCViewer::NotifyEventSelectedItemChanged );
						m_communityNPCList->EventDoubleClickItem.Bind( this, &CDebugWindowNPCViewer::NotifyOnButtonDoubleClicked );
						communityNPCTab->AddChild( m_communityNPCList );
						m_communityNPCList->SetDock( RedGui::DOCK_Fill );
					}

					// encounter NPC
					CRedGuiControl* encountersNPCTab = m_npcTab->GetTabAt( STT_Encounter );
					{
						m_encounterNPCList = new RedGui::CRedGuiList( 0, 0, 280, 500 );
						m_encounterNPCList->AppendColumn( TXT("NPC list spawned from encounters"), 100 );
						m_encounterNPCList->SetBorderVisible( false );
						m_encounterNPCList->EventSelectedItem.Bind( this, &CDebugWindowNPCViewer::NotifyEventSelectedItemChanged );
						m_encounterNPCList->EventDoubleClickItem.Bind( this, &CDebugWindowNPCViewer::NotifyOnButtonDoubleClicked );
						encountersNPCTab->AddChild( m_encounterNPCList );
						m_encounterNPCList->SetDock( RedGui::DOCK_Fill );
					}

					// others NPC
					CRedGuiControl* spawnersNPCTab = m_npcTab->GetTabAt( STT_Spawner );
					{
						m_spawnersNPCList = new RedGui::CRedGuiList( 0, 0, 280, 500 );
						m_spawnersNPCList->AppendColumn( TXT("NPC list spawned from spawners"), 100 );
						m_spawnersNPCList->SetBorderVisible( false );
						m_spawnersNPCList->EventSelectedItem.Bind( this, &CDebugWindowNPCViewer::NotifyEventSelectedItemChanged );
						m_spawnersNPCList->EventDoubleClickItem.Bind( this, &CDebugWindowNPCViewer::NotifyOnButtonDoubleClicked );
						spawnersNPCTab->AddChild( m_spawnersNPCList );
						m_spawnersNPCList->SetDock( RedGui::DOCK_Fill );
					}
				}

				// open default tab
				m_npcTab->SetActiveTab( STT_Community );
			}
		}

		m_npcInfoPanel = new RedGui::CRedGuiPanel(0, 0, 750, 550);
		m_npcInfoPanel->SetBackgroundColor(Color(0,0,0,0));
		m_npcInfoPanel->SetBorderVisible(false);
		m_npcInfoPanel->SetMargin(Box2(10, 10, 10, 10));
		AddChild(m_npcInfoPanel);
		m_npcInfoPanel->SetDock(RedGui::DOCK_Fill);

		m_npcInfoCategories = new RedGui::CRedGuiTab(0,0,0,0);
		m_npcInfoPanel->AddChild(m_npcInfoCategories);
		m_npcInfoCategories->SetDock(RedGui::DOCK_Fill);
		m_npcInfoCategories->AddTab(TXT("General"));
		m_npcInfoCategories->AddTab(TXT("Stats"));
		m_npcInfoCategories->AddTab(TXT("Inventory"));
		m_npcInfoCategories->AddTab(TXT("Movement"));
		m_npcInfoCategories->AddTab(TXT("Quest locks"));
		m_npcInfoCategories->AddTab(TXT("Noticed objects"));
		m_npcInfoCategories->AddTab(TXT("Attitudes"));
		m_npcInfoCategories->AddTab(TXT("Mesh components"));
		m_npcInfoCategories->AddTab(TXT("Story informations"));
		m_npcInfoCategories->AddTab(TXT("Morale"));
		m_npcInfoCategories->AddTab(TXT("Loot"));

		// create controls in tabs
		CreateGeneralTab();
		CreateStatsTab();
		CreateInventoryTab();
		CreateMovementTab();
		CreateQuestLocksTab();
		CreateNoticedObjectsTab();
		CreateAttitudesTab();
		CreateMeshesTab();
		CreateStoryTab();
		CreateMoraleTab();
		CreateLootTab();

		// open default tab
		m_npcInfoCategories->SetActiveTab( IVTT_General );
	}

	CDebugWindowNPCViewer::~CDebugWindowNPCViewer()
	{
		/*intentionally empty*/
	}

	void CDebugWindowNPCViewer::RefreshInformation()
	{
		// Clear list
		CNewNPC* prevActive = m_active;
		m_active = nullptr;

		m_lockRotation = true;
		m_allNpcs.Clear();
		m_communityNPCList->RemoveAllItems();
		m_encounterNPCList->RemoveAllItems();
		m_spawnersNPCList->RemoveAllItems();

		ResetAllInformation();
		Bool active = false;

		// Collect list of NPC
		const CCommonGame::TNPCCharacters& npcs = GCommonGame->GetNPCCharacters();
		for ( CCommonGame::TNPCCharacters::const_iterator it=npcs.Begin(); it!=npcs.End(); ++it )
		{
			::CNewNPC* npc = *it;

			// Skip npc which is a vehicle
			if( npc->FindComponent< CVehicleComponent >() != nullptr )
			{
				continue;
			}

			// Add to list
			m_allNpcs.PushBack( npc );
		}

		if ( GCommonGame->GetPlayer() )
		{
			Vector playerPos = GCommonGame->GetPlayer()->GetWorldPosition();
			Sort( m_allNpcs.Begin(), m_allNpcs.End(), 
				[ &playerPos ]( THandle< CNewNPC > npc1, THandle< CNewNPC > npc2){ return (npc1->GetWorldPosition() - playerPos).Mag3() < (npc2->GetWorldPosition() - playerPos).Mag3(); } );
		}

		Uint32 counter = 0;
		for ( auto it = m_allNpcs.Begin(); it != m_allNpcs.End(); ++it, ++counter )
		{
			CNewNPC* npc = *it;
			active = false;

			// Remember name in separate list since handle can be lost
			String name = npc->GetFriendlyName();
			name = name.StringAfter(TXT("::"), true);

			if ( npc == prevActive )
			{
				m_active = npc;
				active = true;
				ShowInfoAboutNPC( npc );
			}

			// create new item for list
			RedGui::CRedGuiListItem* newItem = new RedGui::CRedGuiListItem( name );
			newItem->SetUserString( TXT("Index"), ToString( counter ) );

			// decide to which list npc should be add
			{
				// check encounter
				TDynArray< IActorTerminationListener* >& terminationListenrs = npc->GetTerminationListeners();
				CEncounter* encounter = nullptr;
				for( Uint32 i=0; i < terminationListenrs.Size(); ++i  )
				{
					encounter = terminationListenrs[ i ]->AsEncounter();
					if( encounter != nullptr )
					{
						Uint32 ind = m_encounterNPCList->AddItem( newItem );
						m_encounterNPCList->SetSelection( ind, active );
						break;
					}
				}
				if( encounter != nullptr )
				{
					continue;
				}

				// check community
				CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
				if ( cs != nullptr )
				{
					const SAgentStub* agentStub = cs->FindStubForNPC( npc );
					if( agentStub != nullptr )
					{
						Uint32 ind = m_communityNPCList->AddItem( newItem );
						m_communityNPCList->SetSelection( ind, active );
						continue;
					}
				}

				// add to spawner list 
				Uint32 ind = m_spawnersNPCList->AddItem( newItem );
				m_spawnersNPCList->SetSelection( ind, active );
			}
		}

		// Use current camera target
		m_cameraPosition = m_cameraTarget;

		// show or hide tabs
		if( m_allNpcs.Empty() )
		{
			m_npcInfoCategories->SetActiveTab( IVTT_General );
		}
	}

	void CDebugWindowNPCViewer::OnWindowClosed( CRedGuiControl* control )
	{
		// Unpause game
		GGame->Unpause( TXT( "CDebugPageNPCList" ) );

		SetEnabled(false);
	}

	void CDebugWindowNPCViewer::OnWindowOpened( CRedGuiControl* control )
	{
		// Pause game
		GGame->Pause( TXT("CDebugPageNPCList") );

		SetEnabled(true);

		m_cameraTarget = Vector::ZEROS;
		if( GGame != nullptr )
		{
			if( const CWorld* world = GGame->GetActiveWorld() )
			{
				if( const CCameraDirector* camera = world->GetCameraDirector() )
				{
					m_cameraTarget = camera->GetCameraPosition();
				}
			}
		}

		RefreshInformation();

		m_cameraPosition = m_cameraTarget;
	}

	void CDebugWindowNPCViewer::NotifyOnButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_refreshList)
		{
			RefreshInformation();
		}
		else if( sender == m_teleportToNPC)
		{
			// teleport player to selected NPC
			Int32 selectedIndex = -1;

			if( m_npcTab->GetActiveTabIndex() == STT_Community )
			{
				selectedIndex = m_communityNPCList->GetSelection();
			}
			else if( m_npcTab->GetActiveTabIndex() == STT_Encounter )
			{
				selectedIndex = m_encounterNPCList->GetSelection();
			}
			else if( m_npcTab->GetActiveTabIndex() == STT_Spawner )
			{
				selectedIndex = m_spawnersNPCList->GetSelection();
			}

			if( selectedIndex != -1 )
			{
				if( CNewNPC* npc = GetNPCFromIndex( selectedIndex ) )
				{
					if( GCommonGame->GetPlayer() != nullptr )
					{
						GCommonGame->GetPlayer()->Teleport(npc, true);
						m_teleportPlayer = true;

						GGame->UnpauseAll();
					}
					else
					{
						GRedGui::GetInstance().MessageBox( TXT("On the scene there is no player."), TXT("Warning"), RedGui::MESSAGEBOX_Warning );
					}
				}
			}
		}
	}

	void CDebugWindowNPCViewer::NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		CNewNPC* npc = GetNPCFromIndex( value );
		if( npc != nullptr )
		{
			m_active = npc;

			ShowInfoAboutNPC( npc );

			if(value != -1)
			{
				m_teleportToNPC->SetEnabled(true);
			}
			else
			{
				m_teleportToNPC->SetEnabled(false);
			}
		}
	}

	void CDebugWindowNPCViewer::NotifyOnFrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if( m_teleportPlayer )
		{
			GGame->Pause( TXT( "CDebugPageNPCList" ) );
			m_teleportPlayer = false;

			RefreshInformation();
		}

		if( GetEnabled() && !m_pauseRotation )
		{
			m_cameraRotation += timeDelta * 30.0f;
		}
	}

	void CDebugWindowNPCViewer::NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		RED_UNUSED( eventPackage );

		if( m_releaseCamera->GetToggleValue() )
		{
			CGameFreeCamera& freeCamera = const_cast<CGameFreeCamera&>(GGame->GetFreeCamera());
			freeCamera.ProcessInput(key, action, data);
		}
	}

	void CDebugWindowNPCViewer::SyncToNewNPC( CNewNPC* npc )
	{
		// Reset vertical offset
		m_verticalOffset = 0;

		// Reset camera
		m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();

		// set new npc
		if ( npc != nullptr )
		{
			m_active = npc;
			m_lockRotation = false;

			m_cameraTarget = CalculateCameraOrbit( npc );
			m_cameraPosition = m_cameraTarget;
		}
	}

	Vector CDebugWindowNPCViewer::CalculateCameraOrbit( CNewNPC* npc )
	{
		// Try to use neck bone :)
		CAnimatedComponent* ac = npc->GetRootAnimatedComponent();
		if ( ac )
		{
			Int32 boneIndex = ac->FindBoneByName( TXT("neck") );
			if ( boneIndex != -1 )
			{
				Matrix boneToWorld = ac->GetBoneMatrixWorldSpace( boneIndex );
				return boneToWorld.GetTranslation();
			}
		}

		// Start with center of the bounding box + some offset
		Uint32 numMeshes = 0;
		Vector pos = Vector::ZEROS;
		for ( ComponentIterator< CMeshComponent > it( npc ); it; ++it )
		{
			// Calculate bounding box
			const Vector meshCenter = (*it)->GetBoundingBox().CalcCenter();;
			pos += meshCenter;
			numMeshes += 1;
		}

		// Use center of drawable shit
		if ( numMeshes )
		{
			pos /= ( Float ) numMeshes;
			return pos + Vector( 0.0f, 0.0f, 0.7f );
		}

		// Use entity position
		return npc->GetWorldPosition();
	}

	void CDebugWindowNPCViewer::NotifyViewportCalculateCamera( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, CRenderCamera& camera )
	{
		RED_UNUSED( eventPackage );

		if( GetEnabled() == true && m_releaseCamera->GetToggleValue() == false && m_lockRotation == false )
		{
			// Show the selected actor
			CNewNPC* npc = m_active.Get();
			if ( npc != nullptr )
			{
					// Orbit camera around NPC
					Vector pos = m_cameraPosition;//CalculateCameraOrbit( npc );
					EulerAngles rot( 0.0f, 0.0f, m_cameraRotation );
	
					// Offset camera
					Vector dir, right;
					rot.ToAngleVectors( &dir, &right, nullptr );
					pos -= dir * 1.5f;
	
					// Construct preview camera
					CRenderCamera previewCamera( pos, rot, camera.GetFOV(), camera.GetAspect(), camera.GetNearPlane(), camera.GetFarPlane(), camera.GetZoom() );
					camera = previewCamera;
				}
			}
		}

	void CDebugWindowNPCViewer::ShowInfoAboutNPC( CNewNPC* npc )
	{
		ResetAllInformation();

		if(npc == nullptr)
		{
			m_npcInfoPanel->SetVisible(false);
		}
		else
		{
			SetGeneralInformation(npc);
			SetStatsInformation( npc );
			SetScriptInformation(npc);
			SetCommunityInformation(npc);
			SetInventoryInformation(npc);
			SetMovementInformation(npc);
			SetQuestInformation(npc);
			SetNoticedInformation(npc);
			SetAttitudesInformation(npc);
			SetMeshInformation(npc);
			SetStoryInformation(npc);
		}
	}

	void CDebugWindowNPCViewer::NotifyOnButtonDoubleClicked( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		SyncToNewNPC( GetNPCFromIndex( value ) );
	}

	void CDebugWindowNPCViewer::SetGeneralInformation( CNewNPC* npc )
	{

		// Template name
		String templateName = (npc->GetEntityTemplate() != nullptr) ? npc->GetEntityTemplate()->GetFriendlyName() : TXT("Unknown");
		m_generalTemplateName->SetText(String::Printf( TXT("Pointer: 0x%llX, Template: '%ls'"), (Uint64)npc, templateName.AsChar()), Color::WHITE);
		m_generalOpenEntityTemplate->SetOutOfDate();

		// Mem usage
		Uint32 memoryUsageByObject = CObjectMemoryAnalizer::CalcObjectSize( npc ) / 1024;
		Uint32 memoryUsageByTemplate = CObjectMemoryAnalizer::CalcObjectSize( npc->GetEntityTemplate() ) / 1024;
		m_generalMemoryUsageByObject->SetText(String::Printf( TXT("Memory usage by object: '%dkB'"), memoryUsageByObject ), Color::YELLOW);
		m_generalMemoryUsageByTemplate->SetText(String::Printf( TXT("Memory usage by template: '%dkB'"), memoryUsageByTemplate ), Color::YELLOW);

		// Tag
		String tags = npc->GetTags().ToString();
		m_generalTags->SetText(String::Printf( TXT("Tags: '%ls'"), tags.AsChar() ), Color::WHITE );

		// Appearance
		CAppearanceComponent* appearanceComponent = npc->GetAppearanceComponent();
		if ( appearanceComponent )
		{
			String appearance = appearanceComponent->GetAppearance().AsString();
			m_generalAppearance->SetText( String::Printf( TXT("Appearance: '%ls'"), appearance.AsChar() ), Color::WHITE );
		}
		else
		{
			m_generalAppearance->SetText( TXT("Without appearance components"), Color::WHITE );
		}

		// Voice tag
		String voiceTag = npc->GetVoiceTag().AsString();
		m_generalVoiceTag->SetText( String::Printf( TXT("VoiceTag: '%ls'"), voiceTag.AsChar() ), Color::WHITE );

		// Attitude group name
		CAIProfile* profileRecursive = npc->GetEntityTemplate() ? npc->GetEntityTemplate()->FindParameter< CAIProfile >( true ) : nullptr;
		m_generalProfileRecursive->SetText( String::Printf( TXT("Attitude group: %ls (def. %ls)"), npc->GetAttitudeGroup().AsString().AsChar(), (profileRecursive ? profileRecursive->GetAttitudeGroup().AsString().AsChar() : TXT("No AI profile defined")) ) );

		// Is alive
		m_generalIsAlive->SetText( String::Printf( TXT("Is alive: %ls"), ( npc->IsAlive() == true) ? TXT("YES") : TXT("NO") ) );

		// Behavior machine info
		CBehTreeMachine* behTM = npc->GetBehTreeMachine();
		if( behTM != nullptr )
		{
			m_generalBehMachine->SetText( behTM->GetInfo() );
		}

		// Encounter
		TDynArray< IActorTerminationListener* >& terminationListenrs = npc->GetTerminationListeners();
		Uint32 debugCount = terminationListenrs.Size();
		TDynArray< String > activeEntries;
		Bool firstEnc = true;
		for( Uint32 i = 0; i < terminationListenrs.Size(); ++i  )
		{
			if ( CEncounter* encounter = terminationListenrs[ i ]->AsEncounter() )
			{
				if ( firstEnc )
				{
					firstEnc = false;
					m_generalEncounter->SetText( String::Printf( TXT("Encounter: %ls at %ls"), encounter->GetName().AsChar(), encounter->GetLayer()->GetDepotPath().AsChar() ) );
				}

				CEncounter::SActiveEntry* entry = encounter->FindActiveCreatureEntry( npc );
				if ( entry && entry->m_entry )
				{
					String text = entry->m_entry->GetName().AsString();
#ifndef NO_EDITOR_STEERING_SUPPORT
					text += String::Printf( TXT(" (%ls)"), entry->m_entry->GetComment().AsChar() );
#endif
					activeEntries.PushBack( text );
				}
			}
		}
		m_generalEncounterActiveEntry->SetText( String::Printf( TXT("Encounter entry: %ls"), String::Join( activeEntries, TXT(",") ).AsChar() ) );
	}

	void CDebugWindowNPCViewer::SetStatsInformation( CNewNPC* npc )
	{
		// this is copy from script file, only for internal using
		enum EBaseCharacterStats
		{
			BCS_Vitality,
			BCS_Essence,
			BCS_Stamina,
			BCS_Toxicity,
			BCS_Focus,
			BCS_Morale,
			BCS_Drunkenness,
			BCS_Air,
			BCS_Undefined
		};

		UpdateBaseStatEntry( npc, m_statVitality, BCS_Vitality );
		UpdateBaseStatEntry( npc, m_statEssence, BCS_Essence );
		UpdateBaseStatEntry( npc, m_statStamina, BCS_Stamina );
		UpdateBaseStatEntry( npc, m_statToxicity, BCS_Toxicity );
		UpdateBaseStatEntry( npc, m_statFocus, BCS_Focus );
		UpdateBaseStatEntry( npc, m_statMorale, BCS_Morale );
		UpdateBaseStatEntry( npc, m_statDrunkenness, BCS_Drunkenness );
		UpdateBaseStatEntry( npc, m_statAir, BCS_Air );

		// this is copy from script file, only for internal using
		enum ECharacterPowerStats
		{
			CPS_AttackPower,
			CPS_SpellPower,
			CPS_Undefined
		};

		UpdateAttackStatEntry( npc, m_statAttackPower, CPS_AttackPower, TXT("Attack power") );
		UpdateAttackStatEntry( npc, m_statSpellPower, CPS_SpellPower, TXT("Spell power") );
		UpdateGlobalDamageEntry( npc, m_statDamageBonus );
		
		// this is copy from script file, only for internal using
		enum ECharacterRegenStats
		{
			CRS_Undefined,
			CRS_Vitality,
			CRS_Essence,
			CRS_Morale,
			CRS_Toxicity,
			CRS_Drunkenness,
			CRS_Stamina,
			CRS_Air
		};

		UpdateRegenStatEntry( npc, m_statVitalityRegen, CRS_Vitality, TXT("Vitality") );
		UpdateRegenStatEntry( npc, m_statEssenceRegen, CRS_Essence, TXT("Essence") );
		UpdateRegenStatEntry( npc, m_statMoraleRegen, CRS_Morale, TXT("Morale") );
		UpdateRegenStatEntry( npc, m_statToxicityRegen, CRS_Toxicity, TXT("Toxicity") );
		UpdateRegenStatEntry( npc, m_statDrunkennessRegen, CRS_Drunkenness, TXT("Drunkenness") );
		UpdateRegenStatEntry( npc, m_statStaminaRegen, CRS_Stamina, TXT("Stamina") );
		UpdateRegenStatEntry( npc, m_statAirRegen, CRS_Air, TXT("Air") );

		// this is copy from script file, only for internal using
		enum ECharacterDefenseStats
		{
			CDS_None,
			CDS_PhysicalRes,
			CDS_BleedingRes,
			CDS_PoisonRes,
			CDS_FireRes,
			CDS_FrostRes,
			CDS_ShockRes,
			CDS_ForceRes,
			CDS_FreezeRes,
			CDS_WillRes,
			CDS_BurningRes
		};

		UpdateResistStatEntry( npc, m_statNoneRes, CDS_None );
		UpdateResistStatEntry( npc, m_statPhysicalRes, CDS_PhysicalRes );
		UpdateResistStatEntry( npc, m_statBleedingRes, CDS_BleedingRes );
		UpdateResistStatEntry( npc, m_statPoisonRes, CDS_PoisonRes );
		UpdateResistStatEntry( npc, m_statFireRes, CDS_FireRes );
		UpdateResistStatEntry( npc, m_statFrostRes, CDS_FrostRes );
		UpdateResistStatEntry( npc, m_statShockRes, CDS_ShockRes );
		UpdateResistStatEntry( npc, m_statForceRes, CDS_ForceRes );
		UpdateResistStatEntry( npc, m_statFreezeRes, CDS_FreezeRes );
		UpdateResistStatEntry( npc, m_statWillRes, CDS_WillRes );
		UpdateResistStatEntry( npc, m_statBurningRes, CDS_BurningRes );
	}

	void CDebugWindowNPCViewer::SetScriptInformation( CNewNPC* npc )
	{
		// Get state name
		String stateName = npc->GetCurrentStateName().AsString();
		m_scriptStateName->SetText( String::Printf( TXT("State: '%ls'"), stateName.AsChar() ), Color::WHITE );

		// Get current thread
		const CFunction* topFunction = npc->GetTopLevelFunction();
		m_scriptTopFunction->SetText( String::Printf( TXT("Function: '%ls'"), topFunction ? topFunction->GetName().AsString().AsChar() : TXT("None") ), Color::WHITE );
	}

	void CDebugWindowNPCViewer::SetCommunityInformation( CNewNPC* npc )
	{
		CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( cs != nullptr )
		{
			// Is NPC in Community
			const SAgentStub *agentStub = cs->FindStubForNPC( npc );
			m_communityIsInCommunity->SetText(String::Printf( TXT("Is NPC in Community: %ls"), agentStub ? TXT("yes") : TXT("no") ) );

			// Agent stub info
			if(agentStub != nullptr)
			{
				// Story phase
				const CName storyPhase = agentStub->GetActivePhaseName();
				m_communityStoryPhase->SetText(String::Printf( TXT("Story phase: %ls"), (storyPhase == CName::NONE) ? TXT("Default") : storyPhase.AsString().AsChar() ) );

				// Community
				m_communitySpawnsetName->SetText(String::Printf( TXT("Spawnset name: %ls"), agentStub->GetSpawnsetName().AsChar()) );
				m_generalOpenSpawnset->SetOutOfDate();

				// Agent stub state
				m_communityCurrentStubState->SetText( String::Printf(TXT("Current stub state: %ls"), CCommunityUtility::GetFriendlyAgentStateName( agentStub->m_state ).AsChar()) );
			}

			// NPC Action Schedule
			const NewNPCScheduleProxy &npcSchedule = npc->GetActionSchedule();
			if ( npcSchedule.GetActiveAP() != ActionPointBadID )
			{
				m_communityActiveAP->SetText( String::Printf(TXT("Active AP: %ls"), GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( npcSchedule.GetActiveAP() ).AsChar()) );
				m_generalOpenAP->SetOutOfDate();
			}
			else
			{
				m_communityActiveAP->SetText(String::Printf(TXT("Active AP is EMPTY")) );
			}
			if ( npcSchedule.GetLastAP() != ActionPointBadID )
			{
				m_communityLastAP->SetText( String::Printf(TXT("Last AP: %ls"), GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( npcSchedule.GetLastAP() ).AsChar()) );
			}
			else
			{
				m_communityLastAP->SetText(String::Printf(TXT("Last AP is EMPTY")) );
			}
			//
			m_communityUsingLastAP->SetText( String::Printf(TXT("Is using last AP: %ls"), npcSchedule.UsesLastAP() ? TXT("true") : TXT("false")) );

			if ( npc->IsWorkingInAP() && npc->GetCurrentActionPoint() != ActionPointBadID )
			{
				m_communityWorkingInAP->SetText( String::Printf(TXT("Is working in AP: %ls"), GCommonGame->GetSystem< CCommunitySystem >()->GetActionPointManager()->GetFriendlyAPName( npc->GetCurrentActionPoint() ).AsChar()) );
			}
			else
			{
				m_communityWorkingInAP->SetText( TXT("Is not working" ) );
			}
		}
	}

	void CDebugWindowNPCViewer::SetInventoryInformation( CNewNPC* npc )
	{
		m_inventoryItemList->RemoveAllItems();

		// Get inventory component
		CInventoryComponent* ic = npc->GetInventoryComponent();
		if ( ic != nullptr)
		{
			m_inventoryState->SetText(TXT("Inventory state: NPC has a inventory component"));

			// inventory item count
			const auto & items = ic->GetItems();
			m_inventorySize->SetText( String::Printf( TXT("Item count: %ls"), ( items.Empty() == true ) ? TXT("Empty") : ToString( items.Size() ).AsChar() ) );

			// item list
			for ( auto it=items.Begin(); it!=items.End(); ++it)
			{
				const SInventoryItem& item = *it;

				// Item info
				String text = String::Printf( TXT("%ls"), item.GetName().AsString().AsChar() );
				if ( item.IsHeld() == true ) text += TXT("     |     HELD");
				if ( item.IsMounted() == true ) text += TXT("     |     MOUNTED");
				if ( item.IsCloned() == true ) text += TXT("     |     CLONED");
				m_inventoryItemList->AddItem(text);
			}
		}
		else
		{
			m_inventoryState->SetText(TXT("Inventory state: No inventory component"));
			m_inventorySize->ClearText();
		}
	}

	void CDebugWindowNPCViewer::SetMovementInformation( CNewNPC* npc )
	{
		m_movementLocomotionLines->RemoveAllItems();

		// get moving agent component
		CMovingAgentComponent* mac = npc->GetMovingAgentComponent();
		if ( mac != nullptr)
		{
			m_movementHasComponent->SetText(TXT("Movement component: NPC has a movement component"));

			// movement component state
			m_movementState->SetText(String::Printf(TXT("Movement component state: %ls"), (mac->IsEnabled() == true) ? TXT("ENABLED") : TXT("DISABLED") ) );

			// teleport info
			const Vector& teleportPos = mac->GetTeleportedPosition();
			const EulerAngles& teleportRot = mac->GetTeleportedRotation();

			m_movementLastTeleport->SetText(String::Printf(
				TXT( "Last teleport: pos( %.2f, %.2f, %.2f ), rot( %.2f, %.2f, %.2f )" ),
				teleportPos.X, teleportPos.Y, teleportPos.Z,
				teleportRot.Yaw, teleportRot.Pitch, teleportRot.Roll ));
			m_movementTeleportPlayerToLastPoint->SetOutOfDate();

			// top representation
			m_movementTopRepresentation->SetText(String::Printf( TXT( "Active representation: %ls" ), mac->GetActiveRepresentationName().AsChar() ));

			// physical agent component
			CMovingPhysicalAgentComponent* physicalMac = Cast< CMovingPhysicalAgentComponent >( mac );
			if ( physicalMac != nullptr )
			{
				InteractionPriorityType playerInteractionPriority = InteractionPriorityTypeZero;

				CMovingPhysicalAgentComponent* playerPhysicalMac = nullptr;
				if ( GCommonGame != nullptr && GCommonGame->GetPlayer() != nullptr && GCommonGame->GetPlayer()->GetMovingAgentComponent() != nullptr )
				{
					playerPhysicalMac = Cast< CMovingPhysicalAgentComponent >( GCommonGame->GetPlayer()->GetMovingAgentComponent() );
				}

				if ( playerPhysicalMac != nullptr )
				{
					playerInteractionPriority = playerPhysicalMac->GetActualInteractionPriority();
				}

				m_movementPriority->SetText(String::Printf( TXT( "Character movement priority: %d (Player: %d) " ), physicalMac->GetActualInteractionPriority(), playerInteractionPriority ) );
			}

			// static rotation
			const CStaticMovementTargeter* staticTarget = mac->GetStaticTarget();
			if( staticTarget != nullptr)
			{
				m_movementStaticRotation->SetText(String::Printf( TXT( "Static rotation is: %ls" ), (staticTarget->IsRotationTargetSet() == true) ? TXT("SET") : TXT("NOT SET")) );
			}

			// locomotion lines
			m_movementLocomotionLines->RemoveAllItems();
			TDynArray< String > lines;
			mac->GetLocoDebugInfo( lines );
			m_movementLocomotionLines->AddItems( lines );
		}
		else
		{
			m_movementHasComponent->SetText(TXT("Movement component: Absence"));
			m_movementState->ClearText();
			m_movementLastTeleport->ClearText();
			m_movementTopRepresentation->ClearText();
			m_movementPriority->ClearText();
			m_movementQueuePosition->ClearText();
			m_movementStaticRotation->ClearText();
		}
	}

	void CDebugWindowNPCViewer::SetQuestInformation( CNewNPC* npc )
	{
		m_questLocksList->RemoveAllItems();

		// Quest locks
		TDynArray<String> infoList;
		const TDynArray< NPCQuestLock* >& locks = npc->GetQuestLockHistory();
		for ( Uint32 i=0; i<locks.Size(); ++i )
		{
			NPCQuestLock* lock = locks[i];

			// get info about lock
			String info;
			(lock->GetLockState() == true) ? info += TXT("LOCK   - ") : TXT("UNLOCK - ");
			info += lock->GetQuestBlock()->GetCaption().AsChar();
			info += TXT(" in ");
			info += lock->GetQuestPhase()->GetFriendlyName().AsChar();

			// add to container
			infoList.PushBack(info);
		}

		// fill list
		m_questLocksList->AddItems( infoList );
	}

	void CDebugWindowNPCViewer::SetNoticedInformation( CNewNPC* npc )
	{
		m_noticedObjectsList->RemoveAllItems();

		const TDynArray< NewNPCNoticedObject >& noticedObjects = npc->GetNoticedObjects();
		for ( TDynArray< NewNPCNoticedObject >::const_iterator noticedObj = noticedObjects.Begin(); noticedObj != noticedObjects.End(); ++noticedObj )
		{
			m_noticedObjectsList->AddItem(noticedObj->ToString());
		}
	}

	void CDebugWindowNPCViewer::SetAttitudesInformation( CNewNPC* npc )
	{
		m_attitudesState->SetText( String::Printf( TXT( "NPC attitude state: %ls" ), ToString( npc->GetAttitude( GCommonGame->GetPlayer() ) ).AsChar() ) );
		m_attitudesList->RemoveAllItems();

		TActorAttitudeMap attMap;
		npc->GetAttitudeMap( attMap );
		TActorAttitudeMap::const_iterator iter = attMap.Begin();
		String name, attitude;
		CEnum* attEnum = SRTTI::GetInstance().FindEnum( CNAME( EAIAttitude ) );
		ASSERT( attEnum );
		for( ; iter!= attMap.End(); ++iter )
		{
			if ( iter->m_first != nullptr )
				name = iter->m_first->GetName();
			else
				name = TXT("nullptr");

			attitude.Clear();
			Bool res = attEnum->ToString( &(iter->m_second), attitude );
			ASSERT( res );
			m_attitudesList->AddItem(String::Printf( TXT("Actor: %ls, attitude: %ls"), name.AsChar(), attitude.AsChar() ));
		}
	}

	void CDebugWindowNPCViewer::SetMeshInformation( CNewNPC* npc )
	{
		m_meshesList->RemoveAllItems();

		const TDynArray< CComponent* >& components = npc->GetComponents();
		for ( Uint32 i = 0; i < components.Size(); ++i )
		{
			if ( components[i]->IsA<CMeshComponent>() == true)
			{
				String debugText;
				if ( ((CMeshComponent*)components[i])->GetMeshNow() != nullptr )
				{
					debugText = static_cast< CMeshComponent* >(components[i])->GetMeshNow()->GetDepotPath();
				}
				else
				{
					debugText = TXT("Unknown");
				}

				m_meshesList->AddItem( debugText, static_cast< CMeshComponent* >(components[i])->IsVisible() ? Color::GREEN : Color::RED );
			}
		}
	}

	void CDebugWindowNPCViewer::SetStoryInformation( CNewNPC* npc )
	{
		m_storyQuestList->RemoveAllItems();

		const TDynArray< NPCQuestLock* >& quests = npc->GetQuestLockHistory();
		for( Uint32 i=0; i<quests.Size(); ++i)
		{
			if(quests[i]->GetLockState() == true)
			{
				CQuestPhase* questPhase = quests[i]->GetQuestPhase();
				CQuestGraphBlock* block =  quests[i]->GetQuestBlock();

				m_storyQuestList->AddItem(String::Printf( TXT("[%i]: %ls"), i, questPhase->GetFriendlyName().AsChar() ) );
			}
		}
	}

	void CDebugWindowNPCViewer::NotifyEventToggleButtonChange( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if(sender == m_stopRotate)
		{
			m_pauseRotation = value;

			if(value == true)
			{
				// disable free camera
				m_releaseCamera->SetToggleValue(false);

				m_stopRotate->SetText(TXT("Resume rotation"));
			}
			else
			{
				m_stopRotate->SetText(TXT("Pause rotation"));
			}
		}
		else if(sender == m_releaseCamera)
		{
			if(value == true)
			{
				// disable rotation
				m_pauseRotation = true;
				m_stopRotate->SetText(TXT("Resume rotation"));
				m_stopRotate->SetToggleValue(true);

				// release camera
				GGame->EnableFreeCamera(true);
				EulerAngles rot( 0.0f, 0.0f, m_cameraRotation );
				CGameFreeCamera& freeCamera = const_cast<CGameFreeCamera&>(GGame->GetFreeCamera());
				freeCamera.MoveTo( m_cameraPosition, rot);
				m_releaseCamera->SetText(TXT("Attach camera"));
			}
			else
			{
				// attach camera to player
				GGame->EnableFreeCamera(false);
				m_releaseCamera->SetText(TXT("Release camera"));
			}
		}
	}

	void CDebugWindowNPCViewer::NotifyMouseButtonDoubleClickOnMeshComponent( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

#ifndef NO_EDITOR_EVENT_SYSTEM
		String meshPath = m_meshesList->GetItemText( value );
		SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( meshPath ) );
#endif	// NO_EDITOR_EVENT_SYSTEM
	}

	void CDebugWindowNPCViewer::NotifyEventButtonClickedOnOpenEntity( RedGui::CRedGuiEventPackage& eventPackage )
	{
#ifndef NO_EDITOR_EVENT_SYSTEM
		CNewNPC* npc = m_active.Get();
		if( npc != nullptr )
		{
			CEntityTemplate* entityTemp = npc->GetEntityTemplate();
			if( entityTemp != nullptr )
			{
				String templatePath = entityTemp->GetDepotPath();
				SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( templatePath ) );
			}
		}
#endif	// NO_EDITOR_EVENT_SYSTEM
	}

	void CDebugWindowNPCViewer::NotifyEventButtonClickedOnTeleportPlayerToLastPoint( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CNewNPC* npc = m_active.Get();
			if ( npc != nullptr )
			{
				// get moving agent component
				CMovingAgentComponent* mac = npc->GetMovingAgentComponent();
				if ( mac != nullptr)
				{
					const Vector& teleportPos = mac->GetTeleportedPosition();
					const EulerAngles& teleportRot = mac->GetTeleportedRotation();
	
					// teleport player to last teleport point for selected NPC
					if( GCommonGame->GetPlayer() != nullptr )
					{
						GCommonGame->GetPlayer()->Teleport(teleportPos, teleportRot);
	
						m_teleportPlayer = true;
						GGame->UnpauseAll();
					}
					else
					{
						GRedGui::GetInstance().MessageBox( TXT("On the scene there is no player."), TXT("Warning"), RedGui::MESSAGEBOX_Warning );
					}
				}
			}
		}

	CNewNPC* CDebugWindowNPCViewer::GetNPCFromIndex( Uint32 index )
	{
		Int32 selectedIndex = -1;
		RedGui::CRedGuiListItem* listItem = nullptr;

		if( m_npcTab->GetActiveTabIndex() == STT_Community )
		{
			listItem =  m_communityNPCList->GetItem( index );
		}
		else if( m_npcTab->GetActiveTabIndex() == STT_Encounter )
		{
			listItem =  m_encounterNPCList->GetItem( index );
		}
		else if( m_npcTab->GetActiveTabIndex() == STT_Spawner )
		{
			listItem =  m_spawnersNPCList->GetItem( index );
		}

		if( listItem != nullptr )
		{
			FromString< Int32 >( listItem->GetUserString( TXT("Index") ), selectedIndex );
			if( selectedIndex != -1 )
			{
				if( selectedIndex < (Int32)m_allNpcs.Size() )
				{
					return m_allNpcs[selectedIndex].Get();
				}
			}
		}

		return nullptr;
	}

	void CDebugWindowNPCViewer::CreateGeneralTab()
	{
		CRedGuiControl* generalTab = m_npcInfoCategories->GetTabAt( IVTT_General );
		if(generalTab != nullptr)
		{
			RedGui::CRedGuiGroupBox* generalGroupBox = new RedGui::CRedGuiGroupBox(0, 0, 100, 230 );
			generalTab->AddChild(generalGroupBox);
			generalGroupBox->SetDock(RedGui::DOCK_Top);
			generalGroupBox->SetText( TXT("General") );
			if( generalGroupBox != nullptr )
			{
				RedGui::CRedGuiPanel* panel = new RedGui::CRedGuiPanel(0, 0, 100, 15);
				panel->SetBorderVisible(false);
				panel->SetMargin(Box2(10, 10, 10, 3));
				generalGroupBox->AddChild(panel);
				panel->SetDock(RedGui::DOCK_Top);
				{
					m_generalTemplateName = new RedGui::CRedGuiLabel(0,0,0,0);
					panel->AddChild(m_generalTemplateName);
					m_generalTemplateName->SetText(TXT("Template: "));
					m_generalTemplateName->SetDock(RedGui::DOCK_Left);

					m_generalOpenEntityTemplate = new RedGui::CRedGuiButton(0,0, 75, 10);
					panel->AddChild(m_generalOpenEntityTemplate);
					m_generalOpenEntityTemplate->SetText( TXT("Open"));
					m_generalOpenEntityTemplate->SetMargin(Box2(10, 0, 0, 0));
					m_generalOpenEntityTemplate->SetDock(RedGui::DOCK_Left);
					m_generalOpenEntityTemplate->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyEventButtonClickedOnOpenEntity);
				}

				m_generalMemoryUsageByObject = new RedGui::CRedGuiLabel(0,0,0,0);
				generalGroupBox->AddChild(m_generalMemoryUsageByObject);
				m_generalMemoryUsageByObject->SetMargin(Box2(10, 3, 10, 3));
				m_generalMemoryUsageByObject->SetText(TXT("Memory usage by object: "));
				m_generalMemoryUsageByObject->SetDock(RedGui::DOCK_Top);

				m_generalMemoryUsageByTemplate = new RedGui::CRedGuiLabel(0,0,0,0);
				generalGroupBox->AddChild(m_generalMemoryUsageByTemplate);
				m_generalMemoryUsageByTemplate->SetMargin(Box2(10, 3, 10, 3));
				m_generalMemoryUsageByTemplate->SetText(TXT("Memory usage by template: "));
				m_generalMemoryUsageByTemplate->SetDock(RedGui::DOCK_Top);

				m_generalTags = new RedGui::CRedGuiLabel(0,0,0,0);
				generalGroupBox->AddChild(m_generalTags);
				m_generalTags->SetMargin(Box2(10, 3, 10, 3));
				m_generalTags->SetText(TXT("Tags: "));
				m_generalTags->SetDock(RedGui::DOCK_Top);

				m_generalAppearance = new RedGui::CRedGuiLabel(0,0,0,0);
				generalGroupBox->AddChild(m_generalAppearance);
				m_generalAppearance->SetMargin(Box2(10, 3, 10, 3));
				m_generalAppearance->SetText(TXT("Appearance: "));
				m_generalAppearance->SetDock(RedGui::DOCK_Top);

				m_generalVoiceTag = new RedGui::CRedGuiLabel(0,0,0,0);
				generalGroupBox->AddChild(m_generalVoiceTag);
				m_generalVoiceTag->SetMargin(Box2(10, 3, 10, 3));
				m_generalVoiceTag->SetText(TXT("VoiceTag: "));
				m_generalVoiceTag->SetDock(RedGui::DOCK_Top);

				m_generalProfileRecursive = new RedGui::CRedGuiLabel(0,0,0,0);
				generalGroupBox->AddChild(m_generalProfileRecursive);
				m_generalProfileRecursive->SetMargin(Box2(10, 3, 10, 3));
				m_generalProfileRecursive->SetText(TXT("Attitude group: "));
				m_generalProfileRecursive->SetDock(RedGui::DOCK_Top);

				m_generalIsAlive = new RedGui::CRedGuiLabel( 0, 0, 0, 0 );
				generalGroupBox->AddChild( m_generalIsAlive );
				m_generalIsAlive->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_generalIsAlive->SetText( TXT("Is alive: ") );
				m_generalIsAlive->SetDock( RedGui::DOCK_Top );

				m_generalBehMachine = new RedGui::CRedGuiLabel( 0, 0, 0, 0 );
				m_generalBehMachine->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_generalBehMachine->SetText( TXT("Beh Machine Info: ") );
				m_generalBehMachine->SetDock( RedGui::DOCK_Top );
				generalGroupBox->AddChild( m_generalBehMachine );

				m_generalEncounter = new RedGui::CRedGuiLabel( 0, 0, 0, 0 );
				m_generalEncounter->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_generalEncounter->SetText( TXT("Encounter: ") );
				m_generalEncounter->SetDock( RedGui::DOCK_Top );
				generalGroupBox->AddChild( m_generalEncounter );
				
				m_generalEncounterActiveEntry = new RedGui::CRedGuiLabel( 0, 0, 0, 0 );
				m_generalEncounterActiveEntry->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_generalEncounterActiveEntry->SetText( TXT("Encounter active entry: ") );
				m_generalEncounterActiveEntry->SetDock( RedGui::DOCK_Top );
				generalGroupBox->AddChild( m_generalEncounterActiveEntry );
			}

			RedGui::CRedGuiGroupBox* scriptGroupBox = new RedGui::CRedGuiGroupBox(0, 0, 100, 70);
			generalTab->AddChild(scriptGroupBox);
			scriptGroupBox->SetDock(RedGui::DOCK_Top);
			scriptGroupBox->SetText( TXT("Script") );
			if( scriptGroupBox != nullptr )
			{
				m_scriptStateName = new RedGui::CRedGuiLabel(0,0,0,0);
				scriptGroupBox->AddChild(m_scriptStateName);
				m_scriptStateName->SetMargin(Box2(10, 10, 10, 3));
				m_scriptStateName->SetText(TXT("State: "));
				m_scriptStateName->SetDock(RedGui::DOCK_Top);

				m_scriptTopFunction = new RedGui::CRedGuiLabel(0,0,0,0);
				scriptGroupBox->AddChild(m_scriptTopFunction);
				m_scriptTopFunction->SetMargin(Box2(10, 3, 10, 3));
				m_scriptTopFunction->SetText(TXT("Function: "));
				m_scriptTopFunction->SetDock(RedGui::DOCK_Top);
			}

			RedGui::CRedGuiGroupBox* communityGroupBox = new RedGui::CRedGuiGroupBox(0, 0, 100, 250);
			generalTab->AddChild(communityGroupBox);
			communityGroupBox->SetDock(RedGui::DOCK_Top);
			communityGroupBox->SetText( TXT("Community") );
			if( communityGroupBox != nullptr )
			{
				m_communityIsInCommunity = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityIsInCommunity);
				m_communityIsInCommunity->SetMargin(Box2(10, 10, 10, 3));
				m_communityIsInCommunity->SetText(TXT("Is NPC in Community: "));
				m_communityIsInCommunity->SetDock(RedGui::DOCK_Top);

				m_communityStoryPhase = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityStoryPhase);
				m_communityStoryPhase->SetMargin(Box2(10, 3, 10, 3));
				m_communityStoryPhase->SetText(TXT("Story phase: "));
				m_communityStoryPhase->SetDock(RedGui::DOCK_Top);

				RedGui::CRedGuiPanel* spawnsetPanel = new RedGui::CRedGuiPanel(0, 0, 200, 15);
				spawnsetPanel->SetMargin(Box2(10, 3, 10, 3));
				spawnsetPanel->SetBorderVisible( false );
				communityGroupBox->AddChild( spawnsetPanel );
				spawnsetPanel->SetDock( RedGui::DOCK_Top );
				{
					m_communitySpawnsetName = new RedGui::CRedGuiLabel(0,0,0,0);
					spawnsetPanel->AddChild( m_communitySpawnsetName );
					m_communitySpawnsetName->SetText( TXT("Spawnset name: ") );
					m_communitySpawnsetName->SetDock(RedGui::DOCK_Left);

					m_generalOpenSpawnset = new RedGui::CRedGuiButton(0,0, 75, 10 );
					spawnsetPanel->AddChild( m_generalOpenSpawnset );
					m_generalOpenSpawnset->SetText( TXT("Open"));
					m_generalOpenSpawnset->SetMargin( Box2(10, 0, 0, 0) );
					m_generalOpenSpawnset->SetDock( RedGui::DOCK_Left );
					m_generalOpenSpawnset->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyOpenSpawnset );
				}

				m_communityCurrentStubState = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityCurrentStubState);
				m_communityCurrentStubState->SetMargin(Box2(10, 3, 10, 3));
				m_communityCurrentStubState->SetText(TXT("Current stub state: "));
				m_communityCurrentStubState->SetDock(RedGui::DOCK_Top);

				RedGui::CRedGuiPanel* activeAPPanel = new RedGui::CRedGuiPanel(0, 0, 200, 15);
				activeAPPanel->SetMargin(Box2(10, 10, 10, 3));
				activeAPPanel->SetBorderVisible( false );
				communityGroupBox->AddChild( activeAPPanel );
				activeAPPanel->SetDock( RedGui::DOCK_Top );
				{
					m_communityActiveAP = new RedGui::CRedGuiLabel( 0, 0, 0, 0 );
					activeAPPanel->AddChild( m_communityActiveAP );
					m_communityActiveAP->SetText( TXT("Active AP: ") );
					m_communityActiveAP->SetDock( RedGui::DOCK_Left );

					m_generalOpenAP = new RedGui::CRedGuiButton(0,0, 75, 10 );
					activeAPPanel->AddChild( m_generalOpenAP );
					m_generalOpenAP->SetText( TXT("Open"));
					m_generalOpenAP->SetMargin( Box2(10, 0, 0, 0) );
					m_generalOpenAP->SetDock( RedGui::DOCK_Left );
					m_generalOpenAP->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyOpenActiveAP );
				}

				m_communityLastAP = new RedGui::CRedGuiLabel( 0, 0, 0, 0 );
				communityGroupBox->AddChild( m_communityLastAP );
				m_communityLastAP->SetText( TXT("Last AP: ") );
				m_communityLastAP->SetDock( RedGui::DOCK_Top );

				m_communityUsingLastAP = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityUsingLastAP);
				m_communityUsingLastAP->SetMargin(Box2(10, 3, 10, 3));
				m_communityUsingLastAP->SetText(TXT("Is using last AP: "));
				m_communityUsingLastAP->SetDock(RedGui::DOCK_Top);

				m_communityWorkingInAP = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild( m_communityWorkingInAP );
				m_communityWorkingInAP->SetMargin(Box2(10, 3, 10, 3));
				m_communityWorkingInAP->SetText( TXT("Is not working"));
				m_communityWorkingInAP->SetDock(RedGui::DOCK_Top);

				m_communityWorkState = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityWorkState);
				m_communityWorkState->SetMargin(Box2(10, 3, 10, 3));
				m_communityWorkState->SetText(TXT("Work state: "));
				m_communityWorkState->SetDock(RedGui::DOCK_Top);

				m_communityIsInfinite = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityIsInfinite);
				m_communityIsInfinite->SetMargin(Box2(10, 3, 10, 3));
				m_communityIsInfinite->SetText(TXT("Is infinite: "));
				m_communityIsInfinite->SetDock(RedGui::DOCK_Top);

				m_communityExecutionMode = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityExecutionMode);
				m_communityExecutionMode->SetMargin(Box2(10, 3, 10, 3));
				m_communityExecutionMode->SetText(TXT("Execution mode: "));
				m_communityExecutionMode->SetDock(RedGui::DOCK_Top);

				m_communityAPName = new RedGui::CRedGuiLabel(0,0,0,0);
				communityGroupBox->AddChild(m_communityAPName);
				m_communityAPName->SetMargin(Box2(10, 3, 10, 3));
				m_communityAPName->SetText(TXT("AP Name: "));
				m_communityAPName->SetDock(RedGui::DOCK_Top);
			}
		}

		RedGui::CRedGuiGroupBox* actionsGroupBox = new RedGui::CRedGuiGroupBox(0, 0, 100, 55);
		generalTab->AddChild(actionsGroupBox);
		actionsGroupBox->SetDock(RedGui::DOCK_Top);
		actionsGroupBox->SetText( TXT("Fast actions") );
		if( actionsGroupBox != nullptr )
		{
			RedGui::CRedGuiButton* killNPC = new RedGui::CRedGuiButton( 0, 0, 0, 0);
			actionsGroupBox->AddChild( killNPC );
			killNPC->SetMargin( Box2( 5, 5, 5, 5 ) );
			killNPC->SetText( TXT("Kill NPC") );
			killNPC->SetDock( RedGui::DOCK_Left );
			killNPC->SetFitToText( true );
			killNPC->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyKillNPC );

			RedGui::CRedGuiButton* stunNPC = new RedGui::CRedGuiButton( 0, 0, 0, 0);
			actionsGroupBox->AddChild( stunNPC );
			stunNPC->SetMargin( Box2( 5, 5, 5, 5 ) );
			stunNPC->SetText( TXT("Stun NPC") );
			stunNPC->SetDock( RedGui::DOCK_Left );
			stunNPC->SetFitToText( true );
			stunNPC->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyStunNPC );

			RedGui::CRedGuiButton* destroyNPC = new RedGui::CRedGuiButton( 0, 0, 0, 0);
			actionsGroupBox->AddChild( destroyNPC );
			destroyNPC->SetMargin( Box2( 5, 5, 5, 5 ) );
			destroyNPC->SetText( TXT("Destroy NPC") );
			destroyNPC->SetDock( RedGui::DOCK_Left );
			destroyNPC->SetFitToText( true );
			destroyNPC->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyDestroyNPC );
		}
	}

	void CDebugWindowNPCViewer::CreateStatsTab()
	{
		CRedGuiControl* statsTab = m_npcInfoCategories->GetTabAt( IVTT_Stats );
		if( statsTab != nullptr )
		{
			RedGui::CRedGuiGroupBox* baseStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 260 );
			statsTab->AddChild( baseStatsGroupBox );
			baseStatsGroupBox->SetDock( RedGui::DOCK_Top );
			baseStatsGroupBox->SetText( TXT("Base stats") );
			if( baseStatsGroupBox != nullptr )
			{
				// BCS_Vitality
				RedGui::CRedGuiPanel* vitalityPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				vitalityPanel->SetBorderVisible( false );
				vitalityPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( vitalityPanel );
				vitalityPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* vitalityLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					vitalityPanel->AddChild( vitalityLabel );
					vitalityLabel->SetAutoSize( false );
					vitalityLabel->SetText( TXT("Vitality: ") );
					vitalityLabel->SetDock( RedGui::DOCK_Left );

					m_statVitality = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					vitalityPanel->AddChild( m_statVitality );
					m_statVitality->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statVitality->SetDock( RedGui::DOCK_Left );
					m_statVitality->SetShowProgressInformation( true );
					m_statVitality->SetProgressRange( 100 );
					m_statVitality->SetProgressPosition( 0 );
					m_statVitality->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Essence
				RedGui::CRedGuiPanel* essencePanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				essencePanel->SetBorderVisible( false );
				essencePanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( essencePanel );
				essencePanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* essenceLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					essencePanel->AddChild( essenceLabel );
					essenceLabel->SetAutoSize( false );
					essenceLabel->SetText( TXT("Essence: ") );
					essenceLabel->SetDock( RedGui::DOCK_Left );

					m_statEssence = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					essencePanel->AddChild( m_statEssence );
					m_statEssence->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statEssence->SetDock( RedGui::DOCK_Left );
					m_statEssence->SetShowProgressInformation( true );
					m_statEssence->SetProgressRange( 100 );
					m_statEssence->SetProgressPosition( 0 );
					m_statEssence->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Stamina
				RedGui::CRedGuiPanel* staminaPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				staminaPanel->SetBorderVisible( false );
				staminaPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( staminaPanel );
				staminaPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* staminaLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					staminaPanel->AddChild( staminaLabel );
					staminaLabel->SetAutoSize( false );
					staminaLabel->SetText( TXT("Stamina: ") );
					staminaLabel->SetDock( RedGui::DOCK_Left );

					m_statStamina = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					staminaPanel->AddChild( m_statStamina );
					m_statStamina->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statStamina->SetDock( RedGui::DOCK_Left );
					m_statStamina->SetShowProgressInformation( true );
					m_statStamina->SetProgressRange( 100 );
					m_statStamina->SetProgressPosition( 0 );
					m_statStamina->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Toxicity
				RedGui::CRedGuiPanel* toxicityPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				toxicityPanel->SetBorderVisible( false );
				toxicityPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( toxicityPanel );
				toxicityPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* toxicityLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					toxicityPanel->AddChild( toxicityLabel );
					toxicityLabel->SetAutoSize( false );
					toxicityLabel->SetText( TXT("Toxicity: ") );
					toxicityLabel->SetDock( RedGui::DOCK_Left );

					m_statToxicity = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					toxicityPanel->AddChild( m_statToxicity );
					m_statToxicity->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statToxicity->SetDock( RedGui::DOCK_Left );
					m_statToxicity->SetShowProgressInformation( true );
					m_statToxicity->SetProgressRange( 100 );
					m_statToxicity->SetProgressPosition( 0 );
					m_statToxicity->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Focus
				RedGui::CRedGuiPanel* focusPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				focusPanel->SetBorderVisible( false );
				focusPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( focusPanel );
				focusPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* focusLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					focusPanel->AddChild( focusLabel );
					focusLabel->SetAutoSize( false );
					focusLabel->SetText( TXT("Focus: ") );
					focusLabel->SetDock( RedGui::DOCK_Left );

					m_statFocus = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					focusPanel->AddChild( m_statFocus );
					m_statFocus->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statFocus->SetDock( RedGui::DOCK_Left );
					m_statFocus->SetShowProgressInformation( true );
					m_statFocus->SetProgressRange( 100 );
					m_statFocus->SetProgressPosition( 0 );
					m_statFocus->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Morale
				RedGui::CRedGuiPanel* moralePanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				moralePanel->SetBorderVisible( false );
				moralePanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( moralePanel );
				moralePanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* moraleLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					moralePanel->AddChild( moraleLabel );
					moraleLabel->SetAutoSize( false );
					moraleLabel->SetText( TXT("Morale: ") );
					moraleLabel->SetDock( RedGui::DOCK_Left );

					m_statMorale = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					moralePanel->AddChild( m_statMorale );
					m_statMorale->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statMorale->SetDock( RedGui::DOCK_Left );
					m_statMorale->SetShowProgressInformation( true );
					m_statMorale->SetProgressRange( 100 );
					m_statMorale->SetProgressPosition( 0 );
					m_statMorale->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Drunkenness
				RedGui::CRedGuiPanel* drunkennessPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				drunkennessPanel->SetBorderVisible( false );
				drunkennessPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( drunkennessPanel );
				drunkennessPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* drunkennessLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					drunkennessPanel->AddChild( drunkennessLabel );
					drunkennessLabel->SetAutoSize( false );
					drunkennessLabel->SetText( TXT("Drunkenness: ") );
					drunkennessLabel->SetDock( RedGui::DOCK_Left );

					m_statDrunkenness = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					drunkennessPanel->AddChild( m_statDrunkenness );
					m_statDrunkenness->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statDrunkenness->SetDock( RedGui::DOCK_Left );
					m_statDrunkenness->SetShowProgressInformation( true );
					m_statDrunkenness->SetProgressRange( 100 );
					m_statDrunkenness->SetProgressPosition( 0 );
					m_statDrunkenness->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// BCS_Air
				RedGui::CRedGuiPanel* airPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				airPanel->SetBorderVisible( false );
				airPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				baseStatsGroupBox->AddChild( airPanel );
				airPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* airLabel= new RedGui::CRedGuiLabel(0,0,150,20);
					airPanel->AddChild( airLabel );
					airLabel->SetAutoSize( false );
					airLabel->SetText( TXT("Air: ") );
					airLabel->SetDock( RedGui::DOCK_Left );

					m_statAir = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					airPanel->AddChild( m_statAir );
					m_statAir->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statAir->SetDock( RedGui::DOCK_Left );
					m_statAir->SetShowProgressInformation( true );
					m_statAir->SetProgressRange( 100 );
					m_statAir->SetProgressPosition( 0 );
					m_statAir->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}
			}

			RedGui::CRedGuiGroupBox* powerStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 110 );
			statsTab->AddChild( powerStatsGroupBox );
			powerStatsGroupBox->SetDock( RedGui::DOCK_Top );
			powerStatsGroupBox->SetText( TXT("Power stats") );
			if( powerStatsGroupBox != nullptr )
			{
				// Attack power
				m_statAttackPower = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				powerStatsGroupBox->AddChild( m_statAttackPower );
				m_statAttackPower->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statAttackPower->SetDock( RedGui::DOCK_Top );
				m_statAttackPower->SetText( TXT("Attack power:          Base = , Mult = , Add = ") );

				// Spell power
				m_statSpellPower = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				powerStatsGroupBox->AddChild( m_statSpellPower );
				m_statSpellPower->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statSpellPower->SetDock( RedGui::DOCK_Top );
				m_statSpellPower->SetText( TXT("Spell power:		Base = , Mult = , Add = ") );

				// Dabage bonus
				m_statDamageBonus = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				powerStatsGroupBox->AddChild( m_statDamageBonus );
				m_statDamageBonus->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statDamageBonus->SetDock( RedGui::DOCK_Top );
				m_statDamageBonus->SetText( TXT("Damage bonus:		Base = , Mult = , Add = ") );

			}

			RedGui::CRedGuiGroupBox* regenStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 210 );
			statsTab->AddChild( regenStatsGroupBox );
			regenStatsGroupBox->SetDock( RedGui::DOCK_Top );
			regenStatsGroupBox->SetText( TXT("Regen stats") );
			if( regenStatsGroupBox != nullptr )
			{
				// Attack power
				m_statVitalityRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statVitalityRegen );
				m_statVitalityRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statVitalityRegen->SetDock( RedGui::DOCK_Top );
				m_statVitalityRegen->SetText( TXT("Vitality:		Base = , Mult = , Add = ") );

				// Attack power
				m_statEssenceRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statEssenceRegen );
				m_statEssenceRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statEssenceRegen->SetDock( RedGui::DOCK_Top );
				m_statEssenceRegen->SetText( TXT("Essence:		Base = , Mult = , Add = ") );

				// Attack power
				m_statMoraleRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statMoraleRegen );
				m_statMoraleRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statMoraleRegen->SetDock( RedGui::DOCK_Top );
				m_statMoraleRegen->SetText( TXT("Morale:		Base = , Mult = , Add = ") );

				// Attack power
				m_statToxicityRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statToxicityRegen );
				m_statToxicityRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statToxicityRegen->SetDock( RedGui::DOCK_Top );
				m_statToxicityRegen->SetText( TXT("Toxicity:		Base = , Mult = , Add = ") );

				// Attack power
				m_statDrunkennessRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statDrunkennessRegen );
				m_statDrunkennessRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statDrunkennessRegen->SetDock( RedGui::DOCK_Top );
				m_statDrunkennessRegen->SetText( TXT("Drunkenness:		Base = , Mult = , Add = ") );

				// Attack power
				m_statStaminaRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statStaminaRegen );
				m_statStaminaRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statStaminaRegen->SetDock( RedGui::DOCK_Top );
				m_statStaminaRegen->SetText( TXT("Stamina:		Base = , Mult = , Add = ") );

				// Attack power
				m_statAirRegen = new RedGui::CRedGuiLabel( 0, 0, 100, 350 );
				regenStatsGroupBox->AddChild( m_statAirRegen );
				m_statAirRegen->SetMargin( Box2( 10, 10, 10, 3 ) );
				m_statAirRegen->SetDock( RedGui::DOCK_Top );
				m_statAirRegen->SetText( TXT("Air:		Base = , Mult = , Add = ") );
			}

			RedGui::CRedGuiGroupBox* defenceStatsGroupBox = new RedGui::CRedGuiGroupBox( 0, 0, 100, 350 );
			statsTab->AddChild( defenceStatsGroupBox );
			defenceStatsGroupBox->SetDock( RedGui::DOCK_Top );
			defenceStatsGroupBox->SetText( TXT("Defence stats") );
			if( defenceStatsGroupBox != nullptr )
			{
				// CDS_None
				RedGui::CRedGuiPanel* noneResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				noneResPanel->SetBorderVisible( false );
				noneResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( noneResPanel );
				noneResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* noneLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					noneResPanel->AddChild( noneLabel );
					noneLabel->SetAutoSize( false );
					noneLabel->SetText( TXT("None: ") );
					noneLabel->SetDock( RedGui::DOCK_Left );

					m_statNoneRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					noneResPanel->AddChild( m_statNoneRes );
					m_statNoneRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statNoneRes->SetDock( RedGui::DOCK_Left );
					m_statNoneRes->SetShowProgressInformation( true );
					m_statNoneRes->SetProgressRange( 100 );
					m_statNoneRes->SetProgressPosition( 0 );
					m_statNoneRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_PhysicalRes
				RedGui::CRedGuiPanel* physicalResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				physicalResPanel->SetBorderVisible( false );
				physicalResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( physicalResPanel );
				physicalResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* physicalResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					physicalResPanel->AddChild( physicalResLabel );
					physicalResLabel->SetAutoSize( false );
					physicalResLabel->SetText( TXT("Physical: ") );
					physicalResLabel->SetDock( RedGui::DOCK_Left );

					m_statPhysicalRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					physicalResPanel->AddChild( m_statPhysicalRes );
					m_statPhysicalRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statPhysicalRes->SetDock( RedGui::DOCK_Left );
					m_statPhysicalRes->SetShowProgressInformation( true );
					m_statPhysicalRes->SetProgressRange( 100 );
					m_statPhysicalRes->SetProgressPosition( 0 );
					m_statPhysicalRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_BleedingRes
				RedGui::CRedGuiPanel* bleedingResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				bleedingResPanel->SetBorderVisible( false );
				bleedingResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( bleedingResPanel );
				bleedingResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* bleedingResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					bleedingResPanel->AddChild( bleedingResLabel );
					bleedingResLabel->SetAutoSize( false );
					bleedingResLabel->SetText( TXT("Bleeding: ") );
					bleedingResLabel->SetDock( RedGui::DOCK_Left );

					m_statBleedingRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					bleedingResPanel->AddChild( m_statBleedingRes );
					m_statBleedingRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statBleedingRes->SetDock( RedGui::DOCK_Left );
					m_statBleedingRes->SetShowProgressInformation( true );
					m_statBleedingRes->SetProgressRange( 100 );
					m_statBleedingRes->SetProgressPosition( 0 );
					m_statBleedingRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_PoisonRes
				RedGui::CRedGuiPanel* poisonResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				poisonResPanel->SetBorderVisible( false );
				poisonResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( poisonResPanel );
				poisonResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* poisonResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					poisonResPanel->AddChild( poisonResLabel );
					poisonResLabel->SetAutoSize( false );
					poisonResLabel->SetText( TXT("Poison: ") );
					poisonResLabel->SetDock( RedGui::DOCK_Left );

					m_statPoisonRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					poisonResPanel->AddChild( m_statPoisonRes );
					m_statPoisonRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statPoisonRes->SetDock( RedGui::DOCK_Left );
					m_statPoisonRes->SetShowProgressInformation( true );
					m_statPoisonRes->SetProgressRange( 100 );
					m_statPoisonRes->SetProgressPosition( 0 );
					m_statPoisonRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}
				
				// CDS_FireRes
				RedGui::CRedGuiPanel* fireResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				fireResPanel->SetBorderVisible( false );
				fireResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( fireResPanel );
				fireResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* fireResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					fireResPanel->AddChild( fireResLabel );
					fireResLabel->SetAutoSize( false );
					fireResLabel->SetText( TXT("Fire: ") );
					fireResLabel->SetDock( RedGui::DOCK_Left );

					m_statFireRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					fireResPanel->AddChild( m_statFireRes );
					m_statFireRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statFireRes->SetDock( RedGui::DOCK_Left );
					m_statFireRes->SetShowProgressInformation( true );
					m_statFireRes->SetProgressRange( 100 );
					m_statFireRes->SetProgressPosition( 0 );
					m_statFireRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_FrostRes
				RedGui::CRedGuiPanel* frostResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				frostResPanel->SetBorderVisible( false );
				frostResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( frostResPanel );
				frostResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* frostResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					frostResPanel->AddChild( frostResLabel );
					frostResLabel->SetAutoSize( false );
					frostResLabel->SetText( TXT("Frost: ") );
					frostResLabel->SetDock( RedGui::DOCK_Left );

					m_statFrostRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					frostResPanel->AddChild( m_statFrostRes );
					m_statFrostRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statFrostRes->SetDock( RedGui::DOCK_Left );
					m_statFrostRes->SetShowProgressInformation( true );
					m_statFrostRes->SetProgressRange( 100 );
					m_statFrostRes->SetProgressPosition( 0 );
					m_statFrostRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_ShockRes
				RedGui::CRedGuiPanel* shockResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				shockResPanel->SetBorderVisible( false );
				shockResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( shockResPanel );
				shockResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* shockResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					shockResPanel->AddChild( shockResLabel );
					shockResLabel->SetAutoSize( false );
					shockResLabel->SetText( TXT("Shock: ") );
					shockResLabel->SetDock( RedGui::DOCK_Left );

					m_statShockRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					shockResPanel->AddChild( m_statShockRes );
					m_statShockRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statShockRes->SetDock( RedGui::DOCK_Left );
					m_statShockRes->SetShowProgressInformation( true );
					m_statShockRes->SetProgressRange( 100 );
					m_statShockRes->SetProgressPosition( 0 );
					m_statShockRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_ForceRes
				RedGui::CRedGuiPanel* forceResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				forceResPanel->SetBorderVisible( false );
				forceResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( forceResPanel );
				forceResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* forceResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					forceResPanel->AddChild( forceResLabel );
					forceResLabel->SetAutoSize( false );
					forceResLabel->SetText( TXT("Force: ") );
					forceResLabel->SetDock( RedGui::DOCK_Left );

					m_statForceRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					forceResPanel->AddChild( m_statForceRes );
					m_statForceRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statForceRes->SetDock( RedGui::DOCK_Left );
					m_statForceRes->SetShowProgressInformation( true );
					m_statForceRes->SetProgressRange( 100 );
					m_statForceRes->SetProgressPosition( 0 );
					m_statForceRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_FreezeRes
				RedGui::CRedGuiPanel* freezeResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				freezeResPanel->SetBorderVisible( false );
				freezeResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( freezeResPanel );
				freezeResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* freezeResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					freezeResPanel->AddChild( freezeResLabel );
					freezeResLabel->SetAutoSize( false );
					freezeResLabel->SetText( TXT("Freeze: ") );
					freezeResLabel->SetDock( RedGui::DOCK_Left );

					m_statFreezeRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					freezeResPanel->AddChild( m_statFreezeRes );
					m_statFreezeRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statFreezeRes->SetDock( RedGui::DOCK_Left );
					m_statFreezeRes->SetShowProgressInformation( true );
					m_statFreezeRes->SetProgressRange( 100 );
					m_statFreezeRes->SetProgressPosition( 0 );
					m_statFreezeRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_WillRes
				RedGui::CRedGuiPanel* willResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				willResPanel->SetBorderVisible( false );
				willResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( willResPanel );
				willResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* willResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					willResPanel->AddChild( willResLabel );
					willResLabel->SetAutoSize( false );
					willResLabel->SetText( TXT("Will: ") );
					willResLabel->SetDock( RedGui::DOCK_Left );

					m_statWillRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					willResPanel->AddChild( m_statWillRes );
					m_statWillRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statWillRes->SetDock( RedGui::DOCK_Left );
					m_statWillRes->SetShowProgressInformation( true );
					m_statWillRes->SetProgressRange( 100 );
					m_statWillRes->SetProgressPosition( 0 );
					m_statWillRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}

				// CDS_BurningRes
				RedGui::CRedGuiPanel* burningResPanel = new RedGui::CRedGuiPanel( 0, 0, 200, 15 );
				burningResPanel->SetBorderVisible( false );
				burningResPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				defenceStatsGroupBox->AddChild( burningResPanel );
				burningResPanel->SetDock( RedGui::DOCK_Top );
				{
					RedGui::CRedGuiLabel* burningResLabel= new RedGui::CRedGuiLabel(0,0,100,20);
					burningResPanel->AddChild( burningResLabel );
					burningResLabel->SetAutoSize( false );
					burningResLabel->SetText( TXT("Burning: ") );
					burningResLabel->SetDock( RedGui::DOCK_Left );

					m_statBurningRes = new RedGui::CRedGuiProgressBar( 0,0, 150, 12 );
					burningResPanel->AddChild( m_statBurningRes );
					m_statBurningRes->SetMargin( Box2( 10, 0, 0, 0 ) );
					m_statBurningRes->SetDock( RedGui::DOCK_Left );
					m_statBurningRes->SetShowProgressInformation( true );
					m_statBurningRes->SetProgressRange( 100 );
					m_statBurningRes->SetProgressPosition( 0 );
					m_statBurningRes->SetProgressBarColor( Lerp< Vector >( 0.0, Color::RED.ToVector(), Color::GREEN.ToVector() ) );
				}
			}
		}
	}

	void CDebugWindowNPCViewer::CreateInventoryTab()
	{
		CRedGuiControl* inventoryTab = m_npcInfoCategories->GetTabAt( IVTT_Inventory );
		if(inventoryTab != nullptr)
		{
			m_inventoryState = new RedGui::CRedGuiLabel(0,0,0,0);
			inventoryTab->AddChild(m_inventoryState);
			m_inventoryState->SetMargin(Box2(10, 10, 10, 3));
			m_inventoryState->SetText(TXT("Inventory state: "));
			m_inventoryState->SetDock(RedGui::DOCK_Top);

			m_inventorySize = new RedGui::CRedGuiLabel(0,0,0,0);
			inventoryTab->AddChild(m_inventorySize);
			m_inventorySize->SetMargin(Box2(10, 3, 10, 3));
			m_inventorySize->SetText(TXT("Item count: "));
			m_inventorySize->SetDock(RedGui::DOCK_Top);

			RedGui::CRedGuiGroupBox* itemProperties = new RedGui::CRedGuiGroupBox( 0, 0, 0, 200 );
			inventoryTab->AddChild(itemProperties);
			itemProperties->SetMargin(Box2(10, 3, 10, 3));
			itemProperties->SetDock(RedGui::DOCK_Top);
			itemProperties->SetText( TXT("Item properties") );
			if( itemProperties != nullptr )
			{
				RedGui::CRedGuiPanel* definitionItemPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 15 );
				definitionItemPanel->SetBorderVisible(false);
				definitionItemPanel->SetMargin( Box2( 10, 10, 10, 3 ) );
				itemProperties->AddChild( definitionItemPanel );
				definitionItemPanel->SetDock( RedGui::DOCK_Top );
				{
					m_inventoryDefineFile = new RedGui::CRedGuiLabel(0,0,0,0);
					definitionItemPanel->AddChild( m_inventoryDefineFile );
					m_inventoryDefineFile->SetText( TXT("Item definition file: ") );
					m_inventoryDefineFile->SetDock( RedGui::DOCK_Left );

					m_inventoryOpenDefinitionFile = new RedGui::CRedGuiButton(0,0, 75, 10);
					definitionItemPanel->AddChild( m_inventoryOpenDefinitionFile);
					m_inventoryOpenDefinitionFile->SetText( TXT("Open"));
					m_inventoryOpenDefinitionFile->SetMargin(Box2(10, 0, 0, 0));
					m_inventoryOpenDefinitionFile->SetDock(RedGui::DOCK_Left);
					m_inventoryOpenDefinitionFile->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyOpenItemDefinitionFile );
				}

				m_inventoryCategory = new RedGui::CRedGuiLabel(0,0,0,0);
				itemProperties->AddChild( m_inventoryCategory );
				m_inventoryCategory->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_inventoryCategory->SetText( TXT("Category: ") );
				m_inventoryCategory->SetDock( RedGui::DOCK_Top );

				m_inventoryQuantity = new RedGui::CRedGuiLabel(0,0,0,0);
				itemProperties->AddChild( m_inventoryQuantity );
				m_inventoryQuantity->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_inventoryQuantity->SetText( TXT("Quantity: ") );
				m_inventoryQuantity->SetDock( RedGui::DOCK_Top );

				m_inventoryDurability = new RedGui::CRedGuiLabel(0,0,0,0);
				itemProperties->AddChild( m_inventoryDurability );
				m_inventoryDurability->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_inventoryDurability->SetText( TXT("Durability: ") );
				m_inventoryDurability->SetDock( RedGui::DOCK_Top );

				m_inventoryIsWeapon = new RedGui::CRedGuiLabel(0,0,0,0);
				itemProperties->AddChild( m_inventoryIsWeapon );
				m_inventoryIsWeapon->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_inventoryIsWeapon->SetText( TXT("Is weapon: ") );
				m_inventoryIsWeapon->SetDock( RedGui::DOCK_Top );

				m_inventoryIsStackable = new RedGui::CRedGuiLabel(0,0,0,0);
				itemProperties->AddChild( m_inventoryIsStackable );
				m_inventoryIsStackable->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_inventoryIsStackable->SetText( TXT("Is stackable: ") );
				m_inventoryIsStackable->SetDock( RedGui::DOCK_Top );

				m_inventoryIsLootable = new RedGui::CRedGuiLabel(0,0,0,0);
				itemProperties->AddChild( m_inventoryIsLootable );
				m_inventoryIsLootable->SetMargin( Box2( 10, 3, 10, 3 ) );
				m_inventoryIsLootable->SetText( TXT("Is lootable: ") );
				m_inventoryIsLootable->SetDock( RedGui::DOCK_Top );
			}

			m_inventoryItemList = new RedGui::CRedGuiList(0,0,140,300);
			m_inventoryItemList->AppendColumn( TXT("Items"), 100 );
			inventoryTab->AddChild(m_inventoryItemList);
			m_inventoryItemList->SetMargin(Box2(10, 3, 10, 3));
			m_inventoryItemList->SetDock(RedGui::DOCK_Fill);
			m_inventoryItemList->EventSelectedItem.Bind( this, &CDebugWindowNPCViewer::NotifySetItemParameters );
		}
	}

	void CDebugWindowNPCViewer::CreateMovementTab()
	{
		CRedGuiControl* movementTab = m_npcInfoCategories->GetTabAt( IVTT_Movement );
		if(movementTab != nullptr)
		{
			m_movementHasComponent = new RedGui::CRedGuiLabel(0,0,0,0);
			movementTab->AddChild(m_movementHasComponent);
			m_movementHasComponent->SetMargin(Box2(10, 10, 10, 3));
			m_movementHasComponent->SetText(TXT("Movement component: "));
			m_movementHasComponent->SetDock(RedGui::DOCK_Top);

			m_movementState = new RedGui::CRedGuiLabel(0,0,0,0);
			movementTab->AddChild(m_movementState);
			m_movementState->SetMargin(Box2(10, 3, 10, 3));
			m_movementState->SetText(TXT("Movement component state: "));
			m_movementState->SetDock(RedGui::DOCK_Top);

			RedGui::CRedGuiPanel* lastTeleportPanel = new RedGui::CRedGuiPanel(0, 0, 100, 15);
			lastTeleportPanel->SetBorderVisible(false);
			lastTeleportPanel->SetMargin(Box2(10, 3, 10, 3));
			movementTab->AddChild(lastTeleportPanel);
			lastTeleportPanel->SetDock(RedGui::DOCK_Top);
			{
				m_movementLastTeleport = new RedGui::CRedGuiLabel(0,0,0,0);
				lastTeleportPanel->AddChild(m_movementLastTeleport);
				m_movementLastTeleport->SetText(TXT("Last teleport: "));
				m_movementLastTeleport->SetDock(RedGui::DOCK_Left);

				m_movementTeleportPlayerToLastPoint = new RedGui::CRedGuiButton(0,0, 100, 10);
				lastTeleportPanel->AddChild(m_movementTeleportPlayerToLastPoint);
				m_movementTeleportPlayerToLastPoint->SetText( TXT("Teleport player"));
				m_movementTeleportPlayerToLastPoint->SetMargin(Box2(10, 0, 0, 0));
				m_movementTeleportPlayerToLastPoint->SetDock(RedGui::DOCK_Left);
				m_movementTeleportPlayerToLastPoint->EventButtonClicked.Bind( this, &CDebugWindowNPCViewer::NotifyEventButtonClickedOnTeleportPlayerToLastPoint);
			}

			m_movementTopRepresentation = new RedGui::CRedGuiLabel(0,0,0,0);
			movementTab->AddChild(m_movementTopRepresentation);
			m_movementTopRepresentation->SetMargin(Box2(10, 3, 10, 3));
			m_movementTopRepresentation->SetText(TXT("Top representation: "));
			m_movementTopRepresentation->SetDock(RedGui::DOCK_Top);

			m_movementPriority = new RedGui::CRedGuiLabel(0,0,0,0);
			movementTab->AddChild(m_movementPriority);
			m_movementPriority->SetMargin(Box2(10, 3, 10, 3));
			m_movementPriority->SetText(TXT("Character movement priority: "));
			m_movementPriority->SetDock(RedGui::DOCK_Top);

			m_movementQueuePosition = new RedGui::CRedGuiLabel(0,0,0,0);
			movementTab->AddChild(m_movementQueuePosition);
			m_movementQueuePosition->SetMargin(Box2(10, 3, 10, 3));
			m_movementQueuePosition->SetText(TXT("Movement queue position: "));
			m_movementQueuePosition->SetDock(RedGui::DOCK_Top);

			m_movementStaticRotation = new RedGui::CRedGuiLabel(0,0,0,0);
			movementTab->AddChild(m_movementStaticRotation);
			m_movementStaticRotation->SetMargin(Box2(10, 3, 10, 3));
			m_movementStaticRotation->SetText(TXT("Static rotation is: "));
			m_movementStaticRotation->SetDock(RedGui::DOCK_Top);

			m_movementLocomotionLines = new RedGui::CRedGuiList(0, 0, 300, 300);
			m_movementLocomotionLines->AppendColumn( TXT("Locomotion lines"), 100 );
			movementTab->AddChild(m_movementLocomotionLines);
			m_movementLocomotionLines->SetMargin(Box2(10, 3, 10, 3));
			m_movementLocomotionLines->SetDock(RedGui::DOCK_Fill);
		}
	}

	void CDebugWindowNPCViewer::CreateQuestLocksTab()
	{
		CRedGuiControl* questLocksTab = m_npcInfoCategories->GetTabAt( IVTT_QuestLocks );
		if(questLocksTab != nullptr)
		{
			m_questLocksList = new RedGui::CRedGuiList(0, 0, 300, 300);
			m_questLocksList->AppendColumn( TXT("Quest locks"), 100 );
			questLocksTab->AddChild(m_questLocksList);
			m_questLocksList->SetMargin(Box2(10, 10, 10, 3));
			m_questLocksList->SetDock(RedGui::DOCK_Fill);
		}
	}

	void CDebugWindowNPCViewer::CreateNoticedObjectsTab()
	{
		CRedGuiControl* noticedObjectsTab = m_npcInfoCategories->GetTabAt( IVTT_NoticedObjects );
		if(noticedObjectsTab != nullptr)
		{
			m_noticedObjectsList = new RedGui::CRedGuiList(0, 0, 300, 300);
			m_noticedObjectsList->AppendColumn( TXT("Noticed objects"), 100 );
			noticedObjectsTab->AddChild(m_noticedObjectsList);
			m_noticedObjectsList->SetMargin(Box2(10, 10, 10, 3));
			m_noticedObjectsList->SetDock(RedGui::DOCK_Fill);
		}
	}

	void CDebugWindowNPCViewer::CreateAttitudesTab()
	{
		CRedGuiControl* attitudesTab = m_npcInfoCategories->GetTabAt( IVTT_Attitudes );
		if(attitudesTab != nullptr)
		{
			m_attitudesState = new RedGui::CRedGuiLabel(0,0,0,0);
			attitudesTab->AddChild(m_attitudesState);
			m_attitudesState->SetMargin(Box2(10, 10, 10, 3));
			m_attitudesState->SetText(TXT("NPC attitiude state: "));
			m_attitudesState->SetDock(RedGui::DOCK_Top);

			m_attitudesList = new RedGui::CRedGuiList(0, 0, 300, 300);
			m_attitudesList->AppendColumn( TXT("Attitudes"), 100 );
			attitudesTab->AddChild(m_attitudesList);
			m_attitudesList->SetMargin(Box2(10, 3, 10, 3));
			m_attitudesList->SetDock(RedGui::DOCK_Fill);
		}
	}

	void CDebugWindowNPCViewer::CreateMeshesTab()
	{
		CRedGuiControl* meshesTab = m_npcInfoCategories->GetTabAt( IVTT_MeshComponents );
		if(meshesTab != nullptr)
		{
			// create tooltip
			RedGui::CRedGuiPanel* m_meshesListToolTip = new RedGui::CRedGuiPanel(0, 0, 200, 50);
			m_meshesListToolTip->SetBackgroundColor(Color(53, 53, 53));
			m_meshesListToolTip->AttachToLayer(TXT("Pointers"));
			m_meshesListToolTip->SetVisible(false);
			m_meshesListToolTip->SetPadding(Box2(5, 5, 5, 5));
			m_meshesListToolTip->SetAutoSize(true);

			RedGui::CRedGuiLabel* greenInfo = new RedGui::CRedGuiLabel(10, 10, 10, 15);
			greenInfo->SetText(TXT("Mesh is visible"), Color::GREEN);
			(*m_meshesListToolTip).AddChild( greenInfo );

			RedGui::CRedGuiLabel* redInfo = new RedGui::CRedGuiLabel(10, 25, 10, 15);
			redInfo->SetText(TXT("Mesh is invisible"), Color::RED);
			(*m_meshesListToolTip).AddChild( redInfo );

			// create meshes list
			m_meshesList = new RedGui::CRedGuiList(0, 0, 300, 300);
			m_meshesList->AppendColumn( TXT("Meshes"), 100 );
			m_meshesList->SetNeedToolTip(true);
			m_meshesList->SetToolTip(m_meshesListToolTip);
			meshesTab->AddChild(m_meshesList);
			m_meshesList->SetMargin(Box2(10, 10, 10, 3));
			m_meshesList->SetDock(RedGui::DOCK_Fill);
			m_meshesList->EventDoubleClickItem.Bind( this, &CDebugWindowNPCViewer::NotifyMouseButtonDoubleClickOnMeshComponent );
		}
	}

	void CDebugWindowNPCViewer::CreateStoryTab()
	{
		CRedGuiControl* storyTab = m_npcInfoCategories->GetTabAt( IVTT_StoryInformations );
		if(storyTab != nullptr)
		{
			// create active locks list for quests
			m_storyQuestList = new RedGui::CRedGuiList(0, 0, 300, 300);
			m_storyQuestList->AppendColumn( TXT("Active locks"), 100 );
			storyTab->AddChild(m_storyQuestList);
			m_storyQuestList->SetMargin(Box2(10, 10, 10, 3));
			m_storyQuestList->SetDock(RedGui::DOCK_Fill);
		}
	}

	void CDebugWindowNPCViewer::CreateMoraleTab()
	{
		CRedGuiControl* moraleTab = m_npcInfoCategories->GetTabAt( IVTT_Morale );
		if(moraleTab != nullptr)
		{
			// TODO probably for M2 or later
			m_moraleInfo = new RedGui::CRedGuiLabel(0,0,0,0);
			m_moraleInfo->SetAlign(RedGui::IA_MiddleCenter);
			m_moraleInfo->SetText(TXT("Will be done probably for M2"));
			moraleTab->AddChild(m_moraleInfo);
		}
	}

	void CDebugWindowNPCViewer::CreateLootTab()
	{
		CRedGuiControl* lootTab = m_npcInfoCategories->GetTabAt( IVTT_Loot );
		if(lootTab != nullptr)
		{
			// TODO probably for M2 or later
			m_lootInfo = new RedGui::CRedGuiLabel(0,0,0,0);
			m_lootInfo->SetAlign(RedGui::IA_MiddleCenter);
			m_lootInfo->SetText(TXT("Will be done probably for M2"));
			lootTab->AddChild(m_lootInfo);
		}
	}

	void CDebugWindowNPCViewer::UpdateBaseStatEntry( CNewNPC* npc, RedGui::CRedGuiProgressBar* progressBar, Uint32 enumIndex )
	{
		Float stat = 0;
		Float statMax = 0;

		if( CallFunctionRet< Float >( npc, CNAME( GetStatMax ), enumIndex, statMax ) == true )
		{
			progressBar->SetProgressRange( statMax );
		}
		if( CallFunctionRet< Float >( npc, CNAME( GetStat ), enumIndex, stat ) == true )
		{
			progressBar->SetProgressPosition( stat );
		}
		progressBar->SetProgressBarColor( Lerp< Vector >( ( stat/statMax ), Color::RED.ToVector(), Color::GREEN.ToVector() ) );
		progressBar->SetProgressInformation( String::Printf( TXT("%.0f / %.0f"), stat, statMax ) );
	}

	void CDebugWindowNPCViewer::UpdateResistStatEntry( CNewNPC* npc, RedGui::CRedGuiProgressBar* progressBar, Uint32 enumIndex )
	{
		Float points = 0.0f;
		Float percents = 0.0f;

		if( CallFunction( npc, CNAME( GetResistValue ), enumIndex, points, percents ) == true )
		{
			progressBar->SetProgressPosition(  percents * 100.0f );
			progressBar->SetProgressInformation( String::Printf( TXT("%.0f points"), points ) );
		}
	}

	void CDebugWindowNPCViewer::UpdateAttackStatEntry( CNewNPC* npc, RedGui::CRedGuiLabel* label, Uint32 enumIndex, const String& startText )
	{
		SAbilityAttributeValue value;

		if( CallFunction( npc, CNAME( GetPowerStatValue ), enumIndex, value ) == true )
		{
			label->SetText( String::Printf( TXT("%ls:          Base = %f, Mult = %f, Add = %f"), startText.AsChar(), value.m_valueBase, value.m_valueMultiplicative, value.m_valueAdditive ) );
		}
	}

	void CDebugWindowNPCViewer::UpdateRegenStatEntry( CNewNPC* npc, RedGui::CRedGuiLabel* label, Uint32 enumIndex, const String& startText )
	{
		SAbilityAttributeValue value;

		if( CallFunction( npc, CNAME( GetAttributeValue ), enumIndex, value ) == true )
		{
			label->SetText( String::Printf( TXT("%ls:          Base = %f, Mult = %f, Add = %f"), startText.AsChar(), value.m_valueBase, value.m_valueMultiplicative, value.m_valueAdditive ) );
		}
	}

	void CDebugWindowNPCViewer::UpdateGlobalDamageEntry( CNewNPC* npc, RedGui::CRedGuiLabel* label )
	{
		SAbilityAttributeValue value;

		String name( TXT("damage_bonus") );
		if( CallFunction( npc, CNAME( GetAttributeValue ),name , value ) == true )
		{
			label->SetText( String::Printf( TXT("Global damage bonus:          Base = %f, Mult = %f, Add = %f"), value.m_valueBase, value.m_valueMultiplicative, value.m_valueAdditive ) );
		}
	}

	void CDebugWindowNPCViewer::NotifyKillNPC( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CNewNPC* npc = m_active.Get();
		if( npc != nullptr )
		{
			npc->Kill( true );
		}
	}

	void CDebugWindowNPCViewer::NotifyStunNPC( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CNewNPC* npc = m_active.Get();
		if( npc != nullptr )
		{
			npc->Stun( true );
		}
	}

	void CDebugWindowNPCViewer::NotifyDestroyNPC( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CNewNPC* npc = m_active.Get();
		if( npc != nullptr )
		{
			npc->EnterForcedDespawn();
			RefreshInformation();
		}
	}

	void CDebugWindowNPCViewer::NotifyOpenSpawnset( RedGui::CRedGuiEventPackage& eventPackage )
	{
#ifndef NO_EDITOR_EVENT_SYSTEM
		CNewNPC* npc = m_active.Get();
		CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( cs != nullptr && npc != nullptr )
		{
			// Is NPC in Community
			const SAgentStub *agentStub = cs->FindStubForNPC( npc );

			// Agent stub info
			if(agentStub != nullptr)
			{
				const CCommunity* community = agentStub->GetParentSpawnset();
				if( community != nullptr )
				{
					String templatePath = community->GetDepotPath();
					SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( templatePath ) );
				}
			}
		}
#endif	// NO_EDITOR_EVENT_SYSTEM
	}

	void CDebugWindowNPCViewer::NotifyOpenActiveAP( RedGui::CRedGuiEventPackage& eventPackage )
	{
#ifndef NO_EDITOR_EVENT_SYSTEM
		CNewNPC* npc = m_active.Get();
		CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
		if ( cs != nullptr && npc != nullptr )
		{
			const NewNPCScheduleProxy &npcSchedule = npc->GetActionSchedule();
			TActionPointID pointID = npcSchedule.GetActiveAP();
			if( pointID != ActionPointBadID )
			{
				CCommunitySystem* cs = GCommonGame->GetSystem< CCommunitySystem >();
				if( cs != nullptr )
				{
					if ( const CActionPointComponent* actionPointComp = cs->GetActionPointManager()->GetAP( pointID ) )
					{
						CEntity* entity = actionPointComp->GetEntity();
						if( entity != nullptr )
						{
							CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
							if( entityTemplate != nullptr )
							{
								String path = entityTemplate->GetDepotPath();
								SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( path ) );
							}
						}
					}
				}
			}
		}
#endif	// NO_EDITOR_EVENT_SYSTEM
	}

	void CDebugWindowNPCViewer::NotifyOpenItemDefinitionFile( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CNewNPC* npc = m_active.Get();
		if(npc != nullptr )
		{
			Int32 index = m_inventoryItemList->GetSelection();
			if( index != -1 )
			{
				// Get inventory component
				CInventoryComponent* ic = npc->GetInventoryComponent();
				if ( ic != nullptr)
				{
					// inventory item count
					const auto& items = ic->GetItems();
					const SInventoryItem& item = items[index];

					CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
					if( defMgr != nullptr )
					{
						const SItemDefinition* itemDef = defMgr->GetItemDefinition( item.GetName() );
						if( itemDef != nullptr )
						{
#ifdef RED_PLATFORM_CONSOLE
							GRedGui::GetInstance().MessageBox( TXT("Cannot open file on console, only in editor") );

#else
#ifndef NO_DATA_ASSERTS
							String path = itemDef->m_debugXMLFile;
							String cmd = TXT( "explorer /select," );
							cmd += ( GFileManager->GetDataDirectory() + path ).AsChar();
							//::wxExecute( cmd, wxEXEC_ASYNC, nullptr );
							system( UNICODE_TO_ANSI( cmd.AsChar() ) );
#else
							GRedGui::GetInstance().MessageBox( TXT("Cannot open file when NO_DATA_ASSERTS is active") );
#endif	// NO_DATA_ASSERTS
#endif	// RED_PLATFORM_CONSOLE
						}
					}
				}
			}
		}
	}

	void CDebugWindowNPCViewer::NotifySetItemParameters( RedGui::CRedGuiEventPackage& eventPackage, Int32 index )
	{
		CNewNPC* npc = m_active.Get();
		if(npc != nullptr )
		{
			Int32 index = m_inventoryItemList->GetSelection();
			if( index != -1 )
			{
				// Get inventory component
				CInventoryComponent* ic = npc->GetInventoryComponent();
				if ( ic != nullptr)
				{
					// inventory item count
					const auto& items = ic->GetItems();
					const SInventoryItem& item = items[index];

					CDefinitionsManager* defMgr = GCommonGame->GetDefinitionsManager();
					if( defMgr != nullptr )
					{
						const SItemDefinition* itemDef = defMgr->GetItemDefinition( item.GetName() );
						if( itemDef != nullptr )
						{
#ifndef NO_DATA_ASSERTS
							m_inventoryDefineFile->SetText( String::Printf( TXT("Item definition file: %ls"), itemDef->m_debugXMLFile.AsChar() ) );
#else
							m_inventoryDefineFile->SetText( String::Printf( TXT("Item definition file: is unavailable when NO_DATA_ASSERTS is active" ) ) );
#endif	// NO_DATA_ASSERTS
							m_inventoryOpenDefinitionFile->SetOutOfDate();

							m_inventoryCategory->SetText( String::Printf( TXT("Category: %ls"), item.GetCategory().AsChar() ) );
							m_inventoryQuantity->SetText( String::Printf( TXT("Quantity: %d"), item.GetQuantity() ) );
							m_inventoryDurability->SetText( String::Printf( TXT("Durability: %.2f"), item.GetDurability() ) );
							m_inventoryIsWeapon->SetText( String::Printf( TXT("Is weapon: %ls"), ( item.IsWeapon() == true ) ? TXT("YES") : TXT("NO") ) );
							m_inventoryIsStackable->SetText( String::Printf( TXT("Is stackable: %ls"), ( item.IsStackable() == true ) ? TXT("YES") : TXT("NO") ) );
							m_inventoryIsLootable->SetText( String::Printf( TXT("Is lootable: %ls"), ( item.IsLootable() == true ) ? TXT("YES") : TXT("NO") ) );
						}
					}
				}
			}
		}
	}

	void CDebugWindowNPCViewer::ResetAllInformation()
	{
		// clear general tab
		m_generalTemplateName->SetText( TXT("Template: ") );
		m_generalOpenEntityTemplate->SetOutOfDate();
		m_generalMemoryUsageByObject->SetText( TXT("Memory usage by object:") );
		m_generalMemoryUsageByTemplate->SetText( TXT("Memory usage by template:") );
		m_generalTags->SetText( TXT("Tags:") );
		m_generalAppearance->SetText( TXT("Without appearance components") );
		m_generalVoiceTag->SetText( TXT("VoiceTag:") );
		m_generalProfileRecursive->SetText( TXT("Attitude group: No AI profile definied") );
		m_generalIsAlive->SetText( TXT("Is alive:") );
		m_generalBehMachine->SetText( TXT("Beh Machine Info: ") );
		m_generalEncounter->SetText( TXT("Encounter: -") );
		m_generalEncounterActiveEntry->SetText( TXT("Encounter active entry: -") );

		// clear stats tab
		m_statVitality->SetProgressPosition( 0 );
		m_statEssence->SetProgressPosition( 0 );
		m_statStamina->SetProgressPosition( 0 );
		m_statToxicity->SetProgressPosition( 0 );
		m_statFocus->SetProgressPosition( 0 );
		m_statMorale->SetProgressPosition( 0 );
		m_statDrunkenness->SetProgressPosition( 0 );
		m_statAir->SetProgressPosition( 0 );

		m_statAttackPower->SetText( TXT("Attack power: ") );
		m_statSpellPower->SetText( TXT("Spell power: ") );
		m_statDamageBonus->SetText( TXT("Global damage bonus: ") );

		m_statVitalityRegen->SetText( TXT("Vitality: ") );
		m_statEssenceRegen->SetText( TXT("Essence: ") );
		m_statMoraleRegen->SetText( TXT("Morale: ") );
		m_statToxicityRegen->SetText( TXT("Toxicity: ") );
		m_statDrunkennessRegen->SetText( TXT("Drunkenness: ") );
		m_statStaminaRegen->SetText( TXT("Stamina: ") );
		m_statAirRegen->SetText( TXT("Air: ") );

		m_statNoneRes->SetProgressPosition( 0 );
		m_statPhysicalRes->SetProgressPosition( 0 );
		m_statBleedingRes->SetProgressPosition( 0 );
		m_statPoisonRes->SetProgressPosition( 0 );
		m_statFireRes->SetProgressPosition( 0 );
		m_statFrostRes->SetProgressPosition( 0 );
		m_statShockRes->SetProgressPosition( 0 );
		m_statForceRes->SetProgressPosition( 0 );
		m_statFreezeRes->SetProgressPosition( 0 );
		m_statWillRes->SetProgressPosition( 0 );
		m_statBurningRes->SetProgressPosition( 0 );


		// clear script tab
		m_scriptStateName->SetText( TXT("State:") );
		m_scriptTopFunction->SetText( TXT("Function:") );

		// clear community tab
		m_communityIsInCommunity->SetText( TXT("Is NPC in Community:") );
		m_communityStoryPhase->SetText( TXT("Story phase:") );
		m_communitySpawnsetName->SetText( TXT("Spawnset name: ") );
		m_communityCurrentStubState->SetText( TXT("Current stub state:") );
		m_communityActiveAP->SetText(String::Printf(TXT("Active AP is EMPTY")) );
		m_communityLastAP->SetText(String::Printf(TXT("Last AP is EMPTY")) );
		m_communityUsingLastAP->SetText( TXT("Is using last AP:") );
		m_communityWorkingInAP->SetText( TXT("Is not working") );
		m_communityWorkState->ClearText();
		m_communityIsInfinite->ClearText();
		m_communityExecutionMode->ClearText();
		m_communityAPName->ClearText();

		// clear inventory tab
		m_inventoryItemList->RemoveAllItems();
		m_inventoryState->SetText(TXT("Inventory state: No inventory component"));
		m_inventorySize->ClearText();

		// clear movement information
		m_movementLocomotionLines->RemoveAllItems();
		m_movementHasComponent->SetText(TXT("Movement component: Absence"));
		m_movementState->ClearText();
		m_movementLastTeleport->ClearText();
		m_movementTopRepresentation->ClearText();
		m_movementPriority->ClearText();
		m_movementQueuePosition->ClearText();
		m_movementStaticRotation->ClearText();

		// clear quest tab
		m_questLocksList->RemoveAllItems();

		// clear noticed tab
		m_noticedObjectsList->RemoveAllItems();

		// clear attitudes tab
		m_attitudesState->SetText( TXT( "NPC attitude state: -" ) );
		m_attitudesList->RemoveAllItems();

		// clear mesh tab
		m_meshesList->RemoveAllItems();

		// clear story tab
		m_storyQuestList->RemoveAllItems();
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
