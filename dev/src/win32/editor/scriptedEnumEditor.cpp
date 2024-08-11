/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "scriptedEnumEditor.h"

CEdScriptedEnumPropertyEditor::CEdScriptedEnumPropertyEditor( CPropertyItem* item )
	: ICustomPropertyEditor( item )
, m_ctrlChoice( NULL )
{
	ASSERT( item->GetPropertyType()->GetName() == CNAME( Int32 ) );
}

void CEdScriptedEnumPropertyEditor::CloseControls()
{
	// Close combo box
	if ( m_ctrlChoice )
	{
		delete m_ctrlChoice;
		m_ctrlChoice = NULL;
	}
}

Bool CEdScriptedEnumPropertyEditor::GrabValue( String& displayValue )
{
	// Get value from target
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( Int32 ) )
	{
		Int32 value;
		m_propertyItem->Read( &value );

		String enumName = m_propertyItem->GetCustomEditorType().StringAfter( TXT("ScriptedEnum_" ) );

		// Fill choice control with values
		CEnum* en = SRTTI::GetInstance().FindEnum( CName( enumName ) );
		if ( en )
		{
			CName name;
			en->FindName( value, name );

			displayValue = name.AsString();

			return true;
		}
	}

	return false;
}

Bool CEdScriptedEnumPropertyEditor::SaveValue()
{
	String value = m_ctrlChoice->GetString( m_ctrlChoice->GetSelection() ).wc_str();

	// Write result
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( Int32 ) )
	{
		Int32 valueInt = 0;
		FromString( value, valueInt );

		String enumName = m_propertyItem->GetCustomEditorType().StringAfter( TXT("ScriptedEnum_" ) );

		// Fill choice control with values
		CEnum* en = SRTTI::GetInstance().FindEnum( CName( enumName ) );
		if ( en )
		{
			Int32 valueInt = 0;
			en->FindValue( CName( value ), valueInt );

			m_propertyItem->Write( &valueInt );

			return true;
		}
	}

	return false;
}

void CEdScriptedEnumPropertyEditor::OnChoiceChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void CEdScriptedEnumPropertyEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	String enumName = m_propertyItem->GetCustomEditorType().StringAfter( TXT("ScriptedEnum_" ) );

	// Fill choice control with values
	CEnum* en = SRTTI::GetInstance().FindEnum( CName( enumName ) );
	if ( !en )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		const TDynArray< CName > &enumVals = en->GetOptions();

		for ( Uint32 i=0; i<enumVals.Size(); i++ )
		{
			m_ctrlChoice->AppendString( enumVals[i].AsString().AsChar() );
		}

		// Find current value on list and select it
		String str;
		GrabValue( str );
		int index = m_ctrlChoice->FindString( str.AsChar() );
		if ( index >= 0 )
		{
			m_ctrlChoice->SetSelection( index );
		}
		else
		{
			m_ctrlChoice->SetSelection( 0 );
		}

		// Notify of selection changes
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdScriptedEnumPropertyEditor::OnChoiceChanged ), NULL, this );
	}

}
