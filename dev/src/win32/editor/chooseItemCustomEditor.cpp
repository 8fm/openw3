/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "chooseItemDialog.h"
#include "chooseItemCustomEditor.h"

CEdChooseItemCustomEditor::CEdChooseItemCustomEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
{
}

void CEdChooseItemCustomEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	wxBitmap editIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_PICK") );
	wxBitmap deleteIcon = SEdResources::GetInstance().LoadBitmap( TEXT("IMG_PB_DELETE") );
	m_propertyItem->AddButton( editIcon, wxCommandEventHandler( CEdChooseItemCustomEditor::OnShowEditor ), this );
	m_propertyItem->AddButton( deleteIcon, wxCommandEventHandler( CEdChooseItemCustomEditor::OnClearValue ), this );
}

Bool CEdChooseItemCustomEditor::GrabValue( String& displayValue )
{
	// Read the tree
	CName itemValue;
	m_propertyItem->Read( &itemValue );

	// Show the info
	displayValue = itemValue.AsString();
	return true;
}

void CEdChooseItemCustomEditor::OnShowEditor( wxCommandEvent& event )
{
	// Read the condition
	CName itemValue;
	m_propertyItem->Read( &itemValue );

	//// Redraw property
	m_propertyItem->GrabPropertyValue();

	m_chooseDialog = new CEdChooseItemDialog( m_propertyItem->GetPage(), itemValue );
	m_chooseDialog->Connect( wxEVT_CHOOSE_ITEM_OK, wxCommandEventHandler( CEdChooseItemCustomEditor::OnEditorOk ), NULL, this );
	m_chooseDialog->Connect( wxEVT_CHOOSE_ITEM_CANCEL, wxCommandEventHandler( CEdChooseItemCustomEditor::OnEditorCancel ), NULL, this );

	m_chooseDialog->ShowModal();
}

void CEdChooseItemCustomEditor::OnClearValue( wxCommandEvent& event )
{
	// Read the value
	CName itemValue = CName::NONE;

	// Write changes
	m_propertyItem->Write( &itemValue );

	// Redraw property ( show the "none" label )
	m_propertyItem->GrabPropertyValue();
}

void CEdChooseItemCustomEditor::OnEditorOk( wxCommandEvent &event )
{
	ASSERT( m_chooseDialog );

	// Update tags
	CName itemName;
	if ( m_propertyItem->Read( &itemName ) )
	{
		itemName = m_chooseDialog->GetItem();

		// Write value
		m_propertyItem->Write( &itemName );
		m_propertyItem->GrabPropertyValue();
	}

	// Unlink tag editor
	m_chooseDialog = NULL;
}

void CEdChooseItemCustomEditor::OnEditorCancel( wxCommandEvent &event )
{
	// Unlink choose item editor
	ASSERT( m_chooseDialog );
	m_chooseDialog = NULL;
}
