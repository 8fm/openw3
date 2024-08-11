/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define LOG_R6( format, ... ) RED_LOG( R6, format, ##__VA_ARGS__ )
#define ERR_R6( format, ... ) RED_LOG_ERROR( R6, format, ##__VA_ARGS__ )
#define WARN_R6( format, ... ) RED_LOG_WARNING( R6, format, ##__VA_ARGS__ );

#include "r6error.h"
#include "r6TypeRegistry.h"
#include "r6NamesRegistry.h"
#include "r6SystemOrder.h"
#include "r6game.h"

void RegisterR6GameClasses();
CGame* CreateR6Game();