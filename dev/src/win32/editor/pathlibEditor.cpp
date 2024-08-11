#include "build.h"

#include "pathlibEditor.h"

#include "../../common/core/depot.h"

#include "../../common/engine/clipMap.h"
#include "../../common/engine/pathlibConst.h"
#include "../../common/engine/pathlibAreaDescription.h"
#include "../../common/engine/pathlibWorld.h"
#include "../../common/engine/pathlibTerrain.h"
#include "../../common/engine/pathlibVisualizer.h"
#include "../../common/engine/pathlibNavmesh.h"
#include "../../common/engine/pathlibNavmeshArea.h"
#include "../../common/engine/pathlibNavmeshComponent.h"
#include "../../common/engine/pathlibObstacleMapper.h"
#include "../../common/engine/pathlibNavmeshLegacy.h"
#include "../../common/engine/pathlibAgent.h"
#include "../../common/engine/pathlibObstaclesMap.h"
#include "../../common/engine/pathlibSimpleBuffers.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/renderFrame.h"

#include "../../common/game/wayPointComponent.h"

#include "filterPanel.h"
#include "sceneExplorer.h"
#include "../../common/engine/worldIterators.h"

#define INVALID_LIST_POSITION -1

wxDEFINE_EVENT( wxEVT_VIEWTOOL_TOGGLED, wxCommandEvent );
wxDEFINE_EVENT( wxEVT_EXCLUSIVETOOL_TOGGLED, wxCommandEvent );

IMPLEMENT_ENGINE_CLASS( CEdPathlib );

wxIMPLEMENT_CLASS( CEdPathlibPanel, CEdDraggablePanel );

BEGIN_EVENT_TABLE( CEdPathlibPanel, CEdDraggablePanel )
END_EVENT_TABLE()

const Float CEdPathlib::RECALCULATOR_DELAY_FOR_DOUBLECLICK = 1.f;

extern CEdFrame* wxTheFrame;

////////////////////////////////////////////////////////////////////////////
// CPathlibListener
////////////////////////////////////////////////////////////////////////////
void CEdPathlibPanel::CPathlibListener::WorldDestroyedLocked()
{

}
void CEdPathlibPanel::CPathlibListener::TaskStatusChangedLocked( PathLib::AreaId taskId, ETaskStatus status, const String& taskDescription )
{
	m_owner->SetStatus( taskId, taskDescription, status );
}

////////////////////////////////////////////////////////////////////////////
// CEdPathlibPanel
////////////////////////////////////////////////////////////////////////////
CEdPathlibPanel::CEdPathlibPanel( CEdRenderingPanel* viewport, CEdPathlib* tool, wxWindow* parent, CWorld* world )
	: m_world( world )
	, m_owner( tool )
	, m_status( NULL )
	, m_log( NULL )
	, m_previousActiveItem( INVALID_LIST_POSITION )
	, m_exclusiveToggleBar( NULL )
	, m_activeTool( NULL )
	, m_wxIds( wxID_HIGHEST )
	, m_lastLogVersion( 0xbaadf00d )
	, m_updateStatus( false )
	, m_pathlibListener( NULL )
{
	VERIFY( wxXmlResource::Get()->LoadPanel( this, parent, wxT("PathLibPanel") ) );

	CPathLibWorld* pathlib = world->GetPathLibWorld();
	PathLib::CVisualizer* visualizer = pathlib ? pathlib->GetVisualizer() : nullptr;

	// Status
	m_status = XRCCTRL( *this, "StatusList", wxListCtrl );
	ASSERT( m_status != NULL );

	m_status->InsertColumn( SC_Message, wxT( "Task" ) );
	m_status->InsertColumn( SC_Status, wxT( "Status" ), wxLIST_FORMAT_LEFT, wxLIST_AUTOSIZE_USEHEADER );

	// Log
	m_log = XRCCTRL( *this, "LogList", wxListCtrl );
	ASSERT( m_log != NULL );

	m_log->InsertColumn( 0, wxT( "Message" ) );
	
	// Toolbars
	m_optionBar = XRCCTRL( *this, "OptionBar", wxToolBar );
	ASSERT( m_optionBar != NULL );

	m_exclusiveToggleBar = XRCCTRL( *this, "ExclusiveToggleBar", wxToolBar );
	ASSERT( m_exclusiveToggleBar != NULL );

	// View panel
	m_viewList = XRCCTRL( *this, "ViewList", wxCheckListBox );
	ASSERT( m_viewList != NULL );

	m_viewList->Bind( wxEVT_COMMAND_CHECKLISTBOX_TOGGLED, &CEdPathlibPanel::OnViewItemChecked, this );

	// Custom Controls - Brush
	m_brushRadiusSpin = XRCCTRL( *this, "BrushRadiusSpin", wxSpinCtrl );
	ASSERT( m_brushRadiusSpin != NULL );

	m_brushRadiusSlider = XRCCTRL( *this, "BrushRadiusBar", wxSlider );
	ASSERT( m_brushRadiusSlider != NULL );

	m_brushRadiusSpin->Bind( wxEVT_COMMAND_SPINCTRL_UPDATED, &CEdPathlibPanel::OnBrushRadiusChangeSpin, this );
	m_brushRadiusSlider->Bind( wxEVT_COMMAND_SLIDER_UPDATED, &CEdPathlibPanel::OnBrushRadiusChangeSlider, this );

	// Dummy Tool example
	AddTool( TXT( "Update waypoints" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnRecalculateWaypoints, this );
	//AddTool( TXT( "Cook" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnCook, this );
	//AddTool( TXT( "Convert\nALL\nNavmeshes" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnConvertAllNavmeshes, this );
	AddTool( TXT( "Cleanup local folder" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnCleanupLocalFolder, this );

	m_showNavgraph[ 0 ] = AddToggleTool( TXT("Show base navgraph"), TXT("IMG_BRICKS"), &CEdPathlibPanel::OnShowNavgraph0, this, visualizer ? visualizer->GetDebuggedNavgraph() == 0 : true );
	m_showNavgraph[ 1 ] = AddToggleTool( TXT("Show navgraph I"), TXT("IMG_BRICKS"), &CEdPathlibPanel::OnShowNavgraph1, this, visualizer ? visualizer->GetDebuggedNavgraph() == 1 : false );
	m_showNavgraph[ 2 ] = AddToggleTool( TXT("Show navgraph II"), TXT("IMG_BRICKS"), &CEdPathlibPanel::OnShowNavgraph2, this, visualizer ? visualizer->GetDebuggedNavgraph() == 2 : false );
	m_showNavgraph[ 3 ] = AddToggleTool( TXT("Show navgraph III"), TXT("IMG_BRICKS"), &CEdPathlibPanel::OnShowNavgraph3, this, visualizer ? visualizer->GetDebuggedNavgraph() == 3 : false );
	//AddExclusiveToggleTool( TXT( "Brush\nBlocker" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnBrushNonWalkable, this );
	//AddExclusiveToggleTool( TXT( "Brush\nWalkable" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnBrushWalkable, this );
	AddExclusiveToggleTool( TXT( "Recalculator" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnRecalculatorToggle, this );
	//AddExclusiveToggleTool( TXT( "Navmesh editor" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnNavmeshEditorToggle, this );
	AddExclusiveToggleTool( TXT( "Delete navmesh" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnNavmeshDeleterToggle, this );
	AddExclusiveToggleTool( TXT( "Path debugger" ), TXT( "IMG_BRICKS" ), &CEdPathlibPanel::OnPathDebuggerToggle, this );



	// debug viewers
	Bool navmeshFilterOn = false;
	Bool navmeshTrianglesFilterOn = false;
	Bool navmeshOverlay = false;
	Bool navgraphFilterOn = false;
	Bool navgraphNoOcclusionFilterOn = false;
	Bool navgraphRegionsFilterOn = false;
	Bool terrainFilterOn = false;
	Bool obstaclesFilterOn = false;

	// read debug viewers state
	const EShowFlags* flags = wxTheFrame->GetFilterPanel()->GetViewportFlags( GGame->IsActive() ? VFT_GAME : VFT_EDITOR );
	while ( (*flags) != SHOW_MAX_INDEX )
	{
		switch ( *flags )
		{
		case SHOW_NavMesh:
			navmeshFilterOn = true;
			break;
		case SHOW_NavMeshTriangles:
			navmeshTrianglesFilterOn = true;
			break;
		case SHOW_NavMeshOverlay:
			navmeshOverlay = true;
			break;
		case SHOW_NavTerrain:
			terrainFilterOn = true;
			break;
		case SHOW_NavGraph:
			navgraphFilterOn = true;
			break;
		case SHOW_NavGraphNoOcclusion:
			navgraphNoOcclusionFilterOn = true;
			break;
		case SHOW_NavGraphRegions:
			navgraphRegionsFilterOn = true;
			break;
		case SHOW_NavObstacles:
			obstaclesFilterOn = true;
			break;
		
		default:
			break;
		}
		++flags;
	}

	AddView( TXT( "Navmesh" ), &CEdPathlibPanel::OnViewNavmesh, this, navmeshFilterOn );
	AddView( TXT( "NavmeshTriangles" ), &CEdPathlibPanel::OnViewNavmeshTriangles, this, navmeshTrianglesFilterOn );
	AddView( TXT( "NavmeshOverlay" ), &CEdPathlibPanel::OnViewNavmeshOverlay, this, navmeshOverlay );
	AddView( TXT( "Navgraph" ), &CEdPathlibPanel::OnViewNavgraph, this, navgraphFilterOn );
	AddView( TXT( "NavgraphOverlay" ), &CEdPathlibPanel::OnViewNavgraphNoOcclusion, this, navgraphNoOcclusionFilterOn );
	AddView( TXT( "NavgraphRegions" ), &CEdPathlibPanel::OnViewNavgraphRegions, this, navgraphRegionsFilterOn );
	AddView( TXT( "Terrain" ), &CEdPathlibPanel::OnViewTerrain, this, terrainFilterOn );
	AddView( TXT( "Obstacles" ), &CEdPathlibPanel::OnViewObstacles, this, obstaclesFilterOn );

	m_detachablePanel.Initialize( this, TXT( "PathLib tool" ) );

	if ( pathlib )
	{
		m_pathlibListener = new CPathlibListener( world, this );
		pathlib->GetGenerationManager()->RegisterStatusListener( m_pathlibListener );
	}
}

CEdPathlibPanel::~CEdPathlibPanel()
{
	if ( m_pathlibListener )
	{
		m_pathlibListener->Unregister();
		delete m_pathlibListener;
	}
}

void CEdPathlibPanel::SetStatus( PathLib::AreaId areaId, const String& message, PathLib::CGenerationManager::CStatusListener::ETaskStatus status )
{
	StatusId itemId = m_status->FindItem( -1, areaId );
	if ( itemId < 0 )
	{
		itemId = m_status->InsertItem( m_status->GetItemCount(), message.AsChar() );
		m_status->SetItemData( itemId, areaId );
	}
	const Char* statusText;
	const wxColour* statusColor;
	switch( status )
	{
	case PathLib::CGenerationManager::CStatusListener::Status_Pending:
		statusText = TXT("Pending");
		statusColor = wxLIGHT_GREY;
		break;
	case PathLib::CGenerationManager::CStatusListener::Status_Active:
		statusText = TXT("Active");
		statusColor = wxYELLOW;
		break;
	case PathLib::CGenerationManager::CStatusListener::Status_Complete:
		statusText = TXT("Completed");
		statusColor = wxGREEN;
		break;
	case PathLib::CGenerationManager::CStatusListener::Status_Failed:
	default:
		statusText = TXT("Failed");
		statusColor = wxRED;
		break;
	}
	m_status->SetItem( itemId, SC_Status, statusText );
	if ( status != PathLib::CGenerationManager::CStatusListener::Status_Complete && status != PathLib::CGenerationManager::CStatusListener::Status_Failed )
	{
		m_status->SetItem( itemId, SC_Message, message.AsChar() );
	}
	m_status->SetItemTextColour( itemId, *statusColor );

	m_updateStatus = true;

	if( status == PathLib::CGenerationManager::CStatusListener::Status_Active )
	{
		if( IsRowVisible( m_previousActiveItem, m_status ) )
		{
			m_status->EnsureVisible( itemId );
		}
		m_previousActiveItem = itemId;
	}
}


void CEdPathlibPanel::Log( const Char* message, ... )
{
	va_list arglist;
	va_start( arglist, message );
	
	String formattedMessage = String::PrintfV( message, arglist );
	long row = m_log->InsertItem( m_log->GetItemCount(), formattedMessage.AsChar() );
	m_log->SetColumnWidth( 0, wxLIST_AUTOSIZE );
	
	if( IsRowVisible( row - 1, m_log ) )
	{
		m_log->EnsureVisible( row );
	}

	va_end( arglist );
}

void CEdPathlibPanel::Update()
{
	PathLib::CLog* log = PathLib::CLog::GetInstance();
	if ( log->GetCurrentVersion() != m_lastLogVersion )
	{
		String text;

		PathLib::CLog::Lock locked( log->GetMutex() );
		m_lastLogVersion = log->GetCurrentVersion();

		// get new lines count
		Int32 newLines = 0;
		for ( Int32 linesCount = log->GetLinesCount(); newLines < linesCount; ++newLines )
		{
			if ( log->GetLineFlag( newLines ) & PathLib::CLog::F_RECEIVED )
			{
				break;
			}
			log->MarkLineAsReceived( newLines );
		}

		if ( newLines > 0 )
		{
			Int32 firstRow = -1;
			Int32 lastRow = 0;
			for ( Int32 i = newLines-1; i >= 0; --i )
			{
				const String& line = log->GetLine( i );
				lastRow = m_log->InsertItem( m_log->GetItemCount(), line.AsChar() );
				if ( firstRow == -1 )
				{
					firstRow = lastRow-1;
				}
				m_log->SetColumnWidth( 0, wxLIST_AUTOSIZE );
			}

			m_log->EnsureVisible( lastRow );
		}
	}
	if ( m_updateStatus )
	{
		m_status->SetColumnWidth( SC_Message, wxLIST_AUTOSIZE );
		m_updateStatus = false;
	}
}

void CEdPathlibPanel::OnExclusiveToolToggled( wxCommandEvent& event )
{
	if( m_activeTool )
	{
		m_exclusiveToggleBar->ToggleTool( m_activeTool->GetId(), false );

		wxCommandEvent newEvent( wxEVT_EXCLUSIVETOOL_TOGGLED );
		newEvent.SetEventObject( this );
		newEvent.SetId( m_activeTool->GetId() );
		newEvent.SetInt( 0 );

		// Since we're already in an event, process immediately
		GetEventHandler()->ProcessEvent( newEvent );

		m_activeTool = NULL;
	}

	wxToolBarToolBase* toggledTool = m_exclusiveToggleBar->FindById( event.GetId() );
	ASSERT( toggledTool != NULL );

	if( toggledTool->IsToggled() )
	{
		wxCommandEvent newEvent( wxEVT_EXCLUSIVETOOL_TOGGLED );
		newEvent.SetEventObject( this );
		newEvent.SetId( event.GetId() );
		newEvent.SetInt( 1 );

		// Since we're already in an event, process immediately
		GetEventHandler()->ProcessEvent( newEvent );

		m_activeTool = toggledTool;
	}
}

void CEdPathlibPanel::OnViewItemChecked( wxCommandEvent& event )
{
	CEdViewToolData* data = static_cast< CEdViewToolData* >( m_viewList->GetClientObject( event.GetInt() ) );
	ASSERT( data != NULL, TXT( "Warning, no ID wxClientData associated with CEdPathlibPanel view item" ) );

	wxCommandEvent newEvent( wxEVT_VIEWTOOL_TOGGLED );
	newEvent.SetEventObject( this );
	newEvent.SetId( data->id );
	newEvent.SetInt( ( m_viewList->IsChecked( event.GetInt() ) ) ? 1 : 0 );

	// Since we're already in an event, process immediately
	GetEventHandler()->ProcessEvent( newEvent );
}

void CEdPathlibPanel::OnBrushRadiusChangeSpin( wxSpinEvent& event )
{
	m_brushRadius = event.GetPosition();
	m_brushRadiusSlider->SetValue( m_brushRadius );
	m_owner->SetBrushRadius( m_brushRadius );
}

void CEdPathlibPanel::OnBrushRadiusChangeSlider( wxCommandEvent& event )
{
	m_brushRadius = event.GetInt();
	m_brushRadiusSpin->SetValue( m_brushRadius );
	m_owner->SetBrushRadius( m_brushRadius );
}

Bool CEdPathlibPanel::IsRowVisible( long row, wxListCtrl* listCtrl ) const
{
	Int32 pageCount		= listCtrl->GetCountPerPage();
	Int32 TopVisibleRow	= listCtrl->GetTopItem();

	if( row >= TopVisibleRow && row < TopVisibleRow + pageCount )
	{
		return true;
	}

	return false;
}

RED_INLINE CPathLibWorld* CEdPathlibPanel::GetPathlib()
{
	CWorld* world = m_world.Get();
	if ( world )
	{
		return world->GetPathLibWorld();
	}
	return nullptr;
}

void CEdPathlibPanel::OnCleanupLocalFolder( wxCommandEvent& event )
{
	 CPathLibWorld* pathlib = GetPathlib();
	 if ( !pathlib )
	 {
		 return;
	 }
	 CDirectory* dir = pathlib->ForceGetLocalDataDirectory();
	 if ( !dir )
	 {
		 return;
	 }
	 TFiles files = dir->GetFiles();

	 for ( auto it = files.Begin(), end = files.End(); it != end; ++it )
	 {
		 CDiskFile* file = *it;
		 file->GetStatus();
		 file->Delete( false, false );
	 }
}

void CEdPathlibPanel::OnConvertAllNavmeshes( wxCommandEvent& event )
{
	struct CResaver
	{
		enum ESupportedTypes
		{
			T_NOT_SUPPORTED,
			T_NAVMESH,
		};

		CResaver()
			: m_extensionNavmesh( CNavmesh::GetFileExtension() )

		{}

		ESupportedTypes GetFileType( const String& fileName )
		{
			if ( fileName.EndsWith( m_extensionNavmesh ) )
			{
				return T_NAVMESH;
			}

			return T_NOT_SUPPORTED;
		}

		void DoDirectoryLevel( CDirectory* dir )
		{
			auto funIterate =
				[ this ] ( CDiskFile* file )
			{
				const String& fileName = file->GetFileName();
				ESupportedTypes t = GetFileType( fileName.ToLower() );
				if ( t == T_NOT_SUPPORTED )
				{
					return;
				}

				if ( file->Load() )
				{
					CResource* res = file->GetResource();
					if ( res )
					{
						const String& filePath = file->GetDepotPath();
						// Its old type resource. Almost for sure its one of ours!
						file->SilentCheckOut();
						Bool isLegacy = false;
						Bool hasRead = false;
						Bool hasSaved = false;

						switch ( t )
						{
						case T_NAVMESH:
							{
								CNavmesh* legacyNavmesh = Cast< CNavmesh >( res );
								if ( legacyNavmesh )
								{
									isLegacy = true;
									hasRead = true;

									PathLib::CNavmeshRes* convertedNavmesh = new PathLib::CNavmeshRes();

									PathLib_TEMP_Convert( legacyNavmesh, convertedNavmesh );

									if ( convertedNavmesh->Save( filePath ) )
									{
										hasSaved = true;
										RED_LOG( Gui, TXT("Navmesh '%s' resaved.\n"), filePath.AsChar() );
									}
									delete convertedNavmesh;
								}
							}

						default:
							ASSUME( false );
						}

						if ( !isLegacy )
						{
							RED_LOG( Gui, TXT("File is decent '%s'.\n"), filePath.AsChar() );
						}
						else
						{
							if( !hasRead )
							{
								RED_LOG( Gui, TXT("Problems reading '%s'.\n"), filePath.AsChar() );
							}
							else if ( !hasSaved )
							{
								RED_LOG( Gui, TXT("Problems saving '%s'.\n"), filePath.AsChar() );
							}
						}


						file->Unload();
					}
				}

			};

			for ( auto it : dir->GetFiles() )
			{
				funIterate( it );
			}

			for ( CDirectory* childDir : dir->GetDirectories() )
			{
				DoDirectoryLevel( childDir );
			}
		}

		String			m_extensionNavmesh;
		//String			m_extensionNavgraph;
		//String			m_extensionTerrain;
		//String			m_extensionObstacles;
	} resaver;


	resaver.DoDirectoryLevel( GDepot );
}
//void CEdPathlibPanel::OnCook( wxCommandEvent& event )
//{
//	CPathLibWorld* pathlibWorld = GetPathlib();
//	if ( pathlibWorld )
//	{
//	}
//}
void CEdPathlibPanel::OnReinitializeSystem( wxCommandEvent& event )
{
	CPathLibWorld* pathlibWorld = GetPathlib();
	if ( pathlibWorld )
	{
		pathlibWorld->ReinitializeSystem();
	}
}

void CEdPathlibPanel::OnRecalculateWaypoints( wxCommandEvent& event )
{
	CWayPointComponent::RefreshWaypoints();
}

void CEdPathlibPanel::OnShowNavgraph0( wxCommandEvent& event )
{
	CPathLibWorld* pathlibWorld = GetPathlib();
	PathLib::CVisualizer* vizualizer = pathlibWorld ? pathlibWorld->GetVisualizer() : nullptr;
	if ( vizualizer )
	{
		vizualizer->DebugNavgraph( 0 );
		for( Uint32 i = 0; i < 4; ++i )
		{
			m_showNavgraph[ i ]->Toggle( i == 0 );
		}
	}
}

void CEdPathlibPanel::OnShowNavgraph1( wxCommandEvent& event )
{
	CPathLibWorld* pathlibWorld = GetPathlib();
	PathLib::CVisualizer* vizualizer = pathlibWorld ? pathlibWorld->GetVisualizer() : nullptr;
	if ( vizualizer )
	{
		vizualizer->DebugNavgraph( 1 );
		for( Uint32 i = 0; i < 4; ++i )
		{
			m_showNavgraph[ i ]->Toggle( i == 1 );
		}
	}
}

void CEdPathlibPanel::OnShowNavgraph2( wxCommandEvent& event )
{
	CPathLibWorld* pathlibWorld = GetPathlib();
	PathLib::CVisualizer* vizualizer = pathlibWorld ? pathlibWorld->GetVisualizer() : nullptr;
	if ( vizualizer )
	{
		vizualizer->DebugNavgraph( 2 );
		for( Uint32 i = 0; i < 4; ++i )
		{
			m_showNavgraph[ i ]->Toggle( i == 2 );
		}
	}
}

void CEdPathlibPanel::OnShowNavgraph3( wxCommandEvent& event )
{
	CPathLibWorld* pathlibWorld = GetPathlib();
	PathLib::CVisualizer* vizualizer = pathlibWorld ? pathlibWorld->GetVisualizer() : nullptr;
	if ( vizualizer )
	{
		vizualizer->DebugNavgraph( 3 );
		for( Uint32 i = 0; i < 4; ++i )
		{
			m_showNavgraph[ i ]->Toggle( i == 3 );
		}
	}
}

void CEdPathlibPanel::OnViewNavmesh( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavMesh, event.IsChecked() );
}
void CEdPathlibPanel::OnViewNavmeshTriangles( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavMeshTriangles, event.IsChecked() );
}
void CEdPathlibPanel::OnViewNavmeshOverlay( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavMeshOverlay, event.IsChecked() );
}
void CEdPathlibPanel::OnViewNavgraph( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavGraph, event.IsChecked() );
}
void CEdPathlibPanel::OnViewNavgraphNoOcclusion( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavGraphNoOcclusion, event.IsChecked() );
}
void CEdPathlibPanel::OnViewNavgraphRegions( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavGraphRegions, event.IsChecked() );
}
void CEdPathlibPanel::OnViewTerrain( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavTerrain, event.IsChecked() );
}
void CEdPathlibPanel::OnViewObstacles( wxCommandEvent& event )
{
	SetRenderingFlag( SHOW_NavObstacles, event.IsChecked() );
}
void CEdPathlibPanel::SetRenderingFlag( EShowFlags flag, Bool b ) const
{
	wxTheFrame->GetFilterPanel()->SetViewportFlag( GGame->IsActive() ? VFT_GAME : VFT_EDITOR, flag, b );
}

void CEdPathlibPanel::OnBrushWalkable( wxCommandEvent& event )
{
	if( event.IsChecked() )
	{
		m_owner->SetBrushActive( true );
		m_owner->SetBrushDrawBlockers( false );
	}
	else
	{
		m_owner->SetBrushActive( false );
	}
}

void CEdPathlibPanel::OnBrushNonWalkable( wxCommandEvent& event )
{
	if( event.IsChecked() )
	{
		m_owner->SetBrushActive( true );
		m_owner->SetBrushDrawBlockers( true );
	}
	else
	{
		m_owner->SetBrushActive( false );
	}
}

void CEdPathlibPanel::OnPathDebuggerToggle( wxCommandEvent& event )
{
	m_owner->SetPathDebuggerMode( event.IsChecked() );
}

void CEdPathlibPanel::OnRecalculatorToggle( wxCommandEvent& event )
{
	m_owner->SetRecalculatorActive( event.IsChecked() );
}

void CEdPathlibPanel::OnNavmeshEditorToggle( wxCommandEvent& event )
{
	m_owner->SetNavmeshEditorActive( event.IsChecked() );
}
void CEdPathlibPanel::OnNavmeshDeleterToggle( wxCommandEvent& event )
{
	m_owner->SetNavmeshEditorActive( event.IsChecked() );
	m_owner->SetNavmeshDeleterActive( event.IsChecked() );
}

////////////////////////////////////////////////////////////////////////////
// CEdNavmeshEditor
////////////////////////////////////////////////////////////////////////////
CEdNavmeshEditor::CEdNavmeshEditor( EngineTime updateFrequency )
	: m_isNavmeshEditorActive( false )
	, m_isNavmeshEdgeSelected( false )
	, m_isNavmeshEdgePhantom( false )
	, m_nextEdgeSelectionTest()
	, m_selectedNavmeshTriangle( PathLib::CNavmesh::INVALID_INDEX )
	, m_selectedNavmeshTriangleEdge( 0 )
	, m_updateFrequency( updateFrequency )
{}

CNavmeshComponent* CEdNavmeshEditor::GetEditedNavmeshComponent()
{
	return NULL;
}
Bool CEdNavmeshEditor::HandleViewportMouseMove( const CMousePacket& packet )
{
	if ( m_isNavmeshEditorActive )
	{
		ProcessNavmeshSelection( packet );
		return true;
	}
	return false;
}
Bool CEdNavmeshEditor::HandleViewportTrack( const CMousePacket& packet )
{
	if ( m_isNavmeshEditorActive )
	{
		ProcessNavmeshSelection( packet );
		return true;
	}
	return false;
}
Bool CEdNavmeshEditor::HandleViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_isNavmeshEdgeSelected )
	{
		RenderNavmeshEdgeSelection( view, frame );
	}
	return true;
};

void CEdNavmeshEditor::ProcessNavmeshSelection( const CMousePacket& packet )
{
	if ( GEngine->GetRawEngineTime() < m_nextEdgeSelectionTest )
	{
		return;
	}
	m_nextEdgeSelectionTest = GEngine->GetRawEngineTime() + m_updateFrequency;
	m_isNavmeshEdgeSelected = false;
	m_selectedNavmeshTriangle = PathLib::CNavmesh::INVALID_INDEX;

	PathLib::CNavmesh* navmesh = FindNavmeshForEdition( packet );
	if ( !navmesh )
		return;

	Vector3 hitPos;
	// TODO: ray world to local
	PathLib::CNavmesh::TriangleIndex tri = navmesh->Debug_GetTriangleIntersectingRay( packet.m_rayOrigin.AsVector3() + Vector3( 0.f, 0.f, -0.1f ), packet.m_rayDirection.AsVector3(), hitPos );
	if ( tri != PathLib::CNavmesh::INVALID_INDEX )
	{
		m_selectedNavmeshTriangle = tri;
		Int32 bestNeighbour = -1;
		Float closestNeighbourSq = FLT_MAX;
		PathLib::CNavmesh::TriangleIndex neighbours[ 3 ];
		navmesh->GetTriangleNeighbours( tri, neighbours );
		for ( Uint32 i = 0; i < 3; ++i )
		{
			if ( neighbours[ i ] == PathLib::CNavmesh::INVALID_INDEX || PathLib::CNavmesh::IsPhantomEdge( neighbours[ i ] ) )
			{
				Vector3 edgeVerts[ 2 ];
				navmesh->GetTriangleEdge( tri, i, edgeVerts );

				Vector2 closestPoint;
				MathUtils::GeometryUtils::TestClosestPointOnLine2D( hitPos.AsVector2(), edgeVerts[ 0 ].AsVector2(), edgeVerts[ 1 ].AsVector2(), closestPoint );
				Float distSq = (hitPos.AsVector2() - closestPoint).SquareMag();
				if ( distSq < closestNeighbourSq )
				{
					closestNeighbourSq = distSq;
					bestNeighbour = i;
				}
			}
		}
		if ( bestNeighbour >= 0 )
		{
			m_isNavmeshEdgeSelected = true;
			m_selectedNavmeshTriangleEdge = bestNeighbour;
			m_isNavmeshEdgePhantom = PathLib::CNavmesh::IsPhantomEdge( neighbours[ bestNeighbour ] );
		}
	}
}
void CEdNavmeshEditor::ClearNavmeshSelection()
{
	m_isNavmeshEdgeSelected = false;
	m_selectedNavmeshTriangle = PathLib::CNavmesh::INVALID_INDEX;
	m_selectedNavmeshTriangleEdge = 0;
	m_isNavmeshEdgePhantom = false;
}
Bool CEdNavmeshEditor::GetEditedInstanceGuid( CGUID& guid )
{
	return false;
}
void CEdNavmeshEditor::RenderNavmeshEdgeSelection( IViewport* view, CRenderFrame* frame )
{
	if ( !m_isNavmeshEdgeSelected )
	{
		return;
	}
	PathLib::CNavmesh* navmesh = GetEditedNavmesh();
	if ( !navmesh )
		return;

	Vector3 verts[ 2 ];
	navmesh->GetTriangleEdge( m_selectedNavmeshTriangle, m_selectedNavmeshTriangleEdge, verts );
	frame->AddDebugFatLine( verts[ 0 ], verts[ 1 ], m_isNavmeshEdgePhantom ? Color( 255, 255, 0 ) : Color( 0, 255, 0 ), 0.15f, true );
}

////////////////////////////////////////////////////////////////////////////
// CEdPathlib
////////////////////////////////////////////////////////////////////////////
CEdPathlib::CEdPathlib()
	: CEdNavmeshEditor( 0.3333f )
	, m_panel( NULL )
	, m_isBrushActive( false )
	, m_isBrushDrawing( false )
	, m_brushDrawBlockers( true )
	, m_isRecalculatorActive( false )
	, m_isPathDebuggerActive( false )
	, m_isPathDebuggerStartingPointSet( false )
	, m_selectedNavmeshArea( PathLib::INVALID_AREA_ID )
	, m_recalculatorAreaSelected( PathLib::INVALID_AREA_ID )
	, m_brushRadius( 15 )
	, m_cursorPosition( 0, 0, 0, 0 )
{
}
CEdPathlib::~CEdPathlib()
{
	if ( m_searchData )
	{
		m_searchData->Release();
		m_searchData = nullptr;
	}
}

String CEdPathlib::GetCaption() const
{
	return TXT("PathLib Tool");
}

Bool CEdPathlib::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Load layout from XRC
	m_world = world;
	m_isBrushActive = false;
	m_isBrushDrawing = false;

	m_panel = new CEdPathlibPanel( viewport, this, panel, world );

	panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
	panel->Layout();


	return true;
}
void CEdPathlib::End()
{

}

Bool CEdPathlib::OnViewportTick( IViewport* view, Float timeDelta )
{
	// >>>> REMOVE ME ----
	if ( m_panel )
	{
		m_panel->Update();
	}
	// <<<< REMOVE ME ----

	return false;
}
Bool CEdPathlib::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( m_isBrushActive )
	{
		if (button == 0)
		{
			if (state == true) // LMB pressed
			{
				view->SetMouseMode( MM_Clip );
				m_isBrushDrawing = true;
			}
			else
			{
				// Painting done
				m_isBrushDrawing = false;

				// Uncapture mouse
				view->SetMouseMode( MM_Normal );
			}

			return true;
		}
	}
	if ( m_isPathDebuggerActive && button == 0 && state == true )
	{
		if ( !m_isPathDebuggerStartingPointSet )
		{
			m_isPathDebuggerStartingPointSet = true;
			m_pathDebuggerStartingPoint = m_cursorPosition;
		}
		else
		{
			m_isPathDebuggerStartingPointSet = false;
			CWorld* world = m_world.Get();
			if ( world )
			{
				CPathLibWorld* pathlibWorld = world->GetPathLibWorld();
				if ( pathlibWorld )
				{
					if ( !m_searchData )
					{
						m_searchData = new PathLib::CSearchData();
						m_searchData->AddRef();
					}
					//if ( pathlibWorld->TestLine( m_pathDebuggerStartingPoint.AsVector3(), m_cursorPosition.AsVector3(), 0.5f, PathLib::CT_DEFAULT ) )
					//{
					//	PATHLIB_LOG( TXT("Line test is ok!\n") );
					//}
					//else
					//{
					//	PATHLIB_LOG( TXT("Line test failed!\n") );
					//}
					m_searchData->Setup(
						pathlibWorld->GetVisualizer()->GetDebuggedNavgraph(),																			// category
						pathlibWorld->GetGlobalSettings().GetCategoryPersonalSpace( pathlibWorld->GetVisualizer()->GetDebuggedNavgraph() ) );			// personal space
					PathLib::EPathfindResult result = m_searchData->PlotPath( pathlibWorld, m_pathDebuggerStartingPoint.AsVector3(), m_cursorPosition.AsVector3() );
					switch ( result )
					{
					case PathLib::PATHRESULT_FAILED_OUTOFNAVDATA:
					case PathLib::PATHRESULT_FAILED:
						PATHLIB_LOG( TXT("Couldn't find path!\n") );
						break;
					case PathLib::PATHRESULT_SUCCESS:
						PATHLIB_LOG( TXT("Successfully found path (%d nodes)!\n"), m_searchData->GetOutputWaypointsCount() );
						break;
					default:
					case PathLib::PATHRESULT_PENDING:
						PATHLIB_LOG( TXT("Pending result...\n") );
						break;
					}
				}
			}
		}
		return true;
	}
	if ( m_isNavmeshEditorActive )
	{
		if ( m_isNavmeshDeleterActive )
		{
			if ( m_selectedNavmeshArea != PathLib::INVALID_AREA_ID && button == 0 && state == true && m_selectedNavmeshTriangle != PathLib::CNavmesh::INVALID_INDEX )
			{
				CWorld* world = m_world.Get();
				if ( world )
				{
					CPathLibWorld* pathlibWorld = world->GetPathLibWorld();
					if ( pathlibWorld )
					{
						//if ( YesNo( TXT("Really delete instance_%04x?\n"), m_selectedNavmeshArea ) )
						{
							CNavmeshComponent* naviComponent = GetEditedNavmeshComponent();
							CGUID guid;
							if ( GetEditedInstanceGuid( guid ) && naviComponent && naviComponent->GetGUID4PathLib() == guid )
							{
								naviComponent->ClearNavmesh();
							}
							
							pathlibWorld->NotifyOfInstanceRemoved( m_selectedNavmeshArea );
							ClearNavmeshSelection();
							m_selectedNavmeshArea = PathLib::INVALID_AREA_ID;
						}
					}
				}
			}
		}
	}
	if ( m_isRecalculatorActive )
	{
		// left mouse button pressed
		if ( button == 0 && state == true )
		{
			Bool done = false;
			if ( m_recalculatorConfirmRecalculation )
			{
				if ( GEngine->GetRawEngineTime() <= m_recalculatorClickTime + RECALCULATOR_DELAY_FOR_DOUBLECLICK )
				{
					DoRecalculate();
					
					done = true;
					m_recalculatorConfirmRecalculation = false;
				}
			}
			if ( !done )
			{
				CWorld* world = m_world.Get();
				CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
				if ( pathlib )
				{
					PathLib::CAreaDescription* area = pathlib->GetAreaDescription( m_recalculatorAreaSelected );
					if ( area && !area->IsProcessing() )
					{
						m_recalculatorConfirmRecalculation = true;
						Red::System::DateTime time;
						Red::System::Clock::GetInstance().GetLocalTime( time );
						m_recalculatorClickTime = GEngine->GetRawEngineTime();
					}
				}
			}
		}
	}

	m_isBrushDrawing = false;
	return false;
}

void CEdPathlib::UpdateCursor( const CMousePacket& packet )
{
	CWorld* world = m_world.Get();
	if ( !world || !world->GetTerrain() )
	{
		return;
	}

	Vector position;
	if ( !world->GetTerrain()->Intersect( packet.m_rayOrigin, packet.m_rayDirection, position ) )
	{
		return;
	}
	m_cursorPosition = position;
}
PathLib::CNavmesh* CEdPathlib::GetEditedNavmesh()
{
	if ( m_selectedNavmeshArea == PathLib::INVALID_AREA_ID )
	{
		return NULL;
	}

	CWorld* world = m_world.Get();
	if ( !world )
	{
		return NULL;
	}
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( !pathlib )
	{
		return NULL;
	}
	PathLib::CNavmeshAreaDescription* area = pathlib->GetInstanceAreaDescription( m_selectedNavmeshArea );
	if ( !area )
	{
		return NULL;
	}
	return area->GetNavmesh();
}
PathLib::CNavmesh* CEdPathlib::FindNavmeshForEdition( const CMousePacket& packet )
{
	CWorld* world = m_world.Get();
	if ( !world )
	{
		return NULL;
	}
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( !pathlib )
	{
		return NULL;
	}

	PathLib::CVisualizer* visualizer = pathlib->GetVisualizer();
	const auto& idList = visualizer->GetNavmeshesBeingRendered();

	for ( auto it = idList.Begin(), end = idList.End(); it != end; ++it )
	{
		PathLib::CNavmeshAreaDescription* naviArea = pathlib->GetInstanceAreaDescription( *it );
		if ( !naviArea )
		{
			continue;
		}

		PathLib::CNavmesh* navmesh = naviArea->GetNavmesh();
		if ( !navmesh )
		{
			continue;
		}

		Vector3 hitPos;
		PathLib::CNavmesh::TriangleIndex tri = navmesh->Debug_GetTriangleIntersectingRay( packet.m_rayOrigin.AsVector3() + Vector3( 0.f, 0.f, -0.1f ), packet.m_rayDirection.AsVector3(), hitPos );
		if ( tri == PathLib::CNavmesh::INVALID_INDEX )
		{
			continue;
		}

		m_selectedNavmeshArea = naviArea->GetId();
		return navmesh;
	}

	return NULL;
}
CWorld* CEdPathlib::GetEditedWorld()
{
	return m_world.Get();
}
Bool CEdPathlib::GetEditedInstanceGuid( CGUID& guid )
{
	if ( m_selectedNavmeshArea == PathLib::INVALID_AREA_ID )
	{
		return false;
	}

	CWorld* world = m_world.Get();
	if ( !world )
	{
		return false;
	}
	CPathLibWorld* pathlib = world->GetPathLibWorld();
	if ( !pathlib )
	{
		return false;
	}
	PathLib::CNavmeshAreaDescription* area = pathlib->GetInstanceAreaDescription( m_selectedNavmeshArea );
	if ( !area )
	{
		return false;
	}
	guid = area->GetOwnerGUID();
	return true;
}
CNavmeshComponent* CEdPathlib::GetEditedNavmeshComponent()
{
	CGUID guid;
	if ( GetEditedInstanceGuid( guid ) )
	{
		CWorld* world = m_world.Get();
		if ( !world )
		{
			return NULL;
		}
		for ( WorldAttachedComponentsIterator it( world ); it; ++it )
		{
			CNavmeshComponent* naviComponent = ::Cast< CNavmeshComponent >( (*it) );
			if ( naviComponent && naviComponent->GetGUID4PathLib() == guid )
			{
				return naviComponent;
			}
		}
	}
	return NULL;
}
Bool CEdPathlib::OnViewportMouseMove( const CMousePacket& packet )
{
	UpdateCursor( packet );

	if ( m_isBrushDrawing )
	{
		ProcessBrush();
	}
	if ( m_isRecalculatorActive )
	{
		ProcessRecalculator( packet.m_rayOrigin, packet.m_rayDirection );
	}

	if ( CEdNavmeshEditor::HandleViewportMouseMove( packet ) )
	{
		return true;
	}

	return true;
}
Bool CEdPathlib::OnViewportTrack( const CMousePacket& packet )
{
	UpdateCursor( packet );

	if ( m_isBrushDrawing )
	{
		ProcessBrush();
	}

	if ( m_isRecalculatorActive )
	{
		ProcessRecalculator( packet.m_rayOrigin, packet.m_rayDirection );
	}

	CEdNavmeshEditor::HandleViewportTrack( packet );

	return true;
}
void CEdPathlib::ProcessBrush()
{
}

void CEdPathlib::ProcessRecalculator( const Vector& cameraPosition, const Vector& dir )
{
	CWorld* world = m_world.Get();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( !pathlib || !pathlib->GetVisualizer() )
	{
		return;
	}

	PathLib::AreaId prevArea = m_recalculatorAreaSelected;
	m_recalculatorAreaSelected = PathLib::INVALID_AREA_ID;

	Float closestNaviAreaDistSq = FLT_MAX;

	const auto& idList = pathlib->GetVisualizer()->GetNavmeshesBeingRendered();
	for( auto it = idList.Begin(), end = idList.End(); it != end; ++it )
	{
		PathLib::AreaId areaId = *it;
		PathLib::CNavmeshAreaDescription* naviArea = pathlib->GetInstanceAreaDescription( areaId );
		if ( naviArea )
		{
			PathLib::CNavmesh* navmesh = naviArea->GetNavmesh();
			if ( navmesh )
			{
				Vector3 hitPos;
				PathLib::CNavmesh::TriangleIndex tri = navmesh->Debug_GetTriangleIntersectingRay( cameraPosition.AsVector3() + Vector3( 0.f, 0.f, -0.1f ), dir.AsVector3(), hitPos );
				if ( tri != PathLib::CNavmesh::INVALID_INDEX )
				{
					Float distSq = ( hitPos - cameraPosition.AsVector3() ).SquareMag();
					if ( distSq < closestNaviAreaDistSq )
					{
						closestNaviAreaDistSq = distSq;
						m_recalculatorAreaSelected = areaId;
					}
				}
			}
		}
	}

	if ( m_recalculatorAreaSelected == PathLib::INVALID_AREA_ID )
	{
		CClipMap* terrain = world->GetTerrain();
		if ( terrain )
		{
			Vector position;
			if ( world->GetTerrain()->Intersect( cameraPosition, dir, position ) )
			{
				PathLib::AreaId areaId = pathlib->GetTerrainInfo().GetTileIdAtPosition( position.AsVector2() );
				if ( areaId != PathLib::INVALID_AREA_ID )
				{
					PathLib::CTerrainAreaDescription* terrainArea = pathlib->GetTerrainAreaDescription( areaId );
					if ( terrainArea )
					{
						m_recalculatorAreaSelected = terrainArea->GetId();
					}
				}
			}
		}
	}
	if ( prevArea != m_recalculatorAreaSelected )
	{
		m_recalculatorConfirmRecalculation = false;
	}
}

void CEdPathlib::DoRecalculate()
{
	CWorld* world = m_world.Get();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( !pathlib )
	{
		return;
	}
	
	PathLib::CAreaDescription* area = pathlib->GetAreaDescription( m_recalculatorAreaSelected );
	if ( area )
	{
		area->MarkDirty( PathLib::CAreaDescription::DIRTY_ALL, 0.25f );
	}
}

Bool CEdPathlib::OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
{
	if ( m_isBrushActive )
	{
		RenderBrush( view, frame );
	}
	if ( m_isPathDebuggerActive )
	{
		RenderPathDebugger( view, frame );
	}

	if ( m_isNavmeshEdgeSelected )
	{
		RenderNavmeshEdgeSelection( view, frame );
	}

	if ( m_isRecalculatorActive )
	{
		RenderRecalculator( view, frame );
	}

	return true;
}
void CEdPathlib::RenderPathDebugger( IViewport* view, CRenderFrame* frame )
{
	if ( m_isPathDebuggerStartingPointSet )
	{
		frame->AddDebugLineWithArrow( m_pathDebuggerStartingPoint + Vector( 0,0,10,0 ), m_pathDebuggerStartingPoint, 1.f, 0.2f, 0.2f, Color::LIGHT_RED, true );
	}
	Uint32 waypoints = m_searchData ? m_searchData->GetOutputWaypointsCount() : 0;
	if ( waypoints > 1 )
	{
		TDynArray< DebugVertex > vertexes((waypoints-1)*2);
		for ( Uint32 i = 0; i < waypoints-1; ++i )
		{
			vertexes[ i * 2 + 0 ].Set( m_searchData->GetOutputWaypoint( i ), (i & 1) ? Color::BLACK : Color::BLUE );
			vertexes[ i * 2 + 1 ].Set( m_searchData->GetOutputWaypoint( i+1 ), (i & 1) ? Color::BLUE : Color::BLACK  );
		}
		frame->AddDebugLines( vertexes.TypedData(), vertexes.Size(), true );
	}
}
void CEdPathlib::RenderBrush( IViewport* view, CRenderFrame* frame )
{
	// CTRL+C CTRL+V from CEdTerrainEditTool::RenderBrush
	CWorld* world = m_world.Get();
	CClipMap* terrain = world ? world->GetTerrain() : NULL;
	if ( !terrain )
	{
		return;
	}

	// Circle subdivision based on brush radius
	const Uint32 numPoints = Min( Max( Int32( 2.0 * M_PI * m_brushRadius ), 32 ), 256 );

	TStaticArray< Vector, 256 > ringPoints;
	for ( Uint32 i = 0; i < numPoints; i ++ )
	{
		const Float localAngle = 2.0f * M_PI * ( i / (Float)numPoints );

		// Calc vertex world space xy position
		Vector pos 				
			( MCos( localAngle ) * m_brushRadius + m_cursorPosition.X
			, MSin( localAngle ) * m_brushRadius + m_cursorPosition.Y
			, 0 );


		terrain->GetHeightForWorldPosition( pos, pos.Z );
		ringPoints.PushBack( pos );
	}

	Color color = m_brushDrawBlockers ? Color::RED : Color::BLUE;
	for ( Uint32 i = 0; i < ringPoints.Size(); ++i )
	{
		frame->AddDebugLine( ringPoints[i], ringPoints[ ( i + 1 ) % ringPoints.Size()], color, true );
	}
}

void CEdPathlib::RenderAreaFrame( PathLib::CAreaDescription* area, Color boxColor, CRenderFrame* frame )
{
	static TStaticArray< DebugVertex, 8 > vertexes( 8 );
	static struct IndexArray : public TStaticArray< Uint16, 72 >
	{
		IndexArray()
		{
			Resize( 72 );
			Uint32 i = 0;
			for ( Uint32 wall = 0; wall < 4; ++wall )
			{
				Uint32 wallNext = (wall+1) % 4;

				(*this)[ i++ ] = wall;
				(*this)[ i++ ] = wallNext;
				(*this)[ i++ ] = wall+4;

				(*this)[ i++ ] = wallNext;
				(*this)[ i++ ] = wallNext+4;
				(*this)[ i++ ] = wall+4;

				(*this)[ i++ ] = wall+4;
				(*this)[ i++ ] = wallNext;
				(*this)[ i++ ] = wall;

				(*this)[ i++ ] = wall+4;
				(*this)[ i++ ] = wallNext+4;
				(*this)[ i++ ] = wallNext;
			}
			for ( Uint32 base = 0; base < 8; base += 4 )
			{
				(*this)[ i++ ] = base;
				(*this)[ i++ ] = base+1;
				(*this)[ i++ ] = base+2;

				(*this)[ i++ ] = base+2;
				(*this)[ i++ ] = base+3;
				(*this)[ i++ ] = base;

				(*this)[ i++ ] = base+2;
				(*this)[ i++ ] = base+1;
				(*this)[ i++ ] = base;

				(*this)[ i++ ] = base;
				(*this)[ i++ ] = base+3;
				(*this)[ i++ ] = base+2;
			}
			ASSERT( i == 72 );
		}
	} indexArray;


	// draw selected area bounding box
	Box bbox = area->GetBBox();

	// custom code for terrain areas
	if ( area->IsTerrainArea() )
	{
		CClipMap* clipMap = m_world.Get()->GetTerrain();
		bbox.Min.Z = clipMap->GetLowestElevation();
		bbox.Max.Z = clipMap->GetHeighestElevation();
	}

	vertexes[ 0 ].Set( Vector( bbox.Min.X, bbox.Min.Y, bbox.Min.Z ), boxColor );
	vertexes[ 1 ].Set( Vector( bbox.Max.X, bbox.Min.Y, bbox.Min.Z ), boxColor );
	vertexes[ 2 ].Set( Vector( bbox.Max.X, bbox.Max.Y, bbox.Min.Z ), boxColor );
	vertexes[ 3 ].Set( Vector( bbox.Min.X, bbox.Max.Y, bbox.Min.Z ), boxColor );
	vertexes[ 4 ].Set( Vector( bbox.Min.X, bbox.Min.Y, bbox.Max.Z ), boxColor );
	vertexes[ 5 ].Set( Vector( bbox.Max.X, bbox.Min.Y, bbox.Max.Z ), boxColor );
	vertexes[ 6 ].Set( Vector( bbox.Max.X, bbox.Max.Y, bbox.Max.Z ), boxColor );
	vertexes[ 7 ].Set( Vector( bbox.Min.X, bbox.Max.Y, bbox.Max.Z ), boxColor );


	frame->AddDebugTriangles( Matrix::IDENTITY, vertexes.TypedData(), vertexes.Size(), indexArray.TypedData(), indexArray.Size(), boxColor, false );
}

void CEdPathlib::RenderRecalculator( IViewport* view, CRenderFrame* frame )
{
	CWorld* world = m_world.Get();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( !pathlib )
	{
		return;
	}


	Int32 x = view->GetWidth() / 2 - 50;
	Int32 y = view->GetHeight() / 2 - 5;
	static const String PLZ_SELECT( TXT("Recalculator active please select something.") );
	String textToShow = PLZ_SELECT;
	Color textColor = Color::RED;

	PathLib::CAreaDescription* area = pathlib->GetAreaDescription( m_recalculatorAreaSelected );
	if ( area )
	{
		String areaDesc;
		if ( area->IsTerrainArea() )
		{
			Int32 tileX, tileY;
			pathlib->GetTerrainInfo().GetTileCoordsFromId( area->GetId(), tileX, tileY );
			areaDesc = String::Printf( TXT("terrain area %d, %d" ), tileX, tileY );
		}
		else
		{
			areaDesc = String::Printf( TXT("navmesh area %04x" ), area->GetId() );
		}

		Color lineColor = Color::WHITE;
		Color boxColor = Color::BLUE;

		// custom code for terrain areas
		if ( area->IsTerrainArea() )
		{
			boxColor = Color::GREEN;
		}

		if( area->IsDirty() )
		{
			boxColor = Color::LIGHT_RED;
		}

		Bool textSet = false;

		if ( m_recalculatorConfirmRecalculation )
		{
			Red::System::DateTime time;
			Red::System::Clock::GetInstance().GetLocalTime( time );
			if ( m_recalculatorClickTime + RECALCULATOR_DELAY_FOR_DOUBLECLICK  > GEngine->GetRawEngineTime() )
			{
				lineColor = Color::RED;
				boxColor = Color::LIGHT_MAGENTA;
				textToShow = String::Printf( TXT("Confirm %s"), areaDesc.AsChar() );
				textColor = Color::YELLOW;
				textSet = true;
			}
		}
		else if ( area->IsProcessing() )
		{
			boxColor = Color::WHITE;
			textSet = true;
			textToShow = String::Printf( TXT("Processing %s"), areaDesc.AsChar() );
		}
		
		if ( !textSet )
		{
			textColor = Color::WHITE;
			textToShow = String::Printf( TXT("Selected %s"), areaDesc.AsChar() );
		}
		boxColor.A = 100;
		
		RenderAreaFrame( area, boxColor, frame );
	}

	
	frame->AddDebugScreenText( x, y, textToShow, textColor );

}

void CEdPathlib::RenderGenerationScheduler( IViewport* view, CRenderFrame* frame )
{
	CWorld* world = m_world.Get();
	CPathLibWorld* pathlib = world ? world->GetPathLibWorld() : NULL;
	if ( !pathlib )
	{
		return;
	}

	Bool selectedAreaIsScheduled = false;
	for ( Uint32 i = 0, n = m_generationScheduledAreas.Size(); i < n; ++i )
	{
		if ( m_generationScheduledAreas[ i ] == m_recalculatorAreaSelected )
		{
			selectedAreaIsScheduled = true;
			continue;
		}
		PathLib::CAreaDescription* area = pathlib->GetAreaDescription( m_generationScheduledAreas[ i ] );
		if ( area )
		{
			RenderAreaFrame( area, Color( 180, 0, 40, 50 ), frame );
		}
	}

	if ( m_recalculatorAreaSelected != PathLib::INVALID_AREA_ID )
	{
		PathLib::CAreaDescription* area = pathlib->GetAreaDescription( m_recalculatorAreaSelected );
		if ( area )
		{
			Color boxColor = selectedAreaIsScheduled ? Color( 255, 255, 0 ) : Color( 0 ,0, 255 );
			boxColor.A = m_recalculatorConfirmRecalculation ? 200 : 30;
			RenderAreaFrame( area, boxColor, frame );
		}
	}

}

