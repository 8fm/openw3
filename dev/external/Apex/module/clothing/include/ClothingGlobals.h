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

#ifndef CLOTHING_GLOBALS_H
#define CLOTHING_GLOBALS_H

#include "NxApexUsingNamespace.h"

namespace physx
{
namespace apex
{
namespace clothing
{


struct ClothingConstants
{
	enum Enum
	{
		ImmediateClothingInSkinFlag =		0x80000000, // only highest (sign?) bit. The rest is the index in the clothSkinMapB
		ImmediateClothingInvertNormal =		0x40000000, // if second highest bit is set, invert the normal from NxCloth
		ImmediateClothingBadNormal =		0x20000000, // the normal is neither correct nor inverted, just different, use mesh-mesh skinning from neighboring triangles
		ImmediateClothingInvalidValue =		0x1fffffff, // the lowest bit is set, all others are maxed out
		ImmediateClothingReadMask =			0x0fffffff, // read mask, use this to read the number (two flags can still be put there so far)
	};
};

}
} // namespace apex
} // namespace physx


#endif // CLOTHING_GLOBALS_H
