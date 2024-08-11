/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../../common/game/jobTreeLeaf.h"
#include "jobActionAnimSelection.h"

#include "../../common/core/depot.h"
#include "../../common/core/gatheredResource.h"

CJobActionAnimSelection::CJobActionAnimSelection( CPropertyItem* item )
	: CListSelection( item )
{
	m_jobAction = SafeCast< CJobActionBase >( item->GetParentObject( 0 ).AsObject() );
}

void CJobActionAnimSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Fill choice control with values

	const String &animCategory = m_jobAction->GetAnimCategory();
	const C2dArray& animsets2dArray = SAnimationsCategoriesResourcesManager::GetInstance().Get2dArray();
	String animsetDepotPath( String::EMPTY );
	CSkeletalAnimationSet *animsetRes = NULL;
	TDynArray< String > animNames;

	// Find animset depot path and load animset resource if path was found
	Uint32 colSize, rowSize;
	animsets2dArray.GetSize( colSize, rowSize );
	for ( Uint32 rowNum = 0; rowNum < rowSize; rowNum++ )
	{
		if ( animsets2dArray.GetValue( 0, rowNum ) == animCategory )
		{
			animsetDepotPath = animsets2dArray.GetValue( 1, rowNum );
			animsetRes = Cast< CSkeletalAnimationSet >( GDepot->LoadResource( animsetDepotPath ) );
			break;
		}
	}

	if ( animsetRes )
	{
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		animsetRes->GetAnimations( animations );
		for ( TDynArray< CSkeletalAnimationSetEntry* >::const_iterator skelAnimEntry = animations.Begin();
			  skelAnimEntry != animations.End();
			  ++skelAnimEntry )
		{
			animNames.PushBack( (*skelAnimEntry)->GetAnimation()->GetName().AsString() );
		}
	}

	if ( animNames.Empty() )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->AppendString( TXT("( no animations available )") );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		// Sort list and add to combo box
		Sort( animNames.Begin(), animNames.End() );
		m_ctrlChoice->Freeze();
		for ( Uint32 i=0; i < animNames.Size(); i++ )
		{
			m_ctrlChoice->AppendString( animNames[i].AsChar() );
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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CJobActionAnimSelection::OnChoiceChanged ), NULL, this );
		m_ctrlChoice->Thaw();
		m_ctrlChoice->Refresh();
	}
}
