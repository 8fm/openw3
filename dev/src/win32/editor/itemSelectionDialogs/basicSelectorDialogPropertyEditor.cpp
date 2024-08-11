#include "build.h"
#include "basicSelectorDialogPropertyEditor.h"

CEdBasicSelectorDialogPropertyEditor::CEdBasicSelectorDialogPropertyEditor( CPropertyItem* item )
:	ICustomPropertyEditor( item ),
	m_focusPanel( nullptr ),
	m_textDisplay( nullptr )
{

}

CEdBasicSelectorDialogPropertyEditor::~CEdBasicSelectorDialogPropertyEditor()
{

}

void CEdBasicSelectorDialogPropertyEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	if( !m_focusPanel )
	{
		m_focusPanel = new wxPanel( m_propertyItem->GetPage(), wxID_ANY, propRect.GetTopLeft(), propRect.GetSize() );

		wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );

		String defaultValue;
		GrabValue( defaultValue );

		m_textDisplay = new wxTextCtrlEx( m_focusPanel, wxID_ANY, defaultValue.AsChar(), wxDefaultPosition, wxDefaultSize, wxTE_NO_VSCROLL | wxNO_BORDER | wxTE_RICH2 | wxTE_READONLY );
		sizer->Add( m_textDisplay, 1, wxEXPAND, 0 );

		wxBitmapButton* selectButton = new wxBitmapButton( m_focusPanel, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TXT( "IMG_PB_PICK" ) ) );
		selectButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdBasicSelectorDialogPropertyEditor::OnSelectDialog, this );
		sizer->Add( selectButton, 0, wxEXPAND, 0 );

		m_focusPanel->SetSizer( sizer );
		m_focusPanel->Layout();

		m_textDisplay->SetFocus();
	}
}

void CEdBasicSelectorDialogPropertyEditor::CloseControls()
{
	if( m_focusPanel )
	{
		m_focusPanel->Destroy();
		m_focusPanel = NULL;
	}
}
