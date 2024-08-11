/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "unitTestMode.h"

namespace Red
{
namespace System
{
	static bool unitTestMode = false;

	bool UnitTestMode()
	{
		return unitTestMode;
	}

	void SetUnitTestMode()
	{
		unitTestMode = true;
	}
}
}
