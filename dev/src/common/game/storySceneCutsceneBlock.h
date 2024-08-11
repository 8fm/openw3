#pragma once

#include "storySceneSectionBlock.h"

/// Block in the scene graph that represents cutscene
class CStorySceneCutsceneSectionBlock : public CStorySceneSectionBlock
{
	DECLARE_ENGINE_CLASS( CStorySceneCutsceneSectionBlock, CStorySceneSectionBlock, 0 )

public:
	CStorySceneCutsceneSectionBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get the name of the block
	virtual String GetBlockName() const;

	//! Get title bar color
	virtual Color GetTitleColor() const;

	virtual CClass* GetBlockClass() { return ClassID< CStorySceneCutsceneSectionBlock >(); }

	//! Rebuild block layout
	virtual void OnRebuildSockets();

#endif

};

BEGIN_CLASS_RTTI( CStorySceneCutsceneSectionBlock );
	PARENT_CLASS( CStorySceneSectionBlock );
END_CLASS_RTTI();
