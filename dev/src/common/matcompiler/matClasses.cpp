/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#undef _H_MATERIAL_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "matTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterMatClasses()
{
	#undef _H_MATERIAL_TYPE_REGISTRY
	#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass* classDesc = touchClass##_className(); (void)classDesc; }
	#include "matTypeRegistry.h"
	#undef REGISTER_RTTI_TYPE
}
