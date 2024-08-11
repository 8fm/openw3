/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/cooker.h"

// Cooker for non-resource textures (loose PNGs, DDSs, etc. used by middleware). Sadly, because they aren't CResource, this can't just
// go through the regular texture cookers.
extern Bool CookNonResourceTexture( const String& depotPath, ECookingPlatform platform );
