#pragma once
#include "storyScene.h"

class CStorySceneSpawner : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( CStorySceneSpawner, CGameplayEntity, 0 );

protected:
	THandle< CStoryScene >	m_storyScene;
	String						m_inputName;

public:
	CStoryScene*	GetStoryScene() { return m_storyScene.Get(); }
};

BEGIN_CLASS_RTTI( CStorySceneSpawner );
PARENT_CLASS( CGameplayEntity );
PROPERTY_EDIT( m_storyScene, TXT( "Story Scene File" ) );
PROPERTY_CUSTOM_EDIT( m_inputName, TXT( "Name of input to play" ), TXT( "SceneInputSelector" ) );
END_CLASS_RTTI();