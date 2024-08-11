#include "build.h"
#include "directorySelectionEditor.h"

#include "../../common/core/depot.h"

CEdDirectorySelectionEditor::CEdDirectorySelectionEditor( CPropertyItem* propertyItem )
:	ICustomPropertyEditor( propertyItem ),
	m_focusPanel( NULL )
{

}

CEdDirectorySelectionEditor::~CEdDirectorySelectionEditor()
{

}

void CEdDirectorySelectionEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	if( !m_focusPanel )
	{
		m_focusPanel = new wxPanel( m_propertyItem->GetPage(), wxID_ANY, propertyRect.GetTopLeft(), propertyRect.GetSize() );

		wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );

		String path;
		m_propertyItem->Read( &path );

		m_textDisplay = new wxTextCtrlEx( m_focusPanel, wxID_ANY, path.AsChar(), wxDefaultPosition, wxDefaultSize, wxTE_READONLY | wxTE_NO_VSCROLL | wxNO_BORDER | wxCLIP_CHILDREN );
		sizer->Add( m_textDisplay, 1, wxEXPAND, 0 );

		wxBitmapButton* selectBtn = new wxBitmapButton( m_focusPanel, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TXT( "IMG_PB_USE" ) ), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | wxCLIP_CHILDREN );
		sizer->Add( selectBtn, 0, wxEXPAND, 0 );

		m_focusPanel->SetSizer( sizer );
		m_focusPanel->Layout();

		m_textDisplay->SetFocus();
		
		selectBtn->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdDirectorySelectionEditor::OnSelectDirectory, this );
	}
}

void CEdDirectorySelectionEditor::CloseControls()
{
	if( m_focusPanel )
	{
		m_focusPanel->Destroy();
		m_focusPanel = NULL;

		m_textDisplay = NULL;
	}
}

void CEdDirectorySelectionEditor::OnSelectDirectory( wxCommandEvent& event )
{
	String path = GetActiveDirectory();

	CDirectory* dir = GDepot->FindPath( path.AsChar() );

	if( dir )
	{
		m_propertyItem->Write( &path );
		m_textDisplay->ChangeValue( path.AsChar() );
	}
}

Bool CEdDirectorySelectionEditor::GrabValue( String& displayValue )
{
	m_propertyItem->Read( &displayValue );
	return true;
}
