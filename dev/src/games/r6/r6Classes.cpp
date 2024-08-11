/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#undef _H_R6_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "r6TypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterR6GameClasses()
{
#undef _H_R6_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); }
#include "r6TypeRegistry.h"
#undef REGISTER_RTTI_TYPE
}

CGame* CreateR6Game()
{
	// Find game class
	CClass* gameClass = SRTTI::GetInstance().FindClass( CNAME( CR6Game ) );
	if ( !gameClass )
	{
		WARN_R6( TXT("Unable to find R6 game class") );
		return NULL;
	}

	// Create game
	GCommonGame = CreateObject< CR6Game >( gameClass );
	return GCommonGame;
}