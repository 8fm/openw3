/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/gameConfiguration.h"

#undef _H_EDITOR_TYPE_REGISTRY
#define REGISTER_RTTI_TYPE( _className ) RED_DEFINE_RTTI_NAME( _className )
#include "editorTypeRegistry.h"
#undef REGISTER_RTTI_TYPE

void RegisterEditorClasses()
{
	#undef _H_EDITOR_TYPE_REGISTRY
	#define REGISTER_RTTI_TYPE( _className ) { extern CClass* touchClass##_className(); CClass *classDesc = touchClass##_className(); }
	#include "editorTypeRegistry.h"
	#undef REGISTER_RTTI_TYPE
}

void RegisterGameClasses()
{
	RegisterEditorClasses();

	RegisterR4GameClasses();
}

CGame* CreateGame()
{
	// Find game class
	CClass* gameClass = SRTTI::GetInstance().FindClass( CName( GGameConfig::GetInstance().GetGameClassName() ) );
	if ( !gameClass )
	{
		WARN_EDITOR( TXT("Unable to find Witcher 3 game class") );
		return NULL;
	}

	// Create game
	GCommonGame = CreateObject< CCommonGame >( gameClass );
	GR4Game = Cast< CR4Game >( GCommonGame );
	return GCommonGame;
}

