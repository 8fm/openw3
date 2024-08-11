/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/drawableComponent.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/materialDefinition.h"


// HACK : We treat the enableMask property special, so that we can show it only when the definition supports it.
// So, need to check it by name :(
// Any changes to the property/variable name need to be reflected here too! #define so it's less likely to miss one.
RED_DEFINE_STATIC_NAME( enableMask );
#define ENABLE_MASK_NAME RED_NAME( enableMask )

IMPLEMENT_RTTI_CLASS( CMaterialInstanceGroupItem )

CMaterialInstanceGroupItem::CMaterialInstanceGroupItem()
	: CBaseGroupItem( nullptr, nullptr )
{
}

// HACK: The assingment operator here is only to enable compilation of TTypedClass::Copy, but shouldn't be used.
// This should be resolved in the future by property handling non-copyable IReferencables.
void CMaterialInstanceGroupItem::operator = ( const CMaterialInstanceGroupItem& )
{
	RED_HALT( "Illegal assignment" );
}

CMaterialInstanceGroupItem::CMaterialInstanceGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, CMaterialInstance* materialInstance )
	: CBaseGroupItem( page, parent )
	, m_materialInstance( materialInstance )
	, m_parametersGroupItem( nullptr )
	, m_enableMaskItem( nullptr )
{}

void CMaterialInstanceGroupItem::RemoveSpecialItems()
{
	if ( m_enableMaskItem )
	{
		m_page->DiscardItem( m_enableMaskItem );
		m_children.Remove( m_enableMaskItem );
		m_enableMaskItem = nullptr;
	}
	if ( m_parametersGroupItem )
	{
		m_parametersGroupItem->Collapse();
		m_page->DiscardItem( m_parametersGroupItem );
		m_children.Remove( m_parametersGroupItem );
		m_parametersGroupItem = nullptr;
	}
}

void CMaterialInstanceGroupItem::RecreateSpecialItems()
{
	const Bool wasParametersExpanded = (m_parametersGroupItem && m_parametersGroupItem->IsExpanded());

	// Remove existing
	RemoveSpecialItems();

	// Create optional "enableMask" item
	{
		IMaterialDefinition* definition = m_materialInstance->GetMaterialDefinition();
		if ( definition != nullptr && definition->IsMasked() && definition->CanInstanceOverrideMasked() )
		{
			CProperty *prop = m_materialInstance->GetClass()->FindProperty( ENABLE_MASK_NAME );
			RED_ASSERT( prop != nullptr && prop->IsEditable() );

			m_enableMaskItem = CreatePropertyItem( m_page, this, prop );
			m_enableMaskItem->GrabPropertyValue();
		}
	}


	// Create parameters group
	{
		m_parametersGroupItem = new CMaterialGroupItem( m_page, this, m_materialInstance );

		// Set appropriate expanded state
		if ( wasParametersExpanded != m_parametersGroupItem->IsExpanded() )
		{
			if ( wasParametersExpanded )
			{
				m_parametersGroupItem->Expand();
			}
			else
			{
				m_parametersGroupItem->Collapse();
			}
		}
	}
}

void CMaterialInstanceGroupItem::Expand()
{
	TDynArray< CProperty* > properties;
	m_materialInstance->GetClass()->GetProperties( properties );
	
	for ( Uint32 i=0; i<properties.Size(); ++i )
	{
		CProperty *prop = properties[i];
		if ( !prop->IsEditable() )
			continue;

		// HACK : We only want to show "enableMask" when the base material can be masked. So we'll skip it here, and
		// add it manually after. This way we can easily hide/show it whenever the base material changes, just like
		// updating the material properties group.
		if ( prop->GetName() == ENABLE_MASK_NAME )
			continue;

		if ( CPropertyItem* item = CreatePropertyItem( m_page, this, prop ) )
		{
			item->GrabPropertyValue();
		}
	}

	RecreateSpecialItems();
	if ( m_parametersGroupItem && !m_parametersGroupItem->IsExpanded() )
	{
		m_parametersGroupItem->Expand();
	}
}

void CMaterialInstanceGroupItem::Collapse()
{
	RemoveSpecialItems();
	CBaseGroupItem::Collapse();
}

String CMaterialInstanceGroupItem::GetCaption() const
{
	return TXT("Material Instance");
}

Bool CMaterialInstanceGroupItem::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	return CBaseGroupItem::ReadImp( childItem, buffer, objectIndex );
}

Bool CMaterialInstanceGroupItem::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex )
{
	if ( CBaseGroupItem::WriteImp( childItem, buffer, objectIndex ) )
	{
		THandle< CMaterialInstanceGroupItem > self = this;
		RunLaterOnce( [ self ]( ){
			if ( self )
			{
				self->RecreateSpecialItems();
			}
		} );
		return true;
	}

	// Not changed
	return false;
}
