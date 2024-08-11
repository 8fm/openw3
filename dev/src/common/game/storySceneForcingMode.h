/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum EStorySceneForcingMode
{
	SSFM_ForceNothing,
	SSFM_ForcePosition,
	SSFM_ForcePositionWithCamera,
	SSFM_SpawnAndForcePosition,
	SSFM_SpawnWhenNeeded
};

BEGIN_ENUM_RTTI(EStorySceneForcingMode);
	ENUM_OPTION_DESC( TXT("Don't force anything"), SSFM_ForceNothing );
	ENUM_OPTION_DESC( TXT("Force position"), SSFM_ForcePosition );
	ENUM_OPTION_DESC( TXT("Force position with camera blending"), SSFM_ForcePositionWithCamera );
	ENUM_OPTION_DESC( TXT("Spawn and force position"), SSFM_SpawnAndForcePosition );
	ENUM_OPTION_DESC( TXT( "Spawn actors when needed" ), SSFM_SpawnWhenNeeded )
END_ENUM_RTTI();
