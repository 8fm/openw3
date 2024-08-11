#include "build.h"

#include "selectFromListDialog.h"

wxIMPLEMENT_CLASS( CEdSelectFromListDialog, wxSmartLayoutDialog );

CEdSelectFromListDialog::CEdSelectFromListDialog( wxWindow* parent, const TDynArray< String >& itemList, TDynArray< Uint32 >& selectedIndicies )
:	m_selectedIndicies( selectedIndicies )
{
	// Load window
	VERIFY( wxXmlResource::Get()->LoadDialog( this, parent, wxT( "SelectFromListDialog" ) ) );

	SetSize( 400, 500 );

	// Extract widgets
	m_listBox = XRCCTRL( *this, "CheckList", wxCheckListBox );
	ASSERT( m_listBox != NULL, TXT( "CheckList not defined in frame SelectFromListDialog in errorDialogs XRC" ) );

	m_selectAll = XRCCTRL( *this, "SelectAll", wxCheckBox );
	ASSERT( m_listBox != NULL, TXT( "SelectAll not defined in frame SelectFromListDialog in errorDialogs XRC" ) );

	wxArrayString items;

	for( Uint32 i = 0; i < itemList.Size(); ++i )
	{
		items.Add( itemList[ i ].AsChar() );
	}

	m_listBox->InsertItems( items, 0 );

	Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdSelectFromListDialog::OnOK, this, wxID_OK );
	Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdSelectFromListDialog::OnSelectAll, this );
	m_listBox->Bind( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &CEdSelectFromListDialog::OnItemSelected, this );
}

CEdSelectFromListDialog::~CEdSelectFromListDialog()
{

}

void CEdSelectFromListDialog::OnOK( wxCommandEvent& event )
{
	for( Uint32 i = 1; i < m_listBox->GetCount(); ++i )
	{
		if( m_listBox->IsChecked( i ) )
		{
			m_selectedIndicies.PushBack( i );
		}
	}

	event.Skip();
}

void CEdSelectFromListDialog::OnSelectAll( wxCommandEvent& event )
{
	for( Uint32 i = 0; i < m_listBox->GetCount(); ++i )
	{
		m_listBox->Check( i, event.IsChecked() );
	}
}

void CEdSelectFromListDialog::OnItemSelected( wxCommandEvent& event )
{
	Bool allSelected = m_listBox->IsChecked( 0 );

	for( Uint32 i = 1; i < m_listBox->GetCount(); ++i )
	{
		if( m_listBox->IsChecked( i ) != allSelected )
		{
			m_selectAll->Set3StateValue( wxCHK_UNDETERMINED );
			return;
		}
	}

	m_selectAll->Set3StateValue( allSelected ? wxCHK_CHECKED : wxCHK_UNCHECKED );
}
