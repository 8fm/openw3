/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#define STORY_SCENE_VOICE_TAGS_TABLE TXT("gameplay\\globals\\scene_voice_tags.csv")

class CStorySceneVoiceTagsManager
{
public:
	CStorySceneVoiceTagsManager();
	~CStorySceneVoiceTagsManager();

public:	

	//! if voice tag file path already present return false
	Bool AddVoiceTagsTable( const String& filePath );
	//! if voice tag file path not present return false
	Bool RemVoiceTagsTable( const String& filePath );

	const C2dArray* ReloadVoiceTags();
	const C2dArray* GetVoiceTags();

private:
	void Clear();
	void LoadVoiceTags( const Char* filePath );

private:
	C2dArray			m_cachedVoiceTagsArray; 
	TDynArray<String>	m_sceneVoiceTagsTableFilePaths;
};

typedef TSingleton< CStorySceneVoiceTagsManager, TDefaultLifetime, TCreateUsingNew > SStorySceneVoiceTagsManager;