/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "textureGroupSelection.h"
#include "../../common/engine/renderSettings.h"

CTextureGroupSelectionList::CTextureGroupSelectionList( CPropertyItem* item )
	: CListSelection( item )
{
}

void CTextureGroupSelectionList::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Fill choice control with values
	TDynArray< String > groupNames;
	TTextureGroupsMap groups = SRenderSettingsManager::GetInstance().GetTextureGroups();
	for ( TTextureGroupsMap::const_iterator i=groups.Begin(); i!=groups.End(); ++i )
	{
		if ( i->m_second.m_isUser )
		{
			String groupName = i->m_first.AsString();
			groupNames.PushBack( groupName );
		}
	}

	// Sort list and add to combo box
	Sort( groupNames.Begin(), groupNames.End() );
	for ( Uint32 i=0; i<groupNames.Size(); i++ )
	{
		m_ctrlChoice->AppendString( groupNames[i].AsChar() );
	}

	// Find current value on list and select it
	String str;
	GrabValue( str );
	int index = m_ctrlChoice->FindString( str.AsChar() );
	if ( index >= 0 )
	{
		m_ctrlChoice->SetSelection( index );
	}
	else
	{
		m_ctrlChoice->SetSelection( 0 );
	}

	// Notify of selection changes
	m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CTextureGroupSelectionList::OnChoiceChanged ), NULL, this );
}
