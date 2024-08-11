/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RED_MEMORY_TLSF_CONTANT_H_
#define _RED_MEMORY_TLSF_CONTANT_H_

namespace red
{
namespace memory
{
	// There is no point splitting small allocation sizes into multiple 
	// first-level lists as the size granularity is too low
	// This constant controls the minimum size to start segregating
	// (Any allocations small than this go into the FLI 0)
	// Note this must be a power of 2 (as this is only for the first level)
	const u32 c_tlsfSmallestSegregatedSizeLog2 = 7;		// Log(128) = 7
	const u32 c_tlsfSmallestSegregatedSize = (1 << c_tlsfSmallestSegregatedSizeLog2);

	// Maximum SLI (Second level index) width (log2). 5 = max of 32 splits
	const u32 c_tlsfMaximumSecondLevelLog2 = 5;
	const u32 c_tlsfMaximumSecondLevelDivisions = (1 << c_tlsfMaximumSecondLevelLog2);

	const u32 c_tlsfDefaultAlignment = 16;
	const u32 c_tlsfMinimumAlignment = 8;
	const u64 c_tlsfBlockHeaderAlignment = 16;
	const u32 c_tlsfSmallestAllocSize = 16;

	const u32 c_tlsfUsedBlockHeaderSize = 16;
	const u32 c_tlsfFreeBlockHeaderSize = 32;

	// Block status masks (first 2 bits)
	const u32 c_tlsfBlockFreeStatusMask = 0x1;					// Used / free
	const u32 c_tlsfBlockPrevStateMask = 0x2;					// Previous physical block used / free
	const u32 c_tlsfBlockStatusMask = c_tlsfBlockFreeStatusMask | c_tlsfBlockPrevStateMask;		// Full mask

	// Block free status
	const u32 c_tlsfBlockIsFree = 0x1;
	const u32 c_tlsfBlockIsUsed = 0x0;

	// Previous physical block status
	const u32 c_tlsfPreviousBlockIsFree = 0x2;
	const u32 c_tlsfPreviousBlockIsUsed = 0x0;

	const u8 c_tlsfUnitTestAllocFiller = 0xa2;
	const u8 c_tlsfUnitTestFreeFiller = 0xf2;
}
};

#endif
