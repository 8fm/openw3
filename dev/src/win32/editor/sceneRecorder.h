/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CStorySceneLinkElement;
class CStorySceneSection;
class CStorySceneOutput;
class CEdSceneEditor;
class CStoryScene;

#define LOG_RECORDER( format, ... ) RED_LOG( Recorder, format, ##__VA_ARGS__ )
#define ERR_RECORDER( format, ... ) RED_LOG_ERROR( Recorder, format, ##__VA_ARGS__ )
#define WARN_RECORDER( format, ... ) RED_LOG_WARNING( Recorder, format, ##__VA_ARGS__ );

namespace LoopbackAPI
{
	class LoopbackRecorder; 
}

class CSceneRecorder: IEdEventListener
{
public:
	CSceneRecorder(void);
	~CSceneRecorder(void);

	Bool ParseCommandline( const THashMap< String, String >& commandLine );
	void OnStart();

protected:
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data  );

private:

	String LIBAV_DIR;

	enum ESceneRecorderProcessingState 
	{
		SRPS_None,
		SRPS_ChangingSection,
		SRPS_EditorClosing,
		SRPS_WaitingForWorld,
		SRPS_WaitingForSection,
		SRPS_Recording
	};

	ESceneRecorderProcessingState				m_state;

	enum ESceneRecorderRecordingMode
	{
		SRRM_None,
		SRRM_Audio,
		SRRM_Video
	};

	ESceneRecorderRecordingMode					m_mode;

	struct Config
	{
		Uint32						height;
		Uint32						width;
		Bool						useUbersample;
		Bool						deleteScreenshots;
		Bool						skipVideoRecording;
		String						outputDirPath;
		String						sectionName;
		Uint32						audioSampleCount;

		Config();
	};

	Config										m_config;

	struct SceneInfo
	{
		String						m_scenePath;
		TDynArray< String >			m_layerNames;
	};

	struct RecordingInfo
	{
		String						m_worldPath;
		TDynArray< SceneInfo >		m_scenesToRecord;
	};

	RecordingInfo								m_recordingInfo;
	CDiskFile*									m_currentSceneFile;
	CStorySceneSection*							m_currentSection;
	TDynArray< CStorySceneSection* >			m_currentSceneSections;

	// Scene editor for currently recorded scene
	CEdSceneEditor*								m_editor;

	Uint32										m_currentSampleNumber;

	// Class with logic for loopback audio capturing
	LoopbackAPI::LoopbackRecorder*				m_audioRecorder;

	// Resource with pairs of scene file names and their modification timestamps
	C2dArray*									m_scenesMetadata;

	// Loads scene files paths and corresponding layer names to be excluded
	Bool LoadFromConfigFile( const String& path );

	// Finds all .w2scene files in selected directory and its subdirectories and if 'scenes_metadata.csv' file is present in depot
	// then it chooses only those scene files that have different timestamp than the one provided in metadata file
	void LoadModifiedFromDir( const String& path, const String& scenePattern );

	// load temporary file that holds information about already recorded sections
	void LoadMetadataFile( const String& fileName );

	// Initialize all required variables (viewport, world, renderer, etc.)
	void SetupEnvironment();

	Bool WasRecentlyRecorded( const CStorySceneSection* section );
	Bool ShouldExcludeSection( const CStorySceneSection* section );
	// Returns next section for current or next scene
	CStorySceneSection* GetNextSection();

	void RecordNextSection();
	Bool LoadWorldWithAllLayers( const String& worldPath );
	void UnloadExcludedLayers( TDynArray< String >& layerNames );

	// Determines all scene paths for unique input-output pairs and merges recorded sections
	void AssembleScenePaths( CStoryScene* currentScene );

	// Recursive function used to find all paths with unique output scene elements from selected link element
	void FindOutputs( const CStorySceneLinkElement* currentElement, 
					  TDynArray< const CStorySceneLinkElement* >& visited,
					  THashSet< const CStorySceneOutput* >& usedOutputs,
					  TDynArray< TDynArray< String > >& foundPaths );

	void ProbeFile( const String& fileName, Float& fileDurationSeconds, String& fileDurationString );
	void CalculateDurations( const Float videoDurationSeconds, String& audioDurationString, String& audioTrimOffset, const Uint32 sampleNumber );
	void ConvertSequenceToVideo( const String& outputFile, const String& outputPath, const String& duration, 
								 const String& audioDuration, const String& audioTrimOffset, const Uint32& sampleNumber );
	void MergeSectionsToScenePath( TDynArray< String >& sectionsNames, String& outputPath, String& sceneName );

	// Event dispatching functions
	void OnSectionChanged();
	void OnVideoPassFinished();
	void OnAudioPassFinished();
	void OnAudioTrackStarted();

};

typedef TSingleton< CSceneRecorder > SSceneRecorder;