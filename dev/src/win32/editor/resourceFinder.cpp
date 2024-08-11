/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "resourceFinder.h"
#include "undoManager.h"
#include "undoCreate.h"

#include "../../common/core/clipboardBase.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/worldIterators.h"


void SOccurenceData::CollectOccurences( CWorld* world, CResource* resource, const CName& componentClassName )
{
	m_occurences.Clear();
	m_resource = resource;

	GFeedback->BeginTask( TXT("Collecting resource occurences..."), false );

	for ( WorldAttachedEntitiesIterator it( world ); it; ++it )
	{
		CEntity* ent = *it;
		if ( ent->GetLayer() == world->GetDynamicLayer() )
		{
			continue;
		}

		if ( ent->GetEntityTemplate() )
		{
			CResource* entityTemplateResource = SafeCast< CResource >( ent->GetEntityTemplate() );
			if ( entityTemplateResource && entityTemplateResource == resource )
			{
				IncludeOccurence( ent );
			}
		}

		TDynArray< CComponent* > components;

		SEntityStreamingState streamingState;
		ent->PrepareStreamingComponentsEnumeration( streamingState, true );
		components = ent->GetComponents();

		for ( Uint32 j=0; j<components.Size(); ++j )
		{
			CComponent* component = components[j];

			TDynArray< CResource* > resources;
			component->CollectUsedResources( resources );
			if ( resources.Exist( m_resource ) )
			{
				if( componentClassName.Empty() || component->GetClass()->GetName() == componentClassName )
				IncludeOccurence( ent );
			}
		}

		ent->FinishStreamingComponentsEnumeration( streamingState );
	}

	GFeedback->EndTask();
}

SOccurenceData::Info* SOccurenceData::FindOccurenceInfo( CEntity* entity )
{
	for ( Uint32 i=0; i<m_occurences.Size(); ++i )
	{
		if ( m_occurences[i].m_entity.Get() == entity )
		{
			return &(m_occurences[i]);
		}
	}

	return NULL;
}

void SOccurenceData::IncludeOccurence( CEntity* entity )
{
	if ( !entity )
	{
		return;
	}

	if ( Info* occurence = FindOccurenceInfo( entity ) )
	{
		++occurence->m_count;
	}
	else
	{
		// Entity uses our resource, create occurence entry
		new ( m_occurences ) Info;
		m_occurences.Back().m_entity = entity;
	}
}

// Event table
BEGIN_EVENT_TABLE( CEdResourceFinder, wxSmartLayoutPanel )
	EVT_LISTBOX_DCLICK( XRCID("m_occurencesListBox"), CEdResourceFinder::OnGotoResource )
	EVT_LISTBOX( XRCID("m_occurencesListBox"), CEdResourceFinder::OnSelectionChanged )

	EVT_BUTTON( XRCID("m_selectAllOccurencesBtn"), CEdResourceFinder::OnSelectAll )
	EVT_BUTTON( XRCID("m_unselectAllOccurencesBtn"), CEdResourceFinder::OnUnselectAll )
	EVT_BUTTON( XRCID("m_refreshBtn"), CEdResourceFinder::OnRefresh )
	EVT_BUTTON( XRCID("m_copyPath"), CEdResourceFinder::OnCopyPath )	

	EVT_BUTTON( XRCID("m_deleteOccurenceBtn"), CEdResourceFinder::OnDelete )
	EVT_BUTTON( XRCID("m_replaceBtn"), CEdResourceFinder::OnReplaceButton )

	EVT_UPDATE_UI( wxID_ANY, CEdResourceFinder::OnUpdateUI )

END_EVENT_TABLE()

TSortedMap< String, CEdResourceFinder* > CEdResourceFinder::s_resourceFinderWindows;

CEdResourceFinder::CEdResourceFinder( wxWindow* parent, CResource* resource, const CName& componentClassName )
	: wxSmartLayoutPanel( parent, TXT("ResourceFinder"), false )
	, m_resource( resource )
	, m_pauseSelectionUpdate( false )
	, m_componentClassName( componentClassName )
{
	// Set icon
	wxIcon iconSmall;
	iconSmall.CopyFromBitmap( SEdResources::GetInstance().LoadBitmap( _T("IMG_MATERIALS") ) );
	SetIcon( iconSmall );

	// Set caption
	String title = String::Printf( TXT("%s - Resource Finder"), m_resource->GetDepotPath().AsChar() );
	SetTitle( title.AsChar() );

	// Import tree view
	m_listBox = XRCCTRL( *this, "m_occurencesListBox", wxListBox );
	
	// Import options
	m_selectInWorldCheckBox = XRCCTRL( *this, "m_reflectSelectionCheckBox", wxCheckBox );
	wxToolTip* selectInWorldTT = m_selectInWorldCheckBox->GetToolTip();
	if ( selectInWorldTT )
	{
		// Set tooltip to appear instantly
		selectInWorldTT->SetDelay( 0 );
		selectInWorldTT->SetAutoPop( 999999 );
	}
	
	// fill data
	RefreshList();

	// Update and finalize layout
	Layout();

	SEvents::GetInstance().RegisterListener( CNAME( SelectionChanged ), this );
}

CEdResourceFinder::~CEdResourceFinder()
{
	s_resourceFinderWindows.EraseByValue( this );
	SEvents::GetInstance().UnregisterListener( this );
}

void CEdResourceFinder::RefreshList()
{
	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		m_occurenceData.CollectOccurences( world, m_resource, m_componentClassName );

		const TDynArray< SOccurenceData::Info >& occurences = m_occurenceData.m_occurences;

		m_listBox->Freeze();
		m_listBox->Clear();
		for ( Uint32 i=0; i<occurences.Size(); ++i )
		{
			CEntity* entity = occurences[i].m_entity.Get();
			if ( !entity || !entity->GetLayer() )
			{
				ASSERT( 0 && "Aaaaaaaaaaaa" );
				continue;
			}

			String layerPath;
			entity->GetLayer()->GetLayerInfo()->GetHierarchyPath( layerPath, false );
			layerPath += TXT("\\");
			layerPath += entity->GetName().AsChar();
			if ( occurences[i].m_count > 1 )
			{
				layerPath += TXT(" x");
				layerPath += ToString< Int32 >( occurences[i].m_count );
			}

			m_listBox->AppendString( layerPath.AsChar() );
		}

		m_listBox->Thaw();
		//m_listBox->Refresh();
	}
}

void CEdResourceFinder::OnGotoResource( wxCommandEvent &event )
{
	//wxArrayInt selections;
	Int32 selected = event.GetSelection();

	if ( selected < 0 ) return;

	SOccurenceData::Info& occurence = m_occurenceData.m_occurences[ selected ];
	CEntity* entity = occurence.m_entity.Get();

	if ( !entity || !entity->GetLayer() ) return;

	entity->CreateStreamedComponents( SWN_NotifyWorld );
	wxTheFrame->GetWorldEditPanel()->LookAtNode( entity );
}

static int wxCMPFUNC_CONV sort_int_wx(int *first, int *second)
{
	if ( *first == *second ) return 0;
	return *first > *second;
}

TDynArray< CEntity* > CEdResourceFinder::ExtractSelectedEntities()
{
	TDynArray< CEntity* > entities;

	wxArrayInt selections;
	m_listBox->GetSelections( selections );
	selections.Sort( sort_int_wx );

	for ( size_t i = 0; i < selections.size(); ++i )
	{
		Int32 selIndex = selections[i];
		if ( CEntity* entity = m_occurenceData.m_occurences[selIndex].m_entity.Get() )
		{
			entities.PushBack( entity );
		}
	}

	return entities;
}

void CEdResourceFinder::OnDelete( wxCommandEvent &event )
{
	TDynArray< CEntity* > selection = ExtractSelectedEntities();
	wxTheFrame->GetWorldEditPanel()->DeleteEntities( selection );
	RefreshList();
}

void CEdResourceFinder::OnSelectAll( wxCommandEvent &event )
{
	m_pauseSelectionUpdate = true;

	Int32 count = m_listBox->GetCount();
	for ( Int32 i=0; i<count; ++i )
	{
		m_listBox->Select( i );
	}

	m_pauseSelectionUpdate = false;

	UpdateWorldSelection();
}

void CEdResourceFinder::OnUnselectAll( wxCommandEvent &event )
{
	m_pauseSelectionUpdate = true;
	m_listBox->DeselectAll();
	m_pauseSelectionUpdate = false;

	UpdateWorldSelection();
}

void CEdResourceFinder::OnFocus( wxFocusEvent& event )
{
	RefreshList();
}

void CEdResourceFinder::OnRefresh( wxCommandEvent &event )
{
	RefreshList();
}

void CEdResourceFinder::OnCopyPath( wxCommandEvent &event )
{
	String clip = TXT("");

	wxArrayString selections;
	selections = m_listBox->GetStrings();
		
	if(selections.size() > 0)
	{
		for(Uint32 i=0; i<selections.size(); i++)
		{	
			clip += selections[i].wc_str();
			clip += TXT("\n");
		}		
	}	
		
	GClipboard->Copy( clip );
	selections.Clear();
}

enum
{
	ID_REPLACE_BY_ENTITY,
	ID_REPLACE_BY_ENTITY_COPY_COMPONENTS,
	ID_REPLACE_BY_MESH,
	ID_REPLACE_BY_STATIC_MESH
};

void CEdResourceFinder::OnReplace( wxCommandEvent &event )
{
	TDynArray< CEntity* > selection = CEdResourceFinder::ExtractSelectedEntities();

	switch ( event.GetId() )
	{
	case ID_REPLACE_BY_ENTITY:
	case ID_REPLACE_BY_ENTITY_COPY_COMPONENTS:
		wxTheFrame->GetWorldEditPanel()->ReplaceEntitiesWithEntity( selection, GetId() == ID_REPLACE_BY_ENTITY_COPY_COMPONENTS );
		break;
	case ID_REPLACE_BY_MESH:
	case ID_REPLACE_BY_STATIC_MESH:
		wxTheFrame->GetWorldEditPanel()->ReplaceEntitiesWithMesh( selection, GetId() == ID_REPLACE_BY_STATIC_MESH );
		break;
	}
}

void CEdResourceFinder::OnReplaceButton( wxCommandEvent &event )
{
	wxMenu menu;
	
	menu.Append( ID_REPLACE_BY_ENTITY, TXT("entity from AssetBrowser"), wxEmptyString, false );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdResourceFinder::OnReplace, this, ID_REPLACE_BY_ENTITY );

	menu.Append( ID_REPLACE_BY_ENTITY_COPY_COMPONENTS, TXT("entity from AssetBrowser (copy components)"), wxEmptyString, false );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdResourceFinder::OnReplace, this, ID_REPLACE_BY_ENTITY_COPY_COMPONENTS );

	menu.Append( ID_REPLACE_BY_MESH, TXT("mesh from AssetBrowser"), wxEmptyString, false );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdResourceFinder::OnReplace, this, ID_REPLACE_BY_MESH );

	menu.Append( ID_REPLACE_BY_STATIC_MESH, TXT("mesh from AssetBrowser (static)"), wxEmptyString, false );
	menu.Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdResourceFinder::OnReplace, this, ID_REPLACE_BY_STATIC_MESH );

	wxButton* replaceBtn = XRCCTRL( *this, "m_replaceBtn", wxButton );
	replaceBtn->PopupMenu( &menu, 0, replaceBtn->GetSize().y );
}

void CEdResourceFinder::OnSelectionChanged( wxCommandEvent& event )
{
	UpdateWorldSelection();
}

void CEdResourceFinder::UpdateWorldSelection()
{
	if ( m_pauseSelectionUpdate || !m_selectInWorldCheckBox->IsChecked() )
	{
		// Don't want to update world selection
		return;
	}

	if ( CWorld* world = GGame->GetActiveWorld() )
	{
		if ( CSelectionManager* selectionManager = world->GetSelectionManager() )
		{
			// Reset selection state
			selectionManager->DeselectAll();

			// Reflect list box selection in world selection
			wxArrayInt selections;
			m_listBox->GetSelections( selections );
			if ( selections.GetCount() > 0 )
			{
				for ( Int32 i=0; i<(Int32)selections.GetCount(); ++i )
				{
					Int32 selIndex = selections[i];
					CEntity* entity = m_occurenceData.m_occurences[selIndex].m_entity.Get();
					if ( entity && entity->GetLayer() && !entity->IsSelected() )
					{
						selectionManager->Select( entity );
					}
				}
			}
		}
	}
}

void CEdResourceFinder::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( SelectionChanged ) )
	{
	}
}


/*static*/ 
void CEdResourceFinder::ShowForResource( CResource* resource, const CName& componentClassName )
{
	String depotPath = resource->GetDepotPath();

	CEdResourceFinder* finder = nullptr;
	if ( s_resourceFinderWindows.Find( depotPath, finder ) )
	{
		finder->RefreshList();
	}
	else
	{
		finder = new CEdResourceFinder( wxTheFrame, resource, componentClassName );
		s_resourceFinderWindows.Insert( depotPath, finder );
	}

	finder->Show();
	finder->SetFocus();
	finder->Raise();
}

void CEdResourceFinder::OnUpdateUI( wxUpdateUIEvent& event )
{
	wxArrayInt sel;
	m_listBox->GetSelections( sel );
	Bool hasSelection = ( !sel.empty() );

	XRCCTRL( *this, "m_deleteOccurenceBtn", wxButton )->Enable( hasSelection );
	XRCCTRL( *this, "m_replaceBtn", wxButton )->Enable( hasSelection );
}
