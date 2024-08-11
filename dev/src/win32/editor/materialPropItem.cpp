/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

CMaterialParameterItem::CMaterialParameterItem( CEdPropertiesPage* page, CBasePropItem* parent )
	: CPropertyItem( page, parent )
	, m_parameter( NULL )
{
	// , parameter->GetParameterProperty(), -1
}

void CMaterialParameterItem::Init( CMaterialParameter *param )
{
	m_parameter = param;
	CPropertyItem::Init( param->GetParameterProperty() );
}

String CMaterialParameterItem::GetCaption() const
{
	return m_parameter->GetParameterName().AsString();
}
