/**
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#undef _H_COMMONGAME_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "gameTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterCommonGameClasses()
{
	#undef _H_COMMONGAME_TYPE_REGISTRY
	#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); }
	#include "gameTypeRegistry.h"
	#undef REGISTER_RTTI_TYPE
}
