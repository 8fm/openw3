/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "oomHandlerIgnore.h"

namespace red
{
namespace memory
{
	OOMHandlerIgnore::OOMHandlerIgnore()
	{}

	OOMHandlerIgnore::~OOMHandlerIgnore()
	{}

	void OOMHandlerIgnore::OnHandleAllocateFailure( const char * , u32 , u32  )
	{
		// Nothing to do, move along. User is handling null ptr that will be returned.
	}
}
}
