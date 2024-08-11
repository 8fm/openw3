/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_ENGINE_POSE_TYPES_H_
#define _RED_ENGINE_POSE_TYPES_H_

#include "../core/atomicSharedPtr.h"

class CPoseProvider;
class CPosePool;

typedef Red::TAtomicSharedPtr< CPoseProvider > PoseProviderHandle;
typedef Red::TAtomicSharedPtr< CPosePool > PosePoolHandle;

#endif
