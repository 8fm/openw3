/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include <wx/wupdlock.h>
#include "entitiesBrowser.h"
#include "assetBrowser.h"
#include "sceneExplorer.h"
#include "../../common/core/diskFile.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/layerInfo.h"
#include "../../common/engine/worldIterators.h"


// Event table
BEGIN_EVENT_TABLE( CEntitiesBrowser, wxPanel )
	EVT_SHOW( CEntitiesBrowser::OnShow )

	EVT_TOOL( XRCID("refresh"), CEntitiesBrowser::OnRefresh )
	EVT_TOOL( XRCID("copyToClipboard"), CEntitiesBrowser::OnCopyToClipboard )
	EVT_TOOL( XRCID("openObjectInspector"), CEntitiesBrowser::OnObjectInspector )
	EVT_TOOL( XRCID("selectAll"), CEntitiesBrowser::OnSelectAll )

	EVT_BUTTON( XRCID("buttonFind"), CEntitiesBrowser::OnFind )
	EVT_RADIOBUTTON( XRCID("searchEntitiesMode"), CEntitiesBrowser::OnModeChange )
	EVT_RADIOBUTTON( XRCID("searchEntityTemplatesMode"), CEntitiesBrowser::OnModeChange )
	EVT_TEXT_ENTER( XRCID("textCtrlFind"), CEntitiesBrowser::OnFind )
END_EVENT_TABLE()

CEntitiesBrowser::CEntitiesBrowser( wxWindow* parent )
	: m_entitiesData( nullptr )
{
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("EntitiesBrowser") );

	// Get GUI
	m_textCtrlFind = XRCCTRL( *this, "textCtrlFind", wxTextCtrl );
	m_checkBoxNameFind = XRCCTRL( *this, "checkBoxNameFind", wxCheckBox );
	m_checkBoxTypeFind = XRCCTRL( *this, "checkBoxTypeFind", wxCheckBox );
	m_checkBoxPathFind = XRCCTRL( *this, "checkBoxPathFind", wxCheckBox );
	m_checkBoxTagFind = XRCCTRL( *this, "checkBoxTagFind", wxCheckBox );
	m_matchWholeWord = XRCCTRL( *this, "matchWholeWorld", wxCheckBox );
	m_matchCase = XRCCTRL( *this, "matchCase", wxCheckBox );
	m_inverseSearch = XRCCTRL( *this, "inverseSearch", wxCheckBox );
	m_searchEntities = XRCCTRL( *this, "searchEntitiesMode", wxRadioButton );
	m_searchEntityTemplates = XRCCTRL( *this, "searchEntityTemplatesMode", wxRadioButton );
	m_showSelected = XRCCTRL( *this, "showSelected", wxCheckBox );

	// create
	m_entitiesGrid = XRCCTRL( *this, "entitiesGrid", wxGrid );
	m_entitiesGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEntitiesBrowser::OnLeftClick, this );
 	m_entitiesGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEntitiesBrowser::OnGotoEntity, this );

	// Set correct values for grid
	m_entitiesGrid->CreateGrid( 0, 4, wxGrid::wxGridSelectRows );
	m_entitiesGrid->SetColLabelSize( 20 );
	m_entitiesGrid->SetRowLabelSize( 1 );
	m_entitiesGrid->SetColLabelValue( 0, TXT("Name") );
	m_entitiesGrid->SetColLabelValue( 1, TXT("Type") );
	m_entitiesGrid->SetColLabelValue( 2, TXT("Path") );
	m_entitiesGrid->SetColLabelValue( 3, TXT("Tags") );
	m_entitiesGrid->SetColSize( 0, 150 );
	m_entitiesGrid->SetColSize( 1, 100 );
	m_entitiesGrid->SetColSize( 2, 300 );
	m_entitiesGrid->SetColSize( 3, 150 );

	// Load and cache all entities
	m_entitiesData.Reset( new CEntitiesDataEntity() );
	InitializeAll();

	// Register listeners
	SEvents::GetInstance().RegisterListener( CNAME( WorldLoaded ), this );
	SEvents::GetInstance().RegisterListener( CNAME( WorldUnloaded ), this );
}

CEntitiesBrowser::~CEntitiesBrowser()
{
	/* intentionally empty */
}

void CEntitiesBrowser::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if( name == CNAME( WorldLoaded ) || name == CNAME( WorldUnloaded ) )
	{
		InitializeAll();
		ApplyFilter();
	}
}

void CEntitiesBrowser::InitializeAll()
{
	if ( !IsShownOnScreen() )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Collecting entities"), false );

	m_entitiesData->Initialize( m_showSelected->GetValue() );

	m_filteredToEntitiesIdx.Clear();
	for( Uint32 i = 0; i < m_entitiesData->Size(); ++i )
	{
		m_filteredToEntitiesIdx.PushBack( i );
	}

	RefreshGUI(); // We have to synchronize GUI cache with Data

	GFeedback->EndTask();
}

void CEntitiesBrowser::ClearGUI()
{
	// clear grid
	if( m_entitiesGrid->GetNumberRows() > 0 )
	{
		m_entitiesGrid->DeleteRows( 0, m_entitiesGrid->GetNumberRows() );
	}
}

void CEntitiesBrowser::RefreshGUI()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	ClearGUI();

	const Uint32 rowCount = m_filteredToEntitiesIdx.Size();
	m_entitiesGrid->AppendRows( rowCount, false );

	for(Uint32 i = 0; i < rowCount; ++i )
	{
		const Int32 filteredIndex = m_filteredToEntitiesIdx[i];

		m_entitiesGrid->DisableRowResize( i );
		m_entitiesGrid->SetCellValue( i, 0, m_entitiesData->GetName( filteredIndex ).AsChar() );
		m_entitiesGrid->SetCellValue( i, 1, m_entitiesData->GetType( filteredIndex ).AsChar() );
		m_entitiesGrid->SetCellValue( i, 2, m_entitiesData->GetPath( filteredIndex ).AsChar() );
		m_entitiesGrid->SetCellValue( i, 3, m_entitiesData->GetTags( filteredIndex ).AsChar() );
	}
}

void CEntitiesBrowser::ApplyFilter()
{
	if ( !IsShownOnScreen() )
	{
		return;
	}

	GFeedback->BeginTask( TXT("Applying filters"), false );

	m_filteredToEntitiesIdx.Clear();

	//
	String searchText = m_textCtrlFind->GetValue();
	searchText.Trim();
	if( m_matchCase->IsChecked() == false )
	{
		searchText.MakeLower();
	}

	const Uint32 rowCount = m_entitiesData->Size();
	for ( Uint32 i=0; i<rowCount; ++i )
	{
		if( m_entitiesData->IsValid( i ) == true )
		{
			if( m_checkBoxNameFind->IsChecked() == true )
			{
				if( CompareTexts( m_entitiesData->GetName( i ), searchText, i ) == true )
				{
					continue;
				}
			}
			if( m_checkBoxTypeFind->IsChecked() == true )
			{
				if( CompareTexts( m_entitiesData->GetType( i ), searchText, i ) == true )
				{
					continue;
				}
			}
			if( m_checkBoxPathFind->IsChecked() == true )
			{
				if( CompareTexts( m_entitiesData->GetPath( i ), searchText, i ) == true )
				{
					continue;
				}
			}
			if( m_checkBoxTagFind->IsChecked() == true )
			{
				TDynArray< String > searchTextTokens = searchText.Split( TXT(";") );
				const Uint32 tokenCount = searchTextTokens.Size();
				for( Uint32 j=0; j<tokenCount; ++j )
				{
					Bool tagFound = false;

					const TagList& tagList = m_entitiesData->GetTagList( i );
					const Uint32 tagCount = tagList.Size();
					for( Uint32 k=0; k<tagCount; ++k )
					{
						if( CompareTexts( tagList.GetTag( k ).AsString(), searchTextTokens[j], i ) == true )
						{
							tagFound = true;
							break;
						}
					}

					if( tagFound ==  true )
					{
						break;
					}
				}
			}
		}
	}

	RefreshGUI();

	GFeedback->EndTask();
}

void CEntitiesBrowser::OnRefresh( wxCommandEvent &event )
{
	InitializeAll();
}

void CEntitiesBrowser::OnFind( wxCommandEvent &event )
{
	ApplyFilter();
}

void CEntitiesBrowser::OnGotoEntity( wxGridEvent &event )
{
	Int32 selectedRow = event.GetRow();

	if( selectedRow != -1 && selectedRow < m_entitiesGrid->GetNumberRows() )
	{
		m_entitiesData->Goto( m_filteredToEntitiesIdx[selectedRow] );
	}
}

void CEntitiesBrowser::UpdateWorldSelection()
{
	m_entitiesData->ClearSelection();
	wxTheFrame->GetSceneExplorer()->SelectSceneObject( nullptr, true );

	wxArrayInt selections = m_entitiesGrid->GetSelectedRows();
	const Uint32 selectionCount = selections.GetCount();
	for( Uint32 i = 0; i < selectionCount; ++i )
	{
		Uint32 m_worldEntitiesIdx = m_filteredToEntitiesIdx[ selections[i] ];
		m_entitiesData->Select( m_worldEntitiesIdx );
	}
}

void CEntitiesBrowser::OnLeftClick( wxGridEvent &event )
{
	Int32 currentRow = event.GetRow();

	if ( event.ShiftDown() && !m_entitiesGrid->GetSelectedRows().IsEmpty() )
	{
		// find last selected row and select all rows between last selected and current row
		Int32 lastSelected = m_entitiesGrid->GetSelectedRows().Last();
		Int32 step = currentRow > lastSelected ? 1 : -1;
		for ( Int32 i = lastSelected + step; i != currentRow; i += step )
		{
			m_entitiesGrid->SelectRow( i, true );
		}
		m_entitiesGrid->SelectRow( currentRow, true );
	}
	else
	{
		if ( !event.ControlDown() && !m_entitiesGrid->GetSelectedRows().IsEmpty() )
		{
			m_entitiesGrid->ClearSelection();
		}

		if ( m_entitiesGrid->GetSelectedRows().Index( currentRow ) < 0 )
		{
			m_entitiesGrid->SelectRow( currentRow, true );
		}
		else
		{
			m_entitiesGrid->DeselectRow( currentRow );
		}
	}

	UpdateWorldSelection();
}

void CEntitiesBrowser::OnCopyToClipboard( wxCommandEvent &event )
{
	// Write some text to the clipboard
	if (wxTheClipboard->Open())
	{
		wxString clipboardString;
		for ( Uint32 i = 0; i < m_entitiesData->Size(); ++i )
		{
			clipboardString += m_entitiesData->GetShortInfo( i ).AsChar();
			clipboardString += wxT("\r\n");
		}
		// This data objects are held by the clipboard, 
		// so do not delete them in the app.
		wxTheClipboard->SetData( new wxTextDataObject(clipboardString) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
}

void CEntitiesBrowser::OnObjectInspector( wxCommandEvent &event )
{
	wxArrayInt selections = m_entitiesGrid->GetSelectedRows();
	const Uint32 selectionCount = selections.GetCount();
	if ( selectionCount == 0 ) return;
	int selected = selections[0];
 
 	Uint32 m_worldEntitiesIdx = m_filteredToEntitiesIdx[ selected ];

 	if ( !m_entitiesData->OnObjectInspector( this, m_worldEntitiesIdx ) )
 	{
 		wxMessageBox( wxT("Object inspector not available"), wxT("Warning") );
 	}
}

void CEntitiesBrowser::OnSelectAll( wxCommandEvent& event )
{
	const Uint32 rowCount = (Uint32)m_entitiesGrid->GetNumberRows();
	for( Uint32 i=0; i < rowCount; ++i )
	{
		m_entitiesGrid->SelectRow( i, true );
	}
	
	UpdateWorldSelection();	
}

void CEntitiesBrowser::OnModeChange( wxCommandEvent& event )
{
	ASSERT( m_searchEntities );
	ASSERT( m_searchEntityTemplates );

	if( m_searchEntities->GetValue() == true )
	{
		m_entitiesData.Reset( new CEntitiesDataEntity() );
	}
	else if( m_searchEntityTemplates->GetValue() == true )
	{
		m_entitiesData.Reset( new CEntitiesDataEntityTemplate() );
	}

	ClearGUI();
	InitializeAll();
}

Bool CEntitiesBrowser::CompareTexts( const String& text, const String& searchText, Uint32 dataIndex )
{
	String internalText = text;
	internalText.Trim();

	Bool noCase = ( m_matchCase->IsChecked() == false );
	if( noCase == true )
	{
		internalText.MakeLower();
	}

	if( m_matchWholeWord->IsChecked() == true )
	{
		if( internalText == searchText )
		{
			if( m_inverseSearch->IsChecked() == false )
			{
				m_filteredToEntitiesIdx.PushBackUnique( dataIndex );
				return true;
			}
		}
		else
		{
			if( m_inverseSearch->IsChecked() == true )
			{
				m_filteredToEntitiesIdx.PushBackUnique( dataIndex );
				return true;
			}
		}

	}
	else
	{
		if( internalText.ContainsSubstring( searchText, false, 0, noCase ) == true )
		{
			if( m_inverseSearch->IsChecked() == false )
			{
				m_filteredToEntitiesIdx.PushBackUnique( dataIndex );
				return true;
			}
		}
		else
		{
			if( m_inverseSearch->IsChecked() == true )
			{
				m_filteredToEntitiesIdx.PushBackUnique( dataIndex );
				return true;
			}
		}
	}

	return false;
}

void CEntitiesBrowser::OnShow( wxShowEvent& event )
{
	InitializeAll();
}

//////////////////////////////////////////////////////////////////////////

void CEntitiesDataEntity::Initialize( Bool onlySelected )
{
	// Reset data
	m_worldEntities.Clear();

	// Check prerequisites
	if ( GGame->GetActiveWorld() == nullptr )
	{
		return;
	}

	// Get all world entities
	// TW> Side note: the whole browser thing should operate on iterators, not on it's own copied table.
	// TW> For now, I'm leaving it as it is, there are more important things to solve right now.
	for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CEntity* entity = ( *it );
		if( onlySelected == true )
		{
			if( entity->IsSelected() == true )
			{
				m_worldEntities.PushBack( entity );
			}
		}
		else
		{
			m_worldEntities.PushBack( entity );
		}
	}
}

Uint32 CEntitiesDataEntity::Size() const
{
	return m_worldEntities.Size();
}

const TagList & CEntitiesDataEntity::GetTagList( Uint32 i ) const
{
	return m_worldEntities[ i ].Get()->GetTags();
}

String CEntitiesDataEntity::GetShortInfo( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();

		String className = entity->GetClass()->GetName().AsString();

		String layerPath;
		if ( entity->GetLayer() && entity->GetLayer()->GetLayerInfo() )
		{
			entity->GetLayer()->GetLayerInfo()->GetHierarchyPath( layerPath, true );
		}

		return className + TXT(" > ") + layerPath + TXT(" : ") + entity->GetName() + TXT( " Tags: ") + entity->GetTags().ToString();
	}
	return TXT("Invalid object");
}

Bool CEntitiesDataEntity::IsValid( Uint32 i ) const
{
	if ( i >= m_worldEntities.Size() ) return false;

	return m_worldEntities[ i ].Get() != nullptr ? true : false;;
}

void CEntitiesDataEntity::ClearSelection() const
{
	CWorld *activeWorld = GGame->GetActiveWorld();
	if ( activeWorld == nullptr ) return;
	wxTheFrame->GetWorldEditPanel()->GetSelectionManager()->DeselectAll();
}

void CEntitiesDataEntity::Select( Uint32 i ) const
{
	CWorld *activeWorld = GGame->GetActiveWorld();
	if ( activeWorld == nullptr ) return;

	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();
		wxTheFrame->GetWorldEditPanel()->GetSelectionManager()->Select( entity );

		wxTheFrame->GetSceneExplorer()->SelectSceneObject( entity, false );
	}
}

void CEntitiesDataEntity::Goto( Uint32 i ) const
{
	CWorld *activeWorld = GGame->GetActiveWorld();
	if ( activeWorld == nullptr ) return;

	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();

		// select entity
		wxTheFrame->GetWorldEditPanel()->GetSelectionManager()->Select( entity );		
		wxTheFrame->GetSceneExplorer()->SelectSceneObject( entity, false );

		wxTheFrame->GetWorldEditPanel()->GetSelectionManager()->DeselectAll();
		wxTheFrame->GetWorldEditPanel()->GetSelectionManager()->Select( entity );

		// go to entity
		wxTheFrame->GetWorldEditPanel()->SetCameraPosition( entity->GetWorldPosition() );
	}
}

Bool CEntitiesDataEntity::OnObjectInspector( wxWindow* parent, Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();
		InspectObject( entity );
		return true;
	}

	return false;
}

String CEntitiesDataEntity::GetName( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();
		return entity->GetName();
	}
	return TXT("Invalid object");
}

String CEntitiesDataEntity::GetType( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();
		return entity->GetClass()->GetName().AsString();
	}
	return TXT("Invalid object");
}

String CEntitiesDataEntity::GetPath( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		String layerPath = String::EMPTY;

		CEntity *entity = m_worldEntities[ i ].Get();
		if ( entity->GetLayer() != nullptr && entity->GetLayer()->GetLayerInfo() != nullptr )
		{
			entity->GetLayer()->GetLayerInfo()->GetHierarchyPath( layerPath, true );
		}

		return layerPath;
	}
	return TXT("Invalid object");
}

String CEntitiesDataEntity::GetTags( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntity *entity = m_worldEntities[ i ].Get();
		return entity->GetTags().ToString();
	}
	return TXT("Invalid object");
}


//////////////////////////////////////////////////////////////////////////


void CEntitiesDataEntityTemplate::Initialize( Bool onlySelected )
{
	// Reset data
	m_worldEntityTemplates.Clear();

	// Check prerequisites
	if ( GGame->GetActiveWorld() == nullptr ) return;

	for ( WorldAttachedEntitiesIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CEntity* entity = ( *it );
		CEntityTemplate *entityTemplate = entity->GetEntityTemplate();

		if( entityTemplate != nullptr )
		{
			if( onlySelected == true )
			{
				if( entity->IsSelected() == true )
				{
					m_worldEntityTemplates.PushBackUnique( entityTemplate );
				}
			}
			else
			{
				m_worldEntityTemplates.PushBackUnique( entityTemplate );
			}
		}
	}
}

Uint32 CEntitiesDataEntityTemplate::Size() const
{
	return m_worldEntityTemplates.Size();
}

const TagList & CEntitiesDataEntityTemplate::GetTagList( Uint32 i ) const
{
	return m_worldEntityTemplates[ i ]->GetEntityObject()->GetTags();
}

String CEntitiesDataEntityTemplate::GetShortInfo( Uint32 i ) const
{
	return m_worldEntityTemplates[ i ]->GetFriendlyName();
}

Bool CEntitiesDataEntityTemplate::IsValid( Uint32 i ) const
{
	if ( i >= m_worldEntityTemplates.Size() ) return false;

	return m_worldEntityTemplates[ i ].Get() != nullptr ? true : false;;
}

void CEntitiesDataEntityTemplate::ClearSelection() const
{
	/* intentionally empty */
}

void CEntitiesDataEntityTemplate::Select( Uint32 i ) const
{
	/* intentionally empty */
}

void CEntitiesDataEntityTemplate::Goto( Uint32 i ) const
{
	if ( m_worldEntityTemplates[ i ]->GetFile() )
	{
#ifndef NO_EDITOR_EVENT_SYSTEM
		SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( m_worldEntityTemplates[ i ]->GetFile()->GetDepotPath() ) );
#endif	// NO_EDITOR_EVENT_SYSTEM
	}
}

String CEntitiesDataEntityTemplate::GetName( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntityTemplate* entityTemplate = m_worldEntityTemplates[ i ].Get();
		CFilePath path( entityTemplate->GetDepotPath() );
		return path.GetFileName();
	}
	return TXT("Invalid object");
}

String CEntitiesDataEntityTemplate::GetType( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntityTemplate* entityTemplate = m_worldEntityTemplates[ i ].Get();
		return entityTemplate->GetClass()->GetName().AsString();
	}
	return TXT("Invalid object");
}

String CEntitiesDataEntityTemplate::GetPath( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntityTemplate* entityTemplate = m_worldEntityTemplates[ i ].Get();
		return entityTemplate->GetDepotPath();
	}
	return TXT("Invalid object");
}

String CEntitiesDataEntityTemplate::GetTags( Uint32 i ) const
{
	if( IsValid( i ) == true )
	{
		CEntityTemplate* entityTemplate = m_worldEntityTemplates[ i ].Get();
		return entityTemplate->GetEntityObject()->GetTags().ToString();
	}
	return TXT("Invalid object");
}
