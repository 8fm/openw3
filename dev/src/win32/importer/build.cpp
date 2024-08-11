/**
 * Copyright © 2007 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#undef _H_IMPORTER_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "importerTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterImportClasses()
{
	#undef _H_IMPORTER_TYPE_REGISTRY
	#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); }
	#include "importerTypeRegistry.h"
	#undef REGISTER_RTTI_TYPE
}
