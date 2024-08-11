/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CFlashValue;

namespace Flash
{
	//CHANGEME: Replace args with CFlashValue that contains an Array
	Bool CallEvent( IScriptable* context, CName eventName, const TDynArray< CFlashValue >& args );
}