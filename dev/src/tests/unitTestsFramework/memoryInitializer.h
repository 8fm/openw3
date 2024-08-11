/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "test.h"

namespace Red
{
namespace UnitTest
{
	class MemoryInitializer : public testing::Environment
	{
	public:

		virtual void SetUp() override final;
		virtual void TearDown() override final;
	
	private:
	};
}
}
