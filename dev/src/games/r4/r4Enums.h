/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

//////////////////////////////////////////////////////////////////////////

enum EDifficultyMode
{
	EDM_NotSet,
	EDM_Easy,
	EDM_Medium,
	EDM_Hard,
	EDM_Hardcore,
	EDM_Total,
};

BEGIN_ENUM_RTTI( EDifficultyMode );
	ENUM_OPTION( EDM_NotSet );
	ENUM_OPTION( EDM_Easy );
	ENUM_OPTION( EDM_Medium );
	ENUM_OPTION( EDM_Hard );
	ENUM_OPTION( EDM_Hardcore );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

enum ENpcStance
{
	NS_Normal,
	NS_Strafe,
	NS_Retreat,
	NS_Guarded,
	NS_Wounded,
	NS_Fly,
	NS_Swim,
};

BEGIN_ENUM_RTTI( ENpcStance );
	ENUM_OPTION( NS_Normal );
	ENUM_OPTION( NS_Strafe );
	ENUM_OPTION( NS_Retreat );
	ENUM_OPTION( NS_Guarded );
	ENUM_OPTION( NS_Wounded );
	ENUM_OPTION( NS_Fly );
	ENUM_OPTION( NS_Swim );
END_ENUM_RTTI();
