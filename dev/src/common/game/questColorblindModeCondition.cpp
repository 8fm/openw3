/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "questColorblindModeCondition.h"
#include "../../common/engine/renderGameplayEffects.h"

IMPLEMENT_ENGINE_CLASS( CQuestColorblindModeCondition );

Bool CQuestColorblindModeCondition::OnIsFulfilled()
{
	return Config::cvColorblindFocusMode.Get(); 
}