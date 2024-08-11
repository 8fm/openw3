/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../redMath/redmathbase.h"

/// Animation math - a types defined either to be Havok or Red math
/// depending on compilation parameters. After Havok math is no longer needed
/// this can be refactored using Visual Assist's "Rename" option or by using
/// visuals "Replace in Files". For the time being it allows to reduce the
/// complexity of some of the header files.

typedef RedQsTransform AnimQsTransform;
typedef RedQuaternion AnimQuaternion;
typedef RedVector4 AnimVector4;
typedef Float AnimFloat;
typedef RedMatrix4x4 AnimMatrix44;
typedef RedAABB AnimAABB;
