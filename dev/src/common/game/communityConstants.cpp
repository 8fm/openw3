/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "communityConstants.h"


// The spawn point radius default value
Float CCommunityConstants::SPAWN_POINT_RADIUS = 5.0f;

// Visibility system: spawn agent stub to NPC radius
Float CCommunityConstants::VISIBILITY_SPAWN_RADIUS = 40.0f;

// Visibility system: despawn NPC to agent stub radius
Float CCommunityConstants::VISIBILITY_DESPAWN_RADIUS = 60.0f;

// Visibility system: despawn NPC to agent stub radius when player is in community area
Float CCommunityConstants::VISIBILITY_AREA_DESPAWN_RADIUS = 10.0f;

// Visibility system: despawn NPC to agent stub radius when player is in community area
Float CCommunityConstants::VISIBILITY_AREA_SPAWN_RADIUS = 10.0f;

// Visibility system: if true, than changing visibility radius from gameplay is allowed
Bool CCommunityConstants::VISIBILITY_RADIUS_CHANGE_ENABLED = true;
