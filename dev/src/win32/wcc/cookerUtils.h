/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/commandlet.h"

enum ECookingPlatform : Int32;

namespace CookerUtils
{
	ECookingPlatform GetCookingPlatform( const ICommandlet::CommandletOptions& options );
}