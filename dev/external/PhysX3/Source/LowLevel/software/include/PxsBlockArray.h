// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright (c) 2008-2013 NVIDIA Corporation. All rights reserved.
// Copyright (c) 2004-2008 AGEIA Technologies, Inc. All rights reserved.
// Copyright (c) 2001-2004 NovodeX AG. All rights reserved.  


#ifndef PXS_BLOCK_ARRAY_H
#define PXS_BLOCK_ARRAY_H

namespace physx
{


template<typename T, int BLOCK_SIZE>
struct PxsBlockArray
{
	struct Block : Ps::UserAllocated { T items[BLOCK_SIZE]; };
	struct BlockInfo
	{
		Block* block;
		PxU32 count; // number of elements in this block
		BlockInfo(Block* aBlock, PxU32 aCount) : block(aBlock), count(aCount) {}
	};
	Ps::InlineArray<BlockInfo,4> blocks;

	PxsBlockArray()
	{
		blocks.pushBack(BlockInfo(PX_NEW(Block), 0));
	}

	~PxsBlockArray()
	{
		for (PxU32 i = 0; i < blocks.size(); i++)
		{
			PX_DELETE(blocks[i].block);
		}
	}

	void clear()
	{
		 // at least one block is expected to always be present in the array
		PX_ASSERT(blocks.size());

		// delete all but the first block
		for (PxU32 i = 1; i < blocks.size(); i++)
		{
			PX_DELETE(blocks[i].block);
		}
		
		blocks.resizeUninitialized(1);

		// destroy any remaining items in the first block
		blocks[0].count = 0;
		blocks[0].block->~Block();
		PX_PLACEMENT_NEW(blocks[0].block, Block);
	}

	T& pushBack()
	{
		PxU32 numBlocks = blocks.size();
		if (blocks[numBlocks-1].count == BLOCK_SIZE)
		{
			blocks.pushBack(BlockInfo(PX_NEW(Block), 0));
			numBlocks ++;
		}
		const PxU32 count = blocks[numBlocks-1].count ++;

		return blocks[numBlocks-1].block->items[count];
	}

	T& pushBack(const T& data)
	{
		PxU32 numBlocks = blocks.size();
		if (blocks[numBlocks-1].count == BLOCK_SIZE)
		{
			blocks.pushBack(BlockInfo(PX_NEW(Block), 0));
			numBlocks ++;
		}
		const PxU32 count = blocks[numBlocks-1].count ++;
		blocks[numBlocks-1].block->items[count] = data;
		return blocks[numBlocks-1].block->items[count];
	}

	void popBack()
	{
		PxU32 numBlocks = blocks.size();
		PX_ASSERT(blocks[numBlocks-1].count > 0);
		if (blocks[numBlocks-1].count > 1 || numBlocks == 1)
			blocks[numBlocks-1].count --;
		else
		{
			PX_DELETE(blocks[numBlocks-1].block);
			blocks.popBack();
		}
	}

	PxU32 size() const
	{
		return (blocks.size()-1)*BLOCK_SIZE + blocks[blocks.size()-1].count;
	}

	T& operator[] (PxU32 index) const
	{
		PX_ASSERT(index/BLOCK_SIZE < blocks.size());
		PX_ASSERT(index%BLOCK_SIZE < blocks[index/BLOCK_SIZE].count);
		return blocks[index/BLOCK_SIZE].block->items[index%BLOCK_SIZE];
	}
};


} // namespace physx

#endif
