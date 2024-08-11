/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "chooseRewardCustomEditor.h"
#include "rewardEditor.h"

CEdChooseRewardCustomEditor::CEdChooseRewardCustomEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
}

void CEdChooseRewardCustomEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxBitmap editIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	wxBitmap deleteIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
	m_propertyItem->AddButton( editIcon, wxCommandEventHandler( CEdChooseRewardCustomEditor::OnShowEditor ), this );
	m_propertyItem->AddButton( deleteIcon, wxCommandEventHandler( CEdChooseRewardCustomEditor::OnClearValue ), this );
}

Bool CEdChooseRewardCustomEditor::GrabValue( String& displayValue )
{
	// Read the tree
	CName rewardValue;
	m_propertyItem->Read( &rewardValue );

	// Show the info
	displayValue = rewardValue.AsString();
	return true;
}

void CEdChooseRewardCustomEditor::OnShowEditor( wxCommandEvent& event )
{
	// Read the condition
	CName rewardValue;
	m_propertyItem->Read( &rewardValue );

	//// Redraw property
	m_propertyItem->GrabPropertyValue();

	m_chooseDialog = new CEdRewardEditor( m_propertyItem->GetPage(), true, rewardValue );
	m_chooseDialog->Connect( wxEVT_CHOOSE_REWARD_OK, wxCommandEventHandler( CEdChooseRewardCustomEditor::OnEditorOk ), NULL, this );
	m_chooseDialog->Connect( wxEVT_CHOOSE_REWARD_CANCEL, wxCommandEventHandler( CEdChooseRewardCustomEditor::OnEditorCancel ), NULL, this );

	m_chooseDialog->Show();
}

void CEdChooseRewardCustomEditor::OnClearValue( wxCommandEvent& event )
{
	// Read the value
	CName rewardValue = CName::NONE;

	// Write changes
	m_propertyItem->Write( &rewardValue );

	// Redraw property ( show the "none" label )
	m_propertyItem->GrabPropertyValue();
}

void CEdChooseRewardCustomEditor::OnEditorOk( wxCommandEvent &event )
{
	ASSERT( m_chooseDialog );

	// Update tags
	CName rewardName;
	if ( m_propertyItem->Read( &rewardName ) )
	{
		rewardName = m_chooseDialog->GetSelectedRewardName();

		// Write value
		m_propertyItem->Write( &rewardName );
		m_propertyItem->GrabPropertyValue();
	}

	// Unlink tag editor
	m_chooseDialog = NULL;
}

void CEdChooseRewardCustomEditor::OnEditorCancel( wxCommandEvent &event )
{
	// Unlink choose item editor
	ASSERT( m_chooseDialog );
	m_chooseDialog = NULL;
}
