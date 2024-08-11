/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "minimapGeneratorTool.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/minimapGenerator.h"

BEGIN_EVENT_TABLE( CMinimapGeneratorTool, wxDialog )
	EVT_BUTTON( XRCID("m_bGenerateExteriors"), CMinimapGeneratorTool::OnGenerateExteriors )
	EVT_BUTTON( XRCID("m_bGenerateInteriors"), CMinimapGeneratorTool::OnGenerateInteriors )
	EVT_RADIOBUTTON( XRCID("rbOnlyCameraTile"), CMinimapGeneratorTool::OnTileSelectionRadioButton )
	EVT_RADIOBUTTON( XRCID("rbAroundCamera"), CMinimapGeneratorTool::OnTileSelectionRadioButton )
	EVT_RADIOBUTTON( XRCID("m_rbTileRange"), CMinimapGeneratorTool::OnTileSelectionRadioButton )
	EVT_RADIOBUTTON( XRCID("rbAllTiles"), CMinimapGeneratorTool::OnTileSelectionRadioButton )
	EVT_CHECKBOX( XRCID("cbSelectAll"), CMinimapGeneratorTool::OnSelectAllMasks )
	EVT_CHECKBOX( XRCID("m_cbForcedEnvSettings"), CMinimapGeneratorTool::OnForcedEnvSettings )
	EVT_CHECKLISTBOX( XRCID("m_clbMasks"), CMinimapGeneratorTool::OnSelectSingleMask )
	EVT_DIRPICKER_CHANGED( XRCID("m_dpOutputDirectory"), CMinimapGeneratorTool::OnOutputDirChanged )

	EVT_BUTTON( XRCID("m_gatherAllEntityTemplates"), CMinimapGeneratorTool::OnGatherAllEntityTemplates )
	EVT_BUTTON( XRCID("m_createInstances"), CMinimapGeneratorTool::OnCreateEntityTemplatesInstances )
	EVT_BUTTON( XRCID("m_createNavmesh"), CMinimapGeneratorTool::OnCreateNavmesh )
	EVT_BUTTON( XRCID("m_createNewInteriors"), CMinimapGeneratorTool::OnCreateNewInterior )
END_EVENT_TABLE()

CMinimapGeneratorTool::CMinimapGeneratorTool()
{
	// create new instance of minimap generator
	m_minimapGenerator.Reset( new CMinimapGenerator() );

	LoadControls();
	LoadOptionsFromConfig();
	SetEnableControls( true );
}

CMinimapGeneratorTool::~CMinimapGeneratorTool()
{
	/* intentionally empty */
}

RED_INLINE void CMinimapGeneratorTool::SaveOptionsToConfig()
{
	ISavableToConfig::SaveSession();
}

RED_INLINE void CMinimapGeneratorTool::LoadOptionsFromConfig()
{
	ISavableToConfig::RestoreSession();
}

void CMinimapGeneratorTool::SaveSession( CConfigurationManager &config )
{
	//
	config.Write( TXT("Tools/MinimapGenerator/X"), this->GetPosition().x );
	config.Write( TXT("Tools/MinimapGenerator/Y"), this->GetPosition().y );
	config.Write( TXT("Tools/MinimapGenerator/Width"), this->GetSize().GetWidth() );
	config.Write( TXT("Tools/MinimapGenerator/Height"), this->GetSize().GetHeight() );

	// general controls
	config.Write( TXT("Tools/MinimapGenerator/OutputDirectory"), m_dpOutputDirectory->GetDirName().GetFullPath().wc_str() );
	config.Write( TXT("Tools/MinimapGenerator/ForcedEnvSettings"), m_cbForcedEnvSettings->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/EnvFile"), m_fpEnvFile->GetFileName().GetFullPath().wc_str() );
	config.Write( TXT("Tools/MinimapGenerator/OnlyCameraTypeCheck"), m_rbOnlyCameraTile->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/AroundCameraCheck"), m_rbAroundCamera->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/TileRange"), m_rbTileRange->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/AllTilesCheck"), m_rbAllTiles->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/ExtraTiles"), m_scExtraTiles->GetValue() );
	config.Write( TXT("Tools/MinimapGenerator/TileRangeMinX"), m_scTileRangeMinX->GetValue() );
	config.Write( TXT("Tools/MinimapGenerator/TileRangeMinY"), m_scTileRangeMinY->GetValue() );
	config.Write( TXT("Tools/MinimapGenerator/TileRangeMaxX"), m_scTileRangeMaxX->GetValue() );
	config.Write( TXT("Tools/MinimapGenerator/TileRangeMaxY"), m_scTileRangeMaxY->GetValue() );

	// exteriors controls
	config.Write( TXT("Tools/MinimapGenerator/FilenamePrefix"), m_fileNamePrefix->GetValue().wc_str() );
	config.Write( TXT("Tools/MinimapGenerator/DefaultDirLayout"), m_rbDefaultDirLayout->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/PhotoshopDirLayout"), m_rbPhotoshopDirLayout->GetValue() ? 1 : 0 );
	config.Write( TXT("Tools/MinimapGenerator/TileSize"), m_cImageSize->GetSelection() );
	config.Write( TXT("Tools/MinimapGenerator/TileZoom"), m_scTileZoom->GetValue() );
	config.Write( TXT("Tools/MinimapGenerator/TileOffset"), m_scTileOffset->GetValue() );
	const Uint32 maskCount = m_clbMasks->GetCount();
	for( Uint32 i=0; i<maskCount; ++i )
	{
		config.Write( String::Printf( TXT("Tools/MinimapGenerator/Masks/%i"), i ), m_clbMasks->IsChecked( i ) ? 1 : 0 );
	}
}

void CMinimapGeneratorTool::RestoreSession( CConfigurationManager &config )
{
	//
	Int32 x = config.Read( TXT("Tools/MinimapGenerator/X"), this->GetPosition().x );
	Int32 y = config.Read( TXT("Tools/MinimapGenerator/Y"), this->GetPosition().y );
	Int32 width = config.Read( TXT("Tools/MinimapGenerator/Width"), this->GetSize().GetWidth() );
	Int32 height = config.Read( TXT("Tools/MinimapGenerator/Height"), this->GetSize().GetHeight() );

	this->SetPosition( wxPoint( x, y ) );
	this->SetSize( width, height );

	// general controls
	m_dpOutputDirectory->SetDirName( wxString( config.Read( TXT("Tools/MinimapGenerator/OutputDirectory"), TXT("") ).AsChar() ) );
	m_cbForcedEnvSettings->SetValue( config.Read( TXT("Tools/MinimapGenerator/ForcedEnvSettings"), 0 ) != 0  );
	m_fpEnvFile->SetFileName( wxString( config.Read( TXT("Tools/MinimapGenerator/EnvFile"), TXT("") ).AsChar() ) );
	m_rbOnlyCameraTile->SetValue( config.Read( TXT("Tools/MinimapGenerator/OnlyCameraTypeCheck"), 1 ) != 0 );
	m_rbAroundCamera->SetValue( config.Read( TXT("Tools/MinimapGenerator/AroundCameraCheck"), 0 ) != 0 );
	m_rbTileRange->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileRange"), 0 ) != 0 );
	m_rbAllTiles->SetValue( config.Read( TXT("Tools/MinimapGenerator/AllTilesCheck"), 0 ) != 0 );
	m_scExtraTiles->SetValue( config.Read( TXT("Tools/MinimapGenerator/ExtraTiles"), 0 ) );
	m_scTileRangeMinX->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileRangeMinX"), 0 ) );
	m_scTileRangeMinY->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileRangeMinY"), 0 ) );
	m_scTileRangeMaxX->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileRangeMaxX"), 0 ) );
	m_scTileRangeMaxY->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileRangeMaxY"), 0 ) );

	// exteriors controls
	m_fileNamePrefix->SetValue( config.Read( TXT("Tools/MinimapGenerator/FilenamePrefix"), TXT("Tile") ).AsChar() );
	m_rbDefaultDirLayout->SetValue( config.Read( TXT("Tools/MinimapGenerator/DefaultDirLayout"), 1 ) != 0 );
	m_rbPhotoshopDirLayout->SetValue( config.Read( TXT("Tools/MinimapGenerator/PhotoshopDirLayout"), 1 ) != 0 );
	m_cImageSize->SetSelection( config.Read( TXT("Tools/MinimapGenerator/TileSize"), 0 ) );
	m_scTileZoom->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileZoom"), 1 ) );
	m_scTileOffset->SetValue( config.Read( TXT("Tools/MinimapGenerator/TileOffset"), 0 ) );
	const Uint32 maskCount = m_clbMasks->GetCount();
	for( Uint32 i=0; i<maskCount; ++i )
	{
		m_clbMasks->Check( i, config.Read( String::Printf( TXT("Tools/MinimapGenerator/Masks/%i"), i ), 1 ) != 0 );
	}

	//
	m_fpEnvFile->Enable( m_cbForcedEnvSettings->IsChecked() );

	CheckPreviousSession();
}

void CMinimapGeneratorTool::OnGenerateExteriors( wxCommandEvent& event )
{
	Generate( TGM_Exteriors );
	CheckPreviousSession();
}

void CMinimapGeneratorTool::OnGenerateInteriors( wxCommandEvent& event )
{
	Generate( TGM_Interiors );
	m_tcInteriorErrors->SetValue( m_minimapGenerator->GetInteriorsErrors().AsChar() );
}

void CMinimapGeneratorTool::OnTileSelectionRadioButton( wxCommandEvent& event )
{
	// disable
	m_scExtraTiles->Enable( false );
	m_scTileRangeMinX->Enable( false );
	m_scTileRangeMinY->Enable( false );
	m_scTileRangeMaxX->Enable( false );
	m_scTileRangeMaxY->Enable( false );

	// enable selected
	if( event.GetEventObject() == m_rbAroundCamera )
	{
		m_scExtraTiles->Enable( m_rbAroundCamera->IsEnabled() && m_rbAroundCamera->GetValue() );
	}
	else if( event.GetEventObject() == m_rbTileRange )
	{
		m_scTileRangeMinX->Enable( m_rbTileRange->IsEnabled() && m_rbTileRange->GetValue() );
		m_scTileRangeMinY->Enable( m_rbTileRange->IsEnabled() && m_rbTileRange->GetValue() );
		m_scTileRangeMaxX->Enable( m_rbTileRange->IsEnabled() && m_rbTileRange->GetValue() );
		m_scTileRangeMaxY->Enable( m_rbTileRange->IsEnabled() && m_rbTileRange->GetValue() );
	}
}

void CMinimapGeneratorTool::OnSelectAllMasks( wxCommandEvent& event )
{
	Bool value = ( event.GetInt() == 0 ) ? false : true;

	const Uint32 count = m_clbMasks->GetCount();
	for( Uint32 i=0; i<count; ++i )
	{
		m_clbMasks->Check( i ,value );
	}
}

void CMinimapGeneratorTool::OnSelectSingleMask( wxCommandEvent& event )
{
	Uint32 itemIndex = event.GetInt();
	if( m_clbMasks->IsChecked( itemIndex ) == false )
	{
		m_cbSelectAll->SetValue( false );
	}
}

void CMinimapGeneratorTool::LoadControls()
{
	// Load dialog resource
	wxXmlResource::Get()->LoadDialog( this, NULL, wxT("MinimapGeneratorDialog") );

	// General controls
	m_dpOutputDirectory = XRCCTRL( *this, "m_dpOutputDirectory", wxDirPickerCtrl );
	m_cbForcedEnvSettings = XRCCTRL( *this, "m_cbForcedEnvSettings", wxCheckBox );
	m_fpEnvFile = XRCCTRL( *this, "m_fpEnvFile", wxFilePickerCtrl );
	m_rbOnlyCameraTile = XRCCTRL( *this, "rbOnlyCameraTile", wxRadioButton );
	m_rbAroundCamera = XRCCTRL( *this, "rbAroundCamera", wxRadioButton );
	m_rbTileRange = XRCCTRL( *this, "m_rbTileRange", wxRadioButton );
	m_rbAllTiles = XRCCTRL( *this, "rbAllTiles", wxRadioButton );
	m_scExtraTiles = XRCCTRL( *this, "scExtraTiles", wxSpinCtrl );
	m_scTileRangeMinX = XRCCTRL( *this, "m_scTileRangeMinX", wxSpinCtrl );
	m_scTileRangeMinY = XRCCTRL( *this, "m_scTileRangeMinY", wxSpinCtrl );
	m_scTileRangeMaxX = XRCCTRL( *this, "m_scTileRangeMaxX", wxSpinCtrl );
	m_scTileRangeMaxY = XRCCTRL( *this, "m_scTileRangeMaxY", wxSpinCtrl );

	// Exteriors controls
	m_fileNamePrefix = XRCCTRL( *this, "m_tcFileNamePrefix", wxTextCtrl );
	m_rbDefaultDirLayout = XRCCTRL( *this, "m_rbDefaultDirLayout", wxRadioButton );
	m_rbPhotoshopDirLayout = XRCCTRL( *this, "m_rbPhotoshopDirLayout", wxRadioButton );
	m_cImageSize = XRCCTRL( *this, "m_cImageSize", wxChoice );
	m_scTileZoom = XRCCTRL( *this, "m_scTileZoom", wxSpinCtrl );
	m_scTileOffset = XRCCTRL( *this, "m_scTileOffset", wxSpinCtrl );
	m_cbSelectAll = XRCCTRL( *this, "cbSelectAll", wxCheckBox );
	m_clbMasks = XRCCTRL( *this, "m_clbMasks", wxCheckListBox );
	m_pContinueGenerationPanel = XRCCTRL( *this, "m_pContinueGenerationPanel", wxPanel );
	m_cbContinueGeneration = XRCCTRL( *this, "m_cbContinueGeneration", wxCheckBox );
	m_bGenerateExteriors = XRCCTRL( *this, "m_bGenerateExteriors", wxButton );

	// fill checkbox list
	wxArrayString items;
	for( Uint32 i=0; i<MM_Count; ++i )
	{
		items.Add( ToString( static_cast< EMinimapMask >( i ) ).AsChar() );
	}
	m_clbMasks->InsertItems( items, 0 );

	// Interiors controls
	m_tcInteriorErrors = XRCCTRL( *this, "m_tcInteriorErrors", wxTextCtrl );
	m_bGenerateInteriors = XRCCTRL( *this, "m_bGenerateInteriors", wxButton );
	m_tcInteriorEntities = XRCCTRL( *this, "m_tcInteriorEntities", wxTextCtrl );

	// set init data
	String envPath = String::EMPTY;
	GDepot->GetAbsolutePath( envPath );
	envPath += TXT("environment\\definitions");
	m_fpEnvFile->SetInitialDirectory( wxString( envPath.AsChar() ) );
}

void CMinimapGeneratorTool::SetEnableControls( Bool enable )
{
	m_rbOnlyCameraTile->Enable( enable );
	m_rbAroundCamera->Enable( enable );
	m_rbTileRange->Enable( enable );
	m_rbAllTiles->Enable( enable );
	m_scExtraTiles->Enable( enable && m_rbAroundCamera->GetValue() );
	m_scTileRangeMinX->Enable( enable && m_rbTileRange->GetValue() );
	m_scTileRangeMinY->Enable( enable && m_rbTileRange->GetValue() );
	m_scTileRangeMaxX->Enable( enable && m_rbTileRange->GetValue() );
	m_scTileRangeMaxY->Enable( enable && m_rbTileRange->GetValue() );
	m_dpOutputDirectory->Enable( enable );
	m_fpEnvFile->Enable( enable );
	m_cbForcedEnvSettings->Enable( enable );
	m_rbDefaultDirLayout->Enable( enable );
	m_rbPhotoshopDirLayout->Enable( enable );
	m_cImageSize->Enable( enable );
	m_scTileZoom->Enable( enable );
	m_clbMasks->Enable( enable );
	m_bGenerateExteriors->Enable( enable );
	m_bGenerateInteriors->Enable( enable );
}

void CMinimapGeneratorTool::FillMinimapSettings( SMinimapSettings &minimapSettings )
{
	// masks
	const Uint32 maskCount = m_clbMasks->GetCount();
	for( Uint32 i=0; i<maskCount; ++i )
	{
		minimapSettings.m_exteriors.m_enabledMasks[i] = m_clbMasks->IsChecked( i );
	}

	// directory layout
	minimapSettings.m_exteriors.m_dirLayout = ( m_rbPhotoshopDirLayout->GetValue() == true ) ? DL_Photoshop : DL_Default;

	// file prefix
	minimapSettings.m_exteriors.m_fileNamePrefix = m_fileNamePrefix->GetValue().wc_str();

	// final image size
	FromString< Uint32 >( m_cImageSize->GetString( m_cImageSize->GetSelection() ).wc_str(), minimapSettings.m_exteriors.m_imageSize );

	// offset
	minimapSettings.m_exteriors.m_imageOffset = m_scTileOffset->GetValue();

	// zoom
	minimapSettings.m_exteriors.m_imageZoom = m_scTileZoom->GetValue();	

	// destination folder
	minimapSettings.m_outputDir = m_dpOutputDirectory->GetDirName().GetFullPath().wc_str();

	// camera position
	minimapSettings.m_exteriors.m_cameraPosition = wxTheFrame->GetWorldEditPanel()->GetCameraPosition();

	// tiles coordinates
	if ( m_rbOnlyCameraTile->GetValue() == true )
	{
		minimapSettings.m_generationMode = GM_CurrentTile;
	}
	else if ( m_rbAroundCamera->GetValue() == true )
	{
		minimapSettings.m_generationMode = GM_CurrentTilePlus;
		minimapSettings.m_tileCountPlus = m_scExtraTiles->GetValue();
	}
	else if( m_rbTileRange->GetValue() == true )
	{
		minimapSettings.m_generationMode = GM_TileRange;
		minimapSettings.m_tileRange = Box2( m_scTileRangeMinX->GetValue(), m_scTileRangeMinY->GetValue(), 
											m_scTileRangeMaxX->GetValue(), m_scTileRangeMaxY->GetValue() );
	}
	else if ( m_rbAllTiles->GetValue() == true )
	{
		minimapSettings.m_generationMode = GM_AllTiles;
	}

	// continue mode
	minimapSettings.m_exteriors.m_continueMode = m_cbContinueGeneration->GetValue();

	// forced env settings
	if( m_cbForcedEnvSettings->IsChecked() == true )
	{
		minimapSettings.m_exteriors.m_envSettingsPath = m_fpEnvFile->GetFileName().GetFullPath().wc_str();;
	}
}

void CMinimapGeneratorTool::CheckPreviousSession()
{
	Int32 x = 0;
	Int32 y = 0;
	const String destDir = m_dpOutputDirectory->GetDirName().GetFullPath().wc_str();
	if( m_minimapGenerator->IsUnsuccessfulFileExist( destDir, x, y ) == true )
	{
		m_cbContinueGeneration->SetLabel( String::Printf( TXT("Continue generation process from tile %i x %i"), x, y ).AsChar() );
		m_pContinueGenerationPanel->Show();
	}
	else
	{
		m_pContinueGenerationPanel->Hide();
	}
	LayoutRecursively( this, false );
}

void CMinimapGeneratorTool::OnOutputDirChanged( wxFileDirPickerEvent& event )
{
	CheckPreviousSession();
}

void CMinimapGeneratorTool::Generate( EToolGenerationMode mode )
{
	// Check the output directory name validity
	if( m_dpOutputDirectory->GetTextCtrl()->GetValue().Trim().IsEmpty() )
	{
		wxMessageBox( wxT("Please enter a valid directory name for the images."), wxT("Invalid directory name"), wxCENTRE|wxOK|wxICON_ERROR, this );
		m_dpOutputDirectory->SetFocus();
		return;
	}

	// Save the options
	SaveOptionsToConfig();
	SetEnableControls( false );

	// get settings from UI and fill minimap settings structure
	SMinimapSettings minimapSettings;
	FillMinimapSettings( minimapSettings );
	m_minimapGenerator->SetSettings( minimapSettings );

	// generate minimaps
	switch( mode )
	{
	case TGM_Exteriors:
		m_minimapGenerator->GenerateExteriors();
		break;
	case TGM_Interiors:
		m_minimapGenerator->GenerateInteriors();
		break;
	}

	// unlock UI
	SetEnableControls( true );

	// inform about generation result
	CFilePath worldPath( GGame->GetActiveWorld()->GetFriendlyName() );
	String worldName = worldPath.GetFileName();
	String descDir = m_dpOutputDirectory->GetDirName().GetFullPath().wc_str();
	if( GFeedback->AskYesNo( TXT("The minimap exteriors were exported\nDo you want to open the containing folder?") ) == true )
	{
		OpenExternalFile( TXT("explorer ") + descDir + worldName );
	}
}

void CMinimapGeneratorTool::OnForcedEnvSettings( wxCommandEvent& event )
{
	Bool value = ( event.GetInt() == 0 ) ? false : true;
	m_fpEnvFile->Enable( value );
}

void CMinimapGeneratorTool::OnGatherAllEntityTemplates( wxCommandEvent& event )
{
	pathToEntityTemplates.ClearFast();
	entities.ClearFast();

	TDynArray<String> specifiedEntities;
	Int32 lineNum = m_tcInteriorEntities->GetNumberOfLines();

	for( Int32 i=0; i<lineNum; ++i )
	{
		wxString lineWx = m_tcInteriorEntities->GetLineText( i );
#ifdef UNICODE
		specifiedEntities.PushBack( String( ANSI_TO_UNICODE( lineWx.c_str().AsChar() ) ) );
#else
		specifiedEntities.PushBack( String( lineWx.c_str().AsChar() ) );
#endif
	}

	m_minimapGenerator->GatherAllEntityTemplatesWithInterior( pathToEntityTemplates, specifiedEntities );
	wxStaticText* text = XRCCTRL( *this, "m_foundEntityTemplateCount", wxStaticText );
	text->SetLabel( ToString( pathToEntityTemplates.Size() ).AsChar() );
}

void CMinimapGeneratorTool::OnCreateEntityTemplatesInstances( wxCommandEvent& event )
{
	m_minimapGenerator->CreateInstances( pathToEntityTemplates, entities );
	wxStaticText* text = XRCCTRL( *this, "m_createdInstanceCount", wxStaticText );
	text->SetLabel( m_minimapGenerator->GetCreatedInstanceCount().AsChar() );
}

void CMinimapGeneratorTool::OnCreateNavmesh( wxCommandEvent& event )
{
	m_minimapGenerator->CreateNavmesh( entities );
}

void CMinimapGeneratorTool::OnCreateNewInterior( wxCommandEvent& event )
{
	m_minimapGenerator->CreateNewInteriors();
}
