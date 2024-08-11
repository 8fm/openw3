/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "localizedStringEditor.h"
#include "stringSelectorTool.h"
#include "../../common/core/configVarSystem.h"
#include "../../common/core/configVarLegacyWrapper.h"
#include "../../common/engine/localizationManager.h"

CLocalizedStringEditor::CLocalizedStringEditor( CPropertyItem* item, Bool allowIdReset )
	: ICustomPropertyEditor( item )
	, m_stringEditField( NULL )
	, m_stringIdField( NULL )
	, m_panel( NULL )
	, m_allowIdReset( allowIdReset )
	, m_unlockStringId( false )
	, m_stringSelectorTool( false )
{
	Int32 unlockLevel = 0;
	// THIS IS AN EXCEPTION - we allow this to be taken from normal config
	SConfig::GetInstance().GetLegacy().ReadParam( TXT("user"), TXT("Editor"), TXT("UnlockStringId"), unlockLevel );
	if ( unlockLevel == 0 )
	{
		m_allowIdReset = false;
		m_unlockStringId = false;
	}
	else if ( unlockLevel == 1 )
	{
		m_allowIdReset = false;
		m_unlockStringId = true;
	}
	else if ( unlockLevel == 1 )
	{
		m_allowIdReset = true;
		m_unlockStringId = true;
	}
}

void CLocalizedStringEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	m_panel = new wxPanel(m_propertyItem->GetPage(), wxID_ANY, propRect.GetTopLeft(), propRect.GetSize());
	wxBoxSizer* sizer1 = new wxBoxSizer( wxHORIZONTAL );

	long idFieldStyle = wxNO_BORDER | wxTE_PROCESS_ENTER;
	
	m_stringIdField = new wxTextCtrlEx( m_panel, wxID_ANY, wxT( "" ) );
	m_stringIdField->SetWindowStyle( idFieldStyle  );
	m_stringIdField->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_INACTIVEBORDER ) );
	m_stringIdField->Enable( m_unlockStringId  );

	m_stringEditField = new wxTextCtrlEx( m_panel, wxID_ANY, wxT( "" ) );
	m_stringEditField->SetWindowStyle( wxNO_BORDER | wxTE_PROCESS_ENTER );
	m_stringEditField->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_stringEditField->Enable( true );

	sizer1->Add( m_stringEditField, 4, wxEXPAND, 0 );
	sizer1->Add( m_stringIdField, 1, wxEXPAND, 0 );

	if ( m_allowIdReset == true )
	{
		wxBitmapButton* resetButton = new wxBitmapButton( m_panel, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_DELETE") ) );
		resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CLocalizedStringEditor::OnResetId ), NULL, this );
		sizer1->Add( resetButton, 0, wxEXPAND, 0 );
	}

	wxBitmapButton* selectStringButton = new wxBitmapButton( m_panel, wxID_ANY, SEdResources::GetInstance().LoadBitmap( TXT("IMG_PB_PICK") ) );
	selectStringButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CLocalizedStringEditor::OnSelectStringDialog ), NULL, this );
	sizer1->Add( selectStringButton, 0, wxEXPAND, 0 );

	m_panel->SetSizer(sizer1);
	m_panel->Layout();

	m_stringEditField->SetFocus();

	// Display current value
	Uint32 stringId = 0;
	String stringValue;
	GrabValue( stringId, stringValue );
	
	m_stringIdField->ChangeValue( ToString( stringId ).AsChar() );

	m_stringEditField->ChangeValue( stringValue.AsChar() );

	m_stringEditField->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CLocalizedStringEditor::OnStringValueChanged ), NULL, this );
	m_stringEditField->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CLocalizedStringEditor::OnKeyDown ), NULL, this );

	m_stringIdField->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CLocalizedStringEditor::OnStringIdChanged ), NULL, this );
	//m_stringIdField->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CLocalizedStringEditor::OnStringIdChanged ), NULL, this );
}

void CLocalizedStringEditor::CloseControls()
{
	if ( m_stringEditField )
	{
		delete m_stringEditField;
		m_stringEditField = NULL;
	}
	if ( m_stringIdField )
	{
		delete m_stringIdField;
		m_stringIdField = NULL;
	}
	if ( m_panel )
	{
		delete m_panel;
		m_panel = NULL;
	}
}

Bool CLocalizedStringEditor::GrabValue( String& stringValue )
{
	Uint32 stringId;
	GrabValue( stringId, stringValue );

	return true;
}

Bool CLocalizedStringEditor::GrabValue( Uint32& stringId, String& stringValue )
{
	LocalizedString localizedString;
	m_propertyItem->Read( &localizedString );
	stringId = localizedString.GetIndex();
	stringValue = localizedString.GetString();
	return true;
}

Bool CLocalizedStringEditor::SaveValue()
{
	if ( m_stringEditField == NULL || m_stringIdField == NULL )
	{
		return false;
	}

	Uint32 stringId;
	FromString( m_stringIdField->GetValue().wc_str(), stringId );
	
	String stringValue = m_stringEditField->GetValue().wc_str();

	LocalizedString localizedString;
	localizedString.SetIndex( stringId );
	localizedString.SetString( stringValue );

	m_propertyItem->Write( &localizedString );
	return true;
}

void CLocalizedStringEditor::OnStringValueChanged( wxCommandEvent& event )
{
	m_propertyItem->SavePropertyValue();
}

void CLocalizedStringEditor::OnKeyDown( wxKeyEvent& event )
{
	event.Skip();
}

void CLocalizedStringEditor::OnResetId( wxCommandEvent& event )
{
	if ( m_allowIdReset == false )
	{
		return;
	}

	if ( YesNo( TXT( "Are you REALLY REALLY SURE you want to reset this string id?" ) ) == true )
	{
		m_stringIdField->SetValue( ToString( 0 ).AsChar() );
	}
}

void CLocalizedStringEditor::OnStringIdChanged( wxCommandEvent& event )
{
	Uint32 stringId;
	FromString( m_stringIdField->GetValue().wc_str(), stringId );
	
	String stringValue = SLocalizationManager::GetInstance().GetLocalizedText( stringId, SLocalizationManager::GetInstance().GetCurrentLocale() );
	m_stringEditField->ChangeValue( stringValue.AsChar() );

	m_propertyItem->SavePropertyValue();
}

void CLocalizedStringEditor::OnSelectStringDialog( wxCommandEvent& event )
{
	if( !m_stringSelectorTool )
	{
		m_stringSelectorTool = new CEdStringSelector( m_panel );

		m_stringSelectorTool->Bind( wxEVT_CHOOSE_STRING_OK, wxCommandEventHandler( CLocalizedStringEditor::OnSelectStringDialogOK ), this );
		m_stringSelectorTool->Bind( wxEVT_CHOOSE_STRING_CANCEL, wxEventHandler( CLocalizedStringEditor::OnSelectStringDialogCancel ), this );

		m_stringSelectorTool->Show();
	}
}

void CLocalizedStringEditor::OnSelectStringDialogOK( wxCommandEvent& event )
{
	Uint32 stringID = event.GetInt();
	m_stringIdField->ChangeValue( ToString( stringID ).AsChar() );

	OnStringIdChanged( event );

	m_stringSelectorTool = false;
}

void CLocalizedStringEditor::OnSelectStringDialogCancel( wxEvent& event )
{
	m_stringSelectorTool = false;
}