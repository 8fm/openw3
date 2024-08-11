/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneExporter.h"
#include "../../common/core/exporter.h"
#include "../../common/core/gatheredResource.h"


CGatheredResource resDefaultExportMaterial( TXT("environment\\shaders\\simple\\diff_spec.w2mg"), RGF_Startup );


IMPLEMENT_ENGINE_CLASS( CEdSceneExporterTool );

wxIMPLEMENT_CLASS( CEdSceneExporterToolPanel, CEdDraggablePanel );

BEGIN_EVENT_TABLE( CEdSceneExporterToolPanel, CEdDraggablePanel )
	EVT_BUTTON( XRCID("ExportScene"), CEdSceneExporterToolPanel::OnExportScene )
END_EVENT_TABLE()

CEdSceneExporterTool::CEdSceneExporterTool()
{
}

CEdSceneExporterTool::~CEdSceneExporterTool()
{
}

String CEdSceneExporterTool::GetCaption() const
{
	return TXT("Scene Exporter");
}

Bool CEdSceneExporterTool::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Get the scene name
	if ( !GGame->GetActiveWorld() )
	{
		wxMessageBox( wxT("No world to export"), wxT("Export scene"), wxOK | wxICON_WARNING );
		return false;
	}

	// Get the default name from the scene
	String fileName = TXT("WorldScene");
	CDiskFile* file = GGame->GetActiveWorld()->GetFile();
	if ( file )
	{
		CFilePath filePath( file->GetAbsolutePath() );
		fileName = filePath.GetFileName();
	}

	// Create tool panel
	m_panel = new CEdSceneExporterToolPanel( this, panel, fileName );

	// Create panel for custom window
	panelSizer->Add( m_panel, 1, wxEXPAND, 5 );
	panel->Layout();

	// Start tool
	return true;
}

void CEdSceneExporterTool::End()
{

}

CEdSceneExporterToolPanel::CEdSceneExporterToolPanel( CEdSceneExporterTool* tool, wxWindow* parent, const String& sceneFileName )
{
	// Load layout from XRC
	wxXmlResource::Get()->LoadPanel( this, parent, wxT("SceneExportTool") );

	// Enumerate exportable formats
	TDynArray< CFileFormat > exportFormats;
	IExporter::EnumExportFormats( ClassID< CWorld >(), exportFormats );

	// Configure saver
	m_exportFile.AddFormats( exportFormats );
	m_exportFile.SetIniTag( TXT("SceneExport") );
	m_exportFile.SetMultiselection( false );

	m_detachablePanel.Initialize( this, TXT( "Scene Exporter" ) );
}

CEdSceneExporterToolPanel::~CEdSceneExporterToolPanel()
{
}

void CEdSceneExporterToolPanel::OnExportScene( wxCommandEvent &event )
{
	// No world :)
	CWorld* worldToExport = GGame->GetActiveWorld();
	if ( !worldToExport )
	{
		wxMessageBox( wxT("No world to export. Load something."), wxT("Scene export"), wxOK | wxICON_ERROR );
		return;
	}

	// Read settings
	extern Bool GExportVolumeMeshes;
	GExportVolumeMeshes = XRCCTRL( *this, "ExportVolume", wxCheckBox )->GetValue();
	extern Bool GExportWorldSelectionOnly;
	GExportWorldSelectionOnly = XRCCTRL( *this, "SelectionOnly", wxCheckBox )->GetValue();
	extern Bool GExportWorldMeshes;
	GExportWorldMeshes = XRCCTRL( *this, "ExportMeshes", wxCheckBox )->GetValue();
	extern Bool GExportWorldTerrain;
	GExportWorldTerrain = XRCCTRL( *this, "ExportTerrain", wxRadioButton )->GetValue();
	extern Bool GExportWorldTerrainAllTiles;
	GExportWorldTerrainAllTiles = XRCCTRL( *this, "ExportTerrainAllTiles", wxRadioButton )->GetValue();
	extern Bool GExportWorldFoliage;
	GExportWorldFoliage = XRCCTRL( *this, "ExportFoliage", wxCheckBox )->GetValue();
	extern int GExportWorldTerrainMip;
	GExportWorldTerrainMip = XRCCTRL( *this, "ExportTerrainMip", wxSpinCtrl )->GetValue();
	extern Int32 GExportWorldTerrainTilesAmount;
	GExportWorldTerrainTilesAmount = XRCCTRL( *this, "ExportTerrainTilesAmount", wxSpinCtrl )->GetValue();
	extern Int32 GExportIgnoreFoliageSmallerThan;
	GExportIgnoreFoliageSmallerThan = XRCCTRL( *this, "ExportIgnoreFoliageSmallerThan", wxSpinCtrl )->GetValue();
	extern Float GExportWorldFoliageRadius;
	GExportWorldFoliageRadius = XRCCTRL( *this, "ExportFoliageDistance", wxSpinCtrl )->GetValue();

	// Terrain point around where the terrain will be exported
	extern Vector GExportWorldTerrainPoint;
	GExportWorldTerrainPoint = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();

	// Terrain box to clip geometry inside
	extern Box GExportWorldTerrainBox;
	Float minX, minY, maxX, maxY;
	if ( wxTheFrame->GetWorldEditPanel()->GetVisibleTerrainBounds( minX, minY, maxX, maxY ) )
	{
		GExportWorldTerrainBox = Box( Vector( minX, minY, FLT_MIN ), Vector( maxX, maxY, FLT_MAX ) );
	}
	else
	{
		GExportWorldTerrainBox = Box( Vector( 0, 0, FLT_MIN ), Vector( 0, 0, FLT_MAX ) );
	}

	// Read pivot
	extern Vector GExportWorldOffset;
	if ( XRCCTRL( *this, "AlignToPivot", wxCheckBox )->GetValue() )
	{
		// Get selection
		if ( !worldToExport->GetSelectionManager()->GetSelectionCount() )
		{
			wxMessageBox( wxT("Select pivot before exporting"), wxT("Scene export"), wxOK | wxICON_ERROR );
			return;
		}

		// Use pivot offset
		GExportWorldOffset = worldToExport->GetSelectionManager()->GetPivotPosition();
	}
	else
	{
		// Export in world space
		GExportWorldOffset = Vector::ZEROS;
	}

	// Ask for file name
	if ( !m_exportFile.DoSave( (HWND) GetHWND() ) )
	{
		wxMessageBox( wxT("No world exporters found"), wxT("Scene export"), wxOK | wxICON_ERROR );
		return;
	}

	// Get file path
	IExporter* exporter = IExporter::FindExporter( ClassID< CWorld >(), m_exportFile.GetFileFormat().GetExtension() );
	if ( !exporter )
	{
		wxMessageBox( wxT("No world exporters found"), wxT("Scene export"), wxOK | wxICON_ERROR );
		return;		
	}

	// Setup
	IExporter::ExportOptions exportOptions;
	exportOptions.m_resource = GGame->GetActiveWorld();
	exportOptions.m_saveFileFormat = m_exportFile.GetFileFormat();
	exportOptions.m_saveFilePath = m_exportFile.GetFile();
	exporter->DoExport( exportOptions );
}
