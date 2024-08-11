/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerGroupListSelection.h"
#include "layerGroupList.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/layerInfo.h"

CEdLayerGroupListSelection::CEdLayerGroupListSelection( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
}

void CEdLayerGroupListSelection::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// We need a world to edit layer
	if ( GGame->GetActiveWorld() )
	{
		wxBitmap editIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
		m_propertyItem->AddButton( editIcon, wxCommandEventHandler( CEdLayerGroupListSelection::OnShowEditor ), this );
		wxBitmap deleteIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
		m_propertyItem->AddButton( deleteIcon, wxCommandEventHandler( CEdLayerGroupListSelection::OnClearLayer ), this );
	}
}

void CEdLayerGroupListSelection::CloseControls()
{
}

CLayerGroup* CEdLayerGroupListSelection::GetBaseGroup()
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

Bool CEdLayerGroupListSelection::GrabValue( String& displayValue )
{
	// Read the group name
	TDynArray< String > groupNames;
	m_propertyItem->Read( &groupNames );

	// Get in context of world
	if ( GGame->GetActiveWorld() )
	{
		if ( groupNames.Empty() )
		{
			displayValue = TXT("<Empty>");
		}
		else
		{
			displayValue = String::Printf( TXT("<%i layer(s)>"), groupNames.Size() );
		}
	}
	else
	{
		displayValue = TXT("<Invalid>");
	}

	// Done
	return true;
}

Bool CEdLayerGroupListSelection::SaveValue()
{
	// Do nothing
	return false;
}

void CEdLayerGroupListSelection::OnShowEditor( wxCommandEvent &event )
{
	CLayerGroup* baseGroup = GetBaseGroup();
	if ( baseGroup )
	{
		// Get current layer name
		TDynArray< String > groupNames;
		m_propertyItem->Read( &groupNames );

		// Show editor
		CEdLayerGroupList dialog( m_propertyItem->GetPage(), GetBaseGroup(), groupNames );
		if ( 0 == dialog.ShowModal() )
		{
			TDynArray< String > newGroupNames;
			const TDynArray< CLayerGroup* >& selected = dialog.GetSelectedGroups();
			for ( Uint32 i=0; i<selected.Size(); i++ )
			{				
				String groupName = selected[i]->GetGroupPathName( baseGroup );
				if ( !groupName.Empty() )
				{
					newGroupNames.PushBack( groupName );
				}
			}

			// Refresh
			m_propertyItem->Write( &newGroupNames );
			m_propertyItem->GrabPropertyValue();
		}
	}
}

void CEdLayerGroupListSelection::OnClearLayer( wxCommandEvent &event )
{
	// Reset value
	TDynArray< String > groupNames;
	m_propertyItem->Write( &groupNames );
	m_propertyItem->GrabPropertyValue();
}
