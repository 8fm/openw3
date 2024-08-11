/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageMocks.h"

CFoliageBrokerMock::CFoliageBrokerMock()
{}

CFoliageBrokerMock::~CFoliageBrokerMock()
{}

CFoliageGridMock::CFoliageGridMock()
{}

CFoliageGridMock::~CFoliageGridMock()
{}

CFoliageCellMock::CFoliageCellMock()
{}

CFoliageCellMock::~CFoliageCellMock()
{
	Die();
}

CFoliageResourceLoaderMock::CFoliageResourceLoaderMock()
{}

CFoliageResourceLoaderMock::~CFoliageResourceLoaderMock()
{}

CFoliageResourceHandlerMock::CFoliageResourceHandlerMock()
{}

CFoliageResourceHandlerMock::~CFoliageResourceHandlerMock()
{}

CFoliageResourceMock::CFoliageResourceMock()
{}

CFoliageResourceMock::~CFoliageResourceMock()
{
	SetFlag( OF_Finalized );
}

CSRTBaseTreeMock::CSRTBaseTreeMock()
{}

CSRTBaseTreeMock::~CSRTBaseTreeMock()
{
	SetFlag( OF_Finalized );
}

CFoliageCollisionHandlerMock::CFoliageCollisionHandlerMock()
{}

CFoliageCollisionHandlerMock::~CFoliageCollisionHandlerMock()
{}

CFoliageRenderCommandDispatcherMock::CFoliageRenderCommandDispatcherMock()
{}

CFoliageRenderCommandDispatcherMock::~CFoliageRenderCommandDispatcherMock()
{}


