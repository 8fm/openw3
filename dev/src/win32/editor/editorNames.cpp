/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#undef _H_EDITOR_NAMES_REGISTRY
#define REGISTER_NAME( name_ ) RED_DEFINE_NAME( name_ )
#define REGISTER_NAMED_NAME( varname_, string_ ) RED_DEFINE_NAMED_NAME( varname_, string_ )
#include "namesregistry.h"
#undef REGISTER_RTTI_TYPE
#undef REGISTER_NAMED_NAME

void RegisterEditorNames()
{
}

void RegisterGameNames()
{
	RegisterEditorNames();

	extern void RegisterR4Names();
	RegisterR4Names();
	
	//extern void RegisterR6Names();
	//RegisterR6Names();
}