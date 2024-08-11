/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once


///////////////////////////////////////////////////////////////////////////////

class CCommunityConstants
{
public:
	// The spawn point radius default value
	static Float SPAWN_POINT_RADIUS;

	// Visibility system: spawn agent stub to NPC radius
	static Float VISIBILITY_SPAWN_RADIUS;

	// Visibility system: despawn NPC to agent stub radius
	static Float VISIBILITY_DESPAWN_RADIUS;

	// Visibility system: despawn NPC to agent stub radius when player is in community area
	static Float VISIBILITY_AREA_DESPAWN_RADIUS;

	// Visibility system: spawn NPC to agent stub radius when player is in community area
	static Float VISIBILITY_AREA_SPAWN_RADIUS;

	// Visibility system: if true, than changing visibility radius from gameplay is allowed
	static Bool VISIBILITY_RADIUS_CHANGE_ENABLED;
};

///////////////////////////////////////////////////////////////////////////////
