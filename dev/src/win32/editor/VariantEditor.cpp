/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "variantEditor.h"

CVariantEditor::CVariantEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlEdit( NULL ), m_ctrlType( NULL ), m_panel( NULL )
{
}

void CVariantEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_panel = new wxPanel(m_propertyItem->GetPage(), wxID_ANY, propRect.GetTopLeft(), propRect.GetSize());
	wxBoxSizer* sizer1 = new wxBoxSizer( wxHORIZONTAL );

	m_ctrlType = new CEdChoice(m_panel, wxDefaultPosition, wxDefaultSize, true );
	m_ctrlType->SetWindowStyle( wxNO_BORDER );
	m_ctrlType->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	m_ctrlType->AppendString( RED_NAME( Bool ).AsString().AsChar() );
	m_ctrlType->AppendString( RED_NAME( Float ).AsString().AsChar() );
	m_ctrlType->AppendString( RED_NAME( Int32 ).AsString().AsChar() );
	m_ctrlType->AppendString( RED_NAME( CName ).AsString().AsChar() );
	m_ctrlType->AppendString( RED_NAME( CEntity ).AsString().AsChar() );
	m_ctrlType->AppendString( RED_NAME( TagList ).AsString().AsChar() );

	m_ctrlEdit = new wxTextCtrlEx(m_panel, wxID_ANY, wxT(""));
	m_ctrlEdit->SetWindowStyle( wxNO_BORDER | wxTE_PROCESS_ENTER );
	m_ctrlEdit->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	sizer1->Add( m_ctrlType, 2, wxEXPAND, 0 );
	sizer1->Add( m_ctrlEdit, 4, wxEXPAND, 0 );

	m_panel->SetSizer(sizer1);
	m_panel->Layout();

	m_ctrlType->SetFocus();

	// Display current value
	String type, value;
	GrabValue( type, value );
	m_ctrlType->SetValue( type.AsChar() );
	m_ctrlEdit->SetValue( value.AsChar() );

	m_ctrlType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CVariantEditor::OnChoiceChanged ), NULL, this );
	m_ctrlEdit->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CVariantEditor::OnChoiceChanged ), NULL, this );
}

void CVariantEditor::CloseControls()
{
	// Close combo box
	if ( m_ctrlEdit )
	{
		delete m_ctrlEdit;
		m_ctrlEdit = NULL;
	}
	if ( m_ctrlType )
	{
		delete m_ctrlType;
		m_ctrlType = NULL;
	}
	if ( m_panel )
	{
		delete m_panel;
		m_panel = NULL;
	}
}

Bool CVariantEditor::GrabValue( String& displayType, String& displayValue )
{
	CVariant var;
	m_propertyItem->Read( &var );
	displayType = var.GetType().AsString();
	displayValue = var.ToString();
	return true;
}

Bool CVariantEditor::SaveValue()
{
	if ( m_ctrlEdit == NULL || m_ctrlType == NULL )
	{
		return false;
	}

	String type = m_ctrlType->GetValue().wc_str();
	String value = m_ctrlEdit->GetValue().wc_str();
	CVariant var;
	var.Init( CName( type ), NULL );
	var.FromString( value.AsChar() );

	m_propertyItem->Write( &var );
	return true;
}

void CVariantEditor::OnChoiceChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////


CVariantEnumEditor::CVariantEnumEditor( CPropertyItem* propertyItem )
: ICustomPropertyEditor( propertyItem )
, m_ctrlValue( NULL ), m_ctrlType( NULL ), m_panel( NULL )
{
}

void CVariantEnumEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_panel = new wxPanel(m_propertyItem->GetPage(), wxID_ANY, propRect.GetTopLeft(), propRect.GetSize());
	wxBoxSizer* sizer1 = new wxBoxSizer( wxHORIZONTAL );

	m_ctrlType = new CEdChoice(m_panel, wxPoint(0, 0), wxDefaultSize);
	m_ctrlType->SetWindowStyle( wxNO_BORDER );
	m_ctrlType->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	String strType, strValue;
	GrabValue( strType, strValue );

	// Fill choice control with values
	TDynArray< CEnum* > enums;
	SRTTI::GetInstance().EnumEnums( enums );

	m_ctrlType->Freeze();

	if ( !enums.Size() )
	{
		// Add empty stub
		m_ctrlType->Enable( false );
		m_ctrlType->SetSelection( 0 );
	}
	else
	{

		// Add enums
		TDynArray< String > enumNames;
		for ( Uint32 i=0; i<enums.Size(); i++ )
		{
			enumNames.PushBack( enums[i]->GetName().AsString() );
		}

		// Sort list and add to combo box
		Sort( enumNames.Begin(), enumNames.End() );
		for ( Uint32 i=0; i<enumNames.Size(); i++ )
		{
			m_ctrlType->AppendString( enumNames[i].AsChar() );
		}

		// Find current value on list and select it		
		if( !m_ctrlType->SetStringSelection( strType.AsChar() ) )
		{
			m_ctrlType->SetSelection( 0 );
		}
	}

	m_ctrlValue = new CEdChoice(m_panel, wxPoint(0, 0), wxDefaultSize);
	m_ctrlValue->SetWindowStyle( wxNO_BORDER  );
	m_ctrlValue->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

	sizer1->Add( m_ctrlType, 1, wxEXPAND, 0 );
	sizer1->Add( m_ctrlValue, 1, wxEXPAND, 0 );

	m_panel->SetSizer(sizer1);
	m_panel->Layout();

	m_ctrlType->SetFocus();	

	m_ctrlType->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CVariantEnumEditor::OnTypeChanged ), NULL, this );
	m_ctrlValue->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CVariantEnumEditor::OnValueChanged ), NULL, this );

	m_ctrlType->Thaw();

	FillEnumValues(true, strValue );
}

void CVariantEnumEditor::FillEnumValues( Bool setValue, const String& strValue )
{
	m_ctrlValue->Freeze();
	static_cast<wxItemContainer*>( m_ctrlValue )->Clear();

	CName nameType =  CName( static_cast<wxItemContainerImmutable*>( m_ctrlType )->GetStringSelection().wc_str() );

	CEnum* e = SRTTI::GetInstance().FindEnum( nameType );	
	if( e )
	{
		m_ctrlValue->Enable( true );
		const TDynArray< CName >& options = e->GetOptions();
		for( Uint32 i=0; i<options.Size(); i++ )
		{
			m_ctrlValue->AppendString( options[i].AsString().AsChar() );
		}

		if( setValue )
		{
			if( !m_ctrlValue->SetStringSelection( strValue.AsChar() ) )
			{
				m_ctrlValue->SetSelection( 0 );
			}
		}
		else
		{
			m_ctrlValue->SetSelection( 0 );
		}
	}
	else
	{
		m_ctrlValue->Enable( false );
	}

	m_ctrlValue->Thaw();
}

void CVariantEnumEditor::CloseControls()
{
	// Close combo box
	if ( m_ctrlValue )
	{
		delete m_ctrlValue;
		m_ctrlValue = NULL;
	}
	if ( m_ctrlType )
	{
		delete m_ctrlType;
		m_ctrlType = NULL;
	}
	if ( m_panel )
	{
		delete m_panel;
		m_panel = NULL;
	}
}

Bool CVariantEnumEditor::GrabValue( String& displayType, String& displayValue )
{
	CVariant var;
	m_propertyItem->Read( &var );
	displayType = var.GetType().AsString();
	displayValue = var.ToString();
	return true;
}

Bool CVariantEnumEditor::SaveValue()
{
	if ( m_ctrlValue == NULL || m_ctrlType == NULL )
	{
		return false;
	}

	String type = static_cast<wxItemContainerImmutable*>( m_ctrlType )->GetStringSelection().wc_str();
	String value = static_cast<wxItemContainerImmutable*>( m_ctrlValue )->GetStringSelection().wc_str();
	CVariant var;
	var.Init( CName( type ), NULL );
	var.FromString( value.AsChar() );

	m_propertyItem->Write( &var );
	return true;
}

void CVariantEnumEditor::OnTypeChanged( wxCommandEvent &event )
{
	FillEnumValues( false, String::EMPTY );
	m_propertyItem->SavePropertyValue();		
}

void CVariantEnumEditor::OnValueChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();	
}
