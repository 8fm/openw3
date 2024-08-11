#pragma once

class CEdInGameConfigurationDlg : public wxDialog, public IRawInputListener
{
	DECLARE_EVENT_TABLE();

	struct SPresetChange
	{
		InGameConfig::IConfigGroup*		m_group;
		Int32							m_chosenPresetId;

		SPresetChange( InGameConfig::IConfigGroup* group, Int32 preset )
		{
			m_group = group;
			m_chosenPresetId = preset;
		}
	};

	struct SOptionChange
	{
		InGameConfig::IConfigVar*		m_option;
		Int32							m_chosenId;
		Bool							m_boolean;
		Float							m_minValue;
		Float							m_maxValue;

		SOptionChange( InGameConfig::IConfigVar* option, Int32 id )
		{
			m_option = option;
			m_chosenId = id;
			m_boolean = false;
		}

		SOptionChange( InGameConfig::IConfigVar* option )
		{
			m_option = option;
			m_chosenId = -1;
			m_boolean = true;
		}

		SOptionChange( InGameConfig::IConfigVar* option, Float min, Float max )
		{
			m_option = option;
			m_minValue = min;
			m_maxValue = max;
		}

		SOptionChange( InGameConfig::IConfigVar* option, Bool isMainKey )
		{
			m_option = option;
			m_chosenId = -1;
			m_boolean = isMainKey;
		}
	};

public:
	CEdInGameConfigurationDlg( wxWindow* parent );

	void Execute();
	void SelectSettingsTab( const StringAnsi& tabname );

private:
	class InGameConfig::IConfigGroup;
	class InGameConfig::IConfigVar;

	void CreateControls();
	void CreateNewTab( InGameConfig::IConfigGroup* group );
	void AppendPresets( wxScrolled<wxPanel>* groupTab, InGameConfig::IConfigGroup* group );
	void AppendVariables( wxScrolled<wxPanel>* groupTab, InGameConfig::IConfigGroup* group );
	void AppendToggleVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar );
	void AppendOptionsVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar );
	void AppendNumberVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar );
	void AppendListVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar );
	void AppendSliderVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar );
	void AppendInputBindingPCVariable( wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar );
	void AppendInputBindingPadVariable(wxScrolled<wxPanel>* parent, wxSizer* optionSizer, InGameConfig::IConfigVar* configVar);

	void OnActivate( wxActivateEvent& event );
	void OnPageChanged( wxCommandEvent& event );
	void OnSaveBtnClicked( wxCommandEvent& event );
	void OnResetBtnClicked( wxCommandEvent& event );
	void OnPresetChanged( wxCommandEvent& event );
	void OnBooleanOptionChanged( wxCommandEvent& event );
	void OnNumberOptionChanged( wxCommandEvent& event );
	void OnListOptionChanged( wxCommandEvent& event );
	void OnSliderOptionChanged( wxCommandEvent& event );
	void OnOptionChanged( wxCommandEvent& event );
	void OnInputBindingClicked( wxCommandEvent& event );
	void OnInputBindingClearClicked( wxCommandEvent& event );
	void RefreshAllValues();

	// Input binding
	Bool ProcessInput(const BufferedInput& input);
	void OnInputBindingPCDetected( EInputKey key, SOptionChange* optionChange );
	void OnInputBindingPadDetected(EInputKey key, SOptionChange* optionChange);
	void OnInputBindingTimeout();
	void OnInputBindingCancel();

	void SetupBindingListening( wxControl* control, SOptionChange* optionChange );
	void ClearBindingListening();
	void OnInputBindingDetected(EInputKey key);

private:
	wxWindow* m_parent;
	wxNotebook* m_tabs;
	TDynArray< wxPanel* > m_groupTabs;

	typedef std::function< void() > CRefreshCallback;
	TDynArray< CRefreshCallback > m_refreshCallbacks;

	THashMap< wxRadioButton*, SPresetChange > m_presetsMap;
	THashMap< wxControl*, SOptionChange > m_optionsMap;

	// Input binding
	wxControl* m_currentInputBindingButton;
	SOptionChange* m_currentOptionForInputButton;
	Bool m_isListeningForMainKey;
	CName m_currentActionForBinding;
	Float m_bindingTimer;
	Float m_afterBindTimer;

};
