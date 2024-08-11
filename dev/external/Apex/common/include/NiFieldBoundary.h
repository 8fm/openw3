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

#ifndef __NI_FIELD_BOUNDARY_H__
#define __NI_FIELD_BOUNDARY_H__

#include "InplaceTypes.h"

namespace physx
{
namespace apex
{


struct NiFieldShapeType
{
	enum Enum
	{
		NONE = 0,
		SPHERE,
		BOX,
		CAPSULE,

		FORCE_DWORD = 0xFFFFFFFFu
	};
};

#ifndef __CUDACC__
template <> struct InplaceTypeTraits<NiFieldShapeType::Enum>
{
	enum { hasReflect = false };
};
#endif

struct NiFieldShapeDesc
{
	NiFieldShapeType::Enum		type;
	physx::PxMat34Legacy		worldToShape;
	//dimensions for
	//SPHERE: x = radius
	//BOX:    (x,y,z) = 1/2 size
	//CAPUSE: x = radius, y = height
	physx::PxVec3				dimensions;

	physx::PxF32				weight; //0..1

#ifndef __CUDACC__
	template <typename R>
	void reflect(R& r)
	{
		r.reflect(type);
		r.reflect(worldToShape);
		r.reflect(dimensions);
		r.reflect(weight);
	}
#endif
};

#ifndef __CUDACC__

struct NiFieldBoundaryDesc
{
	NxGroupsMask64	boundaryFilterData;
};

class NiFieldBoundary
{
public:
	virtual bool updateFieldBoundary(physx::Array<NiFieldShapeDesc>& shapes) = 0;

protected:
	virtual ~NiFieldBoundary() {}
};
#endif

}
} // end namespace physx::apex

#endif // #ifndef __NI_FIELD_BOUNDARY_H__
