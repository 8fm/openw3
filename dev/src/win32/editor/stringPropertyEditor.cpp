#include "build.h"
#include "stringPropertyEditor.h"

CStringPropertyEditor::CStringPropertyEditor( CPropertyItem *propertyItem, const String &boxTitle, const String &boxMessage )
	: ICustomPropertyEditor( propertyItem )
	, m_boxTitle( boxTitle )
	, m_boxMessage( boxMessage )
	, m_ctrlText( NULL )
{
	m_icon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );

	if ( m_propertyItem->GetPropertyType()->GetName() == TXT("String") )
	{
		m_isPropertyCName = false;
	}
	else if ( m_propertyItem->GetPropertyType()->GetName() == TXT("CName") )
	{
		m_isPropertyCName = true;
	}
	else
	{
		ASSERT( 0 && TXT("String custom editor works only for String or CName") );
	}
}

void CStringPropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Calculate placement, hacked !
	wxRect valueRect;
	valueRect.y = propertyRect.y + 3;
	valueRect.height = propertyRect.height - 3;
	valueRect.x = propertyRect.x + 2;
	valueRect.width = propertyRect.width - propertyRect.height * 2 - 2;

	// Create text editor
	m_ctrlText = new wxTextCtrlEx( m_propertyItem->GetPage(), wxID_ANY,
		wxEmptyString, valueRect.GetTopLeft(), valueRect.GetSize(), wxNO_BORDER | wxTE_PROCESS_ENTER | ( m_propertyItem->GetProperty()->IsReadOnly() ? wxTE_READONLY : 0 ));
	m_ctrlText->Connect( wxEVT_KEY_DOWN, wxKeyEventHandler( CStringPropertyEditor::OnEditKeyDown ), NULL, this );
	m_ctrlText->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CStringPropertyEditor::OnEditTextEnter ), NULL, this );
	m_ctrlText->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_ctrlText->SetFont( m_propertyItem->GetPage()->GetStyle().m_drawFont );
	m_ctrlText->SetSelection( -1, -1 );
	m_ctrlText->SetFocus();

	m_propertyItem->AddButton( m_icon, wxCommandEventHandler( CStringPropertyEditor::OnSpawnStringEditor ), this );
}

void CStringPropertyEditor::OnSpawnStringEditor( wxCommandEvent &event )
{
	String strText;
	if ( InputBox( m_propertyItem->GetPage(), m_boxTitle, m_boxMessage, strText, true ) )
	{
		if ( m_isPropertyCName )
		{
			CName text;
			if ( m_propertyItem->Read( &text ) )
			{
				text = CName( strText );
				m_propertyItem->Write( &text );
			}
		}
		else
		{
			String text;
			if ( m_propertyItem->Read( &text ) )
			{
				text = strText;
				m_propertyItem->Write( &text );
			}
		}

		m_propertyItem->GrabPropertyValue();
	}
}

void CStringPropertyEditor::OnEditTextEnter( wxCommandEvent& event )
{
	m_propertyItem->SavePropertyValue();
}

void CStringPropertyEditor::OnEditKeyDown( wxKeyEvent& event )
{
	// Navigation, hacked but code is safer that way because
	// we wont call lostFocus() when in the middle of message processing
	if ( event.GetKeyCode() == WXK_UP )
	{
		PostMessage( (HWND) m_propertyItem->GetPage()->GetHandle(), WM_KEYDOWN, VK_UP, 0 );
		return;
	}
	else if ( event.GetKeyCode() == WXK_DOWN )
	{
		PostMessage( (HWND) m_propertyItem->GetPage()->GetHandle(), WM_KEYDOWN, VK_DOWN, 0 );
		return;
	}

	// Escape, restore original value
	if ( event.GetKeyCode() == WXK_ESCAPE )
	{
		m_propertyItem->GrabPropertyValue();
		return;
	}

	// Allow text ctrl to process key
	event.Skip();
}

Bool CStringPropertyEditor::GrabValue( String& displayValue )
{
	displayValue = String::EMPTY;

	if ( m_isPropertyCName )
	{
		CName text;
		if ( m_propertyItem->Read( &text ) )
		{
			displayValue = text.AsString();
		}
	}
	else
	{
		String text;
		if ( m_propertyItem->Read( &text ) )
		{
			displayValue = text;
		}
	}

	if ( m_ctrlText )
	{
		m_ctrlText->SetValue( displayValue.AsChar() );
	}

	return true; // overridden
}

Bool CStringPropertyEditor::SaveValue()
{
	if ( m_ctrlText )
	{
		String text = m_ctrlText->GetValue().wc_str();
		
		if ( m_isPropertyCName )
		{
			CName tmpText( text );
			m_propertyItem->Write( &tmpText );
		}
		else
		{
			String tmpText = text.AsChar();
			m_propertyItem->Write( &tmpText );
		}

		m_propertyItem->GrabPropertyValue();
	}
	return true;
}

void CStringPropertyEditor::CloseControls()
{
	if ( m_ctrlText )
	{
		delete m_ctrlText;
		m_ctrlText = NULL;
	}
}
