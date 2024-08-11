/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicPropGroupItem.h"

CDynamicPropGroupItem::CDynamicPropGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, IDynamicPropertiesSupplier* supplier )
	: CBaseGroupItem( page, parent )
	, m_supplier( supplier )
{
	// Autoexpand
	m_isExpandable = true;
	Expand();

	// Register event listener
	SEvents::GetInstance().RegisterListener( CName( TXT("UpdateDynamicProperties") ), this );
}

CDynamicPropGroupItem::~CDynamicPropGroupItem()
{
	SEvents::GetInstance().UnregisterListener( this );
}

String CDynamicPropGroupItem::GetCaption() const
{
	return TXT("Dynamic Properties");
}

void CDynamicPropGroupItem::Collapse()
{
	// Pass to base class
	CBaseGroupItem::Collapse();

	// Destroy fake properties 
	m_fakeProperties.ClearPtr();
}

void CDynamicPropGroupItem::Expand()
{
	// Get dynamic properties names
	TDynArray< CName > dynamicProperties;
	m_supplier->GetDynamicProperties( dynamicProperties );

	// Build properties
	for ( Uint32 i=0; i<dynamicProperties.Size(); i++ )
	{
		CName propName = dynamicProperties[i];

		// Read value
		CVariant propValue;
		if ( m_supplier->ReadDynamicProperty( propName, propValue ) )
		{
			if ( propValue.IsValid() )
			{
				// Detect inlined types
				Bool isInlined = false;
				if ( propValue.GetRTTIType()->GetType() == RT_Pointer || propValue.GetRTTIType()->GetType() == RT_Handle )
				{
					CClass* pointedClass;

					if ( propValue.GetRTTIType()->GetType() == RT_Handle )
					{
						CRTTIHandleType* handleType = ( CRTTIHandleType* ) propValue.GetRTTIType();
						pointedClass = static_cast< CClass* >( handleType->GetPointedType() );
					}
					else
					{
						CRTTIPointerType* pointerType = ( CRTTIPointerType* ) propValue.GetRTTIType();
						pointedClass = static_cast< CClass* >( pointerType->GetPointedType() );
					}

					// Do not show nodes...
					if ( pointedClass->IsBasedOn( ClassID< CNode >() ) )
					{
						continue;
					}

					// We can inline non resource pointers ( not very safe :P )
					if ( !pointedClass->IsBasedOn( ClassID< CResource >() ) )
					{
						isInlined = true;
					}
				}

				// Create fake property of matching type and name
				CProperty* fakeProperty = new CProperty( propValue.GetRTTIType(), NULL, 0, propName, String::EMPTY, PF_Editable | ( isInlined ? PF_Inlined : 0 ) );
				if ( fakeProperty )
				{
					// Remember the fake property
					m_fakeProperties.PushBack( fakeProperty );

					// Create expandable property
					if ( CPropertyItem* item = CreatePropertyItem( m_page, this, fakeProperty ) )
					{
						item->GrabPropertyValue();
					}
				}
			}
		}
	}

	CBasePropItem::Expand();
}

Bool CDynamicPropGroupItem::ReadProperty( CProperty *property, void* buffer, Int32 objectIndex /*=0*/ )
{
    if ( objectIndex == -1 )
    {
        return false;
    }

	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	// Read property
	CVariant propValue;
	if ( !m_supplier->ReadDynamicProperty( property->GetName(), propValue ) )
	{
		return false;
	}

	// Extract value
	ASSERT( propValue.GetRTTIType() );
	propValue.GetRTTIType()->Copy( buffer, propValue.GetData() );
	return true;
}

Bool CDynamicPropGroupItem::WriteProperty( CProperty *property, void* buffer, Int32 objectIndex /*=0*/ )
{
	// Check for invalid object index
	if ( objectIndex < 0 || (Int32)objectIndex >= GetNumObjects() )
	{
		return false;
	}

	STypedObject object = GetParentObject( objectIndex );
	
	m_page->PropertyPreChange( property, object );

	// Build variant
	CVariant propValue( property->GetType()->GetName(), buffer );
	Bool propertyWritten = m_supplier->WriteDynamicProperty( property->GetName(), propValue );

	m_page->PropertyPostChange( property, object );

	return propertyWritten;
}

Bool CDynamicPropGroupItem::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	ASSERT( objectIndex == 0 || objectIndex == -1 );
	ASSERT( childItem->GetProperty() );

	return ReadProperty( childItem->GetProperty(), buffer, objectIndex );
}

Bool CDynamicPropGroupItem::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*=0*/ )
{
	ASSERT( objectIndex == 0 );
	ASSERT( childItem->GetProperty() );

	return WriteProperty( childItem->GetProperty(), buffer, objectIndex );
}

void CDynamicPropGroupItem::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == TXT("UpdateDynamicProperties") )
	{
		IDynamicPropertiesSupplier* supp = dynamic_cast< IDynamicPropertiesSupplier* >( GetEventData< CObject* >( data ) );
		if ( supp == m_supplier )
		{
			Collapse();
			Expand();
		}
	}
}
