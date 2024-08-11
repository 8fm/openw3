/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "profilerChannels.h"

IMPLEMENT_RTTI_ENUM( EProfilerBlockChannel );

namespace ProfilerChannelsHelper
{
	Int32 ConvertChannelsToInt(const String& value)
	{
		Int32 result = 0;
		TDynArray<String> channels = value.Split( TXT("|") );
		CEnum* profilerBlockChannel = SRTTI::GetInstance().FindEnum( CNAME( EProfilerBlockChannel ) );

		for( const String& channel : channels )
		{
			Int32 channelValue = 0;
			profilerBlockChannel->FindValue( CName( channel.AsChar() ), channelValue );
			result |= channelValue;
		}

		return result;
	}
}
