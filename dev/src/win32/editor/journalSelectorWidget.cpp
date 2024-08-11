#include "build.h"
#include "journalSelectorWidget.h"
#include "journalTree.h"

#include "../../common/game/journalPath.h"

IMPLEMENT_CLASS( CJournalSelectorWidget, wxTextCtrl );
wxDEFINE_EVENT( wxEVT_PATH_SELECTED, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_PATH_CLEARED, wxCommandEvent );

CJournalSelectorWidget::CJournalSelectorWidget( wxWindow* parent, wxWindowID id, Uint32 journalFlags, const CClass* typeSelectable )
:	wxTextCtrl( parent, id ),
	m_flags( journalFlags ),
	m_typeSelectable( typeSelectable ),
	m_path( NULL )
{
	m_selectorWidget = new CEdJournalTree();

	m_selectorWidget->Create
	(
		parent,
		wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT | wxTR_LINES_AT_ROOT | wxTR_SINGLE | wxBORDER
	);

	m_selectorWidget->Initialize();

	Bind( wxEVT_SET_FOCUS, &CJournalSelectorWidget::OnFocus, this );

	m_selectorWidget->Bind( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, &CJournalSelectorWidget::OnItemSelected, this );
	m_selectorWidget->Bind( wxEVT_KILL_FOCUS, &CJournalSelectorWidget::OnFocusLost, this );

	m_selectorWidget->Hide();
}

CJournalSelectorWidget::~CJournalSelectorWidget()
{
}

void CJournalSelectorWidget::SetPath( THandle< CJournalPath > path )
{
	m_path = path;
	if( !m_selectorWidget->IsShown() )
	{
		SetValue( m_path? m_path->GetPathAsString().AsChar(): wxT("") );
	}
}

void CJournalSelectorWidget::OnItemSelected( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();

	wxTreeItemData* data = m_selectorWidget->GetItemData( item );

	if( data )
	{
		CEdJournalTreeItemData* journalData = static_cast< CEdJournalTreeItemData* >( data );

		if( !m_typeSelectable || journalData->m_entry->IsA( m_typeSelectable ) )
		{
			m_path = CJournalPath::ConstructPathFromTargetEntry( journalData->m_entry, journalData->m_resource );

			m_selectorWidget->Hide();

			// Send to parent
			wxCommandEvent event( wxEVT_PATH_SELECTED );
			event.SetEventObject( this );
			ProcessEvent( event );

			SetValue( m_path->GetPathAsString().AsChar() );
			Show();
		}
	}
}

void CJournalSelectorWidget::OnFocus( wxFocusEvent& event )
{
	wxPoint position = GetPosition();
	wxSize size = GetSize();
	size.SetHeight( 200 );

	m_selectorWidget->SetPosition( position );
	m_selectorWidget->SetSize( size );

	m_selectorWidget->DeleteAllItems();
	for( Uint32 i = 0; i < CEdJournalTree::TreeCategory_Max; ++i )
	{
		if( ( m_flags & ( 1 << i ) ) )
		{
			// Are there other categories specified?
			if( !ISPOW2( m_flags ) )
			{
				m_selectorWidget->AddCategoryRoot( static_cast< CEdJournalTree::eTreeCategory >( i ) );
			}

			m_selectorWidget->PopulateTreeSection( static_cast< CEdJournalTree::eTreeCategory >( i ) );
		}
	}

	if( m_path )
	{
		m_selectorWidget->ExpandPath( m_path );
	}

	m_selectorWidget->Show();
	m_selectorWidget->SetFocus();
	Hide();
}

void CJournalSelectorWidget::OnFocusLost( wxFocusEvent& event )
{
	m_selectorWidget->Hide();
	Show();
}

void CJournalSelectorWidget::OnItemCleared()
{
	wxCommandEvent event( wxEVT_PATH_CLEARED );
	event.SetEventObject( this );
	ProcessEvent( event );
}
