/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "localizationToolsFrame.h"

#include "../../common/game/storyScene.h"

#include "exporters/sceneLocalizationExporter.h"
#include "exporters/gameplayEntityLocalizationExporter.h"
#include "exporters/locStringsWithKeysLocalizationExporter.h"
#include "exporters/customSceneExporters.h"
#include "..\..\common\engine\localizationManager.h"

#include "lipsyncDataSceneExporter.h"
#include "sceneUsageExporter.h"
#include "journalLocalizationExporter.h"

#include "../../common/core/directory.h"
#include "../../common/core/feedback.h"
#include "dialogEditorLocAnalyser.h"
#include "sceneAnimationReporter.h"

BEGIN_EVENT_TABLE( CEdLocalizationTool, wxSmartLayoutPanel )
	EVT_BUTTON( XRCID( "ExportButton" ), CEdLocalizationTool::OnExportButtonClick )
	EVT_BUTTON( XRCID( "OpenPresetButton" ), CEdLocalizationTool::OnOpenPresetButton )
	EVT_BUTTON( XRCID( "SavePresetButton" ), CEdLocalizationTool::OnSavePresetButton )
	EVT_BUTTON( XRCID( "SelectAllButton" ), CEdLocalizationTool::OnSelectAllButton )
	EVT_BUTTON( XRCID( "DeselectAllButton" ), CEdLocalizationTool::OnDeselectAllButton )
	EVT_BUTTON( XRCID( "ToggleAllButton" ), CEdLocalizationTool::OnToggleAllButton )
	
	EVT_CHECKBOX( XRCID( "RecursiveFilesCheckbox" ), CEdLocalizationTool::OnRecursiveFilesChanged )
	EVT_TREE_SEL_CHANGED( XRCID( "DirectoryTree" ), CEdLocalizationTool::OnDirectorySelected )
	EVT_BUTTON( XRCID( "ImportButton" ), CEdLocalizationTool::OnImportButtonClick )
	EVT_FILEPICKER_CHANGED( XRCID( "ImportFileChoice" ), CEdLocalizationTool::OnFileSelected )
	EVT_CHECKLISTBOX( XRCID( "ExportFileList" ), CEdLocalizationTool::OnExportFileListCheckListBox )
	EVT_CHOICE( XRCID( "ExportTypeChoice" ), CEdLocalizationTool::OnExportFileTypeChanged )

END_EVENT_TABLE()

CEdLocalizationTool::CEdLocalizationTool( wxWindow* parent )
	: wxSmartLayoutPanel( parent, TEXT( "LocalizationTool" ), false )
	, m_exporter( NULL )
	, m_rootBatchExportGroup( NULL )
{
	m_exportTypeChoice			= XRCCTRL( *this, "ExportTypeChoice",		wxChoice );
	m_exportFilter				= XRCCTRL( *this, "ExportFilter",			wxTextCtrl );
	m_directoryTree				= XRCCTRL( *this, "DirectoryTree",			wxTreeCtrl );
	m_exportFileList			= XRCCTRL( *this, "ExportFileList",			wxCheckListBox );
	m_exportButton				= XRCCTRL( *this, "ExportButton",			wxButton );
	m_statusBar					= XRCCTRL( *this, "StatusBar",				wxGauge );
	m_statusLabel				= XRCCTRL( *this, "StatusLabel",			wxStaticText );
	m_sourceLanguageChoice		= XRCCTRL( *this, "SourceLanguageChoice",	wxChoice );
	m_recursiveFilesCheckbox	= XRCCTRL( *this, "RecursiveFilesCheckbox", wxCheckBox );

	m_checkoutCheckbox				= XRCCTRL( *this, "CheckoutCheckbox",				wxCheckBox );
	m_validateExportEntriesCheckbox	= XRCCTRL( *this, "ValidateExportEntriesCheckbox",	wxCheckBox );
	m_targetLanguageList			= XRCCTRL( *this, "TargetLanguageList",				wxCheckListBox );
	m_emptyColumnsNumberSpin		= XRCCTRL( *this, "TagetLanguagesCountSpin",		wxSpinCtrl );
	m_minPathDepthSpin				= XRCCTRL( *this, "MinPathDepthSpin",				wxSpinCtrl );
	m_maxPathDepthSpin				= XRCCTRL( *this, "MaxPathDepthSpin",				wxSpinCtrl );

	m_markImportantVoicetagsCheckbox	= XRCCTRL( *this, "MarkImportantVoicetagCheckbox",	wxCheckBox );
	m_cutsceneDescriptionsCheckbox		= XRCCTRL( *this, "CutsceneDesctiptionsCheckbox", wxCheckBox );
	m_onlyCutscenesCheckbox				= XRCCTRL( *this, "OnlyCutsceneCheckbox", wxCheckBox );

	m_importButton				= XRCCTRL( *this, "ImportButton",			wxButton );
	m_importFilePicker			= XRCCTRL( *this, "ImportFileChoice",		wxFilePickerCtrl );

	// Export Filter Panel
	m_voicetagFilterEnableCheckbox	= XRCCTRL( *this, "VoicetagFilterEnableCheckbox",	wxCheckBox );
	m_voiceTagFilterList			= XRCCTRL( *this, "VoiceTagFilterList",				wxCheckListBox );
	m_voicetagFilterOperatorRadio	= XRCCTRL( *this, "VoicetagFilterOperatorRadio",	wxRadioBox );

	m_exportTypeChoice->Clear();

	m_exportTypeChoice->Append( wxT( "Story scene" ), new wxStringClientData( wxT( "w2scene" ) ) );			// ET_StoryScene
	m_exportTypeChoice->Append( wxT( "Entity" ), new wxStringClientData( wxT( "w2ent" ) ) );				// ET_GameplayEntity
	m_exportTypeChoice->Append( wxT( "GUI" ), new wxStringClientData( wxT( "GUI" ) ) );						// ET_LosStrWithKeys
	m_exportTypeChoice->Append( wxT( "Scene for lipsync" ), new wxStringClientData( wxT( "w2scene" ) ) );	// ET_SceneForLipsync
	m_exportTypeChoice->Append( wxT( "Scene facts" ), new wxStringClientData( wxT( "w2scene" ) ) );			// ET_SceneFact
	m_exportTypeChoice->Append( wxT( "Scene Usage" ), new wxStringClientData( wxT( "w2quest" ) ) );			// ET_SceneUsage
	m_exportTypeChoice->Append( wxT( "Journal" ), new wxStringClientData( wxT( "journal" ) ) );				// ET_Journal
	m_exportTypeChoice->Append( wxT( "Loc Analyzer" ), new wxStringClientData( wxT( "w2scene" ) ) );		// ET_LocAnalyzer
	m_exportTypeChoice->Append( wxT( "Animations Reporter" ), new wxStringClientData( wxT( "w2scene" ) ) );	// ET_AnimationReporter
	m_exportTypeChoice->SetSelection( ET_StoryScene );
	m_currentExporterType = ET_StoryScene;

	m_emptyColumnsNumberSpin->SetValue( 0 );

	m_sourceLanguageChoice->Clear();
	m_targetLanguageList->Clear();


	TDynArray< String > textLanguages, speechLanguages;
	SLocalizationManager::GetInstance().GetAllAvailableLanguages( textLanguages, speechLanguages );

	for ( TDynArray< String >::const_iterator langIter = textLanguages.Begin(); 
		langIter != textLanguages.End(); ++langIter )
	{
		m_sourceLanguageChoice->Append( langIter->AsChar() );
		m_targetLanguageList->Append( langIter->AsChar() );
	}

	m_sourceLanguageChoice->SetSelection( 
		m_sourceLanguageChoice->FindString( SLocalizationManager::GetInstance().GetCurrentLocale().AsChar() ) );

	wxCommandEvent dummyEvent;
	OnExportFileTypeChanged( dummyEvent );
}

AbstractLocalizationExporter* CEdLocalizationTool::SetupExporter( const String& savePath, const String& errorsFilePath )
{
	ASSERT( m_exporter != NULL );
	
	String sourceLanguage = m_sourceLanguageChoice->GetStringSelection().wc_str();

	TDynArray< String > targetLanguages;
	for ( Uint32 i = 0; i < m_targetLanguageList->GetCount(); ++i )
	{
		if ( m_targetLanguageList->IsChecked( i ) == true )
		{
			targetLanguages.PushBack( m_targetLanguageList->GetString( i ).wc_str() );
		}
	}

	String targetLanguage = targetLanguages.Empty() == true ? String::EMPTY : targetLanguages[ 0 ];

	m_exporter->SetLanguages( sourceLanguage );
	m_exporter->SetTargetLanguages( targetLanguages );
	m_exporter->SetNumberOfTargetLanguages( targetLanguages.Size() + m_emptyColumnsNumberSpin->GetValue() );
	m_exporter->SetSavePath( savePath );
	m_exporter->SetErrorsPath( errorsFilePath );
	m_exporter->SetMinResourcePathDepth( m_minPathDepthSpin->GetValue() );
	m_exporter->SetMaxResourcePathDepth( m_maxPathDepthSpin->GetValue() );


	return m_exporter;
}

String CEdLocalizationTool::DetermineExportedResourceExtension()
{
	Int32 exportTypeSelection = m_exportTypeChoice->GetSelection();
	wxStringClientData* exportTypeStringData = static_cast< wxStringClientData* >( m_exportTypeChoice->GetClientObject( exportTypeSelection ) );
	String extension = exportTypeStringData->GetData().wc_str();
	return extension;
}

Bool CEdLocalizationTool::DetermineSavePath( String& savePath, String& errorsFilePath )
{
	if( m_exporter && m_exporter->ExportToDirectory() )
	{
		return DetermineSaveDirectoryPath( savePath, errorsFilePath );
	}
	else
	{
		return DetermineSaveFilePath( savePath, errorsFilePath );
	}
}

Bool CEdLocalizationTool::DetermineSaveDirectoryPath( String& savePath, String& errorsFilePath )
{
	wxDirDialog saveDirDialog( this, TXT( "Export to" ) );
	if ( saveDirDialog.ShowModal() != wxID_OK )
	{
		return false;
	}

	savePath = String( saveDirDialog.GetPath().wc_str() );
	errorsFilePath = savePath + TXT( "\\_errors.csv" );
	return true;
}

Bool CEdLocalizationTool::DetermineSaveFilePath( String& savePath, String& errorsFilePath )
{
	Red::System::DateTime time;
	Red::System::Clock::GetInstance().GetLocalTime( time );

	// + 1 so that the we start counting from "1" rather than "0"
	Int32 month = time.GetMonth() + 1;
	Int32 day = time.GetDay() + 1;

	wxTreeItemId ditTreeSelItemId = m_directoryTree->GetSelection();

	// Set default file name
	String defaultFilename;
	if ( ditTreeSelItemId.IsOk() )
	{
		String directoryName = m_directoryTree->GetItemText( m_directoryTree->GetSelection() ).wc_str();
		directoryName = directoryName.StringBefore( TXT( " " ) );

		defaultFilename = String::Printf
		(
			TXT( "%s_%s_export_%04d_%02d_%02d.csv" ),
			DetermineExportedResourceExtension().AsChar(),
			directoryName.AsChar(),
			time.GetYear(),
			month,
			day
		);
	}
	else
	{
		defaultFilename = String::Printf
		(
			TXT( "export_%04d_%02d_%02d.csv" ),
			time.GetYear(),
			month,
			day
		);
	}

	wxFileDialog saveFileDialog( this, TXT("Export as"), 
		String::EMPTY.AsChar(), defaultFilename.AsChar(), 
		TXT( "CSV Files (*.csv)|*.csv" ), wxFD_SAVE );

	if ( saveFileDialog.ShowModal() != wxID_OK )
	{
		return false;
	}

	savePath = String( saveFileDialog.GetPath().wc_str() );
	errorsFilePath = savePath;
	errorsFilePath.Replace( TXT( ".csv" ), TXT( "_errors.csv" ) );
	return true;
}

void CEdLocalizationTool::FillExportResourcesPaths( TDynArray< String > &exportResourcesPaths )
{
	if ( m_currentExporterType != ET_Journal )
	{
		for ( TSet< String >::iterator resPathI = m_selFilesToExp.Begin(); resPathI != m_selFilesToExp.End(); ++resPathI )
		{
			exportResourcesPaths.PushBack( *resPathI );
		}
		m_resourceSorter.SortResourcePaths( exportResourcesPaths );
	}
	else
	{
		CJournalLocalizationExporter* exporter = static_cast< CJournalLocalizationExporter* >( m_exporter );
		if ( !exporter )
		{
			return;
		}
		const THashMap< String, Int32 >& strings = exporter->GetJournalStrings();

		TSortedMap< Int32, String > orderedStrings;
		for ( TSet< String >::iterator resPathI = m_selFilesToExp.Begin(); resPathI != m_selFilesToExp.End(); ++resPathI )
		{
			const Int32* order = strings.FindPtr( *resPathI );
			if ( order )
			{
				orderedStrings.Insert( *order, *resPathI );
			}
			else
			{
				RED_HALT( "Missing string [%s]", resPathI->AsChar() );
			}
		}
		for ( TSortedMap< Int32, String >::iterator itString = orderedStrings.Begin(); itString != orderedStrings.End(); ++itString )
		{
			exportResourcesPaths.PushBack( itString->m_second );
		}
}
}
void CEdLocalizationTool::UpdateDirTreeItemName( DirectorySelectionItemWrapper* itemDataWrapper, wxTreeItemId selectedDirectoryId )
{
	Int32 selItemsRec = itemDataWrapper->m_selectedFiles + itemDataWrapper->m_selectedFilesRecursive;
	String itemText( String::Printf( TXT("%s : %d/%d"), itemDataWrapper->m_directory->GetName().AsChar(), selItemsRec,
		itemDataWrapper->m_allFilesRecursive ) );
	if ( itemDataWrapper->m_selectedFiles > 0 || selItemsRec > 0 )
	{
		m_directoryTree->SetItemText( selectedDirectoryId, itemText.AsChar() );
	}
	else
	{
		m_directoryTree->SetItemText( selectedDirectoryId, itemDataWrapper->m_directory->GetName().AsChar() );
	}
}

void CEdLocalizationTool::UpdateExportFileCheck( Int32 checkListBoxIndex )
{
	if ( m_currentExporterType == ET_LosStrWithKeys )
	{
		String selText = m_exportFileList->GetString( checkListBoxIndex ).wc_str();
		if ( m_exportFileList->IsChecked( checkListBoxIndex ) )
		{
			m_selFilesToExp.Insert( selText );
		}
		else
		{
			m_selFilesToExp.Erase( selText );
		}
	}
	else
	{
		// Add/Remove file path to the export file paths list

		String filename = m_exportFileList->GetString( checkListBoxIndex ).wc_str();
		String resourcePath;
		m_filepaths.Find( filename, resourcePath );
		if ( resourcePath.Empty() == true ) return;
		wxTreeItemId foundLeaf;

		wxTreeItemId selectedDirectoryId = m_directoryTree->GetSelection();
		ExportGroupSelectionItemWrapper* itemDataWrapper
			= static_cast< ExportGroupSelectionItemWrapper* >( m_directoryTree->GetItemData( selectedDirectoryId ) );


		Int32 count = 0;
		if ( m_exportFileList->IsChecked( checkListBoxIndex ) )
		{
			Bool success = m_selFilesToExp.Insert( resourcePath );
			ASSERT( success && TXT("Inserting existing resource path") );

			count = 1;
		}
		else
		{
			Bool success = m_selFilesToExp.Erase( resourcePath );
			ASSERT( success && TXT("Erasing non-existing resource path") );

			count = -1;
		}

		foundLeaf = UpdateTreeItem( selectedDirectoryId, resourcePath, count );

		// Update selected directory tree item name
		UpdateGroupTreeItemName( itemDataWrapper, selectedDirectoryId );

		UpdateTreeItemUpwards( foundLeaf, count );
	}
}

wxTreeItemId CEdLocalizationTool::UpdateTreeItem( const wxTreeItemId& root, const String& resourcePath, Int32 count )
{
	wxTreeItemId item = root, child;
	wxTreeItemIdValue cookie;

	while( item.IsOk() )
	{
		ExportGroupSelectionItemWrapper* itemDataWrapper = static_cast< ExportGroupSelectionItemWrapper* >( m_directoryTree->GetItemData( item ) );
		if ( itemDataWrapper->m_exportGroup->m_groupEntries.Exist( resourcePath ) == true )
		{
			itemDataWrapper->m_selectedFiles += count;
			UpdateGroupTreeItemName( itemDataWrapper, item );
			ASSERT( itemDataWrapper->m_selectedFiles >= 0 );
			return item; // Stop recursion
		}

		child = m_directoryTree->GetFirstChild( item, cookie );
		if ( child.IsOk() ) child = UpdateTreeItem( child, resourcePath, count );
		if ( child.IsOk() ) return child;
		item = m_directoryTree->GetNextSibling( item );
	}

	return item;
}

void CEdLocalizationTool::UpdateTreeItemUpwards( const wxTreeItemId& leaf, Int32 count )
{
	wxTreeItemId item = m_directoryTree->GetItemParent( leaf );
	if ( !item.IsOk() ) return;

	ExportGroupSelectionItemWrapper* itemDataWrapperParent = static_cast< ExportGroupSelectionItemWrapper* >( m_directoryTree->GetItemData( item ) );

	itemDataWrapperParent->m_selectedFilesRecursive += count;

	UpdateGroupTreeItemName( itemDataWrapperParent, item );

	UpdateTreeItemUpwards( item, count );
}

void CEdLocalizationTool::OnExportButtonClick( wxCommandEvent& event )
{
	TDynArray< String > exportResourcesPaths;
	String savePath;
	String errorsFilePath;

	FillExportResourcesPaths( exportResourcesPaths );

	if ( exportResourcesPaths.Empty() == true || DetermineSavePath( savePath, errorsFilePath ) == false)
	{
		return;
	}

	m_statusBar->SetRange( exportResourcesPaths.Size() );
	m_statusBar->SetValue( 0 );

	if( m_exporter )
	{
		m_exporter->ShowSetupDialog( this );
	}

	IBatchExporter* exporter = SetupExporter( savePath, errorsFilePath );
	RED_ASSERT( exporter );

	exporter->BeginBatchExport();

	Bool exportEntries = true;
	Bool validateResources = m_validateExportEntriesCheckbox->IsChecked();
	if( validateResources )
	{
		m_statusLabel->SetLabel( "Validating entries..." );
		Bool allResourcesValid = HandleResourceValidation( exportResourcesPaths );

		if( !allResourcesValid )
		{
			String question(TXT("Validation complete - some resources are invalid (see export log). Start export anyway?"));
			wxMessageDialog dlg( nullptr, question.AsChar(), "Validation complete", wxYES_NO | wxICON_QUESTION );
			exportEntries = ( dlg.ShowModal() == wxID_YES );
		}

		// reset status bar
		m_statusBar->SetValue( 0 );
	}

	if( exportEntries )
	{
		for ( Uint32 i = 0; i < exportResourcesPaths.Size(); ++i )
		{
			m_statusBar->SetValue( m_statusBar->GetValue() + 1 );
			m_statusLabel->SetLabel( wxString::Format( wxT("Exporting entry %s."), exportResourcesPaths[ i ].AsChar() ) );

			exporter->ExportBatchEntry( exportResourcesPaths[ i ] );
		}
	}

	exporter->EndBatchExport();

	m_statusBar->SetValue( exportResourcesPaths.Size() );
	m_statusLabel->SetLabel( wxT( "Done" ) );
}

void CEdLocalizationTool::OnDirectorySelected( wxTreeEvent& event )
{
	if ( m_blockDirectorySelectionEvent == true )
	{
		return;
	}

	String  resourceExtension = TXT( "." ) + DetermineExportedResourceExtension();

	wxTreeItemId selectedDirectoryId = m_directoryTree->GetSelection();

	ExportGroupSelectionItemWrapper* itemDataWrapper 
		= static_cast< ExportGroupSelectionItemWrapper* >( m_directoryTree->GetItemData( selectedDirectoryId ) );

	m_filepaths.Clear();
	m_exportFileList->Clear();

	if ( itemDataWrapper->m_exportGroup == NULL )
	{
		return;
	}

	ListExportGroupEntries( *(itemDataWrapper->m_exportGroup) );
}

void CEdLocalizationTool::OnRecursiveFilesChanged( wxCommandEvent& event )
{
	wxTreeEvent treeEvent;
	OnDirectorySelected( treeEvent );
}

void CEdLocalizationTool::OnToggleAllButton( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_exportFileList->GetCount(); ++i )
	{
		Bool isChecked = m_exportFileList->IsChecked( i ) == false;
		m_exportFileList->Check( i, isChecked );
		
		UpdateExportFileCheck( i );
	}
}

void CEdLocalizationTool::OnExportFileListCheckListBox( wxCommandEvent& event )
{
	Int32 i = event.GetInt();
	if ( i < 0 ) return;

	UpdateExportFileCheck( i );
}

void CEdLocalizationTool::OnFileSelected( wxFileDirPickerEvent& event )
{
	m_importFile = event.GetPath().wc_str();
}

void CEdLocalizationTool::OnImportButtonClick( wxCommandEvent& event )
{
	if( m_importFile == TXT("") )
	{
		GFeedback->ShowError( TXT( "Please choose file to be imported" ) );
		return;
	}
	
	

	C2dArray *csv = C2dArray::CreateFromString( m_importFile );

	if( !csv )
	{
		GFeedback->ShowError( TXT( "Unable to load selected file" ) );
		return;
	}

	Int32 idIndex = csv->GetColumnIndex( TXT( "ID" ) );
	Int32 textIndex = csv->GetColumnIndex( TXT( "TEXT" ) );
	
	Uint32 numberOfColumns;
	Uint32 numberOfRows;
	csv->GetSize( numberOfColumns, numberOfRows );

	if( ( idIndex < 0 ) /*|| ( textIndex < 0 )*/ || ( numberOfRows == 0 ) )
	{
		GFeedback->ShowError( TXT( "Wrong file format" ) );
		csv->Discard();
		return;
	}

	

	GFeedback->BeginTask( TXT( "Importing csv file" ), false );
	
	for ( Uint32 k = 1; k < numberOfColumns; ++k )
	{
		THashMap< Uint32, String > stringsToImport;

		for( Uint32 i = 0; i < numberOfRows; ++i )
		{
			String idText = csv->GetValue( 0, i );
			String text = csv->GetValue( k, i );

			Uint32 id;

			if ( idText.Empty() == true )
			{
				continue;
			}

			if ( !FromString( idText, id ) )
			{
				GFeedback->ShowError( TXT( "Unable to convert id to string at row %u" ), i );
				GFeedback->EndTask();
				csv->Discard();
				return;			
			}

			GFeedback->UpdateTaskProgress( i, numberOfRows );

			stringsToImport.Insert( id, text );
		}


		SLocalizationManager::GetInstance().UpdateStringDatabase( stringsToImport, csv->GetHeader( k ) );
	}

	GFeedback->EndTask();

	csv->Discard();

}

void CEdLocalizationTool::OnOpenPresetButton( wxCommandEvent& event )
{
	String loadPath;
	wxFileDialog loadFileDialog( this, TXT("Load Preset"), 
		String::EMPTY.AsChar(), String::EMPTY.AsChar(), 
		TXT( "Localization Preset Files (*.lpf)|*.lpf" ), wxFD_OPEN );

	if ( loadFileDialog.ShowModal() == wxID_OK )
	{
		loadPath = String( loadFileDialog.GetPath().wc_str() );
	}
	else 
	{
		return;
	}

	IFile* presetReader = GFileManager->CreateFileReader( loadPath, FOF_AbsolutePath );
	if ( presetReader != NULL )
	{
		TDynArray< String > selectedFilesArray;
		*presetReader << selectedFilesArray;
		
		m_selFilesToExp.Clear();

		for ( TDynArray< String >::iterator fileIter = selectedFilesArray.Begin(); 
			fileIter != selectedFilesArray.End(); ++fileIter )
		{
			m_selFilesToExp.Insert( *fileIter );
		}

		m_exportFileList->Clear();
		

		m_blockDirectorySelectionEvent = true;

		m_directoryTree->DeleteAllItems();

		//if ( m_rootBatchExportGroup != NULL )
		//{
		//	m_rootBatchExportGroup->Clear();
		//	delete m_rootBatchExportGroup;
		//}
		//m_rootBatchExportGroup = new BatchExportGroup();
		
		//m_exporter->GetTopExportGroup( *m_rootBatchExportGroup );
		AddExportGroupToTree( *(m_exporter)->GetRootExportGroup(), 0 );

		m_blockDirectorySelectionEvent = false;

		delete presetReader;
	}
}

void CEdLocalizationTool::OnSavePresetButton( wxCommandEvent& event )
{

	String defaultFilename = TXT( "export_preset.lpf" );

	String savePath;
	wxFileDialog saveFileDialog( this, TXT("Save Preset As"), 
		String::EMPTY.AsChar(), defaultFilename.AsChar(), 
		TXT( "Localization Preset Files (*.lpf)|*.lpf" ), wxFD_SAVE );

	if ( saveFileDialog.ShowModal() == wxID_OK )
	{
		savePath = String( saveFileDialog.GetPath().wc_str() );
	}
	else 
	{
		return;
	}



	IFile* presetWriter = GFileManager->CreateFileWriter( savePath, FOF_AbsolutePath );
	if ( presetWriter != NULL )
	{
		TDynArray< String > selectedFilesArray;
		for ( TSet< String >::iterator fileIter = m_selFilesToExp.Begin(); 
			fileIter != m_selFilesToExp.End(); ++fileIter )
		{
			selectedFilesArray.PushBack( *fileIter );
		}

		*presetWriter << selectedFilesArray;
		delete presetWriter;
	}

}

void CEdLocalizationTool::OnSelectAllButton( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_exportFileList->GetCount(); ++i )
	{
		if ( m_exportFileList->IsChecked( i ) == true )
		{
			continue;
		}

		m_exportFileList->Check( i, true );

		UpdateExportFileCheck( i );
	}
}

void CEdLocalizationTool::OnDeselectAllButton( wxCommandEvent& event )
{
	for ( Uint32 i = 0; i < m_exportFileList->GetCount(); ++i )
	{
		if ( m_exportFileList->IsChecked( i ) == false )
		{
			continue;
		}

		m_exportFileList->Check( i, false );

		UpdateExportFileCheck( i );
	}
}

void CEdLocalizationTool::OnExportFileTypeChanged( wxCommandEvent& event )
{
	// Set current exporter type
	CreateExporter( (EExporterType) m_exportTypeChoice->GetSelection() );

	m_selFilesToExp.Clear();
	m_exportFileList->Clear();

	m_blockDirectorySelectionEvent = true;

	m_directoryTree->DeleteAllItems();
	m_directoryTree->Disable();

	AddExportGroupToTree( *( m_exporter->GetRootExportGroup() ), 0 );

	m_blockDirectorySelectionEvent = false;

	m_directoryTree->Enable();
}

AbstractLocalizationExporter* CEdLocalizationTool::CreateExporter( EExporterType exporterType )
{
	if ( m_exporter != NULL )
	{
		delete m_exporter;
		m_exporter = NULL;
	}
	
	Int32 options = LEAC_None;

	if ( m_cutsceneDescriptionsCheckbox->IsChecked() == true )
	{
		options |= LEAC_CSDescription;
	}
	if ( m_onlyCutscenesCheckbox->IsChecked() == true )
	{
		options |= LEAC_OnlyCutscenes;
	}

	if ( exporterType == ET_StoryScene )
	{
		CStorySceneLocalizationExporter* sceneExporter = new CStorySceneLocalizationExporter( options );
		sceneExporter->SetMarkVoicetags( m_markImportantVoicetagsCheckbox->IsChecked() );
		m_exporter = sceneExporter;
	}
	else if ( exporterType == ET_GameplayEntity )
	{
		m_exporter = new CGameplayEntityLocalizationExporter();
	}
	else if ( exporterType == ET_LosStrWithKeys )
	{
		m_exporter = new CLocStringsWithKeysLocalizationExporter();
	}
	else if ( exporterType == ET_SceneForLipsync )
	{
		m_exporter = new CLipsyncDataSceneExporter();
	}
	else if ( exporterType == ET_SceneFact )
	{
		m_exporter = new CSceneFactsExporter();
	}
	else if ( exporterType == ET_SceneUsage )
	{
		m_exporter = new CSceneUsageExporter();
	}
	else if ( exporterType == ET_Journal )
	{
		m_exporter = new CJournalLocalizationExporter();
	}
	else if ( exporterType == ET_LocAnalyzer )
	{
		m_exporter = new CEdSceneLocAnalyser();
	}
	else if ( exporterType == ET_AnimationReporter )
	{
		m_exporter = new CEdSceneAnimationReporter();
	}
	else
	{
		HALT( "Unknown exporter type selected" );
		ERR_EDITOR( TXT( "Localization Tool: Unknown exporter type selected" ) );
		return NULL;
	}

	m_currentExporterType = exporterType;
	ASSERT( m_exporter != NULL );

	return m_exporter;
}

void CEdLocalizationTool::AddExportGroupToTree( BatchExportGroup& exportGroup, wxTreeItemId parentItem )
{
	wxTreeItemId groupTreeId = 0;
	Sort( exportGroup.m_subGroups.Begin(), exportGroup.m_subGroups.End(), BatchExportGroup::Compare );

	ExportGroupSelectionItemWrapper* groupWrapper = new ExportGroupSelectionItemWrapper( &exportGroup );

	if ( !parentItem.IsOk() )
	{
		groupTreeId = m_directoryTree->AddRoot( exportGroup.m_groupName.AsChar(), 0, -1, groupWrapper );
	}
	else
	{
		groupTreeId = m_directoryTree->AppendItem( parentItem, exportGroup.m_groupName.AsChar(), 1, -1, groupWrapper );
	}

	if ( groupTreeId.IsOk() == false )
	{
		return;
	}

	for ( Uint32 i = 0; i < exportGroup.m_subGroups.Size(); ++i )
	{
		AddExportGroupToTree( exportGroup.m_subGroups[ i ], groupTreeId );
	}

	// Count possible files to export
	String  resourceExtension = TXT( "." ) + DetermineExportedResourceExtension();
	const TDynArray< String > & groupFiles = exportGroup.m_groupEntries;
	for ( TDynArray< String >::const_iterator fileIter = groupFiles.Begin();
		fileIter != groupFiles.End(); ++fileIter )
	{
		String filename = (*fileIter);

		groupWrapper->m_allFilesRecursive += 1;

		if ( m_selFilesToExp.Find( filename ) != m_selFilesToExp.End() )
		{
			groupWrapper->m_selectedFiles += 1;
		}
	}

	UpdateGroupTreeItemName( groupWrapper, groupTreeId );

	if ( parentItem.IsOk() == true )
	{
		ExportGroupSelectionItemWrapper* parentDirectoryWrapper
			= static_cast< ExportGroupSelectionItemWrapper* >( m_directoryTree->GetItemData( parentItem ) );
		parentDirectoryWrapper->m_allFilesRecursive += groupWrapper->m_allFilesRecursive;
		parentDirectoryWrapper->m_selectedFilesRecursive += groupWrapper->m_selectedFiles + groupWrapper->m_selectedFilesRecursive;
	}
}

void CEdLocalizationTool::ListExportGroupEntries( BatchExportGroup& exportGroup )
{
	const TDynArray< String >& groupFiles = exportGroup.m_groupEntries;
	for ( TDynArray< String >::const_iterator entryIter = groupFiles.Begin();
		entryIter != groupFiles.End(); ++entryIter )
	{
		String entry = (*entryIter);

		// NOTE: Be careful with this one - this was recursive
		String displayedEntryName = entry.StringAfter( TXT( "\\" ), true );
		if ( displayedEntryName.Empty() == true )
		{
			displayedEntryName = entry;
		}

		if ( m_filepaths.Find( displayedEntryName ) != m_filepaths.End() )
		{
			displayedEntryName += String::Printf( TXT( " @ %s" ), entry.StringBefore( TXT( "\\" ) ).AsChar() );
		}

		int currCheckListBoxIndex = m_exportFileList->Append( displayedEntryName.AsChar() );

		// Set checked state for files list box item
		if ( m_selFilesToExp.Find( entry ) != m_selFilesToExp.End() )
		{
			m_exportFileList->Check( currCheckListBoxIndex );
		}


		m_filepaths.Insert( displayedEntryName, entry );
	}

	if ( m_recursiveFilesCheckbox->IsChecked() == false )
	{
		return;
	}

	for ( Uint32 i = 0; i < exportGroup.m_subGroups.Size(); ++i )
	{
		ListExportGroupEntries( exportGroup.m_subGroups[ i ] );
	}
}

void CEdLocalizationTool::UpdateGroupTreeItemName( ExportGroupSelectionItemWrapper* itemDataWrapper, wxTreeItemId selectedGroupId )
{
	Int32 selItemsRec = itemDataWrapper->m_selectedFiles + itemDataWrapper->m_selectedFilesRecursive;
	String itemText( String::Printf( TXT("%s : %d/%d"), itemDataWrapper->m_exportGroup->m_groupName.AsChar(), selItemsRec,
		itemDataWrapper->m_allFilesRecursive ) );
	if ( itemDataWrapper->m_selectedFiles > 0 || selItemsRec > 0 )
	{
		m_directoryTree->SetItemText( selectedGroupId, itemText.AsChar() );
	}
	else
	{
		m_directoryTree->SetItemText( selectedGroupId, itemDataWrapper->m_exportGroup->m_groupName.AsChar() );
	}
}

/*
Checks if resources are valid and possibly validates them.

\param exportResourcesPaths List of resources to check/validate.
\return True - all resources are valid (this includes invalid resources that were successfully validated).
False - at least one resource is invalid (it couldn't be validated or the user refused to validate it).
*/
Bool CEdLocalizationTool::HandleResourceValidation( const TDynArray< String >& exportResourcesPaths )
{
	Bool allResourcesValid = true;

	for( Uint32 i = 0, numResources = exportResourcesPaths.Size(); i < numResources; ++i )
	{
		const String& entry = exportResourcesPaths[ i ];

		m_statusBar->SetValue( m_statusBar->GetValue() + 1 );
		m_statusLabel->SetLabel( wxString::Format( wxT("Checking validity - entry %s."), entry.AsChar() ) );

		Bool entryValid = m_exporter->IsBatchEntryValid( entry );
		if( !entryValid )
		{
			// ask the user whether exporter should validate the entry
			String validateQuestion = TXT( "Entry " ) + entry + TXT( " is not valid. Do you want to validate it?" );
			wxMessageDialog validateDialog( 0, validateQuestion.AsChar(), "Question", wxYES_NO | wxICON_QUESTION );
			Int32 validateAnswer = validateDialog.ShowModal();

			if( validateAnswer == wxID_YES )
			{
				Bool validated = m_exporter->ValidateBatchEntry( entry );
				if( !validated )
				{
					m_exporter->PushBatchEntryError( entry, TXT( "Entry is invalid and it couldn't be validated." ) );
					allResourcesValid = false;
				}
			}
			else
			{
				m_exporter->PushBatchEntryError( entry, TXT( "Entry needs validation but was not validated due to export options or user choice." ) );
				allResourcesValid = false;
			}
		}
	}

	return allResourcesValid;
}


