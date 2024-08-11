/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "tlsfBlock.h"

namespace red
{
namespace memory
{
	// constant assume those size ! see tlsfconstant.h
	static_assert( sizeof( TLSFBlockHeader ) == 32, "TLSF Block need to be of size 32." );
	static_assert( sizeof( TLSFFreeBlockData ) == 16, "TLSF Free Block need to be of size 16." );
}
}
