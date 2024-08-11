/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"
#include "layerGuidListSelection.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/worldIterators.h"

CLayerGUIDListSelection::CLayerGUIDListSelection( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_ctrlChoice( NULL )
{
}

void CLayerGUIDListSelection::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propertyRect.GetTopLeft(), propertyRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	TDynArray< String > layersNames;
	m_layersIndexes.Clear();
	m_layersGUIDs.Clear();

	if ( CWorld *world = GGame->GetActiveWorld() )
	{
		Int32 index = 0;
		for ( WorldAttachedLayerIterator it( world ); it; ++it )
		{
			CLayer* layer = *it;
			if ( layer && layer->GetLayerInfo() )
			{
				const String &layerFriendlyName = layer->GetLayerInfo()->GetShortName();
				layersNames.PushBack( layerFriendlyName );
				m_layersIndexes.Insert( layer->GetGUID(), index++ );
				m_layersGUIDs.PushBack( layer->GetGUID() );
			}
		}
	}

	for ( Uint32 i=0; i < layersNames.Size(); i++ )
	{
		m_ctrlChoice->AppendString( layersNames[i].AsChar() );
	}

	// Find current value on list and select it
	CGUID layerGUID;
	m_propertyItem->Read( &layerGUID );
	int *index = m_layersIndexes.FindPtr( layerGUID );
	if ( index )
	{
		m_ctrlChoice->SetSelection( *index );
	}
	else
	{
		m_ctrlChoice->SetSelection( 0 );
	}

	// Notify of selection changes
	m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CLayerGUIDListSelection::OnChoiceChanged ), NULL, this );
}

void CLayerGUIDListSelection::CloseControls()
{
	if ( m_ctrlChoice )
	{
		delete m_ctrlChoice;
		m_ctrlChoice = NULL;
	}
}

Bool CLayerGUIDListSelection::GrabValue( String& displayValue )
{
	// As CName
	CGUID layerGUID;
	m_propertyItem->Read( &layerGUID );

	if ( CWorld *world = GGame->GetActiveWorld() )
	{
		CLayer *layer = world->FindLayer( layerGUID );
		if ( layer && layer->GetLayerInfo() )
		{
			displayValue = layer->GetLayerInfo()->GetShortName();
		}
	}

	// Assume grabbed value
	return true;
}

Bool CLayerGUIDListSelection::SaveValue()
{
	int index = m_ctrlChoice->GetSelection();
	if ( index >= 0)
	{
		CGUID layerGUID = m_layersGUIDs[ index ];
		m_propertyItem->Write( &layerGUID );
	}

	// Assume written
	return true;
}

void CLayerGUIDListSelection::OnChoiceChanged( wxCommandEvent &event )
{
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}
