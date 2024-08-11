/**
* Copyright c 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "cookerUtils.h"
#include "../../common/core/dependencyMapper.h"

namespace CookerUtils
{
	ECookingPlatform GetCookingPlatform( const ICommandlet::CommandletOptions& options )
	{
		for( Uint32 c = 0; c < options.GetFreeArguments().Size(); ++c )
		{
			String lowerCaseOption =  options.GetFreeArguments()[c].ToLower();
			if( lowerCaseOption == TXT("pc") || lowerCaseOption == TXT("windows") )
			{
				return PLATFORM_PC;
			}
#ifndef WCC_LITE
			else if( lowerCaseOption == TXT("xboxone") || lowerCaseOption == TXT("durango") )
			{
				return PLATFORM_XboxOne;
			}
			else if( lowerCaseOption == TXT("ps4") || lowerCaseOption == TXT("orbis") )
			{
				return PLATFORM_PS4;
			}
#endif
		}

		return PLATFORM_None;
	}
}