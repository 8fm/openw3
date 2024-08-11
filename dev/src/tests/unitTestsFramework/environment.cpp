/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "environment.h"
#include "../../common/redSystem/error.h"
#include "../../common/core/memory.h"
#include "../../common/redSystem/unitTestMode.h"

namespace Red
{
namespace UnitTest
{
#ifdef RED_PLATFORM_WINPC
	LONG WINAPI HandleException( EXCEPTION_POINTERS* )
	{
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif

	void Environment::SetUp()
	{
#ifdef RED_PLATFORM_WINPC
		System::Error::Handler::GetInstance();
		m_exceptionHandler = SetUnhandledExceptionFilter( &HandleException );
#endif
		Red::System::SetUnitTestMode();
	}

	void Environment::TearDown()
	{
#ifdef RED_PLATFORM_WINPC
		SetUnhandledExceptionFilter(m_exceptionHandler);
#endif
	}
}
}
