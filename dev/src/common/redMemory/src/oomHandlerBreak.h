/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_OUT_OF_MEMORY_HANDLER_BREAK_H_
#define _RED_MEMORY_OUT_OF_MEMORY_HANDLER_BREAK_H_

#include "oomHandler.h"

namespace red
{
namespace memory
{
	class Reporter;

	class OOMHandlerBreak : public OOMHandler
	{
	public:
		OOMHandlerBreak();
		~OOMHandlerBreak();

		void Initialize( const Reporter * reporter );

	private:

		virtual void OnHandleAllocateFailure( const char * poolName, u32 size, u32 alignment ) override final;
	
		const Reporter * m_reporter;
	};

}
}

#endif
