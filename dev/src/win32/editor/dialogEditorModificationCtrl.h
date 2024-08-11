
#pragma once

class CEdSceneEditor;
class CStorySceneLinkElement;

#include "../../common/game/storyScene.h"

class CEdSceneModCtrl : public IStorySceneModifier
{
	const CEdSceneEditor*	m_editor;

public:
	CEdSceneModCtrl();

	void Init( const CEdSceneEditor* editor );

public:
	//...
};
