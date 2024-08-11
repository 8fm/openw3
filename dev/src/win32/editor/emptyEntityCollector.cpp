/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "emptyEntityCollector.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/layerGroup.h"
#include "../../common/engine/entityGroup.h"

BEGIN_EVENT_TABLE( CEdEmptyEntityCollector, wxFrame )
	EVT_LISTBOX_DCLICK(	XRCID("m_emptyEntitiesList"),	CEdEmptyEntityCollector::OnEmptyEntitiesDoubleClick )
	EVT_BUTTON(			XRCID("m_refresh"),				CEdEmptyEntityCollector::OnRefreshClicked )
	EVT_BUTTON(			XRCID("m_focus"),				CEdEmptyEntityCollector::OnFocusClicked )
	EVT_BUTTON(			XRCID("m_delete"),				CEdEmptyEntityCollector::OnDeleteClicked )
	EVT_BUTTON(			XRCID("m_selectAll"),			CEdEmptyEntityCollector::OnSelectAllClicked )
	EVT_BUTTON(			XRCID("m_selectNone"),			CEdEmptyEntityCollector::OnSelectNoneClicked )
	EVT_BUTTON(			XRCID("m_close"),				CEdEmptyEntityCollector::OnCloseClicked )
	EVT_BUTTON(			XRCID("m_import"),				CEdEmptyEntityCollector::OnImportClicked )
	EVT_BUTTON(			XRCID("m_export"),				CEdEmptyEntityCollector::OnExportClicked )
END_EVENT_TABLE()

CEdEmptyEntityCollector::CEdEmptyEntityCollector( wxWindow* parent )
{
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("EmptyEntityCollector") );
	
	m_emptyEntitiesList = XRCCTRL( *this, "m_emptyEntitiesList", wxCheckListBox );

	OnRefreshClicked( wxCommandEvent() );
}

CEdEmptyEntityCollector::~CEdEmptyEntityCollector()
{
}

void CEdEmptyEntityCollector::ScanLayer( CLayer* layer, Bool showTagged )
{
	const LayerEntitiesArray& entities = layer->GetEntities();

	// Scan layer for empty entities
	for ( auto it=entities.Begin(); it != entities.End(); ++it )
	{
		CEntity* entity = *it;

		if ( !entity->IsA< CEntityGroup >() && !m_entitiesInStreaming.Exist( entity->GetGUID() ) && entity->GetComponents().Empty() )
		{
			// Skip tagged entities if we dont care about them
			if ( !showTagged && !entity->GetTags().Empty() )
			{
				continue;
			}

			m_emptyEntities.PushBack( entity );
			m_emptyEntitiesList->Append( wxString::Format( wxT("%s (%s)"), entity->GetName().AsChar(), layer->GetDepotPath().AsChar() ) );
			m_foundEntities.Insert( entity->GetGUID(), entity );
		}
	}
}

void CEdEmptyEntityCollector::ScanLayerGroup( CLayerGroup* layerGroup, Bool showTagged )
{
	TDynArray< CLayerInfo* > layerInfos;
	layerGroup->GetLayers( layerInfos, true, true, true );

	// Scan all the layers in this group
	for ( auto it=layerInfos.Begin(); it != layerInfos.End(); ++it )
	{
		CLayerInfo* layerInfo = *it;

		if ( layerInfo->IsLoaded() && !layerInfo->GetDepotPath().Empty() )
		{
			ScanLayer( layerInfo->GetLayer(), showTagged );
		}
	}

	// Scan all the subgroups
	const CLayerGroup::TGroupList& subGroups = layerGroup->GetSubGroups();
	for ( auto it=subGroups.Begin(); it != subGroups.End(); ++it )
	{
		ScanLayerGroup( *it, showTagged );
	}
}

void CEdEmptyEntityCollector::Refresh()
{
#if 0
	Bool showTaggedEntities = XRCCTRL( *this, "m_showTagged", wxCheckBox )->GetValue();

	// Remove current entities
	m_entitiesInStreaming.Clear();
	m_foundEntities.Clear();
	m_emptyEntities.Clear();
	m_emptyEntitiesList->Clear();

	// Make sure we have a world to work with
	CWorld* world = GGame->GetActiveWorld();
	if ( !world )
	{
		return;
	}

	// Scan streaming tiles to build the guid hashset
	CLayerGroup* streamingTilesGroup = world->GetWorldLayers()->FindGroup( TXT("streaming_tiles") );
	if ( !streamingTilesGroup )
	{
		return;
	}
	TDynArray< CLayerInfo* > streamingLayerInfos;
	streamingTilesGroup->GetLayers( streamingLayerInfos, false, true, false );

	GFeedback->BeginTask( TXT("Scanning Steaming Tiles"), false );
	for ( Uint32 i=0; i < streamingLayerInfos.Size(); ++i )
	{
		CLayerInfo* layerInfo = streamingLayerInfos[i];
		Bool wasLoaded = layerInfo->IsLoaded();

		// Load the layer if necessary
		if ( !wasLoaded )
		{
			LayerLoadingContext ctx;
			layerInfo->SyncLoad( ctx );
		}

		// If the load failed, just skip this layer (we'll possibly get false positives here)
		if ( !layerInfo->IsLoaded() )
		{
			continue;
		}

		// Collect GUIDs from the layer and put them in the hashset
		CLayer* layer = layerInfo->GetLayer();
		for ( auto it = layer->m_streamingEntries.Begin(); it != layer->m_streamingEntries.End(); ++it )
		{
			m_entitiesInStreaming.Insert( (*it).m_entityGuid );
		}
		for ( int lod=0; lod < 3; ++lod )
		{
			for ( auto it = layer->m_streamedDetachedData[lod].Begin(); it != layer->m_streamedDetachedData[lod].End(); ++it )
			{
				m_entitiesInStreaming.Insert( (*it).m_entityGuid );
			}
		}

		// If the layer was unloaded, unload it
		if ( !wasLoaded )
		{
			layerInfo->SyncUnload();
		}

		// Update feedback
		String msg = String::Printf( TXT("Scanning tile layer %d of %d"), (int)i, (int)streamingLayerInfos.Size() );
		GFeedback->UpdateTaskInfo( msg.AsChar() );
		GFeedback->UpdateTaskProgress( i, streamingLayerInfos.Size() );
	}

	// Invalidate the streaming caches to make sure the load/unload changes above didn't break anything
	world->UpdateStreamingLater();

	// Collect entities from the world
	GFeedback->UpdateTaskInfo( TXT("Scanning the world for empty layers") );
	GFeedback->UpdateTaskProgress( 1, 1 );
	if ( GGame->GetActiveWorld() )
	{
		ScanLayerGroup( GGame->GetActiveWorld()->GetWorldLayers(), showTaggedEntities );
	}
	GFeedback->EndTask();
#endif
}

void CEdEmptyEntityCollector::Focus()
{
	int index = m_emptyEntitiesList->GetSelection();
	if ( index == wxNOT_FOUND )
	{
		return;
	}

	wxTheFrame->GetWorldEditPanel()->LookAtNode( m_emptyEntities[index] );
}

void CEdEmptyEntityCollector::DeleteSelected()
{
	// Collect all used layers
	TDynArray< CLayer* > layers;
	for ( Uint32 i=0; i<m_emptyEntities.Size(); ++i )
	{
		if ( m_emptyEntitiesList->IsChecked( (int)i ) )
		{
			layers.PushBackUnique( m_emptyEntities[i]->GetLayer() );
		}
	}

	// Try to mark them as modified
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		if ( (*it)->GetLayerInfo() )
		{
			(*it)->GetLayerInfo()->MarkModified();
		}
		(*it)->MarkModified();
	}

	// Delete the entities that belong in modified layers
	for ( Uint32 i=0; i<m_emptyEntities.Size(); ++i )
	{
		CEntity* entity = m_emptyEntities[i];

		if ( m_emptyEntitiesList->IsChecked( (int)i ) && entity->GetLayer()->IsModified() )
		{
			entity->Destroy();
		}
	}

	// Refresh
	Refresh();
}

void CEdEmptyEntityCollector::SelectAll()
{
	m_emptyEntitiesList->Freeze();
	for ( unsigned int i=0; i<m_emptyEntitiesList->GetCount(); ++i )
	{
		m_emptyEntitiesList->Check( i, true );
	}
	m_emptyEntitiesList->Thaw();
}

void CEdEmptyEntityCollector::SelectNone()
{
	m_emptyEntitiesList->Freeze();
	for ( unsigned int i=0; i<m_emptyEntitiesList->GetCount(); ++i )
	{
		m_emptyEntitiesList->Check( i, false );
	}
	m_emptyEntitiesList->Thaw();
}

void CEdEmptyEntityCollector::ExportList( Uint32 parts, const wxString& fileName )
{
	for ( Uint32 part=0; part < parts; ++part )
	{
		// Open file for writing
		wxFile file( fileName + wxString::Format( wxT("_part%d"), (int)part ), wxFile::write );
		if ( !file.IsOpened() )
		{
			wxMessageBox( wxT("Failed to create the file"), wxT("Error"), wxICON_ERROR | wxOK );
			return;
		}

		// Calculate first and last entity (not included)
		Uint32 entitiesPerPart = m_emptyEntities.Size()/parts;
		Uint32 first = part * entitiesPerPart;
		Uint32 last = part == parts - 1 ? m_emptyEntities.Size() : first + entitiesPerPart;

		// Save the entity GUIDs
		for ( Uint32 i=first; i < last; ++i )
		{
			CGUID guid = m_emptyEntities[i]->GetGUID();
			file.Write( guid.guid, sizeof( guid.guid ) );
		}
		file.Close();
	}
}

void CEdEmptyEntityCollector::ImportList( const wxString& fileName )
{
	// Open file for reading
	wxFile file( fileName, wxFile::read );
	if ( !file.IsOpened() )
	{
		wxMessageBox( wxT("Failed to open the file"), wxT("Error"), wxICON_ERROR | wxOK );
		return;
	}

	// Clear local lists
	m_emptyEntities.Clear();
	m_emptyEntitiesList->Clear();

	// Read GUIDs
	TDynArray< CGUID > guids;
	while ( !file.Eof() )
	{
		CGUID guid;
		file.Read( &guid.guid, sizeof( guid.guid ) );
		guids.PushBackUnique( guid );
	}

	// Fill lists
	Bool missing = false;
	for ( Uint32 i=0; i < guids.Size(); ++i )
	{
		if ( m_foundEntities.KeyExist( guids[i] ) && !m_entitiesInStreaming.Exist( guids[i] ) )
		{
			CEntity* entity = m_foundEntities[guids[i]];
			m_emptyEntities.PushBack( entity );
			m_emptyEntitiesList->Append( wxString::Format( wxT("%s (%s)"), entity->GetName().AsChar(), entity->GetLayer()->GetDepotPath().AsChar() ) );
		}
		else
		{
			missing = true;
		}
	}

	file.Close();

	// Warn if there were missing entities
	if ( missing )
	{
		wxMessageBox( wxT("Some entities in the list were missing from the world. Make sure the relevant layers are loaded."), wxT("Missing Entities"), wxOK | wxICON_WARNING );
	}
}

void CEdEmptyEntityCollector::OnEmptyEntitiesDoubleClick( wxCommandEvent& event )
{
	OnFocusClicked( event );
}

void CEdEmptyEntityCollector::OnRefreshClicked( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ Refresh(); } );
}

void CEdEmptyEntityCollector::OnFocusClicked( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ Focus(); } );
}

void CEdEmptyEntityCollector::OnDeleteClicked( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ DeleteSelected(); } );
}

void CEdEmptyEntityCollector::OnSelectAllClicked( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ SelectAll(); } );
}

void CEdEmptyEntityCollector::OnSelectNoneClicked( wxCommandEvent& event )
{
	RunLaterOnce( [ this ](){ SelectNone(); } );
}

void CEdEmptyEntityCollector::OnCloseClicked( wxCommandEvent& event )
{
	DestroyLater( this );
}

void CEdEmptyEntityCollector::OnExportClicked( wxCommandEvent& event )
{
	// Ask the user for parts and filename
	Uint32 parts = (Uint32)wxGetNumberFromUser( wxT("The empty entities list can be exported to multiple parts for distributing the entity inspection to multiple people."), wxT("Enter number of parts (at least 1)"), wxT("Export entity list"), 1, 1, 4096, this );
	wxString filename = wxFileSelector( wxT("Export to file"), wxEmptyString, wxT("empty_entities"), wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_SAVE|wxFD_OVERWRITE_PROMPT );
	if ( filename.IsEmpty() )
	{
		return;
	}

	// Set a task to perform the export
	RunLaterOnce( [ this, parts, filename ](){ ExportList( parts, filename ); } );
}

void CEdEmptyEntityCollector::OnImportClicked( wxCommandEvent& event )
{
	// Ask the user for parts and filename
	wxString filename = wxFileSelector( wxT("Import from file"), wxEmptyString, wxT("empty_entities_part0"), wxEmptyString, wxFileSelectorDefaultWildcardStr, wxFD_OPEN|wxFD_FILE_MUST_EXIST );
	if ( filename.IsEmpty() )
	{
		return;
	}

	// Set a task to perform the import
	RunLaterOnce( [ this, filename ](){ ImportList( filename ); } );
}
