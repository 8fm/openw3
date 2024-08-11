/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "tagEditor.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"

#define ID_FILTER_TIMER		12345

wxDEFINE_EVENT( wxEVT_TAGEDITOR_OK, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_TAGEDITOR_CANCEL, wxCommandEvent );

// Event table
BEGIN_EVENT_TABLE( CEdTagEditor, wxDialog )
	EVT_BUTTON( XRCID( "SetTag" ), CEdTagEditor::OnSetTag )
	EVT_BUTTON( XRCID( "ClearTag" ), CEdTagEditor::OnClearTag )
	EVT_BUTTON( XRCID( "AddNewTag" ), CEdTagEditor::OnAddNewTag )
	EVT_LISTBOX_DCLICK( XRCID( "LocalTags"), CEdTagEditor::OnEditTag )
	EVT_TREE_ITEM_ACTIVATED( XRCID( "WorldTags" ), CEdTagEditor::OnActivateTag )
	EVT_BUTTON( XRCID( "OK" ), CEdTagEditor::OnOK )
	EVT_BUTTON( XRCID( "Cancel" ), CEdTagEditor::OnCancel )
	EVT_CLOSE( CEdTagEditor::OnClose )
	EVT_TIMER( ID_FILTER_TIMER, CEdTagEditor::OnTimer )
	EVT_TEXT( XRCID( "Filter" ), CEdTagEditor::OnFilterChanged )
END_EVENT_TABLE()

CEdTagEditor::CEdTagEditor( wxWindow* parent, const TagList& currentTagList )
	: m_tagList( currentTagList )
	, m_filterTimer( this, ID_FILTER_TIMER )
	, m_filter( NULL )
	, m_worldTags( NULL )
	, m_localTags( NULL )
{
	// Load designed dialog from resource
	wxXmlResource::Get()->LoadDialog( this, parent, TEXT("TagEditor") );

	// Copy current tag list
	m_tags = m_tagList.GetTags();

	// Global tag list
	m_worldTags = XRCCTRL( *this, "WorldTags", wxTreeListCtrl );
	m_worldTags->AddColumn( TXT("Name"), 300 );
	m_worldTags->AddColumn( TXT("Count") );

	// Setup images
	wxImageList* images = new wxImageList( 16, 16, true, 2 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_ENTITY") ) );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_COMPONENT") ) );
	m_worldTags->AssignImageList( images );

	// Local tag list
	m_localTags = XRCCTRL( *this, "LocalTags", wxListBox );

	// Get filter 
	m_filter = XRCCTRL( *this, "Filter", wxTextCtrl );

	LoadOptionsFromConfig();
    // Update tag lists
	UpdateWorldTagList();
	UpdateLocalTagList();
}

CEdTagEditor::~CEdTagEditor()
{
	SaveOptionsToConfig();
}

void CEdTagEditor::SaveOptionsToConfig()
{
	SaveLayout(TXT("/Dialogs/TagEditor"));

    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Dialogs/TagMiniEditor") );
	
    String sTags;
    TSet<String>::iterator tag_curr = m_historyTags.Begin(),
                           tag_last = m_historyTags.End();
    for(; tag_curr != tag_last; ++tag_curr)
        sTags += *tag_curr + TXT(";");
    config.Write( TXT("History"), sTags );
}

void CEdTagEditor::LoadOptionsFromConfig()
{
	LoadLayout(TXT("/Dialogs/TagEditor"));

    CUserConfigurationManager &config = SUserConfigurationManager::GetInstance();
	CConfigurationScopedPathSetter pathSetter( config, TXT("/Dialogs/TagMiniEditor") );

    String sTags;
    if (config.Read( TXT("History"), &sTags ))
    {
        TDynArray<String> tags;
        String(sTags).Slice(tags, TXT(";"));
    
        TDynArray<String>::iterator tag_curr = tags.Begin(),
                                    tag_last = tags.End();
        for(; tag_curr != tag_last; ++tag_curr)
        {
            (*tag_curr).Trim();
            if (!tag_curr->Empty())
                m_historyTags.Insert(*tag_curr);
        }
    }
}

void CEdTagEditor::UpdateWorldTagList()
{
	// Begin update
	m_worldTags->Freeze();
	m_worldTags->DeleteRoot();

	// Get filter
	String filter = m_filter->GetValue().wc_str();
	filter.MakeLower();

    // Create list root item
	wxTreeItemId root = m_worldTags->AddRoot( TXT("Known Tags") );

    // Update historical tags
    {
        wxTreeItemId historyRoot = m_worldTags->AppendItem( root, TXT("History"), 0, 0 );

		// Fill list
		for ( TSet< String >::iterator i = m_historyTags.Begin(); i != m_historyTags.End(); ++i )
		{
            if ( filter.Empty() || i->ToLower().ContainsSubstring( filter ) )
            {
			    wxTreeItemId id = m_worldTags->AppendItem( historyRoot, i->AsChar(), 1 );
            }
		}
    }

	// Get active world
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetWorldLayers() )
	{
		// Create list root item
		wxTreeItemId worldRoot = m_worldTags->AppendItem( root, TXT("World"), 0, 0 );

		// Get list of layers from active world
		TDynArray< CLayerInfo* > layers;
		world->GetWorldLayers()->GetLayers( layers, false );

		// Create layer node for each loaded layer
		for ( Uint32 k=0; k<layers.Size(); k++ )
		{
			CLayerInfo* info = layers[k];
			if ( info->IsLoaded() )
			{
				CLayer* trueLayer = info->GetLayer();

				// Scan layer searching for nodes with tags
				TDynArray< CNode* > nodesToProcess;
				{
					// Get entities
					TDynArray< CEntity* > entities;
					trueLayer->GetEntities( entities );

					// Process components from entities
					for ( Uint32 nEnt=0; nEnt<entities.Size(); nEnt++ )
					{
						// Add entity
						CEntity* entity = entities[ nEnt ];
						if ( entity )
						{
							if ( !entity->GetTags().Empty() )
							{
								nodesToProcess.PushBack( entity );
							}

							// Process components
							const TDynArray< CComponent* >& components = entity->GetComponents();
							for ( Uint32 nComp=0; nComp<components.Size(); nComp++ )
							{
								CComponent* component = components[ nComp ];
								if ( component && !component->GetTags().Empty() )
								{
									nodesToProcess.PushBack( component );
								}
							}
						}
					}
				}

				// Collect tags and count them
				THashMap< CName, Uint32 > names;
				{
					for ( Uint32 i=0; i<nodesToProcess.Size(); i++ )
					{
						CNode* node = nodesToProcess[i];

						// Collect tags
						const TDynArray< CName >& localNames = node->GetTags().GetTags();
						for ( Uint32 j=0; j<localNames.Size(); j++ )
						{
							Uint32 count = 0;
							const CName& name = localNames[ j ];

							if ( filter.Empty() || name.AsString().ToLower().ContainsSubstring( filter ) )
							{
								names.Find( name, count );
								names[ name ] = count + 1;
							}
						}
					}
				}

				// Add layer item only if there are some tags inside it
				if ( names.Size() )
				{
					String caption = String::Printf( TXT("Layer '%ls'"), info->GetShortName().AsChar() );
					wxTreeItemId layer = m_worldTags->AppendItem( worldRoot, caption.AsChar(), 0, 0 );

					// Fill list
					for ( THashMap< CName, Uint32 >::const_iterator i = names.Begin(); i != names.End(); ++i )
					{
						wxTreeItemId id = m_worldTags->AppendItem( layer, i->m_first.AsString().AsChar(), 1 );
						m_worldTags->SetItemText( id, 1, String::Printf( TXT("%i"), i->m_second ).AsChar() );
					}
				}
			}
		}
	}

    // Show tree structure
	m_worldTags->ExpandAll( root );

	// End update
	m_worldTags->Thaw();
	m_worldTags->Refresh();
}

void CEdTagEditor::UpdateLocalTagList()
{
	// Begin update
	m_localTags->Freeze();
	m_localTags->Clear();

	// Fill list
	for ( Uint32 i=0; i<m_tags.Size(); i++ )
	{
		CName tag = m_tags[i];
		m_localTags->AppendString( tag.AsString().AsChar() );
	}

	// End update
	m_localTags->Thaw();
	m_localTags->Refresh();
}

void CEdTagEditor::OnAddNewTag( wxCommandEvent& event )
{
	// Ask for tag name
	String tagName;
	if ( InputBox( this, TXT( "Create New Tag" ), TXT( "Specify the name of the new tag" ), tagName ) )
	{
		if ( !tagName.Empty() )
		{
			// Add name to tag list
			CName name( tagName.AsChar() );
			m_tags.PushBackUnique( name );

			// Update local tag list
			UpdateLocalTagList();

			// Select tag
			SelectLocalTag( name );
		}
	}
}

void CEdTagEditor::SelectLocalTag( const CName& tag )
{
	Int32 index = m_localTags->FindString( tag.AsString().AsChar(), false );
	if ( index != -1 )
	{
		m_localTags->Select( index );
	}
}

void CEdTagEditor::OnEditTag( wxCommandEvent& event )
{
	Int32 selected = m_localTags->GetSelection();
	if ( selected != -1 )
	{
		String oldTagName = m_localTags->GetString( selected ).wc_str();
		String currentTagName = oldTagName;
		if ( InputBox( this, TXT( "Edit Tag Name" ), TXT( "Specify the new name of the tag" ), currentTagName ) )
		{
			// Remove old tag
			m_tags.Remove( CName( oldTagName ) );

			// Add new tag
			CName name( currentTagName.AsChar() );
			if ( name )
			{
				// Add name to tag list
				m_tags.PushBackUnique( name );
			}

			// Update local tag list
			UpdateLocalTagList();

			// Select tag
			SelectLocalTag( name );
		}
	}
}

void CEdTagEditor::OnSetTag( wxCommandEvent& event )
{
	// Get selected item
	wxTreeItemId id = m_worldTags->GetSelection();
	if ( id.IsOk() )
	{
		String text = m_worldTags->GetItemText( id, 0 ).wc_str();
		if ( !text.Empty() && m_worldTags->GetItemImage(id) == 1 )
		{
			// Add name to tag list
			CName name( text.AsChar() );
			m_tags.PushBackUnique( name );

			// Update local tag list
			UpdateLocalTagList();

			// Select tag
			SelectLocalTag( name );
		}
	}
}

void CEdTagEditor::OnClearTag( wxCommandEvent& event )
{
	Int32 selected = m_localTags->GetSelection();
	if ( selected != -1 )
	{
		// Remove tag
		CName tagName( m_localTags->GetString( selected ).wc_str() );
		m_tags.Remove( tagName );

		// Update local tag list
		UpdateLocalTagList();
	}
}

void CEdTagEditor::OnOK( wxCommandEvent& event )
{
    TDynArray<CName>::iterator tag_curr = m_tags.Begin(),
                               tag_last = m_tags.End();
    for(; tag_curr != tag_last; ++tag_curr)
        m_historyTags.Insert(tag_curr->AsString().AsChar());

	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_TAGEDITOR_OK );
	ProcessEvent( okPressedEvent );

	// Close window
	EndModal( wxOK );
	Destroy();
}

void CEdTagEditor::OnCancel( wxCommandEvent& event )
{
	// Send to parent
	wxCommandEvent okPressedEvent( wxEVT_TAGEDITOR_CANCEL );
	ProcessEvent( okPressedEvent );

	// Close window
	EndModal( wxCANCEL );
	Destroy();
}

void CEdTagEditor::OnClose( wxCloseEvent& event )
{
	wxCommandEvent fakeEvent;
	OnCancel( fakeEvent );
}

void CEdTagEditor::OnActivateTag( wxTreeEvent& event )
{
	wxCommandEvent fakeEvent;
	OnSetTag( fakeEvent );
}

void CEdTagEditor::OnTimer( wxTimerEvent& event )
{
	UpdateWorldTagList();
}

void CEdTagEditor::OnFilterChanged( wxCommandEvent& event )
{
	m_filterTimer.Start( 400, true );	
}
