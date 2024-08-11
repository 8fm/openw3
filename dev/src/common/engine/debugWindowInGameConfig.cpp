/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_DEBUG_WINDOWS
#include "debugWindowInGameConfig.h"
#include "inGameConfigInterface.h"
#include "inGameConfig.h"

#include "redGuiTab.h"
#include "redGuiScrollPanel.h"
#include "redGuiGroupBox.h"
#include "redGuiGridLayout.h"
#include "redGuiButton.h"
#include "redGuiCheckBox.h"
#include "redGuiComboBox.h"
#include "redGuiSlider.h"
#include "redGuiLabel.h"
#include "redGuiPanel.h"
#include "redGuiSpin.h"

#include "baseEngine.h"
#include "renderSettings.h"

namespace DebugWindows
{
	CDebugWindowInGameConfig::CDebugWindowInGameConfig()
		: RedGui::CRedGuiWindow( 200, 200, 800, 800 )
	{
		SetCaption( TXT("In game configuration") );

		m_refreshing = false;
		m_initializing = true;

		CreateControls();
	}

	CDebugWindowInGameConfig::~CDebugWindowInGameConfig()
	{

	}

	void CDebugWindowInGameConfig::CreateControls()
	{
		m_initializing = true;

		auto mainPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 25 );
		mainPanel->SetDock( RedGui::DOCK_Fill );
		mainPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
		this->AddChild( mainPanel );
		{
			// Create main tabs
			m_mainTab = new RedGui::CRedGuiTab( 0, 0, 100, 100 );
			m_mainTab->SetDock( RedGui::DOCK_Fill );
			m_mainTab->SetMargin( Box2( 5, 5, 5, 5 ) );
			m_mainTab->EventTabChanged.Bind( this, &CDebugWindowInGameConfig::OnChangeTab );
			mainPanel->AddChild( m_mainTab );

			// Create all config group GUI
			TDynArray< InGameConfig::IConfigGroup* > configGroups;
			GInGameConfig::GetInstance().ListAllConfigGroups( configGroups );

			Int32 tabIndex = 0;
			for( auto group : configGroups )
			{
				// Create new tab
				if( group->IsVisible() == true )
				{
					m_mainTab->AddTab( group->GetDisplayName() );

					TDynArray< InGameConfig::SConfigPresetOption > presetOptions;
					group->ListPresets( presetOptions );

					if( presetOptions.Empty() )
					{
						// Add group to that tab
						AddConfigGroupToPanel( m_mainTab, tabIndex, group );
					}
					else
					{
						AddConfigGroupPresetableToPanel( m_mainTab, tabIndex, group );
					}

					tabIndex++;
				}
			}

			// Create bottom panel for general buttons
			auto commonPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 25 );
			commonPanel->SetDock( RedGui::DOCK_Bottom );
			commonPanel->SetMargin( Box2( 5, 5, 5, 5 ) );
			mainPanel->AddChild( commonPanel );
			{
				// Create save config button
				auto saveConfigButton = new RedGui::CRedGuiButton( 0, 0, 100, 25 );
				saveConfigButton->SetText( TXT("Save config") );
				saveConfigButton->SetDock( RedGui::DOCK_Right );
				saveConfigButton->EventButtonClicked.Bind( this, &CDebugWindowInGameConfig::OnSaveConfigButtonClicked );
				commonPanel->AddChild( saveConfigButton );
				
				// Create apply settings button
				auto applyButton = new RedGui::CRedGuiButton( 0, 0, 100, 25 );
				applyButton->SetText( TXT("Refresh engine") );
				applyButton->SetDock( RedGui::DOCK_Right );
				applyButton->EventButtonClicked.Bind( this, &CDebugWindowInGameConfig::OnApplySettingsButtonClicked );
				commonPanel->AddChild( applyButton );

				// Create apply settings button
				auto resetButton = new RedGui::CRedGuiButton( 0, 0, 100, 25 );
				resetButton->SetText( TXT("Reset configs") );
				resetButton->SetDock( RedGui::DOCK_Right );
				resetButton->EventButtonClicked.Bind( this, &CDebugWindowInGameConfig::OnResetConfigButtonClicked );
				commonPanel->AddChild( resetButton );
			}
		}

		m_initializing = false;
	}

	void CDebugWindowInGameConfig::AddConfigGroupToPanel( RedGui::CRedGuiTab* tabs, Int32 tabIndex, InGameConfig::IConfigGroup* group )
	{
		TDynArray< InGameConfig::IConfigVar* > configVarTable;
		group->ListConfigVars( configVarTable );

		// Add grid layout to that tab
		RedGui::CRedGuiGridLayout* grid = new RedGui::CRedGuiGridLayout( 0, 0, 100, 28 * configVarTable.Size() );
		grid->SetDock( RedGui::DOCK_Top );
		grid->SetDimensions( 2, configVarTable.Size() );
		tabs->GetTabAt( tabIndex )->AddChild( grid );

		// Add controls for config vars
		for( auto configVar : configVarTable)
		{
			if( configVar->IsVisible() )
			{
				AddConfigVarControlsToGrid( grid, configVar );
			}
		}
	}

	void CDebugWindowInGameConfig::AddConfigGroupPresetableToPanel( RedGui::CRedGuiTab* tabs, Int32 tabIndex, InGameConfig::IConfigGroup* group )
	{
		// Add preset buttons for group
		auto controlsPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 25 );
		controlsPanel->SetDock( RedGui::DOCK_Top );
		tabs->GetTabAt( tabIndex )->AddChild( controlsPanel );

		TDynArray< InGameConfig::SConfigPresetOption > presetOptions;
		group->ListPresets( presetOptions );

		// Add grid layout to that preset control
		RedGui::CRedGuiGridLayout* presetControlGrid = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
		presetControlGrid->SetDock( RedGui::DOCK_Fill );
		presetControlGrid->SetDimensions( presetOptions.Size(), 1 );
		controlsPanel->AddChild( presetControlGrid );

		for( auto opt : presetOptions )
		{
			auto presetButton = new RedGui::CRedGuiButton( 0, 0, 40, 25 );
			presetButton->SetText( opt.displayName );
			presetButton->SetDock( RedGui::DOCK_Fill );
			presetButton->EventButtonClicked.Bind( this, &CDebugWindowInGameConfig::OnPresetButtonForConfigGroupClicked );
			m_presetButtonToConfigGroupMapping.PushBack( SPresetControlToConfigGroupMapping( opt, presetButton, group ) );

			m_refreshCallbacks.PushBack( [presetButton, opt, group]
				{
					Int32 val = group->GetActivePreset();
					if( opt.id != val )
						presetButton->SetText( presetButton->GetText(), Color::GRAY );
					else
						presetButton->SetText( presetButton->GetText(), Color::WHITE );
				} );

			presetControlGrid->AddChild( presetButton );
		}

		TDynArray< InGameConfig::IConfigVar* > configVarTable;
		group->ListConfigVars( configVarTable );

		// Add grid layout to that tab
		RedGui::CRedGuiGridLayout* grid = new RedGui::CRedGuiGridLayout( 0, 0, 100, 28 * configVarTable.Size() );
		grid->SetDock( RedGui::DOCK_Top );
		grid->SetDimensions( 2, configVarTable.Size() );
		tabs->GetTabAt( tabIndex )->AddChild( grid );

		// Add controls for config vars
		for( auto configVar : configVarTable)
		{
			if( configVar->IsVisible() )
			{
				AddConfigVarControlsToGrid( grid, configVar );
			}
		}
	}

	void CDebugWindowInGameConfig::OnChangeTab( RedGui::CRedGuiEventPackage& eventPackage, RedGui::CRedGuiControl* tab )
	{
		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::AddConfigVarControlsToGrid(RedGui::CRedGuiGridLayout* grid, InGameConfig::IConfigVar* configVar)
	{
		RedGui::CRedGuiLabel* configLabel = new RedGui::CRedGuiLabel( 0, 0, 100, 25 );
		configLabel->SetDock( RedGui::DOCK_Top );
		configLabel->SetText( configVar->GetDisplayName() );
		grid->AddChild( configLabel );

		auto controlsPanel = new RedGui::CRedGuiPanel( 0, 0, 100, 25 );
		controlsPanel->SetDock( RedGui::DOCK_Top );
		grid->AddChild( controlsPanel );

		if( configVar->GetDisplayType() == TXT("TOGGLE") )
		{
			auto checkbox = new RedGui::CRedGuiCheckBox( 0, 0, 100, 25 );
			checkbox->SetDock( RedGui::DOCK_Top );
			m_checkBoxToConfigVarMapping.PushBack( SControlToConfigVarMapping( checkbox, configVar ) );
			checkbox->EventCheckedChanged.Bind( this, &CDebugWindowInGameConfig::OnCheckBoxForConfigVarChanged );

			m_refreshCallbacks.PushBack( [checkbox, configVar]
				{
					Bool val = configVar->GetValue().GetAsBool( false );
					checkbox->SetChecked( val );
				} );

			controlsPanel->AddChild( checkbox );
		}
		else if( configVar->GetDisplayType() == TXT("OPTIONS") )
		{
			TDynArray< InGameConfig::SConfigPresetOption > options;
			configVar->ListOptions( options );

			// Add grid layout to that preset control
			RedGui::CRedGuiGridLayout* presetControlGrid = new RedGui::CRedGuiGridLayout( 0, 0, 100, 100 );
			presetControlGrid->SetDock( RedGui::DOCK_Fill );
			presetControlGrid->SetDimensions( options.Size(), 1 );
			controlsPanel->AddChild( presetControlGrid );

			Int32 optId = 0;
			for( auto opt : options )
			{
				auto presetButton = new RedGui::CRedGuiButton( 0, 0, 40, 25 );
				presetButton->SetText( opt.displayName );
				presetButton->SetDock( RedGui::DOCK_Fill );
				presetButton->EventButtonClicked.Bind( this, &CDebugWindowInGameConfig::OnPresetButtonForConfigVarClicked );
				m_presetButtonToConfigVarMapping.PushBack( SPresetControlToConfigVarListOfOptMapping( optId, presetButton, configVar ) );

				m_refreshCallbacks.PushBack( [presetButton, optId, configVar]
					{
						Int32 val = configVar->GetValue().GetAsInt( 0 );
						if( optId != val )
							presetButton->SetText( presetButton->GetText(), Color::GRAY );
						else
							presetButton->SetText( presetButton->GetText(), Color::WHITE );
					} );

				presetControlGrid->AddChild( presetButton );
				optId++;
			}
		}
		else if( configVar->GetDisplayType() == TXT("NUMBER") )
		{
			auto spinBox = new RedGui::CRedGuiSpin( 0, 0, 100, 40 );
			spinBox->SetValue( 0 );
			spinBox->SetDock( RedGui::DOCK_Fill );
			spinBox->EventValueChanged.Bind( this, &CDebugWindowInGameConfig::OnSpinBoxForConfigVarChanged );

			controlsPanel->AddChild( spinBox );

			m_SpinBoxToConfigVarSliderMapping.PushBack( SControlToConfigVarMapping( spinBox, configVar ) );

			m_refreshCallbacks.PushBack( [spinBox, configVar]
			{
				Int32 val = configVar->GetValue().GetAsInt( 0 );
				spinBox->SetValue( val );
			} );
		}
		else if( configVar->GetDisplayType() == TXT("LIST") )
		{
			auto comboBox = new RedGui::CRedGuiComboBox( 0, 0, 120, 20 );
			
			TDynArray< InGameConfig::SConfigPresetOption > options;
			configVar->ListOptions( options );
			Int32 optId = 0;
			for( auto opt : options )
			{
				comboBox->AddItem( opt.displayName );
				optId++;
			}
			comboBox->SetDock( RedGui::DOCK_Fill );
			comboBox->EventSelectedIndexChanged.Bind( this, &CDebugWindowInGameConfig::OnComboBoxForConfigVarChanged );
			controlsPanel->AddChild( comboBox );

			m_ComboBoxToConfigVarListOfValuesMapping.PushBack( SControlToConfigVarListOfValuesMapping( comboBox, configVar ) );

			m_refreshCallbacks.PushBack( [comboBox, configVar]
			{
				comboBox->SetSelectedIndex( configVar->GetValue().GetAsInt() );
			} );
		}
		else
		{
			TDynArray<String> splitted = configVar->GetDisplayType().Split(TXT(";"));
			if( splitted[0] == TXT("SLIDER") || splitted[0] == TXT("GAMMA") )
			{
				auto slider = new RedGui::CRedGuiSlider( 0, 0, 100, 20 );
				slider->SetValue( 0.0f );
				slider->SetDock( RedGui::DOCK_Fill );
				slider->EventScroll.Bind( this, &CDebugWindowInGameConfig::OnSliderForConfigVarChanged );

				Float min = 0.0f;
				Float max = 100.0f;
				Float resolution = 100.0f;

				if( splitted.Size() > 1 )
				{
					FromString( splitted[1], min );
				}
				if( splitted.Size() > 2 )
				{
					FromString( splitted[2], max );
				}
				if( splitted.Size() > 3 )
				{
					FromString( splitted[3], resolution );
				}

				slider->SetMinValue( min );
				slider->SetMaxValue( max );
				slider->SetStepValue( (max-min) / resolution );

				controlsPanel->AddChild( slider );

				m_SliderToConfigVarSliderMapping.PushBack( SControlToConfigVarMapping( slider, configVar ) );

				m_refreshCallbacks.PushBack( [slider, configVar]
				{
					Float val = configVar->GetValue().GetAsFloat( 0.0f );
					slider->SetValue( val );
				} );
			}
		}
	}

	void CDebugWindowInGameConfig::OnCheckBoxForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Bool value)
	{
		for( auto mapping : m_checkBoxToConfigVarMapping )
		{
			if( mapping.controlRef == eventPackage.GetEventSender() )
			{
				mapping.var->SetValue( InGameConfig::CConfigVarValue( value ), InGameConfig::eConfigVarAccessType_UserAction );
				break;
			}
		}
	}

	void CDebugWindowInGameConfig::OnPresetButtonForConfigGroupClicked(RedGui::CRedGuiEventPackage& eventPackage)
	{
		for( auto mapping : m_presetButtonToConfigGroupMapping )
		{
			if( mapping.controlRef == eventPackage.GetEventSender() )
			{
				auto presetableGroup = mapping.presetable;
				presetableGroup->ApplyPreset( mapping.presetOption.id, InGameConfig::eConfigVarAccessType_UserAction );
				break;
			}
		}

		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::OnPresetButtonForConfigVarClicked(RedGui::CRedGuiEventPackage& eventPackage)
	{
		for( auto mapping : m_presetButtonToConfigVarMapping )
		{
			if( mapping.controlRef == eventPackage.GetEventSender() )
			{
				auto presetableVar = mapping.configVar;
				presetableVar->SetValue( InGameConfig::CConfigVarValue( mapping.optionId ), InGameConfig::eConfigVarAccessType_UserAction );
				break;
			}
		}

		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::OnSaveConfigButtonClicked(RedGui::CRedGuiEventPackage& eventPackage)
	{
		SConfig::GetInstance().Save();
	}

	void CDebugWindowInGameConfig::RefreshConfigUI()
	{
#ifdef RED_PLATFORM_WINPC
		if( m_initializing == true )
			return;

		m_refreshing = true;
		for( auto callback : m_refreshCallbacks )
		{
			callback();
		}
		m_refreshing = false;
#endif
	}

	void CDebugWindowInGameConfig::OnWindowOpened(CRedGuiControl* control)
	{
		RedGui::CRedGuiWindow::OnWindowOpened( control );
		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::OnComboBoxForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Int32 value)
	{
		if( m_refreshing == true ) return;
		for( auto mapping : m_ComboBoxToConfigVarListOfValuesMapping )
		{
			if( mapping.controlRef == eventPackage.GetEventSender() )
			{
				auto configVar = mapping.configVar;
				configVar->SetValue( InGameConfig::CConfigVarValue( mapping.controlRef->GetSelectedItemName() ), InGameConfig::eConfigVarAccessType_UserAction );
				break;
			}
		}

		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::OnSliderForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Float value)
	{
		if( m_refreshing == true ) return;
		for( auto mapping : m_SliderToConfigVarSliderMapping )
		{
			if( mapping.controlRef == eventPackage.GetEventSender() )
			{
				auto configVar = mapping.var;
				configVar->SetValue( InGameConfig::CConfigVarValue( value ), InGameConfig::eConfigVarAccessType_UserAction );
				break;
			}
		}

		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::OnSpinBoxForConfigVarChanged(RedGui::CRedGuiEventPackage& eventPackage, Int32 value)
	{
		if( m_refreshing == true ) return;
		for( auto mapping : m_ComboBoxToConfigVarListOfValuesMapping )
		{
			if( mapping.controlRef == eventPackage.GetEventSender() )
			{
				auto configVar = mapping.configVar;
				configVar->SetValue( InGameConfig::CConfigVarValue( value ), InGameConfig::eConfigVarAccessType_UserAction );
				break;
			}
		}

		RefreshConfigUI();
	}

	void CDebugWindowInGameConfig::OnApplySettingsButtonClicked(RedGui::CRedGuiEventPackage& eventPackage)
	{
		//GEngine->OnRequestRefresh( CNAME( refreshEngine ) );
	}

	void CDebugWindowInGameConfig::OnResetConfigButtonClicked(RedGui::CRedGuiEventPackage& eventPackage)
	{
		SConfig::GetInstance().ResetUserSettings(Config::eConfigVarSetMode_User);
	}

}
#endif
