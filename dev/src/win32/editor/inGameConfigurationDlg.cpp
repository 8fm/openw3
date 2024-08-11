#include "build.h"
#include "inGameConfigurationDlg.h"
#include "../../common/engine/inGameConfig.h"
#include "../../common/engine/inGameConfigInterface.h"
#include "../../common/core/enum.h"

BEGIN_EVENT_TABLE( CEdInGameConfigurationDlg, wxDialog )
	EVT_ACTIVATE( CEdInGameConfigurationDlg::OnActivate )
END_EVENT_TABLE()

namespace InputBindingHelper
{
	void UnpackBindingValue( const String& value, EInputKey& mainKey, EInputKey& alternativeKey )
	{
		TDynArray<String> keyStrings = value.Split(TXT(";"));

		CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

		mainKey = IK_None;
		alternativeKey = IK_None;

		if( keyStrings.Size() > 0 )
		{
			Int32 mainKeyValue;
			keyEnum->FindValue( CName( keyStrings[0] ), mainKeyValue );
			mainKey = (EInputKey)mainKeyValue;
		}
		
		if( keyStrings.Size() > 1 )
		{
			Int32 alternativeKeyValue;
			keyEnum->FindValue( CName( keyStrings[1] ), alternativeKeyValue );
			alternativeKey = (EInputKey)alternativeKeyValue;
		}
	}

	void UnpackBindingValue( const String& value, EInputKey& key )
	{
		CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

		Int32 mainKeyValue;
		keyEnum->FindValue( CName( value ), mainKeyValue );
		key = (EInputKey)mainKeyValue;
	}

	void PackBindingValue( EInputKey mainKey, EInputKey alternativeKey, String& outValue )
	{
		CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );
		CName mainKeyName;
		keyEnum->FindName( (Int32)mainKey, mainKeyName );
		CName alternativeKeyName;
		keyEnum->FindName( (Int32)alternativeKey, alternativeKeyName );

		outValue = mainKeyName.AsString();
		outValue += TXT(";");
		outValue += alternativeKeyName.AsString();
	}

	void PackBindingValue( EInputKey key, String& outValue )
	{
		CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );
		CName keyName;
		keyEnum->FindName( (Int32)key, keyName );

		outValue = keyName.AsString();
	}
};

CEdInGameConfigurationDlg::CEdInGameConfigurationDlg( wxWindow* parent )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("InGameConfigDlg") );

	m_tabs = XRCCTRL( *this, "mGroupTab", wxNotebook );
	m_tabs->Bind( wxEVT_COMMAND_NOTEBOOK_PAGE_CHANGED, &CEdInGameConfigurationDlg::OnPageChanged, this );

	wxButton* saveBtn = XRCCTRL( *this, "mSaveBtn", wxButton );
	saveBtn->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdInGameConfigurationDlg::OnSaveBtnClicked, this );

	wxButton* resetBtn = XRCCTRL( *this, "mResetBtn", wxButton );
	resetBtn->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdInGameConfigurationDlg::OnResetBtnClicked, this );

	CreateControls();
	SetSize( 600, 600 );
	Layout();

	SRawInputManager::GetInstance().RegisterListener( this );

	m_bindingTimer = 0.0f;
	m_afterBindTimer = 0.0f;
}

void CEdInGameConfigurationDlg::CreateControls()
{
	TDynArray< InGameConfig::IConfigGroup* > configGroups;
	GInGameConfig::GetInstance().ListAllConfigGroups( configGroups );

	for( auto group : configGroups )
	{
		CreateNewTab( group );
	}
	RefreshAllValues();
}

void CEdInGameConfigurationDlg::OnActivate( wxActivateEvent& event )
{
	RefreshAllValues();
}

void CEdInGameConfigurationDlg::OnPageChanged( wxCommandEvent& event )
{
	RefreshAllValues();
}

void CEdInGameConfigurationDlg::OnSaveBtnClicked( wxCommandEvent& event )
{
	SConfig::GetInstance().Save();
	if( GGame != nullptr )
	{
		if( GGame->GetInputManager() != nullptr )
		{
			GGame->GetInputManager()->SaveUserMappings();
		}
	}
}

void CEdInGameConfigurationDlg::OnResetBtnClicked( wxCommandEvent& event )
{
	int selectedIndex = m_tabs->GetSelection();

	if( selectedIndex != wxNOT_FOUND )
	{
		wxString pageText = m_tabs->GetPageText( selectedIndex );
		wxString messageBoxText;
		messageBoxText.Printf( wxT( "All settings saved under '%ls' will be reset!" ), pageText.wc_str() );

		int result = wxMessageBox( messageBoxText, wxT( "Are you sure?" ), wxOK | wxCANCEL | wxCANCEL_DEFAULT | wxICON_EXCLAMATION | wxSTAY_ON_TOP | wxCENTRE, this );

		if( result == wxOK )
		{
			TDynArray< InGameConfig::IConfigGroup* > configGroups;
			GInGameConfig::GetInstance().ListAllConfigGroups( configGroups );

			for( InGameConfig::IConfigGroup* group : configGroups )
			{
				if( pageText == group->GetDisplayName().AsChar() )
				{
					group->ResetToDefault();
					RefreshAllValues();
					break;
				}
			}
		}
	}
}

void CEdInGameConfigurationDlg::OnPresetChanged( wxCommandEvent& event )
{
	wxRadioButton* rBtn = (wxRadioButton*)event.GetEventObject();
	if ( SPresetChange* newPresetChoice = m_presetsMap.FindPtr( rBtn ) )
	{
		newPresetChoice->m_group->ApplyPreset( newPresetChoice->m_chosenPresetId, InGameConfig::eConfigVarAccessType_UserAction );
		RefreshAllValues();
	}
}

void CEdInGameConfigurationDlg::OnBooleanOptionChanged( wxCommandEvent& event )
{
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( (wxControl*)event.GetEventObject() ) )
	{
		optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( event.IsChecked() ), InGameConfig::eConfigVarAccessType_UserAction );
	}
}

void CEdInGameConfigurationDlg::OnNumberOptionChanged( wxCommandEvent& event )
{
	wxSpinCtrl* ctrl = (wxSpinCtrl*)event.GetEventObject();
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( ctrl ) )
	{
		optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( ctrl->GetValue() ), InGameConfig::eConfigVarAccessType_UserAction );
	}
}

void CEdInGameConfigurationDlg::OnListOptionChanged( wxCommandEvent& event )
{
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( (wxControl*)event.GetEventObject() ) )
	{
		optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( String( event.GetString().wc_str() ) ), InGameConfig::eConfigVarAccessType_UserAction );
	}
}

void CEdInGameConfigurationDlg::OnSliderOptionChanged( wxCommandEvent& event )
{
	wxSlider* slider = (wxSlider*)event.GetEventObject();
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( slider ) )
	{
		Float relativeValue = (Float)( slider->GetValue() - slider->GetMin() ) / (Float)( slider->GetMax() - slider->GetMin() );
		optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( optionChange->m_minValue + relativeValue * optionChange->m_maxValue ), InGameConfig::eConfigVarAccessType_UserAction );
	}
}

void CEdInGameConfigurationDlg::OnOptionChanged( wxCommandEvent& event )
{
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( (wxControl*)event.GetEventObject() ) )
	{
		optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( optionChange->m_chosenId ), InGameConfig::eConfigVarAccessType_UserAction );
	}
}

void CEdInGameConfigurationDlg::OnInputBindingClicked(wxCommandEvent& event)
{
	wxButton* btn = (wxButton*)event.GetEventObject();
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( btn ) )
	{
		m_currentActionForBinding = optionChange->m_option->GetConfigId();

		if( m_currentActionForBinding != CName::NONE )
		{
			SetupBindingListening( btn, optionChange );
		}
	}
}

void CEdInGameConfigurationDlg::OnInputBindingClearClicked(wxCommandEvent& event)
{
	if( m_afterBindTimer > 0.0f )		// Ignore clear events just after new binding
	{
		return;
	}

	wxButton* btn = (wxButton*)event.GetEventObject();
	if ( SOptionChange* optionChange = m_optionsMap.FindPtr( btn ) )
	{
		String configValue = optionChange->m_option->GetValue().GetAsString();
		EInputKey mainKey;
		EInputKey alternativeKey;
		InputBindingHelper::UnpackBindingValue( configValue, mainKey, alternativeKey );

		if( optionChange->m_boolean == true )		// Is for main key
		{
			mainKey = IK_None;
		}
		else		// Is for alternative key
		{
			alternativeKey = IK_None;
		}

		String newConfigValue;
		InputBindingHelper::PackBindingValue( mainKey, alternativeKey, newConfigValue );
		optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( newConfigValue ), InGameConfig::eConfigVarAccessType_UserAction );
		RefreshAllValues();
	}
}

void CEdInGameConfigurationDlg::RefreshAllValues()
{
	for( CRefreshCallback& callback : m_refreshCallbacks )
	{
		callback();
	}
}

void CEdInGameConfigurationDlg::CreateNewTab( InGameConfig::IConfigGroup* group )
{
	if ( group->IsVisible() )
	{
		wxScrolled<wxPanel>* groupTab = new wxScrolled<wxPanel>( m_tabs, wxID_ANY );
		groupTab->SetSizer( new wxBoxSizer( wxVERTICAL ) );
		groupTab->SetScrollRate( 0, 5 );
		m_tabs->AddPage( groupTab, group->GetDisplayName().AsChar() );

		AppendPresets( groupTab, group );
		AppendVariables( groupTab, group );
	}
}

void CEdInGameConfigurationDlg::AppendPresets( wxScrolled<wxPanel>* groupTab, InGameConfig::IConfigGroup* group )
{
	TDynArray< InGameConfig::SConfigPresetOption > presetOptions;
	group->ListPresets( presetOptions );

	if ( !presetOptions.Empty() )
	{
		wxPanel* presetPanel = new wxPanel( groupTab, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ), wxSIMPLE_BORDER );
		groupTab->GetSizer()->Add( presetPanel, 0, wxALIGN_CENTER_HORIZONTAL | wxALIGN_TOP | wxALL, 10 );
		wxSizer* presetSizer = new wxBoxSizer( wxHORIZONTAL );
		presetPanel->SetSizer( presetSizer );

		Bool first = true;
		for ( InGameConfig::SConfigPresetOption& preset : presetOptions )
		{
			wxRadioButton* btn = new wxRadioButton( presetPanel, wxID_ANY, preset.displayName.AsChar(), wxDefaultPosition, wxSize( 100, 50 ), first ? wxRB_GROUP : 0 );
			m_presetsMap.Insert( btn, SPresetChange( group, preset.id ) );
			btn->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnPresetChanged ), NULL, this );

			presetSizer->Add( btn, 0, wxALIGN_CENTER | wxALL, 5 );

			if ( first )
			{
				first = false;
			}

			m_refreshCallbacks.PushBack( [btn, preset, group]
			{
				Int32 val = group->GetActivePreset();
				if( preset.id == val )
				{
					btn->SetValue( true );
				}
			} );
		}
	}
}

void CEdInGameConfigurationDlg::AppendVariables( wxScrolled<wxPanel>* groupTab, InGameConfig::IConfigGroup* group )
{
	TDynArray< InGameConfig::IConfigVar* > configVarTable;
	group->ListConfigVars( configVarTable );

	for( InGameConfig::IConfigVar* configVar : configVarTable)
	{
		if( configVar->IsVisible() )
		{
			wxSizer* optionSizer = new wxBoxSizer( wxHORIZONTAL );
			groupTab->GetSizer()->Add( optionSizer, 0, wxLEFT | wxALL, 5 );
			optionSizer->Add( new wxStaticText( groupTab, wxID_ANY, configVar->GetDisplayName().AsChar(), wxDefaultPosition, wxSize( 130, -1 ) ), 0, wxALIGN_LEFT | wxALL, 5 );

			if ( configVar->GetDisplayType() == TXT("TOGGLE") )
			{
				AppendToggleVariable( groupTab, optionSizer, configVar );
			}
			else if ( configVar->GetDisplayType() == TXT("OPTIONS") )
			{
				AppendOptionsVariable( groupTab, optionSizer, configVar );
			}
			else if ( configVar->GetDisplayType() == TXT("NUMBER") )
			{
				AppendNumberVariable( groupTab, optionSizer, configVar );
			}
			else if ( configVar->GetDisplayType() == TXT("LIST") )
			{
				AppendListVariable( groupTab, optionSizer, configVar );
			}
			else if ( configVar->GetDisplayType() == TXT("INPUTPC") )
			{
				AppendInputBindingPCVariable( groupTab, optionSizer, configVar );
			}
			else if( configVar->GetDisplayType() == TXT("INPUTPAD") )
			{
				AppendInputBindingPadVariable( groupTab, optionSizer, configVar );
			}
			else //slider
			{
				AppendSliderVariable( groupTab, optionSizer, configVar );
			}
		}
	}
}

void CEdInGameConfigurationDlg::AppendToggleVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar )
{
	wxCheckBox* cBox = new wxCheckBox( parent, wxID_ANY, TXT("On") );
	m_optionsMap.Insert( cBox, SOptionChange( configVar ) );
	cBox->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnBooleanOptionChanged ), NULL, this );
	optionSizer->Add( cBox, 0, wxALIGN_LEFT | wxALL, 5 );

	m_refreshCallbacks.PushBack( [cBox, configVar]
	{
		Bool val = configVar->GetValue().GetAsBool( false );
		cBox->SetValue( val );
	} );
}

void CEdInGameConfigurationDlg::AppendOptionsVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar )
{
	TDynArray< InGameConfig::SConfigPresetOption > options;
	configVar->ListOptions( options );

	if ( !options.Empty() )
	{
		wxPanel* presetPanel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );
		optionSizer->Add( presetPanel, 0, wxALIGN_LEFT | wxALL, 5 );
		wxSizer* presetSizer = new wxBoxSizer( wxHORIZONTAL );
		presetPanel->SetSizer( presetSizer );

		Bool first = true;
		for ( InGameConfig::SConfigPresetOption& option : options )
		{
			wxRadioButton* btn = new wxRadioButton( presetPanel, wxID_ANY, option.displayName.AsChar(), wxDefaultPosition, wxSize( -1, -1 ), first ? wxRB_GROUP : 0 );
			m_optionsMap.Insert( btn, SOptionChange( configVar, option.id ) );
			btn->Connect( wxEVT_COMMAND_RADIOBUTTON_SELECTED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnOptionChanged ), NULL, this );
			//add event handler
			presetSizer->Add( btn, 0, wxALIGN_LEFT | wxALL, 10 );

			if ( first )
			{
				first = false;
			}

			m_refreshCallbacks.PushBack( [btn, option, configVar]
			{
				Int32 val = configVar->GetValue().GetAsInt( 0 );
				if( option.id == val )
				{
					btn->SetValue( true );
				}
			} );
		}
	}
}

void CEdInGameConfigurationDlg::AppendNumberVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar )
{
	wxSpinCtrl* spin = new wxSpinCtrl( parent );
	m_optionsMap.Insert( spin, SOptionChange( configVar ) );
	spin->Connect( wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnNumberOptionChanged ), NULL, this );
	optionSizer->Add( spin, 0, wxALIGN_LEFT | wxALL, 5 );

	m_refreshCallbacks.PushBack( [spin, configVar]
	{
		Int32 val = configVar->GetValue().GetAsInt( 0 );
		spin->SetValue( val );
	} );
}

void CEdInGameConfigurationDlg::AppendListVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar )
{
	wxChoice* choice = new wxChoice( parent, wxID_ANY );
	m_optionsMap.Insert( choice, SOptionChange( configVar ) );
	choice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnListOptionChanged ), NULL, this );
	optionSizer->Add( choice, 0, wxALIGN_LEFT | wxALL, 5 );

	TDynArray< InGameConfig::SConfigPresetOption > options;
	configVar->ListOptions( options );
	for( InGameConfig::SConfigPresetOption& opt : options )
	{
		choice->AppendString( opt.displayName.AsChar() );
	}

	m_refreshCallbacks.PushBack( [choice, configVar]
	{
		choice->SetSelection( configVar->GetValue().GetAsInt() );
	} );
}

void CEdInGameConfigurationDlg::AppendSliderVariable(wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar)
{
	String splitter = configVar->GetDisplayType().ContainsSubstring( TXT (";") ) ? TXT(";") : TXT(":");
	TDynArray<String> splitted = configVar->GetDisplayType().Split( splitter );
	if( splitted[0] == TXT("SLIDER") || splitted[0] == TXT("GAMMA") )
	{
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

		wxSlider* slider = new wxSlider( parent, wxID_ANY, 0, 0, resolution, wxDefaultPosition, wxSize( 400, -1 ) );
		m_optionsMap.Insert( slider, SOptionChange( configVar, min, max ) );
		slider->Connect( wxEVT_COMMAND_SLIDER_UPDATED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnSliderOptionChanged ), NULL, this );
		optionSizer->Add( slider, 0, wxALIGN_LEFT | wxALL, 5 );

		Float sliderMax = resolution;
		m_refreshCallbacks.PushBack( [slider, configVar, min, max, sliderMax]
		{
			Float val = configVar->GetValue().GetAsFloat( 0.0f );
			Float relativeVal = (val - min) / ( max - min );

			slider->SetValue( relativeVal * sliderMax );
		} );
	}
}

void CEdInGameConfigurationDlg::AppendInputBindingPCVariable(wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar)
{
	wxPanel* presetPanel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );
	optionSizer->Add( presetPanel, 0, wxALIGN_LEFT | wxALL, 5 );
	wxSizer* presetSizer = new wxBoxSizer( wxHORIZONTAL );
	presetPanel->SetSizer( presetSizer );

	// Main key button
	wxButton* mainKeyBtn = new wxButton( presetPanel, wxID_ANY, TXT(""), wxDefaultPosition, wxSize( -1, -1 ) );
	m_optionsMap.Insert( mainKeyBtn, SOptionChange( configVar, true ) );
	mainKeyBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnInputBindingClicked ), NULL, this );
	//add event handler
	presetSizer->Add( mainKeyBtn, 0, wxALIGN_LEFT | wxALL, 10 );

	// Main key clear button
	wxButton* mainKeyClearBtn = new wxButton( presetPanel, wxID_ANY, TXT("X"), wxDefaultPosition, wxSize( 40, -1 ) );
	m_optionsMap.Insert( mainKeyClearBtn, SOptionChange( configVar, true ) );
	mainKeyClearBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnInputBindingClearClicked ), NULL, this );
	//add event handler
	presetSizer->Add( mainKeyClearBtn, 0, wxALIGN_LEFT | wxALL, 10 );

	// Alternative key button
	wxButton* alternativeKeyBtn = new wxButton( presetPanel, wxID_ANY, TXT(""), wxDefaultPosition, wxSize( -1, -1 ) );
	m_optionsMap.Insert( alternativeKeyBtn, SOptionChange( configVar, false ) );
	alternativeKeyBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnInputBindingClicked ), NULL, this );
	//add event handler
	presetSizer->Add( alternativeKeyBtn, 0, wxALIGN_LEFT | wxALL, 10 );

	// Alternative key clear button
	wxButton* alternativeKeyClearBtn = new wxButton( presetPanel, wxID_ANY, TXT("X"), wxDefaultPosition, wxSize( 40, -1 ) );
	m_optionsMap.Insert( alternativeKeyClearBtn, SOptionChange( configVar, false ) );
	alternativeKeyClearBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnInputBindingClearClicked ), NULL, this );
	//add event handler
	presetSizer->Add( alternativeKeyClearBtn, 0, wxALIGN_LEFT | wxALL, 10 );

	m_refreshCallbacks.PushBack( [mainKeyBtn, alternativeKeyBtn, configVar]
	{
		EInputKey mainKey;
		EInputKey alternativeKey;

		InputBindingHelper::UnpackBindingValue( configVar->GetValue().GetAsString(), mainKey, alternativeKey );

		CName mainKeyValue;
		CName alternativeKeyValue;

		CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

		keyEnum->FindName( (Int32)mainKey, mainKeyValue );
		keyEnum->FindName( (Int32)alternativeKey, alternativeKeyValue );

		mainKeyBtn->SetLabel( wxString( mainKeyValue.AsChar() ) );
		alternativeKeyBtn->SetLabel( wxString( alternativeKeyValue.AsChar() ) );
	} );
}

void CEdInGameConfigurationDlg::AppendInputBindingPadVariable(wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar)
{
	wxPanel* presetPanel = new wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );
	optionSizer->Add( presetPanel, 0, wxALIGN_LEFT | wxALL, 5 );
	wxSizer* presetSizer = new wxBoxSizer( wxHORIZONTAL );
	presetPanel->SetSizer( presetSizer );

	// Main key button
	wxButton* keyBtn = new wxButton( presetPanel, wxID_ANY, TXT(""), wxDefaultPosition, wxSize( -1, -1 ) );
	m_optionsMap.Insert( keyBtn, SOptionChange( configVar, true ) );
	keyBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnInputBindingClicked ), NULL, this );
	//add event handler
	presetSizer->Add( keyBtn, 0, wxALIGN_LEFT | wxALL, 10 );

	// Main key clear button
	wxButton* keyClearBtn = new wxButton( presetPanel, wxID_ANY, TXT("X"), wxDefaultPosition, wxSize( 40, -1 ) );
	m_optionsMap.Insert( keyClearBtn, SOptionChange( configVar, true ) );
	keyClearBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdInGameConfigurationDlg::OnInputBindingClearClicked ), NULL, this );
	//add event handler
	presetSizer->Add( keyClearBtn, 0, wxALIGN_LEFT | wxALL, 10 );

	m_refreshCallbacks.PushBack( [keyBtn, configVar]
	{
		EInputKey key;
		InputBindingHelper::UnpackBindingValue( configVar->GetValue().GetAsString(), key );

		CName keyValue;

		CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

		keyEnum->FindName( (Int32)key, keyValue );

		keyBtn->SetLabel( wxString( keyValue.AsChar() ) );
	} );
}

Bool CEdInGameConfigurationDlg::ProcessInput(const BufferedInput& input)
{
	if( m_currentActionForBinding != CName::NONE )
	{
		for( const SBufferedInputEvent& inputEvent : input )
		{
			if( inputEvent.action == IACT_Release )
			{
				break;
			}

			if( inputEvent.key == IK_Escape )
			{
				OnInputBindingCancel();
				return false;
			}
			
			if( inputEvent.key != IK_MouseX && inputEvent.key != IK_MouseY )
			{
				OnInputBindingDetected( inputEvent.key );
				return false;
			}
		}

		if( GEngine != nullptr )
		{
			if( m_bindingTimer > 0.0f )
			{
				m_bindingTimer -= GEngine->GetLastTimeDelta();
			}
		}

		if( m_bindingTimer < 0.0f )
		{
			OnInputBindingTimeout();
		}
	}

	if( GEngine != nullptr )
	{
		if( m_afterBindTimer > 0.0f )
		{
			m_afterBindTimer -= GEngine->GetLastTimeDelta();
		}
	}

	return false;
}

void CEdInGameConfigurationDlg::OnInputBindingDetected(EInputKey key)
{
	SOptionChange* optionChange = m_optionsMap.FindPtr( m_currentInputBindingButton );

	if( optionChange != nullptr )
	{
		if( optionChange->m_option->GetDisplayType() == TXT("INPUTPC") )
		{
			OnInputBindingPCDetected( key, optionChange );
		}
		else
		{
			OnInputBindingPadDetected( key, optionChange );
		}
	}
}

void CEdInGameConfigurationDlg::OnInputBindingPCDetected(EInputKey key, SOptionChange* optionChange)
{
	// Retrieve previous binding
	String configValue = optionChange->m_option->GetValue().GetAsString();

	EInputKey mainKey = IK_None;
	EInputKey alternativeKey = IK_None;

	InputBindingHelper::UnpackBindingValue( configValue, mainKey, alternativeKey );

	// Create new binding
	if( m_isListeningForMainKey == true )
	{
		mainKey = key;
	}
	else
	{
		alternativeKey = key;
	}

	String newConfigValue;
	InputBindingHelper::PackBindingValue( mainKey, alternativeKey, newConfigValue );
	optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( newConfigValue ), InGameConfig::eConfigVarAccessType_UserAction );

	m_currentInputBindingButton->Enable( true );
	ClearBindingListening();

	RefreshAllValues();
}

void CEdInGameConfigurationDlg::OnInputBindingPadDetected(EInputKey key, SOptionChange* optionChange)
{
	String newConfigValue;
	InputBindingHelper::PackBindingValue( key, newConfigValue );
	optionChange->m_option->SetValue( InGameConfig::CConfigVarValue( newConfigValue ), InGameConfig::eConfigVarAccessType_UserAction );

	m_currentInputBindingButton->Enable( true );
	ClearBindingListening();

	RefreshAllValues();
}

void CEdInGameConfigurationDlg::OnInputBindingTimeout()
{
	OnInputBindingCancel();
}

void CEdInGameConfigurationDlg::OnInputBindingCancel()
{
	SOptionChange* optionChange = m_optionsMap.FindPtr( m_currentInputBindingButton );

	String configValue = optionChange->m_option->GetValue().GetAsString();
	
	EInputKey mainKey;
	EInputKey alternativeKey;

	InputBindingHelper::UnpackBindingValue( configValue, mainKey, alternativeKey );

	CName mainKeyValue;
	CName alternativeKeyValue;

	CEnum* keyEnum = SRTTI::GetInstance().FindEnum( CNAME( EInputKey ) );

	keyEnum->FindName( (Int32)mainKey, mainKeyValue );
	keyEnum->FindName( (Int32)mainKey, alternativeKeyValue );

	if( optionChange->m_boolean == true )
	{
		wxString label = wxString( mainKeyValue.AsChar() );
		m_currentInputBindingButton->SetLabel( label );
	}
	else
	{
		wxString label = wxString( alternativeKeyValue.AsChar() );
		m_currentInputBindingButton->SetLabel( label );
	}

	m_currentInputBindingButton->Enable( true );

	ClearBindingListening();
}

void CEdInGameConfigurationDlg::SetupBindingListening(wxControl* control, SOptionChange* optionChange)
{
	m_currentInputBindingButton = control;
	m_currentOptionForInputButton = optionChange;
	m_currentActionForBinding = optionChange->m_option->GetConfigId();
	m_isListeningForMainKey = optionChange->m_boolean;

	m_bindingTimer = 10.0f;

	control->SetLabel( wxString( "?" ) );
	control->Enable( false );
}

void CEdInGameConfigurationDlg::ClearBindingListening()
{
	m_currentActionForBinding = CName::NONE;
	m_currentInputBindingButton = nullptr;
	m_currentOptionForInputButton = nullptr;
	m_afterBindTimer = 0.5f;
}

void CEdInGameConfigurationDlg::Execute()
{
	RefreshAllValues();
	Show();
}

void CEdInGameConfigurationDlg::SelectSettingsTab(const StringAnsi& tabname)
{
	Int32 pageCount = m_tabs->GetPageCount();
	for( Int32 i=0; i<pageCount; ++i )
	{
		if( Red::System::StringCompare( m_tabs->GetPageText( i ).mb_str(), tabname.AsChar() ) == 0 )
		{
			m_tabs->SetSelection( i );
			break;
		}
	}
}

