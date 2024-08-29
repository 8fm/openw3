/**
* Copyright (c) 2013 CD Projekt Red. All Rights Reserved.
*/
#include "redMemoryMetricsSerialiser.h"

namespace Red { namespace MemoryFramework {

//////////////////////////////////////////////////////////////
// WriteDumpHeader_Platform
//	Linux-specific debug data
void MetricsSerialiser::WriteDumpHeader_Platform()
{
	RED_LOG_ERROR(MetricsSerialiser, TXT("FIX_LINUX MetricsSerialiser::WriteDumpHeader_Platform"));
}

} }
