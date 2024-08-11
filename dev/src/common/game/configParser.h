#pragma once
#include "../../common/core/system.h"
#include "../../common/core/hashmap.h"
#include "../../common/core/gameConfiguration.h"

class GameConfig
{
public:

	static bool ParseConfig( const String& configContents, THashMap< String, THashMap< String, String > >& outConfig );
	static bool LoadConfig( const String& game );
};