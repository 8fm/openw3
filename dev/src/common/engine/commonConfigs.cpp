/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "commonConfigs.h"

namespace Config
{
	TConfigVar<String> cvDatabaseAddress( "Database", "Address",	TXT("CDPRS-MSSQL\\sqlexpress"), eConsoleVarFlag_Save | eConsoleVarFlag_Developer );
	TConfigVar<String> cvDatabaseName	( "Database", "Name",		TXT("EditorStringDataBaseW3"),	eConsoleVarFlag_Save | eConsoleVarFlag_Developer );

}
