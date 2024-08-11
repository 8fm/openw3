/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "audioEventBrowser.h"
#include "soundPropertyEditors.h"
#include "editorExternalResources.h"
#include "../../common/engine/soundSystem.h"

CAudioElementBrowserPropertyEditor::CAudioElementBrowserPropertyEditor( CPropertyItem* propertyItem )
:	ICustomPropertyEditor( propertyItem )
{
	m_iconAudioEventBrowser = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	m_iconRemoveAudioEvent = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_CLEAR") );
}

void CAudioElementBrowserPropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	CPropertyButton* button1 = m_propertyItem->AddButton( m_iconAudioEventBrowser, wxCommandEventHandler( CAudioEventBrowserPropertyEditor::OnSpawnAudioEventBrowser ), this );
	CPropertyButton* button2 = 	m_propertyItem->AddButton( m_iconRemoveAudioEvent, wxCommandEventHandler( CAudioEventBrowserPropertyEditor::OnRemoveAudioEvent ), this );

	Int32 btn1Width = 0, btn2Width = 0;
	if ( button1 )
	{
		btn1Width = button1->GetWidth();
	}
	if ( button2 )
	{
		btn2Width = button2->GetWidth();
	}

	// grab value in property
	String value;
	GrabValue( value );

	// create text box
	wxRect valueRect = propertyRect;
	valueRect.y += 3;
	valueRect.height -= 3;
	valueRect.x += 2;
	valueRect.width -= ( 2 + btn1Width + btn2Width );

	m_textBox = new wxTextCtrlEx( m_propertyItem->GetPage(), wxID_ANY, value.AsChar(), valueRect.GetTopLeft(), valueRect.GetSize(), ( wxNO_BORDER | wxTE_PROCESS_ENTER ) );
	m_textBox->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );
	m_textBox->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CAudioElementBrowserPropertyEditor::OnEditTextEnter ), NULL, this );
	m_textBox->SetFont( m_propertyItem->GetPage()->GetStyle().m_drawFont );
	m_textBox->SetSelection( -1, -1 );
	m_textBox->SetFocus();
}

void CAudioElementBrowserPropertyEditor::CloseControls()
{
	delete m_textBox;
	m_textBox = nullptr;
}

void CAudioElementBrowserPropertyEditor::OnSpawnAudioEventBrowser( wxCommandEvent &event )
{
#ifdef SOUND_DEBUG

	// Get current value
	String value;
	GrabValue( value );

	// Open sound event selector
	TDynArray< String > events = GetElements();
	CEdAudioEventSelectorDialog selector( m_propertyItem->GetPage(), &events, value );
	if ( String* selectedAudioEvent = selector.Execute() )
	{
		SetValue( *selectedAudioEvent );
	}

#endif
}

Bool CAudioElementBrowserPropertyEditor::GrabValue( String& displayValue )
{	
	displayValue = TXT("None");

	const CName typeName = m_propertyItem->GetPropertyType()->GetName();
	if( typeName == CNAME( CName ) )
	{
		CName str;
		if ( m_propertyItem->Read( &str ) )

		{
			displayValue = str.AsString();
		}
	}
	else	
	{
		String str;
		if ( m_propertyItem->Read( &str ) )
		{
			displayValue = typeName == CNAME( String ) ? str : ANSI_TO_UNICODE( str.AsChar() );
		}
	}
	return true;	
}

void CAudioElementBrowserPropertyEditor::OnRemoveAudioEvent( wxCommandEvent &event )
{
	SetValue( TXT("") );
}

void CAudioElementBrowserPropertyEditor::OnEditTextEnter( wxCommandEvent& event )
{
	String value = m_textBox->GetValue();
	SetValue( value );
}

void CAudioElementBrowserPropertyEditor::SetValue( const String& value )
{	
	const CName typeName = m_propertyItem->GetPropertyType()->GetName();
	if( typeName == CNAME( CName ) )
	{
		m_propertyItem->Write( &CName( value.AsChar() ) );
	}
	else
	{
		StringAnsi str = UNICODE_TO_ANSI( value.AsChar() );
		m_propertyItem->Write( &str );
	}

	m_propertyItem->GrabPropertyValue();

	m_textBox->SetValue( value.AsChar() );
}

CAudioEventBrowserPropertyEditor::CAudioEventBrowserPropertyEditor( CPropertyItem* propertyItem )  : CAudioElementBrowserPropertyEditor( propertyItem )
{
}

TDynArray< String > CAudioEventBrowserPropertyEditor::GetElements()
{
	return GSoundSystem->GetDefinedEvents();
}

CAudioSwitchBrowserPropertyEditor::CAudioSwitchBrowserPropertyEditor( CPropertyItem* propertyItem )  : CAudioElementBrowserPropertyEditor( propertyItem )
{
}

TDynArray< String > CAudioSwitchBrowserPropertyEditor::GetElements()
{
	return GSoundSystem->GetDefinedSwitches();
}

CSoundReverbPropertyEditor::CSoundReverbPropertyEditor( CPropertyItem* item ) : CAudioElementBrowserPropertyEditor( item )
{

}

CSoundReverbPropertyEditor::~CSoundReverbPropertyEditor()
{
	
}

TDynArray< String > CSoundReverbPropertyEditor::GetElements()
{
	return GSoundSystem->GetDefinedEnvironments();
}

CSoundBankBrowserPropertyEditor::CSoundBankBrowserPropertyEditor( CPropertyItem* propertyItem )  : CAudioElementBrowserPropertyEditor( propertyItem )
{
}

TDynArray< String > CSoundBankBrowserPropertyEditor::GetElements()
{
	return CSoundBank::GetAvaibleBanks();
}

CSoundGameParamterEditor::CSoundGameParamterEditor( CPropertyItem* propertyItem )  : CAudioElementBrowserPropertyEditor( propertyItem )
{
}

TDynArray< String > CSoundGameParamterEditor::GetElements()
{
	return GSoundSystem->GetDefinedGameParameters();
}