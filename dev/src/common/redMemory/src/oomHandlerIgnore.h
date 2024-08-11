/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_OUT_OF_MEMORY_HANDLER_IGNORE_H_
#define _RED_MEMORY_OUT_OF_MEMORY_HANDLER_IGNORE_H_

#include "oomHandler.h"

namespace red
{
namespace memory
{
	class OOMHandlerIgnore : public OOMHandler
	{
	public:

		OOMHandlerIgnore();
		~OOMHandlerIgnore();

	private:

		virtual void OnHandleAllocateFailure( const char * poolName, u32 size, u32 alignment ) override final;
	};

}
}

#endif
