// This file exists primarily for APEX integration

#ifndef RT_DEF_H
#define RT_DEF_H

#include "NxApexDefs.h"
#include "NxModuleDestructible.h"

#if APEX_RUNTIME_FRACTURE
#define RT_COMPILE 1
#else
#define RT_COMPILE 0
#endif

#if APEX_USE_GRB
#define RT_USE_GRB 1
#else
#define RT_USE_GRB 0
#endif

#endif