/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "debugWindowVehicleViewer.h"

#include "../engine/appearanceComponent.h"
#include "vehicle.h"
#include "../engine/freeCamera.h"
#include "../engine/redGuiManager.h"
#include "../engine/redGuiPanel.h"
#include "../engine/redGuiButton.h"
#include "../engine/redGuiList.h"
#include "../engine/redGuiTab.h"
#include "../engine/redGuiScrollPanel.h"
#include "../engine/redGuiLabel.h"
#include "../core/fileSkipableBlock.h"
#include "../core/memoryFileAnalizer.h"
#include "../engine/layerGroup.h"
#include "../engine/layerInfo.h"
#include "../engine/dynamicLayer.h"
#include "../engine/meshComponent.h"


namespace DebugWindows
{
	CDebugWindowVehicleViewer::SVehicleEntry::SVehicleEntry()
		: m_vehicleComponent( nullptr )
		, m_entityName( String::EMPTY )
		, m_vehicleEntity( nullptr )
	{
		/* intentionally empty */
	}

	CDebugWindowVehicleViewer::SVehicleEntry::SVehicleEntry( CVehicleComponent* vehicleComponent, String entityName, CEntity* vehicleEntity )
		: m_vehicleComponent( vehicleComponent )
		, m_entityName( entityName )
		, m_vehicleEntity( vehicleEntity )
	{
		/* intentionally empty */
	}

	CDebugWindowVehicleViewer::CDebugWindowVehicleViewer()
		: RedGui::CRedGuiWindow(50,50, 1100, 600)
		, m_cameraRotation( 0.0f )
		, m_cameraTarget( Vector::ZEROS )
		, m_cameraPosition( Vector::ZEROS )
		, m_active( -1 )
		, m_verticalOffset( 0 )
		, m_pauseRotation(false)
	{
		SetCaption(TXT("Vehicle viewer"));

		GRedGui::GetInstance().EventTick.Bind(this, &CDebugWindowVehicleViewer::NotifyOnFrameTick);
		GRedGui::GetInstance().EventCalculateCamera.Bind(this, &CDebugWindowVehicleViewer::NotifyViewportCalculateCamera);
		GRedGui::GetInstance().EventViewportInput.Bind(this, &CDebugWindowVehicleViewer::NotifyViewportInput);

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
					firstLine->SetMargin(Box2(5, 5, 5, 5));
					leftBottomPanel->AddChild(firstLine);
					firstLine->SetDock(RedGui::DOCK_Top);

					// buttons
					{
						m_refreshList = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_refreshList->SetMargin(Box2(5, 5, 5, 0));
						m_refreshList->SetText(TXT("Refresh list"));
						m_refreshList->EventButtonClicked.Bind(this, &CDebugWindowVehicleViewer::NotifyOnButtonClicked);
						firstLine->AddChild(m_refreshList);
						m_refreshList->SetDock(RedGui::DOCK_Left);

						m_teleportToVehicle = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_teleportToVehicle->SetMargin(Box2(5, 5, 5, 0));
						m_teleportToVehicle->SetText(TXT("Teleport to vehicle"));
						m_teleportToVehicle->EventButtonClicked.Bind(this, &CDebugWindowVehicleViewer::NotifyOnButtonClicked);
						firstLine->AddChild(m_teleportToVehicle);
						m_teleportToVehicle->SetDock(RedGui::DOCK_Left);
						m_teleportToVehicle->SetEnabled(false);
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
						m_stopRotate->EventCheckedChanged.Bind(this, &CDebugWindowVehicleViewer::NotifyEventToggleButtonChange);
						secondLine->AddChild(m_stopRotate);
						m_stopRotate->SetDock(RedGui::DOCK_Left);

						m_releaseCamera = new RedGui::CRedGuiButton(0,0, 135, 25);
						m_releaseCamera->SetMargin(Box2(5, 5, 5, 0));
						m_releaseCamera->SetText(TXT("Release camera"));
						m_releaseCamera->SetToggleMode(true);
						m_releaseCamera->SetToggleValue(false);
						m_releaseCamera->EventCheckedChanged.Bind(this, &CDebugWindowVehicleViewer::NotifyEventToggleButtonChange);
						secondLine->AddChild(m_releaseCamera);
						m_releaseCamera->SetDock(RedGui::DOCK_Left);
					}
				}
			}

			m_vehiclesList = new RedGui::CRedGuiList(0, 0, 280, 500);
			m_vehiclesList->AppendColumn( TXT("Vehicles list"), 100 );
			m_vehiclesList->EventSelectedItem.Bind(this, &CDebugWindowVehicleViewer::NotifyEventSelectedItemChanged);
			m_vehiclesList->EventDoubleClickItem.Bind(this, &CDebugWindowVehicleViewer::NotifyOnButtonDoubleClicked);
			leftPanel->AddChild(m_vehiclesList);
			m_vehiclesList->SetDock(RedGui::DOCK_Fill);
		}

		m_vehicleInfoPanel = new RedGui::CRedGuiPanel(0, 0, 750, 550);
		m_vehicleInfoPanel->SetBackgroundColor(Color(0,0,0,0));
		m_vehicleInfoPanel->SetBorderVisible(false);
		m_vehicleInfoPanel->SetMargin(Box2(10, 10, 10, 10));
		AddChild(m_vehicleInfoPanel);
		m_vehicleInfoPanel->SetDock(RedGui::DOCK_Fill);

		m_vehicleInfoCategories = new RedGui::CRedGuiTab(0,0,0,0);
		m_vehicleInfoPanel->AddChild(m_vehicleInfoCategories);
		m_vehicleInfoCategories->SetDock(RedGui::DOCK_Fill);
		m_vehicleInfoCategories->AddTab(TXT("General"));

		// create general tab
		const Uint32 spaceBetweenText = 20;

		CRedGuiControl* generalTab = m_vehicleInfoCategories->GetTabAt(0);
		if(generalTab != nullptr)
		{
			Uint32 yPos = 10;

			m_generalTemplateName = new RedGui::CRedGuiLabel(0,yPos,0,0);
			generalTab->AddChild(m_generalTemplateName);
			yPos += spaceBetweenText;

			m_generalMemoryUsageByTemplate = new RedGui::CRedGuiLabel(0,yPos,0,0);
			generalTab->AddChild(m_generalMemoryUsageByTemplate);
			yPos += spaceBetweenText;

			m_generalTags = new RedGui::CRedGuiLabel(0,yPos,0,0);
			generalTab->AddChild(m_generalTags);
			yPos += spaceBetweenText;

			m_generalAppearance = new RedGui::CRedGuiLabel(0,yPos,0,0);
			generalTab->AddChild(m_generalAppearance);
			yPos += spaceBetweenText;

			m_generalState = new RedGui::CRedGuiLabel(0,yPos,0,0);
			generalTab->AddChild(m_generalState);
		}

		m_vehicleInfoCategories->SetActiveTab(0);
	}

	CDebugWindowVehicleViewer::~CDebugWindowVehicleViewer()
	{
		/*intentionally empty*/
	}

	void CDebugWindowVehicleViewer::CollectVehicles()
	{
		CWorld* world = GGame->GetActiveWorld();
		if ( world != nullptr )
		{
			CLayerGroup *worldLayerGroup = world->GetWorldLayers();
			TDynArray< CLayerInfo* > worldLayersInfos;
			worldLayerGroup->GetLayers( worldLayersInfos, false, true );
			for ( Uint32 i = 0; i < worldLayersInfos.Size(); i++ )
			{
				if ( worldLayersInfos[i]->IsLoaded() )
				{
					TDynArray< CEntity* > layerEntities;
					worldLayersInfos[i]->GetLayer()->GetEntities( layerEntities );
					for ( Uint32 entityNum = 0; entityNum < layerEntities.Size(); entityNum++ )
					{
						CEntity *entity = layerEntities[ entityNum ];
						CVehicleComponent* vehicleComponent = entity->FindComponent< CVehicleComponent >();
						if( vehicleComponent != nullptr )
						{
							m_vehicleComponents.PushBack( SVehicleEntry(vehicleComponent, entity->GetName(), entity ) );
							m_vehicleNames.PushBack( entity->GetFriendlyName() );
						}
					}
				}
			}

			// add vehicle components from dynamic layer
			CLayer* layer = world->GetDynamicLayer();
			if(layer != nullptr)
			{
				TDynArray< CEntity* > layerEntities;
				layer->GetEntities( layerEntities );
	
				for ( Uint32 entityNum = 0; entityNum < layerEntities.Size(); entityNum++ )
				{
					CEntity *entity = layerEntities[ entityNum ];
					CVehicleComponent* vehicleComponent = entity->FindComponent< CVehicleComponent >();
					if( vehicleComponent != nullptr )
					{
						m_vehicleComponents.PushBack( SVehicleEntry(vehicleComponent, entity->GetName(), entity ) );
						m_vehicleNames.PushBack( entity->GetFriendlyName() );
					}
				}
			}
		}
	}

	void CDebugWindowVehicleViewer::RefreshInformation()
	{
		// Clear list
		m_active = -1;
		m_vehicleComponents.Clear();
		m_vehicleNames.Clear();
		m_vehiclesList->RemoveAllItems();

		// Collect list of vehicles
		CollectVehicles();

		// Fill vehicles tree
		for(Uint32 i=0; i<m_vehicleNames.Size(); ++i)
		{
			String name = m_vehicleNames[i];
			name = name.StringAfter(TXT("::"), true);
			m_vehiclesList->AddItem(name);
		}

		// Use current camera target
		m_cameraPosition = m_cameraTarget;
	}

	void CDebugWindowVehicleViewer::OnWindowClosed( CRedGuiControl* control )
	{
		// Unpause game
		GGame->Unpause( TXT( "CDebugWindowVehicleViewer" ) );

		SetEnabled(false);
	}

	void CDebugWindowVehicleViewer::OnWindowOpened( CRedGuiControl* control )
	{
		SetEnabled(true);

		if( GGame != nullptr )
		{
			if( GGame->GetActiveWorld() != nullptr )
			{
				if( GGame->GetActiveWorld()->GetCameraDirector() != nullptr )
				{
					m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
				}
			}
		}		

		RefreshInformation();

		m_cameraPosition = m_cameraTarget;

		// Pause game
		GGame->Pause( TXT( "CDebugWindowVehicleViewer" ) );
	}

	void CDebugWindowVehicleViewer::NotifyOnFrameTick( RedGui::CRedGuiEventPackage& eventPackage, Float timeDelta )
	{
		if(m_teleportPlayer == true)
		{
			GGame->Pause( TXT( "CDebugWindowVehicleViewer" ) );
			m_teleportPlayer = false;
		}

		if(GetEnabled() == true)
		{
			if(m_pauseRotation == false)
			{
				m_cameraRotation += timeDelta * 30.0f;
			}
		}
	}

	void CDebugWindowVehicleViewer::NotifyOnButtonClicked( RedGui::CRedGuiEventPackage& eventPackage )
	{
		CRedGuiControl* sender = eventPackage.GetEventSender();

		if( sender == m_refreshList)
		{
			RefreshInformation();
		}
		else if( sender == m_teleportToVehicle)
		{
			if( GGame->IsActive() == true )
			{
				// teleport player to selected vehicle
				Int32 selectedIndex = m_vehiclesList->GetSelection();
				if(selectedIndex != -1)
				{
					CEntity* vehicle = m_vehicleComponents[selectedIndex].m_vehicleEntity;	
					GGame->GetPlayerEntity()->Teleport(vehicle, true);

					m_teleportPlayer = true;
					GGame->UnpauseAll();
				}
			}
		}
	}

	void CDebugWindowVehicleViewer::NotifyOnButtonDoubleClicked( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		SyncToNewVehicle(value);
	}

	void CDebugWindowVehicleViewer::NotifyEventSelectedItemChanged( RedGui::CRedGuiEventPackage& eventPackage, Int32 value )
	{
		RED_UNUSED( eventPackage );

		ShowInfoAboutVehicle(value);

		if(value != -1)
		{
			m_teleportToVehicle->SetEnabled(true);
		}
		else
		{
			m_teleportToVehicle->SetEnabled(false);
		}
	}

	void CDebugWindowVehicleViewer::NotifyViewportInput( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
	{
		RED_UNUSED( eventPackage );

		if(m_releaseCamera->GetToggleValue() == true)
		{
			CGameFreeCamera& freeCamera = const_cast<CGameFreeCamera&>(GGame->GetFreeCamera());
			freeCamera.ProcessInput(key, action, data);
		}
	}

	void CDebugWindowVehicleViewer::NotifyEventToggleButtonChange( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
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

	void CDebugWindowVehicleViewer::NotifyViewportCalculateCamera( RedGui::CRedGuiEventPackage& eventPackage, IViewport* view, CRenderCamera& camera )
	{
		RED_UNUSED( eventPackage );

		if (GetEnabled() == true && m_releaseCamera->GetToggleValue() == false )
		{
			// Show the selected actor :)
			if ( m_active >= 0 )
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

	void CDebugWindowVehicleViewer::ShowInfoAboutVehicle( Int32 value )
	{
		const SVehicleEntry& vehicleEntry = m_vehicleComponents[value];
		SetGeneralInformation(vehicleEntry);
	}

	Vector CDebugWindowVehicleViewer::CalculateCameraOrbit( CEntity* vehicle )
	{
		// Start with center of the bounding box + some offset
		Uint32 numMeshes = 0;
		Vector pos = Vector::ZEROS;
		for ( ComponentIterator< CMeshComponent > it( vehicle ); it; ++it )
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

		return vehicle->GetPosition();
	}

	void CDebugWindowVehicleViewer::SyncToNewVehicle( Int32 npcIndex )
	{
		// Reset vertical offset
		m_verticalOffset = 0;

		// Reset
		m_activeVehicleName = TXT("");
		//m_cameraRotation = 0.0f;

		// Reset camera
		m_cameraTarget = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();

		// Is new shit set ?
		m_active = npcIndex;
		if ( m_active != -1 )
		{
			// Cache name
			m_activeVehicleName = m_vehicleNames[ npcIndex ];

			// Reset rotation
			CEntity* vehicle = m_vehicleComponents[ npcIndex ].m_vehicleEntity;
			if ( vehicle != nullptr )
			{
				//m_cameraRotation = npc->GetWorldYaw() + 180.0f;
				m_cameraTarget = CalculateCameraOrbit( vehicle );
				m_cameraPosition = m_cameraTarget;
			}
		}
	}

	void CDebugWindowVehicleViewer::SetGeneralInformation( const SVehicleEntry& entry )
	{
		// Template name
		String templateName = (entry.m_vehicleEntity != nullptr) ? entry.m_vehicleEntity->GetFriendlyName() : TXT("Unknown");
		m_generalTemplateName->SetText(String::Printf( TXT("Template: '%ls'"), templateName.AsChar() ), Color::WHITE);

		// Mem usage
		Uint32 memoryUsageByTemplate = CObjectMemoryAnalizer::CalcObjectSize( entry.m_vehicleEntity ) / 1024;
		m_generalMemoryUsageByTemplate->SetText(String::Printf( TXT("Memory usage by template: '%dkB'"), memoryUsageByTemplate ), Color::YELLOW);

		// Tag
		String tags = entry.m_vehicleEntity->GetTags().ToString();
		m_generalTags->SetText(String::Printf( TXT("Tags: '%ls'"), tags.AsChar() ), Color::WHITE );

		// Appearance
		CAppearanceComponent* appearanceComponent = entry.m_vehicleEntity->FindComponent<CAppearanceComponent>();
		if ( appearanceComponent != nullptr )
		{
			String appearance = appearanceComponent->GetAppearance().AsString();
			m_generalAppearance->SetText( String::Printf( TXT("Appearance: '%ls'"), appearance.AsChar() ), Color::WHITE );
		}
		else
		{
			m_generalAppearance->SetText( TXT("Without appearance components"), Color::WHITE );
		}

		// State
		String stateText = String::Printf( TXT("State: '%ls'"), entry.m_vehicleEntity->GetCurrentStateName().AsString().AsChar() );
		m_generalState->SetText( String::Printf( TXT("State: '%ls'"), stateText.AsChar() ), Color::WHITE );
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
