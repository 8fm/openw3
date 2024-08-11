#include "build.h"

CEdFileHistoryDialog::CEdFileHistoryDialog( wxWindow *parent, const String &file, 
										    const TDynArray< THashMap< String, String > > &history )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("FileHistoryDialog") );

	// Set the file's history label
	wxStaticText *m_label = XRCCTRL( *this, "Label", wxStaticText );
	wxString label = wxT("History of: ");
	label += file.AsChar();
	m_label->SetLabel( label );

	m_tree = XRCCTRL( *this, "HistoryTree", wxTreeListCtrl );
	m_tree->SetWindowStyle( m_tree->GetWindowStyle() & ~wxTR_HIDE_ROOT );
	m_tree->SetWindowStyle( m_tree->GetWindowStyle() & ~wxTR_ROW_LINES & ~wxTR_COLUMN_LINES );
	
	m_tree->AddColumn( wxT("Revision") );
	m_tree->AddColumn( wxT("Changelist") );
	m_tree->AddColumn( wxT("Action") );
	m_tree->AddColumn( wxT("Date") );
	m_tree->AddColumn( wxT("Submitter") );
	m_tree->AddColumn( wxT("Type") );
	m_tree->AddColumn( wxT("Description") );
	
	wxTreeItemId root = m_tree->AddRoot( file.AsChar() );

	for ( Uint32 i = 0; i < history.Size(); i++ )
	{
		const THashMap< String, String > &map = history[i];
		wxTreeItemId item;
		String value;
		if ( map.Find( TXT("Revision"), value) )
		{
			item = m_tree->AppendItem( root, value.AsChar() );
			if ( map.Find( TXT("Changelist"), value) )
				m_tree->SetItemText( item, 1, value.AsChar() );
			if ( map.Find( TXT("Action"), value) )
				m_tree->SetItemText( item, 2, value.AsChar() );
			if ( map.Find( TXT("Date"), value) )
				m_tree->SetItemText( item, 3, value.AsChar() );
			if ( map.Find( TXT("Submitter"), value) )
				m_tree->SetItemText( item, 4, value.AsChar() );
			if ( map.Find( TXT("Type"), value) )
				m_tree->SetItemText( item, 5, value.AsChar() );
			if ( map.Find( TXT("Description"), value) )
				m_tree->SetItemText( item, 6, value.AsChar() );
		}
	}
	m_tree->ExpandAll( root );
	Center();
}

CEdFileHistoryDialog::~CEdFileHistoryDialog()
{
	delete m_tree;
}