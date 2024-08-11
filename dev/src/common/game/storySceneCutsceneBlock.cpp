#include "build.h"
#include "storySceneCutsceneBlock.h"
#include "storySceneCutsceneSection.h"
#include "storySceneGraphSocket.h"
#include "storySceneChoice.h"
#include "storySceneChoiceLine.h"
#include "../engine/graphConnectionRebuilder.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneCutsceneSectionBlock )

CStorySceneCutsceneSectionBlock::CStorySceneCutsceneSectionBlock(void)
{
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CStorySceneCutsceneSectionBlock::GetBlockName() const
{
	// Use the section name
	return m_section ? m_section->GetName() : TXT("Cutscene");
}

Color CStorySceneCutsceneSectionBlock::GetTitleColor() const
{
	return Color( 128, 0 ,128 );
}

#endif


#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneCutsceneSectionBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	
	// Build layout only for valid sections
	if ( m_section == NULL )
	{
		return;
	}

	TBaseClass::OnRebuildSockets();
}
#endif