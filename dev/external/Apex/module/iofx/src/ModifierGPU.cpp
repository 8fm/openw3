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

#include "Modifier.h"

#if defined(APEX_CUDA_SUPPORT)

#include "ApexCudaWrapper.h"

#include "ModifierData.h"

#define MODIFIER_DECL
#define CURVE_TYPE physx::apex::iofx::Curve
#define EVAL_CURVE(curve, value) 0
#define PARAMS_NAME(name) name ## ParamsGPU

#include "ModifierSrc.h"

#undef MODIFIER_DECL
#undef CURVE_TYPE
#undef EVAL_CURVE
//#undef PARAMS_NAME

namespace physx
{
namespace apex
{
namespace iofx
{


class ModifierParamsMapperGPU_Adapter
{
private:
	ModifierParamsMapperGPU_Adapter& operator=(const ModifierParamsMapperGPU_Adapter&);

	ModifierParamsMapperGPU& _mapper;
	InplaceStorage& _storage;
	physx::PxU8* _params;

public:
	ModifierParamsMapperGPU_Adapter(ModifierParamsMapperGPU& mapper)
		: _mapper(mapper), _storage(mapper.getStorage()), _params(0) {}

	PX_INLINE InplaceStorage& getStorage()
	{
		return _storage;
	}

	PX_INLINE void beginParams(void* params, size_t , size_t , physx::PxU32)
	{
		_params = (physx::PxU8*)params;
	}
	PX_INLINE void endParams()
	{
		_params = 0;
	}

	template <typename T>
	PX_INLINE void mapValue(size_t offset, T value)
	{
		PX_ASSERT(_params != 0);
		*(T*)(_params + offset) = value;
	}

	PX_INLINE void mapCurve(size_t offset, const NxCurve* nxCurve)
	{
		PX_ASSERT(_params != 0);
		Curve& curve = *(Curve*)(_params + offset);

		physx::PxU32 numPoints;
		const NxVec2R* nxPoints = nxCurve->getControlPoints(numPoints);

		curve.create(_storage, numPoints);
		CurvePoint* points = curve.getPoints(_storage);

		for (physx::PxU32 i = 0; i < numPoints; ++i)
		{
			const NxVec2R& nxPoint = nxPoints[i];
			points[i] = CurvePoint(nxPoint.x, nxPoint.y);
		}
	}
};

#define _MODIFIER(name) \
	void name ## Modifier :: mapParamsGPU(ModifierParamsMapperGPU& mapper) const \
	{ \
		ModifierParamsMapperGPU_Adapter adapter(mapper); \
		InplaceHandle< PARAMS_NAME(name) > paramsHandle; \
		PARAMS_NAME(name)* params = paramsHandle.alloc( adapter.getStorage() ); \
		mapParams( adapter, params ); \
		mapper.onParams( paramsHandle, PARAMS_NAME(name)::RANDOM_COUNT ); \
	} \
	 
#include "ModifierList.h"

}
}
} // namespace physx::apex

#endif
