/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/engineMetalinkTypes.h"

enum class EGameMetalinkType : PathLib::MetalinkClassId
{
	T_METALINK = PathLib::MetalinkClassId( EEngineMetalinkType::T_COUNT ),

	T_COUNT
};