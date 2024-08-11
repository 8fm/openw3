/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "hookMarkBlock.h"
#include "hookTypes.h"
#include "hookSettings.h"
#include "utils.h"
#include "../../redSystem/unitTestMode.h"


namespace red
{
namespace memory
{
	void MarkFreeBlock( HookPreParameter & param, void* )
	{
		if( !UnitTestMode() ) // block are not marked in unit test ... unitTestMemory can't work with it on. 
		{	
			if( !*param.size )
			{
				MemsetBlock( *param.block, c_markFreeMemoryPattern );
			}
			else
			{ /* If no block but size, we are in an allocate or reallocate method. */ }	
		}
	}

	void MarkAllocatedBlock( HookPostParameter & param, void*  )
	{
		if( !UnitTestMode() ) // block are not marked in unit test ... unitTestMemory can't work with it on.  
		{
			Block & input = *param.inputBlock;
			Block & output = *param.outputBlock;

			if( !input.address )
			{
				MemsetBlock( output, c_markAllocateMemoryPattern );
			}
			else if( input.size < output.size )
			{
				const u64 size = output.size - input.size;
				const u64 address = output.address + input.size;
				MemsetBlock( address, c_markReallocateMemoryPattern, size );
			}
		}
	}
}
}
