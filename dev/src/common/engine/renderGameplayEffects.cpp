/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "renderGameplayEffects.h"
#include "../core/gatheredResource.h"


namespace Config
{
	TConfigVar< Bool >												cvColorblindFocusMode(			"Gameplay", "ColorblindFocusMode",							false, eConsoleVarFlag_Save );
	TConfigVar< Bool >												cvMotionSicknessFocusMode(		"Gameplay", "MotionSicknessFocusMode",						false, eConsoleVarFlag_Save );	
}

void RegisterRendererFunctions()
{
	// Setup any global script functions related to rendering effects here
}
