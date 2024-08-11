/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_THREAD_ID_PROVIDER_H_
#define _RED_MEMORY_THREAD_ID_PROVIDER_H_

namespace red
{
namespace memory
{
	typedef u32 ThreadId;

	class RED_MEMORY_API ThreadIdProvider
	{
	public:
		RED_MOCKABLE ~ThreadIdProvider();

		RED_MOCKABLE ThreadId GetCurrentId() const;
	};
}
}

#include "threadIdProvider.hpp"

#endif
