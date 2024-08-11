#include "build.h"
#include "inputBoxDlg.h"
#include "mergedGeometryTool.h"
#include "../../common/engine/mergedWorldGeometryEntity.h"

//-------------------------------------------------------

BEGIN_EVENT_TABLE( CEdMergedGeometryToolPanel, wxPanel )
END_EVENT_TABLE();

CEdMergedGeometryToolPanel::CEdMergedGeometryToolPanel( wxWindow* parent, CWorld* world )
	: wxPanel( parent, wxID_ANY )
	, m_world( world )
	, m_currentPlaceInfoValid( false )
	, m_totalSize( 0 )
	, m_totalTriangles( 0 )
	, m_totalVertices( 0 )
{
	wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
	SetSizer( sizer );

	{
		wxButton* rebuild = new wxButton( this, 100, wxT("Rebuild geometry"), wxPoint(10,10), wxSize(200, 21) );
		Connect( 100, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMergedGeometryToolPanel::OnRebuildAllStaticGeometry ), NULL, this );
		sizer->Add( rebuild, 0, wxALL | wxEXPAND, 4 );
	}

	{
		wxButton* rebuild = new wxButton( this, 101, wxT("Rebuild near camera"), wxPoint(10,10), wxSize(200, 21) );
		Connect( 101, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMergedGeometryToolPanel::OnRebuildNearStaticGeometry ), NULL, this );
		sizer->Add( rebuild, 0, wxALL | wxEXPAND, 4 );
	}

	/*{
		wxButton* checkin = new wxButton( this, 102, wxT("Checkin geometry"), wxPoint(10,10), wxSize(200, 21) );
		Connect( 102, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMergedGeometryToolPanel::OnCheckinChanges ), NULL, this );
		sizer->Add( checkin, 0, wxALL | wxEXPAND, 4 );
	}*/

	{
		m_stats = new wxStaticText( this, wxID_ANY, wxT("Stats..."), wxPoint(10,10), wxSize(100,800) );
		sizer->Add( m_stats, 0, wxALL | wxEXPAND, 4 );
	}

	Layout();
	Show();

	RefreshCells();
}

CEdMergedGeometryToolPanel::~CEdMergedGeometryToolPanel()
{
}

void CEdMergedGeometryToolPanel::OnRebuildAllStaticGeometry( wxCommandEvent& event )
{
	GFeedback->BeginTask( TXT("Merging geometry..."), true );
	const Bool ret = m_world->RebuildMergedGeometry( Vector::ZEROS, 32000.0f );
	GFeedback->EndTask();

	if ( GFeedback->IsTaskCanceled() )
	{
		wxMessageBox( wxT("The task was canceled, data was not fully processed"), wxT("Error"), wxICON_ERROR | wxOK, this );
	}

	if ( !ret )
	{
		wxMessageBox( wxT("There were errors processing world's geometry"), wxT("Error"), wxICON_ERROR | wxOK, this );
	}

	RefreshCells();
}

void CEdMergedGeometryToolPanel::OnRebuildNearStaticGeometry( wxCommandEvent& event )
{
	CEdInputBox inputBox( this, wxT("Global geometry tool"), wxT("Specify the distance around the camera that should be recomputed"), wxT("100"), false );
	if ( inputBox.ShowModal() )
	{
		const Vector cameraPos = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();

		Float distance = 100.0f;
		if ( !FromString< Float >( inputBox.GetEditText(), distance ) )
		{
			wxMessageBox( wxT("Invalid distance specified"), wxT("Error"), wxOK | wxICON_ERROR, this );
		}
		else
		{
			GFeedback->BeginTask( TXT("Merging nearby geometry..."), true );
			const Bool ret = m_world->RebuildMergedGeometry( cameraPos, 100.0f );
			GFeedback->EndTask();

			if ( GFeedback->IsTaskCanceled() )
			{
				wxMessageBox( wxT("The task was canceled, data was not fully processed"), wxT("Error"), wxICON_ERROR | wxOK, this );
			}

			if ( !ret )
			{
				wxMessageBox( wxT("There were errors processing world's geometry"), wxT("Error"), wxICON_ERROR | wxOK, this );
			}

			RefreshCells();
		}
	}
}

void CEdMergedGeometryToolPanel::OnCheckinChanges( wxCommandEvent& event )
{
}

void CEdMergedGeometryToolPanel::RefreshStatsForCurrentCamera( const Vector& cameraPosition, const Bool force /*= false*/ )
{
	const Float distFromLastPlace = m_currentPlaceInfoRefPos.DistanceTo( cameraPosition );
	if ( distFromLastPlace < 5.0f && !force )
		return;

	m_currentPlaceInfoRefPos = cameraPosition;
	m_currentPlaceInfoValid = true;
	m_currentPlaceInfo = GridStats();

	ComputeStatsForPlace( cameraPosition, m_currentPlaceInfo );
	RefreshStats();
}

void CEdMergedGeometryToolPanel::ComputeStatsForPlace( const Vector& position, GridStats& outResults ) const
{
	for ( const auto& info : m_gridCells )
	{
		const Float distSq = info.m_pos.DistanceSquaredTo( position );
		if ( distSq <= info.m_streamingDistanceSq )
		{
			outResults.m_numCells += 1;
			outResults.m_dataSize += info.m_dataSize;
			outResults.m_numTriangles += info.m_numTriangles;
			outResults.m_numVertices += info.m_numVertices;
		}
	}
}

void CEdMergedGeometryToolPanel::RefreshCells()
{
	m_totalTriangles = 0;
	m_totalSize = 0;
	m_totalVertices = 0;
	
	m_gridCells.ClearFast();
	m_gridSize = 0;

	// capture data from world
	if ( m_world && m_world->GetMergedGeometryContainer() )
	{
		THandle< CLayer > contentLayer = m_world->GetMergedGeometryContainer()->GetContentLayer( m_world );
		if ( contentLayer )
		{
			TDynArray< CEntity* > allEntities;
			contentLayer->GetEntities( allEntities );

			m_gridCells.Reserve( allEntities.Size() );

			for ( const CEntity* ent : allEntities )
			{
				THandle< CMergedWorldGeometryEntity > gridEntity = Cast< CMergedWorldGeometryEntity >( ent );
				if ( gridEntity )
				{
					// accumulate total statistics
					m_totalSize += gridEntity->GetPayloadDataSize();
					m_totalTriangles += gridEntity->GetPayloadTriangleCount();
					m_totalVertices += gridEntity->GetPayloadVertexCount();

					// gather merged cell info
					GridCell cellInfo;
					cellInfo.m_x = gridEntity->GetGridCoordinates().m_x;
					cellInfo.m_y = gridEntity->GetGridCoordinates().m_y;
					cellInfo.m_dataSize = gridEntity->GetPayloadDataSize();
					cellInfo.m_numTriangles = gridEntity->GetPayloadTriangleCount();
					cellInfo.m_numVertices = gridEntity->GetPayloadVertexCount();
					cellInfo.m_pos = gridEntity->GetWorldPositionRef();
					cellInfo.m_streamingDistanceSq = gridEntity->GetStreamingDistance() * gridEntity->GetStreamingDistance();
					m_gridCells.PushBack( cellInfo );

					// keep track of the grid size
					m_gridSize = Max< Int32 >( m_gridSize, Max< Int32 >( cellInfo.m_x+1, cellInfo.m_y+1 ) );
				}
			}
		}
	}

	// compute grid cell map
	m_gridMap.Resize( m_gridSize*m_gridSize );
	Red::MemorySet( m_gridMap.Data(), -1, m_gridMap.DataSize() );
	for ( Uint32 i=0; i<m_gridCells.Size(); ++i )
	{
		const auto& cell = m_gridCells[i];
		const Int32 mapCoordinate = cell.m_x + cell.m_y*m_gridSize;
		m_gridMap[ mapCoordinate ] = i;
	}

	// refresh stats
	RefreshWorstPlace();

	// refresh current stats
	RefreshStatsForCurrentCamera( m_currentPlaceInfoRefPos, true );
}

void CEdMergedGeometryToolPanel::RefreshWorstPlace()
{

}

void CEdMergedGeometryToolPanel::RefreshStats()
{
	String txt;

	// general stats
	txt += String::Printf( TXT("Grid size: %d\n"), m_gridSize );
	txt += String::Printf( TXT("Grid cells: %d (%d filled)\n"), m_gridMap.Size(), m_gridCells.Size() );
	txt += TXT("\n");
	txt += String::Printf( TXT("Total size: %1.2f MB\n"), (Double)m_totalSize / (1024*1024) );
	txt += String::Printf( TXT("Total triangles: %1.2fM\n"), (Double)m_totalTriangles / (1024*1024) );
	txt += String::Printf( TXT("Total vertices: %1.2fM\n"), (Double)m_totalVertices / (1024*1024) );

	// current camera position stats
	if ( m_currentPlaceInfoValid )
	{
		txt += TXT("\n");
		txt += String::Printf( TXT("Current location cells: %d\n"), m_currentPlaceInfo.m_numCells );
		txt += String::Printf( TXT("Current location size: %1.2f MB\n"), (Double)m_currentPlaceInfo.m_dataSize / (1024*1024) );
		txt += String::Printf( TXT("Current location triangles: %1.2fM\n"), (Double)m_currentPlaceInfo.m_numTriangles / (1024*1024) );
		txt += String::Printf( TXT("Current location vertices: %1.2fM\n"), (Double)m_currentPlaceInfo.m_numVertices / (1024*1024) );
	}

	// compute worst case 
	m_stats->SetLabel( txt.AsChar() );
}

//-------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CEdMergedGeometryTool );

CEdMergedGeometryTool::CEdMergedGeometryTool()
	: m_panel( nullptr )
{
}

CEdMergedGeometryTool::~CEdMergedGeometryTool()
{
}

String CEdMergedGeometryTool::GetCaption() const
{
	return TXT("Merged Geometry");
}

Bool CEdMergedGeometryTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_panel = new CEdMergedGeometryToolPanel( panel, world );

	panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
	panel->Layout();
	return true;
}

void CEdMergedGeometryTool::End()
{
}

Bool CEdMergedGeometryTool::OnViewportCalculateCamera( IViewport* view, CRenderCamera &camera )
{
	if ( wxTheFrame->GetWorldEditPanel() )
	{
		const Vector mainCamera = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();
		m_panel->RefreshStatsForCurrentCamera( mainCamera );
	}

	return false;
}

//-------------------------------------------------------

