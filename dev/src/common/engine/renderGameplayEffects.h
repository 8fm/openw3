/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

namespace Config
{
	extern TConfigVar< Bool > cvColorblindFocusMode;
	extern TConfigVar< Bool > cvMotionSicknessFocusMode;	
}

enum EGameplayEffectOutlineType
{
	FXOUTLINE_Target,			// Outlines a "targeted" object. Outline is visible through other meshes.
	FXOUTLINE_Occlusion,		// Outlines objects when they are occluded by foliage. Not visible through normal meshes.

	FXOUTLINE_MAX
};

static_assert( FXOUTLINE_MAX <= 4, " We use color channels to deal with multiple outlines, so we can only support 4." );
