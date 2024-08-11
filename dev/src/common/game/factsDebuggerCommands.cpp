/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#ifndef NO_DEBUG_SERVER

#include "factsDebuggerCommands.h"
#include "factsDebuggerPlugin.h"

Uint32 CDebugServerCommandGetFactIDs::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	if ( !GGame->GetActiveWorld() )
		return 0;

	CFactsDebuggerPlugin* factsDebuggerPlugin = dynamic_cast< CFactsDebuggerPlugin* >( owner );
	if ( factsDebuggerPlugin == nullptr )
		return 0;

	return factsDebuggerPlugin->SendFactIDs();
}

Uint32 CDebugServerCommandGetFactData::ProcessCommand( CDebugServerPlugin* owner, const TDynArray<String>& data )
{
	if ( !GGame->GetActiveWorld() )
		return 0;

	CFactsDebuggerPlugin* factsDebuggerPlugin = dynamic_cast< CFactsDebuggerPlugin* >( owner );
	if ( factsDebuggerPlugin == nullptr )
		return 0;

	return factsDebuggerPlugin->SendFactData( data[0] );
}

#endif
