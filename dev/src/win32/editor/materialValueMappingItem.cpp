/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "materialValueMappingItem.h"
#include "materialValueMappingEditor.h"
#include "../../common/engine/meshTypeComponent.h"
#include "../../common/engine/meshTypeResource.h"


//////////////////////////////////////////////////////////////////////////
// Material/Value interfaces for use with Material Value Mapping Editor.
// If additional types need to be supported by this, can add more.


/// Materials interface, to get materials from a CMeshTypeResource.
class MeshTypeResourceMaterialsInterface : public IEdMaterialValueMappingMaterialsInterface
{
	CMeshTypeResource* m_meshTypeResource;

public:
	MeshTypeResourceMaterialsInterface( CMeshTypeResource* mtr )
		: m_meshTypeResource( mtr )
	{}
	virtual ~MeshTypeResourceMaterialsInterface() {}

	virtual Uint32		GetNumMaterials() const					{ return m_meshTypeResource->GetMaterials().Size(); }
	virtual IMaterial*	GetMaterial( Uint32 index ) const		{ return m_meshTypeResource->GetMaterials()[index].Get(); }

	// Is is possible for NO_RESOURCE_IMPORT in editor?
#ifndef NO_RESOURCE_IMPORT
	virtual String		GetMaterialName( Uint32 index ) const	{ return m_meshTypeResource->GetMaterialNames()[index]; }
#else
	virtual String		GetMaterialName( Uint32 index ) const	{ return String::Printf( TXT("Material %d"), index ); }
#endif
};



/// Values interface, to edit values held in an array, through the array's CPropertyItem. The array is resized according
/// to the number of materials. Each time a value is changed the property item is re-written.
class ArrayPropertyValuesInterface : public IEdMaterialValueMappingValuesInterface
{
	CPropertyItem*	m_propertyItem;
	CBaseArray		m_valuesArray;		// Mirror of the property's array, so we can write to the property item easily.
	CRTTIArrayType*	m_arrayType;

public:
	ArrayPropertyValuesInterface( CPropertyItem* item )
		: m_propertyItem( item )
		, m_valuesArray()
	{
		void* obj = m_propertyItem->GetParentObject( 0 ).m_object;

		m_arrayType = static_cast< CRTTIArrayType* >( m_propertyItem->GetPropertyType() );
		m_arrayType->Copy( &m_valuesArray, m_propertyItem->GetProperty()->GetOffsetPtr( obj ) );
	}

	virtual ~ArrayPropertyValuesInterface() {}

	virtual void SetNumValues( Uint32 num )
	{
		// Add or remove elements so that the array size matches.
		Uint32 currSize = m_arrayType->GetArraySize( &m_valuesArray );
		if ( num > currSize )
		{
			m_arrayType->AddArrayElement( &m_valuesArray, num - currSize );
		}
		else
		{
			// This is far from optimal, but shouldn't really happen much. The number of materials will probably
			// be fairly constant even between editing sessions, since the number of materials used by a given mesh
			// doesn't really change much unless it gets re-imported and is totally different.
			while ( num < currSize )
			{
				m_arrayType->DeleteArrayElement( &m_valuesArray, currSize - 1 );
				--currSize;
			}
		}

		// And flush the changes to the property item.
		m_propertyItem->Write( &m_valuesArray );
	}

	virtual Uint32 GetNumValues() const { return m_arrayType->GetArraySize( &m_valuesArray ); }

	virtual IRTTIType* GetValueType() const { return m_arrayType->GetInnerType(); }

	virtual void GetValue( Uint32 index, void* outData ) const
	{
		// Don't have to go to the property item, just get value from our local mirror.
		const void* element = m_arrayType->GetArrayElement( &m_valuesArray, index );
		m_arrayType->GetInnerType()->Copy( outData, element );
	}

	virtual void SetValue( Uint32 index, const void* data )
	{
		RED_ASSERT( index < m_arrayType->GetArraySize( &m_valuesArray ) );

		// Write into our own local mirror of the values.
		void* element = m_arrayType->GetArrayElement( &m_valuesArray, index );
		m_arrayType->GetInnerType()->Copy( element, data );

		// Now write to the property item.
		m_propertyItem->Write( &m_valuesArray );
	}
};


//////////////////////////////////////////////////////////////////////////


CEdMaterialValueMappingPropertyItem::CEdMaterialValueMappingPropertyItem( CPropertyItem* propertyItem, Bool inlined )
	: ICustomPropertyEditor( propertyItem )
	, m_inlined( inlined )
	, m_materialsInterface( nullptr )
	, m_valuesInterface( nullptr )
{
	// Create appropriate materials interface
	CClass* objType = m_propertyItem->GetProperty()->GetParent();
	if ( objType )
	{
		// Add further support as needed
		if ( CMeshTypeComponent* component = m_propertyItem->GetParentObject( 0 ).As< CMeshTypeComponent >() )
		{
			m_materialsInterface = new MeshTypeResourceMaterialsInterface( component->GetMeshTypeResource() );
		}
	}
	RED_ASSERT( m_materialsInterface != nullptr, TXT("Property %s [%s] unsupported. No materials source"), propertyItem->GetProperty()->GetName().AsString().AsChar(), m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );


	// Create appropriate Values interface
	if ( m_propertyItem->GetPropertyType()->GetType() == RT_Array )
	{
		m_valuesInterface = new ArrayPropertyValuesInterface( m_propertyItem );
	}
	RED_ASSERT( m_valuesInterface != nullptr, TXT("Property %s [%s] unsupported. No values source"), propertyItem->GetProperty()->GetName().AsString().AsChar(), m_propertyItem->GetPropertyType()->GetName().AsString().AsChar() );
}


CEdMaterialValueMappingPropertyItem::~CEdMaterialValueMappingPropertyItem()
{
}


void CEdMaterialValueMappingPropertyItem::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	if ( m_materialsInterface && m_valuesInterface )
	{
		m_propertyItem->AddButton( m_propertyItem->GetPage()->GetStyle().m_iconPick, wxCommandEventHandler( CEdMaterialValueMappingPropertyItem::OnOpenEditor ), this );
	}
}

void CEdMaterialValueMappingPropertyItem::CloseControls()
{
}

void CEdMaterialValueMappingPropertyItem::OnOpenEditor( wxCommandEvent& event )
{
	CEdMaterialValueMappingEditor* mappingEditor = new CEdMaterialValueMappingEditor( m_propertyItem->GetPage(), 
		m_materialsInterface, m_valuesInterface, m_propertyItem->GetProperty()->GetHint(), m_inlined );
	mappingEditor->Execute();
}
