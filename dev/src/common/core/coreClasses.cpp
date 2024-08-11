/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "namesPool.h"

#undef _H_CORE_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "coreTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterCoreClasses()
{
}
