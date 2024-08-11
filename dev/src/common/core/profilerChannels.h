/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "enumBuilder.h"

enum EProfilerBlockChannel : CEnum::TValueType
{
	PBC_CPU =		FLAG(0),
	PBC_RENDER =	FLAG(1),
	PBC_ALL = ~0,
};

BEGIN_ENUM_RTTI( EProfilerBlockChannel )
	ENUM_OPTION( PBC_CPU							)
	ENUM_OPTION( PBC_RENDER							)
	ENUM_OPTION( PBC_ALL							)
END_ENUM_RTTI()

namespace ProfilerChannelsHelper
{
	Int32 ConvertChannelsToInt( const String& value );
}
