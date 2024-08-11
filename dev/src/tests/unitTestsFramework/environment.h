/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "test.h"
#include "../../common/redSystem/architecture.h"
#include "../../common/redSystem/os.h"

namespace Red
{
namespace UnitTest
{
	class Environment : public testing::Environment
	{
	public:
		virtual void SetUp() override;
		virtual void TearDown() override;
	
	private:

#ifdef RED_PLATFORM_WINPC
		LPTOP_LEVEL_EXCEPTION_FILTER m_exceptionHandler;
#endif
	};
}
}
