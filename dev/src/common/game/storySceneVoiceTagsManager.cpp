/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneVoiceTagsManager.h"
#include "../core/gatheredResource.h"
#include "../core/depot.h"

CStorySceneVoiceTagsManager::CStorySceneVoiceTagsManager()
{
	AddVoiceTagsTable(STORY_SCENE_VOICE_TAGS_TABLE );
	if( GIsEditor ) //! In editor all voice tags files have to be loaded on start
	{
		String voiceTagFullPath;

		const String dlcStoryScenyVoiceTagsTable = String::Printf( TXT("data\\%ls"), STORY_SCENE_VOICE_TAGS_TABLE );

		CDirectory* dlcsDirectory = GDepot->FindPath( TXT("dlc\\") );
		if( dlcsDirectory )
		{
			const TDirs& directorys = dlcsDirectory->GetDirectories();

			TDirs::const_iterator endDir = directorys.End();		
			for( TDirs::const_iterator dir = directorys.Begin(); dir != endDir; ++dir )
			{
				(*dir)->GetDepotPath( voiceTagFullPath );

				voiceTagFullPath += dlcStoryScenyVoiceTagsTable;

				if( GDepot->FileExist( voiceTagFullPath ) )
				{
					AddVoiceTagsTable( voiceTagFullPath );
				}					

			}		
		}
	}

	ReloadVoiceTags();
}

CStorySceneVoiceTagsManager::~CStorySceneVoiceTagsManager()
{
	Clear();
}

Bool CStorySceneVoiceTagsManager::AddVoiceTagsTable( const String& filePath )
{
	if( m_sceneVoiceTagsTableFilePaths.FindPtr( filePath ) == nullptr )
	{
		m_sceneVoiceTagsTableFilePaths.PushBackUnique( filePath );
		return true;
	}
	return false;
}

Bool CStorySceneVoiceTagsManager::RemVoiceTagsTable( const String& filePath )
{
	String* filePathPtr = m_sceneVoiceTagsTableFilePaths.FindPtr( filePath );
	if( filePathPtr != nullptr )
	{
		m_sceneVoiceTagsTableFilePaths.Erase( filePathPtr );
		return true;
	}
	return false;
}

const C2dArray* CStorySceneVoiceTagsManager::GetVoiceTags()
{
	return &m_cachedVoiceTagsArray;
}

const C2dArray* CStorySceneVoiceTagsManager::ReloadVoiceTags()
{
	Clear();

	for( auto& voiceTagFullPath : m_sceneVoiceTagsTableFilePaths )
	{
		LoadVoiceTags( voiceTagFullPath.AsChar() );
	}
	
	return &m_cachedVoiceTagsArray;
}

void CStorySceneVoiceTagsManager::Clear()
{
	m_cachedVoiceTagsArray.Clear();
}

void CStorySceneVoiceTagsManager::LoadVoiceTags( const Char* filePath )
{
	C2dArray* voiceTagsArray = LoadResource< C2dArray >( filePath );
	if( voiceTagsArray )
	{
		if( m_cachedVoiceTagsArray.Empty() )
		{
			C2dArray::Copy( m_cachedVoiceTagsArray, *voiceTagsArray );
		}
		else
		{
			C2dArray::Concatenate( m_cachedVoiceTagsArray, *voiceTagsArray );
		}
	}	
}
