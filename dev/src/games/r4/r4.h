/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#define LOG_R4( format, ... ) RED_LOG( R4, format, ##__VA_ARGS__ )
#define ERR_R4( format, ... ) RED_LOG_ERROR( R4, format, ##__VA_ARGS__ )
#define WARN_R4( format, ... ) RED_LOG_WARNING( R4, format, ##__VA_ARGS__ );

#include "r4TypeRegistry.h"
#include "r4NamesRegistry.h"
#include "r4SystemOrder.h"
#include "r4Game.h"

void RegisterR4GameClasses();
CGame* CreateR4Game();
