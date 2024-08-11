/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "sceneRecorder.h"
#include "sceneExplorer.h"
#include "dialogEditor.h"
#include "dialogPreview.h"
#include "dialogEditorMainControlPanel.h"

#include "../../common/core/2darray.h"
#include "../../common/core/depot.h"

#include "../../common/engine/renderer.h"
#include "../../common/engine/renderCommands.h"
#include "../../common/engine/localizationManager.h"
#include "../../common/engine/environmentManager.h"

#include "../../common/game/storySceneSection.h"
#include "../../common/game/storySceneInput.h"
#include "../../common/game/storySceneCutsceneSection.h"
#include "../../common/game/storySceneVideo.h"
#include "../../common/game/storySceneControlPartsUtil.h"

#include "../../common/renderer/renderSequenceGrabber.h"

#include <shellapi.h>

#include "loopbackRecorder.h"

// there may be some hacks ahead...

CSceneRecorder::Config::Config()
	: width( 800 )
	, height( 600 )
	, useUbersample( false )
	, deleteScreenshots( true )
	, sectionName( String::EMPTY )
	, audioSampleCount( 1 )
	, skipVideoRecording( false )
{
}

CSceneRecorder::CSceneRecorder(void)
	: m_state( SRPS_None )
	, m_mode( SRRM_None )
	, m_config( Config() )
	, m_audioRecorder( nullptr )
	, m_currentSection( nullptr )
{
	if ( GFileManager )
	{
		LIBAV_DIR = GFileManager->GetBaseDirectory() + TXT("tools\\libav\\bin");
	}

	SEvents::GetInstance().RegisterListener( RED_NAME( EditorTick ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( DialogEditorDestroyed ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( SectionChanged ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( SectionFinished ), this );
	SEvents::GetInstance().RegisterListener( RED_NAME( AudioTrackStarted ), this );
}

CSceneRecorder::~CSceneRecorder(void)
{
	SEvents::GetInstance().UnregisterListener( this );
	if ( m_audioRecorder )
	{
		delete m_audioRecorder;
	}

	if ( m_scenesMetadata )
	{
		m_scenesMetadata->RemoveFromRootSet();
		if ( m_scenesMetadata->GetFile() && m_scenesMetadata->GetFile()->IsLoaded() )
		{
			m_scenesMetadata->GetFile()->Unload();
		}

		m_scenesMetadata = nullptr;
	}
}

void CSceneRecorder::LoadMetadataFile( const String& fileName )
{
	CDiskFile* scenesMetadataFile = GDepot->FindFile( fileName.AsChar() );
	if ( scenesMetadataFile )
	{
		LOG_RECORDER( TXT("Found previous recording metadata file. Not modified scenes will be skipped.") );
		if ( !scenesMetadataFile->IsLoaded() )
		{
			scenesMetadataFile->Load();
		}
		m_scenesMetadata = Cast< C2dArray >( scenesMetadataFile->GetResource() );
		if ( m_scenesMetadata == nullptr )
		{
			WARN_RECORDER( TXT("Can't load data from metadata file: %s. Unmodified files will be recorded."), scenesMetadataFile->GetFileName().AsChar() );
		}
	}
	else
	{
		CResource::FactoryInfo< C2dArray > info;
		m_scenesMetadata = info.CreateResource();
		m_scenesMetadata->AddColumn( TXT("Scene_section"), TXT("") );
		m_scenesMetadata->AddColumn( TXT("Scene_time"), TXT("") );
	}
	m_scenesMetadata->AddToRootSet();
}

Bool CSceneRecorder::LoadFromConfigFile( const String& path )
{
	LOG_RECORDER( TXT("Loading scenes from config file: %s"), path.AsChar() );
	size_t charPosition;
	path.FindCharacter( TXT('\\'), charPosition, true );
	String configFileName = path.RightString( path.Size() - charPosition - 2 );
	String destPath = GFileManager->GetDataDirectory() + configFileName;
	GFileManager->CopyFile( path, destPath, true );
	GDepot->Repopulate( false );
	CDiskFile* scenesToRecordFile = GDepot->FindFile( configFileName );

	if ( scenesToRecordFile == nullptr )
	{
		ERR_RECORDER( TXT("Can't find config file: %s"), path.AsChar() );
		return false;
	}
	else
	{
		LOG_RECORDER( TXT("Successfuly loaded config file.") );
		scenesToRecordFile->Load();

		C2dArray* scenesToRecordList = Cast< C2dArray >( scenesToRecordFile->GetResource() );
		if ( scenesToRecordList == nullptr )
		{
			ERR_RECORDER( TXT("Can't load data from config file: %s"), path.AsChar() );
			return false;
		}

		for ( Uint32 rowNum = 0; rowNum < scenesToRecordList->GetNumberOfRows(); rowNum++ )
		{
			SceneInfo sceneInfo;
			sceneInfo.m_scenePath = scenesToRecordList->GetValue( TXT("Scene"), rowNum );

			if ( sceneInfo.m_scenePath == String::EMPTY )
			{
				WARN_RECORDER( TXT("No scene path specified in row: %d. Skipping row."), rowNum );
				continue;
			}

			Uint32 layerColNum = 0;
			String layerName;
			do
			{
				layerName = scenesToRecordList->GetValue( String::Printf( TXT("Exclude%d"), layerColNum ), rowNum );
				if ( layerName != String::EMPTY )
				{
					LOG_RECORDER( TXT("Layer to be excluded: "), layerName );
					sceneInfo.m_layerNames.PushBack( layerName );
				}
				++layerColNum;
			}
			while ( layerName != String::EMPTY );

			LOG_RECORDER( TXT("Scene to be recorded: %s"), sceneInfo.m_scenePath.AsChar() );
			m_recordingInfo.m_scenesToRecord.PushBack( sceneInfo );
		}
		scenesToRecordFile->Unload();
	}
	return true;
}

void CSceneRecorder::LoadModifiedFromDir( const String& path, const String& scenePattern )
{
	LOG_RECORDER( TXT("Loading scenes from root path: "), path.AsChar() );
	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	TDynArray< String > absoluteFilePaths;
	GFileManager->FindFiles( depotPath + path, scenePattern, absoluteFilePaths, true );
	LOG_RECORDER( TXT("Found %d scene(s)"), absoluteFilePaths.Size() );

	for ( String& scenePath : absoluteFilePaths )
	{
		String depotPath;
		GDepot->ConvertToLocalPath( scenePath, depotPath );
		CDiskFile* sceneFile = GDepot->FindFile( depotPath );
		if ( sceneFile == nullptr )
		{
			WARN_RECORDER( TXT("Couldn't find scene file: %s, skipping scene."), depotPath.AsChar() );
			continue;
		}
		SceneInfo sceneInfo;
		sceneInfo.m_scenePath = depotPath;
		m_recordingInfo.m_scenesToRecord.PushBack( sceneInfo );
	}
}

Bool CSceneRecorder::ParseCommandline( const THashMap< String, String >& commandLine )
{
	String path = String::EMPTY;
	if ( commandLine.Find( TXT("file"), path ) )
	{
		if ( !LoadFromConfigFile( path ) )
		{
			return false;
		}
	}
	else if( commandLine.Find( TXT("scenesRootDir"), path ) )
	{
		String scenePattern = TXT("*.w2scene");
		String sectionPattern = TXT("*");
		commandLine.Find( TXT("scene"), scenePattern );
		LoadModifiedFromDir( path, scenePattern );
	}
	else
	{
		ERR_RECORDER( TXT("No config file specified with -file=<path-to-config-file>, or scenes root dir with -scenesRootDir=<path-to-dir>") );
		return false;
	}

	LoadMetadataFile( TXT("scenes_metadata.csv") );

	String value = String::EMPTY;
	if ( ! ( commandLine.Find( TXT("world"), m_recordingInfo.m_worldPath ) ) )
	{
		ERR_RECORDER( TXT("No world path specified with '-world=<relative-path-from-data-dir>'") );
		return false;
	}
	if ( ! ( commandLine.Find( TXT("outDir"), m_config.outputDirPath ) ) )
	{
		m_config.outputDirPath = GFileManager->GetBaseDirectory() + TXT("screenshots\\");
	}
	m_config.outputDirPath.RemoveWhiteSpaces();
	if ( !GFileManager->CreatePath( m_config.outputDirPath ) )
	{
		ERR_RECORDER( TXT("Can't create output path: %s"), m_config.outputDirPath );
		return false;
	}

	commandLine.Find( TXT("section"), m_config.sectionName );
	if ( commandLine.Find( TXT("samples"), value ) )
		FromString( value, m_config.audioSampleCount );
	if ( commandLine.Find( TXT("width"), value ) )
		FromString( value, m_config.width );
	if ( commandLine.Find( TXT("height"), value ) )
		FromString( value, m_config.height );
	if ( commandLine.Find( TXT("ubersample"), value ) )
		FromString( value, m_config.useUbersample );
	if ( commandLine.Find( TXT("deleteSS"), value ) )
		FromString( value, m_config.deleteScreenshots );
	if ( commandLine.Find( TXT("skipVideo"), value ) )
		FromString( value, m_config.skipVideoRecording );

	return true;
}

void CSceneRecorder::OnStart()
{
	SetupEnvironment();

	m_state = SRPS_WaitingForWorld;
}

void CSceneRecorder::SetupEnvironment()
{
	LOG_RECORDER( TXT("Setting up environment...") );
	LOG_RECORDER( TXT("Setting client size to: %d width, %d height."), m_config.width, m_config.height );
	wxTheFrame->GetWorldEditPanel()->SetClientSize( m_config.width, m_config.height );
	wxTheFrame->GetWorldEditPanel()->GetViewport()->AdjustSize( m_config.width, m_config.height );
	
	if ( m_config.useUbersample )
	{
		GRender->RequestResizeRenderSurfaces( m_config.width, m_config.height );
	}

	// ensure shaders are not compiled during scene
	GRender->SetAsyncCompilationMode( false );

	// we don't want any assert pop-ups requiring user reaction
	wxTheAssertHandler = NULL;

	// clear viewport from debug text
	TDynArray< EShowFlags > flags;
	wxTheFrame->GetDebugFlags( flags );
	for( EShowFlags flag : flags )
	{
		GGame->GetViewport()->ClearRenderingMask( flag );
	}
	GGame->GetViewport()->SetRenderingMask( SHOW_Scenes );

	// set cinematic mode to ensure full texture resolution
	( new CRenderCommand_ToggleCinematicMode( true ) )->Commit();
	GRender->Flush();

	SLocalizationManager::GetInstance().SetCurrentLocale( TXT("EN") );
	SEvents::GetInstance().QueueEvent( CNAME( CurrentLocaleChanged ), NULL ); 

	LOG_RECORDER( TXT("Loading world %s with all layers."), m_recordingInfo.m_worldPath.AsChar() );
	if ( !LoadWorldWithAllLayers( m_recordingInfo.m_worldPath ) )
	{
		ERR_EDITOR( TXT("Couldn't load world: %s."), m_recordingInfo.m_worldPath );
	}

	LOG_RECORDER( TXT("Setting fake day cycle hour to 12:00.") );
	CEnvironmentManager* envManager =  GGame->GetActiveWorld()->GetEnvironmentManager();
	CGameEnvironmentParams params = envManager->GetGameEnvironmentParams();
	params.m_dayCycleOverride.m_fakeDayCycleEnable = true;
	params.m_dayCycleOverride.m_fakeDayCycleHour = 12.0f;
	envManager->SetGameEnvironmentParams( params );

	GGame->DisablePausingOnApplicationIdle( CGame::DISABLE_AUTOPAUSE_Cutscene );
	GGame->EnableForcedPrefetch( true );
}

Bool CSceneRecorder::LoadWorldWithAllLayers( const String& worldPath )
{
	 if ( !wxTheFrame->OpenWorld( worldPath ) )
	 {
		 return false;
	 }

	 CEdSceneExplorer* sceneExplorer = wxTheFrame->GetSceneExplorer();
	 if ( sceneExplorer == nullptr )
	 {
		 return false;
	 }

	 sceneExplorer->LoadAllLayers( true );

	 LOG_RECORDER( TXT("Unloading 'helpers' layer group.") );
	 for ( CLayerGroup* layerGroup : GGame->GetActiveWorld()->GetWorldLayers()->GetSubGroups() )
	 {
		 if ( layerGroup->GetName() == TXT("helpers") )
		 {
			 if ( !layerGroup->SyncUnload() )
			 {
				 WARN_RECORDER( TXT("Couldn't unload 'helpers' layer group") );
			 }
		 }
	 }

	 return true;
}

void CSceneRecorder::UnloadExcludedLayers( TDynArray< String >& layerNames )
{
	TDynArray< CLayerInfo* > allLayers;
	GGame->GetActiveWorld()->GetWorldLayers()->GetLayers( allLayers, true );

	for ( CLayerInfo* layer : allLayers )
	{
		for ( String name : layerNames )
		{
			if ( name == layer->GetShortName() )
			{
				layerNames.Remove( name );
				if ( !layer->SyncUnload() )
				{
					WARN_RECORDER( TXT("Couldn't unload excluded layer: %s"), layer->GetShortName() );
				}
				else
				{
					LOG_RECORDER( TXT("Successfuly unloaded excluded layer: %s"), layer->GetShortName() );
				}
				
				break;
			}
		}
	}
}

Bool CSceneRecorder::WasRecentlyRecorded( const CStorySceneSection* section )
{
	if ( section == nullptr )
	{
		return true;
	}

	CDiskFile* sceneFile = m_currentSceneFile;
	if ( sceneFile == nullptr )
	{
		return true;
	}

	C2dArray* scenesMetadata = m_scenesMetadata;
	if ( scenesMetadata == nullptr )
	{
		return true;
	}

	String sectionName = section->GetName();
	sectionName.RemoveWhiteSpaces();

	Red::System::DateTime lastTimeModified;
	Red::System::DateTime currentTime = sceneFile->GetFileTime();
	currentTime.SetTimeRaw( currentTime.GetTimeRaw() - currentTime.GetMilliSeconds() );
	Bool hasEntry = FromString( scenesMetadata->GetValue( TXT("Scene_section"), sectionName, TXT("Scene_time") ), lastTimeModified );
	if ( currentTime != lastTimeModified )
	{
		LOG_RECORDER( TXT("Scene section %s will be recorded."), sectionName.AsChar() );
		if ( hasEntry )
		{
			scenesMetadata->DeleteRow( scenesMetadata->GetRowIndex( TXT("Scene_time"), sectionName ) );
		}
		return false;
	}
	return true;
}

Bool CSceneRecorder::ShouldExcludeSection( const CStorySceneSection* section )
{
	String sectionName = section->GetName();
	sectionName.RemoveWhiteSpaces();

	return	WasRecentlyRecorded( section )
		||	section->IsGameplay()						// gameplay sections require gameplay context
		||	section->IsA< CStorySceneVideoSection >()	// no need to record prerecorded sections
		||	section->GetName().Empty()					// these are just junk disconnected from graph
		||	section->GetNumberOfElements() == 0
		|| (section->GetNextElement() == nullptr && !section->GetChoice() )
		||	section->GetEventsFromAllVariants().Empty()
		||  !m_config.sectionName.Empty() && m_config.sectionName != sectionName;
}

CStorySceneSection* CSceneRecorder::GetNextSection()
{
	if ( m_currentSceneSections.Empty() )
	{
		while ( !m_recordingInfo.m_scenesToRecord.Empty() && m_currentSceneSections.Empty() )
		{
			SceneInfo nextScene = m_recordingInfo.m_scenesToRecord.Front();
			LOG_RECORDER( TXT("Getting next scene to record: %s"), nextScene.m_scenePath.AsChar() );
			m_recordingInfo.m_scenesToRecord.RemoveAt(0);

			LOG_RECORDER( TXT("Unloading excluded layers...") );
			UnloadExcludedLayers( nextScene.m_layerNames );

			CDiskFile* file = GDepot->FindFile( nextScene.m_scenePath );
			if ( file == nullptr )
			{
				WARN_RECORDER( TXT("Couldn't find scene file: %s, skipping scene."), nextScene.m_scenePath.AsChar() );
				continue;
			}

			m_currentSceneFile = file;
			file->Load();
			CStoryScene* scene = Cast< CStoryScene >( file->GetResource() );

			for( Uint32 i = 0; i < scene->GetNumberOfSections(); ++i )
			{
				CStorySceneSection* sceneSection = scene->GetSection(i);
				if ( !ShouldExcludeSection( sceneSection ) )
				{
					LOG_RECORDER( TXT("Adding section %s to recording queue."), sceneSection->GetName().AsChar() );
					m_currentSceneSections.PushBack( scene->GetSection(i) );
				}
			}

			if ( !m_currentSceneSections.Empty() )
			{
				m_editor = new CEdSceneEditor( nullptr, scene );
			}
			else
			{
				file->Unload();
			}
		}
	}

	if ( !m_currentSceneSections.Empty() )
	{
		return m_currentSceneSections.PopBack();
	}
	else
	{
		return nullptr;
	}
}

void CSceneRecorder::RecordNextSection()
{
	CStorySceneSection* section = GetNextSection();

 	if ( section )
	{
		LOG_RECORDER( TXT("Requesting next section %s in scene editor."), section->GetName().AsChar() );
		// request section change to next scene section, it will be loaded when editor is ticked
		m_editor->RequestSection( section );
		m_currentSection = section;
		m_currentSampleNumber = m_config.audioSampleCount;
		m_state = SRPS_ChangingSection;
	}
	else
	{
		LOG_RECORDER( TXT("Recording finished, no more sections to record.") );

		if ( m_scenesMetadata && m_scenesMetadata->GetFile() )
		{
			LOG_RECORDER( TXT("Deleting metadata file.") );
			DeleteFile( m_scenesMetadata->GetFile()->GetAbsolutePath().AsChar() );
		}

		GEngine->RequestExit();
	}
}

Bool ExecBatchFile( const String& batchName, const String& batchDir, String& params )
{
	LOG_RECORDER( TXT("Executing batch file: %s, in %s, with params: %s"), batchName.AsChar(), batchDir.AsChar(), params.AsChar() );
	SHELLEXECUTEINFO execInfo = {0};
	execInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	execInfo.nShow = SW_SHOW;
	execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	execInfo.lpFile = batchName.AsChar();
	execInfo.lpParameters = params.AsChar();
	execInfo.lpDirectory = batchDir.AsChar();
	execInfo.hInstApp = NULL;
	execInfo.lpVerb = NULL;
	execInfo.hwnd = NULL;

	ShellExecuteEx( &execInfo );
	WaitForSingleObject( execInfo.hProcess, INFINITE );

	DWORD exitCode;
	GetExitCodeProcess( execInfo.hProcess, &exitCode );
	CloseHandle( execInfo.hProcess );

	return exitCode != 0;
}

void CSceneRecorder::ConvertSequenceToVideo( 
		const String& outputFile, const String& outputPath, const String& duration,
		const String& audioDuration, const String& audioTrimOffset, const Uint32& sampleNumber
	)
{
	String params = String::Printf( TXT("%s %s %s %s %s"), outputFile.AsChar(), outputPath.AsChar(), duration.AsChar(), audioDuration.AsChar(), audioTrimOffset.AsChar() );
	params += TXT(" ") + ToString( sampleNumber );

	if ( !ExecBatchFile( TXT("conv_to_video.bat"), LIBAV_DIR, params ) )
	{
		ERR_RECORDER( TXT("Failed executing 'conv_to_video.bat' for %s"), outputFile.AsChar() );
	}
}

void CSceneRecorder::MergeSectionsToScenePath( TDynArray< String >& sectionsNames, String& outputPath, String& sceneName )
{
	outputPath.RemoveWhiteSpaces();
	sceneName.RemoveWhiteSpaces();
	String params = TXT("\"");
	for ( Uint32 i = 0; i < sectionsNames.Size(); ++i )
	{
		String& name = sectionsNames[i];
		name.RemoveWhiteSpaces();
		params += outputPath + TXT("\\") + name + TXT(".avi|");
	}
	params.PopBack();
	params += TXT("\" ") + sceneName + TXT(" ") + outputPath;

	if ( !ExecBatchFile( TXT("merge.bat"), LIBAV_DIR, params ) )
	{
		ERR_RECORDER( TXT("Failed executing 'merge.bat' for %s"), sceneName );
	}
}

void CSceneRecorder::FindOutputs( const CStorySceneLinkElement* currentElement, 
								  TDynArray< const CStorySceneLinkElement* >& visited, 
								  THashSet< const CStorySceneOutput* >& usedOutputs,
								  TDynArray< TDynArray< String > >& foundPaths )
{
	if ( currentElement->IsA< CStorySceneOutput >() && !usedOutputs.Exist( Cast< CStorySceneOutput >( currentElement ) ) )
	{
		TDynArray< String > pathNames;
		for ( const CStorySceneLinkElement* element : visited )
		{
			const CStorySceneSection* sectionElement = Cast< CStorySceneSection >( element );
			if ( sectionElement && !ShouldExcludeSection( sectionElement ) )
			{
				pathNames.PushBack( sectionElement->GetName() );
			}
		}
		if ( !pathNames.Empty() )
		{
			foundPaths.PushBack( pathNames );
		}
		usedOutputs.Insert( Cast< CStorySceneOutput >( currentElement ) );
		return;
	}

	TDynArray< const CStorySceneLinkElement* > nextElements;
	StorySceneControlPartUtils::GetNextConnectedElements( currentElement, nextElements );

	for ( const CStorySceneLinkElement* element : nextElements )
	{
		if ( !visited.Exist( element ) )
		{
			visited.PushBack( element );
			FindOutputs( element, visited, usedOutputs, foundPaths );
			visited.PopBack();
		}
	}
}

void CSceneRecorder::AssembleScenePaths( CStoryScene* currentScene )
{
	TDynArray< CStorySceneInput* > inputs;
	currentScene->CollectControlParts< CStorySceneInput >( inputs );

	TDynArray< TDynArray< String > > paths;
	for ( CStorySceneInput* input : inputs )
	{
		TDynArray< const CStorySceneLinkElement* > visited;
		THashSet< const CStorySceneOutput* > usedOutputs;
		FindOutputs( Cast< CStorySceneLinkElement >( input ), visited, usedOutputs, paths );
		usedOutputs.Clear();
	}

	for ( TDynArray< String >& path : paths )
	{
		String outputName = path.Front() + TXT("-") + path.Back();
		MergeSectionsToScenePath( path, m_config.outputDirPath, outputName );
	}
}

void CSceneRecorder::DispatchEditorEvent( const CName& name, IEdEventData* data  )
{
	if ( name == RED_NAME( EditorTick ) )
	{
		if ( m_state == SRPS_WaitingForWorld )
		{
			LOG_RECORDER( TXT("Finished loading world.") );
 			RecordNextSection();
		}
		else if ( m_state == SRPS_WaitingForSection )
		{
			LOG_RECORDER( TXT("Finished loading section.") );
			m_editor->OnPlayToggle( wxCommandEvent() );
			m_editor->m_controller.HACK_GetPlayer()->m_internalState.m_sectionInputNumber = 0; // this prevents sections with input-output mapping from looping

			m_state = SRPS_Recording;
		}
	}
	else if ( name == RED_NAME( DialogEditorDestroyed ) && m_state == SRPS_EditorClosing )
	{
		LOG_RECORDER( TXT("Finished closing scene editor, unloading current scene file.") );
		m_currentSceneFile->Unload();
		m_currentSceneFile = nullptr;
		RecordNextSection();
	}
	else if ( name == RED_NAME( SectionChanged ) && m_state == SRPS_ChangingSection )
	{
		const CStorySceneSection* section = GetEventData< CStorySceneSection* >( data );
		if ( section == m_currentSection )
		{
			LOG_RECORDER( TXT("Changed section to %s."), m_currentSection->GetName().AsChar() );
			OnSectionChanged();
		}
	}
	else if ( name == RED_NAME( SectionFinished ) && m_state == SRPS_Recording )
	{
		const CStorySceneSection* section = GetEventData< CStorySceneSection* >( data );
		if ( m_mode == SRRM_Video && section == m_currentSection )
		{
			static Bool finishedVideoLock = true;
			if ( finishedVideoLock )
			{
				finishedVideoLock = false;
				LOG_RECORDER( TXT("Finished video pass.") );
				OnVideoPassFinished();
				finishedVideoLock = true;
			}
		}
		else if ( m_mode == SRRM_Audio && section == m_currentSection )
		{
			static Bool finishedAudioLock = true;
			if ( finishedAudioLock )
			{
				finishedAudioLock = false;
				LOG_RECORDER( TXT("Finished audio pass.") );
				OnAudioPassFinished();
				finishedAudioLock = true;
			}
		}
	}
	else if ( name == RED_NAME( AudioTrackStarted ) && m_mode == SRRM_Audio )
	{
		static Bool audioTrackStartedLock = true;
		if ( audioTrackStartedLock )
		{
			audioTrackStartedLock = false;
			LOG_RECORDER( TXT("Starting audio recording.") );
			OnAudioTrackStarted();
			audioTrackStartedLock = true;
		}
	}
}

void CSceneRecorder::OnSectionChanged()
{
	ASSERT( m_mode == SRRM_Audio || m_mode == SRRM_None );
	if ( !m_editor->PlayInMainWorld( true ) )
	{
		WARN_RECORDER( TXT("Cannot set scene %s in currently loaded world, skipping."), m_currentSection->GetFriendlyName() );
		RecordNextSection();
		return;
	}

	m_editor->OnMainControlPanel_UpdateCurrentVariantBase();

	//( new CRenderCommand_ResumeRendering() )->Commit();
	( new CRenderCommand_SuspendRendering() )->Commit();
	GRender->Flush();

	if ( GGame->IsBlackscreen() )
	{
		GGame->ResetFadeLock( TXT( "Scene recorder" ) );
		GGame->StartFade( true, TXT( "Scene recorder" ), 0.0f );
	}
	
	//GGame->ToggleContignous( FCSF_PNG, m_config.m_useUbersample );

	//m_mode = SRRM_Video;
	m_mode = SRRM_Audio;
	m_state = SRPS_WaitingForSection;
}

void CSceneRecorder::ProbeFile( const String& fileName, Float& fileDurationSeconds, String& fileDurationString )
{
	String params = String::Printf( TXT("%s %s"), m_config.outputDirPath.AsChar(), fileName.AsChar() );
	ExecBatchFile( TXT("probe.bat"), LIBAV_DIR, params );

	CSystemFile outputFile;
	outputFile.CreateReader( ( m_config.outputDirPath + TXT("probe.log") ).AsChar() );

	AnsiChar buffer[1024];

	outputFile.Read( buffer, outputFile.GetSize() );
	StringAnsi fileContent( buffer );

	StringAnsi durationString( "Duration: " );
	size_t subStringIndex;
	fileContent.FindSubstring( durationString.AsChar(), subStringIndex, false, 0 );
	fileDurationString = String( ANSI_TO_UNICODE( fileContent.MidString( subStringIndex + durationString.GetLength(), 11 ).AsChar() ) ); // Duration: 00:01:02.81

	Float minutes, seconds;
	FromString( fileDurationString.MidString( 3, 2 ), minutes );
	FromString( fileDurationString.MidString( 6, 5 ), seconds );

	fileDurationSeconds = 60.f * minutes + seconds;

	outputFile.Close();
}

void CSceneRecorder::CalculateDurations( const Float videoDurationSeconds, String& audioDurationString, String& audioTrimOffset, const Uint32 sampleNumber )
{
	String sectionName = m_currentSection->GetName().AsChar();
	sectionName.RemoveWhiteSpaces();

	String audioFileName = String::Printf( TXT("%s_%d.wav"), sectionName.AsChar(), sampleNumber );
	Float audioDurationSeconds;
	ProbeFile( audioFileName, audioDurationSeconds, audioDurationString );

	// diff won't be greater than 59.99s
	audioTrimOffset = String::Printf( TXT("00:00:%.3f"), Red::Math::MAbs( videoDurationSeconds - audioDurationSeconds ) );
}

void CSceneRecorder::OnVideoPassFinished()
{
	String outputFileName = m_currentSection->GetName();
	outputFileName.RemoveWhiteSpaces();

	if ( !m_config.skipVideoRecording )
	{
		ASSERT( GScreenshotSequence != nullptr );
		String screenShotBaseName = GScreenshotSequence->GetBaseName();
		String screenShotPattern = GScreenshotSequence->GetPattern();
		screenShotPattern.RemoveWhiteSpaces();

		String inputPath = GFileManager->GetBaseDirectory() + screenShotBaseName;
		String params = String::Printf( TXT("%s %s %s %s %s"), inputPath.AsChar(), screenShotPattern.AsChar(), m_config.outputDirPath.AsChar(), outputFileName.AsChar(), ToString( m_config.deleteScreenshots ).AsChar() );
		ExecBatchFile( TXT("gen_no_audio.bat"), LIBAV_DIR, params );

		GGame->ToggleContignous();
	}

	( new CRenderCommand_SuspendRendering() )->Commit();
	GRender->Flush();

	String videoFileName = String::Printf( TXT("%s_no_audio.avi"), outputFileName.AsChar() );
	String videoDurationString;
	Float videoDurationSeconds;
	ProbeFile( videoFileName, videoDurationSeconds, videoDurationString );

	for ( Uint32 i = 1; i <= m_config.audioSampleCount; ++i )
	{
		String audioDurationString, audioTrimOffset;
		CalculateDurations( videoDurationSeconds, audioDurationString, audioTrimOffset, i );
		ConvertSequenceToVideo( outputFileName, m_config.outputDirPath, videoDurationString, audioDurationString, audioTrimOffset, i );
	}

	// unbind scene from world
	m_editor->PlayInMainWorld( true );

	// mark current scene section as recently recorded
	if ( m_scenesMetadata )
	{
		TDynArray< String > rowData;
		rowData.PushBack( outputFileName );
		rowData.PushBack( ToString( m_currentSceneFile->GetFileTime() ) );
		m_scenesMetadata->AddRow( rowData );
		m_scenesMetadata->SaveAs( GDepot, TXT("scenes_metadata.csv"), true );
	}

	// all sections for current scene has been recorded
	if ( m_currentSceneSections.Empty() )
	{
		// find paths from inputs to outputs in current scene graph and assemble them from recorded sections
		AssembleScenePaths( m_editor->m_storyScene );

		m_editor->Close();
		m_state = SRPS_EditorClosing;
	}
	else
	{
		RecordNextSection();
	}
}

void CSceneRecorder::OnAudioPassFinished()
{
	if ( m_audioRecorder )
	{
		ASSERT( m_audioRecorder->IsRecording() );

		m_audioRecorder->StopRecording();
		delete m_audioRecorder;
		m_audioRecorder = nullptr;
	}

	if ( --m_currentSampleNumber )
	{
		m_editor->m_controller.RestartSection();

		m_mode = SRRM_Audio;
		m_state = SRPS_WaitingForSection;

		return;
	}

	( new CRenderCommand_ResumeRendering() )->Commit();
	GRender->Flush();

	m_editor->m_controller.RestartSection();

	if ( !m_config.skipVideoRecording )
	{
		GGame->ToggleContignous( FCSF_PNG, m_config.useUbersample );
	}

	m_mode = SRRM_Video;
	m_state = SRPS_WaitingForSection;
}

void CSceneRecorder::OnAudioTrackStarted()
{
	if ( !m_audioRecorder )
	{
		String outputFile = m_config.outputDirPath + TXT("\\") + String::Printf( TXT("%s_%d.wav"), m_currentSection->GetName().AsChar(), m_currentSampleNumber );
		outputFile.RemoveWhiteSpaces();

		m_audioRecorder = new LoopbackAPI::LoopbackRecorder( outputFile.AsChar(), outputFile.Size() );
		Int32 result = m_audioRecorder->StartRecording();
		if ( result == LoopbackAPI::STATUS_NO_DEVICE_HANDLE )
		{
			WARN_RECORDER( TXT("Couldn't grab audio device for loopback recording. Skipping audio for current section.") );
			delete m_audioRecorder;
			m_audioRecorder = nullptr;
		}
		else if ( result == LoopbackAPI::STATUS_NO_FILE_HANDLE )
		{
			WARN_RECORDER( TXT("Couldn't create .wav file for loopback recording. Skipping audio for current section.") );
			delete m_audioRecorder;
			m_audioRecorder = nullptr;
		}
	}
}