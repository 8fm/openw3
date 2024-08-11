/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#ifndef UNITTEST_FRAMEWORK_TEST_H_
#define UNITTEST_FRAMEWORK_TEST_H_

#include "../../common/redSystem/architecture.h"

#ifndef _HAS_EXCEPTIONS 
#define _HAS_EXCEPTIONS 0
#endif // !_HAS_EXCEPTIONS 

#ifndef _SECURE_SCL
#define _SECURE_SCL 0
#endif

#define _SCL_SECURE_NO_WARNINGS

#define GTEST_HAS_SEH 0
#define GTEST_HAS_EXCEPTIONS 0
#define GMOCK_HAS_OVERRIDE 1

#if defined( _DURANGO )
	#define GTEST_HAS_STREAM_REDIRECTION 0
	#define _VARIADIC_MAX 10
	#pragma warning( disable : 4702 )
	#include <xlocale>
#elif defined( __ORBIS__ )
	#define GTEST_HAS_STREAM_REDIRECTION 0	
	#define GTEST_HAS_POSIX_RE 0
	#define GTEST_HAS_DEATH_TEST 0
#elif ( _MSC_VER == 1700 )
	#define _VARIADIC_MAX 10
	#pragma warning( disable : 4702 )
	#include <xlocale>
#endif

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#endif
