#include "build.h"
#include "storySceneItems.h"
#include "commonGame.h"
#include "..\engine\world.h"
#include "..\engine\environmentManager.h"

IMPLEMENT_ENGINE_CLASS( IStorySceneItem )
IMPLEMENT_ENGINE_CLASS( CStorySceneActor )
IMPLEMENT_ENGINE_CLASS( CStorySceneProp )
IMPLEMENT_ENGINE_CLASS( CStorySceneEffect )
IMPLEMENT_ENGINE_CLASS( CStorySceneLight )

IMPLEMENT_RTTI_ENUM( ELightType );

IMPLEMENT_ENGINE_CLASS( StorySceneExpectedActor )		// Old class, remove once data has been updated to CStorySceneActor
