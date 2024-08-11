/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dialogEditorDialogsetPanel.h"
#include "dialogPreview.h"
#include "dialogEditor.h"
#include "undoDialogEditor.h"

#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneDialogset.h"

BEGIN_EVENT_TABLE( CEdSceneEditorDialogsetPanel, wxPanel )
	EVT_LIST_ITEM_SELECTED( XRCID( "DialogsetList" ), CEdSceneEditorDialogsetPanel::OnDialogsetInstanceSelected )
	EVT_LIST_BEGIN_LABEL_EDIT( XRCID( "DialogsetList" ), CEdSceneEditorDialogsetPanel::OnDialogsetInstanceNameChangeBegin )
	EVT_LIST_END_LABEL_EDIT( XRCID( "DialogsetList" ), CEdSceneEditorDialogsetPanel::OnDialogsetInstanceNameChange )
	EVT_LISTBOX( XRCID( "SlotList" ), CEdSceneEditorDialogsetPanel::OnDialogsetSlotSelected )
	EVT_BUTTON( XRCID( "NewDialogsetButton" ), CEdSceneEditorDialogsetPanel::OnNewDialogset )
	EVT_BUTTON( XRCID( "ImportDialogsetButton" ), CEdSceneEditorDialogsetPanel::OnImportDialogset )
	EVT_BUTTON( XRCID( "RemoveDialogsetButton" ), CEdSceneEditorDialogsetPanel::OnRemoveDialogset )
	EVT_BUTTON( XRCID( "ReloadDialogsetButton" ), CEdSceneEditorDialogsetPanel::OnReloadDialogset )
	EVT_BUTTON( XRCID( "AddSlotButton" ), CEdSceneEditorDialogsetPanel::OnAddSlot )
	EVT_BUTTON( XRCID( "RemoveSlotButton" ), CEdSceneEditorDialogsetPanel::OnRemoveSlot )
	EVT_BUTTON( XRCID( "ExportDialogsetButton" ), CEdSceneEditorDialogsetPanel::OnExportDialogset )
	EVT_BUTTON( XRCID( "DuplicateDialogsetButton" ), CEdSceneEditorDialogsetPanel::OnDuplicateDialogset )
END_EVENT_TABLE()

enum EDialogsetColumn
{
	DialogsetColumn_Name = 0,

	DialogsetColumn_Max
};

enum EDialogsetImage
{
	DialogsetImage_Current = 0,
	DialogsetImage_Other,

	DialogsetImage_Max
};

RED_INLINE CName CEdSceneEditorDialogsetPanel::GetSelectedDialogsetInstanceName() const
{
	int selectedIndex = m_dialogsetList->GetFirstSelected();

	return CName( m_dialogsetList->GetItemText( selectedIndex, DialogsetColumn_Name ).wc_str() );
}

CEdSceneEditorDialogsetPanel::CEdSceneEditorDialogsetPanel( CEdSceneEditor* parentEditor )
	: m_sceneEditor( parentEditor )
	, m_mediator( parentEditor )
{
	wxXmlResource::Get()->LoadPanel( this, parentEditor, TEXT( "DialogDialogsetSettingsPanel" ) );

	m_dialogsetList = XRCCTRL( *this, "DialogsetList", wxListView );
	m_slotList = XRCCTRL( *this, "SlotList", wxListBox );

	wxBitmap currentlySelectedImage = SEdResources::GetInstance().LoadBitmap( wxT( "IMG_SELECTED" ) );
	wxBitmap otherImage = SEdResources::GetInstance().LoadBitmap( wxT( "IMG_NOT_SELECTED" ) );

	m_imageList.Create( 16, 16 );
	m_imageList.Add( currentlySelectedImage );
	m_imageList.Add( otherImage );

	m_dialogsetList->SetImageList( &m_imageList, wxIMAGE_LIST_NORMAL );
	m_dialogsetList->SetImageList( &m_imageList, wxIMAGE_LIST_SMALL );

	PropertiesPageSettings settings;
	wxPanel* dialogsetPropertiesPanel = XRCCTRL( *this, "DialogsetPropertyPanel", wxPanel );
	m_dialogsetPropertiesPage = new CEdPropertiesPage( dialogsetPropertiesPanel, settings, nullptr );
	dialogsetPropertiesPanel->GetSizer()->Add( m_dialogsetPropertiesPage, 1, wxEXPAND );
	dialogsetPropertiesPanel->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdSceneEditorDialogsetPanel::OnDialogsetPropertyChanged, this );

	wxPanel* slotPropertiesPanel = XRCCTRL( *this, "SlotPropertyPanel", wxPanel );
	m_slotPropertiesPage = new CEdPropertiesPage( slotPropertiesPanel, settings, nullptr );
	slotPropertiesPanel->GetSizer()->Add( m_slotPropertiesPage, 1, wxEXPAND );
	slotPropertiesPanel->Bind( wxEVT_COMMAND_PROPERTY_CHANGED, &CEdSceneEditorDialogsetPanel::OnSlotPropertyChanged, this );
}

void CEdSceneEditorDialogsetPanel::UpdateDialogsetList()
{
	CStoryScene* scene = m_sceneEditor->HACK_GetStoryScene();
	m_dialogsetList->Freeze();
	m_dialogsetList->ClearAll();
	m_slotList->Clear();
	m_dialogsetPropertiesPage->SetNoObject();
	m_slotPropertiesPage->SetNoObject();

	wxListItem itemCol;
	itemCol.SetText( wxT( "Dialogsets" ) );
	itemCol.SetImage( -1 );
	m_dialogsetList->InsertColumn( DialogsetColumn_Name, itemCol );

	CName currentDialogsetInstanceName;
	const CStorySceneDialogsetInstance* currentDialogsetInstance = m_sceneEditor->GetCurrentDialogsetInstance();
	if ( currentDialogsetInstance != NULL )
	{
		currentDialogsetInstanceName = currentDialogsetInstance->GetName();
	}

	TDynArray< CName > dialogsetNames;
	scene->GetDialogsetInstancesNames( dialogsetNames );

	for ( Uint32 i = 0; i < dialogsetNames.Size(); ++i )
	{
		wxString dialogsetInstanceDisplayName( dialogsetNames[ i ].AsString().AsChar() );

		long row = -1;
		row = m_dialogsetList->InsertItem( m_dialogsetList->GetItemCount(), wxEmptyString, DialogsetImage_Other );

		if ( dialogsetNames[ i ] == currentDialogsetInstanceName )
		{
			m_dialogsetList->SetItemImage( row, DialogsetImage_Current );
		}
		else
		{
			m_dialogsetList->SetItemImage( row, DialogsetImage_Other );
		}
		
		m_dialogsetList->SetItem( row, DialogsetColumn_Name, dialogsetNames[ i ].AsString().AsChar() );
		m_dialogsetList->SetItemPtrData( row, reinterpret_cast< wxUIntPtr >( scene->GetDialogsetByName( dialogsetNames[ i ] ) ) );
	}

	m_dialogsetList->SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
	m_dialogsetList->Thaw();
}

void CEdSceneEditorDialogsetPanel::UpdateDialogsetListNames()
{
	CStoryScene* scene = m_sceneEditor->HACK_GetStoryScene();
	TDynArray< CName > dialogsetNames;
	scene->GetDialogsetInstancesNames( dialogsetNames );

	CName currentDialogsetInstanceName;
	const CStorySceneDialogsetInstance* currentDialogsetInstance = m_sceneEditor->GetCurrentDialogsetInstance();
	if ( currentDialogsetInstance != NULL )
	{
		currentDialogsetInstanceName = currentDialogsetInstance->GetName();
	}
	m_dialogsetList->Freeze();

	for ( Uint32 i = 0; i < dialogsetNames.Size(); ++i )
	{
		long row = i;

		if ( i >= static_cast< Uint32 >( m_dialogsetList->GetItemCount() ) )
		{
			row = m_dialogsetList->InsertItem( m_dialogsetList->GetItemCount(), wxEmptyString, DialogsetImage_Other );
			m_dialogsetList->SetItemPtrData( row, reinterpret_cast< wxUIntPtr >( scene->GetDialogsetByName( dialogsetNames[ i ] ) ) );
		}

		if ( dialogsetNames[ i ] == currentDialogsetInstanceName )
		{
			m_dialogsetList->SetItemImage( row, DialogsetImage_Current );
		}
		else
		{
			m_dialogsetList->SetItemImage( row, DialogsetImage_Other );
		}

		m_dialogsetList->SetItem( row, DialogsetColumn_Name, dialogsetNames[ i ].AsString().AsChar() );
	}

	m_dialogsetList->Thaw();
}

void CEdSceneEditorDialogsetPanel::UpdateSlotList( CStorySceneDialogsetInstance* instance )
{
	if ( instance == NULL )
	{
		return;
	}
	
	m_slotList->Clear();
	m_slotPropertiesPage->SetNoObject();

	const TDynArray< CStorySceneDialogsetSlot* >& instanceSlots = instance->GetSlots();
	for ( Uint32 i = 0; i < instanceSlots.Size(); ++i )
	{
		ASSERT( instanceSlots[ i ] != NULL );
		m_slotList->AppendString( instanceSlots[ i ]->GetDescriptionString().AsChar() );
	}
}

void CEdSceneEditorDialogsetPanel::UpdateSlotListNames( CStorySceneDialogsetInstance* instance )
{
	if ( instance == NULL )
	{
		return;
	}

	const TDynArray< CStorySceneDialogsetSlot* >& instanceSlots = instance->GetSlots();
	for ( Uint32 i = 0; i < instanceSlots.Size(); ++i )
	{
		ASSERT( instanceSlots[ i ] != NULL );
		if ( i < m_slotList->GetCount() )
		{
			m_slotList->SetString( i, instanceSlots[ i ]->GetDescriptionString().AsChar() );
		}
		else
		{
			m_slotList->AppendString( instanceSlots[ i ]->GetDescriptionString().AsChar() );
		}
	}
}

void CEdSceneEditorDialogsetPanel::MarkCurrentDialogsetInstance( const CName& dialogsetInstanceName )
{
	//UpdateDialogsetListNames();
}

void CEdSceneEditorDialogsetPanel::OnDialogsetInstanceSelected( wxListEvent& event )
{
	CStorySceneDialogsetInstance* instance = m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( GetSelectedDialogsetInstanceName() );
	if ( instance != NULL )
	{
		m_dialogsetPropertiesPage->SetObject( instance );
		UpdateSlotList(instance);
	}
}

void CEdSceneEditorDialogsetPanel::OnDialogsetSlotSelected( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* instance = m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( GetSelectedDialogsetInstanceName() );
	if ( instance != NULL )
	{
		const TDynArray< CStorySceneDialogsetSlot* >& instanceSlots = instance->GetSlots();
		m_slotPropertiesPage->SetObject( instanceSlots[ event.GetInt() ] );
	}
}

void CEdSceneEditorDialogsetPanel::OnNewDialogset( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* dialogset = m_sceneEditor->CreateNewDialogset();
	UpdateDialogsetListNames();

	if( dialogset )
	{
		if ( m_undoManager )
		{
			CUndoDialogSetExistance::CreateCreationStep( *m_undoManager, this, dialogset );
		}

		long index = m_dialogsetList->FindItem( -1, dialogset->GetName().AsString().AsChar() );
		m_dialogsetList->Select( index );
	}
}

void CEdSceneEditorDialogsetPanel::OnImportDialogset( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* dialogset = m_sceneEditor->CreateDialogsetFromFile();
	UpdateDialogsetListNames();

	if( dialogset )
	{
		if ( m_undoManager )
		{
			CUndoDialogSetExistance::CreateCreationStep( *m_undoManager, this, dialogset );
		}

		long index = m_dialogsetList->FindItem( -1, dialogset->GetName().AsString().AsChar() );
		m_dialogsetList->Select( index );
	}

	m_dialogsetList->SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
}

void CEdSceneEditorDialogsetPanel::OnReloadDialogset( wxCommandEvent& event )
{
	wxString instanceString;
	wxString selectedItem = m_dialogsetList->GetItemText( m_dialogsetList->GetFirstSelected() );

	if ( selectedItem.StartsWith( wxT( "* " ), &instanceString ) == false )
	{
		instanceString = selectedItem;	
	}

	CName instanceName = CName( instanceString.c_str() );

	CStorySceneDialogsetInstance* dialogset = m_sceneEditor->ReloadDialogsetFromFile(instanceName);
	UpdateSlotList(dialogset);

	if( dialogset )
	{
		long index = m_dialogsetList->FindItem( -1, dialogset->GetName().AsString().AsChar() );
		m_dialogsetList->Select( index );
	}

	m_dialogsetList->SetColumnWidth( 0, wxLIST_AUTOSIZE_USEHEADER );
}

void CEdSceneEditorDialogsetPanel::OnRemoveDialogset( wxCommandEvent& event )
{
	wxString instanceString;
	wxString selectedItem = m_dialogsetList->GetItemText( m_dialogsetList->GetFirstSelected() );

	if ( selectedItem.StartsWith( wxT( "* " ), &instanceString ) == false )
	{
		instanceString = selectedItem;	
	}

	CName instanceName = CName( instanceString.c_str() );
	const CStoryScene* scene = m_mediator->OnDialogestPanel_GetScene();
	if( scene->IsDialogsetUsed( instanceName ) )
	{
		String msg( TXT("A number of sections in the scene are using this dialogset. Are you sure you want to delete it ?") );
		if( !GFeedback->AskYesNo( msg.AsChar() ) )
		{
			return;
		}
	}

	if ( m_undoManager )
	{
		if ( CStorySceneDialogsetInstance* dialogset = m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( instanceName ) )
		{
			CUndoDialogSetExistance::CreateDeletionStep( *m_undoManager, this, dialogset );
		}
	}

	m_sceneEditor->HACK_GetStoryScene()->RemoveDialogsetInstance( instanceName );
	UpdateDialogsetList();
}

void CEdSceneEditorDialogsetPanel::OnAddSlot( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* instance = m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( GetSelectedDialogsetInstanceName() );
	if ( instance != NULL )
	{
		CStorySceneDialogsetSlot* slot = ::CreateObject< CStorySceneDialogsetSlot >();

		if ( m_undoManager )
		{
			CUndoDialogSlotExistance::CreateCreationStep( *m_undoManager, this, instance, slot );
		}

		slot->OnCreatedInEditor();

		instance->AddSlot( slot );
		UpdateSlotListNames( instance );
	}
	
}

void CEdSceneEditorDialogsetPanel::OnRemoveSlot( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* instance = m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( GetSelectedDialogsetInstanceName() );
	if ( instance != NULL )
	{
		Uint32 slotIndex = m_slotList->GetSelection();
		if ( slotIndex < instance->GetSlots().Size() )
		{
			CStorySceneDialogsetSlot* slot = instance->GetSlots()[ slotIndex ];

			if ( m_undoManager )
			{
				CUndoDialogSlotExistance::CreateDeletionStep( *m_undoManager, this, instance, slot );
			}

			instance->RemoveSlot( slot->GetSlotName() );
			UpdateSlotList( instance );
		}
	}
}

void CEdSceneEditorDialogsetPanel::OnDialogsetPropertyChanged( wxCommandEvent& event )
{
	UpdateDialogsetListNames();

	m_mediator->OnDialogsetPanel_PropertyChanged();
}

void CEdSceneEditorDialogsetPanel::OnSlotPropertyChanged( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* choosenDialogsetInstance = m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( GetSelectedDialogsetInstanceName() );
	//m_sceneEditor->GetScenePreview()->RefreshPositions();

	m_mediator->OnDialogsetPanel_SlotPropertyChanged();

	UpdateSlotListNames( choosenDialogsetInstance );
}

void CEdSceneEditorDialogsetPanel::OnDuplicateDialogset( wxCommandEvent& event )
{
	CStorySceneDialogsetInstance* choosenDialogsetInstance 
		= m_sceneEditor->HACK_GetStoryScene()->GetDialogsetByName( GetSelectedDialogsetInstanceName() );
	
	if ( CStorySceneDialogsetInstance* dialogset = m_sceneEditor->CreateDialogsetCopy( choosenDialogsetInstance ) )
	{
		if ( m_undoManager )
		{
			CUndoDialogSetExistance::CreateCreationStep( *m_undoManager, this, dialogset );
		}

		UpdateDialogsetList();
	}
}

void CEdSceneEditorDialogsetPanel::OnExportDialogset( wxCommandEvent& event )
{
	m_sceneEditor->SaveDialogsetToFile( GetSelectedDialogsetInstanceName() );
}

void CEdSceneEditorDialogsetPanel::OnDialogsetInstanceNameChangeBegin( wxListEvent& event )
{
	CStorySceneDialogsetInstance* dialogsetInstance = reinterpret_cast< CStorySceneDialogsetInstance* >( m_dialogsetList->GetItemData( event.GetIndex() ) );
	ASSERT( dialogsetInstance != NULL );

	if( !dialogsetInstance->CanModify() )
	{
		event.Veto();
	}
}

void CEdSceneEditorDialogsetPanel::OnDialogsetInstanceNameChange( wxListEvent& event )
{
	CStorySceneDialogsetInstance* dialogsetInstance = reinterpret_cast< CStorySceneDialogsetInstance* >( m_dialogsetList->GetItemData( event.GetIndex() ) );
	ASSERT( dialogsetInstance != NULL );

	dialogsetInstance->SetName( CName( event.GetLabel().wc_str() ) );

	m_dialogsetPropertiesPage->RefreshValues();
}

void CEdSceneEditorDialogsetPanel::SetUndoManager( CEdUndoManager* undoManager )
{
	m_undoManager = undoManager;
	m_dialogsetPropertiesPage->SetUndoManager( m_undoManager );
	m_slotPropertiesPage->SetUndoManager( m_undoManager );
}