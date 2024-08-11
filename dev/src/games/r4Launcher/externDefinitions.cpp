/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "externDefinitions.h"

void RegisterGameClasses()
{
	extern void RegisterR4GameClasses();
	RegisterR4GameClasses();
}

void RegisterGameNames()
{
	extern void RegisterR4Names();
	RegisterR4Names();
}

class CGame;
CGame* CreateGame()
{
	extern CGame * CreateR4Game();
	return CreateR4Game();
}

//===========================================================================
// :(HACK:(:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK
// Dummy implementation of the editor side class that's being used in scripts
#include "../../common/game/game.h"
#include "../../common/game/storyScenePlayer.h"
class CStoryScenePreviewPlayer : public CStoryScenePlayer
{
	DECLARE_ENGINE_CLASS( CStoryScenePreviewPlayer, CStoryScenePlayer, 0 );
};

BEGIN_CLASS_RTTI( CStoryScenePreviewPlayer );
PARENT_CLASS( CStoryScenePlayer );
END_CLASS_RTTI();
IMPLEMENT_ENGINE_CLASS( CStoryScenePreviewPlayer );

void InitialiseExternalClasses()
{
	touchClassCStoryScenePreviewPlayer();
}
// :(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK:(HACK
