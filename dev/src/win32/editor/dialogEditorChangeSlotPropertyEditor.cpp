#include "build.h"
#include "dialogEditorChangeSlotPropertyEditor.h"

#include "dialogEditor.h"
#include "dialogPreview.h"

#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneEventCustomCamera.h"
#include "sceneDialogsetEditor.h"

CEdDialogEditorChangeSlotPropertyEditor::CEdDialogEditorChangeSlotPropertyEditor( CPropertyItem* propertyItem )
:	ISelectionEditor( propertyItem )
{
	if( propertyItem->GetName() == TXT( "sourceSlotName" ) )
	{
		m_isTargetSlot = false;
	}
	else if( propertyItem->GetName() == TXT( "targetSlotName" ) )
	{
		m_isTargetSlot = true;
	}
	else
	{
		ASSERT( false && "Unknown slot type" );
	}
}

void CEdDialogEditorChangeSlotPropertyEditor::FillChoices()
{
	CEdSceneEditor* sceneEditor = CEdSceneEditor::RetrieveSceneEditorObject( m_propertyItem );
	m_ctrlChoice->AppendString( wxT( "None" ) );
	const TDynArray< CStorySceneDialogsetSlot* >* slots = nullptr;
	
	if( sceneEditor )
	{
		const CStorySceneDialogsetInstance* dialogSetInstance = sceneEditor->GetCurrentDialogsetInstance();
		if ( dialogSetInstance )
		{
			slots = &dialogSetInstance->GetSlots();
		}		
	}
	else // dialogset editor
	{
		wxWindow* dialogsetEditorWindow = m_propertyItem->GetPage();
		while ( dialogsetEditorWindow != NULL && dialogsetEditorWindow->IsKindOf( wxCLASSINFO( CEdSceneDialogsetEditor ) ) == false )
		{
			dialogsetEditorWindow = dialogsetEditorWindow->GetParent();
		}	
		CEdSceneDialogsetEditor* dialogsetEdit =  static_cast<CEdSceneDialogsetEditor*>( dialogsetEditorWindow );
		if ( dialogsetEdit )
		{
			slots = &dialogsetEdit->GetDialogset()->GetSlots();
		}			
	}

	if( slots )
	{
		CName selectedSlot;
		m_propertyItem->Read( &selectedSlot );

		for( Uint32 i = 0; i < slots->Size(); ++i )
		{
			m_ctrlChoice->AppendString( (*slots)[ i ]->GetSlotName().AsString().AsChar() );

			if( (*slots)[ i ]->GetSlotName() == selectedSlot )
			{
				m_ctrlChoice->SetSelection( i );
			}
		}
	}	
}

void CEdDialogEditorChangeSlotPropertyEditor::OnChoiceChanged( wxCommandEvent &event )
{
	CName oldSlot;
	m_propertyItem->Read( &oldSlot );

	int selectedSlot = event.GetInt();

	CName newSlot;
	if( selectedSlot != 0 )
	{
		newSlot = CName( String( m_ctrlChoice->GetString( event.GetInt() ) ) );
	}

	m_propertyItem->Write( &newSlot );

	LOG_EDITOR( TXT( "Old Slot: %s, New Slot: %s" ), oldSlot.AsString().AsChar(), newSlot.AsString().AsChar() );
}
