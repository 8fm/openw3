/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerGroupList.h"
#include "../../common/engine/layerGroup.h"

// Event table
BEGIN_EVENT_TABLE( CEdLayerGroupList, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdLayerGroupList::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdLayerGroupList::OnCancel )
	EVT_BUTTON( XRCID("NULL"), CEdLayerGroupList::OnClearAll )
	EVT_BUTTON( XRCID("FillChildren"), CEdLayerGroupList::OnFillChildren )
	EVT_TOGGLEBUTTON( XRCID("ExpandTree"), CEdLayerGroupList::OnExpandTree )
	EVT_LIST_ITEM_ACTIVATED( XRCID("groupList"), CEdLayerGroupList::OnRemoveLayer )
END_EVENT_TABLE()

CEdLayerGroupList::CEdLayerGroupList( wxWindow* parent, CLayerGroup* baseGroup, const TDynArray< String >& currentLayerPaths )
	: m_baseGroup( baseGroup )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("LayerGroupList") );

	// Setup tree
	wxTreeCtrl* groupTree = XRCCTRL( *this, "groupTree", wxTreeCtrl );
	wxListCtrl* groupList = XRCCTRL( *this, "groupList", wxListCtrl );

	// Setup images
	wxImageList* images = new wxImageList( 16, 16, true, 1 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_WORLD16") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER_BW") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER_CHECKED") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER_CHECKED_BW") ) );
	groupList->SetImageList( images, wxIMAGE_LIST_SMALL );
	groupTree->AssignImageList( images );

	// Events
	groupTree->Connect( wxEVT_LEFT_DCLICK, wxMouseEventHandler( CEdLayerGroupList::OnItemActivated ), NULL, this );

	// Format columns
	groupList->InsertColumn( 0, wxT("Group name"), wxLIST_FORMAT_LEFT, 300 );

	// Create tree
	if ( !baseGroup )
	{
		// Disable controls
		XRCCTRL( *this, "groupList", wxListCtrl )->Disable();
		XRCCTRL( *this, "groupTree", wxTreeCtrl )->Disable();
		XRCCTRL( *this, "OK", wxButton )->Disable();
		return;
	}

	// Determine the selected layers
	for ( Uint32 i=0; i<currentLayerPaths.Size(); i++ )
	{
		CLayerGroup* group = baseGroup->FindGroupByPath( currentLayerPaths[i] );
		if ( group )
		{
			m_selectedGroups.PushBack( group );
		}
	}

	// Update tree
	AppendTreeItems( groupTree, wxTreeItemId(), baseGroup );

	//expand the root item
	groupTree->Expand( groupTree->GetRootItem() );

	// Update list
	UpdateListOfSelectedGroups();
}

void CEdLayerGroupList::OnOK( wxCommandEvent& event )
{
	wxListCtrl* groupList = XRCCTRL( *this, "groupList", wxListCtrl );
	if ( groupList && groupList->GetItemCount() )
	{
		// Done
		EndDialog( 0 );
	}
}

void CEdLayerGroupList::OnClearAll( wxCommandEvent& event )
{
	m_selectedGroups.Clear();
	EndDialog( 0 );
}

void CEdLayerGroupList::OnCancel( wxCommandEvent& event )
{
	EndDialog( 1 );
}

static Bool IsGroupInsideGroup( CLayerGroup* group, CLayerGroup* baseGroup )
{
	if ( !group )
	{
		return false;
	}

	if ( group == baseGroup )
	{
		return true;
	}

	return IsGroupInsideGroup( group->GetParentGroup(), baseGroup );
}

void CEdLayerGroupList::OnItemActivated( wxMouseEvent& event )
{
	// Get selected group
	wxTreeCtrl* groupTree = XRCCTRL( *this, "groupTree", wxTreeCtrl );
	wxTreeItemId selectedId = groupTree->GetSelection();
	if ( selectedId.IsOk() )
	{
		SerializableItemWrapper* data = (SerializableItemWrapper*) groupTree->GetItemData( selectedId );
		if ( data )
		{
			// Add only non selected items
			CLayerGroup* clickedGroup = Cast< CLayerGroup >( data->m_object );
			Int32 icon = groupTree->GetItemImage( selectedId );
			if ( clickedGroup && ( 1 == icon || 3 == icon ) )
			{
				// Toggle
				if ( m_selectedGroups.Exist( clickedGroup ) )
				{
					// Remove from list of selected groups
					m_selectedGroups.Remove( clickedGroup  );
				}
				else
				{
					// Add group
					m_selectedGroups.PushBack( clickedGroup );
				}

				// Update layout
				UpdateListOfSelectedGroups();
				UpdateTreeIcons();
			}
		}
	}
}

void CEdLayerGroupList::OnRemoveLayer( wxListEvent& event )
{
	wxListView* listView = ( wxListView* ) XRCCTRL( *this, "groupList", wxListCtrl );
	if ( listView )
	{
		// Get selected item
		long selected = listView->GetFirstSelected();
		if ( selected != -1 )
		{
			CLayerGroup* group = ( CLayerGroup* ) listView->GetItemData( selected );
			if ( m_selectedGroups.Remove( group ) )
			{
				listView->DeleteItem( selected );
				UpdateTreeIcons();
			}
		}
	}
}
 
void CEdLayerGroupList::AppendTreeItems( wxTreeCtrl* tree, wxTreeItemId parent, CLayerGroup* group )
{
	// Add the root item
	wxTreeItemId itemId;
	if ( parent.IsOk() )
	{
		// No layers or sub groups, fuck it !
		if ( group->GetSubGroups().Empty() && group->GetLayers().Empty() )
		{
			return;
		}

		// Create child item
		String name = String::Printf( TXT("%s"), group->GetName().AsChar() );
		itemId = tree->AppendItem( parent, name.AsChar(), 1, 1, new SerializableItemWrapper( group ) );
	}
	else
	{
		// Create root item
		String name = String::Printf( TXT("%s (ROOT)"), group->GetName().AsChar() );
		itemId = tree->AddRoot( name.AsChar(), 0, 0, new SerializableItemWrapper( group ) );
	}

	// Set icon
	const Uint32 icon = GetItemIcon( group );
	tree->SetItemImage( itemId, icon, wxTreeItemIcon_Normal );
	tree->SetItemImage( itemId, icon, wxTreeItemIcon_Selected );

	// Create child items
	const CLayerGroup::TGroupList& subGroups = group->GetSubGroups();
	for ( Uint32 i=0; i<subGroups.Size(); i++ )
	{
		AppendTreeItems( tree, itemId, subGroups[i] );
	}
}

void CEdLayerGroupList::UpdateListOfSelectedGroups()
{
	// Clear elements
	wxListCtrl* groupList = XRCCTRL( *this, "groupList", wxListCtrl );
	groupList->DeleteAllItems();

	// Add elements
	for ( Uint32 i=0; i<m_selectedGroups.Size(); i++ )
	{
		CLayerGroup* group = m_selectedGroups[i];
		groupList->InsertItem( i, group->GetGroupPathName( m_baseGroup ).AsChar() );
		groupList->SetItemPtrData( i, reinterpret_cast< wxUIntPtr >( group ) );
		groupList->SetItemImage( i, 3 );
	}
}

Uint32 CEdLayerGroupList::GetItemIcon( CLayerGroup* group )
{
	// Base group
	if ( !group || group == m_baseGroup )
	{
		return 0;
	}

	// Selected
	for ( Uint32 i=0; i<m_selectedGroups.Size(); i++ )
	{
		if ( group == m_selectedGroups[i] )
		{
			return 3;
		}
	}

	// Normal layer
	return 1;	
}

namespace LayerGroupListHelpers
{
	static void FillRecursive( TDynArray< CLayerGroup* >& layersToFill, const CLayerGroup::TGroupList& layersToProcess )
	{
		for ( Uint32 i=0; i<layersToProcess.Size(); i++ )
		{
			const CLayerGroup* group = layersToProcess[i];
			layersToFill.PushBackUnique( const_cast< CLayerGroup* >(group) );

			const CLayerGroup::TGroupList& subgroups = group->GetSubGroups();

			LayerGroupListHelpers::FillRecursive( layersToFill, subgroups );
		}
	}
}

void CEdLayerGroupList::OnFillChildren( wxCommandEvent& event )
{
	// Fill selected groups children
	TDynArray< CLayerGroup* > layersToProcess = m_selectedGroups;

	LayerGroupListHelpers::FillRecursive( m_selectedGroups, reinterpret_cast< const CLayerGroup::TGroupList& >( layersToProcess ) );

	// Update layout
	UpdateListOfSelectedGroups();
	UpdateTreeIcons();
}

void CEdLayerGroupList::OnExpandTree( wxCommandEvent& event )
{
	wxTreeCtrl* groupTree = XRCCTRL( *this, "groupTree", wxTreeCtrl );
	wxToggleButton* expandTreeButton = XRCCTRL( *this, "ExpandTree", wxToggleButton );
	if ( event.IsChecked() )
	{
		groupTree->ExpandAll();
		expandTreeButton->SetLabel( wxT( "Collapse" ) );
	}
	else
	{
		groupTree->CollapseAll();
		groupTree->Expand( groupTree->GetRootItem() );
		expandTreeButton->SetLabel( wxT( "Expand" ) );
	}
}

void CEdLayerGroupList::UpdateTreeItem( wxTreeCtrl* tree, wxTreeItemId item )
{
	// Update icon
	SerializableItemWrapper* data = (SerializableItemWrapper*) tree->GetItemData( item );
	if ( data )
	{
		CLayerGroup* group = Cast<CLayerGroup>( data->m_object );
		Int32 icon = GetItemIcon( group );
		tree->SetItemImage( item, icon, wxTreeItemIcon_Normal );
		tree->SetItemImage( item, icon, wxTreeItemIcon_Selected );
	}

	// Update children
	wxTreeItemIdValue token;
	wxTreeItemId child = tree->GetFirstChild( item, token );
	while ( child.IsOk() )
	{
		UpdateTreeItem( tree, child );
		child = tree->GetNextSibling( child );
	}
}

void CEdLayerGroupList::UpdateTreeIcons()
{
	wxTreeCtrl* groupTree = XRCCTRL( *this, "groupTree", wxTreeCtrl );
	UpdateTreeItem( groupTree, groupTree->GetRootItem() );
}
