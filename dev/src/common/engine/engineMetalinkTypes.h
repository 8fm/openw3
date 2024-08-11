/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "pathlibMetalinkComponent.h"

enum class EEngineMetalinkType : PathLib::MetalinkClassId
{
	T_STRIPE,

	T_COUNT,

	T_GENERIC = 2,					// for backward data compatibility (as we already send the navdata, we must hand-set new iterator values)

	T_REAL_COUNT
};
