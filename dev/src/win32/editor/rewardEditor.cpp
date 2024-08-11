/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "rewardEditor.h"

#include "../../common/core/factory.h"
#include "../../common/core/versionControl.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

IMPLEMENT_ENGINE_CLASS( CRewardResourceManager );

// Images ID's
#define IMAGE_DIR_OPENED		0
#define	IMAGE_DIR_CLOSED		1
#define IMAGE_GROUP				2
#define	IMAGE_GROUP_CHECKEDOUT	3
#define	IMAGE_REWARD			4

// Popup menu ID's
#define ID_ADD_GROUP			5000
#define ID_ADD_REWARD			5001
#define ID_ADD_DIRECTORY		5002

#define ID_DELETE				5003
#define ID_ADD					5004
#define ID_RENAME				5005
#define ID_SAVE					5006
#define ID_REVERT				5007
#define ID_SUBMIT				5008
#define ID_HISTORY				5009
#define ID_CHECKOUT				5010
#define ID_SYNC					5011
#define ID_DUPLICATE			5012

#define SHOW_ERROR( msg )		wxMessageDialog dialog( 0, msg, wxT("Error"), wxOK | wxICON_ERROR );	\
								dialog.ShowModal();

#define SHOW_WARNING( msg )		wxMessageDialog dialog( 0, msg, wxT("Warning"), wxOK | wxICON_WARNING );	\
								dialog.ShowModal();

DEFINE_EVENT_TYPE( wxEVT_CHOOSE_REWARD_OK )
DEFINE_EVENT_TYPE( wxEVT_CHOOSE_REWARD_CANCEL )

BEGIN_EVENT_TABLE( CEdRewardEditor, wxFrame )
	EVT_CLOSE( CEdRewardEditor::OnExit )
	EVT_TREE_KEY_DOWN( XRCID( "m_rewardTree" ), CEdRewardEditor::OnKeyDown )
	EVT_BUTTON( XRCID( "m_okButton" ), CEdRewardEditor::OnOK )
	EVT_BUTTON( XRCID( "m_cancelButton" ), CEdRewardEditor::OnCancel )
	EVT_TREE_ITEM_EXPANDING( XRCID( "m_rewardTree"), CEdRewardEditor::OnItemExpanding )
	EVT_TREE_ITEM_COLLAPSED( XRCID( "m_rewardTree"), CEdRewardEditor::OnItemCollapsed )
	EVT_TREE_ITEM_RIGHT_CLICK( XRCID( "m_rewardTree"), CEdRewardEditor::OnItemRightClick )
	EVT_TREE_SEL_CHANGED( XRCID( "m_rewardTree"), CEdRewardEditor::OnSelectionChanged )
	EVT_TREE_BEGIN_LABEL_EDIT( XRCID( "m_rewardTree"), CEdRewardEditor::OnLabelEdit )
	EVT_TREE_END_LABEL_EDIT( XRCID( "m_rewardTree"), CEdRewardEditor::OnFinishedLabelEdit )
	EVT_TREE_BEGIN_DRAG( XRCID( "m_rewardTree"), CEdRewardEditor::OnBeginDrag )
	EVT_TREE_END_DRAG( XRCID( "m_rewardTree"), CEdRewardEditor::OnEndDrag )
END_EVENT_TABLE()


CEdRewardEditor::CEdRewardEditor( wxWindow* parent, Bool chooseMode, CName currentReward )
	: m_rewardTree( NULL )
	, m_nameLabel( NULL )
	, m_propertiesBrowser( NULL )
{
	// Load designed dialog from resource
	wxXmlResource::Get()->LoadFrame( this, parent, TEXT("RewardEditor") );

	if( chooseMode )
	{
		wxButton* button = XRCCTRL( *this, "m_okButton", wxButton );
		button->Show();
		button = XRCCTRL( *this, "m_cancelButton", wxButton );
		button->Show();
	}

	// Get controls from resource
	m_rewardTree = XRCCTRL( *this, "m_rewardTree", wxTreeCtrl );
	m_nameLabel = XRCCTRL( *this, "rewardName", wxStaticText );

	// Assign image list to tree
	wxImageList* images = new wxImageList( 16, 16, true, 5 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_OPENED") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_DIR_CLOSED") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHEST_GREY") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CHEST") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_AWARD") ) );
	m_rewardTree->AssignImageList( images );

	// Add root node
	CDirectory* dir = GDepot->FindPath( REWARDS_DIR );
	if( !dir )
	{
		dir = GDepot->CreatePath( REWARDS_DIR );
	}
	m_rewardTree->AddRoot( TXT("All Rewards"), 1, -1, new CRewardTreeData( dir ) );

	// Create properties browser
	PropertiesPageSettings settings;
	wxPanel* propertiesPanel = XRCCTRL( *this, "propsPanel", wxPanel );

	m_propertiesBrowser = new CEdPropertiesPage( propertiesPanel, settings, nullptr );
	m_propertiesBrowser->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdRewardEditor::OnPropertiesChanged ), NULL, this );
	wxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	sizer->Add( m_propertiesBrowser, 1, wxEXPAND | wxALL );

	m_htmlSummary = new CRewardsSummary( propertiesPanel );
	sizer->Add( m_htmlSummary, 1, wxEXPAND | wxALL );
	m_htmlSummary->Hide();

	propertiesPanel->SetSizer( sizer );

	m_resourceManager = CreateObject<CRewardResourceManager>();
	m_resourceManager->AddToRootSet();

	Layout();
	LoadOptionsFromConfig();
	LoadAll( currentReward );

	m_rewardTree->Expand( m_rewardTree->GetRootItem() );
}

CEdRewardEditor::~CEdRewardEditor()
{
	SaveAll();
	SaveOptionsToConfig();

	m_resourceManager->RemoveFromRootSet();
	m_resourceManager->Discard();
}

void CEdRewardEditor::SaveOptionsToConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* splitter = XRCCTRL( *this, "Splitter", wxSplitterWindow );

	config.Write( TXT("/Frames/RewardEditor/SplitterPosition"), splitter->GetSashPosition() );
	config.Write( TXT("/Frames/RewardEditor/FrameWidth"), GetSize().GetWidth() );
	config.Write( TXT("/Frames/RewardEditor/FrameHeight"), GetSize().GetHeight() );
}

void CEdRewardEditor::LoadOptionsFromConfig()
{
	CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();

	wxSplitterWindow* splitter = XRCCTRL( *this, "Splitter", wxSplitterWindow );

	const int pos = config.Read( TXT("/Frames/RewardEditor/SplitterPosition"), 240 );
	splitter->SetSashPosition( pos );

	const wxSize size( config.Read( TXT("/Frames/RewardEditor/FrameWidth"), 650 ),
						config.Read( TXT("/Frames/RewardEditor/FrameHeight"), 400 ) );
	SetSize( size );
}

void CEdRewardEditor::OnKeyDown( wxTreeEvent& event )
{
	if ( event.GetKeyCode() == WXK_DELETE )
	{
		const wxTreeItemId item = m_rewardTree->GetSelection();
		const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

		if( data )
		{
			wxCommandEvent event1;
			if( data->m_dataType == RTDT_Reward )
			{
				OnDeleteReward(event1);
			}
			else if( data->m_dataType == RTDT_Group )
			{
				OnDeleteGroup(event1);
			}
			else if( data->m_dataType == RTDT_Directory )
			{
				OnDeleteDirectory(event1);
			}
		}
	}	
	if ( event.GetKeyCode() == WXK_F2 )
	{
		const wxTreeItemId item = m_rewardTree->GetSelection();
		const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

		if( data && data->m_dataType == RTDT_Reward )
		{
			wxCommandEvent ev;
			EditLabel(ev);
		}
	}
}

void CEdRewardEditor::OnItemExpanding( wxTreeEvent& event )
{
	const wxTreeItemId item = event.GetItem();	
	CRewardTreeData* data = static_cast<CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	if ( data && data->m_dataType == RTDT_Directory )
	{
		m_rewardTree->SetItemImage( item, IMAGE_DIR_OPENED );
	}
}

void CEdRewardEditor::OnItemCollapsed( wxTreeEvent& event )
{
	const wxTreeItemId item = event.GetItem();	
	CRewardTreeData* data = static_cast<CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	if ( data && data->m_dataType == RTDT_Directory )
	{
		m_rewardTree->SetItemImage( item, IMAGE_DIR_CLOSED );
	}
}

void CEdRewardEditor::OnItemRightClick( wxTreeEvent& event )
{
	wxTreeItemId item = event.GetItem();	

	m_rewardTree->SelectItem( item, true );

	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	// If it's a group or directory, popup context menu
	if( data->m_dataType == RTDT_Directory )
	{
		// Build context menu
		wxMenu menu;
		menu.Append( ID_ADD_GROUP, TEXT("Create new group"), wxEmptyString );
		menu.Connect( ID_ADD_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnAddGroup ), NULL, this );
		menu.Append( ID_ADD_DIRECTORY, TEXT("Create new directory"), wxEmptyString );
		menu.Connect( ID_ADD_DIRECTORY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnAddDirectory ), NULL, this );
		menu.Append( ID_DELETE, TEXT("Delete"), wxEmptyString );
		menu.Connect( ID_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnDeleteDirectory ), NULL, this );

		// Show menu
		PopupMenu( &menu );
	}
	else if( data->m_dataType == RTDT_Group )
	{
		CDiskFile* file = data->m_group->GetFile();

		file->GetStatus();

		// Build context menu
		wxMenu menu;
		menu.Append( ID_ADD_GROUP, TEXT("Create new group"), wxEmptyString );
		menu.Connect( ID_ADD_GROUP, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnAddGroup ), NULL, this );
		menu.Append( ID_ADD_REWARD, TEXT("Create new reward"), wxEmptyString );
		menu.Connect( ID_ADD_REWARD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnAddReward ), NULL, this );
		menu.AppendSeparator();
		if( file->IsLocal() )
		{
			menu.Append( ID_ADD, TEXT("Add"), wxEmptyString );
			menu.Connect( ID_ADD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnAdd ), NULL, this );
		}
		else
		{
			if( file->IsEdited() )
			{
				menu.Append( ID_REVERT, TEXT("Revert"), wxEmptyString );
				menu.Connect( ID_REVERT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnRevert ), NULL, this );
				menu.Append( ID_SUBMIT, TEXT("Submit"), wxEmptyString );
				menu.Connect( ID_SUBMIT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnSubmit ), NULL, this );
			}
			else if( file->IsCheckedIn() )
			{
				menu.Append( ID_CHECKOUT, TEXT("Checkout"), wxEmptyString );
				menu.Connect( ID_CHECKOUT, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnCheckout ), NULL, this );
			}
			else if( file->IsNotSynced() )
			{
				menu.Append( ID_SYNC, TEXT("Sync"), wxEmptyString );
				menu.Connect( ID_SYNC, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnSync ), NULL, this );
			}
			menu.Append( ID_HISTORY, TEXT("File's history"), wxEmptyString );
			menu.Connect( ID_HISTORY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnHistory ), NULL, this );
		}
		menu.Append( ID_DELETE, TEXT("Delete"), wxEmptyString );
		menu.Connect( ID_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnDeleteGroup ), NULL, this );
		menu.Append( ID_RENAME, TEXT("Rename"), wxEmptyString );
		menu.Connect( ID_RENAME, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnRename ), NULL, this );
		menu.Append( ID_SAVE, TEXT("Save"), wxEmptyString );
		menu.Connect( ID_SAVE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnSave ), NULL, this );

		// Show menu
		PopupMenu( &menu );
	}
	if( data->m_dataType == RTDT_Reward )
	{
		// Build context menu
		wxMenu menu;
		menu.Append( ID_DUPLICATE, TEXT("Duplicate"), wxEmptyString );
		menu.Connect( ID_DUPLICATE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnDuplicateReward ), NULL, this );

		menu.AppendSeparator();

		menu.Append( ID_DELETE, TEXT("Delete"), wxEmptyString );
		menu.Connect( ID_DELETE, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::OnDeleteReward ), NULL, this );

		menu.Append( ID_RENAME, TEXT("Rename"), wxEmptyString );
		menu.Connect(ID_RENAME , wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler( CEdRewardEditor::EditLabel ), NULL, this );


		// Show menu
		PopupMenu( &menu );
	}
}

void CEdRewardEditor::OnSelectionChanged( wxTreeEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	m_nameLabel->SetLabel( m_rewardTree->GetItemText( item ) );

	if( data )
	{
		wxButton* button = XRCCTRL( *this, "m_okButton", wxButton );

		if( data->m_dataType == RTDT_Reward )
		{
			button->Enable();

			m_htmlSummary->Hide();
			m_propertiesBrowser->SetObject( GetRewardByName(data->m_reward) );
			m_propertiesBrowser->Show();
		}
		else if( data->m_dataType == RTDT_Group )
		{
			button->Disable();

			m_propertiesBrowser->Hide();
			m_htmlSummary->CreateSummary( CRewardIterator( *data->m_group ) );
			m_htmlSummary->Show();
		}
		else if( data->m_dataType == RTDT_Directory )
		{
			button->Disable();

			m_propertiesBrowser->Hide();
			m_htmlSummary->CreateSummary( CRewardIterator( *data->m_directory ) );
			m_htmlSummary->Show();
		}
	}
	else
	{
		m_propertiesBrowser->SetNoObject();
	}
}

void CEdRewardEditor::EditLabel(wxCommandEvent& event)
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	m_rewardTree->EditLabel(item);
}

void CEdRewardEditor::OnLabelEdit( wxTreeEvent& event )
{
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( event.GetItem() ) );
	if( !data || data->m_dataType != RTDT_Reward )
	{
		event.Veto();
	}
}

void CEdRewardEditor::OnFinishedLabelEdit( wxTreeEvent& event )
{
	CRewardTreeData* data = static_cast<CRewardTreeData*>( m_rewardTree->GetItemData( event.GetItem() ) );

	ASSERT( data && data->m_dataType == RTDT_Reward );

	if( data )
	{
		SReward * rew = GetRewardByName(data->m_reward);
		if( rew && rew->m_group->MarkModified() )
		{
			CName name = CName( event.GetLabel() );
			if( GetRewardByName(name) == NULL )
			{
				rew->m_name = name;
				data->m_reward = name;
				m_nameLabel->SetLabel( event.GetLabel() );
			}
			else
			{
				GFeedback->ShowError(TXT("Reward with this name already exists \n name must be unique"));
				event.Veto();
			}
		}
		else
		{
			event.Veto();
		}
	}
}

void CEdRewardEditor::OnBeginDrag( wxTreeEvent& event )
{
	const wxTreeItemId draggedItem = event.GetItem();

	if ( CanItemBeDragged( draggedItem ) )
	{
		m_draggedItem = draggedItem;
		event.Allow();
	}
}

void CEdRewardEditor::OnEndDrag( wxTreeEvent& event )
{
	const wxTreeItemId droppedItem = event.GetItem();

	const wxTreeItemId draggedParentItem		= m_rewardTree->GetItemParent( m_draggedItem );
	const wxTreeItemId droppedParentItem		= m_rewardTree->GetItemParent( droppedItem );
	const CRewardTreeData* draggedData			= static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( m_draggedItem ) );
	const CRewardTreeData* droppedData			= static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( droppedItem ) );
	const CRewardTreeData* draggedParentData	= static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( draggedParentItem ) );
	const CRewardTreeData* droppedParentData	= static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( droppedParentItem ) );

	if ( draggedParentData && droppedData && draggedData && droppedData )
	{
		// we're dragging a reward
		if ( draggedData->m_dataType == RTDT_Reward )
		{
			// same parent
			if ( draggedParentItem == droppedParentItem )
			{
				if ( draggedParentData->m_group )
				{
					if ( draggedParentData->m_group->MoveReward( draggedData->m_reward, nullptr, droppedData->m_reward ) )
					{
						RepopulateGroup( draggedParentItem, draggedParentData->m_group, draggedData->m_reward );
					}
				}
			}
			// different parent
			else
			{
				// dropped on a reward, we need to take parent of this reward
				if ( droppedData->m_dataType == RTDT_Reward )
				{
					CName rewardToSelect = draggedData->m_reward;
					if ( draggedParentData->m_group->MoveReward( draggedData->m_reward, droppedParentData->m_group, droppedData->m_reward ) )
					{
						RepopulateGroup( draggedParentItem, draggedParentData->m_group, CName::NONE );
						RepopulateGroup( droppedParentItem, droppedParentData->m_group, rewardToSelect );
					}
				}
				// dropped on a group, we just need to take group
				if ( droppedData->m_dataType == RTDT_Group )
				{
					CName rewardToSelect = draggedData->m_reward;
					if ( draggedParentData->m_group->MoveReward( draggedData->m_reward, droppedData->m_group, CName::NONE ) )
					{
						RepopulateGroup( draggedParentItem, draggedParentData->m_group, CName::NONE );
						RepopulateGroup( droppedItem,       droppedData->m_group,       rewardToSelect );
					}
				}
			}
		}
	}
}

Bool CEdRewardEditor::CanItemBeDragged( const wxTreeItemId& item )
{
	const CRewardTreeData* draggedData = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );
	if ( draggedData )
	{
		if ( draggedData->m_dataType == RTDT_Reward )
		{
			return true;
		}
	}
	return false;
}

void CEdRewardEditor::OnAddGroup( wxCommandEvent& event )
{
	const wxTreeItemId parentItem = m_rewardTree->GetSelection();

	m_rewardTree->Expand( parentItem );

	const CRewardTreeData* parentData = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( parentItem ) );

	ASSERT( parentData && parentData->m_dataType != RTDT_Reward && parentData->m_directory );

	CDirectory* dir = NULL;
	CRewardGroup* parent = NULL;
	if( parentData->m_dataType == RTDT_Directory )
	{
		dir = parentData->m_directory;
	}
	else
	{
		if( parentData->m_group->MarkModified() )
		{
			m_rewardTree->SetItemImage( parentItem, IMAGE_GROUP_CHECKEDOUT );
			parent = parentData->m_group;
			dir = parent->GetFile()->GetDirectory();
		}
		else
		{
			SHOW_ERROR( TXT("Cannot create a subgroup of an uncheckedout group resource.") );
			return;
		}
	}

	ASSERT( dir );
	CRewardGroup* group = CreateResource( dir, parent );
	if( group )
	{
		const wxTreeItemId item = m_rewardTree->AppendItem( parentItem, group->GetFile()->GetFileName().StringBefore( TXT(".") ).AsChar(), IMAGE_GROUP_CHECKEDOUT );

		m_rewardTree->SetItemData( item, new CRewardTreeData( group ) );

		m_rewardTree->SelectItem( item );
	}
}

void CEdRewardEditor::OnAddReward( wxCommandEvent& event )
{
	String name;
	if( !InputBox( this, TXT("New Reward"), TXT("Enter new reward name:"), name ) )
	{
		return;
	}
	CName  cname(name);
	if( GetRewardByName(cname) != NULL )
	{
		GFeedback->ShowError(TXT("Reward with this name already exists \n name must be unique"));
		return;
	}

	const wxTreeItemId groupItem = m_rewardTree->GetSelection();
	const CRewardTreeData* groupData = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( groupItem ) );
	ASSERT( groupData && groupData->m_dataType == RTDT_Group && groupData->m_group );

	SReward* newReward = groupData->m_group->CreateReward();

	if( newReward )
	{
		newReward->m_name = cname;

		m_rewardTree->SetItemImage( groupItem, IMAGE_GROUP_CHECKEDOUT );

		const wxTreeItemId item = m_rewardTree->AppendItem( groupItem, name.AsChar(), IMAGE_REWARD );

		m_rewardTree->Expand( groupItem );

		m_rewardTree->SetItemData( item, new CRewardTreeData( newReward->m_name ) );

		m_rewardTree->SelectItem( item );
	}
}

void CEdRewardEditor::OnDuplicateReward( wxCommandEvent& event )
{
	const wxTreeItemId selectedItem = m_rewardTree->GetSelection();
	const wxTreeItemId parentItem   = m_rewardTree->GetItemParent( selectedItem );

	const CRewardTreeData* selectedItemData = static_cast< const CRewardTreeData* >( m_rewardTree->GetItemData( selectedItem ) );
	const CRewardTreeData* parentItemData   = static_cast< const CRewardTreeData* >( m_rewardTree->GetItemData( parentItem ) );
	ASSERT( selectedItemData && selectedItemData->m_dataType == RTDT_Reward );
	ASSERT( parentItemData &&   parentItemData->m_dataType ==   RTDT_Group );

	SReward* existingReward = GetRewardByName( selectedItemData->m_reward );
	if ( existingReward )
	{
		SReward* newReward = parentItemData->m_group->CreateReward( existingReward );
		if( newReward )
		{
			newReward->m_name = CName( existingReward->m_name.AsString() + TXT(" Copy") );

			m_rewardTree->SetItemImage( parentItem, IMAGE_GROUP_CHECKEDOUT );
			const wxTreeItemId item = m_rewardTree->AppendItem( parentItem, newReward->m_name.AsChar(), IMAGE_REWARD );
			m_rewardTree->Expand( parentItem );
			m_rewardTree->SetItemData( item, new CRewardTreeData( newReward->m_name ) );
			m_rewardTree->SelectItem( item );
		}
	}
}

void CEdRewardEditor::OnDeleteReward( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );
	ASSERT( data && data->m_dataType == RTDT_Reward );

	SReward * rew = GetRewardByName(data->m_reward);
	if(rew)
	{
		rew->m_group->RemoveReward(rew);
		m_rewardTree->Delete(item);	
	}


}

CRewardGroup* CEdRewardEditor::CreateResource( CDirectory* dir, CRewardGroup* parent )
{
	// Get suitable factory class for this resource
	IFactory* factory = IFactory::FindFactory( CRewardGroup::GetStaticClass() );
	if ( !factory )
	{
		WARN_EDITOR( TXT("No valid factory for resource '%s'"), CRewardGroup::GetStaticClass()->GetName().AsString().AsChar() );
		return NULL;
	}

	CRewardGroup* created = NULL;

	CEdImportDlg importDlg( this, CRewardGroup::GetStaticClass(), dir, String::EMPTY, factory );
	if ( importDlg.DoModal() == ECR_OK )
	{
		String fileName = importDlg.GetFileName(); 

		if ( !fileName.ContainsCharacter( TXT( ' ' ) )  && (fileName.ToLower() == fileName ) ) 
		{
			IFactory::FactoryOptions options;
			options.m_parentObject = NULL;

			// Create resource
			created = Cast<CRewardGroup>( factory->DoCreate( options ) );
			if ( created )
			{
				if( parent )
				{
					created->m_isSubGroup = true;
				}

				// Save this resource for the first time, this will also create thumbnail
				if( !created->SaveAs( dir, fileName ) )
				{
					created->Discard();
					created = NULL;

					// Report error
					SHOW_ERROR( String::Printf( TXT("Unable to create file '%s'"), fileName.AsChar() ).AsChar() );
				}
				else if( parent )
				{
					// Add to parent only after successful initialization
					parent->m_subGroups.PushBack( created );
				}
				else
				{
					// If no parent add to resource manager to prevent garbage collecting
					m_resourceManager->AddRewardGroup( created );
				}

				wxTheFrame->GetAssetBrowser()->UpdateResourceList();
			}
			else
			{
				// Report error
				SHOW_ERROR( String::Printf( TXT("Unable to create '%s'"), CRewardGroup::GetStaticClass()->GetName().AsString().AsChar() ).AsChar() );
			}
		}
		else
		{
			SHOW_ERROR( TXT("Cannot create file with whitespaces or large letters") );
		}
	}

	return created;
}

void CEdRewardEditor::RepopulateGroup( const wxTreeItemId& item, CRewardGroup* group, const CName& selectName )
{
	m_rewardTree->DeleteChildren( item );

	LoadGroup( *group, item, selectName );
}

void CEdRewardEditor::OnDeleteDirectory( wxCommandEvent& event )
{

}

void CEdRewardEditor::OnAddDirectory( wxCommandEvent& event )
{
	String name;
	if( !InputBox( this, TXT("New Directory"), TXT("Enter new directory name:"), name ) )
	{
		return;
	}

	const wxTreeItemId parentItem = m_rewardTree->GetSelection();

	const CRewardTreeData* parentData = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( parentItem ) );

	ASSERT( parentData && parentData->m_dataType == RTDT_Directory && parentData->m_directory );

	if( !parentData->m_directory->FindPath( name.AsChar() ) )
	{
		CDirectory* dir = parentData->m_directory->CreateNewDirectory( name );
		ASSERT( dir );
		if( dir->CreateOnDisk() )
		{
			const wxTreeItemId item = m_rewardTree->AppendItem( parentItem, name.AsChar(), IMAGE_DIR_CLOSED );
			m_rewardTree->SetItemData( item, new CRewardTreeData( dir ) );

			m_rewardTree->SelectItem( item );

			wxTheFrame->GetAssetBrowser()->UpdateResourceList();
		}
		else
		{
			SHOW_ERROR( String::Printf( TXT("Unable to create directory \"%s\"."), name.AsChar() ).AsChar() );
		}
	}
	else
	{
		SHOW_ERROR( String::Printf( TXT("Directory \"%s\" already exists."), name.AsChar() ).AsChar() );
	}
	
}

void CEdRewardEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	if( data )
	{
		if( data->m_dataType == RTDT_Reward )
		{
			ASSERT( data->m_reward );
			SReward * rew = GetRewardByName(data->m_reward);
			if( !rew || !rew->m_group->MarkModified() )
			{
				SHOW_WARNING( TXT("The file you are trying to edit has not been checked out, your changes will NOT be saved!\nYou have been warned, have a nice day.") );
			}
		}
	}
}

void CEdRewardEditor::SaveAll()
{
	const wxTreeItemId item = m_rewardTree->GetRootItem();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Directory );

	if( data && data->m_directory )
	{
		for( CRewardDirectoryIterator it( *data->m_directory, true ); it; ++it )
		{
			if( (*it) && (*it)->IsModified() )
			{
				(*it)->Save();
			}
		}
	}
}

void CEdRewardEditor::LoadRewards( CRewardGroup& group, const wxTreeItemId& item, CName selectReward )
{
	TDynArray< SReward >::iterator end = group.m_rewards.End();
	for( TDynArray< SReward >::iterator it = group.m_rewards.Begin(); it != end; ++it )
	{
		it->m_group = &group;
		const wxTreeItemId newReward = m_rewardTree->AppendItem( item, it->m_name.AsString().AsChar(), IMAGE_REWARD, -1, new CRewardTreeData( it->m_name ) );
		if( selectReward == it->m_name )
		{
			m_rewardTree->SelectItem( newReward );
		}
	}
}

void CEdRewardEditor::LoadGroup( CRewardGroup& group, const wxTreeItemId& item, CName selectReward )
{
	for( CRewardGroupIterator it( group ); it; ++it )
	{
		if( (*it)->GetFile() )
		{
			(*it)->GetFile()->GetStatus();
			const wxTreeItemId newItem = m_rewardTree->AppendItem( item, (*it)->GetFile()->GetFileName().StringBefore( TXT(".") ).AsChar(),
				(*it)->GetFile()->IsLocal() || (*it)->GetFile()->IsCheckedOut() ? IMAGE_GROUP_CHECKEDOUT : IMAGE_GROUP, -1, new CRewardTreeData( *it ) );

			LoadGroup( **it, newItem, selectReward );
		}
	}

	LoadRewards( group, item, selectReward );
}

void CEdRewardEditor::LoadDirectory( CDirectory& dir, const wxTreeItemId& item, CName selectReward )
{
	for ( CDirectory* newDir : dir.GetDirectories() )
	{
		const wxTreeItemId newItem = m_rewardTree->AppendItem( item, newDir->GetName().AsChar(), IMAGE_DIR_CLOSED, -1, new CRewardTreeData( newDir ) );
		LoadDirectory( *newDir, newItem, selectReward );
	}

	for( CRewardDirectoryIterator it( dir ); it; ++it )
	{
		// Do not load subgroups here, owner group will do that
		if( !(*it)->IsSubGroup() )
		{
			// Add to resource manager to prevent garbage collecting
			m_resourceManager->AddRewardGroup( *it );
			(*it)->GetFile()->GetStatus();
			const wxTreeItemId newItem = m_rewardTree->AppendItem( item, (*it)->GetFile()->GetFileName().StringBefore( TXT(".") ).AsChar(),
				(*it)->GetFile()->IsLocal() || (*it)->GetFile()->IsCheckedOut() ? IMAGE_GROUP_CHECKEDOUT : IMAGE_GROUP, -1, new CRewardTreeData( *it ) );

			LoadGroup( **it, newItem, selectReward );
		}
	}
}

void CEdRewardEditor::LoadAll( CName selectReward )
{
	const wxTreeItemId item = m_rewardTree->GetRootItem();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Directory );

	if( data && data->m_directory )
	{
		LoadDirectory( *data->m_directory, item, selectReward );
	}
}

void CEdRewardEditor::OnDeleteGroup( wxCommandEvent& event )
{
	const wxTreeItemId selectedItem = m_rewardTree->GetSelection();
	const CRewardTreeData* selectedData = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( selectedItem ) );

	ASSERT( selectedData && selectedData->m_dataType == RTDT_Group );

	if( selectedData )
	{
		CRewardGroup* selectedRewardGroup = selectedData->m_group;
		if ( selectedRewardGroup )
		{
			CDiskFile* f = selectedRewardGroup->GetFile();

			String message = String::Printf( TXT("You are going to delete \'%s\' file.\n\nAre you sure you want to continue?\n"), f->GetFileName().AsChar() );
			String caption = TXT("Delete file");
			if ( wxMessageBox( message.AsChar(), caption.AsChar(), wxYES_NO | wxNO_DEFAULT | wxICON_QUESTION, this ) != wxYES )
			{
				return;
			}

			if ( selectedRewardGroup->m_isSubGroup )
			{
				const wxTreeItemId parentItem = m_rewardTree->GetItemParent( selectedItem );
				const CRewardTreeData* parentData = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( parentItem ) );
				if ( parentData )
				{
					CRewardGroup* parentRewardGroup = parentData->m_group;
					if ( parentRewardGroup )
					{
						parentRewardGroup->m_subGroups.RemoveFast( selectedRewardGroup );
					}
				}
			}

			f->GetStatus();
			f->Delete( false, false );

			wxTheFrame->GetAssetBrowser()->UpdateResourceList();
			
			m_rewardTree->Delete( selectedItem );
		}
	}
}

void CEdRewardEditor::OnRevert( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	CRewardTreeData* data = static_cast<CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		f->GetStatus();
		if( !f->IsEdited() )
		{
			return;
		}

		if( !f->Revert() )
		{
			SHOW_ERROR( TXT("Failed to revert file.") );
		}
		else
		{
			if ( f->IsLoaded() )
			{
				m_resourceManager->RemoveRewardGroup( data->m_group );
				f->GetResource()->Reload(false);
				f->UpdateThumbnail();
			}

			data->m_group = SafeCast<CRewardGroup>( f->GetResource() );
			ASSERT( data->m_group );
			m_resourceManager->AddRewardGroup( data->m_group );

			f->GetStatus();
			if( f->IsCheckedIn() )
			{
				m_rewardTree->SetItemImage( item, IMAGE_GROUP );
			}
			m_rewardTree->DeleteChildren( item );
			LoadGroup( *data->m_group, item );
		}
	}
}

void CEdRewardEditor::OnSubmit( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		f->GetStatus();
		if( f->IsEdited() && f->Save() )
		{
			if( f->Submit() )
			{
				m_rewardTree->SetItemImage( item, IMAGE_GROUP );
			}
		}
	}
}

void CEdRewardEditor::OnHistory( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		f->GetStatus();
		if( !f->IsLocal() )
		{
			TDynArray< THashMap< String, String > > history;
			GVersionControl->FileLog( *f, history );
		}
	}
}

void CEdRewardEditor::OnAdd( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		f->GetStatus();
		if( f->IsLocal() )
		{
			f->Add();
		}
	}
}

void CEdRewardEditor::OnRename( wxCommandEvent& event )
{
	// TODO MR: Hell yeah!
	SHOW_WARNING( TXT("Sorry not implemented.") );
}

void CEdRewardEditor::OnSave( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		if( !f->Save() )
		{
			SHOW_ERROR( TXT("Failed to save file.") );
		}
	}
}

void CEdRewardEditor::OnCheckout( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	const CRewardTreeData* data = static_cast<const CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		if( f->CheckOut() )
		{
			m_rewardTree->SetItemImage( item, IMAGE_GROUP_CHECKEDOUT );
		}
	}
}

void CEdRewardEditor::OnSync( wxCommandEvent& event )
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	CRewardTreeData* data = static_cast<CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	ASSERT( data && data->m_dataType == RTDT_Group );

	if( data && data->m_group )
	{
		CDiskFile* f = data->m_group->GetFile();

		f->Sync();

		if( !GFileManager->GetFileSize( f->GetAbsolutePath() ) )
		{
			if( f->IsLoaded() )
			{
				m_resourceManager->RemoveRewardGroup( data->m_group );
				f->Rebind(NULL);
				data->m_group->Discard();
			}

			if (f->GetDirectory())
			{
				f->GetDirectory()->DeleteFile(*f);
			}

			m_rewardTree->Delete( item );
		}
		else
		{	
			if( f->IsLoaded() )
			{
				m_resourceManager->RemoveRewardGroup( data->m_group );
				f->GetResource()->Reload(false);
			}
			f->UpdateThumbnail();

			data->m_group = SafeCast<CRewardGroup>( f->GetResource() );
			ASSERT( data->m_group );
			m_resourceManager->AddRewardGroup( data->m_group );

			m_rewardTree->DeleteChildren( item );
			LoadGroup( *data->m_group, item );
		}
	}
}

void CEdRewardEditor::OnOK( wxCommandEvent& event )
{
	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_CHOOSE_REWARD_OK );
	ProcessEvent( okPressedEvent );

	// Close window
	Destroy();
}

void CEdRewardEditor::OnCancel( wxCommandEvent& event )
{
	// Send to parent
	wxCommandEvent cancelPressedEvent( wxEVT_CHOOSE_REWARD_CANCEL );
	ProcessEvent( cancelPressedEvent );

	// Close window
	Destroy();
}

void CEdRewardEditor::OnExit( wxCloseEvent& event )
{
	OnCancel( wxCommandEvent() );
}

CName CEdRewardEditor::GetSelectedRewardName() const
{
	const wxTreeItemId item = m_rewardTree->GetSelection();
	CRewardTreeData* data = static_cast<CRewardTreeData*>( m_rewardTree->GetItemData( item ) );

	if( data && data->m_dataType == RTDT_Reward )
	{
		return data->m_reward;
	}

	return CName::NONE;
}

void CEdRewardEditor::GetRewardNames( TDynArray<CName>& names )
{
	CDirectory* dir = GDepot->FindPath( REWARDS_DIR );
	if( dir )
	{
		for( CRewardDirectoryIterator it( *dir, true ); it; ++it )
		{
			const CRewardGroup* group = *it;
			TDynArray< SReward >::const_iterator end = group->m_rewards.End();
			for( TDynArray< SReward >::const_iterator rit = group->m_rewards.Begin(); rit != end; ++rit )
			{
				names.PushBack( rit->m_name );
			}
		}
	}
}

SReward * CEdRewardEditor::GetRewardByName( CName  name )
{
	TDynArray< THandle< CRewardGroup > > *  groups = m_resourceManager->GetRewardGroups();

	for( auto it = groups->Begin(); it != groups->End() ; ++it )
	{
		for( auto rit = (*it)->m_rewards.Begin(); rit != (*it)->m_rewards.End() ; ++rit )
		{
			if(rit->m_name == name)
			{
				return &(*rit);
			}
		}
	}

	return NULL;
}
