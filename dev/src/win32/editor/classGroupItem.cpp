/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dynamicPropGroupItem.h"

CClassGroupItem::CClassGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, CClass* classFilter )
	: CBaseGroupItem( page, parent )
	, m_class( classFilter )
{
	m_isExpandable = true;
	Expand();
}

String CClassGroupItem::GetCaption() const
{
	return m_class->GetName().AsString().AsChar();	
}

void CClassGroupItem::Expand()
{
	// Grab class properties
	const auto& properties = m_class->GetLocalProperties();

	// Add struct sub properties
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		CProperty* prop = properties[i];
		if ( !prop->IsEditable() )
		{
			continue;
		}
		
		// SimpleCurve and CCurve can both use the CPropertyItemSimpleCurve
		IRTTIType *type = prop->GetType();
		if ( type && ( type->GetName() == CNAME( SSimpleCurve ) || type->GetName() == CNAME( CCurve ) ) )
		{
			CPropertyItemSimpleCurve *item = new CPropertyItemSimpleCurve( m_page, this, prop );
		}
		// Other
		else
		{
			if ( CPropertyItem* item = CreatePropertyItem( m_page, this, prop ) )
			{
				item->GrabPropertyValue();
			}
		}
	}

	// Extra shit for dynamic properties
	if ( m_page->GetSettings().m_showDynamicProperties )
	{
		// Object uses dynamic property crap
		if ( m_class->IsObject() )
		{
			CObject* object = m_class->CastTo< CObject >( GetParentObject( 0 ).m_object );
			IDynamicPropertiesSupplier* dynamicProperties = object->QueryDynamicPropertiesSupplier();
			if ( dynamicProperties && object->GetClass() == m_class )
			{
				new CDynamicPropGroupItem( m_page, this, dynamicProperties );
			}
		}
	}

	CBasePropItem::Expand();
}
