/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sceneDialogsetEditor.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"

BEGIN_EVENT_TABLE( CEdSceneDialogsetEditor, wxSmartLayoutPanel )
	EVT_LISTBOX( XRCID( "PersonalCamerasList" ), CEdSceneDialogsetEditor::OnPersonalCameraSelected )
	EVT_LISTBOX( XRCID( "MasterCamerasList" ), CEdSceneDialogsetEditor::OnMasterCameraSelected )
	EVT_LISTBOX( XRCID( "CustomCamerasList" ), CEdSceneDialogsetEditor::OnCustomCameraSelected )

	EVT_MENU( XRCID( "SaveMenuItem" ), CEdSceneDialogsetEditor::OnSave )
	EVT_MENU( XRCID( "CloseMenuItem" ), CEdSceneDialogsetEditor::OnClose )
	EVT_MENU( XRCID( "ImportOldMenuItem" ), CEdSceneDialogsetEditor::OnImportOldDialogset )

	EVT_BUTTON( XRCID( "ImportCharacterTrajButton" ), CEdSceneDialogsetEditor::OnImportCharacterTrajectoriesButton )
	EVT_BUTTON( XRCID( "ImportCameraTrajButton" ), CEdSceneDialogsetEditor::OnImportCameraTrajectoriesButton )

	EVT_BUTTON( XRCID( "AddPersonalCameraButton" ), CEdSceneDialogsetEditor::OnAddPersonalCamera )
	EVT_BUTTON( XRCID( "RemovePersonalCameraButton" ), CEdSceneDialogsetEditor::OnRemovePersonalCamera )
	EVT_BUTTON( XRCID( "AddMasterCameraButton" ), CEdSceneDialogsetEditor::OnAddMasterCamera )
	EVT_BUTTON( XRCID( "RemoveMasterCameraButton" ), CEdSceneDialogsetEditor::OnRemoveMasterCamera )
	EVT_BUTTON( XRCID( "AddCustomCameraButton" ), CEdSceneDialogsetEditor::OnAddCustomCamera )
	EVT_BUTTON( XRCID( "RemoveCustomCameraButton" ), CEdSceneDialogsetEditor::OnRemoveCustomCamera )

	EVT_TOOL( XRCID( "ShowPlaceablesTool" ), CEdSceneDialogsetEditor::OnTogglePlaceables )
	EVT_TOOL( XRCID( "CharacterGhostsTool" ), CEdSceneDialogsetEditor::OnToggleCharacters )

END_EVENT_TABLE()

enum EDialogsetEditorActions
{
	ID_IMPORT	= 8000,
	ID_REMOVE,
	ID_REIMPORT,
	ID_RENAME,
	ID_REIMPORTALL,
	ID_REMOVEALL,
	ID_HACK_ACTIVATECAM
};


CEdSceneDialogsetEditor::CEdSceneDialogsetEditor( wxWindow* parent, CStorySceneDialogset* dialogset )
	: wxSmartLayoutPanel( parent, TEXT( "DialogsetEditor" ), false )
	, m_dialogset( dialogset )
{
	dialogset->AddToRootSet();

	m_cameraShotList = XRCCTRL( *this, "CameraAnimationsList", wxListBox );
	m_characterTrajectoriesNumberLabel = XRCCTRL( *this, "CharacterTrajectoriesNumberLabel", wxStaticText );
	m_cameraTrajectoriesNumberLabel = XRCCTRL( *this, "CameraTrajectoriesNumberLabel", wxStaticText );
	m_personalCamerasList = XRCCTRL( *this, "PersonalCamerasList", wxListBox );
	m_masterCamerasList = XRCCTRL( *this, "MasterCamerasList", wxListBox );

	m_cameraShotList->Connect( wxEVT_RIGHT_DOWN, 
		wxMouseEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationsRightClick ), NULL, this );


	wxPanel* previewPanel = XRCCTRL( *this, "DialogsetPreviewPanel", wxPanel );
	m_dialogsetPreview = new CEdSceneDialogsetPreviewPanel( previewPanel, m_dialogset );
	previewPanel->GetSizer()->Add( m_dialogsetPreview, 1, wxEXPAND );


	PropertiesPageSettings propertiesSettings;

	// Prepare personal camera list
	wxPanel* personalCamerasPropertiesPanel = XRCCTRL( *this, "PersonalCamerasPropertiesPanel", wxPanel );
	m_personalCamerasProperties = new CEdSceneDialogsetPropertyPage( personalCamerasPropertiesPanel, propertiesSettings, m_dialogset, nullptr );
	personalCamerasPropertiesPanel->GetSizer()->Add( m_personalCamerasProperties, 1, wxEXPAND );
	m_personalCamerasProperties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, 
		wxCommandEventHandler( CEdSceneDialogsetEditor::OnPersonalCameraPropertyChanged ), NULL, this );

	// Prepare master camera list
	wxPanel* masterCamerasPropertiesPanel = XRCCTRL( *this, "MasterCamerasPropertiesPanel", wxPanel );
	m_masterCamerasProperties = new CEdSceneDialogsetPropertyPage( masterCamerasPropertiesPanel, propertiesSettings, m_dialogset, nullptr );
	masterCamerasPropertiesPanel->GetSizer()->Add( m_masterCamerasProperties, 1, wxEXPAND );
	m_masterCamerasProperties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, 
		wxCommandEventHandler( CEdSceneDialogsetEditor::OnMasterCameraPropertyChanged ), NULL, this );

	// Prepare custom camera list
	wxPanel* customCamerasPropertiesPanel = XRCCTRL( *this, "CustomCamerasPropertiesPanel", wxPanel );
	m_customCamerasProperties = new CEdSceneDialogsetPropertyPage( customCamerasPropertiesPanel, propertiesSettings, m_dialogset, nullptr );
	customCamerasPropertiesPanel->GetSizer()->Add( m_customCamerasProperties, 1, wxEXPAND );
	m_customCamerasProperties->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, 
		wxCommandEventHandler( CEdSceneDialogsetEditor::OnCustomCameraPropertyChanged ), NULL, this );


	m_customCamerasList = XRCCTRL( *this, "CustomCamerasList", wxListBox );
	m_customCamerasList->Clear();
	const TDynArray< SSceneCustomCameraDescription >& customCameras = m_dialogset->GetCustomCameras();
	for( TDynArray< SSceneCustomCameraDescription >::const_iterator customIter = customCameras.Begin();
		customIter != customCameras.End(); ++customIter )
	{
		m_customCamerasList->AppendString( customIter->m_cameraName.AsString().AsChar() );
	}

	wxPanel* dialogsetPropertiesPanel = XRCCTRL( *this, "DialogsetPropertiesPanel", wxPanel );
	m_dialogsetProperties = new CEdPropertiesPage( dialogsetPropertiesPanel, propertiesSettings, nullptr );
	dialogsetPropertiesPanel->GetSizer()->Add( m_dialogsetProperties, 1, wxEXPAND );
	

	UpdateDialogsetProperties();

	Fit();
	Layout();
}

CEdSceneDialogsetEditor::~CEdSceneDialogsetEditor()
{
	m_dialogset->RemoveFromRootSet();
}

void CEdSceneDialogsetEditor::UpdateDialogsetProperties()
{
	m_cameraShotList->Clear();
	TDynArray< CSkeletalAnimationSetEntry* > cameraShotAnimations;
	m_dialogset->GetAnimations( cameraShotAnimations );
	for ( Uint32 i = 0; i < cameraShotAnimations.Size(); ++i )
	{
		m_cameraShotList->AppendString( cameraShotAnimations[ i ]->GetName().AsString().AsChar() );
	}

	m_characterTrajectoriesNumberLabel->SetLabel( wxString::Format( TXT( "Character Trajectories Number: %d" ), m_dialogset->GetNumberOfSlots() ) );
	m_cameraTrajectoriesNumberLabel->SetLabel( wxString::Format( TXT( "Camera Trajectories Number: %d" ), m_dialogset->GetCameraTrajectories().Size() ) );


	PropertiesPageSettings propertiesSettings;

	// Prepare personal camera list
	m_personalCamerasList->Clear();
	const TDynArray< SScenePersonalCameraDescription >& personalCameras = m_dialogset->GetPersonalCameras();
	for( TDynArray< SScenePersonalCameraDescription >::const_iterator personalIter = personalCameras.Begin();
		personalIter != personalCameras.End(); ++personalIter )
	{
		m_personalCamerasList->AppendString( personalIter->m_cameraName.AsString().AsChar() );
	}
	m_personalCamerasProperties->SetPersonalCameraDescription( NULL );

	// Prepare master camera list
	m_masterCamerasList->Clear();
	const TDynArray< SSceneMasterCameraDescription >& masterCameras = m_dialogset->GetMasterCameras();
	for( TDynArray< SSceneMasterCameraDescription >::const_iterator masterIter = masterCameras.Begin();
		masterIter != masterCameras.End(); ++masterIter )
	{
		m_masterCamerasList->AppendString( masterIter->m_cameraName.AsString().AsChar() );
	}
	m_masterCamerasProperties->SetMasterCameraDescription( NULL );

	m_dialogsetProperties->SetObject( m_dialogset );
}

CName CEdSceneDialogsetEditor::GetSelectedCameraAnimationName()
{
	return CName( m_cameraShotList->GetStringSelection().wc_str() );
}

void CEdSceneDialogsetEditor::OnPersonalCameraPropertyChanged( wxCommandEvent& event )
{
	TDynArray< SScenePersonalCameraDescription >& personalCameras = m_dialogset->GetPersonalCameras();
	ASSERT( personalCameras.Size() == m_personalCamerasList->GetCount() );
	for ( Uint32 i = 0; i < personalCameras.Size(); ++i )
	{
		m_personalCamerasList->SetString( i, personalCameras[ i ].m_cameraName.AsString().AsChar() );
	}

	wxCommandEvent dummyEvent( wxEVT_COMMAND_PROPERTY_CHANGED, 0 );
	OnPersonalCameraSelected( dummyEvent );

}

void CEdSceneDialogsetEditor::OnMasterCameraPropertyChanged( wxCommandEvent& event )
{
	TDynArray< SSceneMasterCameraDescription >& masterCameras = m_dialogset->GetMasterCameras();
	ASSERT( masterCameras.Size() == m_masterCamerasList->GetCount() );
	for ( Uint32 i = 0; i < masterCameras.Size(); ++i )
	{
		m_masterCamerasList->SetString( i, masterCameras[ i ].m_cameraName.AsString().AsChar() );
	}

	wxCommandEvent dummyEvent( wxEVT_COMMAND_PROPERTY_CHANGED, 0 );
	OnMasterCameraSelected( dummyEvent );
}

void CEdSceneDialogsetEditor::OnCustomCameraPropertyChanged( wxCommandEvent& event )
{
	TDynArray< SSceneCustomCameraDescription >& customCameras = m_dialogset->GetCustomCameras();
	ASSERT( customCameras.Size() == m_customCamerasList->GetCount() );
	for ( Uint32 i = 0; i < customCameras.Size(); ++i )
	{
		m_customCamerasList->SetString( i, customCameras[ i ].m_cameraName.AsString().AsChar() );
	}
}

void CEdSceneDialogsetEditor::OnPersonalCameraSelected( wxCommandEvent& event )
{
	TDynArray< SScenePersonalCameraDescription >& personalCameras = m_dialogset->GetPersonalCameras();
	Int32 personalCameraIndex = m_personalCamerasList->GetSelection();

	ASSERT( personalCameras.Size() == m_personalCamerasList->GetCount() );
	ASSERT( personalCameraIndex >= 0 && (Uint32) personalCameraIndex < personalCameras.Size() );

	SScenePersonalCameraDescription& personalCameraDescription = personalCameras[ personalCameraIndex ];
	m_personalCamerasProperties->SetPersonalCameraDescription( &personalCameraDescription );

	m_dialogsetPreview->SetSelectedCameraNumber( personalCameraDescription.m_cameraNumber );
}

void CEdSceneDialogsetEditor::OnMasterCameraSelected( wxCommandEvent& event )
{
	TDynArray< SSceneMasterCameraDescription >& masterCameras = m_dialogset->GetMasterCameras();
	Int32 masterCameraIndex = m_masterCamerasList->GetSelection();

	ASSERT( masterCameras.Size() == m_masterCamerasList->GetCount() );
	ASSERT( masterCameraIndex >= 0 && (Uint32) masterCameraIndex < masterCameras.Size() );

	SSceneMasterCameraDescription& masterCameraDescription = masterCameras[ masterCameraIndex ];
	m_masterCamerasProperties->SetMasterCameraDescription( &masterCameraDescription );

	m_dialogsetPreview->SetSelectedCameraNumber( masterCameraDescription.m_cameraNumber );
}

void CEdSceneDialogsetEditor::OnCustomCameraSelected( wxCommandEvent& event )
{
	TDynArray< SSceneCustomCameraDescription >& customCameras = m_dialogset->GetCustomCameras();
	Int32 customCameraIndex = m_customCamerasList->GetSelection();

	ASSERT( customCameras.Size() == m_customCamerasList->GetCount() );
	ASSERT( customCameraIndex >= 0 && (Uint32) customCameraIndex < customCameras.Size() );

	SSceneCustomCameraDescription& customCameraDescription = customCameras[ customCameraIndex ];
	m_customCamerasProperties->SetObject( &customCameraDescription );
}

void CEdSceneDialogsetEditor::OnAddPersonalCamera( wxCommandEvent& event )
{
	Uint32 cameraNumber = m_personalCamerasList->GetCount() + 1;
	String newCameraName = String::Printf( TXT( "Personal %d" ), cameraNumber );

	//m_dialogset->AddPersonalCamera( CName( newCameraName ), cameraNumber );
	m_personalCamerasList->AppendString( newCameraName.AsChar() );
}

void CEdSceneDialogsetEditor::OnRemovePersonalCamera( wxCommandEvent& event )
{
	Int32 selectedCameraIndex = m_personalCamerasList->GetSelection();
	if ( selectedCameraIndex < 0 || (Uint32) selectedCameraIndex >= m_personalCamerasList->GetCount() )
	{
		return;
	}

	m_personalCamerasList->Delete( selectedCameraIndex );

	if ( m_dialogset->GetPersonalCameras().Size() > Uint32( selectedCameraIndex ) )
	{
		m_dialogset->GetPersonalCameras().Erase( m_dialogset->GetPersonalCameras().Begin() + selectedCameraIndex );
	}
}

void CEdSceneDialogsetEditor::OnAddMasterCamera( wxCommandEvent& event )
{
	Uint32 cameraNumber = m_masterCamerasList->GetCount() + 1;
	String newCameraName = String::Printf( TXT( "Master %d" ), cameraNumber );

	//m_dialogset->AddMasterCamera( CName( newCameraName ), cameraNumber );
	m_masterCamerasList->AppendString( newCameraName.AsChar() );
}

void CEdSceneDialogsetEditor::OnRemoveMasterCamera( wxCommandEvent& event )
{
	Int32 selectedCameraIndex = m_masterCamerasList->GetSelection();
	if ( selectedCameraIndex < 0 || (Uint32) selectedCameraIndex >= m_masterCamerasList->GetCount() )
	{
		return;
	}

	m_masterCamerasList->Delete( selectedCameraIndex );
	
	if ( m_dialogset->GetMasterCameras().Size() > Uint32( selectedCameraIndex ) )
	{
		m_dialogset->GetMasterCameras().Erase( m_dialogset->GetMasterCameras().Begin() + selectedCameraIndex );
	}
}

void CEdSceneDialogsetEditor::OnAddCustomCamera( wxCommandEvent& event )
{
	Uint32 cameraNumber = m_customCamerasList->GetCount() + 1;
	String newCameraName = String::Printf( TXT( "Custom %d" ), cameraNumber );

	//m_dialogset->AddCustomCamera( CName( newCameraName ) );
	m_customCamerasList->AppendString( newCameraName.AsChar() );
}

void CEdSceneDialogsetEditor::OnRemoveCustomCamera( wxCommandEvent& event )
{
	Int32 selectedCameraIndex = m_customCamerasList->GetSelection();
	if ( selectedCameraIndex < 0 || (Uint32) selectedCameraIndex >= m_customCamerasList->GetCount() )
	{
		return;
	}

	m_customCamerasList->Delete( selectedCameraIndex );

	if ( m_dialogset->GetCustomCameras().Size() > Uint32( selectedCameraIndex ) )
	{
		m_dialogset->GetCustomCameras().Erase( m_dialogset->GetCustomCameras().Begin() + selectedCameraIndex );
	}
}

void CEdSceneDialogsetEditor::OnSave( wxCommandEvent& event )
{
	m_dialogset->Save();
}

void CEdSceneDialogsetEditor::OnClose( wxCommandEvent& event )
{
	Close();
}

void CEdSceneDialogsetEditor::OnImportOldDialogset( wxCommandEvent& event )
{
	C2dArray* oldDialogsetTable = Cast< C2dArray >( GDepot->LoadResource( TXT("gameplay\\globals\\scenes\\scene_dialogsets.csv") ) );

	if ( oldDialogsetTable == NULL )
	{
		return;
	}

	TDynArray< String > oldDialogsetNames;
	
	for ( Uint32 i = 0; i < oldDialogsetTable->GetNumberOfRows(); ++i )
	{
		oldDialogsetNames.PushBack( oldDialogsetTable->GetValue( TXT( "Name" ) , i ) );
	}

	String selectedOldDialogsetName = InputComboBox( this, TXT( "Import old dialogset" ), 
		TXT( "Select old dialogset name to import" ), TXT( "dialogset_4_vs_4" ), oldDialogsetNames );

	Uint32 charactersInDialogset 
		= oldDialogsetTable->GetValue< Uint32 >( TXT( "Name" ), selectedOldDialogsetName, TXT( "Characters" ) );
	Uint32 camerasInDialogset 
		= oldDialogsetTable->GetValue< Uint32 >( TXT( "Name" ), selectedOldDialogsetName, TXT( "Cameras" ) );


	m_dialogset->ImportFromOldDialogset( CName( selectedOldDialogsetName ), charactersInDialogset, camerasInDialogset );
	UpdateDialogsetProperties();
}

void CEdSceneDialogsetEditor::OnImportCharacterTrajectoriesButton( wxCommandEvent& event )
{
	m_dialogset->ConvertSlots();	
}

void CEdSceneDialogsetEditor::OnImportCameraTrajectoriesButton( wxCommandEvent& event )
{
	m_dialogset->ConvertCameraDefinitions();
}

void CEdSceneDialogsetEditor::OnActivateCamAdjust( wxCommandEvent& event )
{
	Uint32 camNr = m_dialogset->GetCameraDefinitions().Size();
	for( Uint32 i = 0; i < camNr; i++ )
	{
		StorySceneCameraDefinition& cam = m_dialogset->GetCameraDefinitions()[i];

		//geralt eyes level
		cam.m_sourceEyesHeigth = 1.75381f;
		cam.m_targetEyesLS = Vector( -0.00626384f, 0.107412f, 1.75381f );
		cam.m_cameraAdjustVersion = 3;
	}
}

void CEdSceneDialogsetEditor::OnCameraAnimationsRightClick( wxMouseEvent& event )
{
	if ( m_cameraShotList == NULL )
	{
		return;
	}

	wxMenu menu;

	menu.Append( ID_IMPORT, TXT( "Import camera animations" ) );
	menu.Connect( ID_IMPORT, wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationImport ), NULL, this );
	
	menu.Append( ID_HACK_ACTIVATECAM, TXT( "HCK Activate camera adjust" ) );
	menu.Connect( ID_HACK_ACTIVATECAM, wxEVT_COMMAND_MENU_SELECTED, 
		wxCommandEventHandler( CEdSceneDialogsetEditor::OnActivateCamAdjust ), NULL, this );

	if ( m_cameraShotList->GetSelection() != wxNOT_FOUND )
	{
		menu.AppendSeparator();
		menu.Append( ID_REMOVE, TXT( "Remove" ) );
		menu.Append( ID_REIMPORT, TXT( "Reimport" ) );
		menu.Append( ID_HACK_ACTIVATECAM, TXT( "Rename" ) );

		menu.Connect( ID_REMOVE, wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationRemove ), NULL, this );
		menu.Connect( ID_REIMPORT, wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationReimport ), NULL, this );
		menu.Connect( ID_RENAME, wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationRename ), NULL, this );
	}

	if ( m_cameraShotList->GetCount() != 0 )
	{
		menu.AppendSeparator();
		menu.Append( ID_REMOVEALL, TXT( "Remove all" ) );
		menu.Append( ID_REIMPORTALL, TXT( "Reimport all" ) );

		menu.Connect( ID_REMOVEALL, wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationRemoveAll ), NULL, this );
		menu.Connect( ID_REIMPORTALL, wxEVT_COMMAND_MENU_SELECTED, 
			wxCommandEventHandler( CEdSceneDialogsetEditor::OnCameraAnimationReimportAll ), NULL, this );
	}
	
	PopupMenu( &menu );
}

void CEdSceneDialogsetEditor::OnCameraAnimationRemove( wxCommandEvent& event )
{
	CSkeletalAnimationSetEntry* animationEntry = m_dialogset->FindAnimation( GetSelectedCameraAnimationName() );
	m_dialogset->RemoveAnimation( animationEntry );

	UpdateDialogsetProperties();
}

void CEdSceneDialogsetEditor::OnCameraAnimationReimport( wxCommandEvent& event )
{
	CSkeletalAnimationSetEntry* animationEntry = m_dialogset->FindAnimation( GetSelectedCameraAnimationName() );

	ImportAnimationOptions options;
	options.m_entry = animationEntry;
	options.m_preferBetterQuality = true;
	m_dialogset->ReimportAnimation( options );

	UpdateDialogsetProperties();
}

void CEdSceneDialogsetEditor::OnCameraAnimationRename( wxCommandEvent& event )
{
	CName selectedCameraAnimationName = GetSelectedCameraAnimationName();
	

	CSkeletalAnimationSetEntry* animationEntry = m_dialogset->FindAnimation( selectedCameraAnimationName );
	String newName = InputBox( this, TXT("Rename animation"), 
		TXT("Write new name:"), selectedCameraAnimationName.AsString() );

	m_dialogset->RenameAnimation( animationEntry, CName( newName ) );


	UpdateDialogsetProperties();
	//m_dialogset->CalculateCameraEyePositions();
}

void CEdSceneDialogsetEditor::OnCameraAnimationImport( wxCommandEvent& event )
{
	// Get supported file formats for animations
	TDynArray< CFileFormat > animationFileFormats;
	IImporter::EnumImportFormats( ClassID< CSkeletalAnimation >(), animationFileFormats );

	// Ask for files
	CEdFileDialog fileDialog;
	fileDialog.AddFormats( animationFileFormats );
	fileDialog.SetMultiselection( true );
	fileDialog.SetIniTag( TXT("DialogsetCameraAnimImport") );
	if ( fileDialog.DoOpen( (HWND) GetHWND(), true ) && m_dialogset->MarkModified() )
	{
		const TDynArray< String >& files = fileDialog.GetFiles();
		if ( files.Empty() == true )
		{
			return;
		}

		wxTextEntryDialog animationPrefixDialog( this, wxT( "Enter prefix part to replace"), 
			wxT( "Enter animation prefix phrase to replace with camera name" ),
			CFilePath( files[ 0 ] ).GetFileName().AsChar(), 
			wxOK );

		String animationPrefix = String::EMPTY;
		if ( animationPrefixDialog.ShowModal() == wxID_OK )
		{
			animationPrefix = animationPrefixDialog.GetValue().wc_str();
		}


		// Begin import phase
		GFeedback->BeginTask( TXT("Importing animations"), true );

		// Import animations
		
		for ( Uint32 i=0; i<files.Size(); i++ )
		{
			// Cancel task
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			// Update progress
			GFeedback->UpdateTaskInfo( TXT("Importing '%s'..."), files[i].AsChar() );
			GFeedback->UpdateTaskProgress( i, files.Size() );

			// Import animation
			const String& importAnimFile = files[i];

			// Change animation name
			CFilePath filePath( importAnimFile );
			
			String parentDirectoryName = filePath.GetDirectories().Back();
			String animationName = filePath.GetFileName();
			
			animationName.Replace( animationPrefix, parentDirectoryName );

			ImportAnimationOptions options;
			options.m_animationFile = importAnimFile;
			options.m_animationName = CName( animationName );
			options.m_preferBetterQuality = true;

			m_dialogset->ImportAnimation( options );
		}

		//m_dialogset->CalculateCameraEyePositions();

		UpdateDialogsetProperties();

		// Select last animation from import set
		if ( files.Size() > 0 )
		{
			const String& importAnimFile = files.Back();
			m_cameraShotList->SetStringSelection( importAnimFile.AsChar() );
		}

		// End import phase
		GFeedback->EndTask();
	}
	
}

void CEdSceneDialogsetEditor::OnCameraAnimationRemoveAll( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_cameraShotList->GetCount(); ++i )
	{
		CSkeletalAnimationSetEntry* animationEntry = m_dialogset->FindAnimation( 
			CName( m_cameraShotList->GetString( i ).wc_str() ) );
		m_dialogset->RemoveAnimation( animationEntry );
	}

	UpdateDialogsetProperties();
	//m_dialogset->CalculateCameraEyePositions();
}

void CEdSceneDialogsetEditor::OnCameraAnimationReimportAll( wxCommandEvent& event )
{

	if ( m_dialogset && m_dialogset->MarkModified() )
	{
		// Begin import phase
		GFeedback->BeginTask( TXT("Reimporting animations"), true );

		// Get animations
		TDynArray< CSkeletalAnimationSetEntry* > animations;
		m_dialogset->GetAnimations( animations );

		// Reimport all animations
		for ( Uint32 i=0; i<animations.Size(); i++ )
		{
			// Cancel task
			if ( GFeedback->IsTaskCanceled() )
			{
				break;
			}

			CSkeletalAnimationSetEntry* entry = animations[i];
			if ( entry && entry->GetAnimation() )
			{
				// Update progress
				GFeedback->UpdateTaskInfo( TXT("Reimport '%s'..."), entry->GetAnimation()->GetName().AsString().AsChar() );
				GFeedback->UpdateTaskProgress( i, animations.Size() );

				// Reimport
				ImportAnimationOptions options;
				options.m_entry = entry;
				options.m_preferBetterQuality = true;

				m_dialogset->ReimportAnimation( options );
			}
		}

		// End import phase
		GFeedback->EndTask();

		UpdateDialogsetProperties();
		//m_dialogset->CalculateCameraEyePositions();

		// Select last
		if ( animations.Size() > 0 )
		{
			m_cameraShotList->SetStringSelection( animations.Back()->GetName().AsString().AsChar() );
		}
	}	
}

void CEdSceneDialogsetEditor::OnTogglePlaceables( wxCommandEvent& event )
{
	m_dialogsetPreview->TogglePlaceables( event.IsChecked() );
}

void CEdSceneDialogsetEditor::OnToggleCharacters( wxCommandEvent& event )
{
	m_dialogsetPreview->ToggleCharacters( event.IsChecked() );
}