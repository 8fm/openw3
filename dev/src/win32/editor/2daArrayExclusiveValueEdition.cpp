/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "2daArrayExclusiveValueEdition.h"
#include "../../common/core/gatheredResource.h"



namespace // anonymous
{
	struct ArrIdx : public wxClientData
	{
		Int32 idx;
		ArrIdx( Int32 _idx ) : idx( _idx ) {}
	};
} // anonymous


C2dArrayExclusiveValueEdition::C2dArrayExclusiveValueEdition( CPropertyItem* item, Bool exclusive, CGatheredResource& csvResource )
	: CListSelection( item )
	, m_csvResource( csvResource )
	, m_exclusive( exclusive )
	, m_ctrl( NULL )
	, m_descrColumnName( TXT( "tag" ) )
{
}

void C2dArrayExclusiveValueEdition::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxArrayString choices;
	m_ctrl = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize(), true, wxTE_PROCESS_ENTER );
	m_ctrl->Append( choices );
	m_ctrl->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrl->SetFocus();
	m_ctrl->Connect( wxEVT_COMMAND_COMBOBOX_SELECTED, wxCommandEventHandler( C2dArrayExclusiveValueEdition::OnChoiceChanged ), NULL, this );
	m_ctrl->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( C2dArrayExclusiveValueEdition::OnTextEntered ), NULL, this );

	// Fill in the editor with data

	S2daValueProperties properties;
	Bool doPropertiesExist = GetProperties( properties );
	ASSERT( doPropertiesExist );

	GrabValue( m_oldValue );
	RefreshValues( properties );
	m_ctrl->SetValue( wxString( m_oldValue.AsChar() ) );
	m_ctrl->SetStringSelection( wxString( m_oldValue.AsChar() ) );
}

void C2dArrayExclusiveValueEdition::CloseControls()
{
	if ( m_ctrl )
	{
		delete m_ctrl;
		m_ctrl = NULL;
	}
}

Bool C2dArrayExclusiveValueEdition::GrabValue( String& displayValue )
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
		HALT( "Type: %s not supported by 2d array value selection", m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
	}

	return true;
}

Bool C2dArrayExclusiveValueEdition::SaveValue()
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
		HALT( "Type: %s not supported by 2d array value selection", m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
	}

	return true;
}

void C2dArrayExclusiveValueEdition::OnTextEntered( wxCommandEvent &event )
{
	wxString newVal = m_ctrl->GetValue();
	newVal.Trim( true );
	newVal.Trim( false );

	S2daValueProperties properties;
	Bool doPropertiesExist = GetProperties( properties );
	ASSERT( doPropertiesExist );

	if ( newVal.Length() > 0 )
	{
		// check if an entry like that exists in the array
		C2dArray* arr = properties.m_array;
		Int32 rowIdx = FindElemRow( *properties.m_array, properties.m_descrColumnName, newVal.wc_str() ) ;
		if ( rowIdx < 0 )
		{
			AddEntry( properties, newVal.wc_str() );
		}

		SelectNewValue( properties, rowIdx, newVal.wc_str() );
	}
	else
	{
		ReleaseValue( properties );
	}
	RefreshValues( properties );
	m_ctrl->SetValue( newVal.wc_str() );
	m_ctrl->SetStringSelection( newVal.wc_str() );

	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void C2dArrayExclusiveValueEdition::OnChoiceChanged( wxCommandEvent &event )
{
	int selIdx = m_ctrl->GetSelection();
	wxString newVal = static_cast<wxItemContainerImmutable*>( m_ctrl )->GetStringSelection();

	// Only need to do anything more if we've actually changed the selection. If we don't check for this, we'll end up calling
	// m_ctrl->SetStringSelection(), which will just cause this function to be called again... which then calls m_ctrl->SetStringSelection()
	// and comes back here... fun!
	if ( String( newVal.wc_str() ) != m_oldValue )
	{
		S2daValueProperties properties;
		Bool doPropertiesExist = GetProperties( properties );
		ASSERT( doPropertiesExist );

		// update option availability status
		// m_oldValue is updated by SelectNewValue() and ReleaseValue()...
		if ( newVal.Length() > 0 )
		{
			ArrIdx* data = dynamic_cast< ArrIdx* >( m_ctrl->GetClientObject( selIdx ) );
			SelectNewValue( properties, data->idx, newVal.wc_str() );
		}
		else
		{
			ReleaseValue( properties );
		}
		RefreshValues( properties );

		m_ctrl->SetValue( newVal.wc_str() );
		m_ctrl->SetStringSelection( newVal.wc_str() );
	}

	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void C2dArrayExclusiveValueEdition::AddEntry( S2daValueProperties& properties, const String& newVal ) const
{
	C2dArray* arr = properties.m_array;

	Uint32 rowIdx = arr->GetNumberOfRows();
	arr->InsertRow( rowIdx );

	Uint32 descrColIdx = arr->GetColumnIndex( properties.m_descrColumnName );
	ASSERT( descrColIdx >= 0 );

	Int32 availabilityColIdx = arr->GetColumnIndex( properties.m_valueColumnName );
	arr->SetValue( newVal, descrColIdx, rowIdx );
	if ( availabilityColIdx >= 0 )
	{
		arr->SetValue( TXT( "1" ), availabilityColIdx, rowIdx );
	}

	arr->MarkModified();
}


void C2dArrayExclusiveValueEdition::ReleaseValue( S2daValueProperties& properties )
{
	C2dArray* arr = properties.m_array;
	Int32 availabilityColIdx = arr->GetColumnIndex( properties.m_valueColumnName );

	if ( availabilityColIdx >= 0 )
	{
		Int32 oldValueRowIdx = FindElemRow( *arr, properties.m_descrColumnName, m_oldValue );
		arr->SetValue( TXT( "0" ), availabilityColIdx, oldValueRowIdx );
		arr->MarkModified();
	}

	m_oldValue = TXT( "" );
}

void C2dArrayExclusiveValueEdition::SelectNewValue( S2daValueProperties& properties,
												   Uint32 newValueRowIdx,
												   const String& newVal )
{
	C2dArray* arr = properties.m_array;
	Int32 availabilityColIdx = arr->GetColumnIndex( properties.m_valueColumnName );

	if ( availabilityColIdx >= 0 )
	{
		Int32 oldValueRowIdx = FindElemRow( *arr, properties.m_descrColumnName, m_oldValue );
		arr->SetValue( TXT( "0" ), availabilityColIdx, oldValueRowIdx );
		arr->SetValue( TXT( "1" ), availabilityColIdx, newValueRowIdx );
		arr->MarkModified();
	}

	m_oldValue = newVal;
}

Int32 C2dArrayExclusiveValueEdition::FindElemRow( C2dArray& arr, const String& colName, const String& val ) const
{
	Uint32 colIdx = arr.GetColumnIndex( colName );

	Uint32 colsCount, rowsCount;
	arr.GetSize( colsCount, rowsCount );

	Uint32 rowIdx = 0;
	for ( rowIdx = 0; rowIdx < rowsCount; ++rowIdx )
	{
		if ( arr.GetValue( colIdx, rowIdx ) == val )
		{
			break;
		}
	}

	if ( rowIdx >= rowsCount )
	{
		rowIdx = -1;
	}

	return rowIdx;
}

void C2dArrayExclusiveValueEdition::RefreshValues( S2daValueProperties& properties )
{
	static_cast<wxItemContainer*>( m_ctrl )->Clear();

	C2dArray* arr = properties.m_array;

	Uint32 colNum, rowNum;
	arr->GetSize( colNum, rowNum );

	Int32 colName  = arr->GetColumnIndex( properties.m_descrColumnName );
	ASSERT( colName >= 0 );

	Int32 colValue  = arr->GetColumnIndex( properties.m_valueColumnName );
	Bool availabilityColSpecified = colValue >= 0;

	// add a null entry so that we have a possibility of clearing the value
	m_ctrl->Append( wxT( "" ) , new ArrIdx( -1 ) );

	// add entires from the array
	String entryAvailableID( TXT( "0" ) );
	for ( Uint32 j = 0; j < rowNum; ++j )
	{
		if ( !availabilityColSpecified || ( arr->GetValue( colValue, j ) == entryAvailableID ) )
		{
			String val = arr->GetValue( colName, j );
			m_ctrl->Append( val.AsChar(), new ArrIdx( j ) );
		}
	}
}

Bool C2dArrayExclusiveValueEdition::GetProperties( S2daValueProperties& properties )
{
	properties.m_array = m_csvResource.LoadAndGet< C2dArray >();
	if ( !properties.m_array || !properties.m_array->GetFile() )
	{
		ERR_EDITOR( TXT( "Can't load '%s' file - invalid resource" ), m_csvResource.GetPath().ToString() );
	}
	properties.m_descrColumnName = m_descrColumnName;
	if ( m_exclusive )
	{
		properties.m_valueColumnName = TXT( "isInUse" );	
	}

	return true;
}

void C2dArrayExclusiveValueEdition::SetDescriptionColumnName( const String& columnName )
{
	m_descrColumnName = columnName;
}
