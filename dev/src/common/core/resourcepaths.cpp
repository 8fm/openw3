/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "resourcePaths.h"
#include "directory.h"
#include "filePath.h"

namespace Red { namespace Core {

namespace ResourceManagement {

const Char* CResourcePaths::STREAMING_DIRECTORY_NAME		= TXT( "streaming" );
const Char* CResourcePaths::BUNDLES_DIRECTORY_NAME			= TXT( "bundles" );
const Char* CResourcePaths::WORKING_DIRECTORY_NAME			= TXT( "temp" );
const Char* CResourcePaths::QUEST_WORKING_DIRECTORY_NAME	= TXT( "quest" );
const Char* CResourcePaths::SOURCE_FOLIAGE_DIRECTORY_NAME	= TXT( "source_foliage" );


const Char* CResourcePaths::METADATA_FILE_EXTENSION			= TXT( ".wsm" );
const Char* CResourcePaths::BUNDLEDEF_FILE_EXTENSION		= TXT( ".json" );
const Char* CResourcePaths::DATABASE_FILE_EXTENSION			= TXT( ".dbt" );
const Char* CResourcePaths::QUEST_METADATA_FILE_EXTENSION	= TXT( ".qsm" );

const Char* CResourcePaths::LAYER_DEFINITION_FILENAME_ADDITION = TXT( ".Layers" );

CResourcePaths::CResourcePaths()
{
	m_paths.Resize( Path_Max );
	m_directories.Resize( Directory_Max );
}

CResourcePaths::~CResourcePaths()
{

}

void CResourcePaths::Initialize( CDirectory* worldDirectory, const String& worldFilename )
{
	// Create Sub-directories
	CDirectory* streamingDirectory		= worldDirectory->CreateNewDirectory( STREAMING_DIRECTORY_NAME );
	CDirectory* workingDirectory		= streamingDirectory->CreateNewDirectory( WORKING_DIRECTORY_NAME );
	CDirectory* bundlesDirectory		= streamingDirectory->CreateNewDirectory( BUNDLES_DIRECTORY_NAME );
	CDirectory* questWorkingDirectory	= workingDirectory->CreateNewDirectory( QUEST_WORKING_DIRECTORY_NAME );
	CDirectory* foliageDirectory		= worldDirectory->CreateNewDirectory( SOURCE_FOLIAGE_DIRECTORY_NAME );

	// Ensure they exist on disk
	workingDirectory->CreateOnDisk();
	foliageDirectory->CreateOnDisk();

	// Helper 
	CFilePath worldFilePath( worldFilename );
	
	// Calculate various paths and filenames
	m_paths[ Path_StreamingDirectoryDepot ]			= streamingDirectory->GetDepotPath();
	m_paths[ Path_WorkingDirectoryDepot ]			= workingDirectory->GetDepotPath();
	m_paths[ Path_BundlesDirectoryAbsolute ]		= bundlesDirectory->GetAbsolutePath();
	m_paths[ Path_WorldMetadataDepot ]				= streamingDirectory->GetDepotPath() + worldFilePath.GetFileName() + METADATA_FILE_EXTENSION;
	m_paths[ Path_DatabaseDepot ]					= streamingDirectory->GetDepotPath() + worldFilePath.GetFileName() + DATABASE_FILE_EXTENSION;
	m_paths[ Path_DatabaseAbsolute ]				= streamingDirectory->GetAbsolutePath() + worldFilePath.GetFileName() + DATABASE_FILE_EXTENSION;
	m_paths[ Path_BundleDefinitionFilename ]		= worldFilePath.GetFileName() + BUNDLEDEF_FILE_EXTENSION;
	m_paths[ Path_LayerBundleDefinitionFilename ]	= worldFilePath.GetFileName() + LAYER_DEFINITION_FILENAME_ADDITION + BUNDLEDEF_FILE_EXTENSION;
	m_paths[ Path_QuestMetadataDepot ]				= questWorkingDirectory->GetDepotPath() + worldFilePath.GetFileName() + QUEST_METADATA_FILE_EXTENSION;
	m_paths[ Path_FoliageSourceData ]				= foliageDirectory->GetDepotPath();

	m_directories[ Directory_Working ]				= workingDirectory;
	m_directories[ Directory_WorkingQuest ]			= questWorkingDirectory;
}

} // namespace ResourceManagement {

} } // namespace Red { namespace Core {

