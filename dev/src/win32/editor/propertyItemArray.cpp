/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemArray.h"
#include "../../common/core/xmlFile.h"

CPropertyItemArray::CPropertyItemArray( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
}

void CPropertyItemArray::Expand()
{
	// Expand only if array is determined
	if ( m_isDetermined )
	{
		IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );

		// Get array size
		CPropertyDataBuffer buffer( m_propertyType );
		Read( buffer.Data() );
		const Uint32 arraySize = arrayType->ArrayGetArraySize( buffer.Data() );

		// Add sub properties
		for ( Uint32 i=0; i<arraySize; i++ )
		{
			IRTTIType* arrayInnerType = arrayType->ArrayGetInnerType();
			if ( CPropertyItem* item = CreatePropertyItem( m_page, this, arrayInnerType, i ) )
			{
				item->GrabPropertyValue();
			}
		}
	}
}

void CPropertyItemArray::CreateMainControl()
{
	// Add buttons to array property
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );
	if ( arrayType->ArrayIsResizable() )
	{
		// Add buttons	
		AddButton( m_page->GetStyle().m_iconClear, wxCommandEventHandler( CPropertyItemArray::OnArrayClear ) );
		AddButton( m_page->GetStyle().m_iconAdd, wxCommandEventHandler( CPropertyItemArray::OnArrayAdd ) );
	}

	wxCommandEvent event( wxEVT_COMMAND_PROPERTY_SELECTED );
	event.SetClientData( GetProperty() );
	wxPostEvent( GetPage(), event );
}

void CPropertyItemArray::OnArrayClear( wxCommandEvent& event )
{
	// Hide array
	m_page->Freeze();
	Collapse();
	
	for( Int32 i=0; i<GetNumObjects(); ++i )
	{
		CPropertyDataBuffer buffer( m_propertyType );
		Write( buffer.Data(), i );
	}

	// Update
	m_page->Thaw();
	m_page->Refresh();

	// Grab info
	GrabPropertyValue();
}

void CPropertyItemArray::OnArrayAdd( wxCommandEvent& event )
{
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );
	ASSERT( arrayType->ArrayIsResizable() );

	// Hide current content
	m_page->Freeze();
	Collapse();

	// Add element to each array
	for( Int32 i=0; i<GetNumObjects(); ++i )
	{
		CPropertyDataBuffer buffer( m_propertyType );
		Read( buffer.Data(), i );

		if ( -1 != arrayType->ArrayAddElement( buffer.Data() ) )
		{
			Write( buffer.Data(), i );
		}
	}

	// Expand array
	Expand();

	// Grab info
	GrabPropertyValue();

	m_page->Thaw();
	m_page->Refresh();
}

void CPropertyItemArray::OnArrayDeleteItem( Uint32 arrayIndex )
{
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );
	ASSERT( arrayType->ArrayIsResizable() );

	// Expand parent to show new layout
	Uint32 deletedArrayIndex = arrayIndex;
	CEdPropertiesPage* page = m_page;
	CPropertyItem* parent = (CPropertyItem* )m_parent;
	page->Freeze();
	page->SelectItem( parent );

	// remove element
	for( Int32 i=0; i<GetNumObjects(); ++i )
	{
		CPropertyDataBuffer buffer( arrayType );
		Read( buffer.Data(), i );

		// MemTODO - check if we're clearing pointer array and delete objects if necessary

		// Remove array element
		arrayType->ArrayDeleteElement( buffer.Data(), arrayIndex );

		// Write it back
		Write( buffer.Data(), i );
	}


	Collapse();
	Expand();

	// Grab info
	GrabPropertyValue();

	page->Thaw();
	page->Refresh();
	page->UpdateLayout();

	// Refocus on array item
	if ( GetChildren().Size() )
	{
		if ( deletedArrayIndex >= GetChildren().Size() )
		{
			deletedArrayIndex = GetChildren().Size() - 1;
		}

		page->SelectItem( GetChildren()[ deletedArrayIndex ] );
	}
}

void CPropertyItemArray::OnArrayInsertItem( Uint32 arrayIndex )
{
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );
	ASSERT( arrayType->ArrayIsResizable() );

	// Read array to buffer
	for( Int32 i=0; i<GetNumObjects(); ++i )
	{
		CPropertyDataBuffer buffer( arrayType );
		Read( buffer.Data(), i );

		// Insert array element
		arrayType->ArrayInsertElement( buffer.Data(), arrayIndex );

		// Write it back
		Write( buffer.Data(), i );
	}

	// Expand parent to show new layout
	Uint32 insertedArrayIndex = arrayIndex;
	CEdPropertiesPage* page = m_page;
	CPropertyItem* parent = (CPropertyItem* )m_parent;
	page->Freeze();
	page->SelectItem( parent );
	Collapse(); 
	Expand();

	// Grab info
	GrabPropertyValue();

	page->Thaw();
	page->Refresh();
	page->UpdateLayout();

	// Refocus on array item
	if ( GetChildren().Size() )
	{
		page->SelectItem( GetChildren()[ insertedArrayIndex ] );
	}
}

Bool CPropertyItemArray::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );

	const Int32 itemIndex = childItem->GetArrayIndex();
	ASSERT( itemIndex >= 0 );

	CPropertyDataBuffer propBuffer( arrayType );
	Read( propBuffer.Data(), objectIndex );

	// narrow case: default array does not have enough elements, create fake element
	if ( objectIndex == -1 )
	{
		if ( arrayType->ArrayIsResizable() )
		{
			if ( itemIndex >= (Int32)arrayType->ArrayGetArraySize( propBuffer.Data() ) )
			{
				const Int32 addedItemIndex = arrayType->ArrayAddElement( propBuffer.Data() );
				arrayType->ArrayGetInnerType()->Copy( buffer, arrayType->ArrayGetArrayElement( propBuffer.Data(), addedItemIndex ) );
				return true;
			}
		}
	}

	// access the element data directly
	const Int32 arraySize = arrayType->ArrayGetArraySize( propBuffer.Data() );
	ASSERT( itemIndex < arraySize );
	if ( itemIndex < arraySize  )
	{
		arrayType->ArrayGetInnerType()->Copy( buffer, arrayType->ArrayGetArrayElement( propBuffer.Data(), itemIndex ) );
		return true;
	}

	// index out of range
	return false;
}

Bool CPropertyItemArray::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );

	const Int32 itemIndex = childItem->GetArrayIndex();
	ASSERT( itemIndex >= 0 );

	CPropertyDataBuffer propBuffer( arrayType );
	Read( propBuffer.Data(), objectIndex );

	// Insert array element
	const Int32 arraySize = arrayType->ArrayGetArraySize( propBuffer.Data() );
	ASSERT( itemIndex < arraySize );
	if ( itemIndex < arraySize )
	{
		void* subObjectData = arrayType->ArrayGetArrayElement( propBuffer.Data(), itemIndex );
		arrayType->ArrayGetInnerType()->Copy( subObjectData, buffer );
	}

	// Write it back
	return Write( propBuffer.Data(), objectIndex );
}

Bool CPropertyItemArray::SerializeXML( IXMLFile& file )
{
	if ( file.IsWriter() )
	{
		return CPropertyItem::SerializeXML( file );
	}
	else
	{
		IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );

		// Remove all elements
		Collapse();
		for( Int32 i = 0; i < GetNumObjects(); ++i )
		{
			// write empty arrays
			CPropertyDataBuffer buffer( arrayType );
			Write( buffer.Data(), i );
		}

		// Get number of array elements from xml
		file.BeginNode( TXT("property") );
		const Uint32 childCount = file.GetChildCount();

		// Fill the array with new elements
		if ( childCount > 0 )
		{
			for( Int32 i = 0; i < GetNumObjects(); ++i )
			{
				CPropertyDataBuffer buffer( arrayType );
				Read( buffer.Data(), i );
				if ( arrayType->ArrayAddElement( buffer.Data(), childCount ) != -1 )
				{
					Write( buffer.Data(), i );
				}
			}
		}

		// Create propertyItems for each array element
		Expand();

		// and serialize them
		const TDynArray< CBasePropItem * > &children = GetChildren();
		ASSERT( children.Size() == childCount );
		for ( Uint32 i = 0; i < children.Size(); ++i )
		{
			if ( !children[i]->SerializeXML( file ) )
			{
				return false;
			}
		}

		file.EndNode();
		return true;
	}
}

void* CPropertyItemArray::GetArrayElement( Uint32 arrayIndex, Int32 objectIndex )
{
	IRTTIBaseArrayType* arrayType = static_cast< IRTTIBaseArrayType* >( m_propertyType );
	void* arrayData = GetProperty()->GetOffsetPtr( GetParentObject( objectIndex ).m_object );
	return arrayType->ArrayGetArrayElement( arrayData, arrayIndex );
}
