#include "build.h"

#include "entityVoiceTagEditor.h"
#include "itemSelectionDialogs/voiceTagSelectorDialog.h"

CEntityVoiceTagEditor::CEntityVoiceTagEditor( CPropertyItem* item )
:	CEdBasicSelectorDialogPropertyEditor( item )
{

}

CEntityVoiceTagEditor::~CEntityVoiceTagEditor()
{

}

void CEntityVoiceTagEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	CEdBasicSelectorDialogPropertyEditor::CreateControls( propRect, outSpawnedControls);

	wxBitmapButton* clearButton = new wxBitmapButton( m_focusPanel, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TXT( "IMG_PB_CLEAR" ) ) );
	clearButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEntityVoiceTagEditor::OnClearDialog, this );
	m_focusPanel->GetSizer()->Add( clearButton, 0, wxEXPAND, 0 );

	m_focusPanel->Layout();
}

Bool CEntityVoiceTagEditor::GrabValue( String& displayValue )
{
	CName intermediary;
	m_propertyItem->Read( &intermediary );
	displayValue = intermediary.AsString();

	return true;
}

void CEntityVoiceTagEditor::OnSelectDialog( wxCommandEvent& event )
{
	CName currentValue;
	m_propertyItem->Read( &currentValue );

	CEdVoiceTagSelectorDialog selector( m_focusPanel, currentValue );
	if ( CName* selectedVoiceTag = selector.Execute() )
	{
		m_propertyItem->Write( selectedVoiceTag );
		m_textDisplay->ChangeValue( selectedVoiceTag->AsString().AsChar() );
	}
}

void CEntityVoiceTagEditor::OnClearDialog( wxCommandEvent& event )
{
	CName none = CName::NONE;
	m_propertyItem->Write( &none );
	m_textDisplay->ChangeValue( none.AsString().AsChar() );
}