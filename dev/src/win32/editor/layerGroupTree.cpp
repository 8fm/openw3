/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "layerGroupTree.h"
#include "../../common/engine/layerGroup.h"

// Event table
BEGIN_EVENT_TABLE( CEdLayerGroupTree, wxDialog )
	EVT_BUTTON( XRCID("OK"), CEdLayerGroupTree::OnOK )
	EVT_BUTTON( XRCID("Cancel"), CEdLayerGroupTree::OnCancel )
	EVT_BUTTON( XRCID("NULL"), CEdLayerGroupTree::OnNULL )
	EVT_TREE_ITEM_ACTIVATED( XRCID("groupTree"), CEdLayerGroupTree::OnItemActivated )
	EVT_TREE_SEL_CHANGED( XRCID("groupTree"), CEdLayerGroupTree::OnItemSelected )
END_EVENT_TABLE()

CEdLayerGroupTree::CEdLayerGroupTree( wxWindow* parent, CLayerGroup* baseGroup, const String& currentPath )
	: m_selectedGroup( NULL )
	, m_baseGroup( baseGroup )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("LayerGroupPicker") );

	// Setup tree
	wxTreeCtrl* groupTree = XRCCTRL( *this, "groupTree", wxTreeCtrl );

	// Setup images
	wxImageList* images = new wxImageList( 16, 16, true, 1 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_WORLD16") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_LAYER") ) );
	groupTree->AssignImageList( images );

	// Create tree
	if ( baseGroup )
	{
		// Create items and select default group
		CLayerGroup* groupToSelect = baseGroup->FindGroupByPath( currentPath );
		AppendTreeItems( groupTree, wxTreeItemId(), baseGroup, groupToSelect );
	}
	else
	{
		// Disable controls
		XRCCTRL( *this, "groupTree", wxTreeCtrl )->Disable();
		XRCCTRL( *this, "OK", wxButton )->Disable();
	}
}

void CEdLayerGroupTree::AppendTreeItems( wxTreeCtrl* tree, wxTreeItemId parent, CLayerGroup* group, CLayerGroup* groupToSelect )
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

	// Select group
	if ( groupToSelect == group )
	{
		tree->SelectItem( itemId, true );
	}

	// Create child items
	const CLayerGroup::TGroupList& subGroups = group->GetSubGroups();
	for ( Uint32 i=0; i<subGroups.Size(); i++ )
	{
		AppendTreeItems( tree, itemId, subGroups[i], groupToSelect );
	}

	// Expand item
	tree->Expand( itemId );
}

void CEdLayerGroupTree::OnOK( wxCommandEvent& event )
{
	m_selectedGroup = GetSelectedLayerGroup();
	if ( m_selectedGroup )
	{
		EndDialog( 0 );
	}
}

CLayerGroup* CEdLayerGroupTree::GetSelectedLayerGroup()
{
	// Get selected group
	wxTreeCtrl* groupTree = XRCCTRL( *this, "groupTree", wxTreeCtrl );
	wxTreeItemId selectedId = groupTree->GetSelection();
	if ( selectedId.IsOk() )
	{
		SerializableItemWrapper* data = (SerializableItemWrapper*) groupTree->GetItemData( selectedId );
		if ( data )
		{
			return Cast< CLayerGroup >( data->m_object );
		}
	}

	// No data
	return NULL;
}

void CEdLayerGroupTree::OnItemSelected( wxTreeEvent& event )
{
	CLayerGroup* group = GetSelectedLayerGroup();
	XRCCTRL( *this, "OK", wxButton )->Enable( m_baseGroup && group != NULL && group != m_baseGroup );
}

void CEdLayerGroupTree::OnItemActivated( wxTreeEvent& event )
{
	m_selectedGroup = GetSelectedLayerGroup();
	if ( m_selectedGroup )
	{
		EndDialog( 0 );
	}
}

void CEdLayerGroupTree::OnNULL( wxCommandEvent& event )
{
	m_selectedGroup = NULL;
	EndDialog( 0 );
}

void CEdLayerGroupTree::OnCancel( wxCommandEvent& event )
{
	m_selectedGroup = NULL;
	EndDialog( 1 );
}