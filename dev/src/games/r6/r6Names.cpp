/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#undef _H_R6_NAMES_REGISTRY
#define REGISTER_NAME( name_ ) RED_DEFINE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DEFINE_NAMED_NAME( varname_, string_ )
#include "r6NamesRegistry.h"
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NAMED_NAME

void RegisterR6Names()
{
}
