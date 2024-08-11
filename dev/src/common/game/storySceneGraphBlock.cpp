/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneGraphBlock.h"
#include "storyScene.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneGraphBlock )
	
#ifndef NO_EDITOR_GRAPH_SUPPORT

void CStorySceneGraphBlock::OnDestroyed()
{
	CStorySceneControlPart* controlPart = GetControlPart();
	if ( controlPart != NULL )
	{
		controlPart->GetScene()->RemoveControlPart( controlPart );
	}
}

#endif
