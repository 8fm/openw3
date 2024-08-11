/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "spawnsetPhasesEditor.h"

#include "../../common/game/communityData.h"

#include "sortNames.h"


CSpawnsetPhasesEditor::CSpawnsetPhasesEditor( CPropertyItem* item )
: CListSelection( item )
, m_ctrl( NULL )
{
}

void CSpawnsetPhasesEditor::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxArrayString choices;
	m_ctrl = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize(), true, wxTE_PROCESS_ENTER ); 
	m_ctrl->Append( choices );
	m_ctrl->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrl->SetFocus();
	m_ctrl->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( CSpawnsetPhasesEditor::OnChoiceChanged ), NULL, this );
	// Fill in the editor with data

	THashSet< CName > propertiesMap;
	Bool doPropertiesExist = GetProperties( propertiesMap );
	ASSERT( doPropertiesExist );
	TDynArray< CName > propertiesArray;
	propertiesArray.Reserve( propertiesMap.Size() );
	for ( auto it = propertiesMap.Begin(), end = propertiesMap.End(); it != end; ++it )
	{
		propertiesArray.PushBack( *it );
	}

	::SortNames( propertiesArray );

	String oldValue;
	GrabValue( oldValue );
	RefreshValues( propertiesArray );
	m_ctrl->SetValue( wxString( oldValue.AsChar() ) );
}

void CSpawnsetPhasesEditor::CloseControls()
{
	if ( m_ctrl )
	{
		delete m_ctrl;
		m_ctrl = NULL;
	}
}

Bool CSpawnsetPhasesEditor::GrabValue( String& displayValue )
{
	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( String ) )
	{
		m_propertyItem->Read( &displayValue );
	}
	else if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		CName propertyValue;
		m_propertyItem->Read( &propertyValue );
		displayValue = propertyValue.AsString();
	}
	else
	{
		HALT( "Type: %s is not a valid spawnset phase name type", m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
	}

	return true;
}

Bool CSpawnsetPhasesEditor::SaveValue()
{
	String value( m_ctrl->GetValue().wc_str() );

	if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( String ) )
	{
		m_propertyItem->Write( &value );
	}
	else if ( m_propertyItem->GetPropertyType()->GetName() == CNAME( CName ) )
	{
		m_propertyItem->Write( &CName(value) );
	}
	else
	{
		HALT( "Type: %s is not a valid spawnset phase name type", m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
	}

	return true;
}

void CSpawnsetPhasesEditor::OnChoiceChanged( wxCommandEvent &event )
{
	int selIdx = m_ctrl->GetSelection();
	wxString newVal = static_cast<wxItemContainerImmutable*>( m_ctrl )->GetStringSelection();

	m_ctrl->SetValue( newVal.wc_str() );
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void CSpawnsetPhasesEditor::RefreshValues( const TDynArray< CName >& properties )
{
	static_cast<wxItemContainer*>( m_ctrl )->Clear();

	// add a null entry so that we have a possibility of clearing the value
	m_ctrl->Append( wxT( "" ) );

	// add entires from the array
	for ( auto it = properties.Begin(), end = properties.End(); it != end; ++it )
	{
		m_ctrl->Append( it->AsString().AsChar() );
	}
}

Bool CSpawnsetPhasesEditor::GetProperties( THashSet< CName >& outProperties )
{
	ISpawnsetPhaseNamesGetter *propertyOwner = m_propertyItem->FindPropertyParentWithInterface< ISpawnsetPhaseNamesGetter >( 0 );

	ASSERT( propertyOwner );
	if ( ! propertyOwner )
		return false;

	propertyOwner->GetSpawnsetPhaseNames( m_propertyItem->GetProperty(), outProperties );

	return true;
}
