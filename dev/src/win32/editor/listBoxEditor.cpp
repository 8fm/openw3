#include "build.h"
#include "listBoxEditor.h"

CListBoxEditor::CListBoxEditor( CPropertyItem* item )
	: ICustomPropertyEditor( item )
	, m_listBoxCtrl( NULL )
	, m_listBoxMinHeight( 100 )
{
}

CListBoxEditor::~CListBoxEditor(void)
{
}

void CListBoxEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	Int32 width, height;
	m_propertyItem->GetPage()->GetSize( &width, &height );

	wxPoint listBoxPoint = propertyRect.GetBottomLeft();
	wxSize listBoxSize = propertyRect.GetSize();

	Int32 listboxHeight = height - propertyRect.GetBottom();
	if ( listboxHeight < m_listBoxMinHeight )
	{
		listboxHeight = m_listBoxMinHeight;
		listBoxPoint.y = propertyRect.GetTop() - listboxHeight;//m_listBoxMinHeight - listboxHeight;
	}	
	listBoxSize.SetHeight( listboxHeight );

	m_listBoxCtrl = new wxCheckListBox( m_propertyItem->GetPage(), wxID_ANY, listBoxPoint, listBoxSize, GetListElements() );
	m_listBoxCtrl->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_listBoxCtrl->SetFocus();

	SelectPropertyElements();

	m_listBoxCtrl->Connect( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, 
		wxCommandEventHandler( CListBoxEditor::OnElementSelection ), NULL, this );

	outSpawnedControls.PushBack( m_listBoxCtrl );
}

void CListBoxEditor::CloseControls()
{
	if ( m_listBoxCtrl )
	{
		delete m_listBoxCtrl;
		m_listBoxCtrl = NULL;
	}
}

void CListBoxEditor::OnElementSelection( wxCommandEvent& event )
{
	Int32 elementIndex = event.GetInt();
	wxString element = m_listBoxCtrl->GetString( elementIndex );
	if ( m_listBoxCtrl->IsChecked( elementIndex ) )
	{
		SelectElement( element );
	}
	else
	{
		DeselectElement( element );
	}

	m_propertyItem->GrabPropertyValue();
}