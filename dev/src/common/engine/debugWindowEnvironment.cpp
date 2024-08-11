/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_RED_GUI
#ifndef NO_DEBUG_WINDOWS

#include "weatherManager.h"

#include "redGuiCheckBox.h"
#include "redGuiList.h"
#include "redGuiTreeView.h"
#include "redGuiTreeNode.h"
#include "redGuiComboBox.h"
#include "redGuiGroupBox.h"
#include "redGuiLabel.h"
#include "redGuiSlider.h"
#include "redGuiPanel.h"
#include "redGuiScrollPanel.h"
#include "redGuiTab.h"
#include "redGuiButton.h"
#include "redGuiManager.h"
#include "debugWindowEnvironment.h"
#include "viewport.h"
#include "game.h"
#include "gameTime.h"
#include "../core/resource.h"
#include "environmentComponentArea.h"
#include "world.h"
#include "layerGroup.h"
#include "dynamicLayer.h"
#include "environmentDefinition.h"
#include "layerInfo.h"
#include "entity.h"

namespace DebugWindows
{
	CDebugWindowEnvironment::CDebugWindowEnvironment()
		: RedGui::CRedGuiWindow( 100, 100, 800, 650 )
	{
		SetCaption(TXT("Environment settings"));

		GRedGui::GetInstance().EventTick.Bind( this, &CDebugWindowEnvironment::NotifyOnTickEvent );

		// create middle panel
		RedGui::CRedGuiPanel* leftPanel = new RedGui::CRedGuiPanel( 0, 0, 300, 200 );
		leftPanel->SetDock( RedGui::DOCK_Left );
		AddChild( leftPanel );
		leftPanel->SetMargin( Box2(5, 5, 5, 5) );
		{
			RedGui::CRedGuiGroupBox* daycyclePanel = new RedGui::CRedGuiGroupBox( 0, 0, 200, 125 );
			daycyclePanel->SetDock( RedGui::DOCK_Top );
			leftPanel->AddChild( daycyclePanel );
			daycyclePanel->SetText( TXT("Daycycle info") );
			{
				m_activeTimeLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 20);
				m_activeTimeLabel->SetMargin( Box2(5, 5, 5, 5) );
				m_activeTimeLabel->SetText(TXT("Current time: "));
				daycyclePanel->AddChild(m_activeTimeLabel);
				m_activeTimeLabel->SetDock(RedGui::DOCK_Top);

				m_fakeTimeLable = new RedGui::CRedGuiLabel( 0, 0, 150, 25 );
				m_fakeTimeLable->SetDock( RedGui::DOCK_Top );
				daycyclePanel->AddChild( m_fakeTimeLable );
				m_fakeTimeLable->SetMargin( Box2(5, 5, 5, 5) );
				m_fakeTimeLable->SetText( TXT("Fake time: ") );

				m_enableFakeDaycycle = new RedGui::CRedGuiCheckBox( 0, 0, 150, 25 );
				m_enableFakeDaycycle->SetDock( RedGui::DOCK_Top );
				m_enableFakeDaycycle->SetText( TXT("Enable fake daycycle") );
				daycyclePanel->AddChild( m_enableFakeDaycycle );
				m_enableFakeDaycycle->SetMargin( Box2(5, 5, 5, 5) );
				m_enableFakeDaycycle->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFakeDaycycleEnabled );

				m_fakeDaycycle = new RedGui::CRedGuiSlider( 0, 0, 200, 25 );
				m_fakeDaycycle->SetDock( RedGui::DOCK_Top );
				daycyclePanel->AddChild( m_fakeDaycycle );
				m_fakeDaycycle->SetMargin( Box2(5, 5, 5, 5) );
				m_fakeDaycycle->SetMinValue( 0 );
				m_fakeDaycycle->SetMaxValue( 400 );
				m_fakeDaycycle->EventScroll.Bind( this, &CDebugWindowEnvironment::NotifyFakeDaycycleChanged );
			}

			// create weather panel
			RedGui::CRedGuiGroupBox* weatherPanel = new RedGui::CRedGuiGroupBox( 0, 0, 200, 100 );
			weatherPanel->SetDock( RedGui::DOCK_Top );
			leftPanel->AddChild( weatherPanel );
			weatherPanel->SetText( TXT("Weather info") );
			{
				m_currentWeatherLabel = new RedGui::CRedGuiLabel( 0, 0, 140, 25 );
				m_currentWeatherLabel->SetDock( RedGui::DOCK_Top );
				weatherPanel->AddChild( m_currentWeatherLabel );
				m_currentWeatherLabel->SetMargin( Box2(5, 5, 5, 5) );
				m_currentWeatherLabel->SetText( TXT("Current weather: ") );

				m_targetWeatherLabel = new RedGui::CRedGuiLabel( 0, 0, 150, 25 );
				m_targetWeatherLabel->SetDock( RedGui::DOCK_Top );
				weatherPanel->AddChild( m_targetWeatherLabel );
				m_targetWeatherLabel->SetMargin( Box2(5, 5, 5, 5) );
				m_targetWeatherLabel->SetText( TXT("Target weather: ") );

				m_weatherPresets = new RedGui::CRedGuiComboBox( 0, 0, 150, 20 );
				m_weatherPresets->SetDock( RedGui::DOCK_Top );
				weatherPanel->AddChild( m_weatherPresets );
				m_weatherPresets->SetMargin( Box2(5, 5, 5, 5) );
				m_weatherPresets->EventSelectedIndexChanged.Bind( this, &CDebugWindowEnvironment::NotifyWeatherPresetSelected );
			}

			// create settings panel
			RedGui::CRedGuiGroupBox* settingsPanel = new RedGui::CRedGuiGroupBox( 0, 0, 200, 125 );
			settingsPanel->SetDock( RedGui::DOCK_Fill );
			leftPanel->AddChild( settingsPanel );
			settingsPanel->SetText( TXT("Settings") );
			{
				m_envAreaDebugFlag = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_envAreaDebugFlag->SetText( TXT("Show env areas") );
				m_envAreaDebugFlag->SetDock( RedGui::DOCK_Top );
				m_envAreaDebugFlag->SetMargin(Box2(3, 3, 0, 3));
				m_envAreaDebugFlag->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_envAreaDebugFlag );

				m_envProbeInstancesDebugFlag = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_envProbeInstancesDebugFlag->SetText( TXT("Show env probes instances") );
				m_envProbeInstancesDebugFlag->SetDock( RedGui::DOCK_Top );
				m_envProbeInstancesDebugFlag->SetMargin(Box2(3, 3, 0, 3));
				m_envProbeInstancesDebugFlag->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_envProbeInstancesDebugFlag );

				m_envProbeOverlayDebugFlag = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_envProbeOverlayDebugFlag->SetText( TXT("Show env probes overlay") );
				m_envProbeOverlayDebugFlag->SetDock( RedGui::DOCK_Top );
				m_envProbeOverlayDebugFlag->SetMargin(Box2(3, 3, 0, 3));
				m_envProbeOverlayDebugFlag->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_envProbeOverlayDebugFlag );

				m_instantAdaptionOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_instantAdaptionOption->SetText( TXT("Instant adaption") );
				m_instantAdaptionOption->SetDock( RedGui::DOCK_Top );
				m_instantAdaptionOption->SetMargin(Box2(3, 3, 0, 3));
				m_instantAdaptionOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_instantAdaptionOption );

				m_trajectoryDisplayOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_trajectoryDisplayOption->SetText( TXT("Trajectory display") );
				m_trajectoryDisplayOption->SetDock( RedGui::DOCK_Top );
				m_trajectoryDisplayOption->SetMargin(Box2(3, 3, 0, 3));
				m_trajectoryDisplayOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_trajectoryDisplayOption );

				m_envProbeInstantUpdate = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_envProbeInstantUpdate->SetText( TXT("EnvProbe instant update") );
				m_envProbeInstantUpdate->SetDock( RedGui::DOCK_Top );
				m_envProbeInstantUpdate->SetMargin(Box2(3, 3, 0, 3));
				m_envProbeInstantUpdate->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_envProbeInstantUpdate );

				m_allowEnvProbeUpdateOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowEnvProbeUpdateOption->SetText( TXT("Allow EnvProbe update") );
				m_allowEnvProbeUpdateOption->SetDock( RedGui::DOCK_Top );
				m_allowEnvProbeUpdateOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowEnvProbeUpdateOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowEnvProbeUpdateOption );

				m_allowBloomOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowBloomOption->SetText( TXT("Allow bloom") );
				m_allowBloomOption->SetDock( RedGui::DOCK_Top );
				m_allowBloomOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowBloomOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowBloomOption );

				m_allowColorBalanceOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowColorBalanceOption->SetText( TXT("Allow color balance") );
				m_allowColorBalanceOption->SetDock( RedGui::DOCK_Top );
				m_allowColorBalanceOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowColorBalanceOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowColorBalanceOption );

				m_allowAntialiasingOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowAntialiasingOption->SetText( TXT("Allow antialiasing") );
				m_allowAntialiasingOption->SetDock( RedGui::DOCK_Top );
				m_allowAntialiasingOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowAntialiasingOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowAntialiasingOption );

				m_allowGlobalFogOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowGlobalFogOption->SetText( TXT("Allow global fog") );
				m_allowGlobalFogOption->SetDock( RedGui::DOCK_Top );
				m_allowGlobalFogOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowGlobalFogOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowGlobalFogOption );

				m_allowDepthOfFieldOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowDepthOfFieldOption->SetText( TXT("Allow depth of field") );
				m_allowDepthOfFieldOption->SetDock( RedGui::DOCK_Top );
				m_allowDepthOfFieldOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowDepthOfFieldOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowDepthOfFieldOption );

				m_allowSSAOOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowSSAOOption->SetText( TXT("Allow SSAOO") );
				m_allowSSAOOption->SetDock( RedGui::DOCK_Top );
				m_allowSSAOOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowSSAOOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowSSAOOption );

				m_allowCloudsShadowOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowCloudsShadowOption->SetText( TXT("Allow clouds shadow") );
				m_allowCloudsShadowOption->SetDock( RedGui::DOCK_Top );
				m_allowCloudsShadowOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowCloudsShadowOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowCloudsShadowOption );

				m_allowVignetteOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowVignetteOption->SetText( TXT("Allow vignette") );
				m_allowVignetteOption->SetDock( RedGui::DOCK_Top );
				m_allowVignetteOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowVignetteOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowVignetteOption );

				m_allowWaterShaderOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_allowWaterShaderOption->SetText( TXT("Allow water shader") );
				m_allowWaterShaderOption->SetDock( RedGui::DOCK_Top );
				m_allowWaterShaderOption->SetMargin(Box2(3, 3, 0, 3));
				m_allowWaterShaderOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_allowWaterShaderOption );

				m_forceCutsceneDOFModeOption = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
				m_forceCutsceneDOFModeOption->SetText( TXT("Force cutscene DOF mode") );
				m_forceCutsceneDOFModeOption->SetDock( RedGui::DOCK_Top );
				m_forceCutsceneDOFModeOption->SetMargin(Box2(3, 3, 0, 3));
				m_forceCutsceneDOFModeOption->EventCheckedChanged.Bind( this, &CDebugWindowEnvironment::NotifyFiltersChanged );
				settingsPanel->AddChild( m_forceCutsceneDOFModeOption );
			}
		}

		// create right panel
		RedGui::CRedGuiPanel* rightPanel = new RedGui::CRedGuiPanel( 0, 0, 300, 500);
		rightPanel->SetDock( RedGui::DOCK_Fill );
		AddChild( rightPanel );
		rightPanel->SetMargin( Box2(5, 5, 5, 5) );
		{
			m_activeEnvs = new RedGui::CRedGuiList( 0, 0, 100, 150 );
			m_activeEnvs->SetDock( RedGui::DOCK_Top );
			rightPanel->AddChild( m_activeEnvs );
			m_activeEnvs->SetMargin( Box2(5, 5, 5, 5) );
			m_activeEnvs->SetSelectionMode( RedGui::SM_None );
			m_activeEnvs->AppendColumn( TXT("Active env definitions"), 150 );

			m_refreshInformation = new RedGui::CRedGuiButton( 0, 0, 100, 25 );
			m_refreshInformation->SetDock( RedGui::DOCK_Bottom );
			rightPanel->AddChild( m_refreshInformation );
			m_refreshInformation->SetMargin( Box2(5, 5, 5, 5) );
			m_refreshInformation->SetText( TXT("Refresh loaded definitions") );
			m_refreshInformation->EventButtonClicked.Bind( this, &CDebugWindowEnvironment::NotifyRefreshLoadedDefinitions );

			m_allLoadedEnvs = new RedGui::CRedGuiList( 0, 0, 100, 100 );
			m_allLoadedEnvs->SetDock( RedGui::DOCK_Fill );
			rightPanel->AddChild( m_allLoadedEnvs );
			m_allLoadedEnvs->SetMargin( Box2(5, 5, 5, 5) );
			m_allLoadedEnvs->SetSelectionMode( RedGui::SM_None );
			m_allLoadedEnvs->AppendColumn( TXT("All loaded definitions"), 100 );
		}
	}

	CDebugWindowEnvironment::~CDebugWindowEnvironment()
	{
		/*intentionally empty*/
	}

	void CDebugWindowEnvironment::SetValuesFromEnvToGui()
	{
		CGameEnvironmentParams&	gameParams	= m_envManager->GetGameEnvironmentParams();

		// synchronize fake daycycle slider
		m_fakeDaycycle->SetValue( Lerp( Clamp(gameParams.m_dayCycleOverride.m_fakeDayCycleHour/24.f, 0.f, 1.f), m_fakeDaycycle->GetMinValue(), m_fakeDaycycle->GetMaxValue()), true );

		// synchronize checkboxes
		m_enableFakeDaycycle->SetChecked( gameParams.m_dayCycleOverride.m_fakeDayCycleEnable, true );

		m_instantAdaptionOption->SetChecked( gameParams.m_displaySettings.m_enableInstantAdaptation, true );
		m_trajectoryDisplayOption->SetChecked( gameParams.m_displaySettings.m_enableGlobalLightingTrajectory, true );
		m_envProbeInstantUpdate->SetChecked( gameParams.m_displaySettings.m_enableEnvProbeInstantUpdate, true );
		m_allowEnvProbeUpdateOption->SetChecked( gameParams.m_displaySettings.m_allowEnvProbeUpdate, true );
		m_allowBloomOption->SetChecked( gameParams.m_displaySettings.m_allowBloom, true );
		m_allowColorBalanceOption->SetChecked( gameParams.m_displaySettings.m_allowColorMod, true );
		m_allowAntialiasingOption->SetChecked( gameParams.m_displaySettings.m_allowAntialiasing, true );
		m_allowGlobalFogOption->SetChecked( gameParams.m_displaySettings.m_allowGlobalFog, true );
		m_allowDepthOfFieldOption->SetChecked( gameParams.m_displaySettings.m_allowDOF, true );
		m_allowSSAOOption->SetChecked( gameParams.m_displaySettings.m_allowSSAO, true );
		m_allowCloudsShadowOption->SetChecked( gameParams.m_displaySettings.m_allowCloudsShadow, true );
		m_allowVignetteOption->SetChecked( gameParams.m_displaySettings.m_allowVignette, true );
		m_allowWaterShaderOption->SetChecked( gameParams.m_displaySettings.m_allowWaterShader, true );
		m_forceCutsceneDOFModeOption->SetChecked( gameParams.m_displaySettings.m_forceCutsceneDofMode, true );

		// set rendering flags
		IViewport* gameViewport = GGame->GetViewport();
		m_envAreaDebugFlag->SetChecked( gameViewport->GetRenderingMask()[ SHOW_Areas ], true );
		m_envProbeInstancesDebugFlag->SetChecked( gameViewport->GetRenderingMask()[ SHOW_EnvProbesInstances ], true );
		m_envProbeOverlayDebugFlag->SetChecked( gameViewport->GetRenderingMask()[ SHOW_EnvProbesOverlay ], true );

		//
		const TDynArray< SWeatherCondition >& weatherConditions = m_envManager->GetWeatherManager()->GetWeatherConditions();
		m_weatherPresets->ClearAllItems();
		for ( auto it=weatherConditions.Begin(); it != weatherConditions.End(); ++it )
		{
			const SWeatherCondition& condition = *it;
			m_weatherPresets->AddItem( condition.m_name.AsString() );
		}
	}

	void CDebugWindowEnvironment::OnWindowOpened( CRedGuiControl* control )
	{
		CWorld* world = GGame->GetActiveWorld();
		if( world != nullptr )
		{
			m_envManager = world->GetEnvironmentManager();
			GetClientControl()->SetEnabled( true );

			SetValuesFromEnvToGui();
			RedGui::CRedGuiEventPackage eventPackage( nullptr );
			NotifyRefreshLoadedDefinitions( eventPackage );
		}
		else
		{
			GetClientControl()->SetEnabled( false );
			RED_LOG_ERROR( RED_LOG_CHANNEL( DebugWindows ), TXT("No world loaded. Environment window cannot get environement managet from world." ) );
		}
	}

	void CDebugWindowEnvironment::NotifyFakeDaycycleChanged( RedGui::CRedGuiEventPackage& eventPackage, Float value )
	{
		CGameEnvironmentParams&	gameParams	= m_envManager->GetGameEnvironmentParams();
		gameParams.m_dayCycleOverride.m_fakeDayCycleHour = Lerp( (m_fakeDaycycle->GetValue() - m_fakeDaycycle->GetMinValue() ) / Max( 1.0f, m_fakeDaycycle->GetMaxValue() - m_fakeDaycycle->GetMinValue() ), 0.f, 24.f );
		m_envManager->SetGameEnvironmentParams( gameParams );

		// Update fake day cycle progress value
		{
			Int32 seconds = (Int32) Lerp( (m_fakeDaycycle->GetValue() - m_fakeDaycycle->GetMinValue()) / Max( 1.0f, m_fakeDaycycle->GetMaxValue() - m_fakeDaycycle->GetMinValue() ), 0.f, 24.f * 60.f * 60.f );
			Int32 hour = seconds / (60 * 60);
			Int32 minute = seconds / 60 % 60;
			Int32 second = seconds % 60;		 
			m_fakeTimeLable->SetText( String::Printf( TXT("Fake time: %02i:%02i:%02i"), (int)hour, (int)minute, (int)second).AsChar() );
		}
	}

	void CDebugWindowEnvironment::NotifyFakeDaycycleEnabled( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CGameEnvironmentParams&	gameParams	= m_envManager->GetGameEnvironmentParams();
		gameParams.m_dayCycleOverride.m_fakeDayCycleEnable = value;
		m_envManager->SetGameEnvironmentParams( gameParams );
	}

	void CDebugWindowEnvironment::NotifyFiltersChanged( RedGui::CRedGuiEventPackage& eventPackage, Bool value )
	{
		CGameEnvironmentParams&	gameParams	= m_envManager->GetGameEnvironmentParams();

		gameParams.m_displaySettings.m_enableInstantAdaptation = m_instantAdaptionOption->GetChecked();
		gameParams.m_displaySettings.m_enableGlobalLightingTrajectory = m_trajectoryDisplayOption->GetChecked();
		gameParams.m_displaySettings.m_enableEnvProbeInstantUpdate = m_envProbeInstantUpdate->GetChecked();
		gameParams.m_displaySettings.m_allowEnvProbeUpdate = m_allowEnvProbeUpdateOption->GetChecked();
		gameParams.m_displaySettings.m_allowBloom = m_allowBloomOption->GetChecked();
		gameParams.m_displaySettings.m_allowColorMod = m_allowColorBalanceOption->GetChecked();
		gameParams.m_displaySettings.m_allowAntialiasing = m_allowAntialiasingOption->GetChecked();
		gameParams.m_displaySettings.m_allowGlobalFog = m_allowGlobalFogOption->GetChecked(); 
		gameParams.m_displaySettings.m_allowDOF = m_allowDepthOfFieldOption->GetChecked();
		gameParams.m_displaySettings.m_allowSSAO = m_allowSSAOOption->GetChecked();
		gameParams.m_displaySettings.m_allowCloudsShadow = m_allowCloudsShadowOption->GetChecked();
		gameParams.m_displaySettings.m_allowVignette = m_allowVignetteOption->GetChecked();
		gameParams.m_displaySettings.m_allowWaterShader = m_allowWaterShaderOption->GetChecked();
		gameParams.m_displaySettings.m_forceCutsceneDofMode = m_forceCutsceneDOFModeOption->GetChecked();

		m_envManager->SetGameEnvironmentParams( gameParams );

		// set rendering flags
		IViewport* gameViewport = GGame->GetViewport();
		if ( m_envAreaDebugFlag->GetChecked() == true )
		{
			gameViewport->SetRenderingMask( SHOW_Areas );
		}
		else
		{
			gameViewport->ClearRenderingMask( SHOW_Areas );
		}
		if ( m_envProbeInstancesDebugFlag->GetChecked() == true )
		{
			gameViewport->SetRenderingMask( SHOW_EnvProbesInstances );
		}
		else
		{
			gameViewport->ClearRenderingMask( SHOW_EnvProbesInstances );
		}
		if ( m_envProbeOverlayDebugFlag->GetChecked() == true )
		{
			gameViewport->SetRenderingMask( SHOW_EnvProbesOverlay );
		}
		else
		{
			gameViewport->ClearRenderingMask( SHOW_EnvProbesOverlay );
		}
	}

	void CDebugWindowEnvironment::NotifyOnTickEvent( RedGui::CRedGuiEventPackage& eventPackage, Float deltaTime )
	{
		if( GetVisible() == true )
		{
			if(m_envManager != nullptr )
			{
				// set current game time
				GameTime time = m_envManager->GetCurrentGameTime();
				Int32 hour = time.Hours();
				Int32 minute = time.Minutes();
				Int32 second = time.Seconds();
				m_activeTimeLabel->SetText( String::Printf( TXT("Current time: %02i:%02i:%02i"), (int)hour, (int)minute, (int)second) );
	
				// update weather labels
				const CWeatherManager* weatherManager = m_envManager->GetWeatherManager();
				if( weatherManager != nullptr )
				{
					Float blendRatio = weatherManager->GetBlendingRatio();
					m_currentWeatherLabel->SetText( String::Printf( TXT("Current weather: %s - %d%%"), weatherManager->GetCurrentWeatherCondition().m_name.AsString().AsChar(), (int)( (1.0f - blendRatio) * 100.0f ) ) );
					m_targetWeatherLabel->SetText( String::Printf( TXT("Target weather: %s - %d%%"), weatherManager->GetTargetWeatherCondition().m_name.AsString().AsChar(), (int)( blendRatio * 100.0f ) ) );
				}
	
#ifndef NO_DEBUG_WINDOWS
				// update blending env list
				const CEnvironmentManager::TAreaEnvironmentsArray& envs = m_envManager->GetActiveEnvironments();
	
				m_activeEnvs->RemoveAllItems();
				for ( auto it=envs.Begin(); it != envs.End(); ++it )
				{
					const SEnvManagerAreaEnvData& env = *it;
					String description = String::Printf( TXT("[P%d] %d%% "), env.priority, (int)( env.appliedBlendFactor*100.0f ) );
					if ( -1 != env.appliedMostImportantFactor )
					{
						description += String::Printf( TXT(" (important %d%%)"), (int)( env.appliedMostImportantFactor*100.0f ) );
					}
					switch ( env.timeBlendState )
					{
					case ETBS_BlendIn:
						description += String::Printf( TXT(" (blend in %d%%)"), (int)( env.timeBlendFactor*100.0f ) );
						break;
					case ETBS_BlendOut:
						description += String::Printf( TXT(" (blend out %d%%)"), (int)( env.timeBlendFactor*100.0f ) );
						break;
					}
					String pathToFile = env.pathToEnvDefinition;
					pathToFile.ReplaceAll( TXT("environment\\definitions\\"), TXT("") );
					description += String::Printf( TXT("     %s"), pathToFile.AsChar() );
					//if ( env.areaComponent )
					//{
					//	description += String::Printf( TXT(" [%s]"), env.areaComponent->GetFriendlyName().AsChar() );
					//}
					m_activeEnvs->AddItem( description );
				}
#endif	// !NO_EDITOR
			}
		}
	}

	void CDebugWindowEnvironment::NotifyWeatherPresetSelected( RedGui::CRedGuiEventPackage& eventPackage, Int32 selectedItem )
	{
		RED_UNUSED( eventPackage );

		// Get selected name
		String selectedName = m_weatherPresets->GetSelectedItemName();
		if ( selectedName.Empty() == true )
		{
			return;
		}

		CWeatherManager* weatherManager = m_envManager->GetWeatherManager();
		if( weatherManager == nullptr )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( DebugWindows ), TXT("No weather manager in environment manager, that is impossible!") );
			return;
		}

		// Request a weather change
		if ( weatherManager->RequestWeatherChangeTo( CName( selectedName ) ) == false )
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( DebugWindows ), TXT("Failed to change the weather condition - check if the condition names are correct!") );
			GRedGui::GetInstance().MessageBox( TXT("Failed to change the weather condition - check if the condition names are correct!"), TXT("Weather Failure"), RedGui::MESSAGEBOX_Error );
			return;
		}
	}

	void CDebugWindowEnvironment::NotifyRefreshLoadedDefinitions( RedGui::CRedGuiEventPackage& eventPackage )
	{
		m_allLoadedEnvs->RemoveAllItems();

		CWorld* world = GGame->GetActiveWorld();
		if( world != nullptr )
		{
			String envPath = String::EMPTY;

			// get global env
			if ( nullptr != world->GetEnvironmentParameters().m_environmentDefinition )
			{
				envPath = world->GetEnvironmentParameters().m_environmentDefinition->GetDepotPath();
				envPath.ReplaceAll( TXT("environment\\definitions\\"), TXT("") );
				m_allLoadedEnvs->AddItem( envPath );
			}

			// get scenes env
			if ( nullptr != world->GetEnvironmentParameters().m_scenesEnvironmentDefinition )
			{
				envPath = world->GetEnvironmentParameters().m_scenesEnvironmentDefinition->GetDepotPath();
				envPath.ReplaceAll( TXT("environment\\definitions\\"), TXT("") );
				m_allLoadedEnvs->AddItem( envPath );
			}

			// get envs from weather
			const TDynArray< SWeatherCondition >& conditions = m_envManager->GetWeatherManager()->GetWeatherConditions();
			for( Uint32 i=0; i<conditions.Size(); ++i )
			{
				if( conditions[i].m_environmentDefinition != nullptr )
				{
					envPath = conditions[i].m_environmentDefinition->GetDepotPath();
					envPath.ReplaceAll( TXT("environment\\definitions\\"), TXT("") );
					m_allLoadedEnvs->AddItem( envPath );
				}
			}

			// get envs from env components on the world
			TDynArray< CAreaEnvironmentComponent* > components;
			CollectVehicles( components );
			for( Uint32 i=0; i<components.Size(); ++i )
			{
				if( components[i]->GetEnvironmentDefinition() != nullptr )
				{
					envPath = components[i]->GetEnvironmentDefinition()->GetDepotPath();
					envPath.ReplaceAll( TXT("environment\\definitions\\"), TXT("") );
					m_allLoadedEnvs->AddItem( envPath );
				}
			}
		}		
	}

	void CDebugWindowEnvironment::CollectVehicles( TDynArray< CAreaEnvironmentComponent* >& components )
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
						CAreaEnvironmentComponent* envComponent = entity->FindComponent< CAreaEnvironmentComponent >();
						if( envComponent != nullptr )
						{
							components.PushBack( envComponent );
						}
					}
				}
			}

			// add env components from dynamic layer
			CLayer* layer = world->GetDynamicLayer();
			if(layer != nullptr)
			{
				TDynArray< CEntity* > layerEntities;
				layer->GetEntities( layerEntities );

				for ( Uint32 entityNum = 0; entityNum < layerEntities.Size(); entityNum++ )
				{
					CEntity *entity = layerEntities[ entityNum ];
					CAreaEnvironmentComponent* envComponent = entity->FindComponent< CAreaEnvironmentComponent >();
					if( envComponent != nullptr )
					{
						components.PushBack( envComponent );
					}
				}
			}
		}
	}

}	// namespace DebugWindows

#endif	// NO_DEBUG_WINDOWS
#endif	// NO_RED_GUI
