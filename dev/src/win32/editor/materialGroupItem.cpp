/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "..\..\common\engine\drawableComponent.h"
#include "..\..\common\engine\materialBlock.h"
#include "..\..\common\engine\materialGraph.h"
#include "..\..\common\engine\materialOutputSocket.h"
#include "..\..\common\engine\materialOutputTextureSocket.h"
#include "..\..\common\engine\materialOutputCubeSocket.h"
#include "..\..\common\engine\graphConnection.h"
#include "..\..\common\engine\materialInstance.h"

CMaterialGroupItem::CMaterialGroupItem( CEdPropertiesPage* page, CBasePropItem* parent, IMaterial* material, CName paramGroup )
	: CBaseGroupItem( page, parent )
	, m_material( material )
	, m_paramGroup( paramGroup )
{
	if ( m_paramGroup == CName::NONE )
	{
		Expand();
	}
}

String CMaterialGroupItem::GetCaption() const
{
	return m_paramGroup ? m_paramGroup.AsString() : TXT("Material parameters");
}

Bool CMaterialGroupItem::IsInlined() const
{
	return false;
}

void CMaterialGroupItem::Expand()
{
	TSortedSet< CName > groups;
	TDynArray< CMaterialParameter* > params;

	// Reset
	m_childrenParams.Clear();

	// Get base material graph and extract parameters
	if ( CMaterialGraph* graph = Cast< CMaterialGraph >( m_material->GetMaterialDefinition() ) )
	{
		// Scan for artist variables
		for ( CGraphBlock* block : graph->GraphGetBlocks() )
		{
			if ( CMaterialParameter* param = Cast< CMaterialParameter >( block ) )
			{
				if ( param->GetParameterName() && param->GetParameterProperty() )
				{
					params.PushBack( param );

					if ( m_paramGroup == CName::NONE && param->GetParameterGroup() != CName::NONE ) // collect encountered group
					{
						groups.Insert( param->GetParameterGroup() );
					}
				}
			}
		}
	}

	// = Create groups =

	if ( m_paramGroup == CName::NONE ) // when we are in a root group, create sub-groups
	{
		for ( CName groupName : groups )
		{
			new CMaterialGroupItem( m_page, this, m_material, groupName );
		}
	}

	// = Create properties =

	Sort( params.Begin(), params.End(), []( CMaterialParameter* a, CMaterialParameter* b ) 
	{ 
		return Red::System::StringCompareNoCase( a->GetParameterName().AsChar(), b->GetParameterName().AsChar() ) < 0; 
	} );

	for ( CMaterialParameter* param : params )
	{
		if ( param->GetParameterGroup() == m_paramGroup ) // accept only properties falling into chosen group (which can be NONE)
		{
			CPropertyItem* item = CreatePropertyItem( m_page, this, param->GetParameterProperty() );
			item->SetCaption( param->GetParameterName().AsString() );
			m_childrenParams[ item ] = param;
			item->GrabPropertyValue();
		}
	}

	// Call base implementation
	CBasePropItem::Expand();
}

void CMaterialGroupItem::Collapse()
{
	m_childrenParams.Clear();
	CBasePropItem::Collapse();
}

Bool CMaterialGroupItem::WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	// Get parameter	
	CMaterialParameter* parameter;
	if ( m_childrenParams.Find( childItem, parameter ) )
	{
		// Inform object & properties browser
		m_page->PropertyPreChange( parameter->GetParameterProperty(), STypedObject( m_material ) );

		// Write data !
		m_material->WriteParameterRaw( parameter->GetParameterName(), buffer );

		// Inform object & properties browser
		m_page->PropertyPostChange( parameter->GetParameterProperty(), STypedObject( m_material ) );

		// Found
		return true;
	}

	// Not valid
	return false;
}

Bool CMaterialGroupItem::ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex /*= 0*/ )
{
	// Get parameter
	CMaterialParameter* parameter;
	if ( m_childrenParams.Find( childItem, parameter ) )
	{
        IMaterial *material = ( objectIndex == -1 ) ? m_material->GetBaseMaterial() : m_material;
		//! Get data
        return material->ReadParameterRaw( CName( parameter->GetParameterName() ), buffer );
	}

	// Not valid
	return false;
}
