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

#ifndef APEX_PERMUTE_H
#define APEX_PERMUTE_H

namespace physx
{
namespace apex
{

// permutationBuffer has to contain the indices that map from the new to the old index
template<class Sortable>
inline void ApexPermute(Sortable* sortBuffer, const PxU32* permutationBuffer, PxU32 numElements, PxU32 numElementsPerPermutation = 1)
{
	shdfnd::Array<Sortable> temp;
	temp.resize(numElementsPerPermutation);

	// TODO remove used buffer
	shdfnd::Array<bool> used(numElements, false);

	for (PxU32 i = 0; i < numElements; i++)
	{
		//if (permutationBuffer[i] == (PxU32)-1 || permutationBuffer[i] == i)
		if (used[i] || permutationBuffer[i] == i)
		{
			continue;
		}

		PxU32 dst = i;
		PxU32 src = permutationBuffer[i];
		for (PxU32 j = 0; j < numElementsPerPermutation; j++)
		{
			temp[j] = sortBuffer[numElementsPerPermutation * dst + j];
		}
		do
		{
			for (PxU32 j = 0; j < numElementsPerPermutation; j++)
			{
				sortBuffer[numElementsPerPermutation * dst + j] = sortBuffer[numElementsPerPermutation * src + j];
			}
			//permutationBuffer[dst] = (PxU32)-1;
			used[dst] = true;
			dst = src;
			src = permutationBuffer[src];
			//} while (permutationBuffer[src] != (PxU32)-1);
		}
		while (!used[src]);
		for (PxU32 j = 0; j < numElementsPerPermutation; j++)
		{
			sortBuffer[numElementsPerPermutation * dst + j] = temp[j];
		}
		//permutationBuffer[dst] = (PxU32)-1;
		used[dst] = true;
	}
}

} // namespace apex
} // namespace physx

#endif // APEX_PERMUTE_H
