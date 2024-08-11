/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "foliageMocks.h"

class FoliageResourceFixture : public ::testing::Test
{
public:

	~FoliageResourceFixture()
	{
		resource.Discard();
	}

	CFoliageResource resource;
};
