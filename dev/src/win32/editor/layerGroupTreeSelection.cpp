/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerGroupTreeSelection.h"
#include "layerGroupTree.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"

CEdLayerGroupTreeSelection::CEdLayerGroupTreeSelection( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
}

void CEdLayerGroupTreeSelection::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// We need a world to edit layer
	if ( GGame->GetActiveWorld() )
	{
		wxBitmap editIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
		m_propertyItem->AddButton( editIcon, wxCommandEventHandler( CEdLayerGroupTreeSelection::OnShowEditor ), this );
		//wxBitmap deleteIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
		//m_propertyItem->AddButton( deleteIcon, wxCommandEventHandler( CEdLayerGroupTreeSelection::OnClearLayer ), this );
	}
}

void CEdLayerGroupTreeSelection::CloseControls()
{
}

CLayerGroup* CEdLayerGroupTreeSelection::GetBaseGroup()
{
	// Check first if we are inside a layer, if so use the relative path
	CObject* rootObject = m_propertyItem->GetRootObject(0).AsObject();
	if ( rootObject )
	{
		CLayer* parentLayer = rootObject->FindParent< CLayer >();
		if ( parentLayer )
		{
			CLayerInfo* parentInfo = parentLayer->GetLayerInfo();
			if ( parentInfo )
			{
				CLayerGroup* parentGroup = parentInfo->GetLayerGroup();
				if ( parentGroup )
				{
					return parentGroup;
				}
			}
		}
	}

	// Use world if we have on
	if ( GGame->GetActiveWorld() )
	{
		return GGame->GetActiveWorld()->GetWorldLayers();
	}

	// Nothing
	return NULL;
}

Bool CEdLayerGroupTreeSelection::GrabValue( String& displayValue )
{
	// Read the group name
	String groupName;
	m_propertyItem->Read( &groupName );

	// Get in context of world
	if ( GGame->GetActiveWorld() )
	{
		// Get parent group
		CLayerGroup* foundGroup = NULL;
		CLayerGroup* baseGroup = GetBaseGroup();
		if ( baseGroup )
		{
			foundGroup = baseGroup->FindGroupByPath( groupName );
			if ( foundGroup )
			{
				displayValue = String::Printf( TXT("%s"), groupName.AsChar() );
				return true;
			}
		}
	}

	// Invalid or missing
	displayValue = String::Printf( TXT("INVALID: %s"), groupName.AsChar() );
	return true;
}

Bool CEdLayerGroupTreeSelection::SaveValue()
{
	// Do nothing
	return false;
}

void CEdLayerGroupTreeSelection::OnShowEditor( wxCommandEvent &event )
{
	CLayerGroup* baseGroup = GetBaseGroup();
	if ( baseGroup )
	{
		// Get current layer name
		String currentLayerName;
		m_propertyItem->Read( &currentLayerName );

		// Show editor
		CEdLayerGroupTree dialog( m_propertyItem->GetPage(), GetBaseGroup(), currentLayerName );
		if ( 0 == dialog.ShowModal() )
		{
			CLayerGroup* selected = dialog.GetSelectedGroup();
			if ( selected )
			{
				String groupName = selected->GetGroupPathName( baseGroup );
				m_propertyItem->Write( &groupName );
			}
			else
			{
				String emptyGroupName = TXT("");
				m_propertyItem->Write( &emptyGroupName );
			}

			// Refresh
			m_propertyItem->GrabPropertyValue();
		}
	}
}

void CEdLayerGroupTreeSelection::OnClearLayer( wxCommandEvent &event )
{
	// Reset value
	String value = TXT("");
	m_propertyItem->Write( &value );
	m_propertyItem->GrabPropertyValue();
}
