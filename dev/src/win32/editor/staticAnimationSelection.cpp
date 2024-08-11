#include "build.h"
#include "staticAnimationSelection.h"
#include "..\..\common\game\extAnimGameplayMimicEvent.h"

CEdStaticAnimationSelection::CEdStaticAnimationSelection( CPropertyItem* item )
	: CListSelection( item )
{

}

void CEdStaticAnimationSelection::CreateControls( const wxRect &propRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	m_ctrlChoice = new CEdChoice( m_propertyItem->GetPage(), propRect.GetTopLeft(), propRect.GetSize() );
	m_ctrlChoice->SetWindowStyle( wxNO_BORDER );
	m_ctrlChoice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_ctrlChoice->SetFocus();

	// Fill
	const CSkeletalAnimationSet* set = GetAnimationSet();
	if ( !set )
	{
		// Add empty stub
		m_ctrlChoice->Enable( false );
		m_ctrlChoice->SetSelection( 0 );
	}
	else
	{
		const TDynArray< CSkeletalAnimationSetEntry* >& anims = set->GetAnimations();

		for ( Uint32 i=0; i<anims.Size(); i++ )
		{
			m_ctrlChoice->AppendString( anims[ i ]->GetName().AsString().AsChar() );
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
		m_ctrlChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdStaticAnimationSelection::OnChoiceChanged ), NULL, this );
	}
}

//////////////////////////////////////////////////////////////////////////

CEdGameplayMimicSelection::CEdGameplayMimicSelection( CPropertyItem* item )
	: CEdStaticAnimationSelection( item )
{

}

const CSkeletalAnimationSet* CEdGameplayMimicSelection::GetAnimationSet() const
{
	return CExtAnimGameplayMimicEvent::GetAnimationSet();
}
