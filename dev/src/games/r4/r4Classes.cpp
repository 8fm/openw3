/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#undef _H_R4_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "r4TypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterR4GameClasses()
{
#undef _H_R4_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); }
#include "r4TypeRegistry.h"
#undef REGISTER_RTTI_TYPE
}

CGame* CreateR4Game()
{
	// Find game class
	CClass* gameClass = SRTTI::GetInstance().FindClass( CNAME( CR4Game ) );
	if ( !gameClass )
	{
		WARN_R4( TXT("Unable to find R4 game class") );
		return NULL;
	}

	// Create game
	GCommonGame = ( GR4Game = CreateObject< CR4Game >( gameClass ) );
	GCommonGame->SetViewport( QuickBoot::g_quickInitViewport );
	return GCommonGame;
}