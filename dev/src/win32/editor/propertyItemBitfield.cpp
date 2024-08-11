/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemBitfield.h"

CPropertyItemBitField::CPropertyItemBitField( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
	m_isExpandable = true;
}

CPropertyItemBitField::~CPropertyItemBitField()
{
	m_dynamicProperties.ClearPtr();
}

void CPropertyItemBitField::Expand()
{
	if ( m_propertyType )
	{
		// Add sub properties
		CBitField* bitfield = static_cast< CBitField* >( m_propertyType );
		for ( Uint32 i=0; i<32; i++ )
		{
			CName bitName = bitfield->GetBitName( i );
			if ( bitName )
			{
				// Bit filed bits uses boolean type for displaying properties
				IRTTIType* boolType = SRTTI::GetInstance().FindFundamentalType( ::GetTypeName< Bool >() );
				CProperty* prop = new CProperty( boolType, NULL, i, bitName, TXT("A bit of a bitfield"), PF_Editable );

				// Create and update property
				if ( CPropertyItem* item = CreatePropertyItem( m_page, this, prop ) )
				{
					item->GrabPropertyValue();
				}
			}
		}
	}

	// Redraw
	CPropertyItem::Expand();
}

void CPropertyItemBitField::Collapse()
{
	// Redraw
	CPropertyItem::Collapse();

	// Delete all shit
	m_dynamicProperties.ClearPtr();
}

Bool CPropertyItemBitField::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	// Get the bit index
	Int32 index = childItem->GetProperty()->GetDataOffset();
	ASSERT( index >= 0 );

	// Get the bit field
	CBitField* bitfield = static_cast< CBitField* >( m_propertyType );
	ASSERT( bitfield->GetBitName( index ) );

	// Load data
	Uint32 data = 0;
	Read( &data, objectIndex );

	// Check bit
	Bool bitIsSet = 0 != ( data & ( 1 << index ) );
	*( Bool* ) buffer = bitIsSet;

	// Done
	return true;
}

Bool CPropertyItemBitField::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	// Get the bit index
	Int32 index = childItem->GetProperty()->GetDataOffset();
	ASSERT( index >= 0 );

	// Get the bit field
	CBitField* bitfield = static_cast< CBitField* >( m_propertyType );
	ASSERT( bitfield->GetBitName( index ) );

	// Load data
	Uint32 data = 0;
	Read( &data, objectIndex );

	// Check bit
	if ( *( Bool* ) buffer )
	{
		data |= 1 << index;
	}
	else
	{
		data &= ~(1 << index);
	}

	// Write back
	Write( &data, objectIndex );

	// Update
	GrabPropertyValue();
	return true;
}

Bool CPropertyItemBitField::SerializeXML( IXMLFile& file )
{
	return false;
}
