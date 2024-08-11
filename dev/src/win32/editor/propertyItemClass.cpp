/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemClass.h"

CPropertyItemClass::CPropertyItemClass( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
}

CPropertyItemClass::~CPropertyItemClass()
{
}

Bool CPropertyItemClass::ReadProperty( CProperty *property, void* buffer, Int32 objectIndex /*= 0*/ )
{
    if ( objectIndex == -1 )
    {
        void *object = GetParentObject( -1 ).m_object;
        if ( !object )
        {
            return false;
        }
        property->Get( object, buffer );
        return true;
    }

	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	CPropertyDataBuffer propBuffer;
	void* object = NULL;
	if ( m_parent )
	{
		propBuffer.Reset( GetPropertyType() );
		Read( propBuffer.Data(), objectIndex );
		object = propBuffer.Data();
	}
	else
	{
		object = GetParentObject( objectIndex ).m_object;
	}
		
	property->Get( object, buffer );

	// Done
	return true;
}

STypedObject CPropertyItemClass::GetParentObject( Int32 objectIndex ) const
{
	if ( GetProperty() )
	{
		STypedObject parentObject = m_parent->GetParentObject( objectIndex );
		void* properyObject = GetProperty()->GetOffsetPtr( parentObject.m_object );
		return STypedObject( properyObject, m_propertyType );
	}
	else
	{
		return CPropertyItem::GetParentObject( objectIndex );
	}
}

Bool CPropertyItemClass::WriteProperty( CProperty *property, void* buffer, Int32 objectIndex /*= 0*/ )
{
	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	// Inform object	
	// Note that PropertyBuffer is declared as a field (not locally). It must outlive the method call 
	// because of all those notifications, events carrying data around, and the existence of property transactions
	// (which postpones sending those events). Ugh.

	STypedObject typedObj;

	if ( m_parent )
	{
		typedObj = STypedObject( nullptr, GetPropertyType() );
		m_writePropBuffer.Reset( typedObj.m_class );
		Read( m_writePropBuffer.Data(), objectIndex );
		typedObj.m_object = m_writePropBuffer.Data();
	}
	else
	{
		typedObj = GetParentObject( objectIndex );
	}

	m_page->PropertyPreChange( property, typedObj );
		
	// Write result to object
	property->Set( typedObj.m_object, buffer );

	m_page->PropertyPostChange( property, typedObj );

	if ( m_parent )
	{
		Write( m_writePropBuffer.Data(), objectIndex );		
	}

	return true;
}

Bool CPropertyItemClass::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	ASSERT( childItem->GetProperty() );

	return ReadProperty( childItem->GetProperty(), buffer, objectIndex );
}

Bool CPropertyItemClass::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	ASSERT( childItem->GetProperty() );

	return WriteProperty( childItem->GetProperty(), buffer, objectIndex );
}

void CPropertyItemClass::Expand()
{
	CClass* classObject = (CClass*)m_propertyType;

	if ( classObject )
	{
	    if ( classObject->IsA< CComponent >() )
	    {
		    CreateGroupItem( m_page, this, classObject );
	    }
	    else
	    {
		    //if ( m_dividePropertiesPerClass )
		    {
			    TDynArray< IProperty* > classProps;
			    classObject->GetProperties( classProps );

			    TDynArray< CClass* > classes;
			    for( Uint32 i=0; i<classProps.Size(); ++i )
			    {
				    if ( classProps[i]->IsEditable() )
				    {
					    classes.PushBackUnique( classProps[i]->GetParent() );
				    }
			    }

			    for( Uint32 i=0; i<classes.Size(); ++i )
			    {
				    CreateGroupItem( m_page, this, classes[i] );
			    }
		    }

		    /*
		    else
		    {
			    // Get struct properties
			    TDynArray< IProperty* > classProps;
			    classObject->GetProperties( classProps );

			    // Add struct sub properties
			    for ( Uint32 i=0; i<classProps.Size(); i++ )
			    {
				    CProperty* prop = classProps[i];
				    if ( prop->IsEditable() )
				    {
					    CPropertyItem* item = (CPropertyItem*)CreatePropertyItem( m_page, this, prop );
					    item->GrabPropertyValue();
				    }
			    }
		    }
		    */
	    }
	}
}
