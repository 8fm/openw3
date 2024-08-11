#pragma once

#include "storyScene.h"
#include "behTreeArbitratorPriorities.h"

class CVoicesetPlayer
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_Dialog );

	EArbitratorPriorities		m_priority;
	String						m_voiceset;
	TSoftHandle< CStoryScene >	m_scene;

public:
	CVoicesetPlayer( EArbitratorPriorities priority, const String& voiceset );
	~CVoicesetPlayer();

	Bool IsSceneLoaded() const;

	void PlayVoiceset( CActor* actor );

	Bool StartLoading( TSoftHandle< CStoryScene >& sceneHandle );
};
