/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "2daArrayValueSelection.h"

C2dArrayValueSelection::C2dArrayValueSelection( CPropertyItem* item )
	: CListSelection( item )
	, m_ctrl( NULL )
{
}

void C2dArrayValueSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	I2dArrayPropertyOwner *propertyOwner = NULL;
	CBasePropItem         *pitem = m_propertyItem;
	while ( propertyOwner == NULL && pitem != NULL )
	{
		propertyOwner = dynamic_cast< I2dArrayPropertyOwner *>( pitem->GetParentObject( 0 ).AsSerializable() );
		pitem = pitem->GetParent();
	}

	ASSERT( propertyOwner );
	if ( ! propertyOwner )
		return;

	SConst2daValueProperties properties;
	propertyOwner->Get2dArrayPropertyAdditionalProperties( m_propertyItem->GetProperty(), properties );

	const C2dArray* arr = properties.m_array;
	if (arr)
	{
		m_choices.Clear();
		m_choices.Add( wxEmptyString );

		Uint32 colNum, rowNum;
		arr->GetSize(colNum,rowNum);
		TDynArray< String > uniqueChoices;

		Int32 colValue = arr->GetColumnIndex( properties.m_valueColumnName );
		Int32 colName  = arr->GetColumnIndex( properties.m_descrColumnName );
		if ( colName < 0 )
			colName = colValue;
		if ( colValue >= 0 )
		{
			for ( Uint32 j = 0; j < rowNum; j++ )
			{
				if ( DoesFilterMatch( arr, properties.m_filters, j, colNum ) )
				{
					m_values.PushBackUnique( arr->GetValue( colValue, j ) );
					uniqueChoices.PushBackUnique( arr->GetValue( colName, j ) );
				}
			}
		}
		for ( Uint32 i = 0; i < uniqueChoices.Size(); i++ )
		{
			m_choices.Add( uniqueChoices[ i ].AsChar() );
		}

		// Create editor
		m_ctrl = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
		m_ctrl->Append( m_choices );
		m_ctrl->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
		m_ctrl->SetFocus();	

		// Find current value on list and select it
		String str;
		GrabValue( str );
		int index = -1;
		for ( Uint32 i = 0; i < m_values.Size(); ++i )
		{
			if ( m_values[i] == str )
			{
				index = i;
				break;
			}
		}

		if ( index >= 0 )
		{
			m_ctrl->SetSelection( index+1 );
		}
		else
		{
			m_ctrl->SetSelection( 0 );
		}

		// Notify of selection changes
		m_ctrl->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( C2dArrayValueSelection::OnChoiceChanged ), NULL, this );
	}
}

void C2dArrayValueSelection::CloseControls()
{
	// Close combo box
	if ( m_ctrl )
	{
		delete m_ctrl;
		m_ctrl = NULL;
	}
}

Bool C2dArrayValueSelection::GrabValue( String& displayValue )
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
		HALT( "Type: %s not supported by 2d array value selection" , m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
	}

	return true;
}

Bool C2dArrayValueSelection::SaveValue()
{
	int sel = m_ctrl->GetSelection();
	String value = sel > 0 ? m_values[ sel-1 ] : String::EMPTY;

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

void C2dArrayValueSelection::OnChoiceChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

Bool C2dArrayValueSelection::DoesFilterMatch( const C2dArray *array, const TDynArray< S2daValueFilter > &filters, Uint32 row, Uint32 colSize )
{
	// check every array column
	for ( Uint32 i = 0; i < colSize; i++ )
	{
		// find filter associated with current column
		for ( TDynArray< S2daValueFilter >::const_iterator filter = filters.Begin();
			  filter != filters.End();
			  ++filter )
		{
			if ( array->GetHeader(i) == filter->m_columnName )
			{
				Bool filterMatch = false;
				for ( TDynArray< String >::const_iterator keyword = filter->m_keywords.Begin();
					  keyword != filter->m_keywords.End();
					  ++keyword )
				{
					if ( *keyword == array->GetValue( i, row ) )
					{
						filterMatch = true;
						break; // we need only one match
					}
				}
				if ( !filterMatch )
				{
					return false;
				}
			}
		}
	}

	return true;
}
