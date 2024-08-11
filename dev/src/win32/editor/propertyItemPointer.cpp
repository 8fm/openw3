/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "propertyItemPointer.h"
#include "itemSelectionDialogs\classSelectorDialog.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/engine/curve.h"
#include "../../common/core/xmlFile.h"

CPropertyItemPointer::CPropertyItemPointer( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
{
}

void CPropertyItemPointer::Init( CProperty *prop, Int32 arrayIndex /*= -1*/ )
{
	CPropertyItem::Init( prop, arrayIndex );

	// Inlined object properties are expandable
	if ( IsInlined() )
	{
		m_isExpandable = true;
	}
}

void CPropertyItemPointer::Init( IRTTIType *type, Int32 arrayIndex /*= -1*/ )
{
	CPropertyItem::Init( type, arrayIndex );

	// Inlined object properties are expandable
	if ( IsInlined() )
	{
		m_isExpandable = true;
	}
}

STypedObject CPropertyItemPointer::GetParentObject( Int32 objectIndex ) const
{
    void* returnObject = nullptr;
	IRTTIType* pointedType = nullptr;

	ReadObj( &returnObject, objectIndex );

	if ( m_propertyType->GetType() == RT_Pointer )
	{
		CRTTIPointerType* pointerType = static_cast< CRTTIPointerType* >( m_propertyType );
		pointedType = pointerType->GetPointedType();
	}
	else if ( m_propertyType->GetType() == RT_Handle )
	{
		CRTTIHandleType* pointerType = static_cast< CRTTIHandleType* >( m_propertyType );
		pointedType = pointerType->GetPointedType();
	}

    return STypedObject( returnObject, pointedType );
}

void CPropertyItemPointer::Expand()
{
	if ( IsInlined() )
	{
		// Expand only if array is determined
		if ( m_isDetermined )
		{
			// Get pointed class
			CRTTIPointerType* pointerType = ( CRTTIPointerType* )m_propertyType;
			CClass* pointedClass = static_cast< CClass* >( pointerType->GetPointedType() );

			// Special case for curves...
			{
				if ( pointedClass->GetName() == CNAME( CCurve ) || pointedClass->GetName() == CNAME( SSimpleCurve ) )
				{
					CPropertyItemSimpleCurve *item = new CPropertyItemSimpleCurve( m_page, this, GetProperty() );

					// We add the curve item, and continue to add other properties below. This makes pointer curves
					// behave a bit different from non-pointers, as non-pointers do not show sub-properties. This should
					// be okay, as curve pointers are likely created dynamically in scripts, and so have no way to
					// specify default things like looping settings. So, these can show up as editable properties.
				}
			}

			// We need to get the base class for the inlined object
			CClass* baseInlineClass = NULL;

			// Get the property base class
			if ( pointedClass->IsSerializable() )
			{
				// Determine base class used in inline objects
				// For ISerializable this is easy
				const Uint32 numObjects = GetNumObjects();
				for ( Uint32 i=0; i<numObjects; i++ )
				{
					// Get object
					ISerializable *obj = NULL;
					ReadObj( &obj );
					if ( NULL != obj )
					{
						// Get common class
						if ( !baseInlineClass )
						{
							baseInlineClass = obj->GetClass();
						}
						else
						{
							while ( !obj->GetClass()->IsBasedOn( baseInlineClass ) )
							{
								baseInlineClass = baseInlineClass->GetBaseClass();
							}
						}
					}
				}
			}
			else
			{
				// A direct pointer to the structure, use the structure class
				baseInlineClass = pointedClass;
			}

			// Expand
			if ( NULL != baseInlineClass )
			{
				TDynArray< IProperty* > classProps;
				baseInlineClass->GetProperties( classProps );

				TDynArray< CClass* > classes;
				for ( Uint32 i=0; i<classProps.Size(); ++i )
				{
					if ( classProps[i]->IsEditable() )
					{
						classes.PushBackUnique( classProps[i]->GetParent() );
					}
				}

				for ( Uint32 i=0; i<classes.Size(); ++i )
				{
					CreateGroupItem( m_page, this, classes[i] );
				}
			}
		}
	}
}

void CPropertyItemPointer::CreateMainControl()
{	
	// Get object property and determine object class from that
	CClass* pointedClass = GetParentObject( 0 ).m_class;

	// General inlined solution - works for all classes
	if ( IsInlined() )
	{
		// Display the "delete" icon only if we have some not null pointer
		Bool bHasValidPointer = false;
		{
			const Uint32 numObjects = GetNumObjects();
			for ( Uint32 i=0; i<numObjects; i++ )
			{
				void* obj = NULL;
				ReadObj( &obj );
				if ( NULL != obj )
				{
					bHasValidPointer = true;
					break;
				}
			}
		}

		if ( bHasValidPointer )
		{
			AddButton( m_page->GetStyle().m_iconDelete, wxCommandEventHandler( CPropertyItemPointer::OnObjectInlineDelete ) );
		}

		AddButton( m_page->GetStyle().m_iconNew, wxCommandEventHandler( CPropertyItemPointer::OnObjectInlineNew ) );
	}

	// Special CObject case for pointers (picking)
	else if ( pointedClass->IsObject() )
	{
		AddButton( m_page->GetStyle().m_iconClear, wxCommandEventHandler( CPropertyItemPointer::OnObjectClear ) );

		if ( ShouldLinkIconBeDisplayed() )
		{
			AddButton( m_page->GetStyle().m_iconUse, wxCommandEventHandler( CPropertyItemPointer::OnObjectUse ) );
		}

		AddButton( m_page->GetStyle().m_iconBrowse, wxCommandEventHandler( CPropertyItemPointer::OnObjectBrowser ) );
	}

	// use parent's method to create textControl
	CPropertyItem::CreateMainControl();
}

void CPropertyItemPointer::OnObjectClear( wxCommandEvent& event )
{
	m_displayValue = TXT("NULL");
	SavePropertyValue( false );
	GrabPropertyValue();
}

void CPropertyItemPointer::OnObjectUse( wxCommandEvent& event )
{
	// Get object property and determine object class from that
	CClass* pointedClass = GetParentObject( 0 ).m_class;

	// Is this a resource class
	if ( pointedClass->IsA<CResource>() )
	{
		String selectedResource;
		if ( GetActiveResource( selectedResource ) )
		{
			m_displayValue = selectedResource;
			SavePropertyValue( false );
			GrabPropertyValue();
		}
	}
	// It's an entity, get it from active selection
	else if ( pointedClass->IsA<CEntity>() )
	{
		CEntity* selectedEntity = GetSelectedEntity();
		if ( selectedEntity && selectedEntity->IsA( pointedClass ) )
		{
			// Entity should be in the same layer as each edited object
			Bool sameLayer = true;
			for ( Int32 i=0; i<GetNumObjects(); i++ )
			{
				CObject *root = GetRootObject( i ).AsObject();
				if ( !root )
				{
					continue;
				}

				if ( selectedEntity->GetRoot() != root->GetRoot() )
				{
					// Fuckup
					sameLayer = false;
					break;
				}
			}

			// Valid selected entity, set it
			if ( sameLayer )
			{
				// For each object, write the property value
				const INT numObjects = GetNumObjects();
				for ( INT i=0; i<numObjects; i++ )
				{
					WriteObj( &selectedEntity, i );
				}

				// Get name
				GrabPropertyValue();
			}
		}
	}
}

void CPropertyItemPointer::OnObjectBrowser( wxCommandEvent& event )
{
	// We can browse only determined object
	if ( m_isDetermined )
	{
		// Get object property and determine object class from that
		CClass* pointedClass = GetParentObject( 0 ).m_class;
		if ( pointedClass->IsA< CResource >() )
		{
			CResource *theResource = NULL;
			if ( ReadObj( &theResource ) )
			{
				// We can browse only resources
				if ( theResource && theResource->GetFile() )
				{
					String resourcePath = theResource->GetFile()->GetDepotPath();
					SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( resourcePath ) );
				}
			}
		}
	}
}


void CPropertyItemPointer::OnObjectInlineDelete( wxCommandEvent& event )
{
	// Cleanup old objects
	// TODO: for general ISerializable objects this is causing memory leaks
	const Uint32 numObjects = GetNumObjects();
	for ( Uint32 i=0; i<numObjects; i++ )
	{
		// Write NULL to objects
		void* nullPtr = NULL;
		WriteObj( &nullPtr, i );
	}

	// Collapse
	Collapse();
}

void CPropertyItemPointer::OnObjectInlineNewInternal( CClass* objectClass )
{
	ASSERT( objectClass );
	ASSERT( !objectClass->IsAbstract() );

	// NOTE: [beef] Old objects are not deleted, let them be kept alive inside UNDO steps

	// Create new objects
	const Int32 numObjects = GetNumObjects();
	for ( Int32 i=0; i<numObjects; i++ )
	{
		// The way we create the object depends on the object class
		if ( objectClass->IsObject() )
		{
			// CObject requires a valid parent
			CBasePropItem* p = m_parent;
			CObject* parentObject = NULL;
			while ( p && !parentObject )
			{
				parentObject = p->GetParentObject( i ).AsObject();
				p = p->GetParent();
			}

			// If we didn't find a parent object normally, check if the properties page has a default parent object given.
			if ( !parentObject )
			{
				parentObject = m_page->GetDefaultParentObject();
			}

			// Create object
			if ( parentObject )
			{
				// Create inlined object
				CObject* newObject = CreateObject< CObject >( objectClass, parentObject, OF_Inlined );
				newObject->OnCreatedInEditor();

				// Set object pointer
				WriteObj( &newObject, i );
			}
		}
		else if ( objectClass->IsSerializable() )
		{
			// ISerializable objects are created directly, no memory management
			THandle< ISerializable > newObject = objectClass->CreateObject< ISerializable >();

			ISerializable* objPtr = newObject.Get();

			objPtr->OnCreatedInEditor();

			// Set object pointer
			WriteObj( &objPtr, i );
		}
		else
		{
			// Non serializable object - just a pointer to a raw RTTI structure - we still support it
			const Bool callConstructor = true;
			void* newObject = objectClass->CreateObject( objectClass->GetSize(), callConstructor );

			// Set object pointer
			WriteObj( &newObject, i );
		}
	}

	// Show object content
	Collapse();
	Expand();
}

void CPropertyItemPointer::OnObjectInlineNew( wxCommandEvent& event )
{
	// Get pointed class
	CClass* typeClass = GetParentObject( 0 ).m_class;

	// Get current class of the object that is there
	CClass* currentClass = NULL;
	const Uint32 numObjects = GetNumObjects();
	for ( Uint32 i=0; i<numObjects; i++ )
	{
		void* object = NULL;
		if ( ReadObj( &object, i ) && object != NULL )
		{
			if ( typeClass->IsSerializable() )
			{
				// dynamic class
				currentClass = ((ISerializable*)object)->GetClass();
			}
			else
			{
				// static class
				currentClass = static_cast< CClass* >( typeClass );
			}
		}
	}

	// Pick new class for object
	CEdClassSelectorDialog classPicker( GetPage(), typeClass, currentClass, false );
	if ( CClass* objectClass = classPicker.Execute() )
	{
		CPropertyItemPointer::OnObjectInlineNewInternal( objectClass );
	}
}

Bool CPropertyItemPointer::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
    IRTTIType* pointedType = NULL;
    void *pointedObject = NULL;

    if ( objectIndex == -1 )
    {
        // Get object
        if ( m_propertyType->GetType() == RT_Pointer )
        {
            CRTTIPointerType* pointerType = ( CRTTIPointerType* )m_propertyType;
            pointedType = pointerType->GetPointedType();
            if ( pointedType->GetType() == RT_Class )
            {
                CClass *classPtr = (CClass *)pointedType;
                if ( !classPtr->IsAbstract() )
                {
                    pointedObject = classPtr->GetDefaultObjectImp();
                }
            }
        }
        else if ( m_propertyType->GetType() == RT_Handle )
        {
            CRTTIHandleType* pointerType = ( CRTTIHandleType* )m_propertyType;
            pointedType = pointerType->GetPointedType();
            if ( pointedType->GetType() == RT_Class )
            {
                CClass *classPtr = (CClass *)pointedType;
                if ( !classPtr->IsAbstract() )
                {
                    pointedObject = classPtr->GetDefaultObjectImp();
                }
            }
        }
    }
    else
    {
        // Get value
        CPropertyDataBuffer propBuffer( m_propertyType );
        Read( propBuffer.Data(), objectIndex );

        // Get object
        if ( m_propertyType->GetType() == RT_Pointer )
        {
            CRTTIPointerType* pointerType = ( CRTTIPointerType* )m_propertyType;
            pointedType = pointerType->GetPointedType();
            pointedObject = pointerType->GetPointed( propBuffer.Data() );
        }
        else if ( m_propertyType->GetType() == RT_Handle )
        {
            CRTTIHandleType* pointerType = ( CRTTIHandleType* )m_propertyType;
            pointedType = pointerType->GetPointedType();
            pointedObject = pointerType->GetPointed( propBuffer.Data() );
        }
    }

    if ( pointedObject == NULL )
    {
        return false;
    }

	// Get property value
	if ( childItem->GetProperty() )
	{
		childItem->GetProperty()->Get( pointedObject, buffer );
	}
	else
	{
		ASSERT( pointedType == childItem->GetPropertyType() );
		pointedType->Copy( buffer, pointedObject );
	}

	// Done
	return true;
}

Bool CPropertyItemPointer::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	// Get property value
	CPropertyDataBuffer propBuffer( m_propertyType );
	Read( propBuffer.Data(), objectIndex );

	// Get object
	IRTTIType* pointedType = NULL;
	void *pointedObject = NULL;
	if ( m_propertyType->GetType() == RT_Pointer )
	{
		CRTTIPointerType* pointerType = ( CRTTIPointerType* )m_propertyType;
		pointedType = pointerType->GetPointedType();
		pointedObject = pointerType->GetPointed( propBuffer.Data() );
	}
	else if ( m_propertyType->GetType() == RT_Handle )
	{
		CRTTIHandleType* pointerType = ( CRTTIHandleType* )m_propertyType;
		pointedType = pointerType->GetPointedType();
		pointedObject = pointerType->GetPointed( propBuffer.Data() );
	}

	// Send event
	m_page->PropertyPreChange( childItem->GetProperty(), STypedObject( pointedObject, pointedType ) );

	// Change property value
	if ( childItem->GetProperty() )
	{
		childItem->GetProperty()->Set( pointedObject, buffer );
	}
	else
	{
		ASSERT( pointedType == childItem->GetPropertyType() );
		pointedType->Copy( pointedObject, buffer );
	}

	// Send global event
	m_page->PropertyPostChange( childItem->GetProperty(), STypedObject( pointedObject, pointedType ) );

	// Done
	return true;
}

Bool CPropertyItemPointer::SerializeXML( IXMLFile& file )
{
    CClass* pointedClass = GetParentObject( 0 ).m_class;
    if ( pointedClass->IsA<CCurve>() )
    {
        CCurve* curve = NULL;
        if ( ReadObj( &curve, 0 ) )
        {
            if ( curve )
            {
                String name = GetCaption();
                file.BeginNode( TXT("property") );        
                file.Attribute( TXT("name"), name );
                curve->OnSerializeXML( file );
                file.EndNode();
                return true;
            }
        }
    }
    else
    {
        return CPropertyItem::SerializeXML( file );
    }

    return false;
}

Bool CPropertyItemPointer::ShouldLinkIconBeDisplayed()
{
	// Get object property and determine object class from that
	CClass* pointedClass = GetParentObject( 0 ).m_class;
	ASSERT( pointedClass->GetType() == RT_Class );

	// Is this a resource class
	if ( pointedClass->IsA<CResource>() )
	{
		String selectedResource;
		return GetActiveResource( selectedResource );
	}

	// It's an entity, get it from active selection
	if ( pointedClass->IsA<CEntity>() )
	{
		CEntity* selectedEntity = GetSelectedEntity();
		if ( selectedEntity && selectedEntity->IsA( pointedClass ) )
		{
			// Entity should be in the same layer as each edited object
			Bool sameLayer = true;
			for ( Int32 i=0; i<GetNumObjects(); i++ )
			{
				CObject *root = GetRootObject( i ).AsObject();
				if ( !root )
				{
					continue;
				}

				if ( selectedEntity->GetRoot() != root->GetRoot() )
				{
					return false;
				}
			}

			return true;
		}
	}

	// Cannot link
	return false;
}

Bool  CPropertyItemPointer::ReadObj( void* buffer, Int32 objectIndex /*=0*/ ) const
{
	if ( m_propertyType->GetType() == RT_Pointer )
	{
		return ((CPropertyItem*)this)->Read( buffer, objectIndex );
	}
	else if ( m_propertyType->GetType() == RT_Handle )
	{
		BaseSafeHandle handle;
		if ( ! ((CPropertyItem*)this)->Read( &handle, objectIndex ) )
		{
			return false;
		}
		*(void**) buffer = handle.Get();
		return true;
	}

	ASSERT( !"WTF" );
	return false;
}

void CPropertyItemPointer::WriteObj( void* buffer, Int32 objectIndex /*=0*/ ) const
{
	if ( m_propertyType->GetType() == RT_Pointer )
	{
		((CPropertyItem*)this)->Write( buffer, objectIndex );
	}
	else if ( m_propertyType->GetType() == RT_Handle )
	{
		ISerializable* object = *(ISerializable**) buffer;
		THandle< ISerializable > handle( object );
		((CPropertyItem*)this)->Write( &handle, objectIndex );
	}

}
