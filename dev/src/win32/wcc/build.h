/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once

// Core
#include "..\..\common\core\core.h"

// Engine
#include "..\..\common\engine\engine.h"

// Game
#include "..\..\common\game\game.h"

// R4
#include "..\..\games\r4\r4.h"

// R6
#include "..\..\games\r6\r6.h"

// THE COOKER CANNOT BE COMPILED WITHOUT EDITOR SUPPORT
#ifdef NO_EDITOR
	#error "WCC can only be compiled for editor configuration"
#endif

// Types
#include "wccTypeRegistry.h"

#define LOG_WCC( format, ... ) RED_LOG( WCC, format, ##__VA_ARGS__ )
#define ERR_WCC( format, ... ) RED_LOG_ERROR( WCC, format, ##__VA_ARGS__ )
#define WARN_WCC( format, ... ) RED_LOG_WARNING( WCC, format, ##__VA_ARGS__ );

// Warnings
RED_DISABLE_WARNING_MSC( 4748 )
RED_DISABLE_WARNING_MSC( 4996 )	// 'function': was declared deprecated

//Wcc
#include "wccEngine.h"
#include "wccExternalReporter.h"

/// Texture cache cooker
class CTextureCacheCooker;
extern CTextureCacheCooker* GTextureCacheCooker;


