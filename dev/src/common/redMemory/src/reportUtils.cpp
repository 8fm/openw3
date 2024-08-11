/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../include/reportUtils.h"
#include "vault.h"

namespace red
{
namespace memory
{
	void LogFullReport()
	{
		AcquireVault().LogMemoryReport();
	}
}
}
