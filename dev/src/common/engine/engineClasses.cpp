/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#undef _H_ENGINE_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "engineTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterEngineClasses()
{
	#undef _H_ENGINE_TYPE_REGISTRY
	#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); RED_UNUSED( classDesc ); }
	#include "engineTypeRegistry.h"
	#undef REGISTER_RTTI_TYPE
}
