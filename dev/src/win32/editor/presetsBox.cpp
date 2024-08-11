/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "presetsBox.h"

class CEdPresetsBoxPresetsHook : public IEdPresetsHook
{
public:
	CEdPresetsBox*		m_box;

	virtual void OnPresetsChanged( CEdPresets* source )
	{
		// Inform original hook
		if ( m_box->m_originalHook != nullptr )
		{
			m_box->m_originalHook->OnPresetsChanged( source );
		}

		// Update the box's UI
		m_box->UpdatePresetsUI();
	}

	virtual void OnPresetKeySet( CEdPresets* source, const String& presetName, const String& keyName, const String& newValue )
	{
		// Inform original hook
		if ( m_box->m_originalHook != nullptr )
		{
			m_box->m_originalHook->OnPresetKeySet( source, presetName, keyName, newValue );
		}
	}

	virtual void OnPresetKeyRemoved( CEdPresets* source, const String& presetName, const String& keyName )
	{
		// Inform original hook
		if ( m_box->m_originalHook != nullptr )
		{
			m_box->m_originalHook->OnPresetKeyRemoved( source, presetName, keyName );
		}
	}

	virtual void OnPresetLoaded( CEdPresets* source, const String& presetName )
	{
		// Inform original hook
		if ( m_box->m_originalHook != nullptr )
		{
			m_box->m_originalHook->OnPresetLoaded( source, presetName );
		}

		// Update the box's UI
		m_box->UpdatePresetsUI();
	}

	virtual void OnPresetSaving( CEdPresets* source, const String& presetName )
	{
		// Inform original hook
		if ( m_box->m_originalHook != nullptr )
		{
			m_box->m_originalHook->OnPresetSaving( source, presetName );
		}
	}
};

//////////////////////////////////////////////////////////////////////////

CEdPresetsBox::CEdPresetsBox( wxWindow* parent, wxWindowID id /*= wxID_ANY*/, const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/, long style /*= wxTAB_TRAVERSAL */ )
	: wxPanel( parent, id, pos, size, style )
	, m_presets( nullptr )
	, m_hook( nullptr )
	, m_originalHook( nullptr )
{
	// Create our own hook
	m_boxHook = new CEdPresetsBoxPresetsHook();
	m_boxHook->m_box = this;

	// Create UI
	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxHORIZONTAL );

	m_presetName = new wxComboBox( this, wxID_ANY, wxT(""), wxDefaultPosition, wxDefaultSize, 0, NULL, 0 ); 
	bSizer1->Add( m_presetName, 1, wxTOP|wxBOTTOM, 2 );

	m_saveButton = new wxBitmapButton( this, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_SAVE16") ), wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW );
	m_saveButton->SetToolTip( wxT("Save the current preset") );

	bSizer1->Add( m_saveButton, 0, wxTOP|wxLEFT|wxBOTTOM, 2 );

	m_deleteButton = new wxBitmapButton( this, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_MARKED_DELETE") ), wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW );
	m_deleteButton->SetToolTip( wxT("Delete the current preset") );

	bSizer1->Add( m_deleteButton, 0, wxTOP|wxBOTTOM, 2 );

	m_menuButton = new wxBitmapButton( this, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TEXT("IMG_TOOL") ), wxDefaultPosition, wxSize( 21,21 ), wxBU_AUTODRAW );
	m_menuButton->SetToolTip( wxT("More preset actions") );
	m_menuButton->Hide();

	//bSizer1->Add( m_menuButton, 0, wxTOP|wxBOTTOM, 2 );

	this->SetSizer( bSizer1 );
	this->Layout();

	// Connect Events
	m_presetName->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CEdPresetsBox::OnPresetNameSelected ), NULL, this );
	m_saveButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPresetsBox::OnSavePresetClick ), NULL, this );
	m_deleteButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPresetsBox::OnDeletePresetClick ), NULL, this );
	m_menuButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdPresetsBox::OnMenuClick ), NULL, this );
}

CEdPresetsBox::~CEdPresetsBox()
{
	SetPresets( nullptr );
}

void CEdPresetsBox::OnPresetNameSelected( wxCommandEvent& event )
{
	String selectedName = m_presetName->GetValue().wc_str();

	// If we have no hook, just ignore this
	if ( !m_hook )
	{
		return;
	}

	// Make sure we can apply the preset
	if ( !m_hook->OnCanApplyPreset( this, selectedName ) )
	{
		// Reset the name
		m_presetName->SetValue( m_lastKnownName.AsChar() );
		return;
	}

	// Apply the preset
	m_hook->OnApplyPreset( this, selectedName );

    // Save last known name
    m_lastKnownName = selectedName;
}

void CEdPresetsBox::OnSavePresetClick( wxCommandEvent& event )
{
	String selectedName = m_presetName->GetValue().wc_str();
	selectedName.Trim();

	// If we have no hook or the preset name is empty, just ignore this
	if ( !m_hook || selectedName.Empty() )
	{
		return;
	}

	// Inform the hook that we want to save the preset
	m_hook->OnSavePreset( this, selectedName );

    // Save last known name
    m_lastKnownName = selectedName;
}

void CEdPresetsBox::OnDeletePresetClick( wxCommandEvent& event )
{
	String selectedName = m_presetName->GetValue().wc_str();
	selectedName.Trim();

	// If we have no hook or the preset name is empty, just ignore this
	if ( !m_hook || selectedName.Empty() )
	{
		return;
	}

	// Delete the preset
	m_presets->RemovePreset( selectedName );

	// Clear the name
	m_presetName->SetValue( wxEmptyString );
	m_lastKnownName = String::EMPTY;

	// Inform the hook
	if ( m_hook )
	{
		m_hook->OnPresetDeleted( this, selectedName );
	}
}

void CEdPresetsBox::OnMenuClick( wxCommandEvent& event )
{
}

void CEdPresetsBox::SetPresets( CEdPresets* presets )
{
	// Make sure we aren't going to do unnecessary work
	if ( presets == m_presets )
	{
		return;
	}

	// Update presets reference
	if ( m_presets != nullptr )
	{
		m_presets->SetHook( m_originalHook );
		m_originalHook = nullptr;
	}
	m_presets = presets;
	if ( m_presets != nullptr )
	{
		m_originalHook = m_presets->GetHook();
		m_presets->SetHook( m_boxHook );
	}

	// Reset last known name
	m_lastKnownName = String::EMPTY;

	// Update UI
	UpdatePresetsUI();
}

void CEdPresetsBox::UpdatePresetsUI()
{
	// Special case when we have no presets object: disable the UI
	if ( m_presets == nullptr )
	{
		m_presetName->Clear();
		m_presetName->Disable();
		m_saveButton->Disable();
		m_deleteButton->Disable();
		m_menuButton->Disable();
		return;
	}

	// Make sure the UI is enabled
	m_presetName->Enable();
	m_saveButton->Enable();
	m_deleteButton->Enable();
	m_menuButton->Enable();

	// Update presets list
	wxString previousSelection = m_presetName->GetValue();
	m_presetName->Freeze();
	m_presetName->Clear();
	TDynArray< String > names;
	m_presets->GetPresetNames( names );
	for ( auto it=names.Begin(); it != names.End(); ++it )
	{
		m_presetName->Append( (*it).AsChar() );
	}
	m_presetName->SetValue( previousSelection );
	m_presetName->Thaw();
}

void CEdPresetsBox::SetPresetsBoxHook( IEdPresetsBoxHook* hook )
{
	m_hook = hook;
}
