/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CStoryScene;

class IQuestBlockWithScene
{
public:
	virtual ~IQuestBlockWithScene() {}

	// Returns currently set story scene instance
	virtual CStoryScene* GetScene() const = 0;

#ifndef NO_EDITOR_GRAPH_SUPPORT

	// Sets a new story scene
	virtual void SetScene( CStoryScene* scene ) = 0;

#endif
};

