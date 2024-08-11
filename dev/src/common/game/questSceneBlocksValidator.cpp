#include "build.h"
#include "questSceneBlocksValidator.h"
#include "questGraphSocket.h"
#include "questGraphBlock.h"
#include "storySceneInput.h"
#include "storySceneOutput.h"

#ifndef NO_EDITOR_GRAPH_SUPPORT

Bool CQuestSceneBlocksValidator::IsOutdated( CStoryScene* scene, const CQuestGraphBlock* block ) const
{
	Bool inputsOutdated = !CheckSockets< CStorySceneInput >( scene, block, LSD_Input );
	if  ( inputsOutdated )
	{
		return true;
	}
	Bool outputsOutdated = !CheckSockets< CStorySceneOutput >( scene, block, LSD_Output );
	if  ( outputsOutdated )
	{
		return true;
	}
	return false;
}

#endif