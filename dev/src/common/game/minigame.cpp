/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "minigame.h"

#include "aiProfile.h"
#include "../engine/deniedAreaComponent.h"

#include "../engine/behaviorGraphStack.h"

IMPLEMENT_RTTI_ENUM( EMinigameState );

IMPLEMENT_ENGINE_CLASS( CMinigame );

RED_DEFINE_NAME( Minigame );

RED_DEFINE_STATIC_NAME( OnStarted );
RED_DEFINE_STATIC_NAME( OnEnded );
RED_DEFINE_STATIC_NAME( OnActivatePlayersMimic );

CMinigame::CMinigame()
{
}

CMinigame::~CMinigame()
{
}



//////////////////////////////////////////////////////////////////////////