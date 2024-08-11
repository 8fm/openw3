/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#if 0
#include "idInterlocutorIDEditor.h"

#include "../../games/r6/idResource.h"


CIDInterlocutorIDEditor::CIDInterlocutorIDEditor( CPropertyItem* item )
	: ICustomPropertyEditor( item )
	, m_choice( NULL )
{
}

void CIDInterlocutorIDEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	// Create editor
	wxSize size = propertyRect.GetSize();
	m_choice = new CEdChoice( m_propertyItem->GetPage(), propertyRect.GetTopLeft(), size );
	m_choice->SetWindowStyle( wxBORDER );
	m_choice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_choice->SetFocus();

	// Append the default
	m_choice->AppendString( TXT( "Defualt" ) );

	const Char* err = TXT("CIDInterlocutorIDEditor used incorrectly. Should be used for the property of CInteractiveDialog or it's contents. Please FIX!");

	// Get edited object
	CObject* obj = m_propertyItem->GetRootObject( 0 ).AsObject();
	if ( !obj )
	{
		ASSERT( false, err );
		return;
	}

	// Just get the parent until we reach CInteractiveDialog or NULL
	CInteractiveDialog*	interactiveDialog = Cast< CInteractiveDialog > ( obj );
	while ( interactiveDialog == NULL )
	{
		obj = obj->GetParent();
		if ( NULL == obj )
		{
			ASSERT( false, err ); 
			break;
		}
		interactiveDialog = Cast< CInteractiveDialog > ( obj );
	}

	if ( interactiveDialog == NULL )
	{
		return;
	}

	TDynArray< SIDInterlocutorDefinition >	actorDefinitions = interactiveDialog->GetInterlocutorDefinitions();
	for ( Uint32 i = 0; i < actorDefinitions.Size(); ++i )
	{
		String interlocutor	= actorDefinitions[ i ].m_interlocutorId.AsString();
		m_choice->AppendString( interlocutor.AsChar() );
	}

	// Append the everyone / no one
	m_choice->AppendString( TXT( "Nobody" ) );
	
	// Set previous value
	CName currentValue;
	m_propertyItem->Read( &currentValue );
	m_choice->SetStringSelection( currentValue.AsString().AsChar() );

	// Notify of selection changes
	m_choice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CIDInterlocutorIDEditor::OnChoiceChanged ), NULL, this );
}

void CIDInterlocutorIDEditor::CloseControls()
{
	// Close combo box
	if ( m_choice )
	{
		delete m_choice;
		m_choice = NULL;
	}
}

Bool CIDInterlocutorIDEditor::GrabValue( String& displayValue )
{
	return ICustomPropertyEditor::GrabValue( displayValue );
}

Bool CIDInterlocutorIDEditor::SaveValue()
{	
	return ICustomPropertyEditor::SaveValue();
}

Bool CIDInterlocutorIDEditor::DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour )
{
	return ICustomPropertyEditor::DrawValue( dc, valueRect, textColour );
}

void CIDInterlocutorIDEditor::OnChoiceChanged( wxCommandEvent &event )
{
	CName settingName( event.GetString() );

	m_propertyItem->Write( &settingName );
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}
#endif