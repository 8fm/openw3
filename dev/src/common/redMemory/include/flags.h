/**
* Copyright © 2016 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_INCLUDE_FLAGS_H_
#define _RED_MEMORY_INCLUDE_FLAGS_H_

#include "redMemoryInternal.h"
#include "utils.h"

namespace red
{
namespace memory
{
	enum Flags : u32
	{
		Flags_CPU_Read = RED_FLAGS( 0 ),
		Flags_CPU_Write = RED_FLAGS( 1 ),
		Flags_GPU_Read = RED_FLAGS( 2 ),
		Flags_GPU_Write = RED_FLAGS( 3 ),
		Flags_GPU_Coherent = RED_FLAGS( 4 ),

		Flags_Onion_Bus = RED_FLAGS( 5 ),
		Flags_Garlic_Bus = RED_FLAGS( 6 ),

		Flags_CPU_Read_Write = Flags_CPU_Read | Flags_CPU_Write,
		Flags_GPU_Read_Write = Flags_GPU_Read | Flags_GPU_Write
	};
}
}

#endif
