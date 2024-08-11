/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_OUT_OF_MEMORY_HANDLER_H_
#define _RED_MEMORY_OUT_OF_MEMORY_HANDLER_H_

#include "../include/poolTypes.h"

namespace red
{
namespace memory
{
	class PoolRegistry;

	class RED_MEMORY_API OOMHandler
	{
	public:
		
		OOMHandler();

		RED_MOCKABLE void HandleAllocateFailure( PoolHandle poolHandle, u32 size, u32 alignment );
	
		void SetPoolRegistry( const PoolRegistry * registry );

	protected:

		~OOMHandler();

	private:

		virtual void OnHandleAllocateFailure( const char * poolName, u32 size, u32 alignment ) = 0;

		CMutex m_monitor; // Only one thread at a time can be handled.
		const PoolRegistry * m_poolRegistry;
		bool m_isHandlingFailure; // Making sure no recursion.
	};
}
}

#endif
