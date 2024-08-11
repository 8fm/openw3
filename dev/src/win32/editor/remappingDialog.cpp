
#include "build.h"
#include "remappingDialog.h"


class CRemappingSlot : public wxPanel
{
public:

	CRemappingSlot( wxWindow* parent, CEdRemappingDialog::MappingEntry& entry )
		: m_entry( entry )
	{
		Create( parent, wxID_ANY, wxDefaultPosition, wxSize( -1, 30 ), wxCLIP_SIBLINGS | wxCLIP_CHILDREN | wxFULL_REPAINT_ON_RESIZE );
		SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

		m_check = new wxCheckBox( this, wxID_ANY, wxEmptyString );
		GetSizer()->Add( m_check, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2 );
		m_check->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CRemappingSlot::OnCheckClicked, this );			
		m_check->SetValue( entry.m_selectedIdx != -1 );

		wxStaticText* label = new wxStaticText( this, wxID_ANY, entry.m_original.AsChar(), wxDefaultPosition, wxSize( 100, -1 ) );
		GetSizer()->Add( label, 1, wxALL|wxALIGN_CENTER_VERTICAL, 2 );
		if ( !entry.m_toolTip.Empty() )
		{
			label->SetToolTip( entry.m_toolTip.AsChar() );
			label->GetToolTip()->SetMaxWidth( -1 );
		}

		wxStaticText* arrow = new wxStaticText( this, wxID_ANY, entry.m_arrowText.AsChar(), wxDefaultPosition, wxSize( 50, -1 ), wxALIGN_RIGHT );
		GetSizer()->Add( arrow, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2 );

		if ( !entry.m_possibilities.Empty() )
		{
			m_choice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( 100, -1 ) );
			GetSizer()->Add( m_choice, 1, wxALL|wxALIGN_CENTER_VERTICAL, 2 );
			for ( const String& s : entry.m_possibilities )
			{
				m_choice->AppendString( s.AsChar() );
			}

			if ( entry.m_selectedIdx >=0 && entry.m_selectedIdx < static_cast< Int32 >( m_choice->GetCount() ) )
			{
				m_choice->SetSelection( entry.m_selectedIdx );
			}
			else
			{
				m_choice->SetSelection( -1 );
				m_choice->Disable();
			}

			m_choice->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CRemappingSlot::OnChoiceSelected, this );
		}
		else
		{
			m_choice = nullptr;
			m_check->Disable();

			String name = ( entry.m_possibilities.Size() == 1 ) ? entry.m_possibilities[0] : TXT("<error>");
			wxStaticText* nameLabel = new wxStaticText( this, wxID_ANY, name.AsChar() );
			GetSizer()->Add( nameLabel, 3, wxALL|wxALIGN_CENTER_VERTICAL, 2 );
		}

		if ( !entry.m_iconResource.Empty() )
		{
			wxStaticBitmap* icon = new wxStaticBitmap( this, wxID_ANY, SEdResources::GetInstance().LoadBitmap( entry.m_iconResource.AsChar() ) );
			GetSizer()->Add( icon, 0, wxALL|wxALIGN_CENTER_VERTICAL, 2 );
		}
	}

	void Check( Bool value )
	{
		if ( m_check->IsEnabled() )
		{
			m_check->SetValue( value );
			HandleCheckClicked( value );
		}
	}

	void ChangeSelection( const String& newMappingString, Bool appendIfNotExist )
	{
		Int32 newSelection = m_entry.m_possibilities.GetIndex( newMappingString );
		if ( newSelection < 0 && appendIfNotExist )
		{
			m_entry.m_possibilities.PushBack( newMappingString );
			Sort( m_entry.m_possibilities.Begin(), m_entry.m_possibilities.End() );
			newSelection = m_entry.m_possibilities.GetIndex( newMappingString );

			if ( !m_choice )
			{
				m_choice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxSize( 100, -1 ) );
				GetSizer()->Add( m_choice, 1, wxALL|wxALIGN_CENTER_VERTICAL, 2 );
				m_choice->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &CRemappingSlot::OnChoiceSelected, this );
			}

			m_choice->Clear();
			m_choice->Enable();
			for ( const String& s : m_entry.m_possibilities )
			{
				m_choice->AppendString( s.AsChar() );
			}
		}

		m_entry.m_selectedIdx = Max( -1, newSelection );
		if ( m_choice && newSelection >=0 && newSelection < static_cast< Int32 >( m_choice->GetCount() ) )
		{
			m_choice->SetSelection( newSelection );
			m_check->SetValue( true );
		}
		else
		{
			m_choice->SetSelection( -1 );
			m_check->SetValue( false );
		}
	}

	const String& GetOriginal()
	{
		return m_entry.m_original;
	}

private:
	void OnChoiceSelected( wxCommandEvent& evt )
	{
		m_entry.m_selectedIdx = m_choice->GetSelection();
	}

	void OnCheckClicked( wxCommandEvent& evt )
	{
		HandleCheckClicked ( m_check->GetValue() );
	}

	void HandleCheckClicked( Bool value )
	{
		if ( value )
		{
			m_entry.m_selectedIdx = m_choice->GetSelection();
			m_choice->Enable();
		}
		else
		{
			m_entry.m_selectedIdx = -1;
			m_choice->Disable();
		}
	}

	wxCheckBox* m_check;
	wxChoice*   m_choice;

	CEdRemappingDialog::MappingEntry& m_entry;
};


CEdRemappingDialog::CEdRemappingDialog( wxWindow* parent, const String& title )
{
	Create( parent, wxID_ANY, title.AsChar(), wxDefaultPosition, wxSize( 550, 400 ), wxCAPTION|wxSYSTEM_MENU|wxCLOSE_BOX|wxRESIZE_BORDER|wxCLIP_SIBLINGS|wxCLIP_CHILDREN );
	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	SetMinSize( wxSize( 550, 100 ) );

	m_slotsPanel = new wxScrolled< wxPanel >( this );
	m_slotsPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
	m_slotsPanel->SetScrollRate( 5, 5 );
	GetSizer()->Add( m_slotsPanel, 1, wxEXPAND|wxALL, 5 );

	m_selectAll = new wxCheckBox( this, wxID_ANY, TXT("Select all") );
	GetSizer()->Add( m_selectAll, 0, wxALL|wxALIGN_CENTER_VERTICAL, 10 );
	m_selectAll->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdRemappingDialog::OnSelectAll ), NULL, this );			
	m_selectAll->SetValue( false );

	wxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
	GetSizer()->Add( sizer, 0, wxEXPAND, 0 );

	m_specialActionBtn = new wxButton( this, wxID_ANY );
	m_specialActionBtn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdRemappingDialog::OnSpecialActionButtonClicked ), NULL, this );
	sizer->Add( m_specialActionBtn, 0, wxALL|wxALIGN_CENTER, 5 );
	m_specialActionBtn->Hide();
	sizer->AddStretchSpacer(); 

	wxStdDialogButtonSizer* stdButtons = new wxStdDialogButtonSizer();
	stdButtons->AddButton( new wxButton( this, wxID_OK ) );
	stdButtons->AddButton( new wxButton( this, wxID_CANCEL ) );
	stdButtons->Realize();
	sizer->Add( stdButtons, 0, wxALL, 5 );
}

Bool CEdRemappingDialog::Execute( Mappings& mappings )
{
	CenterOnParent();

	ResetMappings( mappings );

	if ( ShowModal() == wxID_OK )
	{
		mappings = m_mappings;
		return true;
	}
	else
	{
		return false;
	}
}

void CEdRemappingDialog::ResetMappings( const Mappings& mappings )
{
	m_mappings = mappings;

	m_slotsPanel->Freeze();
	m_slotsPanel->DestroyChildren(); // in case of reuse

	Int32 idx = 0;
	for ( MappingEntry& mapping : m_mappings )
	{
		CRemappingSlot* slot = new CRemappingSlot( m_slotsPanel, mapping );
		m_slots.PushBack( slot );
		m_slotsPanel->GetSizer()->Add( slot, 0, wxEXPAND, 0 );
	}

	m_slotsPanel->FitInside();
	m_slotsPanel->Thaw();

	Layout();
}

void CEdRemappingDialog::SetupSpecialActionButton( const String& label, std::function< void ( CEdRemappingDialog* reamppingDlg  ) > handler )
{
	m_specialActionFunc = handler;
	m_specialActionBtn->SetLabel( label.AsChar() );
	m_specialActionBtn->Show();
	Layout();
}

void CEdRemappingDialog::OnSelectAll( wxCommandEvent& event )
{
	Bool value = m_selectAll->GetValue();
	for ( CRemappingSlot* slot : m_slots )
	{
		slot->Check( value );
	}
}

void CEdRemappingDialog::OnSpecialActionButtonClicked( wxCommandEvent& event )
{
	 m_specialActionFunc( this );
}

TDynArray< String > CEdRemappingDialog::GetOriginals()
{
	TDynArray< String > ret;
	for ( CRemappingSlot* slot : m_slots )
	{
		ret.PushBack( slot->GetOriginal() );
	}
	return ret;
}

void CEdRemappingDialog::UpdateSelection( const String& original, const String& newValue, Bool appendIfNotExist )
{
	for ( CRemappingSlot* slot : m_slots )
	{
		if ( slot->GetOriginal() == original )
		{
			slot->ChangeSelection( newValue, appendIfNotExist );
		}
	}
}
