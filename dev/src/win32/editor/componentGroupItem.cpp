/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CComponentGroupItem::CComponentGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, CClass *componentClass )
	: CBaseGroupItem( page, parent )
	, m_componentClass( componentClass )
{
	m_isExpandable = true;
}

void CComponentGroupItem::SetObjects( const TDynArray< CComponent* > &objects )
{
	// Collect valid objects and determine common base class
	for ( Uint32 i=0; i<objects.Size(); i++ )
	{
		CComponent* obj = objects[i];
		// Get common class
		if ( !m_componentClass )
		{
			m_componentClass = obj->GetClass();
		}
		else
		{
			while ( !obj->GetClass()->IsBasedOn( m_componentClass ) )
			{
				m_componentClass = m_componentClass->GetBaseClass();
			}
		}

		// Add object to list
		m_components.PushBack( obj );
	}

	// Expand
	if ( m_components.Size() )
	{
		//Init( m_componentClass );

		m_isExpandable = true;

		if ( m_children.Empty() )
		{
			Expand();
		}
	}
}

String CComponentGroupItem::GetCaption() const
{
	return m_componentClass->GetName().AsString();
}

Bool CComponentGroupItem::ReadProperty( CProperty *property, void* buffer, Int32 objectIndex /*= 0*/ )
{
    if ( objectIndex == -1 )
    {
        void *object = m_componentClass->GetDefaultObjectImp();
        const void* readOffset = property->GetOffsetPtr( object );
        property->GetType()->Copy( buffer, readOffset );

        return true;
    }

	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	CPropertyDataBuffer propBuffer;
	CObject* object = m_components[objectIndex];

	const void* readOffset = property->GetOffsetPtr( object );
	property->GetType()->Copy( buffer, readOffset );

	// Done
	return true;
}

Bool CComponentGroupItem::WriteProperty( CProperty *property, void* buffer, Int32 objectIndex /*= 0*/ )
{
	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	// Inform object	
	CPropertyDataBuffer propBuffer;
	CObject* object = m_components[objectIndex];

	m_page->PropertyPreChange( property, STypedObject( object ) );

	// Write result to object
	void* writeOffset = property->GetOffsetPtr( object );
	property->GetType()->Copy( writeOffset, buffer );

	// Inform object & properties browser
	m_page->PropertyPostChange( property, STypedObject( object ) );

	return true;
}

Bool CComponentGroupItem::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	//ASSERT( objectIndex == 0 || objectIndex == -1 );
	ASSERT( childItem->GetProperty() );

	return ReadProperty( childItem->GetProperty(), buffer, objectIndex );
}


Bool CComponentGroupItem::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	//ASSERT( objectIndex == 0 );

	if ( m_components[objectIndex]->IsAttached() )
	{
		CPhysicsWorld* physicsWorld = nullptr;
		ASSERT( m_components[objectIndex]->GetLayer() && m_components[objectIndex]->GetLayer()->GetWorld( ) && m_components[objectIndex]->GetLayer()->GetWorld()->GetPhysicsWorld( physicsWorld ) );
	}

	ASSERT( childItem->GetProperty() );
	WriteProperty( childItem->GetProperty(), buffer, objectIndex );

	return true;
}

void CComponentGroupItem::Expand()
{
	TDynArray< IProperty* > properties;
	m_componentClass->GetProperties( properties );

	// Generate list of classes
	TDynArray< CClass* > groups;
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		IProperty *prop = properties[i];
		if ( prop->IsEditable() )
		{
			groups.PushBackUnique( (CClass*) prop->GetParent() );
		}
	}

	// Create group properties
	for ( Uint32 i=0; i<groups.Size(); i++ )
	{
		new CClassGroupItem( m_page, this, groups[i] );
	}
}

STypedObject CComponentGroupItem::GetParentObject( Int32 objectIndex ) const
{
	return STypedObject( m_components[objectIndex] );
}

