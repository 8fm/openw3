/*
* Copyright © 2013 CD Proiekt Red. All Rights Reserved.
*/
#include "build.h"
#include <wx/wupdlock.h>
#include "meshStats.h"
#include "resourceFinder.h"
#include "sceneExplorer.h"
#include "worldSceneDebugger.h"
#include "../../common/engine/mesh.h"

#include "../../common/core/depot.h"
#include "wxThumbnailImageLoader.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/clipMap.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/engine/foliageBroker.h"
#include "../../common/engine/renderer.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/apexResource.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/worldIterators.h"
#include "../../common/engine/baseTree.h"

namespace
{
	const Uint32 GConstMenuItems = 3;

	enum ETabType
	{
		TT_Entities,
		TT_Meshes,
		TT_Textures,
		TT_TextureArray,
		TT_Apex,
		TT_SpeedTree,

		TT_Count,
	};


	enum EComparerType
	{
		CT_Integer,
		CT_Real,
		CT_Text,
		CT_None,

		CT_Count,
	};

	// tabs
	const Char* GTabNames[TT_Count] = { TXT("Entities"), TXT("Meshes"), TXT("Textures"), TXT("Texture Array"), TXT("Apex"), TXT("Speed Tree") };
	wxGrid* GHelperGrids[TT_Count] = { nullptr };
	wxMenu* GContextMenus[TT_Count] = { nullptr };

	// entities grid
	const Uint32 GEntitiesColCount = 11;
	const Char* GEntitiesColName[GEntitiesColCount] = { TXT("Name"), TXT("Ref count"), TXT("Path"), TXT("Data size [MB]"), TXT("Used mesh count"), TXT("Meshes"), TXT("Layers"), TXT("Duplicated"),
														TXT("Streaming dist"), TXT("Avg componens strDist"), TXT("Max - avg") };
	const EComparerType GEntitiesColComparerType[GEntitiesColCount] = { CT_Text, CT_Integer, CT_Text, CT_Real, CT_Integer, CT_None, CT_Text, CT_Integer, CT_Integer, CT_Integer, CT_Integer };

	// meshes grid
	const Uint32 GMeshesColCount = 12;
	const Char* GMeshesColName[GMeshesColCount] = { TXT("Name"), TXT("Ref count"), TXT("Path"), TXT("Data size [MB]"), TXT("LODs count"), TXT("Scene tri count"), TXT("Scene vert count"), TXT("Material chunks"), 
		TXT("Used textures"), TXT("Textures data size [MB]"), TXT("Texture count"), TXT("Autohide dist") };
	const EComparerType GMeshesColComparerType[GMeshesColCount] = { CT_Text, CT_Integer, CT_Text, CT_Real, CT_Integer, CT_Integer, CT_Integer, CT_Integer, CT_None, CT_Real, CT_Integer, CT_Real };

	// textures grid
	const Uint32 GTexturesColCount = 11;
	const Char* GTexturesColName[GTexturesColCount] = { TXT("Name"), TXT("Ref count"), TXT("Path"), TXT("Data size [MB]"), TXT("Width"), TXT("Height"),
		TXT("MipMaps"),TXT("Used by mesh count"), TXT("Used by mesh"), TXT("Used by texture array count"), TXT("Used by texture array") };
	const EComparerType GTexturesColComparerType[GTexturesColCount] = { CT_Text, CT_Integer, CT_Text, CT_Real, CT_Integer, CT_Integer, CT_Integer, 
		CT_Integer, CT_None, CT_Integer, CT_None };

	// texture arrays grid
	const Uint32 GTextureArrayColCount = 8;
	const Char* GTextureArrayColName[GTextureArrayColCount] = { TXT("Name"), TXT("Ref count"), TXT("Path"), TXT("Data size [MB]"), TXT("Contained texture count"),
		TXT("Contained textures"), TXT("Used by mesh count"), TXT("Used by mesh") };
	const EComparerType GTextureArrayColComparerType[GTextureArrayColCount] = { CT_Text, CT_Integer, CT_Text, CT_Real, CT_Integer, CT_None, CT_Integer, CT_None };

	// apex grid
	const Uint32 GApexColCount = 7;
	const Char* GApexColName[GApexColCount] = { TXT("Name"), TXT("Ref count"), TXT("Path"), TXT("Apex type"), TXT("LODs count"), TXT("Triangles"), TXT("Vertices") };
	const EComparerType GApexColComparerType[GApexColCount] = { CT_Text, CT_Integer, CT_Text, CT_None, CT_Integer, CT_Integer, CT_Integer };

	// speed tree grid
	const Uint32 GSpeedTreeColCount = 7;
	const Char* GSpeedTreeColName[GSpeedTreeColCount] = { TXT("Name"), TXT("Ref count"), TXT("Path"), TXT("Data size [MB]"), TXT("Texture count"), TXT("Textures data size [MB]"), TXT("Used textures") };
	const EComparerType GSpeedTreeColComparerType[GSpeedTreeColCount] = { CT_Text, CT_Integer, CT_Text, CT_Real, CT_Integer, CT_Real, CT_None };
}

// this is simpler way of connecting menu items with callback functions
BEGIN_EVENT_TABLE( CEdWorldSceneDebugger, wxFrame )
	EVT_MENU( XRCID("saveAsCSV"), CEdWorldSceneDebugger::OnExportToCSV )
	EVT_MENU( XRCID("refreshData"), CEdWorldSceneDebugger::OnRefreshData )
	EVT_MENU( XRCID("scrollToSelectedEntity"), CEdWorldSceneDebugger::OnScrollToSelectedEntity )
	EVT_TOGGLEBUTTON( XRCID("autohideToggle"), CEdWorldSceneDebugger::OnAutohideToggle )
	EVT_NOTEBOOK_PAGE_CHANGED( XRCID("modeNotebook"), CEdWorldSceneDebugger::OnModeTabChanged )

	// sliders
	EVT_SLIDER( XRCID("foliageDistanceScaleSlider"), CEdWorldSceneDebugger::OnSliderChanged )
	EVT_SLIDER( XRCID("grassDistanceScaleSlider"), CEdWorldSceneDebugger::OnSliderChanged )
	EVT_SLIDER( XRCID("meshRenderDistanceScaleSlider"), CEdWorldSceneDebugger::OnSliderChanged )
	EVT_SLIDER( XRCID("meshLODDistanceScaleSlider"), CEdWorldSceneDebugger::OnSliderChanged )

	// spinners
	EVT_SPINCTRL( XRCID("foliageDistanceScaleSpinner"), CEdWorldSceneDebugger::OnSpinnerChanged )
	EVT_SPINCTRL( XRCID("grassDistanceScaleSpinner"), CEdWorldSceneDebugger::OnSpinnerChanged )
	EVT_SPINCTRL( XRCID("meshRenderDistanceScaleSpinner"), CEdWorldSceneDebugger::OnSpinnerChanged )
	EVT_SPINCTRL( XRCID("meshLODDistanceScaleSpinner"), CEdWorldSceneDebugger::OnSpinnerChanged )

END_EVENT_TABLE()

Uint32 CEdWorldSceneDebugger::SInternalEntityTemplateInfo::m_supposedDuplicateEntityCount = 0;

CEdWorldSceneDebugger::CEdWorldSceneDebugger( wxWindow* parent )
	: m_dataCollected( false )
	, m_treeDistCurrent( 1.f )
	, m_treeDistPrevious( 1.f )
	, m_grassDistPrevious( 1.f )
	, m_grassDistCurrent( 1.f )
	, m_meshRenderDistCurrent( 1.f )
	, m_meshRenderDistPrevious( 1.f )
	, m_meshLODRenderDistCurrent( 1.f )
	, m_meshLODRenderDistPrevious( 1.f )
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( EnginePerformancePlatformChanged ), this );
#endif
	wxXmlResource::Get()->LoadFrame( this, parent, wxT("WorldSceneDebuggerFrame") );
	
	LoadAndCreateGeneralControls();
	LoadAndCreateEntitiesTab();
	LoadAndCreateMeshesTab();
	LoadAndCreateTexturesTab();
	LoadAndCreateTextureArraysTab();
	LoadAndCreateApexTab();
	LoadAndCreateSpeedTreeTab();

	UpdateForceNoAutohideButtonLabel( CDrawableComponent::IsForceNoAutohideDebug() );
}

CEdWorldSceneDebugger::~CEdWorldSceneDebugger()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( this );
#endif
}

void CEdWorldSceneDebugger::LoadAndCreateGeneralControls()
{
	// load general controls
	m_notebook = XRCCTRL( *this, "notebook", wxNotebook );

	// load sub frames
	m_exportDialogWindow = wxXmlResource::Get()->LoadFrame( this, "ExportToCSVWindow" );
	{
		m_tabsListToExport = XRCCTRL( *m_exportDialogWindow, "tabsListToExport", wxCheckListBox );
		m_exportToFile = XRCCTRL( *m_exportDialogWindow, "exportToFile", wxRadioBox );
		m_selectAllTabsToExport = XRCCTRL( *m_exportDialogWindow, "selectAllTabsToExport", wxCheckBox );
		m_selectAllTabsToExport->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CEdWorldSceneDebugger::OnSelectAllTabsToExport, this );
		m_exportOnlyDuplicated = XRCCTRL( *m_exportDialogWindow, "onyDuplicatedCheckbox", wxCheckBox );

		wxButton* exportButton = XRCCTRL( *m_exportDialogWindow, "exportButton", wxButton );
		exportButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdWorldSceneDebugger::OnExportButtonClicked, this );
		wxButton* cancelButton = XRCCTRL( *m_exportDialogWindow, "cancelButton", wxButton );
		cancelButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdWorldSceneDebugger::OnExportCancelButtonClicked, this );
	}

	// load general controls
	m_modeNotebook = XRCCTRL( *this, "modeNotebook", wxNotebook );
	{
		m_foliageDistanceSlider = XRCCTRL( *m_modeNotebook, "foliageDistanceScaleSlider", wxSlider );
		m_grassDistSlider = XRCCTRL( *m_modeNotebook, "grassDistanceScaleSlider", wxSlider );
		m_meshRenderDistSlider = XRCCTRL( *m_modeNotebook, "meshRenderDistanceScaleSlider", wxSlider );
		m_meshLODDistSlider = XRCCTRL( *m_modeNotebook, "meshLODDistanceScaleSlider", wxSlider );

		m_foliageSpinner = XRCCTRL( *m_modeNotebook, "foliageDistanceScaleSpinner", wxSpinCtrl );
		m_grassSpinner = XRCCTRL( *m_modeNotebook, "grassDistanceScaleSpinner", wxSpinCtrl );
		m_meshRenderDistSpinner = XRCCTRL( *m_modeNotebook, "meshRenderDistanceScaleSpinner", wxSpinCtrl );
		m_meshLODDistSpinner = XRCCTRL( *m_modeNotebook, "meshLODDistanceScaleSpinner", wxSpinCtrl );
	}
	
	// as default set force render options tab
	m_modeNotebook->SetSelection(1);
	UpdateInternalData();
}

void CEdWorldSceneDebugger::LoadAndCreateEntitiesTab()
{
	// load thumbnail preview
	m_entityThumbnailBitmap = XRCCTRL( *this, "m_entityThumbnailImage", wxStaticBitmap );

	// load controls from XRC file
	m_entitiesGrid = XRCCTRL( *this, "entitiesGrid", wxGrid );
	m_entitiesGrid->SetClientData( m_entityThumbnailBitmap );
	GHelperGrids[TT_Entities] = m_entitiesGrid;
	m_entitiesGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowColumnContextMenu, this );
	m_entitiesGrid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowRowContextMenu, this );
	m_entitiesGrid->Bind( wxEVT_GRID_COL_SORT, &CEdWorldSceneDebugger::OnSortByColumn, this );
	m_entitiesGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEdWorldSceneDebugger::OnRowClicked, this );
	m_entitiesGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEdWorldSceneDebugger::OnOpenEditWindow, this );
	m_entitiesGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdWorldSceneDebugger::UpdateThumbnail, this );

	// load labels with statistics
	m_duplicateEntitiesCount = XRCCTRL( *this, "duplicateEntitiesCount", wxStaticText );
	m_differentEntitiesCount = XRCCTRL( *this, "differentEntitiesCount", wxStaticText );
	m_differentEntitiesDataCount = XRCCTRL( *this, "differentEntitiesDataCount", wxStaticText );
	m_differentEntitiesOccuringOnceCount = XRCCTRL( *this, "differentEntitiesOccuringOnceCount", wxStaticText );
	m_differentDataInEntitiesOccuringOnceCount = XRCCTRL( *this, "differentDataInEntitiesOccuringOnceCount", wxStaticText );

	// Set correct values for grid
	m_entitiesGrid->CreateGrid( 0, GEntitiesColCount, wxGrid::wxGridSelectRows );
	m_entitiesGrid->SetColLabelSize( 20 );
	m_entitiesGrid->SetRowLabelSize( 1 );
	for( Uint32 i=0; i<GEntitiesColCount; ++i )
	{
		m_entitiesGrid->SetColLabelValue( i, GEntitiesColName[i] );
		m_entitiesGrid->SetColSize( i, 300 );
	}
	m_entitiesGrid->HideCol( GEntitiesColCount - 1 );

	// create context menu
	GContextMenus[TT_Entities] = new wxMenu();
	GContextMenus[TT_Entities]->Append( 0, TXT("Show in asset browser"), nullptr );
	GContextMenus[TT_Entities]->Append( 1, TXT("Show in explorer"), nullptr );
	GContextMenus[TT_Entities]->Append( 2, TXT("Show all resource instances"), nullptr );
	GContextMenus[TT_Entities]->Append( 3, TXT("Show in scene explorer"), nullptr );
	GContextMenus[TT_Entities]->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnRowPopupClick, this );
}

void CEdWorldSceneDebugger::LoadAndCreateMeshesTab()
{
	// load thumbnail preview
	m_meshThumbnailBitmap = XRCCTRL( *this, "m_meshThumbnailImage", wxStaticBitmap );

	// load controls from XRC file
	m_meshesGrid = XRCCTRL( *this, "meshesGrid", wxGrid );
	m_meshesGrid->SetClientData( m_meshThumbnailBitmap );
	GHelperGrids[TT_Meshes] = m_meshesGrid;
	m_meshesGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowColumnContextMenu, this );
	m_meshesGrid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowRowContextMenu, this );
	m_meshesGrid->Bind( wxEVT_GRID_COL_SORT, &CEdWorldSceneDebugger::OnSortByColumn, this );
	m_meshesGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEdWorldSceneDebugger::OnRowClicked, this );
	m_meshesGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEdWorldSceneDebugger::OnOpenEditWindow, this );
	m_meshesGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdWorldSceneDebugger::UpdateThumbnail, this );

	// load labels with statistics
	m_differentMeshesCount = XRCCTRL( *this, "differentMeshesCount", wxStaticText );
	m_differentChunksCount = XRCCTRL( *this, "differentChunksCount", wxStaticText );
	m_differentMeshesDataCount = XRCCTRL( *this, "differentMeshesDataCount", wxStaticText );
	m_differentMeshesOccuringOnceCount = XRCCTRL( *this, "differentMeshesOccuringOnceCount", wxStaticText );
	m_differentChunksInMeshesOccuringOnceCount = XRCCTRL( *this, "differentChunksInMeshesOccuringOnceCount", wxStaticText );
	m_differentMeshDataInMeshesOccuringOnceCount = XRCCTRL( *this, "differentMeshDataInMeshesOccuringOnceCount", wxStaticText );

	// Set correct values for grid
	m_meshesGrid->CreateGrid( 0, GMeshesColCount, wxGrid::wxGridSelectRows );
	m_meshesGrid->SetColLabelSize( 20 );
	m_meshesGrid->SetRowLabelSize( 1 );
	for( Uint32 i=0; i<GMeshesColCount; ++i )
	{
		m_meshesGrid->SetColLabelValue( i, GMeshesColName[i] );
		m_meshesGrid->SetColSize( i, 300 );
	}

	// create context menu
	GContextMenus[TT_Meshes] = new wxMenu();
	GContextMenus[TT_Meshes]->Append( 0, TXT("Show in asset browser"), nullptr );
	GContextMenus[TT_Meshes]->Append( 1, TXT("Show in explorer"), nullptr );
	GContextMenus[TT_Meshes]->Append( 2, TXT("Show all resource instances"), nullptr );
	GContextMenus[TT_Meshes]->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnRowPopupClick, this );
}

void CEdWorldSceneDebugger::LoadAndCreateTexturesTab()
{
	// load thumbnail preview
	m_textureThumbnailBitmap = XRCCTRL( *this, "m_textureThumbnailImage", wxStaticBitmap );

	// load controls from XRC file
	m_texturesGrid = XRCCTRL( *this, "texturesGrid", wxGrid );
	m_texturesGrid->SetClientData( m_textureThumbnailBitmap );
	GHelperGrids[TT_Textures] = m_texturesGrid;
	m_texturesGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowColumnContextMenu, this );
	m_texturesGrid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowRowContextMenu, this );
	m_texturesGrid->Bind( wxEVT_GRID_COL_SORT, &CEdWorldSceneDebugger::OnSortByColumn, this );
	m_texturesGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEdWorldSceneDebugger::OnRowClicked, this );
	m_texturesGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEdWorldSceneDebugger::OnOpenEditWindow, this );
	m_texturesGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdWorldSceneDebugger::UpdateThumbnail, this );

	// load labels with statistics
	m_differentTexturesCount = XRCCTRL( *this, "differentTexturesCount", wxStaticText );
	m_differentTexturesDataCount = XRCCTRL( *this, "differentTexturesDataCount", wxStaticText );
	m_differentTextureOccuringOnceCount = XRCCTRL( *this, "differentTextureOccuringOnceCount", wxStaticText );
	m_differentDataInTextureOccuringOnceCount = XRCCTRL( *this, "differentDataInTextureOccuringOnceCount", wxStaticText );

	// Set correct values for grid
	m_texturesGrid->CreateGrid( 0, GTexturesColCount, wxGrid::wxGridSelectRows );
	m_texturesGrid->SetColLabelSize( 20 );
	m_texturesGrid->SetRowLabelSize( 1 );
	for( Uint32 i=0; i<GTexturesColCount; ++i )
	{
		m_texturesGrid->SetColLabelValue( i, GTexturesColName[i] );
		m_texturesGrid->SetColSize( i, 300 );
	}

	// create context menu
	GContextMenus[TT_Textures] = new wxMenu();
	GContextMenus[TT_Textures]->Append( 0, TXT("Show in asset browser"), nullptr );
	GContextMenus[TT_Textures]->Append( 1, TXT("Show in explorer"), nullptr );
	GContextMenus[TT_Textures]->Append( 2, TXT("Show all resource instances"), nullptr );
	GContextMenus[TT_Textures]->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnRowPopupClick, this );
}

void CEdWorldSceneDebugger::LoadAndCreateTextureArraysTab()
{
	// load thumbnail preview
	m_textureArrayThumbnailBitmap = XRCCTRL( *this, "m_textureArrayThumbnailImage", wxStaticBitmap );

	// load controls from XRC file
	m_textureArraysGrid = XRCCTRL( *this, "textureArraysGrid", wxGrid );
	m_textureArraysGrid->SetClientData( m_textureArrayThumbnailBitmap );
	GHelperGrids[TT_TextureArray] = m_textureArraysGrid;
	m_textureArraysGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowColumnContextMenu, this );
	m_textureArraysGrid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowRowContextMenu, this );
	m_textureArraysGrid->Bind( wxEVT_GRID_COL_SORT, &CEdWorldSceneDebugger::OnSortByColumn, this );
	m_textureArraysGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEdWorldSceneDebugger::OnRowClicked, this );
	m_textureArraysGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEdWorldSceneDebugger::OnOpenEditWindow, this );
	m_textureArraysGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdWorldSceneDebugger::UpdateThumbnail, this );

	// load labels with statistics
	m_differentTextureArraysCount = XRCCTRL( *this, "differentTextureArraysCount", wxStaticText );
	m_differentTextureArraysDataCount = XRCCTRL( *this, "differentTextureArraysDataCount", wxStaticText );
	m_differentTextureArraysOccuringOnceCount = XRCCTRL( *this, "differentTextureArraysOccuringOnceCount", wxStaticText );
	m_differentDataInTextureArraysOccuringOnceCount = XRCCTRL( *this, "differentDataInTextureArraysOccuringOnceCount", wxStaticText );

	// Set correct values for grid
	m_textureArraysGrid->CreateGrid( 0, GTextureArrayColCount, wxGrid::wxGridSelectRows );
	m_textureArraysGrid->SetColLabelSize( 20 );
	m_textureArraysGrid->SetRowLabelSize( 1 );
	for( Uint32 i=0; i<GTextureArrayColCount; ++i )
	{
		m_textureArraysGrid->SetColLabelValue( i, GTextureArrayColName[i] );
		m_textureArraysGrid->SetColSize( i, 300 );
	}	

	// create context menu
	GContextMenus[TT_TextureArray] = new wxMenu();
	GContextMenus[TT_TextureArray]->Append( 0, TXT("Show in asset browser"), nullptr );
	GContextMenus[TT_TextureArray]->Append( 1, TXT("Show in explorer"), nullptr );
	GContextMenus[TT_TextureArray]->Append( 2, TXT("Show all resource instances"), nullptr );
	GContextMenus[TT_TextureArray]->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnRowPopupClick, this );
}

void CEdWorldSceneDebugger::LoadAndCreateApexTab()
{
	// load thumbnail preview
	m_apexThumbnailBitmap = XRCCTRL( *this, "m_apexThumbnailImage", wxStaticBitmap );

	// load controls from XRC file
	m_apexGrid = XRCCTRL( *this, "apexGrid", wxGrid );
	m_apexGrid->SetClientData( m_apexThumbnailBitmap );
	GHelperGrids[TT_Apex] = m_apexGrid;
	m_apexGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowColumnContextMenu, this );
	m_apexGrid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowRowContextMenu, this );
	m_apexGrid->Bind( wxEVT_GRID_COL_SORT, &CEdWorldSceneDebugger::OnSortByColumn, this );
	m_apexGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEdWorldSceneDebugger::OnRowClicked, this );
	m_apexGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEdWorldSceneDebugger::OnOpenEditWindow, this );
	m_apexGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdWorldSceneDebugger::UpdateThumbnail, this );

	// load labels with statistics
	m_differentApexClothComponentCount = XRCCTRL( *this, "differentApexClothComponent", wxStaticText );
	m_differentApexDestructibleComponentCount = XRCCTRL( *this, "differentApexDestructibleComponent", wxStaticText );
	m_differentApexComponentOccuringOnceCount = XRCCTRL( *this, "differentApexComponentOccuringOnce", wxStaticText );

	// Set correct values for grid
	m_apexGrid->CreateGrid( 0, GApexColCount, wxGrid::wxGridSelectRows );
	m_apexGrid->SetColLabelSize( 20 );
	m_apexGrid->SetRowLabelSize( 1 );
	for( Uint32 i=0; i<GApexColCount; ++i )
	{
		m_apexGrid->SetColLabelValue( i, GApexColName[i] );
		m_apexGrid->SetColSize( i, 300 );
	}	

	// create context menu
	GContextMenus[TT_Apex] = new wxMenu();
	GContextMenus[TT_Apex]->Append( 0, TXT("Show in asset browser"), nullptr );
	GContextMenus[TT_Apex]->Append( 1, TXT("Show in explorer"), nullptr );
	GContextMenus[TT_Apex]->Append( 2, TXT("Show all resource instances"), nullptr );
	GContextMenus[TT_Apex]->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnRowPopupClick, this );
}

void CEdWorldSceneDebugger::LoadAndCreateSpeedTreeTab()
{
	// load thumbnail preview
	m_speedTreeThumbnailBitmap = XRCCTRL( *this, "m_speedTreeThumbnailImage", wxStaticBitmap );

	// load controls from XRC file
	m_speedTreeGrid = XRCCTRL( *this, "speedTreeGrid", wxGrid );
	m_speedTreeGrid->SetClientData( m_speedTreeThumbnailBitmap );
	GHelperGrids[TT_SpeedTree] = m_speedTreeGrid;
	m_speedTreeGrid->Bind( wxEVT_GRID_LABEL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowColumnContextMenu, this );
	m_speedTreeGrid->Bind( wxEVT_GRID_CELL_RIGHT_CLICK, &CEdWorldSceneDebugger::OnShowRowContextMenu, this );
	m_speedTreeGrid->Bind( wxEVT_GRID_COL_SORT, &CEdWorldSceneDebugger::OnSortByColumn, this );
	m_speedTreeGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &CEdWorldSceneDebugger::OnRowClicked, this );
	m_speedTreeGrid->Bind( wxEVT_GRID_CELL_LEFT_DCLICK, &CEdWorldSceneDebugger::OnOpenEditWindow, this );
	m_speedTreeGrid->Bind( wxEVT_GRID_SELECT_CELL, &CEdWorldSceneDebugger::UpdateThumbnail, this );

	// load labels with statistics
	m_differentSpeedTreeTypeOccuringOnceCount = XRCCTRL( *this, "differentSpeedTreeTypeOccuringOnceCount", wxStaticText );
	m_totalSpeedTreeData = XRCCTRL( *this, "totalSpeedTreeData", wxStaticText );
	m_differentDataInSpeedTreeTypeOccuringOnceCount = XRCCTRL( *this, "differentDataInSpeedTreeTypeOccuringOnceCount", wxStaticText );
	m_speedTreeNumTypes = XRCCTRL( *this, "speedTreeNumTypes", wxStaticText );

	// Set correct values for grid
	m_speedTreeGrid->CreateGrid( 0, GSpeedTreeColCount, wxGrid::wxGridSelectRows );
	m_speedTreeGrid->SetColLabelSize( 20 );
	m_speedTreeGrid->SetRowLabelSize( 1 );
	for( Uint32 i=0; i<GSpeedTreeColCount; ++i )
	{
		m_speedTreeGrid->SetColLabelValue( i, GSpeedTreeColName[i] );
		m_speedTreeGrid->SetColSize( i, 300 );
	}	

	// create context menu
	GContextMenus[TT_SpeedTree] = new wxMenu();
	GContextMenus[TT_SpeedTree]->Append( 0, TXT("Show in asset browser"), nullptr );
	GContextMenus[TT_SpeedTree]->Append( 1, TXT("Show in explorer"), nullptr );
	GContextMenus[TT_SpeedTree]->Append( 2, TXT("Show all resource instances"), nullptr );
	GContextMenus[TT_SpeedTree]->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnRowPopupClick, this );
}

void CEdWorldSceneDebugger::OnExportToCSV( wxCommandEvent& event )
{
	m_exportDialogWindow->Show();
}

void CEdWorldSceneDebugger::OnScrollToSelectedEntity( wxCommandEvent& event )
{
	CSelectionManager *selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
	TDynArray< CEntity* > entities;
	selectionManager->GetSelectedEntities( entities );

	if( entities.Size() == 0 )
	{
		wxMessageBox( wxT("Please select one entity."), wxT("Warning"), wxICON_WARNING | wxOK );
		return;
	}

	if( entities.Size() > 1 )
	{
		wxMessageBox( wxT("Please select only one entity."), wxT("Warning"), wxICON_WARNING | wxOK );
		return;
	}

	const String& entityName = entities[0]->GetDisplayName();

	// select tab with entities grid
	m_notebook->SetSelection( TT_Entities );

	Int32 rowNumber = -1;
	const Uint32 nameColIndex = 0;
	for( int i=0; i<m_entitiesGrid->GetNumberRows(); ++i )
	{
		String text = m_entitiesGrid->GetCellValue( i, nameColIndex ).wc_str();
		if( text == entityName )
		{
			rowNumber = i;
			break;
		}
	}

	if( rowNumber == -1 )
	{
		const CEntityTemplate* entityTemplate = entities[0]->GetEntityTemplate();
		if( entityTemplate != nullptr )
		{
			const String& entityTemplatePath = entityTemplate->GetFile()->GetDepotPath();

			const Uint32 pathColIndex = 2;
			for( int i=0; i<m_entitiesGrid->GetNumberRows(); ++i )
			{
				String path = m_entitiesGrid->GetCellValue( i, pathColIndex ).wc_str();
				if( path == entityTemplatePath )
				{
					rowNumber = i;
					break;
				}
			}
		}
	}

	if( rowNumber != -1 )
	{
		SelectRow( m_entitiesGrid, rowNumber );
		ScrollToSelectedRow();
	}
}

void CEdWorldSceneDebugger::OnSelectAllTabsToExport( wxCommandEvent& event )
{
	Bool state = true;
	if( event.GetInt() == 0 )
	{
		state = false;
	}

	for( Uint32 i=0; i<m_tabsListToExport->GetCount(); ++i )
	{
		m_tabsListToExport->Check( i, state );
	}
}

void CEdWorldSceneDebugger::OnExportButtonClicked( wxCommandEvent& event )
{
	m_exportDialogWindow->Hide();
	Export();
}

void CEdWorldSceneDebugger::OnExportCancelButtonClicked( wxCommandEvent& event )
{
	m_exportDialogWindow->Hide();
}

void CEdWorldSceneDebugger::CollectDataFromWorld()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	GFeedback->BeginTask( TXT("Collecting data"), false );

	// containers for world elements
	TDynArray< CMeshComponent* > meshComponents;

	// container for info about all resources
	TDynArray< SInternalMeshInfo > allMeshesInfos;
	TDynArray< SInternalEntityTemplateInfo > allEntitiesInfos;
	TDynArray< SInternalTextureInfo > allTexturesInfos;
	TDynArray< SInternalTextureArrayInfo > allTextureArraysInfos;
	TDynArray< SInternalApexInfo > allApexInfos;
	TDynArray< SInternalSpeedTreeInfo > allSpeedTreeInfos;

	// helper variables
	Uint32 duplicateEntities = 0;

	CollectMeshesData( meshComponents, allMeshesInfos );
	CollectTexturesData( allMeshesInfos, allTexturesInfos );
	CollectTextureArrayData( allMeshesInfos, allTextureArraysInfos, allTexturesInfos );
	CollectEntityData( meshComponents, allEntitiesInfos, allMeshesInfos );
	CollectApexData( allApexInfos );
	CollectSpeedTreeData( allSpeedTreeInfos );

	// fill gui
	FillMeshesTab( allMeshesInfos );
	FillEntitiesTab( allEntitiesInfos );
	FillTexturesTab( allTexturesInfos );
	FillTextureArraysTab( allTextureArraysInfos );
	FillApexTab( allApexInfos );
	FillSpeedTreeTab( allSpeedTreeInfos );

	GFeedback->EndTask();
}

void CEdWorldSceneDebugger::CollectMeshesData( TDynArray< CMeshComponent* >& meshComponents, TDynArray< SInternalMeshInfo >& allMeshesInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Collecting meshes data ...") );

	// collect mesh components
	GGame->GetActiveWorld()->GetAttachedComponentsOfClass( meshComponents );

	for( Uint32 i=0; i<meshComponents.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress( i, meshComponents.Size() );

		if( meshComponents[i]->GetMeshNow() != nullptr )
		{
			const CMesh* mesh = meshComponents[i]->GetMeshNow();

			Bool foundMeshInfo = false;
			for( Uint32 j=0; j<allMeshesInfos.Size(); ++j )
			{
				if( allMeshesInfos[j].m_mesh == mesh )
				{
					++allMeshesInfos[j].m_refCount;
					foundMeshInfo = true;
					break;
				}
			}

			if( foundMeshInfo == false )
			{
				// collect all chunks for all LODs
				Uint32 chunks = 0;
				Uint32 dataSize = 0;
				Uint32 triCount = 0;
				Uint32 vertCount = 0;

				Uint32 lodCount = mesh->GetMeshLODLevels().Size();

				for( Uint32 j=0; j<lodCount; ++j )
				{
					triCount += mesh->CountLODTriangles( j );
					vertCount += mesh->CountLODVertices( j );
					chunks += mesh->CountLODChunks( j );
					dataSize += ( MeshStatsNamespace::CalcMeshLodRenderDataSize( mesh, j ) );
				}

				// if mesh is not found in collection
				allMeshesInfos.PushBack( SInternalMeshInfo( mesh, dataSize, chunks, triCount, vertCount, lodCount, mesh->GetAutoHideDistance() ) );
			}
		}
	}	
}

void CEdWorldSceneDebugger::CollectEntityData( TDynArray< CMeshComponent* >& meshComponents, TDynArray< SInternalEntityTemplateInfo >& allEntitiesInfos, TDynArray< SInternalMeshInfo >& allMeshesInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Collecting entities data ...") );

	// reset static variable
	SInternalEntityTemplateInfo::m_supposedDuplicateEntityCount = 0;

	for( Uint32 i=0; i<meshComponents.Size(); ++i )
	{
		GFeedback->UpdateTaskProgress( i, meshComponents.Size() );

		const CEntity* entity = meshComponents[i]->GetEntity();
		String usedMeshPath = String::EMPTY;
		Bool meshComponentIsEmpty = true;

		if( entity != nullptr )
		{
			const CEntityTemplate* entityTemplate = entity->GetEntityTemplate();

			if( entityTemplate != nullptr )
			{
				// check whether the current entity template is already saved in container with information about entities
				SInternalEntityTemplateInfo* foundEntityTemplateInfo = nullptr;
				for( Uint32 j=0; j<allEntitiesInfos.Size(); ++j )
				{
					if( allEntitiesInfos[j].m_entityTemplate == entityTemplate )
					{
						foundEntityTemplateInfo = &allEntitiesInfos[j];
						break;
					}
				}

				//
				Bool foundEntity = false;
				if( foundEntityTemplateInfo != nullptr )
				{
					for( Uint32 j=0; j<foundEntityTemplateInfo->m_entities.Size(); ++j )
					{
						if( foundEntityTemplateInfo->m_entities[j] == static_cast< const CEntity* >( entity ) )
						{
							foundEntity = true;
							break;
						}
					}

					//
					if( foundEntity == false )
					{
						++foundEntityTemplateInfo->m_refCount;
						foundEntityTemplateInfo->m_entities.PushBackUnique( entity );
					}

					//
					if( meshComponents[i]->GetMeshNow() != nullptr ) 
					{
						meshComponentIsEmpty = false;

						usedMeshPath = meshComponents[i]->GetMeshNow()->GetDepotPath();
						String* pathPtr = foundEntityTemplateInfo->m_usedMeshes.FindPtr(usedMeshPath);
						if( pathPtr == nullptr )
						{
							foundEntityTemplateInfo->m_usedMeshes.PushBack( usedMeshPath );

							// update entity data size
							for( Uint32 k=0; k<allMeshesInfos.Size(); ++k )
							{
								if( allMeshesInfos[k].m_mesh == meshComponents[i]->GetMeshNow() )
								{
									foundEntityTemplateInfo->m_dataSize += ( allMeshesInfos[k].m_dataSize + allMeshesInfos[k].m_texturesDataSize );
									break;
								}
							}
						}
					}

					continue;
				}
			}
			
			// add new info about entity template if before didn't find information about current entity templates
			{	
				Vector currentPos = meshComponents[i]->GetWorldPosition();
				Bool isDuplicate = false;

				for ( Uint32 j=i+1; j < meshComponents.Size(); ++j )
				{
					Vector checkPos = meshComponents[j]->GetWorldPosition();

					if ( (currentPos - checkPos).Mag3() < 0.001 && meshComponents[i]->GetMeshNow() == meshComponents[j]->GetMeshNow() ) 
					{
						CEntity* checkEntity = meshComponents[j]->GetEntity();
						if (checkEntity != entity && checkEntity->GetEntityTemplate() == entity->GetEntityTemplate() )
						{
							++SInternalEntityTemplateInfo::m_supposedDuplicateEntityCount;
							isDuplicate = true;
							break;
						}
					}
				}

				//
				if( meshComponents[i]->GetMeshNow() != nullptr ) 
				{
					usedMeshPath = meshComponents[i]->GetMeshNow()->GetDepotPath();
					meshComponentIsEmpty = false;
				}

				// calculate full data size for current entity ( are taken into account only mesh components )
				Uint32 meshDataSize = 0;
				Uint32 streamingDistSum = 0, maxStreamingDist = 0, count = 0;
				TDynArray< CMesh* > calculatedMeshes;
				const TDynArray< CComponent* >& components = entity->GetComponents();
				for( auto it=components.Begin(); it!=components.End(); ++it )
				{
					if ( (*it)->IsStreamed() )
					{
						++count;
						Uint32 dist = (*it)->GetMinimumStreamingDistance();
						streamingDistSum += dist;
						maxStreamingDist = Max( maxStreamingDist, dist );
					}

					CMeshComponent* meshComp = Cast< CMeshComponent >( *it );
					if( meshComp != nullptr )
					{
						CMesh* mesh = meshComp->GetMeshNow();
						if( mesh != nullptr )
						{
							Bool foundTheSameMesh = false;
							// 
							for( auto meshIt=calculatedMeshes.Begin(); meshIt!=calculatedMeshes.End(); ++meshIt )
							{
								CMesh* calculatedMesh = ( *meshIt );
								if( calculatedMesh == mesh )
								{
									foundTheSameMesh = true ;
									break;
								}
							}

							// find info about this mesh
							SInternalMeshInfo* meshInfo = nullptr;
							for( auto meshInfoIt=allMeshesInfos.Begin(); meshInfoIt!=allMeshesInfos.End(); ++meshInfoIt )
							{
								SInternalMeshInfo& info = ( *meshInfoIt );
								if( info.m_mesh == mesh )
								{
									meshInfo = &info;
								}
							}

							if( foundTheSameMesh == true )
							{
								meshDataSize += meshInfo->m_dataSize;
							}
							else
							{
								meshDataSize += meshInfo->m_dataSize;
								meshDataSize += meshInfo->m_texturesDataSize;
								calculatedMeshes.PushBack( mesh );
							}
						}
					}
				}



				allEntitiesInfos.PushBack( SInternalEntityTemplateInfo( entityTemplate, entity, meshDataSize, usedMeshPath, isDuplicate, meshComponentIsEmpty, 
					count > 0 ? streamingDistSum / count : 0, maxStreamingDist ) );
			}
		}
	}
}

void CEdWorldSceneDebugger::CollectTexturesData( TDynArray< SInternalMeshInfo >& allMeshesInfos, TDynArray< SInternalTextureInfo >& allTexturesInfos )
{
	// Gather used textures by mesh
	for( Uint32 i=0; i<allMeshesInfos.Size(); ++i )
	{
		TDynArray< MeshTextureInfo* > usedTextures;
		MeshStatsNamespace::GatherTexturesUsedByMesh( allMeshesInfos[i].m_mesh.Get(), usedTextures );

		for( Uint32 j=0; j<usedTextures.Size(); ++j )
		{
			MeshTextureInfo* texInfo = usedTextures[j];

			// add information about texture to mesh
			allMeshesInfos[i].m_texturesDataSize += texInfo->m_dataSize;
			allMeshesInfos[i].m_usedTextures.PushBackUnique( texInfo->m_texture->GetDepotPath() );

			// searching the same texture
			Bool foundTexture = false;
			for( Uint32 j=0; j<allTexturesInfos.Size(); ++j )
			{
				if( allTexturesInfos[j].m_texture == texInfo->m_texture )
				{
					++allTexturesInfos[j].m_refCount;
					allTexturesInfos[j].m_usedByMeshes.PushBackUnique( allMeshesInfos[i].m_mesh->GetDepotPath() );
					foundTexture = true;
					break;
				}
			}

			if( foundTexture == false )
			{
				// if texture is not found in collection
				Uint32 dataSize = MeshStatsNamespace::CalcTextureDataSize( texInfo->m_texture );
				allTexturesInfos.PushBack( SInternalTextureInfo( texInfo->m_texture, dataSize, allMeshesInfos[i].m_mesh->GetDepotPath() ) );
			}
		}

		// release
		usedTextures.ClearPtr();
	}
}

void CEdWorldSceneDebugger::CollectTextureArrayData( TDynArray< SInternalMeshInfo >& allMeshesInfos, TDynArray< SInternalTextureArrayInfo >& allTextureArraysInfos, TDynArray< SInternalTextureInfo >& allTexturesInfos )
{
	// Gather used textures by mesh
	for( Uint32 i=0; i<allMeshesInfos.Size(); ++i )
	{
		TDynArray< SMeshTextureArrayInfo* > usedTextureArrays;
		MeshStatsNamespace::GatherTextureArraysUsedByMesh( allMeshesInfos[i].m_mesh.Get(), usedTextureArrays );

		for( Uint32 j=0; j<usedTextureArrays.Size(); ++j )
		{
			SMeshTextureArrayInfo* texArrayInfo = usedTextureArrays[j];

			// searching the same texture array
			Bool foundTextureArray = false;
			for( Uint32 k=0; k<allTextureArraysInfos.Size(); ++k )
			{
				if( allTextureArraysInfos[k].m_textureArray == texArrayInfo->m_textureArray )
				{
					++allTextureArraysInfos[k].m_refCount;
					allTextureArraysInfos[k].m_usedByMeshes.PushBackUnique( allMeshesInfos[i].m_mesh->GetDepotPath() );
					foundTextureArray = true;

					// Add information to texture collection because in this moment each texture array duplicates in memory each texture which contained
					// Should be removed when someone add fix for this duplication
					// It is VERY not optimal code... SHOULD BE CHANGE
					{
						UpdateTexturesDataBasedOnTexureArray( allTextureArraysInfos[j], allTexturesInfos, allMeshesInfos[i] );
					}
				}
			}

			if( foundTextureArray == false )
			{
				// if texture array is not found in collection
				SInternalTextureArrayInfo internalTextureArrayInfo( texArrayInfo->m_textureArray.Get() );
				internalTextureArrayInfo.m_usedByMeshes.PushBackUnique( allMeshesInfos[i].m_mesh->GetDepotPath() );
				UpdateTexturesDataBasedOnTexureArray( internalTextureArrayInfo, allTexturesInfos, allMeshesInfos[i] );

				// add object with information to collection
				allTextureArraysInfos.PushBack( internalTextureArrayInfo );
			}
		}

		// release
		usedTextureArrays.ClearPtr();
	}
}

void CEdWorldSceneDebugger::CollectApexData( TDynArray< SInternalApexInfo >& allApexInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Collecting apex data ...") );

	// collect cloth components
	TDynArray< CClothComponent* > clothComponents;
	GGame->GetActiveWorld()->GetAttachedComponentsOfClass( clothComponents );

	// collect destruction components
	TDynArray< CDestructionSystemComponent* > destructionComponents;
	GGame->GetActiveWorld()->GetAttachedComponentsOfClass( destructionComponents );
#ifdef USE_APEX

	//
	const Uint32 clothComponentCount = clothComponents.Size();
	const Uint32 destructionComponentCount = destructionComponents.Size();

	//
	for( Uint32 i=0; i<clothComponentCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, clothComponentCount + destructionComponentCount );

		const THandle< CApexResource >& apexResource = clothComponents[i]->GetParameters().m_resource;
		if( apexResource.IsValid() )
		{
			apexResource->AddRef();
			GatherInformationAboutApexResource( apexResource, allApexInfos, AT_Cloth );
			apexResource->ReleaseRef();
		}
	}

	for( Uint32 i=0; i<destructionComponentCount; ++i )
	{
		GFeedback->UpdateTaskProgress( clothComponentCount + i, clothComponentCount + destructionComponentCount );

		const THandle< CApexResource >& apexResource = destructionComponents[i]->GetParameters().m_resource;
		if( apexResource.IsValid() )
		{
			apexResource->AddRef();
			GatherInformationAboutApexResource( apexResource, allApexInfos, AT_Destruction );
			apexResource->ReleaseRef();
		}
	}
#endif
}

void CEdWorldSceneDebugger::CollectSpeedTreeData( TDynArray< SInternalSpeedTreeInfo >& allSpeedTreeInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Collecting speed tree data ...") );

	CWorld* world = GGame->GetActiveWorld();
	CFoliageEditionController& foliageController = world->GetFoliageEditionController();

	SClipmapParameters params;
	world->GetTerrain()->GetClipmapParameters( &params );
	Float terrainSize = params.terrainSize;

	TDynArray< SFoliageInstanceCollection > collection;
	foliageController.GetInstancesFromArea( Vector::ZEROS, terrainSize * Red::Math::MSqrt( 2 ), collection );
	const Uint32 foliageCount = collection.Size();

	for( Uint32 i=0; i<foliageCount; ++i )
	{
		THandle< CSRTBaseTree > treeType = collection[i].m_baseTree;

		// searching the same texture array
		Bool foundSpeedTree = false;
		for( Uint32 k=0; k<allSpeedTreeInfos.Size(); ++k )
		{
			if( allSpeedTreeInfos[k].m_speedTreeResource == treeType )
			{
				allSpeedTreeInfos[k].m_refCount += collection[i].m_instances.Size();
				foundSpeedTree = true;
			}
		}

		if( foundSpeedTree == false )
		{
			TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats > statsArray;
			GRender->GetSpeedTreeTextureStatistic( treeType->GetRenderObject().Get(), statsArray );

			Uint32 allTexturesDataSize = 0;
			for( auto it=statsArray.Begin(); it!=statsArray.End(); ++it )
			{
				SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats& statistic = ( *it );
				Uint32 textureDataSize = GpuApi::CalcTextureSize( statistic.m_textureDesc );
				allTexturesDataSize += textureDataSize;
			}

			String depotPath = String::EMPTY;
			GDepot->GetAbsolutePath( depotPath );
			depotPath.ReplaceAll( TXT("\\"), TXT("/") );

			// add object with information to collection
			SInternalSpeedTreeInfo info( treeType.Get(), collection[i].m_instances.Size(), treeType->GetSRTDataSize(), statsArray.Size(), allTexturesDataSize );
			for( auto it=statsArray.Begin(); it!=statsArray.End(); ++it )
			{
				SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats& statistic = ( *it );
				statistic.m_fileName.Replace( depotPath, String::EMPTY );
				info.m_usedTextures.PushBack( statistic.m_fileName );
			}

			allSpeedTreeInfos.PushBack( info );
		}
	}
}

void CEdWorldSceneDebugger::UpdateTexturesDataBasedOnTexureArray( SInternalTextureArrayInfo& textureArray, TDynArray< SInternalTextureInfo >& allTexturesInfos, SInternalMeshInfo& meshInfo )
{
	// Get information about textures in array
	TDynArray< CBitmapTexture* > usedTextures;
	textureArray.m_textureArray->GetTextures( usedTextures );

	Uint32 textureArrayDataSize = 0;

	for( Uint32 i=0; i<usedTextures.Size(); ++i )
	{
		// searching the same texture
		Bool foundTexture = false;
		for( Uint32 j=0; j<allTexturesInfos.Size(); ++j )
		{
			// found the same texture
			if( allTexturesInfos[j].m_texture == usedTextures[i] )
			{
				// update texture info
				++allTexturesInfos[j].m_refCount;
				allTexturesInfos[j].m_usedByTextureArray.PushBackUnique( textureArray.m_textureArray->GetDepotPath() );
				for( Uint32 k=0; k<textureArray.m_usedByMeshes.Size(); ++k )
				{
					allTexturesInfos[j].m_usedByMeshes.PushBackUnique( textureArray.m_usedByMeshes[k] );
				}

				// update texture array info
				textureArrayDataSize += allTexturesInfos[j].m_dataSize;
				textureArray.m_containedTextures.PushBackUnique( usedTextures[i]->GetDepotPath() );

				// update mesh info
				meshInfo.m_usedTextures.PushBack( allTexturesInfos[j].m_texture->GetDepotPath() );	// not unique because texture array duplicate texture in memory

				foundTexture = true;
				break;
			}
		}

		if( foundTexture == false )
		{
			// add new texture info if texture is not found in collection
			Uint32 dataSize = MeshStatsNamespace::CalcTextureDataSize( usedTextures[i] );
			SInternalTextureInfo internalTextureInfo( usedTextures[i], dataSize );
			for( Uint32 k=0; k<textureArray.m_usedByMeshes.Size(); ++k )
			{
				internalTextureInfo.m_usedByMeshes.PushBackUnique( textureArray.m_usedByMeshes[k] );
			}
			internalTextureInfo.m_usedByTextureArray.PushBackUnique( textureArray.m_textureArray->GetDepotPath() );
			allTexturesInfos.PushBack( internalTextureInfo );

			// update texture array info
			textureArrayDataSize += dataSize;
			textureArray.m_containedTextures.PushBackUnique( usedTextures[i]->GetDepotPath() );

			// update mesh info
			meshInfo.m_usedTextures.PushBack( internalTextureInfo.m_texture->GetDepotPath() );	// not unique because texture array duplicate texture in memory
		}
	}

	textureArray.m_dataSize = textureArrayDataSize;
	meshInfo.m_texturesDataSize += textureArrayDataSize;
}

void CEdWorldSceneDebugger::FillMeshesTab( TDynArray< SInternalMeshInfo >& allMeshesInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Filling tab with informations about meshes ...") );

	// general counters
	Uint32 differentChunks= 0;
	Float differentMeshData = 0;
	Uint32 differentMeshesOccuringOnce = 0;
	Uint32 differentChunksInMeshesOccuringOnce = 0.0f;
	Float differentMeshDataInMeshesOccuringOnce = 0.0f;

	// clear grid
	if( m_meshesGrid->GetNumberRows() > 0 )
	{
		m_meshesGrid->DeleteRows( 0, m_meshesGrid->GetNumberRows() );
	}

	const Uint32 rowCount = allMeshesInfos.Size();

	// add new row to grid
	m_meshesGrid->AppendRows( rowCount, false );

	for( Uint32 i=0; i<rowCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, rowCount );

		SInternalMeshInfo meshInfo = allMeshesInfos[i];

		// add row to grid
		{
			// prepare information for row
			Uint32 rowIndex = i;
			CFilePath filePath( meshInfo.m_mesh->GetDepotPath() );

			// fill row
			m_meshesGrid->DisableRowResize( rowIndex );
			m_meshesGrid->SetCellValue( rowIndex, 0, filePath.GetFileName().AsChar() );												// Name
			m_meshesGrid->SetCellValue( rowIndex, 1, ToString( meshInfo.m_refCount ).AsChar() );									// Ref count
			m_meshesGrid->SetCellValue( rowIndex, 2, meshInfo.m_mesh->GetDepotPath().AsChar() );									// Path
			m_meshesGrid->SetCellValue( rowIndex, 3, MeshStatsNamespace::MemSizeToText( meshInfo.m_dataSize ).wc_str() );			// Data size
			m_meshesGrid->SetCellValue( rowIndex, 4, ToString( meshInfo.m_lodsCount ).AsChar() );									// LODs count
			m_meshesGrid->SetCellValue( rowIndex, 5, ToString( meshInfo.m_refCount*meshInfo.m_triCount ).AsChar() );				// Scene tri count for mesh
			m_meshesGrid->SetCellValue( rowIndex, 6, ToString( meshInfo.m_refCount*meshInfo.m_vertCount ).AsChar() );				// Scene vert count for mesh
			m_meshesGrid->SetCellValue( rowIndex, 7, ToString( meshInfo.m_chunks ).AsChar() );										// Material chunks
			m_meshesGrid->SetCellValue( rowIndex, 8, ToString( meshInfo.m_usedTextures.Size() ).AsChar() );							// Used textures 
			m_meshesGrid->SetCellValue( rowIndex, 9, MeshStatsNamespace::MemSizeToText( meshInfo.m_texturesDataSize ).wc_str() );	// Used texture data size
			m_meshesGrid->SetCellValue( rowIndex, 10, ToString( meshInfo.m_usedTextures.Size() ).AsChar() );						// Texture amount
			m_meshesGrid->SetCellValue( rowIndex, 11, ToString( meshInfo.m_autoHideDistance ).AsChar() );							// Auto hide distance

			// create information about textures used by mesh
			String texturePaths = String::EMPTY;
			for( Uint32 j=0; j<meshInfo.m_usedTextures.Size(); ++j )
			{
				texturePaths += meshInfo.m_usedTextures[j];
				texturePaths += TXT("\n");
			}
			m_meshesGrid->SetCellValue( rowIndex, 8, texturePaths.AsChar() );																// Used textures
			m_meshesGrid->SetCellRenderer( rowIndex, 8, new wxGridCellAutoWrapStringRenderer() );
			m_meshesGrid->SetCellEditor( rowIndex, 8, new wxGridCellAutoWrapStringEditor() );
		}

		Float sizeInMB = meshInfo.m_dataSize / (1024.f * 1024.f);

		// update general info about all meshes
		differentChunks += meshInfo.m_chunks;
		differentMeshData += sizeInMB;

		if( meshInfo.m_refCount == 1 )
		{
			++differentMeshesOccuringOnce;
			differentChunksInMeshesOccuringOnce += meshInfo.m_chunks;
			differentMeshDataInMeshesOccuringOnce += sizeInMB;
		}
	}

	m_meshesGrid->Fit();

	// set the general info of meshes
	m_differentMeshesCount->SetLabel( ToString( allMeshesInfos.Size() ).AsChar() );
	m_differentChunksCount->SetLabel( ToString( differentChunks ).AsChar() );
	m_differentMeshesDataCount->SetLabel( ToString( differentMeshData ).AsChar() );
	m_differentMeshesOccuringOnceCount->SetLabel( ToString( differentMeshesOccuringOnce ).AsChar() );
	m_differentChunksInMeshesOccuringOnceCount->SetLabel( ToString( differentChunksInMeshesOccuringOnce ).AsChar() );
	m_differentMeshDataInMeshesOccuringOnceCount->SetLabel( ToString( differentMeshDataInMeshesOccuringOnce ).AsChar() );
}

void CEdWorldSceneDebugger::FillEntitiesTab( TDynArray< SInternalEntityTemplateInfo >& allEntitiesInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Filling tab with informations about entities ...") );

	// general counters
	Uint32 differentEntitiesCount = 0;
	Float differentEntitiesDataCount = 0;
	Uint32 differentEntitiesOccuringOnceCount = 0;
	Float differentDataInEntitiesOccuringOnceCount = 0;

	// clear grid
	if( m_entitiesGrid->GetNumberRows() > 0 )
	{
		m_entitiesGrid->DeleteRows( 0, m_entitiesGrid->GetNumberRows() );
	}

	const Uint32 rowCount = allEntitiesInfos.Size();

	// add new row to grid
	m_entitiesGrid->AppendRows( rowCount, false );

	for( Uint32 i = 0; i < rowCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, rowCount );

		const SInternalEntityTemplateInfo& info = allEntitiesInfos[i];

		// add row to grid
		{
			// prepare information for row
			Uint32 rowIndex = i;
			String depotPath = info.m_entityTemplate ? info.m_entityTemplate->GetFile()->GetDepotPath() : String::EMPTY;
			CFilePath filePath( depotPath );

			// fill row
			m_entitiesGrid->DisableRowResize( rowIndex );
			m_entitiesGrid->SetCellValue( rowIndex, 0, info.m_entities[0]->GetDisplayName().AsChar() );						// Name
			m_entitiesGrid->SetCellValue( rowIndex, 1, ToString( info.m_refCount ).AsChar() );								// Count
			m_entitiesGrid->SetCellValue( rowIndex, 2, depotPath.AsChar() );												// Path
			m_entitiesGrid->SetCellValue( rowIndex, 3, MeshStatsNamespace::MemSizeToText( info.m_dataSize ).wc_str() );		// Data size
			m_entitiesGrid->SetCellValue( rowIndex, 4, ToString( info.m_usedMeshes.Size() ).AsChar() );						// Used meshes count

			// create information about textures used by mesh
			String texturePaths = String::EMPTY;
			for( Uint32 j=0; j<info.m_usedMeshes.Size(); ++j )
			{
				texturePaths += info.m_usedMeshes[j];
				texturePaths += TXT("\n");
			}
			m_entitiesGrid->SetCellValue( rowIndex, 5, texturePaths.AsChar() );												// Used meshes
			m_entitiesGrid->SetCellRenderer( rowIndex, 5, new wxGridCellAutoWrapStringRenderer() );
			m_entitiesGrid->SetCellEditor( rowIndex, 5, new wxGridCellAutoWrapStringEditor() );

			String layers = String::EMPTY;
			for ( Uint32 j = 0; j < info.m_entities.Size(); ++j )
			{
				const String layerName = info.m_entities[j]->GetLayer()->GetFile() == nullptr ?
					info.m_entities[j]->GetLayer()->GetFriendlyName()
					: info.m_entities[j]->GetLayer()->GetFile()->GetFileName();

				if ( !layers.ContainsSubstring( layerName ) )
				{
					if ( !layers.Empty() )
					{
						layers += TXT(",\n");
					}
					layers += layerName;
				}
			}

			m_entitiesGrid->SetCellValue( rowIndex, 6, layers.AsChar() );													// Layers	
			m_entitiesGrid->SetCellRenderer( rowIndex, 6, new wxGridCellAutoWrapStringRenderer() );
			m_entitiesGrid->SetCellEditor( rowIndex, 6, new wxGridCellAutoWrapStringEditor() );

			m_entitiesGrid->SetCellValue( rowIndex, 7, info.m_supposedDuplicated ? wxT("1") : wxT("0") );					// Possible duplication
			m_entitiesGrid->SetCellValue( rowIndex, 8, ToString( info.m_entities[0]->GetStreamingDistance() ).AsChar() );	// Streaming distance
			m_entitiesGrid->SetCellValue( rowIndex, 9, ToString( info.m_avgStreamingDist ).AsChar() );						// Avg components streaming distance
			m_entitiesGrid->SetCellValue( rowIndex, 10, ToString( info.m_maxStrDist - info.m_avgStreamingDist ).AsChar() );	// Difference between components max and avg streaming distance

			// color if this entity has error
			wxColour errorColor = *wxWHITE;
			if( info.m_supposedDuplicated == true && info.m_meshComponentIsEmpty == true )
			{
				errorColor = wxColour( 255, 128, 0 );	// orange
			}
			else
			{
				if( info.m_supposedDuplicated == true )
				{
					errorColor = *wxRED;
				}
				if( info.m_meshComponentIsEmpty == true )
				{
					errorColor = *wxYELLOW;
				}
			}

			if( errorColor != *wxWHITE )
			{
				for( Uint32 j=0; j<GEntitiesColCount; ++j )
				{
					m_entitiesGrid->SetCellBackgroundColour( rowIndex, j, errorColor );
				}
			}
		}

		Float sizeInMB = info.m_dataSize / (1024.f * 1024.f);

		// update general info about all meshes
		++differentEntitiesCount;
		differentEntitiesDataCount += sizeInMB;

		if( info.m_refCount == 1 )
		{
			++differentEntitiesOccuringOnceCount;
			differentDataInEntitiesOccuringOnceCount += sizeInMB;
		}
	}

	m_entitiesGrid->Fit();

	// set the general info of entities
	m_duplicateEntitiesCount->SetLabel( ToString( SInternalEntityTemplateInfo::m_supposedDuplicateEntityCount ).AsChar() );
	m_differentEntitiesCount->SetLabel( ToString( differentEntitiesCount ).AsChar() );
	m_differentEntitiesDataCount->SetLabel( ToString( differentEntitiesDataCount ).AsChar() );
	m_differentEntitiesOccuringOnceCount->SetLabel( ToString( differentEntitiesOccuringOnceCount ).AsChar() );
	m_differentDataInEntitiesOccuringOnceCount->SetLabel( ToString( differentDataInEntitiesOccuringOnceCount ).AsChar() );
}

void CEdWorldSceneDebugger::FillTexturesTab( TDynArray< SInternalTextureInfo >& allTexturesInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Filling tab with informations about textures ...") );

	// general counters
	Uint32 differentTexturesCount = 0;
	Float differentTexturesDataCount = 0;
	Uint32 differentTextureOccuringOnceCount = 0;
	Float differentDataInTextureOccuringOnceCount = 0;

	// clear grid
	if( m_texturesGrid->GetNumberRows() > 0 )
	{
		m_texturesGrid->DeleteRows( 0, m_texturesGrid->GetNumberRows() );
	}

	const Uint32 rowCount = allTexturesInfos.Size();

	// add new row to grid
	m_texturesGrid->AppendRows( rowCount, false );

	for (Uint32 i = 0; i < rowCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, rowCount );

		SInternalTextureInfo texInfo = allTexturesInfos[i];

		// add row to grid
		{
			// prepare information for row
			Uint32 rowIndex = i;
			CFilePath filePath( texInfo.m_texture->GetDepotPath() );

			// fill row
			m_texturesGrid->DisableRowResize( rowIndex );
			m_texturesGrid->SetCellValue( rowIndex, 0, filePath.GetFileName().AsChar() );										// Name
			m_texturesGrid->SetCellValue( rowIndex, 1, ToString( texInfo.m_refCount ).AsChar() );								// Ref count
			m_texturesGrid->SetCellValue( rowIndex, 2, texInfo.m_texture->GetDepotPath().AsChar() );							// Path
			m_texturesGrid->SetCellValue( rowIndex, 3, MeshStatsNamespace::MemSizeToText( texInfo.m_dataSize ).wc_str() );		// Data size
			m_texturesGrid->SetCellValue( rowIndex, 4, ToString( texInfo.m_texture->GetWidth() ).AsChar() );					// Dimensions - X
			m_texturesGrid->SetCellValue( rowIndex, 5, ToString( texInfo.m_texture->GetHeight() ).AsChar()  );					// Dimensions - Y
			m_texturesGrid->SetCellValue( rowIndex, 6, ToString( texInfo.m_texture->GetMipCount() ).AsChar()  );				// MipMaps

			// create information about meshes used this texture
			m_texturesGrid->SetCellValue( rowIndex, 7, ToString( texInfo.m_usedByMeshes.Size() ).AsChar()  );					// Used by meshes count
			String meshesPaths = String::EMPTY;
			for( Uint32 j=0; j<texInfo.m_usedByMeshes.Size(); ++j )
			{
				meshesPaths += texInfo.m_usedByMeshes[j];
				meshesPaths += TXT("\n");
			}
			m_texturesGrid->SetCellValue( rowIndex, 8, meshesPaths.AsChar() );													// Used by mesh
			m_texturesGrid->SetCellRenderer( rowIndex, 8, new wxGridCellAutoWrapStringRenderer() );
			m_texturesGrid->SetCellEditor( rowIndex, 8, new wxGridCellAutoWrapStringEditor() );

			// create information about texture array used this texture
			m_texturesGrid->SetCellValue( rowIndex, 9, ToString( texInfo.m_usedByTextureArray.Size() ).AsChar()  );				// Used by texture arrays count
			String textureArraysPaths = String::EMPTY;
			for( Uint32 j=0; j<texInfo.m_usedByTextureArray.Size(); ++j )
			{
				textureArraysPaths += texInfo.m_usedByTextureArray[j];
				textureArraysPaths += TXT("\n");
			}
			m_texturesGrid->SetCellValue( rowIndex, 10, textureArraysPaths.AsChar() );											// Used by texture arrays
			m_texturesGrid->SetCellRenderer( rowIndex, 10, new wxGridCellAutoWrapStringRenderer() );
			m_texturesGrid->SetCellEditor( rowIndex, 10, new wxGridCellAutoWrapStringEditor() );
		}

		Float sizeInMB = texInfo.m_dataSize / (1024.f * 1024.f);

		// update general info about all meshes
		differentTexturesDataCount += sizeInMB;

		if( texInfo.m_refCount == 1 )
		{
			++differentTextureOccuringOnceCount;
			differentDataInTextureOccuringOnceCount += sizeInMB;
		}
	}

	m_texturesGrid->Fit();

	differentTexturesCount = allTexturesInfos.Size();

	// set the general info of meshes
	m_differentTexturesCount->SetLabel( ToString( differentTexturesCount ).AsChar() );
	m_differentTexturesDataCount->SetLabel( ToString( differentTexturesDataCount ).AsChar() );
	m_differentTextureOccuringOnceCount->SetLabel( ToString( differentTextureOccuringOnceCount ).AsChar() );
	m_differentDataInTextureOccuringOnceCount->SetLabel( ToString( differentDataInTextureOccuringOnceCount ).AsChar() );
}

void CEdWorldSceneDebugger::FillTextureArraysTab( TDynArray< SInternalTextureArrayInfo >& allTextureArraysInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Filling tab with informations about texture arrays ...") );

	// general counters
	Uint32 differentTextureArraysCount = 0;
	Float differentTextureArraysDataCount = 0;
	Uint32 differentTextureArraysOccuringOnceCount = 0;
	Float differentDataInTextureArraysOccuringOnceCount = 0;

	// clear grid
	if( m_textureArraysGrid->GetNumberRows() > 0 )
	{
		m_textureArraysGrid->DeleteRows( 0, m_textureArraysGrid->GetNumberRows() );
	}

	const Uint32 rowCount = allTextureArraysInfos.Size();

	// add new row to grid
	m_textureArraysGrid->AppendRows( rowCount, false );

	for (Uint32 i = 0; i < rowCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, rowCount );

		const SInternalTextureArrayInfo& info = allTextureArraysInfos[i];

		// add row to grid
		{
			// prepare information for row
			Uint32 rowIndex = i;
			CFilePath filePath( info.m_textureArray->GetDepotPath() );

			// fill row
			m_textureArraysGrid->DisableRowResize( rowIndex );
			m_textureArraysGrid->SetCellValue( rowIndex, 0, filePath.GetFileName().AsChar() );												// Name
			m_textureArraysGrid->SetCellValue( rowIndex, 1, ToString( info.m_refCount ).AsChar() );											// Ref count
			m_textureArraysGrid->SetCellValue( rowIndex, 2, info.m_textureArray->GetDepotPath().AsChar() );									// Path
			m_textureArraysGrid->SetCellValue( rowIndex, 3, MeshStatsNamespace::MemSizeToText( info.m_dataSize ).wc_str() );				// Data size			

			// create information about textures contained in texture array
			m_textureArraysGrid->SetCellValue( rowIndex, 4, ToString( info.m_containedTextures.Size() ).AsChar() );							// Contained texture count
			String texturePaths = String::EMPTY;
			for( Uint32 j=0; j<info.m_containedTextures.Size(); ++j )
			{
				texturePaths += info.m_containedTextures[j];
				texturePaths += TXT("\n");
			}
			m_textureArraysGrid->SetCellValue( rowIndex, 5, texturePaths.AsChar() );														// Contained texture
			m_textureArraysGrid->SetCellRenderer( rowIndex, 5, new wxGridCellAutoWrapStringRenderer() );
			m_textureArraysGrid->SetCellEditor( rowIndex, 5, new wxGridCellAutoWrapStringEditor() );

			// create information about textures contained in texture array
			m_textureArraysGrid->SetCellValue( rowIndex, 6, ToString( info.m_usedByMeshes.Size() ).AsChar() );							// Used by mesh count
			String usedByMeshesPaths = String::EMPTY;
			for( Uint32 j=0; j<info.m_usedByMeshes.Size(); ++j )
			{
				usedByMeshesPaths += info.m_usedByMeshes[j];
				usedByMeshesPaths += TXT("\n");
			}
			m_textureArraysGrid->SetCellValue( rowIndex, 7, usedByMeshesPaths.AsChar() );													// Used by mesh
			m_textureArraysGrid->SetCellRenderer( rowIndex, 7, new wxGridCellAutoWrapStringRenderer() );
			m_textureArraysGrid->SetCellEditor( rowIndex, 7, new wxGridCellAutoWrapStringEditor() );
		}

		Float sizeInMB = info.m_dataSize / (1024.f * 1024.f);

		// update general info about all meshes
		differentTextureArraysDataCount += sizeInMB;

		if( info.m_refCount == 1 )
		{
			++differentTextureArraysOccuringOnceCount;
			differentDataInTextureArraysOccuringOnceCount += sizeInMB;
		}
	}

	m_textureArraysGrid->Fit();

	differentTextureArraysCount = allTextureArraysInfos.Size();

	// set the general info of texture arrays
	m_differentTextureArraysCount->SetLabel( ToString( differentTextureArraysCount ).AsChar() );
	m_differentTextureArraysDataCount->SetLabel( ToString( differentTextureArraysDataCount ).AsChar()  );
	m_differentTextureArraysOccuringOnceCount->SetLabel( ToString( differentTextureArraysOccuringOnceCount ).AsChar() );
	m_differentDataInTextureArraysOccuringOnceCount->SetLabel( ToString( differentDataInTextureArraysOccuringOnceCount ).AsChar() );
}

void CEdWorldSceneDebugger::FillApexTab( TDynArray< SInternalApexInfo >& allApexInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Filling tab with informations about apex resources ...") );

	// general counters
	Uint32 differentClothComponent = 0;
	Uint32 differentDestructionComponent = 0;
	Uint32 differentApexComponentOccuringOnce = 0;

	// clear grid
	if( m_apexGrid->GetNumberRows() > 0 )
	{
		m_apexGrid->DeleteRows( 0, m_apexGrid->GetNumberRows() );
	}

	const Uint32 rowCount = allApexInfos.Size();

	// add new row to grid
	m_apexGrid->AppendRows( rowCount, false );

	for( Uint32 i=0; i<rowCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, rowCount );

		SInternalApexInfo apexInfo = allApexInfos[i];

		// add row to grid
		{
			// prepare information for row
			Uint32 rowIndex = i;
			CFilePath filePath( apexInfo.m_apexResource->GetDepotPath() );

			// fill row
			m_apexGrid->DisableRowResize( rowIndex );
			m_apexGrid->SetCellValue( rowIndex, 0, filePath.GetFileName().AsChar() );						// Name
			m_apexGrid->SetCellValue( rowIndex, 1, ToString( apexInfo.m_refCount ).AsChar() );				// Ref count
			m_apexGrid->SetCellValue( rowIndex, 2, apexInfo.m_apexResource->GetDepotPath().AsChar() );		// Path
			m_apexGrid->SetCellValue( rowIndex, 3, ConvertEnumToString( apexInfo.m_apexType).AsChar() );	// Apex type
			m_apexGrid->SetCellValue( rowIndex, 4, ToString( apexInfo.m_lods ).AsChar() );					// LOD count
			m_apexGrid->SetCellValue( rowIndex, 5, ToString( apexInfo.m_triangles ).AsChar() );				// All triangle count
			m_apexGrid->SetCellValue( rowIndex, 6, ToString( apexInfo.m_vertices ).AsChar() );				// All vertices count
		}

		if( apexInfo.m_apexType == AT_Cloth )
		{
			++differentClothComponent;
		}
		else if( apexInfo.m_apexType == AT_Destruction )
		{
			++differentDestructionComponent;
		}

		if( apexInfo.m_refCount == 1 )
		{
			++differentApexComponentOccuringOnce;
		}
	}

	m_apexGrid->Fit();

	// set the general info of apex resources
	m_differentApexClothComponentCount->SetLabel( ToString( differentClothComponent ).AsChar() );
	m_differentApexDestructibleComponentCount->SetLabel( ToString( differentDestructionComponent ).AsChar() );
	m_differentApexComponentOccuringOnceCount->SetLabel( ToString( differentApexComponentOccuringOnce ).AsChar() );
}

void CEdWorldSceneDebugger::FillSpeedTreeTab( TDynArray< SInternalSpeedTreeInfo >& allSpeedTreeInfos )
{
	GFeedback->UpdateTaskInfo( TXT("Filling tab with informations about speed tree resources ...") );

	// general counters
	Float totalSpeedTreeData = 0;
	Uint32 differentSpeedTreeTypeOccuringOnce = 0;
	Uint32 differentSpeedTreeTypeOccuringLessThenTenTimes = 0;
	Float differentSpeedTreeTypeOccuringLessThenTenTimesSize = 0;
	Float differentDataInSpeedTreeTypeOccuringOnce = 0;
	Uint32 speedTreeNumberTypes = 0;

	// clear grid
	if( m_speedTreeGrid->GetNumberRows() > 0 )
	{
		m_speedTreeGrid->DeleteRows( 0, m_speedTreeGrid->GetNumberRows() );
	}

	const Uint32 rowCount = allSpeedTreeInfos.Size();

	// add new row to grid
	m_speedTreeGrid->AppendRows( rowCount, false );
	speedTreeNumberTypes = rowCount;
	
	for( Uint32 i=0; i<rowCount; ++i )
	{
		GFeedback->UpdateTaskProgress( i, rowCount );

		SInternalSpeedTreeInfo& speedTreeInfo = allSpeedTreeInfos[i];

		// add row to grid
		{
			// prepare information for row
			Uint32 rowIndex = i;
			CFilePath filePath( speedTreeInfo.m_speedTreeResource->GetDepotPath() );

			// fill row
			m_speedTreeGrid->DisableRowResize( rowIndex );
			m_speedTreeGrid->SetCellValue( rowIndex, 0, filePath.GetFileName().AsChar() );													// Name
			m_speedTreeGrid->SetCellValue( rowIndex, 1, ToString( speedTreeInfo.m_refCount ).AsChar() );									// Ref count
			m_speedTreeGrid->SetCellValue( rowIndex, 2, speedTreeInfo.m_speedTreeResource->GetDepotPath().AsChar() );						// Path
			m_speedTreeGrid->SetCellValue( rowIndex, 3, MeshStatsNamespace::MemSizeToText( speedTreeInfo.m_dataSize ).wc_str() );			// Data size
			m_speedTreeGrid->SetCellValue( rowIndex, 4, ToString( speedTreeInfo.m_textureCount ).AsChar() );								// Used texture count
			m_speedTreeGrid->SetCellValue( rowIndex, 5, MeshStatsNamespace::MemSizeToText( speedTreeInfo.m_texturesDataSize ).wc_str() );	// Data size

			String usedTextures = String::EMPTY;
			for( Uint32 j=0; j<speedTreeInfo.m_usedTextures.Size(); ++j )
			{
				usedTextures += speedTreeInfo.m_usedTextures[j];
				usedTextures += TXT("\n");
			}
			m_speedTreeGrid->SetCellValue( rowIndex, 6, usedTextures.AsChar() );															// Used textures
			m_speedTreeGrid->SetCellRenderer( rowIndex, 6, new wxGridCellAutoWrapStringRenderer() );
			m_speedTreeGrid->SetCellEditor( rowIndex, 6, new wxGridCellAutoWrapStringEditor() );
		}

		Float sizeInMB = speedTreeInfo.m_dataSize / (1024.f * 1024.f);

		totalSpeedTreeData += sizeInMB;

		if( speedTreeInfo.m_refCount == 1 )
		{
			++differentSpeedTreeTypeOccuringOnce;
			differentDataInSpeedTreeTypeOccuringOnce += sizeInMB;
		}
		if( speedTreeInfo.m_refCount < 10 )
		{
			++differentSpeedTreeTypeOccuringLessThenTenTimes;
			differentSpeedTreeTypeOccuringLessThenTenTimesSize += sizeInMB;
		}
		
	}

	m_speedTreeGrid->Fit();

	// set the general info of apex resources
	m_totalSpeedTreeData->SetLabel( ToString( totalSpeedTreeData ).AsChar() );
	m_differentSpeedTreeTypeOccuringOnceCount->SetLabel( ToString( differentSpeedTreeTypeOccuringLessThenTenTimes ).AsChar() );
	m_differentDataInSpeedTreeTypeOccuringOnceCount->SetLabel( ToString( differentSpeedTreeTypeOccuringLessThenTenTimesSize ).AsChar() );
	m_speedTreeNumTypes->SetLabel( ToString( speedTreeNumberTypes ).AsChar() );

	m_totalSpeedTreeData->GetParent()->Layout();
	m_totalSpeedTreeData->Layout();
	m_differentSpeedTreeTypeOccuringOnceCount->Layout();
	m_differentDataInSpeedTreeTypeOccuringOnceCount->Layout();
	m_speedTreeNumTypes->Layout();
}

bool CEdWorldSceneDebugger::Show( bool show /*= true */ )
{
	return wxFrame::Show( show );
}

void CEdWorldSceneDebugger::OnRefreshData( wxCommandEvent& event )
{
	CollectDataFromWorld();
}

void CEdWorldSceneDebugger::OnRowClicked( wxGridEvent& event )
{
	m_selectedGrid = wxDynamicCast( event.GetEventObject(), wxGrid );
	m_selectedRow = event.GetRow();
	event.Skip();
}

void CEdWorldSceneDebugger::OnOpenEditWindow( wxGridEvent& event )
{
	CEdAssetBrowser* browser = wxTheFrame->GetAssetBrowser();
	if( browser != nullptr )
	{
		const Uint32 pathColumnIndex = 2;
		String depotPath = m_selectedGrid->GetCellValue( m_selectedRow, pathColumnIndex );

		CResource* resource = GDepot->LoadResource( depotPath );
		if ( resource != nullptr )
		{
			browser->EditAsset( resource );
		}
	}
}

void CEdWorldSceneDebugger::OnSortByColumn( wxGridEvent& event )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	GFeedback->BeginTask( TXT("Sorting ..."), false );

	// helper typdefs
	typedef TDynArray< String > RowStrings;
	typedef TPair< String, RowStrings > RowsToSort;
	typedef TDynArray< RowsToSort > SortedRows;

	// data sorter
	struct LocalSorter
	{
		static Bool SortStringAscending( const TPair< String, RowStrings >& p1, const TPair< String, RowStrings >& p2 )
		{
			return Red::System::StringCompareNoCase( p1.m_first.AsChar(), p2.m_first.AsChar() ) < 0;
		}

		static Bool SortIntAscending( const TPair< String, RowStrings >& p1, const TPair< String, RowStrings >& p2 )
		{
			Int32 d1, d2;
			FromString< Int32 >(p1.m_first, d1);
			FromString< Int32 >(p2.m_first, d2);
			return d2 > d1;
		}

		static Bool SortFloatAscending( const TPair< String, RowStrings >& p1, const TPair< String, RowStrings >& p2 )
		{
			Float d1, d2;
			FromString< Float >(p1.m_first, d1);
			FromString< Float >(p2.m_first, d2);
			return d2 > d1;
		}
	};

	// get basic information
	m_selectedGrid = wxDynamicCast( event.GetEventObject(), wxGrid );
	m_selectedRow = -1;
	Uint32 columnIndex = event.GetCol();
	Bool ascendingOrder = m_selectedGrid->IsSortOrderAscending();
	Uint32 rowCount = m_selectedGrid->GetNumberRows();
	Uint32 colCount = m_selectedGrid->GetNumberCols();

	// choose the comparer
	EComparerType comparerType = EComparerType::CT_None;
	ETabType selectedGridIndex = (ETabType)GridPointerToIndex( m_selectedGrid );
	if( selectedGridIndex != TT_Count )
	{
		switch( selectedGridIndex )
		{
		case TT_Entities:
			comparerType = GEntitiesColComparerType[columnIndex];
			break;
		case TT_Meshes:
			comparerType = GMeshesColComparerType[columnIndex];
			break;
		case TT_Textures:
			comparerType = GTexturesColComparerType[columnIndex];
			break;
		case TT_TextureArray:
			comparerType = GTextureArrayColComparerType[columnIndex];
			break;
		case TT_Apex:
			comparerType = GApexColComparerType[columnIndex];
			break;
		case TT_SpeedTree:
			comparerType = GSpeedTreeColComparerType[columnIndex];
			break;
		}

		if( comparerType == EComparerType::CT_None )
		{
			GFeedback->EndTask();
			return;		// this column don't need sorting
		}
	}

	// get rows to sorting
	GFeedback->UpdateTaskInfo( TXT("Collecting cells to sort...") );

	SortedRows rows;
	THashMap< String, wxColour > colors;
	for( Uint32 i=0; i<rowCount; ++i )
	{
		RowStrings values;

		for( Uint32 j=0; j<colCount; ++j )
		{
			GFeedback->UpdateTaskProgress( i*rowCount + j, rowCount*colCount );
			values.PushBack( m_selectedGrid->GetCellValue( i, j ).c_str().AsWChar() );
		}

		// create pair
		rows.PushBack( RowsToSort( m_selectedGrid->GetCellValue( i, columnIndex ).c_str().AsWChar(), values ) );

		wxColour color = m_selectedGrid->GetCellBackgroundColour( i, 0 );
		colors.Insert( m_selectedGrid->GetCellValue( i, 0 ).c_str().AsWChar(), color );
	}

	// sorting
	GFeedback->UpdateTaskInfo( TXT("Sorting...") );
	switch( comparerType )
	{
	case CT_Integer:
		Sort( rows.Begin(), rows.End(), LocalSorter::SortIntAscending );
		break;
	case CT_Real:
		Sort( rows.Begin(), rows.End(), LocalSorter::SortFloatAscending );
		break;
	case CT_Text:
		Sort( rows.Begin(), rows.End(), LocalSorter::SortStringAscending );
		break;
	}

	// set correct values
	GFeedback->UpdateTaskInfo( TXT("Setting sorted informations to cells...") );
	for( Uint32 i=0; i<rows.Size(); ++i)
	{
		const RowStrings& values = rows[i].m_second;

		for( Uint32 j=0; j<colCount; ++j )
		{
			GFeedback->UpdateTaskProgress( i*rowCount + j, rowCount*colCount );
			wxColour* color = colors.FindPtr( values[0].AsChar() );
			if( ascendingOrder == true )
			{
				m_selectedGrid->SetCellValue( i, j, values[j].AsChar() );
				if( color != nullptr )
				{
					m_selectedGrid->SetCellBackgroundColour( i, j, *color );
				}
			}
			else
			{
				m_selectedGrid->SetCellValue( rows.Size() - ( i + 1 ), j, values[j].AsChar() );
				if( color != nullptr )
				{
					m_selectedGrid->SetCellBackgroundColour( rows.Size() - ( i + 1 ), j, *color );
				}
			}
		}
	}

	m_selectedGrid->SetSortingColumn( columnIndex, ascendingOrder );
	m_selectedGrid->GetGridColLabelWindow()->Refresh();
	// m_selectedGrid->Layout();
	// m_selectedGrid->Fit();

	GFeedback->EndTask();
}

void CEdWorldSceneDebugger::OnShowColumnContextMenu( wxGridEvent& event )
{
	m_selectedGrid = wxDynamicCast( event.GetEventObject(), wxGrid );
	m_selectedRow = -1;

	// create correct menu for selected grid
	wxMenu* columnContextMenu = new wxMenu();
	for( Int32 i=0; i<m_selectedGrid->GetNumberCols(); ++i )
	{		
		columnContextMenu->Append( i, m_selectedGrid->GetColLabelValue( i ), TXT(""), (wxItemKind)wxITEM_CHECK )->Check( m_selectedGrid->GetColSize( i ) != 0 );
	}
	columnContextMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &CEdWorldSceneDebugger::OnColumnPopupClick, this );

	m_selectedGrid->SelectRow( m_selectedRow );
	wxWindowBase::PopupMenu( columnContextMenu );
}

void CEdWorldSceneDebugger::OnShowRowContextMenu( wxGridEvent& event )
{
	m_selectedGrid = wxDynamicCast( event.GetEventObject(), wxGrid );
	m_selectedRow = event.GetRow();	

	m_selectedGrid->SelectRow( m_selectedRow );

	ETabType selectedGridIndex = (ETabType)GridPointerToIndex( m_selectedGrid );
	switch( selectedGridIndex )
	{
	case TT_Entities:
		CreateAdditionalEntityContextMenu();
		break;
	case TT_Meshes:
		CreateAdditionalMeshesContextMenu();
		break;
	case TT_Textures:
		CreateAdditionalTextureContextMenu();
		break;
	case TT_TextureArray:
		CreateAdditionalTextureArrayContextMenu();
		break;
	}

	// show popup menu
	wxWindowBase::PopupMenu( GContextMenus[selectedGridIndex] );
}

void CEdWorldSceneDebugger::OnRowPopupClick( wxCommandEvent &event )
{
	Uint32 option = event.GetId();
	ETabType selectedGridIndex = (ETabType)GridPointerToIndex( m_selectedGrid );

	// standard operations
	if( option == 0 )
	{
		ShowInAssetBrowser();
	}
	else if( option == 1 )
	{
		ShowInExplorer();
	}
	else if( option == 2 )
	{
		ShowAllResourceInstances();
	}
	else
	{
		switch( selectedGridIndex )
		{
		case TT_Entities:
			OnAdditionalEntitiesContextMenu( event );
			break;
		case TT_Meshes:
			OnAdditionalMeshesContextMenu( event );
			break;
		case TT_Textures:
			OnAdditionalTexturesContextMenu( event );
			break;
		case TT_TextureArray:
			OnAdditionalTextureArraysContextMenu( event );
			break;
		}
	}
}

void CEdWorldSceneDebugger::OnColumnPopupClick( wxCommandEvent &event )
{
	if( event.GetInt() != 0 )
	{
		m_selectedGrid->ShowCol( event.GetId() );
		m_selectedGrid->AutoSizeColumn( event.GetId(), true );
	}
	else
	{
		m_selectedGrid->HideCol( event.GetId() );
	}
}

void CEdWorldSceneDebugger::Export()
{
	wxWindowUpdateLocker localUpdateLocker( this );

	// set default name from world file
	CWorld* world = GGame->GetActiveWorld();
	CFilePath worldPath( world->GetDepotPath() );

	// export to one file
	if( m_exportToFile->GetSelection() == 0 )
	{
		// select path to save file
		CEdFileDialog m_saveDialogWindow;
		m_saveDialogWindow.SetMultiselection( false );
		m_saveDialogWindow.AddFormat( TXT("csv"), TXT("CSV file") );

		if( m_saveDialogWindow.DoSave( (HWND)GetHandle(), String::Printf( TXT("WorldSceneInfo[%s]"), worldPath.GetFileName().AsChar() ).AsChar(),  true ) == true )
		{
			CResource::FactoryInfo< C2dArray > info;
			C2dArray* worldSceneInfos = info.CreateResource();

			// exporting data
			for( Uint32 i=0; i<TT_Count; ++i )
			{
				if( m_tabsListToExport->IsChecked( i ) == true )
				{
					ExportGrid( worldSceneInfos, GHelperGrids[i], GTabNames[i] );
				}
			}

			// converting path to depot path
			String localDepotPath;
			if ( GDepot->ConvertToLocalPath( m_saveDialogWindow.GetFile(), localDepotPath ) == false )
			{
				WARN_EDITOR( TXT("Could not convert '%s' to local depot path."), m_saveDialogWindow.GetFile().AsChar() );
			}

			CFilePath filePath( localDepotPath );
			CDirectory* directory = GDepot->CreatePath( localDepotPath.AsChar() );

			if ( CDiskFile *diskFile = GDepot->FindFile( localDepotPath ) )
			{
				String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), localDepotPath.AsChar() );
				if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
				{
					return;
				}
			}

			// save to file
			worldSceneInfos->SaveAs( directory, filePath.GetFileName(), true );
			worldSceneInfos->Discard();
		}
	}
	else // export to any files (one file for tab)
	{
		wxDirDialog dirDialog( this, wxT("Select directory for file"), GDepot->GetRootDataPath().AsChar(), wxDD_NEW_DIR_BUTTON );
		if( dirDialog.ShowModal() == wxID_OK )
		{
			String selectedDirectoryPath = dirDialog.GetPath().c_str().AsWChar();
			selectedDirectoryPath += TXT("\\");

			// converting path to depot path
			String localDepotPath;
			if ( GDepot->ConvertToLocalPath( selectedDirectoryPath, localDepotPath ) == false )
			{
				WARN_EDITOR( TXT("Could not convert '%s' to local depot path."), selectedDirectoryPath.AsChar() );
			}

			CDirectory* directory = GDepot->CreatePath( localDepotPath.AsChar() );

			// exporting data
			for( Uint32 i=0; i<TT_Count; ++i )
			{
				if( m_tabsListToExport->IsChecked( i ) == true )
				{
					CResource::FactoryInfo< C2dArray > info;
					C2dArray* worldSceneInfos = info.CreateResource();

					ExportGrid( worldSceneInfos, GHelperGrids[i], GTabNames[i] );

					String fullPath = localDepotPath + String::Printf( TXT("WorldSceneInfo[%s] - [%s].csv"), worldPath.GetFileName().AsChar(), GTabNames[i] );
					CFilePath filePath( fullPath );

					if ( CDiskFile *diskFile = GDepot->FindFile( fullPath ) )
					{
						String message = String::Printf( TXT("File '%s' already exists.\nDo you want to replace it?"), fullPath.AsChar() );
						if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
						{
							continue;
						}
					}

					// save to file
					worldSceneInfos->SaveAs( directory, filePath.GetFileName(), true );
					worldSceneInfos->Discard();
				}
			}
		}
	}
}

void CEdWorldSceneDebugger::ExportGrid( C2dArray* array, wxGrid* exportedGrid, const String& gridName )
{
	const Uint32 gridColumnCount = exportedGrid->GetNumberCols();
	const Uint32 gridRowCount = exportedGrid->GetNumberRows();
	const Uint32 initialArrayColumnCount = array->GetNumberOfColumns();
	const Uint32 initialArrayRowCount = array->GetNumberOfRows();

	// check count of columns for grid
	if( initialArrayColumnCount < gridColumnCount )
	{
		Uint32 deltaColumnCount = gridColumnCount - initialArrayColumnCount;
		for( Uint32 i=0; i<deltaColumnCount; ++i )
		{
			array->AddColumn( TXT("") );
		}
	}

	// add name of exported grid
	array->AddRow();
	array->SetValue( gridName, 0, initialArrayRowCount );

	// add headers
	array->AddRow();
	for( Uint32 i=0; i<gridColumnCount; ++i )
	{
		array->SetValue( exportedGrid->GetColLabelValue( i ).c_str().AsWChar(), i, initialArrayRowCount + 1 );
	}

	// export data to array
	Bool onlyDuplicated = m_exportOnlyDuplicated->IsChecked() && exportedGrid == m_entitiesGrid;
	for( Uint32 i=0; i<gridRowCount; ++i )
	{
		if ( !onlyDuplicated || wxAtoi( exportedGrid->GetCellValue( i, gridColumnCount - 1 ) ) == 1 )
		{
			array->AddRow();
			const Uint32 activeRowIndex = array->GetNumberOfRows() - 1;

			for( Uint32 j=0; j<gridColumnCount; ++j )
			{
				String value = exportedGrid->GetCellValue( i, j ).c_str().AsWChar();
				value.ReplaceAll( TXT("\n"), TXT(" | "), false );
				array->SetValue( value , j, activeRowIndex );
			}
		}
	}

	// add two additional rows for better view in file
	array->AddRow();
	array->AddRow();
}

void CEdWorldSceneDebugger::ShowInAssetBrowser( const String& path/* = TXT("")*/ )
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	if( path.Empty() == false )
	{
		SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( path ) );
		return;
	}
	if( m_selectedGrid != nullptr && m_selectedRow != -1 )
	{
		const Uint32 pathColumnIndex = 2;
		String internalPath = m_selectedGrid->GetCellValue( m_selectedRow, pathColumnIndex );
		if( internalPath != String::EMPTY )
		{
			SEvents::GetInstance().QueueEvent( CNAME( SelectAsset ), CreateEventData( internalPath ) );
		}
		else
		{
			// this situation is possible only for entities because entity can be created without entity template (which is stored as resource)
			wxMessageBox( wxT("Selected row doesn't contain path to resource."), wxT("Warning"), wxICON_WARNING | wxOK );
		}
	}
#endif
}

void CEdWorldSceneDebugger::ShowInExplorer()
{
	if( m_selectedGrid != nullptr && m_selectedRow != -1 )
	{
		const Uint32 pathColumnIndex = 2;
		String path = m_selectedGrid->GetCellValue( m_selectedRow, pathColumnIndex );
		if( path != String::EMPTY )
		{
			wxString cmd = wxT( "explorer /select," );
			cmd += ( GFileManager->GetDataDirectory() + path ).AsChar();
			::wxExecute( cmd, wxEXEC_ASYNC, nullptr );
		}
		else
		{
			// this situation is possible only for entities because entity can be created without entity template (which is stored as resource)
			wxMessageBox( wxT("Selected row doesn't contain path to resource."), wxT("Warning"), wxICON_WARNING | wxOK );
		}
	}
}

void CEdWorldSceneDebugger::ShowEntityInSceneExplorer()
{
	CEdSceneExplorer* scene = wxTheFrame->GetSceneExplorer();
	if( scene != nullptr )
	{
		if( m_selectedGrid == GHelperGrids[TT_Entities] && m_selectedRow != -1 )
		{
			const Uint32 entityNameColumnIndex = 0;
			String entityName = m_selectedGrid->GetCellValue( m_selectedRow, entityNameColumnIndex );
			
			ShowEntityOnMap( entityName );
		}

		scene->UpdateSelected();
	}
}

void CEdWorldSceneDebugger::ShowAllResourceInstances()
{
	if( m_selectedGrid != nullptr && m_selectedRow != -1 )
	{
		const Uint32 pathColumnIndex = 2;
		String path = m_selectedGrid->GetCellValue( m_selectedRow, pathColumnIndex );

		if( path != String::EMPTY )
		{
			if ( CResource* resource = GDepot->LoadResource( path ) )
			{
				CEdResourceFinder::ShowForResource( resource );
			}
		}
		else
		{
			// this situation is possible only for entities because entity can be created without entity template (which is stored as resource)
			if( m_selectedGrid == GHelperGrids[TT_Entities] )
			{
				const Uint32 entityNameColumnIndex = 0;
				String entityName = m_selectedGrid->GetCellValue( m_selectedRow, entityNameColumnIndex );
				ShowEntityOnMap( entityName );
			}
		}
	}
}

Uint32 CEdWorldSceneDebugger::GridPointerToIndex( wxGrid* grid )
{
	for( Uint32 i=0; i<TT_Count; ++i )
	{
		if( GHelperGrids[i] == grid )
		{
			return i;
		}
	}
	return TT_Count;
}

void CEdWorldSceneDebugger::CreateAdditionalEntityContextMenu()
{
	// get paths to textures	 
	if( m_entitiesSubMenu != nullptr )
	{
		GContextMenus[TT_Entities]->Delete( m_entitiesSubMenu );
		m_entitiesSubMenu = nullptr;
	}
	if( m_entitiesSubMenu2 != nullptr )
	{
		GContextMenus[TT_Entities]->Delete( m_entitiesSubMenu2 );
		m_entitiesSubMenu2 = nullptr;
	}

	wxMenu* showTextureInAssetBrowser = new wxMenu();
	wxMenu* showTextureInTab = new wxMenu();
	CTokenizer tokens( m_selectedGrid->GetCellValue( m_selectedRow, 5 ).c_str().AsWChar(), TXT("\n") );
	Uint32 tokenCount = tokens.GetNumTokens();
	Uint32 constMenuItems = GConstMenuItems + 1;	// plus 1 because this menu has additional option "Show in scene explorer"
	for( Uint32 i=0; i<tokenCount; ++i ) 
	{
		showTextureInAssetBrowser->Append( i + constMenuItems, tokens.GetToken(i).AsChar(), nullptr );
		showTextureInTab->Append( tokenCount + i + constMenuItems, tokens.GetToken(i).AsChar(), nullptr );
	}
	m_entitiesSubMenu = GContextMenus[TT_Entities]->AppendSubMenu( showTextureInAssetBrowser, TXT("Show mesh") );
	m_entitiesSubMenu2 = GContextMenus[TT_Entities]->AppendSubMenu( showTextureInTab, TXT("Select in meshes tab") );
}

void CEdWorldSceneDebugger::CreateAdditionalMeshesContextMenu()
{
	// get paths to textures	 
	if( m_meshesSubMenu != nullptr )
	{
		GContextMenus[TT_Meshes]->Delete( m_meshesSubMenu );
		m_meshesSubMenu = nullptr;
	}
	if( m_meshesSubMenu2 != nullptr )
	{
		GContextMenus[TT_Meshes]->Delete( m_meshesSubMenu2 );
		m_meshesSubMenu2 = nullptr;
	}

	// create new menus
	wxMenu* showMeshInAssetBrowser = new wxMenu();
	wxMenu* showMeshInTab = new wxMenu();
	CTokenizer tokens( m_selectedGrid->GetCellValue( m_selectedRow, 7 ).c_str().AsWChar(), TXT("\n") );
	Uint32 tokenCount = tokens.GetNumTokens();
	for( Uint32 i=0; i<tokenCount; ++i ) 
	{
		showMeshInAssetBrowser->Append( i + GConstMenuItems, tokens.GetToken(i).AsChar(), nullptr );
		showMeshInTab->Append( tokenCount + i + GConstMenuItems, tokens.GetToken(i).AsChar(), nullptr );
	}
	m_meshesSubMenu = GContextMenus[TT_Meshes]->AppendSubMenu( showMeshInAssetBrowser, TXT("Show texture") );
	m_meshesSubMenu2 = GContextMenus[TT_Meshes]->AppendSubMenu( showMeshInTab, TXT("Select in textures tab") );
}

void CEdWorldSceneDebugger::CreateAdditionalTextureContextMenu()
{
	// get paths to textures	 
	if( m_texturesSubMenu != nullptr )
	{
		GContextMenus[TT_Textures]->Delete( m_texturesSubMenu );
		m_texturesSubMenu = nullptr;
	}
	if( m_texturesSubMenu2 != nullptr )
	{
		GContextMenus[TT_Textures]->Delete( m_texturesSubMenu2 );
		m_texturesSubMenu2 = nullptr;
	}
	if( m_texturesSubMenu3 != nullptr )
	{
		GContextMenus[TT_Textures]->Delete( m_texturesSubMenu3 );
		m_texturesSubMenu3 = nullptr;
	}
	if( m_texturesSubMenu4 != nullptr )
	{
		GContextMenus[TT_Textures]->Delete( m_texturesSubMenu4 );
		m_texturesSubMenu4 = nullptr;
	}

	// create new menus
	wxMenu* showMeshInAssetBrowser = new wxMenu();
	wxMenu* showMeshInTab = new wxMenu();
	wxMenu* showTextureArrayInAssetBrowser = new wxMenu();
	wxMenu* showTextureArrayInTab = new wxMenu();

	// add textures menu items
	CTokenizer meshTokens( m_selectedGrid->GetCellValue( m_selectedRow, 8 ).c_str().AsWChar(), TXT("\n") );
	Uint32 meshTokenCount = meshTokens.GetNumTokens();
	for( Uint32 i=0; i<meshTokenCount; ++i ) 
	{
		showMeshInAssetBrowser->Append( i + GConstMenuItems, meshTokens.GetToken(i).AsChar(), nullptr );
		showMeshInTab->Append( meshTokenCount + i + GConstMenuItems, meshTokens.GetToken(i).AsChar(), nullptr );
	}

	// add meshes menu items
	CTokenizer textureArrayTokens( m_selectedGrid->GetCellValue( m_selectedRow, 10 ).c_str().AsWChar(), TXT("\n") );
	Uint32 textureArrayTokenCount = textureArrayTokens.GetNumTokens();
	for( Uint32 i=0; i<textureArrayTokenCount; ++i ) 
	{
		showTextureArrayInAssetBrowser->Append( ( 2 * meshTokenCount ) + i + GConstMenuItems, textureArrayTokens.GetToken(i).AsChar(), nullptr );
		showTextureArrayInTab->Append( ( 2 * meshTokenCount ) + textureArrayTokenCount + i + GConstMenuItems, textureArrayTokens.GetToken(i).AsChar(), nullptr );
	}

	// connect new menus with parent menu
	m_texturesSubMenu = GContextMenus[TT_Textures]->AppendSubMenu( showMeshInAssetBrowser, TXT("Show mesh") );
	m_texturesSubMenu2 = GContextMenus[TT_Textures]->AppendSubMenu( showMeshInTab, TXT("Select in mesh tab") );
	m_texturesSubMenu3 = GContextMenus[TT_Textures]->AppendSubMenu( showTextureArrayInAssetBrowser, TXT("Show texture array") );
	m_texturesSubMenu4 = GContextMenus[TT_Textures]->AppendSubMenu( showTextureArrayInTab, TXT("Select in texture array tab") );
}

void CEdWorldSceneDebugger::CreateAdditionalTextureArrayContextMenu()
{
	// get paths to textures	 
	if( m_textureArraySubMenu != nullptr )
	{
		GContextMenus[TT_TextureArray]->Delete( m_textureArraySubMenu );
		m_textureArraySubMenu = nullptr;
	}
	if( m_textureArraySubMenu2 != nullptr )
	{
		GContextMenus[TT_TextureArray]->Delete( m_textureArraySubMenu2 );
		m_textureArraySubMenu2 = nullptr;
	}
	if( m_textureArraySubMenu3 != nullptr )
	{
		GContextMenus[TT_TextureArray]->Delete( m_textureArraySubMenu3 );
		m_textureArraySubMenu3 = nullptr;
	}
	if( m_textureArraySubMenu4 != nullptr )
	{
		GContextMenus[TT_TextureArray]->Delete( m_textureArraySubMenu4 );
		m_textureArraySubMenu4 = nullptr;
	}

	// create new menus
	wxMenu* showTextureInAssetBrowser = new wxMenu();
	wxMenu* showTextureInTab = new wxMenu();
	wxMenu* showMeshInAssetBrowser = new wxMenu();
	wxMenu* showMeshInTab = new wxMenu();

	// add textures menu items
	CTokenizer textureTokens( m_selectedGrid->GetCellValue( m_selectedRow, 5 ).c_str().AsWChar(), TXT("\n") );
	Uint32 textureTokenCount = textureTokens.GetNumTokens();
	for( Uint32 i=0; i<textureTokenCount; ++i ) 
	{
		showTextureInAssetBrowser->Append( i + GConstMenuItems, textureTokens.GetToken(i).AsChar(), nullptr );
		showTextureInTab->Append( textureTokenCount + i + GConstMenuItems, textureTokens.GetToken(i).AsChar(), nullptr );
	}

	// add meshes menu items
	CTokenizer meshTokens( m_selectedGrid->GetCellValue( m_selectedRow, 7 ).c_str().AsWChar(), TXT("\n") );
	Uint32 meshTokenCount = meshTokens.GetNumTokens();
	for( Uint32 i=0; i<meshTokenCount; ++i ) 
	{
		showMeshInAssetBrowser->Append( ( 2 * textureTokenCount ) + i + GConstMenuItems, meshTokens.GetToken(i).AsChar(), nullptr );
		showMeshInTab->Append( ( 2 * textureTokenCount ) + meshTokenCount + i + GConstMenuItems, meshTokens.GetToken(i).AsChar(), nullptr );
	}

	// connect new menus with parent menu
	m_textureArraySubMenu = GContextMenus[TT_TextureArray]->AppendSubMenu( showTextureInAssetBrowser, TXT("Show texture") );
	m_textureArraySubMenu2 = GContextMenus[TT_TextureArray]->AppendSubMenu( showTextureInTab, TXT("Select in textures tab") );
	m_textureArraySubMenu3 = GContextMenus[TT_TextureArray]->AppendSubMenu( showMeshInAssetBrowser, TXT("Show mesh") );
	m_textureArraySubMenu4 = GContextMenus[TT_TextureArray]->AppendSubMenu( showMeshInTab, TXT("Select in mesh tab") );
}

void CEdWorldSceneDebugger::OnAdditionalEntitiesContextMenu( wxCommandEvent& event )
{
	Uint32 option = event.GetId();

	// special case for additional option "Show in scene explorer"
	if( event.GetId() == 3 )
	{
		ShowEntityInSceneExplorer();
		return;
	}

	option -= ( GConstMenuItems + 1 );	// plus 1 because this menu has additional option "Show in scene explorer"

	CTokenizer tokens( m_selectedGrid->GetCellValue( m_selectedRow, 5 ).c_str().AsWChar(), TXT("\n") );
	Uint32 tokenCount = tokens.GetNumTokens();

	if( option < tokenCount )
	{
		String path = tokens.GetToken( option );
		ShowInAssetBrowser( path );
	}
	else
	{
		String path = tokens.GetToken( option - tokenCount );
		Goto( m_meshesGrid, TT_Meshes, path );
	}
}

void CEdWorldSceneDebugger::OnAdditionalMeshesContextMenu( wxCommandEvent& event )
{
	Uint32 option = event.GetId() - GConstMenuItems;

	CTokenizer tokens( m_selectedGrid->GetCellValue( m_selectedRow, 7 ).c_str().AsWChar(), TXT("\n") );
	Uint32 tokenCount = tokens.GetNumTokens();

	if( option < tokenCount )
	{
		String path = tokens.GetToken( option );
		ShowInAssetBrowser( path );
	}
	else
	{
		String path = tokens.GetToken( option - tokenCount );
		Goto( m_texturesGrid, TT_Textures, path );
	}
}

void CEdWorldSceneDebugger::OnAdditionalTexturesContextMenu( wxCommandEvent& event )
{
	Uint32 option = event.GetId() - GConstMenuItems;

	CTokenizer meshTokens( m_selectedGrid->GetCellValue( m_selectedRow, 8 ).c_str().AsWChar(), TXT("\n") );
	Uint32 meshTokenCount = meshTokens.GetNumTokens();
	CTokenizer textureArrayTokens( m_selectedGrid->GetCellValue( m_selectedRow, 10 ).c_str().AsWChar(), TXT("\n") );
	Uint32 textureArrayTokenCount = textureArrayTokens.GetNumTokens();

	if( option < meshTokenCount )
	{
		String path = meshTokens.GetToken( option );
		ShowInAssetBrowser( path );
	}
	else if( option < ( 2 * meshTokenCount ) )
	{
		String path = meshTokens.GetToken( option - meshTokenCount );
		Goto( m_meshesGrid, TT_Meshes, path );
	}
	else if( option < ( ( 2 * meshTokenCount ) + textureArrayTokenCount ) )
	{
		String path = textureArrayTokens.GetToken( option - ( 2 * meshTokenCount ) );
		ShowInAssetBrowser( path );
	}
	else
	{
		String path = textureArrayTokens.GetToken( option - ( ( 2 * meshTokenCount ) + textureArrayTokenCount ) );
		Goto( m_textureArraysGrid, TT_TextureArray, path );
	}
}

void CEdWorldSceneDebugger::OnAdditionalTextureArraysContextMenu( wxCommandEvent& event ) 
{
	Uint32 option = event.GetId() - GConstMenuItems;

	CTokenizer textureTokens( m_selectedGrid->GetCellValue( m_selectedRow, 5 ).c_str().AsWChar(), TXT("\n") );
	Uint32 textureTokenCount = textureTokens.GetNumTokens();
	CTokenizer meshTokens( m_selectedGrid->GetCellValue( m_selectedRow, 7 ).c_str().AsWChar(), TXT("\n") );
	Uint32 meshTokenCount = meshTokens.GetNumTokens();

	if( option < textureTokenCount )
	{
		String path = textureTokens.GetToken( option );
		ShowInAssetBrowser( path );
	}
	else if( option < ( 2 * textureTokenCount ) )
	{
		String path = textureTokens.GetToken( option - textureTokenCount );
		Goto( m_texturesGrid, TT_Textures, path );
	}
	else if( option < ( ( 2 * textureTokenCount ) + meshTokenCount ) )
	{
		String path = meshTokens.GetToken( option - ( 2 * textureTokenCount ) );
		ShowInAssetBrowser( path );
	}
	else
	{
		String path = meshTokens.GetToken( option - ( ( 2 * textureTokenCount ) + meshTokenCount ) );
		Goto( m_meshesGrid, TT_Meshes, path );
	}
}

void CEdWorldSceneDebugger::ShowEntityOnMap( const String& entityName /*= String::EMPTY*/ )
{
	CSelectionManager *selectionManager = GGame->GetActiveWorld()->GetSelectionManager();
	CSelectionManager::CSelectionTransaction transaction(*selectionManager);
	selectionManager->DeselectAll();

	// get entity name
	String name = entityName;
	if( name == String::EMPTY )
	{
		if( m_selectedGrid == GHelperGrids[TT_Entities] && m_selectedRow != -1 )
		{
			const Uint32 nameColumnIndex = 0;
			name = m_selectedGrid->GetCellValue( m_selectedRow, nameColumnIndex );
		}
	}

	for ( WorldAttachedComponentsIterator it( GGame->GetActiveWorld() ); it; ++it )
	{
		CMeshComponent *comp = Cast< CMeshComponent > ( *it );
		if ( comp && comp->GetEntity() && comp->GetEntity()->GetDisplayName() == name )
		{
			selectionManager->Select( comp );
			wxTheFrame->GetWorldEditPanel()->LookAtNode( comp );
		}
	}
}

void CEdWorldSceneDebugger::SelectRow( wxGrid* newSelectedGrid, Uint32 rowIndex )
{
	wxWindowUpdateLocker localUpdateLocker( this );

	if( m_selectedGrid != nullptr && m_selectedRow != -1 )
	{
		// unselect old row
		if( newSelectedGrid != m_selectedGrid )
		{
			m_selectedGrid->SelectRow( -1 );
		}
	}

	m_selectedGrid = newSelectedGrid;
	m_selectedRow = rowIndex;
	m_selectedGrid->SelectRow( rowIndex );
}

void CEdWorldSceneDebugger::ScrollToSelectedRow()
{
	if( m_selectedGrid != nullptr && m_selectedRow != -1 )
	{
		int stepx, stepy;
		m_selectedGrid->GetScrollPixelsPerUnit( &stepx, &stepy );

		if ( stepy > 0 )
		{			wxGridSizesInfo info = m_selectedGrid->GetRowSizes();
			int selectedRowPosition = 0;
			for( int i=0; i<m_selectedRow; ++i )
			{
				selectedRowPosition += info.GetSize( i );
			}

			int rowPosition = selectedRowPosition / stepy;

			m_selectedGrid->Scroll( -1, rowPosition );
		}
	}
}

void CEdWorldSceneDebugger::Goto( wxGrid* grid, Uint32 tabIndex, const String& texturePath )
{
	m_notebook->SetSelection( tabIndex );

	Uint32 pathColumnIndex = 2;
	for( Int32 i=0; i<grid->GetNumberRows(); ++i )
	{
		const String path = grid->GetCellValue( i, pathColumnIndex );
		if( texturePath == path )
		{
			SelectRow( grid, i );
			ScrollToSelectedRow();
			return;
		}
	}
}
void CEdWorldSceneDebugger::UpdateThumbnail( wxGridEvent& event )
{
	m_selectedGrid = wxDynamicCast( event.GetEventObject(), wxGrid );
	m_selectedRow = event.GetRow();

	wxStaticBitmap* selectedGridThumbnail = wxDynamicCast( m_selectedGrid->GetClientData(), wxStaticBitmap );

	if( m_selectedRow < m_selectedGrid->GetNumberRows() )
	{
		wxString val = m_selectedGrid->GetCellValue( m_selectedRow, 2 ); // 2 m eans 2 columna with path data
		CName path( val.wc_str() );

		wxImage* image = nullptr;
		Bool createDefaultImage = true;
		Int32 wid = selectedGridThumbnail->GetSize().GetWidth();
		Int32 hei = selectedGridThumbnail->GetSize().GetHeight();

		CResource* res = GDepot->LoadResource( val.wc_str() );
		if ( res )
		{
			CDiskFile* diskFile = res->GetFile();
			if( diskFile )
			{
				diskFile->LoadThumbnail();
				TDynArray< CThumbnail* > thumbnail = diskFile->GetThumbnails();

				if( !thumbnail.Empty() )
				{
					CWXThumbnailImage* wxImg = (CWXThumbnailImage*)( thumbnail[0]->GetImage() );

					if ( wxImg )
					{
						const Uint32 w = wxImg->GetWidth();
						const Uint32 h = wxImg->GetHeight();

						createDefaultImage = false;
						image = new wxImage( w, h );
						Gdiplus::Color pixelColor;
						for ( Uint32 y=0; y<h; y++ )
						{
							Uint32 srcY = y * h / h;
							for( Uint32 x=0; x<w; x++ )
							{
								Uint32 srcX = x * w / w;
								wxImg->GetBitmap()->GetPixel( srcX, srcY, &pixelColor );
								image->SetRGB( x, y, pixelColor.GetRed(), pixelColor.GetGreen(), pixelColor.GetBlue() );
							}
						}
						image->Rescale( wid, hei );
					}
				}
			}
		}
		if ( createDefaultImage )
		{
			image = new wxImage( wid, hei );
			image->SetRGB( wxRect( wxPoint( 0, 0 ), image->GetSize() ), 255, 255, 255 );
		}
		wxBitmap* wb = new wxBitmap( *image );
		selectedGridThumbnail->SetBitmap( *wb );
		this->Refresh();
	}
}

void CEdWorldSceneDebugger::GatherInformationAboutApexResource( const THandle< CApexResource >& apexResource, TDynArray< SInternalApexInfo >& allApexInfos, EApexType apexType )
{
	Bool foundApexInfo = false;
	for( Uint32 j=0; j<allApexInfos.Size(); ++j )
	{
		if( allApexInfos[j].m_apexResource == apexResource )
		{
			++allApexInfos[j].m_refCount;
			foundApexInfo = true;
			break;
		}
	}

	if( foundApexInfo == false )
	{
		// get lod count
		Uint32 lodCount = apexResource->GetNumLODLevels();

		// sum triangles for all LODs
		Uint32 triangleCount = 0;
		Uint32 verticesCount = 0;
		for( Uint32 j=0; j<lodCount; ++j )
		{
			triangleCount += apexResource->CountLODTriangles( j );
			verticesCount += apexResource->CountLODVertices( j );
		}

		// if apex resource is not found in collection
		allApexInfos.PushBack( SInternalApexInfo( apexResource.Get(), apexType, lodCount, triangleCount, verticesCount ) );
	}
}

void CEdWorldSceneDebugger::OnAutohideToggle( wxCommandEvent& event )
{
	Bool val = event.GetInt() == 0 ? false : true;
	UpdateForceNoAutohideButtonLabel( val );
	
	wxTheFrame->ForceNoAutohide( val, true );
}

void CEdWorldSceneDebugger::UpdateForceNoAutohideButtonLabel( Bool forceNoAutohide )
{
	if ( wxToggleButton* btn = XRCCTRL( *this, "autohideToggle", wxToggleButton ) )
	{
		btn->SetLabel( wxString( forceNoAutohide ? TXT("Autohide disabled (turnOff: GeometryProxy)") : TXT("Autohide enabled") ) );
	}
}

void CEdWorldSceneDebugger::OnModeTabChanged( wxNotebookEvent& event )
{
	if (event.GetSelection() != -1)
	{
		Int32 sel = event.GetSelection();
		// 0 means go to memory tab
		if( sel == 0 && !m_dataCollected )
		{
			CollectDataFromWorld();
			m_dataCollected = true;
		}
	}
}

void CEdWorldSceneDebugger::OnSpinnerChanged( wxSpinEvent& event )
{
	PrepareParamsToUpdate();
}
void CEdWorldSceneDebugger::OnSliderChanged( wxCommandEvent& event )
{
	PrepareParamsToUpdate();
}

#ifndef NO_EDITOR_EVENT_SYSTEM
void CEdWorldSceneDebugger::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	// Show/hide layer when in editor and PP mode is changed
	if ( name == CNAME( EnginePerformancePlatformChanged ) )
	{
		UpdateInternalData();
	}
}
#endif // NO_EDITOR_EVENT_SYSTEM

#ifndef NO_EDITOR
void CEdWorldSceneDebugger::UpdateInternalData()
{
	GetSRenderSettingsFromINI();
	UpdateUI();
	//update both tree grass
	PrepareParamsToUpdate();
}

void CEdWorldSceneDebugger::UpdateForceRenderOptions()
{
	CWorld* w = GGame->GetActiveWorld();
	if ( !w ) return;
	CFoliageEditionController &med = w->GetFoliageEditionController();
	med.UpdateFoliageRenderParams( m_foliageRenderParams );

	// turn off autohide for all meshes on the map
	TDynArray< CMeshTypeComponent* > components;
	// dont need to check world cause world scene debugger only run if there is world
	w->GetAttachedComponentsOfClass( components );
	for( Uint32 i=0; i<components.Size(); ++i )
	{
		components[i]->UpdateRenderDistanceParams( m_meshRenderParams );
	}
}

void CEdWorldSceneDebugger::GetSRenderSettingsFromINI()
{
	m_foliageRenderParams.m_foliageDistShift = 1.0f;//SRenderSettingsManager::GetInstance().GetSettings().m_foliageDistanceScale;
	m_foliageRenderParams.m_grassDistScale = 1.0f;//SRenderSettingsManager::GetInstance().GetSettings().m_grassDistanceScale;
	m_meshRenderParams.m_meshRenderDist = Config::cvMeshRenderingDistanceScale.Get();
	m_meshRenderParams.m_meshLODRenderDist = Config::cvMeshLODDistanceScale.Get();
}

void CEdWorldSceneDebugger::UpdateUI()
{
	Float sc = 1000.f;
	m_foliageDistanceSlider->SetValue( m_foliageRenderParams.m_foliageDistShift * sc );
	m_grassDistSlider->SetValue( m_foliageRenderParams.m_grassDistScale * sc );
	m_meshRenderDistSlider->SetValue( m_meshRenderParams.m_meshRenderDist * sc );
	m_meshLODDistSlider->SetValue( m_meshRenderParams.m_meshLODRenderDist * sc );

	//cache values
	m_treeDistCurrent = m_foliageRenderParams.m_foliageDistShift;
	m_grassDistCurrent = m_foliageRenderParams.m_grassDistScale;
	m_meshRenderDistCurrent = m_meshRenderParams.m_meshRenderDist;
	m_meshLODRenderDistCurrent = m_meshRenderParams.m_meshLODRenderDist;
}

void CEdWorldSceneDebugger::PrepareParamsToUpdate()
{
	Float sc = 0.001f;
	Float retValue = 0.f;

	//slider values
	Float fd	= sc * m_foliageDistanceSlider->GetValue();
	Float gd	= sc * m_grassDistSlider->GetValue();
	Float md	= sc * m_meshRenderDistSlider->GetValue();
	Float ld	= sc * m_meshLODDistSlider->GetValue();

	//spinner values
	Int32 fSpin = m_foliageSpinner->GetValue();
	Int32 gSpin = m_grassSpinner->GetValue();
	Int32 mSpin = m_meshRenderDistSpinner->GetValue();
	Int32 lSpin = m_meshLODDistSpinner->GetValue();

	// current values
	fd *= fSpin;
	gd *= gSpin;
	md *= mSpin;
	ld *= lSpin;

	//cache tree dist
	m_treeDistPrevious = m_treeDistCurrent;
	m_treeDistCurrent = 1.f;
	retValue = m_treeDistCurrent / m_treeDistPrevious;
	// set tree dist
	m_foliageRenderParams.m_foliageDistShift	= retValue;

	//cache grass dist
	m_grassDistPrevious = m_grassDistCurrent;
	m_grassDistCurrent = 1.f;
	retValue = m_grassDistCurrent / m_grassDistPrevious;
	// set grass dist
	m_foliageRenderParams.m_grassDistScale		= retValue;

	//cache mesh render dist
	m_meshRenderDistPrevious = m_meshRenderDistCurrent;
	m_meshRenderDistCurrent = 1.f;
	retValue = m_meshRenderDistCurrent / m_meshRenderDistPrevious;
	// set mesh dist
	m_meshRenderParams.m_meshRenderDist			= retValue;

	//cache mesh lod render dist
	m_meshLODRenderDistPrevious = m_meshLODRenderDistCurrent;
	m_meshLODRenderDistCurrent = 1.f;
	retValue = m_meshLODRenderDistCurrent / m_meshLODRenderDistPrevious;
	// set lod mesh dist
	m_meshRenderParams.m_meshLODRenderDist		= retValue;

	//update to default render values
	UpdateForceRenderOptions();

	//scale tree dist
	m_treeDistPrevious = 1.f;
	m_treeDistCurrent = fd;
	retValue = m_treeDistCurrent / m_treeDistPrevious;

	m_foliageRenderParams.m_foliageDistShift	= retValue;

	//scale grass dist
	m_grassDistPrevious = 1.f;
	m_grassDistCurrent = gd;
	retValue = m_grassDistCurrent / m_grassDistPrevious;

	m_foliageRenderParams.m_grassDistScale		= retValue;

	//scale mesh dist
	m_meshRenderDistPrevious = 1.f;
	m_meshRenderDistCurrent = md;
	retValue = m_meshRenderDistCurrent / m_meshRenderDistPrevious;

	m_meshRenderParams.m_meshRenderDist			= retValue;

	//scale lod render params
	m_meshLODRenderDistPrevious = 1.f;
	m_meshLODRenderDistCurrent = ld;
	retValue = m_meshLODRenderDistCurrent / m_meshLODRenderDistPrevious;

	m_meshRenderParams.m_meshLODRenderDist		= retValue;

	//update new values
	UpdateForceRenderOptions();
}

#endif
