/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#undef _H_WCC_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "wccTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterWccClasses()
{
	#undef _H_WCC_TYPE_REGISTRY
	#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); }
	#include "wccTypeRegistry.h"
	#undef REGISTER_RTTI_TYPE
}

void RegisterGameClasses()
{
	RegisterR4GameClasses();

	RegisterWccClasses();
}