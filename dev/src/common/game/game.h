/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "gameTypeRegistry.h"
#include "gameNamesRegistry.h"

#define LOG_GAME( format, ... ) RED_LOG( Game, format, ##__VA_ARGS__ )
#define ERR_GAME( format, ... ) RED_LOG_ERROR( Game, format, ##__VA_ARGS__ )
#define WARN_GAME( format, ... ) RED_LOG_WARNING( Game, format, ##__VA_ARGS__ );

#include "gameSystemOrder.h"
#include "commonGame.h"
#include "gameplayEntity.h"
#include "player.h"
#include "scriptedComponent.h"

#include "aiLog.h"
#include "../core/scriptStackFrame.h"
