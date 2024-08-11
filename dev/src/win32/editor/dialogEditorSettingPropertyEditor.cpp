/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "dialogEditorSettingPropertyEditor.h"

#include "../../common/game/storySceneControlPart.h"
#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneSystem.h"
#include "../../common/game/storySceneSpawner.h"

CEdDialogEditorSettingPropertyEditor::CEdDialogEditorSettingPropertyEditor( CPropertyItem* propertyItem )
	: ICustomPropertyEditor( propertyItem )
	, m_choice( NULL )
{

}

CEdDialogEditorSettingPropertyEditor::~CEdDialogEditorSettingPropertyEditor()
{

}

void CEdDialogEditorSettingPropertyEditor::CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls )
{
	CBasePropItem* basePropertyItem = m_propertyItem;
	while ( basePropertyItem->GetParent() != NULL )
	{
		basePropertyItem = basePropertyItem->GetParent();
	}

	CStorySceneControlPart* section = basePropertyItem->GetParentObject( 0 ).As< CStorySceneControlPart >();

	if ( section == NULL )
	{
		return;
	}

	

	// Create editor
	wxSize size = propertyRect.GetSize();
	m_choice = new CEdChoice( m_propertyItem->GetPage(), propertyRect.GetTopLeft(), size );
	m_choice->SetWindowStyle( wxBORDER );
	m_choice->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );		
	m_choice->SetFocus();

	CStoryScene* scene = section->GetScene();
	
	TDynArray< CName > dialogsetNames;
	scene->GetDialogsetInstancesNames( dialogsetNames );

	m_choice->AppendString( CName::NONE.AsString().AsChar() );
	for ( Uint32 i = 0; i < dialogsetNames.Size(); ++i )
	{
		m_choice->AppendString( dialogsetNames[ i ].AsString().AsChar() );
	}


	// Set previous value
	CName currentValue;
	m_propertyItem->Read( &currentValue );
	m_choice->SetStringSelection( currentValue.AsString().AsChar() );

	// Notify of selection changes
	m_choice->Connect( wxEVT_COMMAND_CHOICE_SELECTED,
		wxCommandEventHandler( CEdDialogEditorSettingPropertyEditor::OnChoiceChanged ), NULL, this );
}

void CEdDialogEditorSettingPropertyEditor::OnChoiceChanged( wxCommandEvent& event )
{
	CName settingName( event.GetString() );

	m_propertyItem->Write( &settingName );
	m_propertyItem->SavePropertyValue();
	m_propertyItem->GrabPropertyValue();
}

void CEdDialogEditorSettingPropertyEditor::CloseControls()
{
	//ICustomPropertyEditor::OnCloseEditors();
	delete m_choice;
	m_choice = NULL;
}

Bool CEdDialogEditorSettingPropertyEditor::DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour )
{
	return ICustomPropertyEditor::DrawValue( dc, valueRect, textColour );
}

Bool CEdDialogEditorSettingPropertyEditor::SaveValue()
{
	return ICustomPropertyEditor::SaveValue();
}

Bool CEdDialogEditorSettingPropertyEditor::GrabValue( String& displayValue )
{
	return ICustomPropertyEditor::GrabValue( displayValue );
}





void CEdSceneInputSelector::FillChoices()
{
	CStoryScene* scene = m_propertyItem->GetParentObject( 0 ).As< CStorySceneSpawner >()->GetStoryScene();

	if ( scene == NULL )
	{
		return;
	}

	TDynArray< String > inputNames = scene->GetInputNames();
	for ( Uint32 i = 0; i < inputNames.Size(); ++i )
	{
		m_ctrlChoice->AppendString( inputNames[ i ].AsChar() );
	}
}
