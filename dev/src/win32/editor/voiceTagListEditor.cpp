/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "voiceTagListEditor.h"
#include "editorExternalResources.h"

#include "../../common/core/depot.h"
#include "../../common/game/storySceneVoiceTagsManager.h"

CVoiceTagListSelection::CVoiceTagListSelection( CPropertyItem* item )
	: CListSelection( item )
{
}

void CVoiceTagListSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Empty voicetag
	m_ctrlChoice->AppendString( wxEmptyString );

	// Get the voice tag list
	const C2dArray* voiceTagArray = SStorySceneVoiceTagsManager::GetInstance().ReloadVoiceTags();
	if ( voiceTagArray )
	{
		// Fill temporary array with voice tags
		TDynArray<String> tempVoiceTags;
		for ( Uint32 i=0; i<voiceTagArray->GetNumberOfRows(); i++ )
		{
			String voiceTag = voiceTagArray->GetValue( 0, i );
			if ( voiceTag.GetLength() > 0 )
			{
				tempVoiceTags.PushBack(voiceTag);
			}
		}

		// Sort voice tags
		Sort(tempVoiceTags.Begin(), tempVoiceTags.End());

		// Fill choice control with sorted voice tags
		for (Uint32 i = 0; i < tempVoiceTags.Size(); ++i)
		{
			m_ctrlChoice->AppendString(tempVoiceTags[i].AsChar());
		}
	}

	// Make sure empty list is displayed properly
	if ( !m_ctrlChoice->GetCount() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no voicetags )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CVoiceTagListSelection::OnChoiceChanged ), NULL, this );
	}
}
