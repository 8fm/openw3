/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "test.h"
#include "helper.h"
#include "environment.h"
#include "memoryInitializer.h"

namespace Red
{
namespace UnitTest
{
	extern void AddLocalTestEnvironment();

	int RunAllTests()
	{
		testing::AddGlobalTestEnvironment( new Red::UnitTest::Environment );
		testing::AddGlobalTestEnvironment( new Red::UnitTest::MemoryInitializer );

		
		AddLocalTestEnvironment();

		return RUN_ALL_TESTS();
	}
}
}
