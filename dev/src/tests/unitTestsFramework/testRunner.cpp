/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "test.h"
#include "helper.h"

#ifdef RED_PLATFORM_WINPC
#pragma comment (lib, "Winmm.lib")
#endif

#ifndef RED_PLATFORM_DURANGO
int main(int argc, char **argv) 
{
	testing::InitGoogleTest(&argc, argv);
	return Red::UnitTest::RunAllTests();

}

#endif
